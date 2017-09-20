/*
 *    LineNumberTableAttribute.java    1.4    03/01/14
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package components;
import java.io.DataOutput;
import java.io.DataInput;
import java.io.IOException;
import util.DataFormatException;

/*
 * A class to represent the LineNumber table Attribute
 * of a method's code.
 */

public
class LineNumberTableAttribute extends Attribute
{
    public LineNumberTableEntry    data[];

    public
    LineNumberTableAttribute( UnicodeConstant name, int l, LineNumberTableEntry d[] ){
    super( name, l );
    data = d;
    }

    protected int
    writeData( DataOutput o ) throws IOException{
    int n = data.length;
    o.writeShort( n );
    for ( int i = 0; i < n; i++ ){
        data[i].write( o );
    }
    return 2+LineNumberTableEntry.size*n;
    }

    public static Attribute
    readAttribute( DataInput i, ConstantObject t[] ) throws IOException{
    UnicodeConstant name;

    name = (UnicodeConstant)t[i.readUnsignedShort()];
    return finishReadAttribute( i, name );
    }

    //
    // for those cases where we already read the name index
    // and know that its not something requiring special handling.
    //
    public static Attribute
    finishReadAttribute( DataInput in, UnicodeConstant name ) throws IOException {
    int l;
    int n;
    LineNumberTableEntry d[];

    l  = in.readInt();
    n  = in.readUnsignedShort();
    d = new LineNumberTableEntry[ n ];
    for ( int i = 0; i < n; i++ ){
        d[i] = new LineNumberTableEntry( in.readUnsignedShort(), in.readUnsignedShort() );
    }
    return new LineNumberTableAttribute( name, l, d );
    }

}
