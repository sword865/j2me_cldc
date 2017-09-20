/*
 *    ArrayClassInfo.java    1.14    99/12/08 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package vm;
import vm.Const;
import components.*;
import util.*;

/*
 * An array is a class.
 * It is a subclass of java.lang.Object.
 * It has all the class-related runtime data structures, or at least
 * may of them.
 * It is not read in from class files, but is made up on-the-fly
 * when the classresolver sees a reference to a classname starting with
 * "[".
 *
 * In order to resolve such references early, we must do likewise here.
 */
public
class ArrayClassInfo extends ClassInfo {

    private static int nFake = 0;
    public int         arrayClassNumber;
    public int         depth;
    public int         baseType;
    public String        baseName;
    public ClassConstant baseClass;
    public ClassConstant subarrayClass;
    

    /*
     * Given signature s,
     * fill in
     * depth ( i.e. number of opening [ )
     * basetype ( i.e. thing after the last [ )
     * and baseClass ( if basetype is a classtype )
     */
    private void fillInTypeInfo( String s ) throws DataFormatException {
    int index = 0;
    char c;
    while( ( c = s.charAt( index ) ) == Const.SIGC_ARRAY )
        index++;
    depth = index;
    switch( c ){
    case Const.SIGC_INT:
        baseType = Const.T_INT;
        baseName = "Int";
        break;
    case Const.SIGC_LONG:
        baseType = Const.T_LONG;
        baseName = "Long";
        break;
    case Const.SIGC_FLOAT:
        baseType = Const.T_FLOAT;
        baseName = "Float";
        break;
    case Const.SIGC_DOUBLE:
        baseType = Const.T_DOUBLE;
        baseName = "Double";
        break;
    case Const.SIGC_BOOLEAN:
        baseType = Const.T_BOOLEAN;
        baseName = "Boolean";
        break;
    case Const.SIGC_BYTE:
        baseType = Const.T_BYTE;
        baseName = "Byte";
        break;
    case Const.SIGC_CHAR:
        baseType = Const.T_CHAR;
        baseName = "Char";
        break;
    case Const.SIGC_SHORT:
        baseType = Const.T_SHORT;
        baseName = "Short";
        break;
    case Const.SIGC_CLASS:
        baseType = Const.T_CLASS;        
        int start = ++index;
        while (s.charAt(index) != Const.SIGC_ENDCLASS) {
        index++;
        }

        baseClass = new ClassConstant(
        new UnicodeConstant( s.substring( start, index ) ) 
        );
        break;

    default:
        throw new DataFormatException(Localizer.getString("arrayclassinfo.malformed_array_type_string.dataformatexception",s));
    }
    // For EVM, we want to know the sub-class, which is different from
    // the base class.
    if ( depth > 1 ){
        subarrayClass = new ClassConstant( new UnicodeConstant( s.substring(1) ) );
    } else if ( (depth == 1 ) && ( baseClass != null ) ){
        subarrayClass = baseClass;
    }

    }

    private void fakeConstantPool(){
    // 0 -- unused
    // 1 -- depth as integer
    // 2 -- base type tag as integer
    // 3 -- base class ref, if a ref type
    // 4 -- subarray class ref, if depth > 1
    //      base type if depth==1 && its a class ref array type
    // 5 -- self class ref
    // 6 -- UTF8 for entry 3
    // 7 -- UTF8 for entry 4
    // 8 -- UTF8 for entry 5

    // MAKE A CHANGE FOR KVM
    
    if (false) {
        int nconstants = 9;
        constants = new ConstantObject[nconstants];
        constants[1] = new SingleValueConstant( depth );
        constants[2] = new SingleValueConstant( baseType );
        if ( baseClass == null ){
        constants[3] = new SingleValueConstant( 0 );
        constants[6] = new SingleValueConstant( 0 );
        } else {
        constants[3] = baseClass;
        constants[6] = baseClass.name;
        }
        // for EVM, point at the subarray type.
        if ( subarrayClass == null ){
        constants[4] = new SingleValueConstant( 0 );
        constants[7] = new SingleValueConstant( 0 );
        } else {
        constants[4] = subarrayClass;
        constants[7] = subarrayClass.name;
        }
        constants[5] = thisClass; // EVM 4 => 5
        constants[8] = thisClass.name; // EVM 5 => 7
    } else { 
        int nconstants = 2;
        constants = new ConstantObject[nconstants];
        if (subarrayClass == null) { 
        constants[1] = new SingleValueConstant(baseType);
        } else { 
        constants[1] = subarrayClass;
        }
    }
    }

    public ArrayClassInfo( boolean v, String s ) throws DataFormatException {
    super(v);
    arrayClassNumber = nFake++;
    fillInTypeInfo( s );
    className = s;
    thisClass = new ClassConstant( new UnicodeConstant( s ) );
    superClassInfo = lookupClass(/*NOI18N*/"java/lang/Object");
    superClass = superClassInfo.thisClass;
    access = Const.ACC_FINAL|Const.ACC_ABSTRACT|Const.ACC_PUBLIC;
    fakeConstantPool();
    methods = new MethodInfo[0];
    fields  = new FieldInfo[0];
    enterClass( s );
    }

    public boolean
    countReferences( boolean v ){
    // some entries are magic. The others
    // can get shared or whacked.
    if (false) { 
        constants[1].incReference();    // depth
        constants[2].incReference();    // baseType code
        constants[3].incReference();    // base class pointer
        constants[4].incReference();    // subarray class pointer
    } else { 
        constants[1].incReference();    // base type 
    }
    return super.countReferences( v );
    }

    public void
    externalize( ConstantPool p ){
    if (verbose){
        log.println(Localizer.getString("arrayclassinfo.externalizing_class", className));
    }
    externalizeConstants( p );
    // DONT DO THISthisClass = (ClassConstant)p.dup( thisClass );
    if ( superClass != null ){
        superClass = (ClassConstant)p.dup( superClass );
        // THIS SHOULD WORK.
    }
    }

    protected String createGenericNativeName() { 
        return /*NOI18N*/"fakeArray" + arrayClassNumber;
    }
}
