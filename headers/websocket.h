#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_

#include <stdint.h>
#include <stdbool.h>
#include "lwip/tcp.h"

// WebSocket frame opcodes
#define WS_OPCODE_CONTINUATION  0x0
#define WS_OPCODE_TEXT          0x1
#define WS_OPCODE_BINARY        0x2
#define WS_OPCODE_CLOSE         0x8
#define WS_OPCODE_PING          0x9
#define WS_OPCODE_PONG          0xA

// WebSocket magic string for handshake
#define WS_MAGIC_STRING "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

// WebSocket connection states
typedef enum {
    WS_STATE_CONNECTING,
    WS_STATE_OPEN,
    WS_STATE_CLOSING,
    WS_STATE_CLOSED
} ws_state_t;

// WebSocket frame structure
typedef struct {
    uint8_t fin;
    uint8_t opcode;
    uint8_t masked;
    uint64_t payload_len;
    uint8_t mask[4];
    uint8_t* payload;
} ws_frame_t;

// WebSocket connection structure
typedef struct ws_connection {
    struct tcp_pcb* pcb;
    ws_state_t state;
    char* receive_buffer;
    size_t receive_buffer_size;
    size_t receive_buffer_pos;
    void (*message_callback)(struct ws_connection* conn, const char* message, size_t length);
    void (*close_callback)(struct ws_connection* conn);
} ws_connection_t;

// Function prototypes
int websocket_init(void);
ws_connection_t* websocket_accept_connection(struct tcp_pcb* pcb, const char* websocket_key);
int websocket_send_text(ws_connection_t* conn, const char* message);
int websocket_send_binary(ws_connection_t* conn, const uint8_t* data, size_t length);
int websocket_close(ws_connection_t* conn);
void websocket_set_message_callback(ws_connection_t* conn, void (*callback)(ws_connection_t*, const char*, size_t));
void websocket_set_close_callback(ws_connection_t* conn, void (*callback)(ws_connection_t*));

// Internal functions
err_t websocket_recv(void* arg, struct tcp_pcb* pcb, struct pbuf* p, err_t err);
void websocket_err(void* arg, err_t err);
err_t websocket_poll(void* arg, struct tcp_pcb* pcb);


#endif // WEBSOCKET_H_
