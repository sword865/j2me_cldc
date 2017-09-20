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

#include <stdlib.h>
#include <string.h>

#ifndef POCKETPC
#include <time.h>
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMSG
#include <windows.h>

#ifndef POCKETPC
#include <process.h>
#ifdef GCC
#include <winsock.h>
#else
#include <winsock2.h>
#endif /* GCC */
#else
#include <winsock.h>
#endif /* POCKETPC */

WSADATA wsaData;
int netError(void);
#define NONBLOCKING 0

#ifdef POCKETPC
int errno;
#endif

/*=========================================================================
 * Protocol methods
 *=======================================================================*/
/*=========================================================================
 * FUNCTION:      getIpNumber()
 * CLASS:         com.sun.cldc.io.j2me.datagram.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Translate a host name into an ip address
 *=======================================================================*/

int prim_com_sun_cldc_io_j2me_datagram_Protocol_getIpNumber(char *host)
{
    struct sockaddr_in sin;
    struct sockaddr_in* sa = &sin;
    struct hostent *hp;
    struct in_addr addr;
    int result;

    if ((hp = gethostbyname(host)) == NULL ) {
#if INCLUDEDEBUGCODE
        if (tracenetworking) {
            fprintf(stdout, "getIpNumber host='%s' res=-1\n", host);
        }
#endif /* INCLUDEDEBUGCODE */
        result = -1;
    } else {
        memset( sa, 0, sizeof(struct sockaddr_in) );
        memcpy(&addr, hp->h_addr_list[0], hp->h_length);

#if INCLUDEDEBUGCODE
        if (tracenetworking) {
            fprintf(stdout, "getIpNumber host='%s' res=%lx\n", host,
                    (long)addr.s_addr);
        }
#endif /* INCLUDEDEBUGCODE */

        result = addr.s_addr;
    }

    return result;
}

/*=========================================================================
 * FUNCTION:      setNonBlocking()
 * CLASS:         com.sun.cldc.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Translate a host name into an ip address
 *=======================================================================*/

void prim_com_sun_cldc_io_j2me_socket_Protocol_setNonBlocking(int fd)
{
    /* Do nothing this. This is a blocking only version */
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
    int falsebuf = FALSE;
    struct sockaddr_in addr;
    int fd = -1, ipn, res;

    ipn = prim_com_sun_cldc_io_j2me_datagram_Protocol_getIpNumber(name);
    if (ipn == -1) {
        *exception = "javax/microedition/io/ConnectionNotFoundException";
        goto fail;
    }

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((unsigned short)port);
    addr.sin_addr.s_addr = ipn;

    do {

        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd == INVALID_SOCKET) {
            goto ioe;
        }

        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&falsebuf, sizeof(int));

        res = connect(fd, (struct sockaddr *)&addr, sizeof(addr));

        if (res == SOCKET_ERROR) {
            errno = netError();
            closesocket(fd);
        }
#if INCLUDEDEBUGCODE
        if (tracenetworking) {
            fprintf(stdout, "socket::open0-connect ipn=%lx port=%ld res=%ld ne=%ld\n",
                    (long)ipn, (long)port, (long)res, (long)netError());
        }
#endif /* INCLUDEDEBUGCODE */

    } while ((res == SOCKET_ERROR) && (errno == WSAEADDRINUSE ||
                                       errno == WSAEINTR ||
                                       errno == WSAEALREADY));

    if (res == SOCKET_ERROR && errno == WSAEISCONN) {
        res = 0;
    } else if (res == SOCKET_ERROR) {
        goto ioe;
    }

    if (res == 0) {
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
    return recv(fd, p, len, 0);
}

/*=========================================================================
 * FUNCTION:      available0
 * CLASS:         com.sun.cldc.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Return the number of bytes available
 *=======================================================================*/

int prim_com_sun_cldc_io_j2me_socket_Protocol_available0(int fd)
{
    u_long len;
    int res = ioctlsocket(fd, FIONREAD, &len);

    if (res < 0) {
        return 0;
    } else {
        return (int)len;
    }
}

/*=========================================================================
 * FUNCTION:      write0()
 * CLASS:         com.sun.cldc.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Write to a TCP socket
 *=======================================================================*/

int prim_com_sun_cldc_io_j2me_socket_Protocol_write0(int fd, char *p, int len)
{
    return send(fd, p, len, 0);
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
    int falsebuf  = FALSE;
    struct sockaddr_in addr;
    int fd = -1, res;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        goto ioe;
    }

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&falsebuf, sizeof (int));

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
    struct sockaddr sa;
    int saLen = sizeof (sa);
    return accept(fd, &sa, &saLen);
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
    if (WSAStartup(0x0101, &wsaData ) != 0) {
        fatalError(KVM_MSG_WSASTARTUP_FAILURE);
    }
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
    return WSAGetLastError();
}

