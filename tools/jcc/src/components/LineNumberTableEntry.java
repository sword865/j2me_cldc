/*
 *    LineNumberTableEntry.java    1.4    03/01/14 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package components;
import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;

/*
 * Classes compiled with -g have line number attributes on their
 * code. These give the correspondence between Java bytecode relative PC
 * and source lines (when used with the filename, too!)
 */
public
class LineNumberTableEntry {
    public int startPC;
    public int lineNumber;

    public static final int size = 4; // bytes in class files

    LineNumberTableEntry( int s, int l ){
    startPC = s;
    lineNumber = l;
    }

    public void write( DataOutput o ) throws IOException {
    o.writeShort( startPC );
    o.writeShort( lineNumber );
    }
}

