/*
 *    FieldInfo.java    1.7    99/04/06 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package components;
import java.io.DataOutput;
import java.io.DataInput;
import java.io.IOException;
import java.util.Hashtable;
import util.DataFormatException;
import vm.Const;

/*
 * A Field of a class. Includes statics, static finals, and
 * of course instance fields.
 * In Java, we don't have enumerations, but we do have
 * static final fields, and these have a value attribute.
 */
public
class FieldInfo extends ClassMemberInfo {

    private Attribute       fieldAttributes[];
    public ConstantObject value;
    private ConstantValueAttribute  valueAttribute;
    public int          instanceOffset = -1; // for instance fields only, of course
    public int        nSlots = -1;         // ditto

    public
    FieldInfo( int name, int sig, int access, ClassInfo p ){
    super( name, sig, access, p );
    }

    /*
     * The only field attribute we care about is
     * the ConstantValue attribute
     */
    private static Hashtable fieldAttributeTypes = new Hashtable();
    static{
    fieldAttributeTypes.put("ConstantValue", ConstantValueAttributeFactory.instance );
    }

    // Read attributes from classfile
    private void
    readAttributes( DataInput in ) throws IOException {
    fieldAttributes = Attribute.readAttributes( in, parent.constants, parent.symbols, fieldAttributeTypes, false );
    }

    // Read field from classfile
    public static FieldInfo 
    readField( DataInput in, ClassInfo p ) throws IOException {
    int acc  = in.readUnsignedShort();
    int name = in.readUnsignedShort();
    int sig =  in.readUnsignedShort();
    FieldInfo fi = new FieldInfo( name, sig, acc, p );
    fi.readAttributes(in );
    fi.resolve( p.symbols );
    return fi;
    }

    public void resolve( ConstantObject table[] ){
    if ( resolved ) return;
    super.resolve( table );
    /*
     * Parse attributes.
     * If we find a value attribute, pick it out
     * for special handling.
     */
    if ( fieldAttributes != null ) {
        for ( int i = 0; i < fieldAttributes.length; i++ ){
        Attribute a = fieldAttributes[i];
        if (a.name.string.equals("ConstantValue") ) {
            valueAttribute = (ConstantValueAttribute)a;
            value = valueAttribute.data;
        }
        }
    }
    switch( type.string.charAt(0) ){
    case 'D':
    case 'J':
        nSlots = 2; access |= Const.ACC_DOUBLEWORD; break;
    case 'L':
    case '[':
        nSlots = 1; access |= Const.ACC_REFERENCE; break;
    default:
        nSlots = 1; break;
    }
    resolved = true;
    }

    public void externalize( ConstantPool p ){
    super.externalize( p );
    Attribute.externalizeAttributes( fieldAttributes, p );
    if ( value != null ){
        value = valueAttribute.data;
    }
    }

    public void
    countConstantReferences( boolean isRelocatable ){
    super.countConstantReferences();
    Attribute.countConstantReferences( fieldAttributes, isRelocatable );
    }

    public void
    write( DataOutput o ) throws IOException {
    o.writeShort( access );
    if ( resolved ){
        o.writeShort( name.index );
        o.writeShort( type.index );
        Attribute.writeAttributes( fieldAttributes, o, false );
    } else {
        o.writeShort( nameIndex );
        o.writeShort( typeIndex );
        o.writeShort( 0 ); // no attribute!
    }
    }

    public String toString(){
    String r = "Field: "+super.toString();
    if ( value != null ){
        r += " Value: "+value.toString();
    }
    return r;
    }

}
