#include "websocket.h"
#include "webconfig_websocket.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "lwip/mem.h"
#include "mbedtls/sha1.h"
#include "base64.h"
#include "pico/time.h"
#include <string.h>
#include <stdlib.h>

// Static array to store active WebSocket connections
#define MAX_WS_CONNECTIONS 4
static ws_connection_t* active_connections[MAX_WS_CONNECTIONS] = {NULL};

// Find free connection slot
static int find_free_connection_slot(void) {
    for (int i = 0; i < MAX_WS_CONNECTIONS; i++) {
        if (active_connections[i] == NULL) {
            return i;
        }
    }
    return -1;
}

// Remove connection from active list
static void remove_connection(ws_connection_t* conn) {
    for (int i = 0; i < MAX_WS_CONNECTIONS; i++) {
        if (active_connections[i] == conn) {
            active_connections[i] = NULL;
            break;
        }
    }
}

// Generate WebSocket accept key
static void generate_websocket_accept_key(const char* websocket_key, char* accept_key) {
    char combined[256];
    snprintf(combined, sizeof(combined), "%s%s", websocket_key, WS_MAGIC_STRING);

    // Calculate SHA1 hash
    mbedtls_sha1_context sha1_state;
    uint8_t hash[20];
    mbedtls_sha1_init(&sha1_state);
    mbedtls_sha1_starts(&sha1_state);
    mbedtls_sha1_update(&sha1_state, (uint8_t*)combined, strlen(combined));
    mbedtls_sha1_finish(&sha1_state, hash);
    mbedtls_sha1_free(&sha1_state);

    // Base64 encode the hash
    std::string encoded = Base64::Encode((char*)hash, 20);
    strcpy(accept_key, encoded.c_str());
}

// Parse WebSocket frame
static int parse_websocket_frame(const uint8_t* data, size_t len, ws_frame_t* frame) {
    if (len < 2) return -1;

    frame->fin = (data[0] & 0x80) >> 7;
    frame->opcode = data[0] & 0x0F;
    frame->masked = (data[1] & 0x80) >> 7;

    uint64_t payload_len = data[1] & 0x7F;
    size_t header_len = 2;

    if (payload_len == 126) {
        if (len < 4) return -1;
        payload_len = (data[2] << 8) | data[3];
        header_len = 4;
    } else if (payload_len == 127) {
        if (len < 10) return -1;
        payload_len = 0;
        for (int i = 2; i < 10; i++) {
            payload_len = (payload_len << 8) | data[i];
        }
        header_len = 10;
    }

    if (frame->masked) {
        if (len < header_len + 4) return -1;
        memcpy(frame->mask, data + header_len, 4);
        header_len += 4;
    }

    if (len < header_len + payload_len) return -1;

    frame->payload_len = payload_len;
    frame->payload = (uint8_t*)malloc(payload_len + 1);
    if (!frame->payload) return -1;

    memcpy(frame->payload, data + header_len, payload_len);

    // Unmask payload if needed
    if (frame->masked) {
        for (uint64_t i = 0; i < payload_len; i++) {
            frame->payload[i] ^= frame->mask[i % 4];
        }
    }

    frame->payload[payload_len] = '\0'; // Null terminate for text frames

    return header_len + payload_len;
}

// Create WebSocket frame
static int create_websocket_frame(uint8_t opcode, const uint8_t* payload, size_t payload_len, uint8_t** frame, size_t* frame_len) {
    size_t header_len = 2;

    if (payload_len > 65535) {
        header_len = 10;
    } else if (payload_len > 125) {
        header_len = 4;
    }

    *frame_len = header_len + payload_len;
    *frame = (uint8_t*)malloc(*frame_len);
    if (!*frame) return -1;

    (*frame)[0] = 0x80 | opcode; // FIN=1, opcode

    if (payload_len > 65535) {
        (*frame)[1] = 127;
        for (int i = 0; i < 8; i++) {
            (*frame)[9 - i] = (payload_len >> (i * 8)) & 0xFF;
        }
    } else if (payload_len > 125) {
        (*frame)[1] = 126;
        (*frame)[2] = (payload_len >> 8) & 0xFF;
        (*frame)[3] = payload_len & 0xFF;
    } else {
        (*frame)[1] = payload_len;
    }

    memcpy(*frame + header_len, payload, payload_len);

    return 0;
}

// WebSocket receive callback
err_t websocket_recv(void* arg, struct tcp_pcb* pcb, struct pbuf* p, err_t err) {
    ws_connection_t* conn = (ws_connection_t*)arg;

    if (err != ERR_OK || p == NULL) {
        if (conn->close_callback) {
            conn->close_callback(conn);
        }
        return ERR_OK;
    }

    // Copy data to receive buffer
    size_t total_len = p->tot_len;
    if (conn->receive_buffer_pos + total_len > conn->receive_buffer_size) {
        // Resize buffer if needed
        conn->receive_buffer_size = conn->receive_buffer_pos + total_len + 1024;
        conn->receive_buffer = (char*)realloc(conn->receive_buffer, conn->receive_buffer_size);
        if (!conn->receive_buffer) {
            pbuf_free(p);
            return ERR_MEM;
        }
    }

    pbuf_copy_partial(p, conn->receive_buffer + conn->receive_buffer_pos, total_len, 0);
    conn->receive_buffer_pos += total_len;

    // Try to parse complete frames
    size_t processed = 0;
    while (processed < conn->receive_buffer_pos) {
        ws_frame_t frame;
        int frame_len = parse_websocket_frame((uint8_t*)(conn->receive_buffer + processed),
                                            conn->receive_buffer_pos - processed, &frame);

        if (frame_len < 0) {
            // Incomplete frame, wait for more data
            break;
        }

        // Handle frame based on opcode
        switch (frame.opcode) {
            case WS_OPCODE_TEXT:
                if (conn->message_callback) {
                    conn->message_callback(conn, (char*)frame.payload, frame.payload_len);
                }
                break;

            case WS_OPCODE_BINARY:
                if (conn->message_callback) {
                    conn->message_callback(conn, (char*)frame.payload, frame.payload_len);
                }
                break;

            case WS_OPCODE_CLOSE:
                conn->state = WS_STATE_CLOSING;
                websocket_close(conn);
                break;

            case WS_OPCODE_PING:
                // Send pong response
                websocket_send_binary(conn, frame.payload, frame.payload_len);
                break;

            case WS_OPCODE_PONG:
                // Handle pong (optional)
                break;
        }

        if (frame.payload) {
            free(frame.payload);
        }

        processed += frame_len;
    }

    // Shift remaining data to beginning of buffer
    if (processed > 0) {
        memmove(conn->receive_buffer, conn->receive_buffer + processed,
                conn->receive_buffer_pos - processed);
        conn->receive_buffer_pos -= processed;
    }

    tcp_recved(pcb, total_len);
    pbuf_free(p);

    return ERR_OK;
}

// WebSocket error callback
void websocket_err(void* arg, err_t err) {
    ws_connection_t* conn = (ws_connection_t*)arg;
    if (conn) {
        conn->state = WS_STATE_CLOSED;
        if (conn->close_callback) {
            conn->close_callback(conn);
        }
        remove_connection(conn);
        if (conn->receive_buffer) {
            free(conn->receive_buffer);
        }
        free(conn);
    }
}

// WebSocket poll callback
err_t websocket_poll(void* arg, struct tcp_pcb* pcb) {
    // Send keepalive ping every 30 seconds
    static uint32_t last_ping = 0;
    uint32_t now = to_us_since_boot(get_absolute_time()) / 1000000;

    if (now - last_ping > 30) {
        ws_connection_t* conn = (ws_connection_t*)arg;
        if (conn && conn->state == WS_STATE_OPEN) {
            uint8_t ping_data[] = "ping";
            uint8_t* frame;
            size_t frame_len;
            if (create_websocket_frame(WS_OPCODE_PING, ping_data, 4, &frame, &frame_len) == 0) {
                tcp_write(pcb, frame, frame_len, TCP_WRITE_FLAG_COPY);
                tcp_output(pcb);
                free(frame);
            }
        }
        last_ping = now;
    }

    return ERR_OK;
}

// Initialize WebSocket system
int websocket_init(void) {
    // Clear active connections
    for (int i = 0; i < MAX_WS_CONNECTIONS; i++) {
        active_connections[i] = NULL;
    }
    return 0;
}

// Accept WebSocket connection
ws_connection_t* websocket_accept_connection(struct tcp_pcb* pcb, const char* websocket_key) {
    int slot = find_free_connection_slot();
    if (slot < 0) {
        return NULL; // No free slots
    }

    ws_connection_t* conn = (ws_connection_t*)malloc(sizeof(ws_connection_t));
    if (!conn) {
        return NULL;
    }

    conn->pcb = pcb;
    conn->state = WS_STATE_OPEN;
    conn->receive_buffer_size = 1024;
    conn->receive_buffer = (char*)malloc(conn->receive_buffer_size);
    conn->receive_buffer_pos = 0;
    conn->message_callback = NULL;
    conn->close_callback = NULL;
    conn->user_data = NULL;

    if (!conn->receive_buffer) {
        free(conn);
        return NULL;
    }

    // Set up TCP callbacks
    tcp_arg(pcb, conn);
    tcp_recv(pcb, websocket_recv);
    tcp_err(pcb, websocket_err);
    tcp_poll(pcb, websocket_poll, 4); // Poll every 2 seconds

    // Generate and send handshake response
    char accept_key[64];
    generate_websocket_accept_key(websocket_key, accept_key);

    char response[512];
    snprintf(response, sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n"
        "\r\n", accept_key);

    tcp_write(pcb, response, strlen(response), TCP_WRITE_FLAG_COPY);
    tcp_output(pcb);

    active_connections[slot] = conn;

    return conn;
}

// Send text message
int websocket_send_text(ws_connection_t* conn, const char* message) {
    if (!conn || conn->state != WS_STATE_OPEN) {
        return -1;
    }

    uint8_t* frame;
    size_t frame_len;

    if (create_websocket_frame(WS_OPCODE_TEXT, (uint8_t*)message, strlen(message), &frame, &frame_len) != 0) {
        return -1;
    }

    err_t err = tcp_write(conn->pcb, frame, frame_len, TCP_WRITE_FLAG_COPY);
    tcp_output(conn->pcb);

    free(frame);

    return (err == ERR_OK) ? 0 : -1;
}

// Send binary data
int websocket_send_binary(ws_connection_t* conn, const uint8_t* data, size_t length) {
    if (!conn || conn->state != WS_STATE_OPEN) {
        return -1;
    }

    uint8_t* frame;
    size_t frame_len;

    if (create_websocket_frame(WS_OPCODE_BINARY, data, length, &frame, &frame_len) != 0) {
        return -1;
    }

    err_t err = tcp_write(conn->pcb, frame, frame_len, TCP_WRITE_FLAG_COPY);
    tcp_output(conn->pcb);

    free(frame);

    return (err == ERR_OK) ? 0 : -1;
}

// Close WebSocket connection
int websocket_close(ws_connection_t* conn) {
    if (!conn) {
        return -1;
    }

    if (conn->state == WS_STATE_OPEN) {
        // Send close frame
        uint8_t* frame;
        size_t frame_len;
        uint8_t close_data[2] = {0x03, 0xe8}; // Normal closure

        if (create_websocket_frame(WS_OPCODE_CLOSE, close_data, 2, &frame, &frame_len) == 0) {
            tcp_write(conn->pcb, frame, frame_len, TCP_WRITE_FLAG_COPY);
            tcp_output(conn->pcb);
            free(frame);
        }

        conn->state = WS_STATE_CLOSING;
    }

    if (conn->pcb) {
        tcp_close(conn->pcb);
    }

    remove_connection(conn);

    if (conn->receive_buffer) {
        free(conn->receive_buffer);
    }

    free(conn);

    return 0;
}

// Set message callback
void websocket_set_message_callback(ws_connection_t* conn, void (*callback)(ws_connection_t*, const char*, size_t)) {
    if (conn) {
        conn->message_callback = callback;
    }
}

// Set close callback
void websocket_set_close_callback(ws_connection_t* conn, void (*callback)(ws_connection_t*)) {
    if (conn) {
        conn->close_callback = callback;
    }
}

// Set user data
void websocket_set_user_data(ws_connection_t* conn, void* user_data) {
    if (conn) {
        conn->user_data = user_data;
    }
}

// Get user data
void* websocket_get_user_data(ws_connection_t* conn) {
    return conn ? conn->user_data : NULL;
}
