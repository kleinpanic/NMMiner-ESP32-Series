#pragma once
#include <errno.h>

// Arduino-ESP32 2.x probes connected() with recv(..., 0). A successful
// zero-length receive need not change errno, so a preceding client's reset
// can otherwise reject the next healthy request. A real receive error still
// replaces this initial value and is handled normally by the SDK.
template<class Server>
void handleSocketServerPass(Server& server) {
    errno = EWOULDBLOCK;
    server.handleClient();
}
