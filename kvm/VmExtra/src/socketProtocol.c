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
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>
#include <async.h>
#include <socketProtocol_md.h>

/*=========================================================================
 * Definitions and declarations
 *=======================================================================*/

/* The following flag allows the code for server
 * sockets to be compiled out if necessary.
 */
#ifndef NO_SERVER_SOCKETS
#define NO_SERVER_SOCKETS 0 /* Include server sockets by default */
#endif

#if INCLUDEDEBUGCODE
#define NDEBUG0(fmt)                         if (tracenetworking) { fprintf(stdout, fmt);                         }
#define NDEBUG1(fmt, p1)                     if (tracenetworking) { fprintf(stdout, fmt, p1);                     }
#define NDEBUG2(fmt, p1, p2)                 if (tracenetworking) { fprintf(stdout, fmt, p1, p2);                 }
#define NDEBUG3(fmt, p1, p2, p3)             if (tracenetworking) { fprintf(stdout, fmt, p1, p2, p3);             }
#define NDEBUG4(fmt, p1, p2, p3, p4)         if (tracenetworking) { fprintf(stdout, fmt, p1, p2, p3, p4);         }
#define NDEBUG5(fmt, p1, p2, p3, p4, p5)     if (tracenetworking) { fprintf(stdout, fmt, p1, p2, p3, p4, p5);     }
#define NDEBUG6(fmt, p1, p2, p3, p4, p5, p6) if (tracenetworking) { fprintf(stdout, fmt, p1, p2, p3, p4, p5, p6); }
#else
#define NDEBUG0(fmt)                         /**/
#define NDEBUG1(fmt, p1)                     /**/
#define NDEBUG2(fmt, p1, p2)                 /**/
#define NDEBUG3(fmt, p1, p2, p3)             /**/
#define NDEBUG4(fmt, p1, p2, p3, p4)         /**/
#define NDEBUG5(fmt, p1, p2, p3, p4, p5)     /**/
#define NDEBUG6(fmt, p1, p2, p3, p4, p5, p6) /**/
#endif

#ifndef MAX_HOST_LENGTH
#define MAX_HOST_LENGTH 256
#endif /* MAX_HOST_LENGTH */

/*=========================================================================
 * Protocol methods
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      setSocketHandle()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Set the contents of the handle field
 * INTERFACE:
 *   parameters:  instance, value
 *   returns:     <nothing>
 *=======================================================================*/

static void setSocketHandle(ASYNCIOCB *aiocb, long value)
{
    aiocb->instance->data[0].cell = value; /* 'handle' must be slot 0 */

    NDEBUG1("setSocketHandle handle=%ld\n",
            (long)aiocb->instance->data[0].cell);
}

/*=========================================================================
 * function:      getSocketHandle()
 * TYPE:          private instance-level operation
 * OVERVIEW:      Get the contents of the handle field
 * INTERFACE:
 *   parameters:  instance
 *   returns:     value
 *=======================================================================*/

static int getSocketHandle(ASYNCIOCB *aiocb)
{
    NDEBUG1("getSocketHandle handle=%ld\n",
            (long)aiocb->instance->data[0].cell);

    return aiocb->instance->data[0].cell; /* 'handle' must be slot 0 */
}

/*=========================================================================
 * FUNCTION:      getPortNumber()
 * TYPE:          private instance-level operation
 * OVERVIEW:      Get the port number from the URL
 * INTERFACE:
 *   parameters:  string
 *   returns:     value
 *=======================================================================*/

static int getPortNumber(char *string)
{
    char *p = string;
    while (TRUE) {
        if (*p == '\0') {
            return -1;
        }
        if (*p++ == ':') {
            break;        /* must have a ':' */
        }
    }

    if (*p < '0' || *p > '9') {
        return -1;        /* must have at least one digit' */
    }

    string = p;

    while (*p != '\0') {
        if (*p < '0' || *p > '9') {
            return -1;
        }
        if (*p++ == ';') {
            break;        /* port numbers may be terminated by EOL or ';' */
        }
    }

    NDEBUG2("getPortNumber string='%s' port=%ld\n",
                string, (long)atoi(string));

    return atoi(string);
}

/*=========================================================================
 * FUNCTION:      getHostName()
 * TYPE:          private instance-level operation
 * OVERVIEW:      Get the name from the URL
 * INTERFACE:
 *   parameters:  string
 *   returns:     string
 *   NOTE: *** The input string is modified by this function ***
 *=======================================================================*/

static char *getHostName(char *string)
{
    char *p = string;

    if (*p++ != '/' || *p++ != '/') {
        return NULL;
    }

    while (TRUE) {
        if (*p == '\0' || *p == ':' || *p == ';') {
            break;
        }
        ++p;
    }

   NDEBUG1("getHostName string='%s'", string);
   *p = '\0';         /* terminate the string there */
   NDEBUG1(" name='%s'\n", string+2);

   return string + 2; /* skip past the "//" */
}

/*=========================================================================
 * FUNCTION:      open0()
 * CLASS:         com.sun.cldc.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Open a TCP socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this, name, mode, append
 *   returns:     none
 *=======================================================================*/

ASYNC_FUNCTION_START(Java_com_sun_cldc_io_j2me_socket_Protocol_open0)
{
    long stringlen;
    long            append    = ASYNC_popStack();
    long            mode      = ASYNC_popStack();
    STRING_INSTANCE string    = ASYNC_popStackAsType(STRING_INSTANCE);
    INSTANCE        instance  = ASYNC_popStackAsType(INSTANCE);
    char           *exception = NULL;
    int             fd        = -1;
    char            name[MAX_HOST_LENGTH];
    char           *host;
    int             port;

    aiocb->instance = instance; /* Save instance in ASYNCIOCB */

   /*
    * The name string has the following format: "//<host>:<port>"
    */

    (void)mode;
    (void)append;

    stringlen = string->length + 1;
    (void)getStringContentsSafely(string, name, stringlen);

    port = getPortNumber(name);    /* Must be called before getHostName() */
    if (port < 0 || port > 0xFFFF) {
        NDEBUG2("socket::open0 name='%s' port=%ld (ERROR)\n",
                name, (long)port);
        goto illarg;
    }

    host = getHostName(name);      /* Must be the last parsing function */
    if (host == NULL) {
        NDEBUG1("socket::open0 name='%s' host=null (ERROR)\n", name);
        goto illarg;
    }

    /*
     * We allow "socket://:nnnn" to mean "serversocket://:nnnn" but we
     * throw an InterruptedException to tell the calling Java code
     * about this
     */

    if (*host == '\0') {
        ASYNC_raiseException("java/lang/InterruptedException");
        goto fail;
    }

#if INCLUDEDEBUGCODE
    if (tracenetworking) {
        fprintf(stdout, "socket::open0 name='%s' host='%s' port=%ld\n",
                name, host, (long)port);
    }
#endif /* INCLUDEDEBUGCODE */

    ASYNC_enableGarbageCollection();
    fd = prim_com_sun_cldc_io_j2me_socket_Protocol_open0(host, port, &exception);
    ASYNC_disableGarbageCollection();

    NDEBUG6("socket::open0 name='%s' host='%s' port=%ld fd = %ld exception = '%s' ne=%ld\n",
             name, host, (long)port, (long)fd,
             (exception == NULL) ? "null" : exception, (long)netError());

    if (exception == NULL) {
       /* For non-blocking systems only */
       prim_com_sun_cldc_io_j2me_socket_Protocol_setNonBlocking(fd);
       goto done;
    }

    ASYNC_raiseException(exception);
    goto fail;

illarg:
    ASYNC_raiseException("java/lang/IllegalArgumentException");
fail:
    fd = -1;
done:
    setSocketHandle(aiocb, fd);
}
ASYNC_FUNCTION_END

/*=========================================================================
 * FUNCTION:      read0()
 * CLASS:         com.sun.cldc.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Read from a TCP socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this, buffer, offset, length
 *   returns:     number of chars read or -1 if at EOF
 *=======================================================================*/

ASYNC_FUNCTION_START(Java_com_sun_cldc_io_j2me_socket_Protocol_read0)
{
    long       length   = ASYNC_popStack();
    long       offset   = ASYNC_popStack();
    BYTEARRAY  array    = ASYNC_popStackAsType(BYTEARRAY);
    INSTANCE   instance = ASYNC_popStackAsType(INSTANCE);
    long fd;
    int  res;
    aiocb->instance = instance;
    fd = getSocketHandle(aiocb);

    NDEBUG4("socket::read0 b=%lx o=%ld l=%ld fd=%ld\n",
            (long)array, offset, length, fd);

    if (array == 0) {
        ASYNC_raiseException("java/lang/NullPointerException");
        res = 0;
        goto done;
    }

    if ((offset < 0) || (offset > (long)array->length) ||
        (length < 0) || ((offset + length) > (long)array->length) ||
        ((offset + length) < 0)) {
        ASYNC_raiseException("java/lang/IndexOutOfBoundsException");
        res = 0;
        goto done;
    }

    {
#if ASYNCHRONOUS_NATIVE_FUNCTIONS
        char buffer[ASYNC_BUFFER_SIZE];
       /*
        * If necessary reduce the length to that of the buffer. The
        * Java code will call back with more I/O operations if needed.
        */
        if (length > ASYNC_BUFFER_SIZE) {
            length = ASYNC_BUFFER_SIZE;
        }
        aiocb->array = array;

        ASYNC_enableGarbageCollection();
        res = prim_com_sun_cldc_io_j2me_socket_Protocol_read0(fd,
                  buffer, length);
        ASYNC_disableGarbageCollection();

        if (res > 0) {
            memcpy((char *)aiocb->array->bdata + offset, buffer, res);
        }
#else
        res = prim_com_sun_cldc_io_j2me_socket_Protocol_read0(fd,
                  (char *)array->bdata + offset, length);
#endif /* ASYNCHRONOUS_NATIVE_FUNCTIONS */
    }

    NDEBUG2("socket::read0 res=%ld ne=%ld\n", (long)res, (long)netError());

    if (res == -2) {
        res = 0;
        goto done;
    }

    if (res < 0) {
        if (res == -3) {
            ASYNC_raiseException("java/io/InterruptedIOException");
        } else {
            ASYNC_raiseException("java/io/IOException");
        }
    }

    if (res == 0) {
        res = -1;
    }

done:
    ASYNC_pushStack(res);
}
ASYNC_FUNCTION_END

/*=========================================================================
 * FUNCTION:      write0()
 * CLASS:         com.sun.cldc.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Write to a TCP socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this, buffer, offset, length
 *   returns:     the length written
 *=======================================================================*/

ASYNC_FUNCTION_START(Java_com_sun_cldc_io_j2me_socket_Protocol_write0)
{
    long       length = ASYNC_popStack();
    long       offset = ASYNC_popStack();
    BYTEARRAY   array = ASYNC_popStackAsType(BYTEARRAY);
    INSTANCE instance = ASYNC_popStackAsType(INSTANCE);
    long fd;
    int  res;

    aiocb->instance = instance;
    fd = getSocketHandle(aiocb);

    NDEBUG4("socket::write0 b=%lx o=%ld l=%ld fd=%ld\n",
                   (long)array, (long)offset, (long)length, fd);

    if (array == 0) {
        ASYNC_raiseException("java/lang/NullPointerException");
        res = 0;
        goto done;
    }

    if ((offset < 0) || (offset > (long)array->length) || (length < 0) ||
       ((offset + length) > (long)array->length) || ((offset + length) < 0)) {
        ASYNC_raiseException("java/lang/IndexOutOfBoundsException");
        res = 0;
        goto done;
    }

    {
#if ASYNCHRONOUS_NATIVE_FUNCTIONS
        char buffer[ASYNC_BUFFER_SIZE];
       /*
        * If necessary reduce the length to that of the buffer. The
        * Java code will call back with more I/O operations if needed.
        */
        if (length > ASYNC_BUFFER_SIZE) {
            length = ASYNC_BUFFER_SIZE;
        }
        memcpy(buffer, (char *)array->bdata + offset, length);

        ASYNC_enableGarbageCollection();
        res = prim_com_sun_cldc_io_j2me_socket_Protocol_write0(fd,
                  buffer, length);
        ASYNC_disableGarbageCollection();
#else
        res = prim_com_sun_cldc_io_j2me_socket_Protocol_write0(fd,
                  (char *)array->bdata + offset, length);
#endif /* ASYNCHRONOUS_NATIVE_FUNCTIONS */
    }

    NDEBUG3("socket::write0 res=%ld l=%ld ne=%ld\n",
       (long)res, (long)length, (long)netError());

    if (res < 0) {
        if (res == -3) {
            ASYNC_raiseException("java/io/InterruptedIOException");
        } else {
            ASYNC_raiseException("java/io/IOException");
        }
    }

done:
    ASYNC_pushStack(res);
}
ASYNC_FUNCTION_END

/*=========================================================================
 * FUNCTION:      available0()
 * CLASS:         com.sun.cldc.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Return available length
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     length
 *=======================================================================*/

ASYNC_FUNCTION_START(Java_com_sun_cldc_io_j2me_socket_Protocol_available0)
{
    INSTANCE instance = ASYNC_popStackAsType(INSTANCE);
    long fd;
    int  res;
    aiocb->instance = instance;
    fd = getSocketHandle(aiocb);

    ASYNC_enableGarbageCollection();
    res = prim_com_sun_cldc_io_j2me_socket_Protocol_available0(fd);
    ASYNC_disableGarbageCollection();

    NDEBUG1("socket::available0 len=%ld\n", (long)res);

    ASYNC_pushStack(res);
}
ASYNC_FUNCTION_END

/*=========================================================================
 * FUNCTION:      close0()
 * CLASS:         com.sun.cldc.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Close a TCP socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     none
 *=======================================================================*/

ASYNC_FUNCTION_START(Java_com_sun_cldc_io_j2me_socket_Protocol_close0)
{
    INSTANCE instance = ASYNC_popStackAsType(INSTANCE);
    long fd;
    int  res = 0;
    aiocb->instance = instance;
    fd = getSocketHandle(aiocb);

    if (fd != -1L) {
        setSocketHandle(aiocb, -1);

        ASYNC_enableGarbageCollection();
        res = prim_com_sun_cldc_io_j2me_socket_Protocol_close0(fd);
        ASYNC_disableGarbageCollection();
    }

    NDEBUG3("socket::close res=%ld fd=%ld ne=%ld\n",
            (long)res, fd, (long)netError());

    if (res < 0) {
        ASYNC_raiseException("java/io/IOException");
    }
}
ASYNC_FUNCTION_END

/*=========================================================================
 * FUNCTION:      registerCleanup
 * CLASS:         com.sun.cldc.io.j2me.socket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Close a TCP socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     none
 *=======================================================================*/

static void
socketCleanup(INSTANCE_HANDLE instanceHandle)
{
    INSTANCE instance = unhand(instanceHandle);
    long fd = instance->data[0].cell;
    if (fd != -1L) {
        prim_com_sun_cldc_io_j2me_socket_Protocol_close0(fd);
    }
}

void Java_com_sun_cldc_io_j2me_socket_Protocol_registerCleanup()
{
    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(INSTANCE, instance, popStackAsType(INSTANCE));
        registerCleanup(&instance, socketCleanup);
    END_TEMPORARY_ROOTS
}

/*=========================================================================
 * FUNCTION:      open()
 * CLASS:         com.sun.cldc.io.j2me.serversocket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Open a listening TCP socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this, name, mode, append
 *   returns:     none
 *=======================================================================*/

ASYNC_FUNCTION_START(Java_com_sun_cldc_io_j2me_serversocket_Protocol_open0)
{
#if NO_SERVER_SOCKETS
    ASYNC_raiseException("java/lang/ClassNotFoundException");
#else
    long stringlen;
    long            append    = ASYNC_popStack();
    long            mode      = ASYNC_popStack();
    STRING_INSTANCE string    = ASYNC_popStackAsType(STRING_INSTANCE);
    INSTANCE        instance  = ASYNC_popStackAsType(INSTANCE);
    char           *exception = NULL;
    int             fd        = -1;
    char            name[MAX_HOST_LENGTH];
    char           *host;
    int             port;

    (void)mode;
    (void)append;

    aiocb->instance = instance;

    /*
     * The name string has the following format: "//<host>:<port>"
     */
    stringlen = string->length + 1;
    (void)getStringContentsSafely(string, name, stringlen);

    /* Must be called before getHostName() */
    port = getPortNumber(name);
    if (port < 0 || port > 0xFFFF) {
        NDEBUG2("serversocket::open name='%s' port=%ld (ERROR)\n",
                name, (long)port);
        goto illarg;
    }

    /* Must be the last parsing function */
    host = getHostName(name);
    if (host == NULL || *host != '\0') {
        NDEBUG2("serversocket::open name='%s' host='%s' (ERROR)\n",
                name, (host==NULL) ? "null" : host);
        goto illarg; /* must start "serversocket://:" */
    }

    NDEBUG3("serversocket::open name='%s' host='%s' port=%ld\n",
            name, host, (long)port);

    ASYNC_enableGarbageCollection();
    fd = prim_com_sun_cldc_io_j2me_serversocket_Protocol_open0(port,
                                                               &exception);
    ASYNC_disableGarbageCollection();

    NDEBUG6("serversocket::open name='%s' host='%s' port=%ld fd = %ld exception = '%s' ne=%ld\n",
            name, host, (long)port, (long)fd,
            (exception == NULL) ? "null" : exception, (long)netError());

    if (exception == NULL) {
        /* For non-blocking systems only */
        prim_com_sun_cldc_io_j2me_socket_Protocol_setNonBlocking(fd);
        goto done;
    }

    ASYNC_raiseException(exception);
    goto fail;

illarg:
    ASYNC_raiseException("java/lang/IllegalArgumentException");
fail:
    fd = -1;
done:
    setSocketHandle(aiocb, fd);
#endif /* NO_SERVER_SOCKETS */
}
ASYNC_FUNCTION_END

/*=========================================================================
 * FUNCTION:      accept()
 * CLASS:         com.sun.cldc.io.j2me.serversocket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Accept and open a connection
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     socket handle
 *=======================================================================*/

ASYNC_FUNCTION_START(Java_com_sun_cldc_io_j2me_serversocket_Protocol_accept)
{
#if NO_SERVER_SOCKETS
    ASYNC_raiseException("java/lang/ClassNotFoundException");
#else
    INSTANCE instance = ASYNC_popStackAsType(INSTANCE);
    long fd;
    int  res = 0;
    aiocb->instance = instance;
    fd = getSocketHandle(aiocb);

    NDEBUG1("serversocket::accept fd=%ld\n", fd);

    ASYNC_enableGarbageCollection();
    res = prim_com_sun_cldc_io_j2me_serversocket_Protocol_accept(fd);
    ASYNC_disableGarbageCollection();

    NDEBUG3("serversocket::accept res=%ld fd=%ld ne=%ld\n",
            (long)res, fd, (long)netError());

    if (res < 0 && res != -2) {
        ASYNC_raiseException("java/io/IOException");
    }

    ASYNC_pushStack(res);
#endif /* NO_SERVER_SOCKETS */
}
ASYNC_FUNCTION_END

/*=========================================================================
 * FUNCTION:      close()
 * CLASS:         com.sun.cldc.io.j2me.serversocket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Close a listening TCP socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     none
 *=======================================================================*/

ASYNC_FUNCTION_START(Java_com_sun_cldc_io_j2me_serversocket_Protocol_close)
{
#if NO_SERVER_SOCKETS
    ASYNC_raiseException("java/lang/ClassNotFoundException");
#else
    INSTANCE instance = ASYNC_popStackAsType(INSTANCE);
    long fd;
    int  res = 0;
    aiocb->instance = instance;
    fd = getSocketHandle(aiocb);

    if (fd != -1L) {
        setSocketHandle(aiocb, -1);
        ASYNC_enableGarbageCollection();
        res = prim_com_sun_cldc_io_j2me_serversocket_Protocol_close(fd);
        ASYNC_disableGarbageCollection();
    }

    NDEBUG3("serversocket::close res=%ld fd=%ld ne=%ld\n",
               (long)res, fd, (long)netError());

    if (res < 0) {
        ASYNC_raiseException("java/io/IOException");
    }
#endif /* NO_SERVER_SOCKETS */
}
ASYNC_FUNCTION_END

/*=========================================================================
 * FUNCTION:      registerCleanup
 * CLASS:         com.sun.cldc.io.j2me.serversocket.Protocol
 * TYPE:          virtual native function
 * OVERVIEW:      Close a TCP socket
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     none
 *=======================================================================*/

static void
serversocketCleanup(INSTANCE_HANDLE instanceHandle)
{
    INSTANCE instance = unhand(instanceHandle);
    long fd = instance->data[0].cell;
    if (fd != -1L) {
        prim_com_sun_cldc_io_j2me_serversocket_Protocol_close(fd);
    }
}

void Java_com_sun_cldc_io_j2me_serversocket_Protocol_registerCleanup()
{
    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(INSTANCE, instance, popStackAsType(INSTANCE));
        registerCleanup(&instance, serversocketCleanup);
    END_TEMPORARY_ROOTS
}

/*=========================================================================
 * FUNCTION:      initalizeInternal();
 * CLASS:         com.sun.cldc.io.j2me.socket.Protocol()
 * TYPE:          virtual native function
 * OVERVIEW:      Initialize the network
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     none
 *
 * This is a static initializer in NetworkConnectionBase, the parent of
 * all ..../Protocol classes that access the network.
 *=======================================================================*/

void
Java_com_sun_cldc_io_j2me_socket_Protocol_initializeInternal()
{
    networkInit();
}

