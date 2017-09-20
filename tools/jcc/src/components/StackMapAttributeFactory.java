/*
 *    StackMapAttributeFactory.java    1.3    03/01/14
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package components;
import java.io.DataInput;
import java.io.IOException;

class StackMapAttributeFactory extends AttributeFactory {

    public static AttributeFactory instance = new StackMapAttributeFactory();

    Attribute finishReadAttribute(
    DataInput in, UnicodeConstant name, ConstantObject locals[], ConstantObject globals[] )
    throws IOException{
        return StackMapAttribute.finishReadAttribute(in, name, globals );
    }
}
