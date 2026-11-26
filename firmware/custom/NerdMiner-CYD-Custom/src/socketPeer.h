#pragma once
#include <errno.h>
#include <stdint.h>
#ifdef ARDUINO
#include <lwip/sockets.h>
#else
#include <sys/socket.h>
#endif

// Inspect socket errors without consuming data or FIN. An HTTP peer may
// half-close its request while still receiving our response; read EOF alone
// must not cancel that response. Writes and the idle deadline handle it.
inline bool socketPeerOpen(int fd) {
    if (fd < 0) return false;
    int error = 0;
    socklen_t size = sizeof(error);
    if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &size) < 0) error = errno;
    return error == 0 || error == EAGAIN || error == EWOULDBLOCK ||
           error == EINTR || error == ENOMEM || error == ENOBUFS;
}

// Honor lwIP's send low-water mark instead of filling its entire TCP queue.
// No wait and no allocation: a blocked bulk peer gets another loop pass later.
inline bool socketWriteReady(int fd) {
    if (fd < 0 || fd >= FD_SETSIZE) return false;
    fd_set writable;
    FD_ZERO(&writable);
    FD_SET(fd, &writable);
    timeval timeout{0, 0};
    return ::select(fd + 1, nullptr, &writable, nullptr, &timeout) > 0;
}

// Only call after the entire declared response body has been queued. Let a
// Content-Length-aware client close first, avoiding a server TIME_WAIT entry
// that rejects an early reuse of the same client port. Never use read EOF to
// cancel a body still being sent: half-closed requests may keep reading it.
inline bool socketResponseMayClose(int fd, uint32_t elapsedMs) {
    if (fd < 0 || elapsedMs >= 2000) return true;
    char byte;
    const int received = ::recv(fd, &byte, 1, MSG_PEEK | MSG_DONTWAIT);
    if (received >= 0) return received == 0;
    return errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR &&
           errno != ENOMEM && errno != ENOBUFS;
}
