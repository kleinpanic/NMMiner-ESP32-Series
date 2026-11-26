#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include "socketPeer.h"

int main() {
    int sockets[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    errno = ECONNRESET; // A stale errno is independent of SO_ERROR.
    assert(socketPeerOpen(sockets[0]));
    assert(socketWriteReady(sockets[0]));
    assert(send(sockets[1], "x", 1, 0) == 1);
    assert(socketPeerOpen(sockets[0]));
    assert(shutdown(sockets[1], SHUT_WR) == 0);
    assert(socketPeerOpen(sockets[0])); // Preserve unread bytes, even before EOF.
    char byte;
    assert(recv(sockets[0], &byte, 1, 0) == 1 && byte == 'x');
    assert(socketPeerOpen(sockets[0])); // EOF only closes the request direction.
    assert(socketPeerOpen(sockets[0])); // Repeated checks must not consume FIN.
    assert(socketWriteReady(sockets[0]));
    assert(send(sockets[0], "y", 1, 0) == 1);
    assert(recv(sockets[1], &byte, 1, 0) == 1 && byte == 'y');
    close(sockets[0]);
    close(sockets[1]);
    assert(!socketPeerOpen(sockets[0]));
    assert(!socketPeerOpen(-1));
    assert(!socketWriteReady(-1));
    assert(!socketWriteReady(FD_SETSIZE));
    int blocked[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, blocked) == 0);
    int budget = 2048;
    assert(setsockopt(blocked[0], SOL_SOCKET, SO_SNDBUF, &budget, sizeof(budget)) == 0);
    char bytes[512]{};
    for (int i = 0; i < 1000 && socketWriteReady(blocked[0]); ++i)
        assert(send(blocked[0], bytes, sizeof(bytes), MSG_DONTWAIT) >= 0);
    assert(!socketWriteReady(blocked[0]));
    close(blocked[0]);
    close(blocked[1]);
    // An actual TCP reset must still release a low-memory retained transfer.
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    assert(listener >= 0);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    assert(bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0);
    socklen_t length = sizeof(address);
    assert(getsockname(listener, reinterpret_cast<sockaddr*>(&address), &length) == 0);
    assert(listen(listener, 1) == 0);
    int peer = socket(AF_INET, SOCK_STREAM, 0);
    assert(peer >= 0);
    assert(connect(peer, reinterpret_cast<sockaddr*>(&address), length) == 0);
    int accepted = accept(listener, nullptr, nullptr);
    assert(accepted >= 0 && socketPeerOpen(accepted));
    linger reset{1, 0};
    assert(setsockopt(peer, SOL_SOCKET, SO_LINGER, &reset, sizeof(reset)) == 0);
    close(peer);
    bool usable = true;
    for (int i = 0; i < 1000 && usable; ++i) {
        usable = socketPeerOpen(accepted);
        if (usable) usleep(1000);
    }
    assert(!usable);
    close(accepted);
    close(listener);
    puts("PASS socket peer: real idle/data/half-close/closed/reset/blocked sockets, stale errno and error-only probe");
}
