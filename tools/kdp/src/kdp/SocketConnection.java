/*
 * @(#)SocketConnection.java    1.7 99/05/21
 * 
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 * 
 */

package kdp;

import kdp.classparser.*;
import java.net.*;
import java.io.*;
import java.util.*;

class SocketConnection implements Runnable {
    Socket socket;
    DataOutputStream out;
    DataInputStream in;
    ProxyListener proxy;

    SocketConnection(ProxyListener proxy, Socket socket) throws IOException {
    this.proxy = proxy;
        this.socket = socket;
        socket.setTcpNoDelay(true);
        in = new DataInputStream(new BufferedInputStream(socket.getInputStream()));
        out = new DataOutputStream(new BufferedOutputStream(socket.getOutputStream()));
    }

    public void close() {
        try {
            out.flush();
            out.close();
            in.close();
            socket.close();
        } catch (Exception e) {
            ;
        }
    }

    public byte receiveByte() throws IOException {
            int b = in.read();
            return (byte)b;
    }

    public void sendByte(byte b) throws IOException {
            out.write(b);
            out.flush();
        }

    public void run() {
    Thread.currentThread().setPriority(Thread.MAX_PRIORITY);
    try {
        while (true) {
        Packet p = receivePacket();
        if ((p.flags & Packet.Reply) == 0 || p.id >= 0) {
            proxy.newPacket(p);
        } else {
            proxy.replyReceived(p);
        }
        }
    } catch (Exception e) {
        try {
            Log.LOGN(2, "Socket exception in " + proxy + e + " ...exiting");
//                  e.printStackTrace();
        proxy.newPacket(null);
        proxy.replyReceived(null);
                    return;
        } catch (Exception ignore) {}
    }
    }

    public Packet receivePacket() throws IOException {

            Packet p = new Packet();
            int b1,b2,b3,b4;
    
            // length
            b1 = in.read();
            b2 = in.read();
            b3 = in.read();
            b4 = in.read();
    
            if (b1<0 || b2<0 || b3<0 || b4<0)
                throw new EOFException();
    
            int length = ((b1 << 24) + (b2 << 16) + (b3 << 8) + (b4 << 0));
    
            // id
            b1 = in.read();
            b2 = in.read();
            b3 = in.read();
            b4 = in.read();
    
            if (b1<0 || b2<0 || b3<0 || b4<0)
                throw new EOFException();
    
            p.id = ((b1 << 24) + (b2 << 16) + (b3 << 8) + (b4 << 0));
    
            p.flags = (short)in.read();
            if (p.flags < 0) {
                throw new EOFException();
            }
            if ((p.flags & Packet.Reply) == 0) {
                p.cmdSet = (short)in.read();
                p.cmd = (short)in.read();
                if (p.cmdSet < 0 || p.cmd < 0){
                    throw new EOFException();
                }
            } else {
                b1 = in.read();
                b2 = in.read();
                if (b1 < 0 || b2 < 0){
                    throw new EOFException();
                }
                p.errorCode = (short)((b1 << 8) + (b2 << 0));
            }
    
            length -= 11; // subtract the header
    
            if (length < 0) {
                // WHoa! this shouldn't be happening!
                System.err.println("length is " + length);
                System.err.println("Read is " + in.read());
            }
            p.data = new byte[length];
    
            int n = 0;
            while (n < p.data.length) {
                int count = in.read(p.data, n, p.data.length - n);
                if (count < 0) {
                    throw new EOFException();
                }
                n += count;
            }
    
            return p;
    }

    public void send(Packet p) throws IOException {

            int length = p.data.length + 11;
    
            // Length
            out.write((length >>> 24) & 0xFF);
            out.write((length >>> 16) & 0xFF);
            out.write((length >>>  8) & 0xFF);
            out.write((length >>>  0) & 0xFF);
    
            // id
            out.write((p.id >>> 24) & 0xFF);
            out.write((p.id >>> 16) & 0xFF);
            out.write((p.id >>>  8) & 0xFF);
            out.write((p.id >>>  0) & 0xFF);
    
            out.write(p.flags);
    
            if ((p.flags & Packet.Reply) == 0) {
                out.write(p.cmdSet);
                out.write(p.cmd);
            } else {
                out.write((p.errorCode >>>  8) & 0xFF);
                out.write((p.errorCode >>>  0) & 0xFF);
            }
            out.write(p.data);
    
        out.flush();
    }
}

