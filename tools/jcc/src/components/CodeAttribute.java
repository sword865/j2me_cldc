/*
 *    CodeAttribute.java    1.8    03/01/14
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package components;
import java.io.DataOutput;
import java.io.DataInput;
import java.io.IOException;
import util.DataFormatException;
import java.util.Hashtable;

/*
 * A class to represent the Code Attribute
 * of a method
 */

public
class CodeAttribute extends Attribute
{
    public int          stack;
    public int          locals;
    public byte          code[];
    public ExceptionEntry exceptionTable[];
    public Attribute      codeAttributes[];

    public
    CodeAttribute( UnicodeConstant name, int l, int ns, int nl, byte c[],
    ExceptionEntry et[], Attribute ca[] ){
    super( name, l );
    stack  = ns;
    locals = nl;
    code   = c;
    exceptionTable = et;
    codeAttributes = ca;
    }

    public void
    externalize( ConstantPool p ){
    super.externalize( p );
    Attribute.externalizeAttributes( codeAttributes, p );
    }

    public void
    countConstantReferences( boolean isRelocatable ){
    super.countConstantReferences( isRelocatable );
    if ( exceptionTable != null ){
        for( int i = 0; i < exceptionTable.length; i++ ){
        exceptionTable[i].countConstantReferences();
        }
    }
    Attribute.countConstantReferences( codeAttributes, isRelocatable );
    }

    protected int
    writeData( DataOutput o ) throws IOException{
    int trueLength = 8 + code.length;
    o.writeShort( stack );
    o.writeShort( locals );
    o.writeInt( code.length );
    o.write( code, 0, code.length );
    if ( exceptionTable == null ){
        o.writeShort( 0 );
        trueLength += 2;
    } else {
        o.writeShort( exceptionTable.length );
        for ( int i = 0; i < exceptionTable.length ; i ++ ){
        exceptionTable[i].write( o );
        }
        trueLength += 2 + exceptionTable.length*ExceptionEntry.size;
    }
    if ( codeAttributes == null ){
        o.writeShort(0);
        trueLength += 2;
    } else {
        Attribute.writeAttributes( codeAttributes, o, false );
        trueLength += Attribute.length( codeAttributes );
    }
    return trueLength;
    }

    /*
     * This hashtable is for use when reading code attributes.
     * The only code attribute we're interested in is
     * the LineNumberTableAttribute.
     * Other stuff we ignore.
     */
    static private Hashtable codeAttributeTypes = new Hashtable();
    static{
    codeAttributeTypes.put( "LineNumberTable", LineNumberTableAttributeFactory.instance );
    codeAttributeTypes.put( "LocalVariableTable", LocalVariableTableAttributeFactory.instance );
    codeAttributeTypes.put( "StackMap", StackMapAttributeFactory.instance );
    }

    public static Attribute
    readAttribute( DataInput i, ConstantObject locals[], ConstantObject globals[] ) throws IOException{
    UnicodeConstant name;

    name = (UnicodeConstant)globals[i.readUnsignedShort()];
    return finishReadAttribute( i, name, locals, globals );
    }

    //
    // for those cases where we already read the name index
    // and know that its not something requiring special handling.
    //
    public static Attribute
    finishReadAttribute( DataInput in, UnicodeConstant name, ConstantObject locals[], ConstantObject globals[] ) throws IOException {
    int l;
    int nstack;
    int nlocals;
    ConstantObject d;
    ExceptionEntry exceptionTable[];

    l       = in.readInt();
    nstack  = in.readUnsignedShort();
    nlocals = in.readUnsignedShort();

    int codesize = in.readInt();
    byte code[] = new byte[ codesize ];
    in.readFully( code );

    int tableSize = in.readUnsignedShort();
    exceptionTable = new ExceptionEntry[tableSize];
    for (int j = 0; j < tableSize; j++) {
        int sPC = in.readUnsignedShort();
        int e = in.readUnsignedShort();
        int h = in.readUnsignedShort();
        int catchTypeIndex = in.readUnsignedShort();
        ClassConstant ctype;
        if ( catchTypeIndex == 0 ){
        ctype = null;
        } else {
        ctype = (ClassConstant)locals[ catchTypeIndex ];
        }
        exceptionTable[j] = new ExceptionEntry( 
        sPC, e, h, ctype );
    }

    Attribute a[] = Attribute.readAttributes( in, locals, globals, codeAttributeTypes, false );
    return new CodeAttribute(
        name, l, nstack, nlocals, code, exceptionTable, a );
    }

}
