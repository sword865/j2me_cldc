/*
 *    UnicodeConstant.java    1.3    99/03/03 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package components;
import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import vm.Const;

// Class that represents a CONSTANT_UTF8 in a classfile's
// constant pool

public
class UnicodeConstant extends ConstantObject
{
    public String string;
    public String UTFstring;
    public int      stringTableOffset;    // used by in-core output writers
    public boolean isSuffix = false;    // used by in-core output writers

    private UnicodeConstant( int t, String s ){
    tag = t;
    string = s;
    nSlots = 1;
    }

    public UnicodeConstant( String s ){
    this( Const.CONSTANT_UTF8, s );
    }

    public static ConstantObject read( int t, DataInput i ) throws IOException{
    return new UnicodeConstant( t, i.readUTF() );
    }

    public void write( DataOutput o ) throws IOException{
    o.writeByte( tag );
    o.writeUTF( string );
    }

    public int hashCode() {
    return string.hashCode();
    }

    public boolean equals(Object o) {
    if (o instanceof UnicodeConstant) {
        UnicodeConstant a = (UnicodeConstant) o;
        return string.equals(a.string);
    } else {
        return false;
    }
    }

    public String toString() {
    return string;
    }

    public boolean isResolved(){ return true; }
}
