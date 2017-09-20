/*
 *    EVMClassFactory.java    1.2    99/05/11 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package vm;
import vm.Const;
import components.ClassInfo;

public class EVMClassFactory implements VMClassFactory{
    
    public ClassClass newVMClass( ClassInfo c ){
    return new EVMClass( c );
    }

    private static boolean
    setType( String name, int value ){
    ClassInfo ci = ClassInfo.lookupClass( name );
    ClassClass cc;
    if ( (ci == null) || ( ( cc = ci.vmClass ) == null) ){
        return false;
    }
    ((EVMClass)cc).typeCode = value;
    return true;

    }

    public void
    setTypes(){
    setType( "void", Const.T_VOID );
    setType( "boolean", Const.T_BOOLEAN );
    setType( "char", Const.T_CHAR );
    setType( "byte", Const.T_BYTE );
    setType( "short", Const.T_SHORT );
    setType( "int", Const.T_INT );
    setType( "long", Const.T_LONG );
    setType( "float", Const.T_FLOAT );
    setType( "double", Const.T_DOUBLE );
    }
}
