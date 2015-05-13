/*

  Copyright (c) 2015 Martin Sustrik

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom
  the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.

*/

#include <assert.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>

#include "tcp.h"
#include "mill.h"

#define MILL_TCP_LISTEN_BACKLOG 10

struct tcplistener {
    int fd;
};

struct tcpconn {
    int fd;
};	

tcplistener tcplisten(const struct sockaddr *addr, socklen_t addrlen) {
    /* Open the listening socket. */
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if(s == -1)
        return NULL;

    /* Make it non-blocking. */
    int opt = fcntl(s, F_GETFL, 0);
    if (opt == -1)
        opt = 0;
    int rc = fcntl(s, F_SETFL, opt | O_NONBLOCK);
    assert(rc != -1);

    /* Start listening. */
    rc = bind(s, addr, addrlen);
    assert(rc != -1);
    rc = listen(s, MILL_TCP_LISTEN_BACKLOG);
    assert(rc != -1);

    /* Create the object. */
    struct tcplistener *listener = malloc(sizeof(struct tcplistener));
    assert(listener);
    listener->fd = s;
    return listener;
}

tcpconn tcpaccept(tcplistener listener) {
	while(1) {
        /* Try to get new connection (non-blocking). */
        int s = accept(listener->fd, NULL, NULL);
        if (s >= 0) {
            /* Put the newly created connection into non-blocking mode. */
            int opt = fcntl(s, F_GETFL, 0);
            if (opt == -1)
                opt = 0;
            int rc = fcntl(s, F_SETFL, opt | O_NONBLOCK);
            assert(rc != -1);

            /* Create the object. */
			struct tcpconn *conn = malloc(sizeof(struct tcpconn));
			assert(conn);
			conn->fd = s;
			return conn;
        }
        assert(s == -1);
        if(errno != EAGAIN && errno != EWOULDBLOCK)
            return NULL;
        /* Wait till new connection is available. */
        int rc = fdwait(listener->fd, FDW_IN);
        assert(rc == FDW_IN);
    }
}

void tcplistener_close(tcplistener listener) {
    int rc = close(listener->fd);
    assert(rc == 0);
    free(listener);
}

tcpconn tcpdial(const struct sockaddr *addr, socklen_t addrlen) {
    /* Open a socket. */
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if(s == -1)
        return NULL;

    /* Make it non-blocking. */
    int opt = fcntl(s, F_GETFL, 0);
    if (opt == -1)
        opt = 0;
    int rc = fcntl(s, F_SETFL, opt | O_NONBLOCK);
    assert(rc != -1);

    /* Connect to the remote endpoint. */
    rc = connect(s, addr, addrlen);
    if(rc != 0) {
		assert(rc == -1);
		if(errno != EINPROGRESS)
		    return NULL;
		rc = fdwait(s, FDW_OUT);
        assert(rc == FDW_OUT);
		/* TODO: Handle errors. */
    }

    /* Create the object. */
	struct tcpconn *conn = malloc(sizeof(struct tcpconn));
	assert(conn);
	conn->fd = s;
	return conn;
}

void tcpconn_close(tcpconn conn) {
    int rc = close(conn->fd);
    assert(rc == 0);
    free(conn);
}

ssize_t tcpwrite(tcpconn conn, const void *buf, size_t sz) {
    assert(0);
}

ssize_t tcpread(tcpconn conn, void *buf, size_t sz,
      const char *delim, size_t delimsz) {
    assert(0);
}

#if 0

-int msocket(int family, int type, int protocol) {
-    int s = socket(family, type, protocol);
-    if(s == -1)
-        return -1;
-    int opt = fcntl(s, F_GETFL, 0);
-    if (opt == -1)
-        opt = 0;
-    int rc = fcntl(s, F_SETFL, opt | O_NONBLOCK);
-    assert(rc != -1);
-    return s;
-}
-
-int mconnect(int s, const struct sockaddr *addr, socklen_t addrlen) {
-    int rc = connect(s, addr, addrlen);
-    if(rc == 0)
-        return 0;
-    assert(rc == -1);
-    if(errno != EINPROGRESS)
-        return -1;
-    mill_wait(s, POLLOUT);
-    /* TODO: Handle errors. */
-    return 0;
-}
-
-int maccept(int s, struct sockaddr *addr, socklen_t *addrlen) {
-    while(1) {
-        int newsock = accept(s, addr, addrlen);
-        if (newsock >= 0) {
-            int opt = fcntl(newsock, F_GETFL, 0);
-            if (opt == -1)
-                opt = 0;
-            int rc = fcntl(newsock, F_SETFL, opt | O_NONBLOCK);
-            assert(rc != -1);
-            return newsock;
-        }
-        assert(newsock == -1);
-        if(errno != EAGAIN && errno != EWOULDBLOCK)
-            return -1;
-        mill_wait(s, POLLIN);
-    }
-}
-
-ssize_t msend(int s, const void *buf, size_t len, int flags) {
-    char *pos = (char*)buf;
-    size_t remaining = len;
-    while(remaining) {
-        ssize_t sz = send(s, pos, remaining, flags);
-        if(sz == -1) {
-            if(errno != EAGAIN && errno != EWOULDBLOCK)
-                return -1;
-            mill_wait(s, POLLOUT);
-            continue;
-        }
-        pos += sz;
-        remaining -= sz;
-    }
-    return len;
-}
-
-ssize_t mrecv(int s, void *buf, size_t len, int flags) {
-    char *pos = (char*)buf;
-    size_t remaining = len;
-    while(remaining) {
-        ssize_t sz = recv(s, pos, remaining, flags);
-        if(sz == 0)
-            return len - remaining;
-        if(sz == -1) {
-            if(errno != EAGAIN && errno != EWOULDBLOCK)
-                return -1;
-            mill_wait(s, POLLIN);
-            continue;
-        }
-        pos += sz;
-        remaining -= sz;
-    }
-    return len;
-}

#endif

