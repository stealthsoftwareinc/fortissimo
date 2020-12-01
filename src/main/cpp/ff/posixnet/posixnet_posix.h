/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX Headers */
#include <poll.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef FF_POSIX_NET_POSIX_H_
#define FF_POSIX_NET_POSIX_H_

#define SET_NON_BLOCKING_SOCKET(socket) \
  fcntl((socket), F_SETFL, fcntl((socket), F_GETFL, 0) | O_NONBLOCK)

#define FF_RECV(socket, buf, len, flags) \
  recv((socket), (buf), (len), (flags))

#define FF_SETSOCKOPT( \
    pollfds, sol_socket, so_reuseaddr, property, value) \
  setsockopt( \
      (pollfds), (sol_socket), (so_reuseaddr), (property), (value))

#define FF_POLL(socket, num_sockets, timeout) \
  poll((socket), (num_sockets), (timeout))

// sock: a socket
// val: a constant integer pointer to 1 (for no delay) or 0 (for yes delay)
#define FF_SET_NO_DELAY(sock, val) \
  setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (val), sizeof(*(val)))

#define ff_pollfd pollfd

#define FF_POLLIN POLLIN
#define FF_POLLOUT POLLOUT
#define FF_POLLERR POLLERR
#define FF_POLLHUP POLLHUP

#endif // FF_POSIX_NET_POSIX_H_
