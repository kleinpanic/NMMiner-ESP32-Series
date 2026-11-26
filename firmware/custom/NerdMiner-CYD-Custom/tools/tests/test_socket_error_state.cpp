#include <assert.h>
#include <stdio.h>
#include "socketErrorState.h"

// Model the installed SDK's zero-length connected() probe boundary. Success
// leaves errno unchanged; a genuine peer error overwrites it.
struct ProbeServer {
    bool actualReset = false;
    bool accepted = false;
    void handleClient() {
        if (actualReset) errno = ECONNRESET;
        accepted = errno != ECONNRESET && errno != EPIPE && errno != ENOTCONN;
    }
};

int main() {
    ProbeServer server;
    errno = ECONNRESET;
    server.handleClient();
    assert(!server.accepted); // Old unguarded call rejects a healthy client.
    const int staleErrors[] = {ECONNRESET, EPIPE, ENOTCONN, EAGAIN};
    for (int stale : staleErrors) {
        errno = stale;
        handleSocketServerPass(server);
        assert(server.accepted);
    }
    server.actualReset = true;
    handleSocketServerPass(server);
    assert(!server.accepted && errno == ECONNRESET);
    puts("PASS socket error state: stale-client errors cleared, actual receive error retained");
}
