#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include "socketPeer.h"

int main() {
    int sockets[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    errno = ECONNRESET; // Real nonblocking receive replaces stale errno.
    assert(socketPeerOpen(sockets[0]));
    assert(send(sockets[1], "x", 1, 0) == 1);
    assert(socketPeerOpen(sockets[0]));
    assert(shutdown(sockets[1], SHUT_WR) == 0);
    assert(socketPeerOpen(sockets[0])); // Preserve unread bytes, even before EOF.
    char byte;
    assert(recv(sockets[0], &byte, 1, 0) == 1 && byte == 'x');
    assert(socketPeerOpen(sockets[0])); // EOF only closes the request direction.
    assert(send(sockets[0], "y", 1, 0) == 1);
    assert(recv(sockets[1], &byte, 1, 0) == 1 && byte == 'y');
    close(sockets[0]);
    close(sockets[1]);
    assert(!socketPeerOpen(sockets[0]));
    assert(!socketPeerOpen(-1));
    puts("PASS socket peer: real idle/data/half-close/closed sockets, stale errno and non-consuming peek");
}
