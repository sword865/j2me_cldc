/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 * 
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Output stream for debugger interface
 * FILE:      debuggerOutputStream.c
 * OVERVIEW:  Presents a stream-based interface to data going 
 *            to the user level debugger.
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

#define INITIAL_ID_ALLOC  50

static void
commonInit(PACKET_OUTPUT_STREAM_HANDLE outH)
{
    unhand(outH)->left = INITIAL_SEGMENT_SIZE;
    unhand(outH)->packet = (Packet *)callocObject(SIZEOF_PACKET, GCT_POINTERLIST);
    unhand(outH)->numPointers = 1;
    unhand(outH)->segment = unhand(outH)->packet->type.cmd.data =
        (PacketData *)callocObject(SIZEOF_PACKETDATA, GCT_POINTERLIST);
    unhand(outH)->numPointers = 2;
    unhand(outH)->packet->type.cmd.numPointers = 1;
    unhand(outH)->segment->length = unhand(outH)->segment->index = 0;
    unhand(outH)->segment->data = 
             (unsigned char *)mallocBytes(INITIAL_SEGMENT_SIZE);
    unhand(outH)->segment->numPointers = 1;
    unhand(outH)->error = JDWP_Error_NONE;
    return;
}

PACKET_OUTPUT_STREAM 
outStream_newCommand(unsigned char flags, 
                     unsigned char commandSet, 
                     unsigned char command)
{
    PACKET_OUTPUT_STREAM stream = 
        (PacketOutputStream *)callocObject(SIZEOF_PACKETSTREAM, 
                                           GCT_POINTERLIST);
    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(PACKET_OUTPUT_STREAM, streamX, stream);
        commonInit(&streamX);
        stream = streamX;       /* since GC may have happen */
    END_TEMPORARY_ROOTS
    stream->packet->type.cmd.id = uniqueID();
    stream->packet->type.cmd.cmdSet = commandSet;
    stream->packet->type.cmd.cmd = command;
    stream->packet->type.cmd.flags = flags;
    return stream;
}

void 
outStream_initReply(PACKET_OUTPUT_STREAM_HANDLE outH, long id)
{
    PACKET_OUTPUT_STREAM stream;
    commonInit(outH);

    stream = unhand(outH);
    stream->packet->type.reply.id = id;
    stream->packet->type.reply.errorCode = REPLY_NoError;
    stream->packet->type.cmd.flags = FLAGS_Reply;
}

static void
writeBytes(PACKET_OUTPUT_STREAM_HANDLE outH, void *source, 
           long size, bool_t isHandle)
{
    unsigned char *bytes;
    unsigned long index = 0;
    unsigned char *newSeg;
    PACKET_OUTPUT_STREAM stream;

    if (EXCESSIVE_GARBAGE_COLLECTION) { 
        garbageCollect(0);
    }

    stream = unhand(outH);
        
    if (stream->error) {
        return;
    }

    if (isHandle) { 
        bytes = (unsigned char *)unhand((CONST_CHAR_HANDLE)source);
    } else { 
        bytes = (unsigned char *)source;
    }
    while (size > 0) {
        long count;
        if (stream->left == 0) {
            int segSize = MAX_SEGMENT_SIZE;
            unhand(outH)->segment->next =
                (struct PacketData *)callocObject(SIZEOF_PACKETDATA,
                                                  GCT_POINTERLIST);
            /* stream may now be trash */
            unhand(outH)->segment->numPointers = 2;
            newSeg = (unsigned char *)mallocBytes(segSize);
            /* Refresh stream */
            stream = unhand(outH);

            stream->segment->next->data = newSeg;
            stream->segment = stream->segment->next;
            stream->segment->numPointers = 1;
            stream->left = segSize;
            if (isHandle) { 
                bytes = (unsigned char *)unhand((CONST_CHAR_HANDLE)source);
            }
        }
        count = MIN(size, stream->left);
        memcpy(&stream->segment->data[stream->segment->index], 
               &bytes[index], count);
        stream->segment->index += count;
        stream->left -= count;
        stream->segment->length += count;
        size -= count;
        index += count;
    }
}

void
outStream_writeBoolean(PACKET_OUTPUT_STREAM_HANDLE outH, bool_t val)
{
    unsigned char byte = (val != 0) ? 1 : 0;
    writeBytes(outH, &byte, sizeof(byte), FALSE);
}

void
outStream_writeByte(PACKET_OUTPUT_STREAM_HANDLE outH, unsigned char val)
{
    writeBytes(outH, &val, sizeof(val), FALSE);
}

void
outStream_writeChar(PACKET_OUTPUT_STREAM_HANDLE outH, short val)
{
    val = HOST_TO_JAVA_CHAR(val);
    writeBytes(outH, &val, sizeof(val), FALSE);
}

void
outStream_writeShort(PACKET_OUTPUT_STREAM_HANDLE outH, short val)
{
    val = HOST_TO_JAVA_SHORT(val);
    writeBytes(outH, &val, sizeof(val), FALSE);
}

void
outStream_writeInt(PACKET_OUTPUT_STREAM_HANDLE outH, int val)
{
    val = HOST_TO_JAVA_INT(val);
    writeBytes(outH, &val, sizeof(val), FALSE);
}

void
outStream_writeLong(PACKET_OUTPUT_STREAM_HANDLE outH, long val)
{
    val = HOST_TO_JAVA_INT(val);
    writeBytes(outH, &val, sizeof(val), FALSE);
}

/* only called to write the code offset which is only 32 bits, so we fake the
 * upper word
 */
void
outStream_writeLong64(PACKET_OUTPUT_STREAM_HANDLE outH, unsigned long val)
{
    long zero = 0;

    val = HOST_TO_JAVA_INT(val);
    writeBytes(outH, &zero, sizeof(zero), FALSE);
    writeBytes(outH, &val, sizeof(val), FALSE);
}

void
outStream_writeLocation(PACKET_OUTPUT_STREAM_HANDLE outH, 
                        BYTE tag, long clazzID, 
                        long methodID, unsigned long loc)
{
    outStream_writeByte(outH, tag);
    outStream_writeLong(outH, clazzID);
    outStream_writeLong(outH, methodID);
    outStream_writeLong64(outH, loc);
}

void
outStream_writeString(PACKET_OUTPUT_STREAM_HANDLE outH, CONST_CHAR_HANDLE string)
{
    long length = strlen((char *)unhand(string));
    outStream_writeLong(outH, length);
    writeBytes(outH, (void **)string, length, TRUE);
}

void
outStream_writeObject(PACKET_OUTPUT_STREAM_HANDLE outH, OBJECT object)
{ 
    outStream_writeLong(outH, getObjectID(object));
}

void
outStream_writeThread(PACKET_OUTPUT_STREAM_HANDLE outH, THREAD thread)
{ 
    if (thread == NULL) { 
        outStream_writeLong(outH, 0);
    } else { 
        outStream_writeObject(outH, (OBJECT)thread->javaThread);
    }
}

void
outStream_writeClassName(PACKET_OUTPUT_STREAM_HANDLE outH, CLASS clazz) 
{ 
    int packageNameLength = 
        (clazz->packageName) ? clazz->packageName->length : 0;
    char *baseName = UStringInfo(clazz->baseName);
    int baseNameLength = clazz->baseName->length;

    if (packageNameLength == 0) { 
        if (IS_ARRAY_CLASS(clazz)) { 
            /* What's stored in the basename is exactly what we want */
            outStream_writeLong(outH, baseNameLength);
            writeBytes(outH, baseName, baseNameLength, FALSE);
        } else { 
            /* We need to add an 'L' and a ';' */
            outStream_writeLong(outH, baseNameLength + 2);
            outStream_writeByte(outH, 'L');
            writeBytes(outH, baseName, baseNameLength, FALSE);
            outStream_writeByte(outH, ';');
        }
    } else { 
        char *packageName = UStringInfo(clazz->packageName);
        if (IS_ARRAY_CLASS(clazz)) {
            outStream_writeLong(outH, packageNameLength + baseNameLength + 1);
            /* Output  the '[' that are at the beginning of the name */
            while (baseName[0] == '[') { 
                outStream_writeByte(outH, '[');
                baseName++; baseNameLength--;
            }
            outStream_writeByte(outH, 'L');
            writeBytes(outH, packageName, packageNameLength, FALSE);
            outStream_writeByte(outH, '/');
            /* Skip the 'L' that's at the beginning, but include the ';' 
             * at the end 
             */
            writeBytes(outH, baseName + 1, baseNameLength - 1, FALSE);
        } else {
            outStream_writeLong(outH, packageNameLength + baseNameLength + 3);
            outStream_writeByte(outH, 'L');
            writeBytes(outH, packageName, packageNameLength, FALSE);
            outStream_writeByte(outH, '/');
            writeBytes(outH, baseName, baseNameLength, FALSE);
            outStream_writeByte(outH, ';');
        }
    }
}

int
outStream_error(PACKET_OUTPUT_STREAM_HANDLE outH)
{
    return unhand(outH)->error;
}

void 
outStream_setError(PACKET_OUTPUT_STREAM_HANDLE outH, int error)
{
    if (!unhand(outH)->error) {
        unhand(outH)->error = error;
    }
}

void 
outStream_sendReply(PACKET_OUTPUT_STREAM_HANDLE outH)
{
    int error;
    PACKET_OUTPUT_STREAM stream = unhand(outH);
    if (stream->error) {
        /*
         * Don't send any collected stream data on an error reply
         */
        stream->packet->type.reply.data->length = 0;
        stream->packet->type.reply.errorCode = (short)stream->error;
    } 
    error = dbgSendPacket(stream->packet);
}

void 
outStream_sendCommand(PACKET_OUTPUT_STREAM_HANDLE outH)
{
    int error;
    PACKET_OUTPUT_STREAM stream = unhand(outH);
    if (!stream->error) {
        error = dbgSendPacket(stream->packet);
    } 
}

long
uniqueID()
{
    static long currentID = 0;
    return currentID++;
}

#endif /* ENABLE_JAVA_DEBUGGER */

