/*
 *    MethodConstant.java    1.2    97/05/27 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package components;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import vm.Const;

// This class represents a CONSTANT_Methodref

public
class MethodConstant extends FMIrefConstant {
    boolean    didLookup;
    MethodInfo    theMethod;

    MethodConstant( int t ){
    tag = t;
    }

    public MethodConstant( ClassConstant c, NameAndTypeConstant sig ){
    super( Const.CONSTANT_METHOD, c, sig );
    }

    public static ConstantObject
    read( int t, DataInput in ) throws IOException {
    FMIrefConstant mc = new MethodConstant( t );
    mc.read( in );
    return mc;
    }

    public MethodInfo find(){
    if ( ! didLookup ){
        theMethod = (MethodInfo)super.find(true);
        didLookup = true;
    }
    return theMethod;
    }

}
