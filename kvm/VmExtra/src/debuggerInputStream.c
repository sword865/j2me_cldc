/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 * 
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Input stream for debugger interface
 * FILE:      debuggerInputStream.c
 * OVERVIEW:  Presents a stream-based interface to data coming
 *            from the user-level debugger.
 * AUTHOR:    From JPDA sources/modified by Bill Pittore, Sun Labs
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>

/*=========================================================================
 * Functions
 *=======================================================================*/

#if ENABLE_JAVA_DEBUGGER

/*
 * TO DO: Support processing of replies through command input streams.
 */
void
inStream_init(PACKET_INPUT_STREAM_HANDLE streamH)
{
    PACKET_INPUT_STREAM stream = unhand(streamH);

    stream->error = JDWP_Error_NONE;
    stream->segment = stream->packet->type.cmd.data;
    stream->left = stream->segment->length;
    stream->segment->index = 0;
}

long 
inStream_id(PACKET_INPUT_STREAM_HANDLE streamH)
{
    return unhand(streamH)->packet->type.cmd.id;
}

#if NOT_USED
unsigned char
inStream_command(PACKET_INPUT_STREAM_HANDLE streamH)
{
    return unhand(streamH)->packet->type.cmd.cmd;
}
#endif

static long 
readBytes(PACKET_INPUT_STREAM_HANDLE streamH, void *dest, long size) 
{
    /*
     * Iteration handles items that span multiple packet segments
     */
    PACKET_INPUT_STREAM stream = unhand(streamH);

    if (stream->error) {
        return stream->error;
    }
    while (size > 0) {
        long count = MIN(size, stream->left);
        if (count == 0) {
            /* end of input */
            stream->error = JDWP_Error_INTERNAL;
            return stream->error;
        }
        if (dest) {
            memcpy(dest, &stream->segment->data[stream->segment->index], count);
        }
        stream->segment->index += count;
        stream->left -= count;
        if (stream->left == 0) {
            /*
             * Move to the next segment
             */
            stream->segment = stream->segment->next;
            if (stream->segment) {
                stream->left = stream->segment->length;
                stream->segment->index = 0;
            }
        }
        size -= count;
        if (dest) {
            dest = (char *)dest + count;
        }
    }
    return stream->error;
}

bool_t 
inStream_readBoolean(PACKET_INPUT_STREAM_HANDLE streamH)
{
    unsigned char flag;
    readBytes(streamH, &flag, sizeof(flag));
    if (unhand(streamH)->error) {
        return 0;
    } else {
        return flag ? TRUE : FALSE;
    }
}

unsigned char 
inStream_readByte(PACKET_INPUT_STREAM_HANDLE streamH)
{
    unsigned char val = 0;
    readBytes(streamH, &val, sizeof(val));
    return val;
}

char 
inStream_readChar(PACKET_INPUT_STREAM_HANDLE streamH)
{
    char val = 0;
    readBytes(streamH, &val, sizeof(val));
    return JAVA_TO_HOST_CHAR(val);
}

short 
inStream_readShort(PACKET_INPUT_STREAM_HANDLE streamH)
{
    short val = 0;
    readBytes(streamH, &val, sizeof(val));
    return JAVA_TO_HOST_SHORT(val);
}

int 
inStream_readInt(PACKET_INPUT_STREAM_HANDLE streamH)
{
    int val = 0;
    readBytes(streamH, &val, sizeof(val));
    return JAVA_TO_HOST_INT(val);
}

long
inStream_readLong(PACKET_INPUT_STREAM_HANDLE streamH)
{
    long val = 0;
    readBytes(streamH, &val, sizeof(val));
    return JAVA_TO_HOST_INT(val);
}

/*  val1 is lower 32 bits, val2 is upper 32 bits */
void
inStream_readLong64(PACKET_INPUT_STREAM_HANDLE streamH, unsigned long *val1, unsigned long *val2)
{

    readBytes(streamH, val2, sizeof(val2));
    readBytes(streamH, val1, sizeof(val1));
    *val1 = JAVA_TO_HOST_INT(*val1);
    *val2 = JAVA_TO_HOST_INT(*val2);
}

char * 
inStream_readString(PACKET_INPUT_STREAM_HANDLE streamH)
{
    long length = inStream_readLong(streamH);
    char *result;

    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(char *, string, mallocBytes(length + 1));
        readBytes(streamH, string, length);
        string[length] = '\0';
        result = string;
    END_TEMPORARY_ROOTS
    return result;
}

OBJECT 
inStream_readObject(PACKET_OUTPUT_STREAM_HANDLE inH)
{
    return getObjectPtr(inStream_readLong(inH));
}

bool_t 
inStream_endOfInput(PACKET_INPUT_STREAM_HANDLE streamH)
{
    return (unhand(streamH)->left > 0);
}

int 
inStream_error(PACKET_INPUT_STREAM_HANDLE streamH)
{
    return unhand(streamH)->error;
}

#endif /* ENABLE_JAVA_DEBUGGER */

