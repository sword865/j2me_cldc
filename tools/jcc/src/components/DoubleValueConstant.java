/*
 *    DoubleValueConstant.java    1.2    97/05/27 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/*
 * Represents a CONSTANT_Double or CONSTANT_Long
 * Should be read or written one whole word after another.
 */
package components;
import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import vm.Const;

public
class DoubleValueConstant extends ConstantObject
{
    // The two words of the constant
    public int highVal;
    public int lowVal;

    private DoubleValueConstant( int t, int h, int l ){
    tag = t;
    highVal = h;
    lowVal = l;
    nSlots = 2;
    }

    public static ConstantObject read( int t, DataInput i ) throws IOException{
    return new DoubleValueConstant( t, i.readInt(), i.readInt() );
    }

    public void write( DataOutput o ) throws IOException{
    o.writeByte( tag );
    o.writeInt( highVal );
    o.writeInt( lowVal );
    }

    public String toString(){
    String t = (tag == Const.CONSTANT_DOUBLE)?"Double [ " : "Long [ ";
    return t + Integer.toHexString( highVal ) + " " + Integer.toHexString( lowVal ) +" ]";
    }

    public int hashCode() {
    return tag + highVal + lowVal;
    }

    public boolean equals(Object o) {
    if (o instanceof DoubleValueConstant) {
        DoubleValueConstant d = (DoubleValueConstant) o;
        return tag == d.tag && highVal == d.highVal && lowVal == d.lowVal;
    } else {
        return false;
    }
    }
    
    public boolean isResolved(){ return true; }
}
