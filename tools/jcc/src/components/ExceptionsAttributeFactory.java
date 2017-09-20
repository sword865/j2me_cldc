/*
 *    ExceptionsAttributeFactory.java    1.2    03/01/14
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package components;
import java.io.DataInput;
import java.io.IOException;

class ExceptionsAttributeFactory extends AttributeFactory {

    public static AttributeFactory instance = new ExceptionsAttributeFactory();

    Attribute finishReadAttribute(
    DataInput in, UnicodeConstant name, ConstantObject locals[], ConstantObject globals[] )
    throws IOException{
        return ExceptionsAttribute.finishReadAttribute( in, name, locals );
    }
}
