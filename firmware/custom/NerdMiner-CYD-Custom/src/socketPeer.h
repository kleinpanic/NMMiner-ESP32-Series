#pragma once
#include <errno.h>
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
