/*
 * @(#)ConnectionService.java    1.5 99/05/21
 * 
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 * 
 */

package kdp;

import java.io.IOException;

/**
 * Connection SPI
 */
public interface ConnectionService {
    void close();
    byte receiveByte() throws IOException;
    void sendByte(byte b) throws IOException;

    // TO DO: Hacks for now; we will eventually have PacketService interfaces
    Packet receivePacket() throws IOException;
    void sendPacket(Packet p) throws IOException;
}

