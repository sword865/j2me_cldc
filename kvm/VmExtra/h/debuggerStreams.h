/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 * 
 */
/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Debugger
 * FILE:      debuggerStreams.h
 * OVERVIEW:  Header file for Java level debugger code that implements a 
 *            subset of the JPDA.  This file defines the input/output
 *            streams that are used to read/write data to/from the user
 *            level Java debugger
 * AUTHOR:    From JPDA sources/modified by Bill Pittore, Sun Labs
 *=======================================================================*/

/*
 * Data structs
 *
 * NOTE! Individual items in the Packet(s) do not need
 * to be adjusted for endianness. Every field *except* for
 * the PacketData.data field is automatically adjusted
 * by the underlying transport. However, there is no way
 * for the transport to know the format of the raw data,
 * so you must handle this. 
 */

#ifndef __STREAM_H__
#define __STREAM_H__

#define FLAGS_None         ((unsigned char)0x0)
#define FLAGS_Reply        ((unsigned char)0x80)

#define REPLY_NoError      ((short)0x0)

typedef struct PacketStream {
    long numPointers;    /* for pointerlist GC */
    struct Packet *packet;
    struct PacketData *segment;
    long left;
    int error;
} PacketStream, PacketInputStream, PacketOutputStream;

typedef PacketInputStream *PACKET_INPUT_STREAM;
typedef PacketInputStream **PACKET_INPUT_STREAM_HANDLE;

typedef PacketOutputStream *PACKET_OUTPUT_STREAM;
typedef PacketOutputStream **PACKET_OUTPUT_STREAM_HANDLE;

typedef struct PacketData {
    long numPointers;    /* for pointerlist GC */
    unsigned char *data;
    struct PacketData *next;
    long length;
    int index;
} PacketData;

typedef struct CmdPacket {
    long numPointers;    /* for pointerlist GC */
    PacketData *data;
    long id;
    unsigned char flags;
    unsigned char cmdSet;
    unsigned char cmd;
} CmdPacket;

typedef struct ReplyPacket {
    long numPointers;    /* for pointerlist GC */
    PacketData *data;
    long id;
    unsigned char flags;
    short errorCode;
} ReplyPacket;

typedef struct Packet {
    union {
        CmdPacket cmd;
        ReplyPacket reply;
    } type;
} Packet;

typedef union Value {
    bool_t z;
    unsigned char    b;
    char    c;
    short   s;
    long     i;
    union {
        unsigned long    j1;
        unsigned long    j2;
    } u;
    OBJECT  l;
} Value;

#if defined(__i386) || defined(x86) || defined(WINDOWS)

#define HOST_TO_JAVA_CHAR(x) (((x & 0xff) << 8) | ((x >> 8) & (0xff)))
#define HOST_TO_JAVA_SHORT(x) (((x & 0xff) << 8) | ((x >> 8) & (0xff)))
#define HOST_TO_JAVA_INT(x)                                             \
                  ((x << 24) |                                          \
                   ((x & 0x0000ff00) << 8) |                            \
                   ((x & 0x00ff0000) >> 8) |                            \
                   (((unsigned int)(x & 0xff000000)) >> 24))
#define HOST_TO_JAVA_LONG(x)                                            \
                  ((x << 56) |                                          \
                   ((x & 0x000000000000ff00) << 40) |                   \
                   ((x & 0x0000000000ff0000) << 24) |                   \
                   ((x & 0x00000000ff000000) << 8) |                    \
                   ((x & 0x000000ff00000000) >> 8) |                    \
                   ((x & 0x0000ff0000000000) >> 24) |                   \
                   ((x & 0x00ff000000000000) >> 40) |                   \
                   (((ulong64)(x & 0xff00000000000000)) >> 56));

#else 

#define HOST_TO_JAVA_CHAR(x)   (x)
#define HOST_TO_JAVA_SHORT(x)  (x)
#define HOST_TO_JAVA_INT(x)    (x)
#define HOST_TO_JAVA_LONG(x)   (x)
#define HOST_TO_JAVA_FLOAT(x)  (x)
#define HOST_TO_JAVA_DOUBLE(x) (x)

#endif /* i386, i86, Windows */
                             
#define JAVA_TO_HOST_CHAR(x)   HOST_TO_JAVA_CHAR(x)  
#define JAVA_TO_HOST_SHORT(x)  HOST_TO_JAVA_SHORT(x) 
#define JAVA_TO_HOST_INT(x)    HOST_TO_JAVA_INT(x)   
#define JAVA_TO_HOST_LONG(x)   HOST_TO_JAVA_LONG(x)  
#define JAVA_TO_HOST_FLOAT(x)  HOST_TO_JAVA_FLOAT(x) 
#define JAVA_TO_HOST_DOUBLE(x) HOST_TO_JAVA_DOUBLE(x)

/*
 * Input stream defines
 */

void inStream_init(PACKET_INPUT_STREAM_HANDLE inH);

long inStream_id(PACKET_INPUT_STREAM_HANDLE inH);
unsigned char inStream_command(PACKET_INPUT_STREAM_HANDLE inH);

unsigned char inStream_readByte(PACKET_INPUT_STREAM_HANDLE inH);
bool_t inStream_readBoolean(PACKET_INPUT_STREAM_HANDLE inH);
char inStream_readChar(PACKET_INPUT_STREAM_HANDLE inH);
short inStream_readShort(PACKET_INPUT_STREAM_HANDLE inH);
int inStream_readInt(PACKET_INPUT_STREAM_HANDLE inH);
long inStream_readLong(PACKET_INPUT_STREAM_HANDLE inH);
void inStream_readLong64(PACKET_INPUT_STREAM_HANDLE inH, 
                         unsigned long *, unsigned long *);
char *inStream_readString(PACKET_INPUT_STREAM_HANDLE inH);
OBJECT inStream_readObject(PACKET_INPUT_STREAM_HANDLE inH);

bool_t inStream_endOfInput(PACKET_INPUT_STREAM_HANDLE inH);
int inStream_error(PACKET_INPUT_STREAM_HANDLE inH);

#define inStream_readClassID(inH)  inStream_readLong(inH)
#define inStream_readObjectID(inH) inStream_readLong(inH)
#define inStream_readThreadID(inH) inStream_readLong(inH)

#define inStream_readClass(inH)  GET_DEBUGGERID_CLASS(inStream_readClassID(inH))

/*
 * Output stream defines
 */

#define INITIAL_SEGMENT_SIZE   256
#define MAX_SEGMENT_SIZE       512
#define SIZEOF_PACKETSTREAM    StructSizeInCells(PacketStream)
#define SIZEOF_PACKETDATA      StructSizeInCells(PacketData)
#define SIZEOF_PACKET          StructSizeInCells(Packet)

PACKET_OUTPUT_STREAM 
outStream_newCommand(unsigned char flags, 
                     unsigned char commandSet, 
                     unsigned char command);
void outStream_initReply(PACKET_OUTPUT_STREAM_HANDLE stream, long id);

#define outStream_id(outH)      unhand(outH)->packet->type.cmd.id
#define outStream_command(outH) unhand(outH)->packet->type.cmd.cmd

#define outStream_writeObjectID(outH, ID) outStream_writeLong(outH, ID)
#define outStream_writeClassID(outH, ID) outStream_writeLong(outH, ID)
#define outStream_writeFrameID(outH, val) outStream_writeLong(outH, (long)val)

#define outStream_writeClass(outH, clazz) \
      outStream_writeClassID(outH, GET_CLASS_DEBUGGERID(clazz))

void outStream_writeBoolean(PACKET_OUTPUT_STREAM_HANDLE outH, bool_t val);
void outStream_writeByte(PACKET_OUTPUT_STREAM_HANDLE outH, unsigned char val);
void outStream_writeChar(PACKET_OUTPUT_STREAM_HANDLE outH, short val);
void outStream_writeShort(PACKET_OUTPUT_STREAM_HANDLE outH, short val);
void outStream_writeInt(PACKET_OUTPUT_STREAM_HANDLE outH, int val);
void outStream_writeLong(PACKET_OUTPUT_STREAM_HANDLE outH, long val);
void outStream_writeObject(PACKET_OUTPUT_STREAM_HANDLE outH, OBJECT val);
void outStream_writeThread(PACKET_OUTPUT_STREAM_HANDLE outH, THREAD val);

void outStream_writeLong64(PACKET_OUTPUT_STREAM_HANDLE outH, unsigned long val);

void outStream_writeLocation(PACKET_OUTPUT_STREAM_HANDLE outH, BYTE tag,
                            long clazzID, long methodID, unsigned long loc);

void outStream_writeString(PACKET_OUTPUT_STREAM_HANDLE outH, 
                          CONST_CHAR_HANDLE string);
void outStream_writeClassName(PACKET_OUTPUT_STREAM_HANDLE outH, CLASS clazz);

int outStream_error(PACKET_OUTPUT_STREAM_HANDLE outH);
void outStream_setError(PACKET_OUTPUT_STREAM_HANDLE outH, int error);

void outStream_sendReply(PACKET_OUTPUT_STREAM_HANDLE outH);
void outStream_sendCommand(PACKET_OUTPUT_STREAM_HANDLE outH);

long uniqueID(void);

#endif /* __STREAM_H__ */

