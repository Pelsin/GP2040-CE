#ifndef WEBCONFIG_WEBSOCKET_H
#define WEBCONFIG_WEBSOCKET_H

// Initialize WebSocket server for gamepad monitoring
void init_websocket_server(void);

// Update WebSocket clients with current gamepad state
void update_websocket_clients(void);

#endif // WEBCONFIG_WEBSOCKET_H
