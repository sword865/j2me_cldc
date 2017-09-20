/*
 * @(#)PacketStream.java    1.24 99/05/21
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 *
 */

package kdp;
import java.util.*;
import java.io.*;

class PacketStream {
    final ProxyListener proxy;
    private int inCursor = 0;
    final Packet pkt;
    private ByteArrayOutputStream dataStream = new ByteArrayOutputStream();
    private boolean isCommitted = false;
    public int id;  // public so proxy can get at it quickly

    PacketStream(ProxyListener proxy, int cmdSet, int cmd) {
        this.proxy = proxy;
        this.pkt = new Packet();
        id = pkt.id;
        pkt.cmdSet = (short)cmdSet;
        pkt.cmd = (short)cmd;
    }

    PacketStream(ProxyListener proxy, int id, short flags, short errorCode ) {
        this.pkt = new Packet();
        this.proxy = proxy;
        this.pkt.id = id;
        this.id = id;
        this.pkt.errorCode = errorCode;
        this.pkt.flags = flags;
    }

    PacketStream(ProxyListener proxy, Packet p) {
        this.pkt = p;
        this.proxy = proxy;
        this.id = p.id;
    }

    int id() {
        return id;
    }

    void send() {
        if (!isCommitted) {
            pkt.data = dataStream.toByteArray();
            try {
                proxy.send(pkt);
            } catch (IOException e) {
            }
            isCommitted = true;
        }
    }

    void waitForReply() throws Exception {
        if (!isCommitted) {
            throw new Exception("waitForReply without send");
        }

        proxy.waitForReply(pkt);

        if (pkt.errorCode != Packet.ReplyNoError) {
            throw new Exception(String.valueOf(pkt.errorCode));
        }
    }

    void writeBoolean(boolean data) {
        if(data) {
            dataStream.write( 1 );
        } else {
            dataStream.write( 0 );
        }
    }

    void writeByte(byte data) {
        dataStream.write( data );
    }

    void writeChar(char data) {
        dataStream.write( (byte)((data >>> 8) & 0xFF) );
        dataStream.write( (byte)((data >>> 0) & 0xFF) );
    }

    void writeShort(short data) {
        dataStream.write( (byte)((data >>> 8) & 0xFF) );
        dataStream.write( (byte)((data >>> 0) & 0xFF) );
    }

    void writeInt(int data) {
        dataStream.write( (byte)((data >>> 24) & 0xFF) );
        dataStream.write( (byte)((data >>> 16) & 0xFF) );
        dataStream.write( (byte)((data >>> 8) & 0xFF) );
        dataStream.write( (byte)((data >>> 0) & 0xFF) );
    }

    void writeLong(long data) {
        dataStream.write( (byte)((data >>> 56) & 0xFF) );
        dataStream.write( (byte)((data >>> 48) & 0xFF) );
        dataStream.write( (byte)((data >>> 40) & 0xFF) );
        dataStream.write( (byte)((data >>> 32) & 0xFF) );

        dataStream.write( (byte)((data >>> 24) & 0xFF) );
        dataStream.write( (byte)((data >>> 16) & 0xFF) );
        dataStream.write( (byte)((data >>> 8) & 0xFF) );
        dataStream.write( (byte)((data >>> 0) & 0xFF) );
    }

    void writeFloat(float data) {
        writeInt(Float.floatToIntBits(data));
    }

    void writeDouble(double data) {
        writeLong(Double.doubleToLongBits(data));
    }

    void writeID(int size, long data) {
        if (size == 8) {
            writeLong(data);
        } else {
            writeInt((int)data);
        }
    }

    void writeByteArray(byte[] data) {
        dataStream.write(data, 0, data.length);
    }

    void writeString(String string) {
        try {
            byte[] stringBytes = string.getBytes("UTF8");
            writeInt(stringBytes.length);
            writeByteArray(stringBytes);
        } catch (java.io.UnsupportedEncodingException e) {
            throw new RuntimeException("Cannot convert string to UTF8 bytes");
        }
    }

    /**
     * Read byte represented as one bytes.
     */
    byte readByte() {
        byte ret = pkt.data[inCursor];
        inCursor += 1;
        return ret;
    }

    /**
     * Read boolean represented as one byte.
     */
    boolean readBoolean() {
        byte ret = readByte();
        return (ret != 0);
    }

    /**
     * Read char represented as two bytes.
     */
    char readChar() {
        int b1, b2;

        b1 = pkt.data[inCursor++] & 0xff;
        b2 = pkt.data[inCursor++] & 0xff;

        return (char)((b1 << 8) + b2);
    }

    /**
     * Read short represented as two bytes.
     */
    short readShort() {
        int b1, b2;

        b1 = pkt.data[inCursor++] & 0xff;
        b2 = pkt.data[inCursor++] & 0xff;

        return (short)((b1 << 8) + b2);
    }

    /**
     * Read int represented as four bytes.
     */
    int readInt() {
        int b1,b2,b3,b4;

        b1 = pkt.data[inCursor++] & 0xff;
        b2 = pkt.data[inCursor++] & 0xff;
        b3 = pkt.data[inCursor++] & 0xff;
        b4 = pkt.data[inCursor++] & 0xff;

        return ((b1 << 24) + (b2 << 16) + (b3 << 8) + b4);
    }

    /**
     * Read long represented as eight bytes.
     */
    long readLong() {
        long b1,b2,b3,b4;
        long b5,b6,b7,b8;

        b1 = pkt.data[inCursor++] & 0xff;
        b2 = pkt.data[inCursor++] & 0xff;
        b3 = pkt.data[inCursor++] & 0xff;
        b4 = pkt.data[inCursor++] & 0xff;

        b5 = pkt.data[inCursor++] & 0xff;
        b6 = pkt.data[inCursor++] & 0xff;
        b7 = pkt.data[inCursor++] & 0xff;
        b8 = pkt.data[inCursor++] & 0xff;

        return ((b1 << 56) + (b2 << 48) + (b3 << 40) + (b4 << 32)
                + (b5 << 24) + (b6 << 16) + (b7 << 8) + b8);
    }

    /**
     * Read double represented as eight bytes.
     */
    double readDouble() {
        return Double.longBitsToDouble(readLong());
    }

    /**
     * Read string represented as four byte length followed by
     * characters of the string.
     */
    String readString() {
        String ret;
        int len = readInt();

        try {
            ret = new String(pkt.data, inCursor, len, "UTF8");
        } catch(IndexOutOfBoundsException e ) {
            System.err.println(e);
            ret = "IndexOutOfBoundsExceptions";
        } catch(java.io.UnsupportedEncodingException e) {
            System.err.println(e);
            ret = "Conversion error!";
        }
        inCursor += len;
        return ret;
    }

    private long readID(int size) {
        if (size == 8) {
            return readLong();
        } else {  // Other sizes???
            return readInt();
        }
    }

    int skipBytes(int n) {
        inCursor += n;
        return n;
    }

    byte command() {
        return (byte)pkt.cmd;
    }

}

