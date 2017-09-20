/*
 *    LocalVariableTableEntry.java    1.3    03/01/14
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
class LocalVariableTableEntry {
    public int pc0;
    public int length;
    public UnicodeConstant name;
    public UnicodeConstant sig;
    public int slot;

    public static final int size = 10; // bytes in class files

    LocalVariableTableEntry( int p, int l, UnicodeConstant n,
                 UnicodeConstant si, int sl ){
    pc0 = p;
    length = l;
    name = n;
    sig = si;
    slot = sl;
    }

    public void write( DataOutput o ) throws IOException {
    o.writeShort( pc0 );
    o.writeShort( length );
    o.writeShort( name.index );
    o.writeShort( sig.index );
    o.writeShort( slot );
    }
}

