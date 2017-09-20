/*
 *    SourceFileAttribute.java    1.4    03/01/14
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
 * A class to represent the SourceFile Attribute
 * of a class
 */

public
class SourceFileAttribute extends Attribute
{
    public UnicodeConstant    sourceName;

    public
    SourceFileAttribute( UnicodeConstant name, int l,
             UnicodeConstant sourceName ) {
    super( name, l );
    this.sourceName = sourceName;
    }

    public void
    externalize( ConstantPool p ){
    super.externalize( p );
    sourceName = (UnicodeConstant)p.add( sourceName );
    }

    protected int
    writeData( DataOutput o ) throws IOException{
    o.writeShort( sourceName.index );
    return 2;
    }

    public void
    countConstantReferences( boolean isRelocatable ){
    super.countConstantReferences( isRelocatable );
    sourceName.incReference();
    }

    public static Attribute
    readAttribute( DataInput i, ConstantObject globals[] ) throws IOException{
    UnicodeConstant name;

    name = (UnicodeConstant)globals[i.readUnsignedShort()];
    return finishReadAttribute( i, name, globals );
    }

    //
    // for those cases where we already read the name index
    // and know that its not something requiring special handling.
    //
    public static Attribute
    finishReadAttribute( DataInput i, UnicodeConstant name, ConstantObject globals[] ) throws IOException {
    int l;
    int n;
    UnicodeConstant d;

    l  = i.readInt();
    n  = i.readUnsignedShort();
    d  = (UnicodeConstant)globals[n];
    return new SourceFileAttribute( name, l, d );
    }

}
