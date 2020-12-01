/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX Headers */
// inet_pton() is defined in ws2tcpip.h. But for MinGW, it is only
// defined when version of Windows is 7 or higher; namely, when
// _WIN32_WINNT is at least 0x0601.  For some reason, even when this
// requirement is met, this flag is not set (or not set accurately):
// /usr/x86_64-w64-mingw32/sys-root/mingw/include/_mingw.h, line 233
// sets this macro to hard-coded value 0x502; not sure why...
// So set/override _WIN32_WINNT to be 0x0601, so that we get inet_pton()
// from MinGW's ws2tcpip.h.
// Also, do this before including winsock2.h (and possibly the other
// Windows includes below), as e.g. WSAPoll will have a similar issue.
#define OLD_WIN32_WINNT_VAL 0x0000
#ifdef _WIN32_WINNT
#undef OLD_WIN32_WINNT_VAL
#define OLD_WIN32_WINNT_VAL _WIN32_WINNT
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#else
#define _WIN32_WINNT 0x0601
#endif
#include <windows.h>
#include <winsock.h>
#include <winsock2.h>
#include <ws2tcpip.h> // For addrinfo, inet_pton().
#if (OLD_WIN32_WINNT_VAL > 0x0000)
#undef _WIN32_WINNT
#define _WIN32_WINNT OLD_WIN32_WINNT_VAL
#endif
#undef OLD_WIN32_WINNT_VAL

#ifndef FF_POSIX_NET_WINDOWS_H_
#define FF_POSIX_NET_WINDOWS_H_

#ifndef POLLIN
typedef struct pollfd {
  SOCKET fd;
  SHORT events;
  SHORT revents;
} WSAPOLLFD, *PWSAPOLLFD, FAR * LPWSAPOLLFD;
WINSOCK_API_LINKAGE int WSAAPI
WSAPoll(LPWSAPOLLFD fdArray, ULONG fds, INT timeout);
#define POLLIN 0x0300
#endif
#ifndef POLLHUP
#define POLLHUP 0x0002
#endif
#ifndef POLLOUT
#define POLLOUT 0x0010
#endif
#ifndef POLLERR
#define POLLERR 0x0001
#endif

#ifndef SHUT_RDWR
#define SHUT_RD SD_RECEIVE
#define SHUT_WR SD_SEND
#define SHUT_RDWR SD_BOTH
#endif

#define SET_NON_BLOCKING_SOCKET(socket) \
  do { \
    u_long mode = 1; \
    ioctlsocket((socket), FIONBIO, &mode); \
  } while (false)

#define FF_RECV(socket, buf, len, flags) \
  recv((socket), (char *)(buf), (len), (flags))

#define FF_SETSOCKOPT( \
    pollfds, sol_socket, so_reuseaddr, property, value) \
  setsockopt( \
      (pollfds), \
      (sol_socket), \
      (so_reuseaddr), \
      (const char *)(property), \
      (value))

#define FF_POLL(socket, num_sockets, timeout) \
  WSAPoll((socket), (num_sockets), (timeout))

// sock: a socket
// val: a constant integer pointer to 1 (for no delay) or 0 (for yes delay)
// This is currently ignored on windows.
#define FF_SET_NO_DELAY(sock, val) 0

#define ff_pollfd WSAPOLLFD

#define FF_POLLIN POLLIN
#define FF_POLLOUT POLLOUT
#define FF_POLLERR POLLERR
#define FF_POLLHUP POLLHUP

#endif // FF_POSIX_NET_WINDOWS_H_
