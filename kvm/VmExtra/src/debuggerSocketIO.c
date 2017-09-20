/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 */

/*=========================================================================
 * KVM
 *=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Java-level debugging interface
 * FILE:      dbg_socket.c
 * OVERVIEW:  Interface to underlying network sockets for Java-level
 *            debugging
 * AUTHOR:    Bill Pittore, Sun Labs
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>

#if ENABLE_JAVA_DEBUGGER

#ifdef PILOT
#include <TimeMgr.h>
#include <NetMgr.h>
#include <HostControl.h>

#define IGNORE_STDIO_STUBS
#define __string_h
#define __stdlib_h
#define __stdarg_h
#define __cstdarg__
#include <sys_socket.h>
#undef IGNORE_STDIO_STUBS

NetSocketRef  dbgSocket;
NetSocketRef listenSocket;
#define NONBLOCKING 0

#endif /* PILOT */

#ifdef WINDOWS
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
#endif

#ifdef POCKETPC
# include <winsock.h>
#else
# ifdef GCC
#  include <winsock.h>
# else
#  include <winsock2.h>
# endif /* GCC */
#endif /* POCKETPC */

WSADATA wsaData;
int  dbgSocket;
int listenSocket;
#define NONBLOCKING 0

#endif /* WINDOWS */

#if UNIX
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#define closesocket close
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR   (-1)
int  dbgSocket;
int listenSocket;
#define NONBLOCKING 0

#endif /* UNIX */

#ifdef POCKETPC
#define close closesocket
int errno;
#endif

/*=========================================================================
 * Functions
 *=======================================================================*/

/*
 * Select on the dbgSocket and wait for a character to arrive.
 * Timeout is in millisecs.
 */

bool_t dbgCharAvail(int timeout)
{
    fd_set readFDs, writeFDs, exceptFDs;
    int numFDs = 0;
    int width;
    struct timeval tv, *tvp;

    if (dbgSocket == 0)
        return FALSE;
    FD_ZERO(&readFDs);
    FD_ZERO(&writeFDs);
    FD_ZERO(&exceptFDs);
    FD_SET((unsigned int)dbgSocket, &readFDs);
    width = dbgSocket;
    if (timeout == -1) {
        tv.tv_sec = 0;
        tv.tv_usec = 20000;
        tvp = &tv;
    tvp = NULL;
    } else {
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        tvp = &tv;
    }
    numFDs = select(width+1, &readFDs, &writeFDs,
        &exceptFDs, tvp);
    if (numFDs <= 0) {
        return(FALSE);
    } else {
        return (TRUE);
    }
}

static int dbgWriteX(int fd, void *buf, int len)
{
    int bytes;

    bytes = send(fd, buf, len, 0);
    return (bytes);
}

int dbgWrite(void *buf, int len)
{
    int bytes;

    if (dbgSocket == 0)
        return 0;
    bytes = dbgWriteX(dbgSocket, buf, len);
    return (bytes);
}

int dbgSendPacket(struct Packet *packet)
{
    long len = 11; /* size of packet header */
    struct PacketData *data;

    data = packet->type.cmd.data;
    do {
        len += data->length;
        data = data->next;
    } while (data != NULL);

    len = (long)htonl(len);

    if (dbgWriteX(dbgSocket,&len,sizeof(long)) != sizeof(long))
        return SYS_ERR;

    packet->type.cmd.id = (long)htonl(packet->type.cmd.id);

    if (dbgWriteX(dbgSocket, &(packet->type.cmd.id),sizeof(long)) != sizeof(long))
        return SYS_ERR;

    if (dbgWriteX(dbgSocket, &(packet->type.cmd.flags),sizeof(unsigned char)) != sizeof(unsigned char))
        return SYS_ERR;

    if (packet->type.cmd.flags & FLAGS_Reply) {
        short errorCode = htons(packet->type.reply.errorCode);
        if (dbgWriteX(dbgSocket, &(errorCode)
                     ,sizeof(short)) != sizeof(short))
            return SYS_ERR;
    } else {
        if (dbgWriteX(dbgSocket, &(packet->type.cmd.cmdSet)
                     ,sizeof(unsigned char)) != sizeof(unsigned char))
            return SYS_ERR;
    
        if (dbgWriteX(dbgSocket,(char *)&(packet->type.cmd.cmd)
                     ,sizeof(unsigned char)) != sizeof(unsigned char))
            return SYS_ERR;
    }

    data = packet->type.cmd.data;
    do {
        if (data->length == 0)
            break;

        if (dbgWriteX(dbgSocket,(char *)data->data,data->length) != data->length)
            return SYS_ERR;

        data = data->next;
    } while (data != NULL);

    return SYS_OK;
}

void dbgFlush()
{
    char buf[16];
    while (dbgRead(buf, 16, 0) > 0)
        continue;
}

int dbgRead(void *buf, int len, int blockflag)
{
    int nread;
    unsigned long nleft = len;
    char *ptr = buf;
    unsigned long total = 0;
    int error;

    if (dbgSocket == 0)
        return 0;
    do {
        nread = recv(dbgSocket, ptr, nleft, 0);
        /* socket is non-blocking so check for errwouldblock */
#if NONBLOCKING
        if (nread < 0 && errno != EWOULDBLOCK) {
            return 0;
        }
#else
        if (nread < 0) {
            error = errno;
            return 0;
        }
#endif /* NONBLOCKING */
        /*
         * just return what we've read so far (may be zero, that's ok)
         * at this point if nread < 0 then err == netErrWouldBlock
         */
        if (nread < 0 && !blockflag)
            break;
        if (nread < 0)
            continue;    /* only get here if nread < 0 AND */
                    /* err == netErrWouldBlock AND blockflag true */
        /* if we got zero back then the connection is closed so return -1 */
        if (nread == 0) {
            if (total > 0)
                return total;
            else
                return 0;
        }
        total += nread;
        nleft -= nread;
        ptr   += nread;
    } while (nleft);

    return total;
}

int dbgReceivePacket(PACKET_INPUT_STREAM_HANDLE streamH)
{
    long length, readLength;
    unsigned char *readBuf;
    unsigned char *newSeg;
    PACKET_INPUT_STREAM stream = unhand(streamH);
    Packet *pkt = stream->packet;
    int sysError;

    sysError = SYS_OK;
    if (dbgRead(&length, sizeof(long), TRUE) < sizeof(long)) {
        return SYS_ERR;
    }
    length = ntohl(length);
    if (dbgRead(&(pkt->type.cmd.id), sizeof(long), TRUE) < sizeof(long)) {
        return SYS_ERR;
    }
    pkt->type.cmd.id = htonl(pkt->type.cmd.id);
    if (dbgRead(&(pkt->type.cmd.flags), sizeof(unsigned char), TRUE) <
        sizeof (unsigned char)) {
        return SYS_ERR;
    }
    if (pkt->type.cmd.flags & FLAGS_Reply) {
        if (dbgRead(&(pkt->type.reply.errorCode), sizeof(short),
            TRUE) < sizeof(short)) {
            return SYS_ERR;
        }
    } else {
        if (dbgRead(&(pkt->type.cmd.cmdSet), sizeof(unsigned char),
            TRUE) < sizeof(unsigned char)) {
                return SYS_ERR;
            }
        if (dbgRead(&(pkt->type.cmd.cmd), sizeof(unsigned char),
            TRUE) < sizeof(unsigned char)) {
                return SYS_ERR;
            }
    }
    length -= ((sizeof(long) * 2) + (sizeof(unsigned char)*3));

    if (length < 0) {
        return SYS_ERR;
    } else if (length == 0) {
        stream->segment->length = 0;
        return SYS_OK;
    } else {
        readBuf = stream->segment->data;
        while (length > 0) {
            if (readBuf == NULL) {
                sysError = SYS_ERR;
                goto breakout;
            }

            /* XXXX */
            if ((readLength = length) > MAX_INPUT_PACKET_SIZE) {
                readLength = MAX_INPUT_PACKET_SIZE;
            }
            stream->segment->length = readLength;

            if (dbgRead(readBuf, readLength, TRUE) != readLength) {
                sysError = SYS_ERR;
                goto breakout;
            }
            length -= readLength;
            if (length == 0) { 
                break;
            }
            unhand(streamH)->segment->next =
                (struct PacketData *)callocObject(SIZEOF_PACKETDATA,
                                                  GCT_POINTERLIST);
            unhand(streamH)->segment->numPointers = 2;
            newSeg = (unsigned char *)mallocBytes(MAX_INPUT_PACKET_SIZE);
            stream = unhand(streamH);
            stream->segment->next->data = newSeg;
            stream->segment->next->numPointers = 1;
            stream->segment = stream->segment->next;
            readBuf = stream->segment->data;
        }
breakout:
        return sysError;
    }
}

/*=========================================================================
 * FUNCTION:        InitDebuggerIO
 * TYPE:            private function
 * OVERVIEW:        Open net library
 *
 *
 * INTERFACE:
 *   parameters:    <none>
 *   returns:        true - no error
 *                    false -  Error
 * NOTE:
 *=======================================================================*/

bool_t InitDebuggerIO()
{

    struct sockaddr_in local_addr;
    int optval;

    int socketErr;
#ifdef PILOT
    unsigned short ifErrs;

    errno = SysLibFind("Net.lib", &AppNetRefnum);
    if (errno) {
        return FALSE;
    }
    errno = NetLibOpen(AppNetRefnum, &ifErrs);
    if ((errno && errno != netErrAlreadyOpen) || ifErrs) {
        if (HostGetHostID() != hostIDPalmOS) {
            AlertUser("Unable to start debugger.\nPlease turn on the \""
                      "Redirect NetLib calls to host TCP/IP\" option in"
                      "the Emulator's Properties setting, and restart the "
                      "debugging session");
        }
        return FALSE;
    }
#endif
#ifdef WINDOWS
    if (WSAStartup(0x0101, &wsaData) != 0) {
        fprintf(stdout, KVM_MSG_DEBUGGER_COULD_NOT_INIT_WINSOCK);
        return FALSE;
    }
#endif
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
#ifdef PILOT
        NetLibClose(AppNetRefnum, FALSE);
#endif
        fprintf(stdout, KVM_MSG_DEBUGGER_COULD_NOT_OPEN_LISTENSOCKET);
        return FALSE;
    }
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(debuggerPort);

    optval = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (void *)&optval, sizeof(optval));
    socketErr = bind(listenSocket, (struct sockaddr *)&local_addr,
        sizeof(local_addr));
    if (socketErr < 0) {
        close(listenSocket);
#ifdef PILOT
        NetLibClose(AppNetRefnum, FALSE);
#endif
        fprintf(stdout, KVM_MSG_DEBUGGER_COULD_NOT_BIND_LISTENSOCKET);
        return FALSE;
    }
    socketErr = listen(listenSocket, 1);
    if (socketErr < 0) {
        close(listenSocket);
#ifdef PILOT
        NetLibClose(AppNetRefnum, FALSE);
#endif
        fprintf(stdout, KVM_MSG_DEBUGGER_COULD_NOT_LISTEN_TO_SOCKET);
        return FALSE;
    }
    return TRUE;
}

bool_t GetDebuggerChannel(int timeout)
{
    struct sockaddr_in rem_addr;
    int rem_addr_len = sizeof(rem_addr);
    fd_set readFDs, writeFDs, exceptFDs;
    int numFDs;
    int width;
    struct timeval tv;
    int optval;
#if NONBLOCKING
#ifndef PILOT
    int flgs;
    int one = 1;
#endif
#endif

    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    FD_ZERO(&readFDs);
    FD_ZERO(&writeFDs);
    FD_ZERO(&exceptFDs);
    FD_SET((unsigned int)listenSocket, &readFDs);
    width = listenSocket;
    numFDs = select(width+1, &readFDs, &writeFDs,
     &exceptFDs, &tv);

    if (numFDs <= 0) {
        fprintf(stdout, KVM_MSG_DEBUGGER_FD_SELECT_FAILED);
        return(FALSE);
    }
    if (FD_ISSET(listenSocket, &readFDs)) {
        dbgSocket = accept(listenSocket, (struct sockaddr *)&rem_addr, &rem_addr_len);
        if (dbgSocket < 0) {
            dbgSocket = 0;
            fprintf(stdout, KVM_MSG_DEBUGGER_FD_ACCEPT_FAILED);
            return (FALSE);
        }
    /* Turn off Nagle algorithm which is slow in NT.  Since we send many
     * small packets Nagle actually slows us down as we send the packet
     * header in tiny chunks before sending the data portion.  Without this
     * option, it could take 200 ms. per packet roundtrip from KVM on NT to
     * Forte running on some other machine.
     */
    optval = 1;
    setsockopt(dbgSocket, IPPROTO_TCP, TCP_NODELAY, (void *)&optval, sizeof(optval));
#if NONBLOCKING
#ifdef PILOT
        NetLibSocketOptionSet(AppNetRefnum, dbgSocket, netSocketOptLevelSocket,
            netSocketOptSockNonBlocking, &one, sizeof (one), timeout, &errno);
#else
        flgs = fcntl(dbgSocket, F_GETFL, 0);
        fcntl(dbgSocket, F_SETFL, flgs | O_NONBLOCK);
#endif
#endif
        return (TRUE);
    } else
        return (FALSE);
}

void CloseDebuggerIO()
{
    fd_set readFDs, writeFDs, exceptFDs;
    int numFDs;
    int width;
    struct timeval tv;

    if (dbgSocket) {
        FD_ZERO(&readFDs);
        FD_ZERO(&writeFDs);
        FD_ZERO(&exceptFDs);
        FD_SET((unsigned int)dbgSocket, &exceptFDs);
        width = dbgSocket;
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        numFDs = select(width+1, &readFDs, &writeFDs,
             &exceptFDs, &tv);
        shutdown(dbgSocket, 2);
        close(dbgSocket);
    }
    if (listenSocket) {
        shutdown(listenSocket, 2);
        close(listenSocket);
    }
    dbgSocket = 0;
    listenSocket = 0;
}

bool_t dbgInitialized()
{
    return (dbgSocket != 0);
}

#endif /* ENABLE_JAVA_DEBUGGER */

