/*
 *    InterfaceConstant.java    1.3    03/01/14 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package components;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;

/*
 * Represents CONSTANT_InterfaceMethodref
 * There is very, very little difference between one of these
 * and a plain Method reference.
 */

public
class InterfaceConstant extends MethodConstant
{
    InterfaceConstant( int t ){
    //tag = t;
    super( t );
    }

    public static ConstantObject
    read( int t, DataInput in ) throws IOException {
    FMIrefConstant mc = new InterfaceConstant( t );
    mc.read( in );
    return mc;
    }

}
