/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/*=========================================================================
 * KVM
 *=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: networking (Generic Connection framework)
 * FILE:      socketProtocol.c
 * OVERVIEW:  This file provides a default implementation of the native
 *            functions that are needed for supporting the "socket:" and
 *            "serversocket:" Generic Connection protocols.
 * AUTHOR(s): Nik Shaylor, Efren Serra
 */

/**
 * This implements the network protocols for Windows
 *
 * @author  Nik Shaylor
 * @version 1.0 1/17/2000
 */

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>
#include <async.h>
#include <socketProtocol_md.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
# ifndef BSD_COMP
#  define BSD_COMP
# endif
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <netinet/in.h>
#include <netdb.h>

#define closesocket close
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR   (-1)
#define HENT_BUF_SIZE 1024
#define NONBLOCKING (!ASYNCHRONOUS_NATIVE_FUNCTIONS)

/*=========================================================================
 * Protocol methods
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      setNonBlocking()
 * CLASS:         com.sun.cldc.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Translate a host name into an ip address
 *=======================================================================*/

void prim_com_sun_cldc_io_j2me_socket_Protocol_setNonBlocking(int fd)
{
#if NONBLOCKING

    int flgs = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flgs | O_NONBLOCK);

#endif /* NONBLOCKING */
}

/*=========================================================================
 * FUNCTION:      getIpNumber()
 * CLASS:         com.sun.cldc.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Translate a host name into an ip address
 *=======================================================================*/

int prim_com_sun_cldc_io_j2me_socket_Protocol_getIpNumber(char *host)
{
    int result;
    char** p;
    struct hostent* hp;
    struct in_addr in;

    if ((hp = gethostbyname(host)) == NULL) {
#if INCLUDEDEBUGCODE
        if (tracenetworking) {
            fprintf(stdout, "getIpNumber host='%s' res=-1\n", host);
        }
#endif /* INCLUDEDEBUGCODE */
        result = -1;
    } else {
        p = hp->h_addr_list;
        (void) memcpy(&in.s_addr, *p, sizeof (in.s_addr));

#if INCLUDEDEBUGCODE
        if (tracenetworking) {
            fprintf(stdout, "getIpNumber host='%s' res=%lx\n", host,
                    (long)in.s_addr);
        }
#endif /* INCLUDEDEBUGCODE */

        result = in.s_addr;
    }

    return (result);
}

/*=========================================================================
 * FUNCTION:      open0()
 * CLASS:         com.sun.cldc.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Open a TCP socket
 *=======================================================================*/

int prim_com_sun_cldc_io_j2me_socket_Protocol_open0(char *name, int port, char **exception)
{
    int truebuf  = TRUE;
    struct sockaddr_in addr;
    int fd = -1, ipn, res;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == INVALID_SOCKET) {
        goto ioe;
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&truebuf, sizeof(int));

    ipn = prim_com_sun_cldc_io_j2me_socket_Protocol_getIpNumber(name);
    if (ipn == -1) {
        *exception = "javax/microedition/io/ConnectionNotFoundException";
        goto fail;
    }

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((unsigned short)port);
    addr.sin_addr.s_addr = ipn;

    res = connect(fd, (struct sockaddr *)&addr, sizeof(addr));

#if INCLUDEDEBUGCODE
    if (tracenetworking) {
        fprintf(stdout, "socket::open0-connect ipn=%lx port=%ld res=%ld ne=%ld\n",
                (long)ipn, (long)port, (long)res, (long)netError());
    }
#endif /* INCLUDEDEBUGCODE */

    if (res >= 0) {
        goto done;
    }

ioe:
    *exception = "java/io/IOException";
    goto fail;

fail:
    if (fd >= 0) {
        closesocket(fd);
        fd = -1;
    }
    goto done;

done:

    return fd;
}

/*=========================================================================
 * FUNCTION:      read0()
 * CLASS:         com.sun.cldc.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Read from a TCP socket
 *=======================================================================*/

int prim_com_sun_cldc_io_j2me_socket_Protocol_read0(int fd, char *p, int len)
{
    int res;

    res = recv(fd, p, len, 0);

#if NONBLOCKING
    if ((res == -1) && (errno == EWOULDBLOCK)) {
        res = -2;
    }
#endif /* NONBLOCKING */

    if (res == -1 && errno == EINTR) {
        res = -3;
    }

    return res;
}

/*=========================================================================
 * FUNCTION:      available 0
 * CLASS:         com.sun.cldc.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Return the number of bytes available
 *=======================================================================*/

int prim_com_sun_cldc_io_j2me_socket_Protocol_available0(int fd)
{
    int n = 0;
    ioctl(fd, FIONREAD, (void*)&n);
    return n;
}

/*=========================================================================
 * FUNCTION:      write0()
 * CLASS:         com.sun.cldc.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Write to a TCP socket
 *=======================================================================*/

int prim_com_sun_cldc_io_j2me_socket_Protocol_write0(int fd, char *p, int len)
{
    int res;

    res = send(fd, p, len, 0);

#if NONBLOCKING
    if ((res == -1) && (errno == EWOULDBLOCK)) {
        res = 0;
    }
#endif /* NONBLOCKING */

    if (res == -1 && errno == EINTR) {
        res = -3;
    }

    return res;
}

/*=========================================================================
 * FUNCTION:      close0()
 * CLASS:         com.sun.cldc.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Close a TCP socket
 *=======================================================================*/

int prim_com_sun_cldc_io_j2me_socket_Protocol_close0(int fd)
{
    return closesocket(fd);
}

/*=========================================================================
 * FUNCTION:      open()
 * CLASS:         com.sun.cldc.io.j2me.serversocket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Open a listening TCP socket
 *=======================================================================*/

int prim_com_sun_cldc_io_j2me_serversocket_Protocol_open0(int port, char **exception)
{
    int truebuf  = TRUE;
    struct sockaddr_in addr;
    int fd = -1, res;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        goto ioe;
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&truebuf, sizeof (int));

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((unsigned short)port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    res = bind(fd, (struct sockaddr*)&addr, sizeof(addr));

    if (res == SOCKET_ERROR) {
        *exception = "javax/microedition/io/ConnectionNotFoundException";
        goto fail;
    }

    res = listen(fd, 3);
    if (res == SOCKET_ERROR) {
        goto ioe;
    }

    goto done;

ioe:
    *exception = "java/io/IOException";
    goto fail;

fail:
    if (fd >= 0) {
        closesocket(fd);
    }
    fd = -1;
    goto done;

done:

    return fd;
}

/*=========================================================================
 * FUNCTION:      accept()
 * CLASS:         com.sun.cldc.io.j2me.serversocket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Accept and open a connection
 *=======================================================================*/

int prim_com_sun_cldc_io_j2me_serversocket_Protocol_accept(int fd)
{
    int res;
    struct sockaddr sa;
    int saLen = sizeof (sa);

    res = accept(fd, &sa, &saLen);

#if NONBLOCKING
    if ((res == -1) && (errno == EWOULDBLOCK)) {
        res = -2;
    }
#endif /* NONBLOCKING */

    return (res);
}

/*=========================================================================
 * FUNCTION:      close0()
 * CLASS:         com.sun.cldc.io.j2me.serversocket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Close a TCP socket
 *=======================================================================*/

int prim_com_sun_cldc_io_j2me_serversocket_Protocol_close(int fd)
{
    return prim_com_sun_cldc_io_j2me_socket_Protocol_close0(fd);
}

/*=========================================================================
 * FUNCTION:      wsaInit()
 * TYPE:          machine-specific implementation of native function
 * OVERVIEW:      Initialize the windows socket system
 * INTERFACE:
 *   parameters:  none
 *   returns:     none
 *=======================================================================*/

void networkInit(void) {
}

/*=========================================================================
 * FUNCTION:      netError()
 * TYPE:          machine-specific implementation of native function
 * OVERVIEW:      return fatal windows socket error
 * INTERFACE:
 *   parameters:  none
 *   returns:     int
 *=======================================================================*/

int netError(void) {
    int res = errno;
    errno = 0;
    return res;
}

