/*
 *    ClassComponent.java    1.3    03/01/14 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package components;
import java.io.DataOutput;
import java.io.IOException;

/*
 * An abstract class for representing components of a class
 * This include any field, method, or "constant".
 */

public
abstract class ClassComponent
{
    // whether or not "resolved" has been called, 
    // which sometimes determines interpretation
    public boolean resolved = false;

    abstract public void write( DataOutput o ) throws IOException;

    public void resolve( ConstantObject table[] ){
    // by default, just note that we're resolved.
    resolved = true;
    }

    public void
    externalize( ConstantPool p ){
    // by default, there's nothing to do
    }

}
