/*
 *    FieldConstant.java    1.2    97/05/27 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package components;
import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import vm.Const;

// Represents a CONSTANT_Field

public
class FieldConstant extends FMIrefConstant
{
    // Reference to the actual field (need it for index into the
    // fieldblock for the field reference).
    boolean    didLookup;
    FieldInfo    theField;

    FieldConstant( int t ){ tag = t; }

    public FieldConstant( ClassConstant c, NameAndTypeConstant s ){
        super( Const.CONSTANT_FIELD, c, s );
    }

    public static ConstantObject
    read( int t, DataInput in ) throws IOException {
    FieldConstant f = new FieldConstant( t );
    f.read( in );
    return f;
    }

    public FieldInfo find(){
    if ( ! didLookup ){
        theField = (FieldInfo)super.find(false);
        didLookup = true;
    }
    return theField;
    }

}
