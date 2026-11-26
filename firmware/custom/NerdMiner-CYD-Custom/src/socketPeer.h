#pragma once
#include <errno.h>
#ifdef ARDUINO
#include <lwip/sockets.h>
#else
#include <sys/socket.h>
#endif

// Probe real socket errors without consuming request data or allocating an RX
// buffer. Read EOF alone is allowed: an HTTP peer may half-close its request
// while still receiving our response. Writes and the idle deadline handle it.
inline bool socketPeerOpen(int fd) {
    if (fd < 0) return false;
    char byte;
    const int received = ::recv(fd, &byte, 1, MSG_PEEK | MSG_DONTWAIT);
    if (received >= 0) return true;
    return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ||
           errno == ENOMEM || errno == ENOBUFS;
}
