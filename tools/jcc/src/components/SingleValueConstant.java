/*
 *    SingleValueConstant.java    1.2    97/05/27 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

// Represents a CONSTANT_Integer or CONSTANT_Float
package components;
import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import vm.Const;

public
class SingleValueConstant extends ConstantObject
{
    public int value;

    private SingleValueConstant( int t, int v ){
    tag = t;
    value = v;
    nSlots = 1;
    }

    public SingleValueConstant( int ival ){
    this( Const.CONSTANT_INTEGER, ival );
    }

    public static ConstantObject read( int t, DataInput i ) throws IOException{
    return new SingleValueConstant( t, i.readInt() );
    }

    public void write( DataOutput o ) throws IOException{
    o.writeByte( tag );
    o.writeInt( value );
    }

    public String toString(){
    return (tag==Const.CONSTANT_FLOAT)? ("Float: "+Float.intBitsToFloat(value) ):
        ("Int: "+Integer.toHexString( value ));
    }

    public int hashCode() {
    return tag + value;
    }

    public boolean equals(Object o) {
    if (o instanceof SingleValueConstant) {
        SingleValueConstant s = (SingleValueConstant) o;
        return tag == s.tag && value == s.value;
    } else {
        return false;
    }
    }

    public boolean isResolved(){ return true; }
}
