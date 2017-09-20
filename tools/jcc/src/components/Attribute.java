/*
 *    Attribute.java    1.7    03/01/14
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package components;
import java.io.DataOutput;
import java.io.DataInput;
import java.io.IOException;
import java.util.Hashtable;
import util.*;

/*
 * A class to represent the general Attribute form.
 * In its most uninterpreted form, "data" is just an
 * array of byte. It can also be other things, even
 * array of ConstantObject. 
 */

public abstract
class Attribute extends ClassComponent
{
    public UnicodeConstant name;
    public int           length;

    protected
    Attribute( UnicodeConstant n, int l ){
    name = n;
    length = l;
    resolved = true;
    }

    public void
    externalize( ConstantPool p ){
    if ( name != null ){
        name = (UnicodeConstant)p.add( name );
    }
    }

    abstract protected int
    writeData( DataOutput o ) throws IOException;

    public void write( DataOutput o ) throws IOException {
    o.writeShort( name.index );
    o.writeInt( length );
    int trueLength = writeData( o );
    if ( length != trueLength ){
        throw new DataFormatException(Localizer.getString("attribute.bad_attribute_length_for.dataformatexception", name ));
    }
    }

    public void countConstantReferences( boolean isRelocatable ){
    //
    // if we are producing relocatable output, then
    // we will need our name in the string table.
    // Else not.
    //
    if ( isRelocatable )
        name.incReference();
    }

    /*
     * Number of bytes we'll write out
     */
    public int length(){
    return 6+length; // believe length unchanged
    }

    public static void
    externalizeAttributes( Attribute a[], ConstantPool p ){
    int arryl = ( a == null ) ? 0 : a.length;
    for ( int i = 0; i < arryl; i++ ){
        a[i].externalize(p);
    }
    }

    public static void
    writeAttributes( Attribute a[], DataOutput o,  boolean verbose ) throws IOException {
    int nattr =  ( a == null ) ? 0 : a.length;
    if ( verbose ){
        System.out.println(Localizer.getString("attribute.writing_attributes", Integer.toString(nattr)));
    }
    o.writeShort( nattr );
    for ( int j = 0; j < nattr; j++ ){
        if ( verbose ){
        System.out.println("    "+a[j].name.string );
        }
        a[j].write( o );
    }
    }

    public static int
    length( Attribute a[] ){
    int l = 0;
    int arryl = ( a == null ) ? 0 : a.length;
    for ( int i = 0; i < arryl; i++ ){
        l += a[i].length();
    }
    return l + 2;
    }

    public static Attribute[] 
    readAttributes( DataInput i, ConstantObject locals[], ConstantObject globals[], Hashtable typetable, boolean verbose ) throws IOException {
    int nattr =  i.readUnsignedShort();
    if ( verbose ){
        System.out.println(Localizer.getString("attribute.reading_attributes", Integer.toString(nattr)));
    }
    if ( nattr == 0 ) return null;
    Attribute a[] = new Attribute[nattr];
    for ( int j = 0; j < nattr; j++ ){
        UnicodeConstant name = (UnicodeConstant)globals[i.readUnsignedShort()];
        String typename = name.string.intern();
        AttributeFactory afact = (AttributeFactory)typetable.get( typename );
        if ( afact == null )
        afact = UninterpretedAttributeFactory.instance;

        a[j] = afact.finishReadAttribute( i, name, locals, globals );
    }
    return a;
    }

    public static void
    countConstantReferences( Attribute a[], boolean isRelocatable ){
    if ( a == null ) return;
    for ( int i = 0; i < a.length; i++ ){
        a[i].countConstantReferences( isRelocatable );
    }
    }

}
