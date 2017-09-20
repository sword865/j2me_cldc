/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package com.sun.cldc.io.j2me.socket;

import java.io.*;
import javax.microedition.io.*;
import com.sun.cldc.io.*;

/**
 * Connection to the J2ME socket API.
 *
 * @author  Nik Shaylor
 * @author  Efren Serra
 * @version 1.0 1/16/2000
 */

public class Protocol implements ConnectionBaseInterface, StreamConnection {

    /**********************************************************\
     * WARNING - 'handle' MUST be the first instance variable *
     *           It is used by native code that assumes this. *
    \**********************************************************/

    /** Socket object used by native code */
    int handle;

    /** Private variable the native code uses */
    int iocb;

    /** Access mode */
    private int mode;

    /** Open count */
    int opens = 0;

    /** Connection open flag */
    private boolean copen = false;

    /** Input stream open flag */
    protected boolean isopen = false;

    /** Output stream open flag */
    protected boolean osopen = false;

    /** Size of the read ahead / write behind buffers */
    public static int bufferSize = 0; /* Default is no buffering */

    /**
     * Class initializer
     */
    static {
        initializeInternal();

        /* See if a read ahead / write behind buffer size has been specified */
        String size = System.getProperty("com.sun.cldc.io.j2me.socket.buffersize");
        if (size != null) {
            try {
                bufferSize = Integer.parseInt(size);
            } catch(NumberFormatException ex) {}
        }
    }

    /* This class allows one to initialize the network, if necessary,
     * before any networking code is called
    */
    private static native void initializeInternal();

    /**
     * Open the connection
     */
    public void open(String name, int mode, boolean timeouts)
        throws IOException {

        throw new RuntimeException("Should not be called");
    }

    /**
     * Open the connection
     * @param name       the target for the connection
     * @param writeable  a flag that is true if the caller expects
     *                   to write to the connection
     * @param timeouts   a flag to indicate that the caller wants
     *                   timeout exceptions
     * <p>
     * The name string for this protocol should be:
     * "socket://&lt;name or IP number&gt;:&lt;port number&gt;
     *
     * We allow "socket://:nnnn" to mean "serversocket://:nnnn".
     * The native code will spot this and signal back with an
     * InterruptedException to tell the calling Java code about this.
     */
    public Connection openPrim(String name, int mode, boolean timeouts)
        throws IOException {

        try {
            open0(name, mode, timeouts);
            registerCleanup();
            opens++;
            copen = true;
            this.mode = mode;
            return this;
        } catch(InterruptedException x) {
//            com.sun.cldc.io.j2me.serversocket.Protocol con;
//            con = new com.sun.cldc.io.j2me.serversocket.Protocol();
//            con.open(name, mode, timeouts);
//            return con;
              return null;
        }
    }

    /**
     * Open the connection
     *
     * @param handle an already formed socket handle
     * <p>
     * This function is only used by com.sun.cldc.io.j2me.socketserver;
     */
    public void open(int handle, int mode) throws IOException {
        this.handle = handle;
        opens++;
        copen = true;
        this.mode = mode;
    }

    /**
     * Ensure connection is open
     */
    void ensureOpen() throws IOException {
        if (!copen) {
            throw new IOException("Connection closed");
        }
    }

    /**
     * Returns an input stream for this socket.
     *
     * @return     an input stream for reading bytes from this socket.
     * @exception  IOException  if an I/O error occurs when creating the
     *                          input stream.
     */
    synchronized public InputStream openInputStream() throws IOException {

        ensureOpen();

        if ((mode&Connector.READ) == 0) {
            throw new IOException("Connection not open for reading");
        }

        if (isopen) {
            throw new IOException("Input stream already opened");
        }

        isopen = true;
        InputStream in;

        if (bufferSize == 0) {
            in = new PrivateInputStream(this);
        } else {
            in = new PrivateInputStreamWithBuffer(this, bufferSize);
        }

        opens++;
        return in;
    }

    /**
     * Returns an output stream for this socket.
     *
     * @return     an output stream for writing bytes to this socket.
     * @exception  IOException  if an I/O error occurs when creating the
     *                          output stream.
     */
    synchronized public OutputStream openOutputStream() throws IOException {
        ensureOpen();

        if ((mode&Connector.WRITE) == 0) {
            throw new IOException("Connection not open for writing");
        }

        if (osopen) {
            throw new IOException("Output stream already opened");
        }

        osopen = true;
        OutputStream os;

        os = new PrivateOutputStream(this);

        opens++;
        return os;
    }

    /**
     * Open and return a data input stream for a connection.
     *
     * @return                 An input stream
     * @exception IOException  If an I/O error occurs
     */
    public DataInputStream openDataInputStream() throws IOException {
        return new DataInputStream(openInputStream());
    }

    /**
     * Open and return a data output stream for a connection.
     *
     * @return                 An input stream
     * @exception IOException  If an I/O error occurs
     */
    public DataOutputStream openDataOutputStream() throws IOException {
        return new DataOutputStream(openOutputStream());
    }

    /**
     * Close the connection.
     *
     * @exception  IOException  if an I/O error occurs when closing the
     *                          connection.
     */
    synchronized public void close() throws IOException {
        if (copen) {
            copen = false;
            realClose();
        }
    }

    /**
     * Close the connection.
     *
     * @exception  IOException  if an I/O error occurs.
     */
    synchronized void realClose() throws IOException {
        if (--opens == 0) {
             close0();
        }
    }

   /*
    * A note about read0() and write0()
    *
    * These routines will return the number of bytes transferred. It this
    * value is zero then it means that the data could not be read or written
    * and the calling code should call Waiter.waitForIO() to let some other
    * thread run.
    */

    protected native void open0(String name, int mode, boolean timeouts)
        throws IOException, InterruptedException;
    protected native int  read0(byte b[], int off, int len)
        throws IOException;
    protected native int  write0(byte b[], int off, int len)
        throws IOException;

    protected native int  available0() throws IOException;
    protected native void close0() throws IOException;
    private native void registerCleanup();
}

/**
 * Input stream for the connection
 */
class PrivateInputStream extends InputStream {

    /**
     * The reference to the protocol.
     */
    protected Protocol parent;

    /**
     * The end of file flag.
     */
    boolean eof = false;

    /**
     * Single byte buffer.
     */
    byte[] bytebuf;

    /**
     * Constructor
     * @param pointer to the connection object
     *
     * @exception  IOException  if an I/O error occurs.
     */
    public PrivateInputStream(Protocol parent) throws IOException {
        this.parent = parent;
    }

    /**
     * Check the stream is open
     *
     * @exception  IOException  if the stream is not open.
     */
    void ensureOpen() throws IOException {
        if (parent == null) {
            throw new IOException("Stream closed");
        }
    }

    /**
     * Reads the next byte of data from the input stream.
     *
     * @return     the next byte of data, or <code>-1</code> if the end of the
     *             stream is reached.
     * @exception  IOException  if an I/O error occurs.
     */
    synchronized public int read() throws IOException {
        if (bytebuf == null) {
            bytebuf = new byte[1];
        }
        int res = read(bytebuf, 0, 1);
        if (res == 1) {
            return bytebuf[0] & 0xFF;
        } else {
            return res;
        }
    }

    /**
     * Reads up to <code>len</code> bytes of data from the input stream into
     * an array of bytes.
     * <p>
     * Polling the native code is done here to allow for simple
     * asynchronous native code to be written. Not all implementations
     * work this way (they block in the native code) but the same
     * Java code works for both.
     *
     * @param      b     the buffer into which the data is read.
     * @param      off   the start offset in array <code>b</code>
     *                   at which the data is written.
     * @param      len   the maximum number of bytes to read.
     * @return     the total number of bytes read into the buffer, or
     *             <code>-1</code> if there is no more data because the end of
     *             the stream has been reached.
     * @exception  IOException  if an I/O error occurs.
     */
    synchronized public int read(byte b[], int off, int len)
        throws IOException {

        ensureOpen();
        if (eof) {
            return -1;
        }
        if (b == null) {
            throw new NullPointerException();
        }
        if (len == 0) {
            return 0;
        }
        if (len <0) {
            throw new IndexOutOfBoundsException();
        }
        for (;;) {
            int count = read1(b, off, len);
            if (parent == null) {
                throw new InterruptedIOException();
            }
            if (count != 0) {
                if (count < 0) {
                    eof = true;
                }
                return count;
            }
            Waiter.waitForIO(); /* Wait a while for I/O to become ready */
        }
    }

   /*
    * read1
    */
    protected int read1(byte b[], int off, int len) throws IOException {
        if (parent != null) {
            return parent.read0(b, off, len);
        } else {
            return -1;
        }
    }

    /**
     * Returns the number of bytes that can be read (or skipped over) from
     * this input stream without blocking by the next caller of a method for
     * this input stream.
     *
     * @return     the number of bytes that can be read from this input stream.
     * @exception  IOException  if an I/O error occurs.
     */
    synchronized public int available() throws IOException {
        ensureOpen();
        return parent.available0();
    }

    /**
     * Close the stream.
     *
     * @exception  IOException  if an I/O error occurs
     */
    public void close() throws IOException {
        if (parent != null) {
            ensureOpen();
            parent.realClose();
            parent.isopen = false;
            parent = null;
        }
    }
}

/**
 * Input stream for the connection
 */
class PrivateInputStreamWithBuffer extends PrivateInputStream {

    /**
     * The internal buffer array where the data is stored.
     * When necessary, it may be replaced by another array
     * of a different size.
     */
    protected byte buf[];

    /**
     * The index one greater than the index of the last valid
     * byte in the buffer.
     * This value is always in the range
     * <code>0</code> through <code>buf.length</code>;
     * elements <code>buf[0]</code> through <code>buf[count-1]
     * </code>contain buffered input data obtained
     * from the underlying input stream.
     */
    protected int count;

    /**
     * The current position in the buffer. This is the index
     * of the next character to be read from the
     * <code>buf</code> array.
     * <p>
     * This value is always in the range <code>0</code>
     * through <code>count</code>. If it is less
     * than <code>count</code>, then <code>buf[pos]</code>
     * is the next byte to be supplied as input;
     * if it is equal to <code>count</code>, then
     * the  next <code>read</code> or <code>skip</code>
     * operation will require more bytes to be
     * read from the contained input stream.
     */
    protected int pos;

    /**
     * Constructor
     * @param pointer to the connection object
     *
     * @exception  IOException  if an I/O error occurs.
     */
    public PrivateInputStreamWithBuffer(Protocol parent, int bufferSize)
        throws IOException {

        super(parent);
        buf = new byte[bufferSize];
    }

   /*
    * read1
    */
    protected int read1(byte b[], int off, int len) throws IOException {
        if (count == 0) {
            if (len >= buf.length) {
                return super.read1(b, off, len);
            } else {
                pos = 0;
                int res = super.read1(buf, pos, buf.length);
                if (res <= 0) {
                    return res;
                } else {
                    count = res;
                }
            }
        }
        if (len > count) {
            len = count;
        }
        System.arraycopy(buf, pos, b, off, len);
        count -= len;
        pos   += len;
        return len;
    }

    /**
     * Returns the number of bytes that can be read (or skipped over) from
     * this input stream without blocking by the next caller of a method for
     * this input stream.
     *
     * @return     the number of bytes that can be read from this
     *             input stream.
     * @exception  IOException  if an I/O error occurs.
     */
    public synchronized int available() throws IOException {
        ensureOpen();
        return count + super.available();
    }
}

/**
 * Output stream for the connection
 */
class PrivateOutputStream extends OutputStream {

    /**
     * Pointer to the connection
     */
    protected Protocol parent;

    /**
     * Single byte buffer.
     */
    byte[] bytebuf;

    /**
     * Constructor
     * @param pointer to the connection object
     *
     * @exception  IOException  if an I/O error occurs.
     */
    public PrivateOutputStream(Protocol parent) throws IOException {
        this.parent = parent;
    }

    /**
     * Check the stream is open
     *
     * @exception  IOException  if it is not.
     */
    void ensureOpen() throws IOException {
        if (parent == null) {
            throw new IOException("Stream closed");
        }
    }

    /**
     * Writes the specified byte to this output stream.
     * <p>
     * Polling the native code is done here to allow for simple
     * asynchronous native code to be written. Not all implementations
     * work this way (they block in the native code) but the same
     * Java code works for both.
     *
     * @param      b   the <code>byte</code>.
     * @exception  IOException  if an I/O error occurs. In particular,
     *             an <code>IOException</code> may be thrown if the
     *             output stream has been closed.
     */
    synchronized public void write(int b) throws IOException {
        if (bytebuf == null) {
            bytebuf = new byte[1];
        }
        bytebuf[0] = (byte)b;
        write(bytebuf, 0, 1);
    }

    /**
     * Writes <code>len</code> bytes from the specified byte array
     * starting at offset <code>off</code> to this output stream.
     * <p>
     * Polling the native code is done here to allow for simple
     * asynchronous native code to be written. Not all implementations
     * work this way (they block in the native code) but the same
     * Java code works for both.
     *
     * @param      b     the data.
     * @param      off   the start offset in the data.
     * @param      len   the number of bytes to write.
     * @exception  IOException  if an I/O error occurs. In particular,
     *             an <code>IOException</code> is thrown if the output
     *             stream is closed.
     */
    synchronized public void write(byte b[], int off, int len)
        throws IOException {

        ensureOpen();
        if (b == null) {
            throw new NullPointerException();
        }
        if (len == 0) {
            return;
        }
        int n = 0;
        while (true) {
            int count = write1(b, off + n, len - n);
            n += count;
            if (n == len) {
                break;
            }
            if (count == 0) {
                Waiter.waitForIO(); /* Wait a while for I/O to become ready */
            }
        }
    }

   /**
    * write1
    */
    protected int write1(byte b[], int off, int len) throws IOException {
        if (parent == null) {
            throw new InterruptedIOException();
        }
        return parent.write0(b, off, len);
    }

    /**
     * Close the stream.
     *
     * @exception  IOException  if an I/O error occurs
     */
    public void close() throws IOException {
        if (parent != null) {
            ensureOpen();
            flush();
            parent.realClose();
            parent.osopen = false;
            parent = null;
        }
    }
}

