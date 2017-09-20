/*
 *    ExceptionEntry.java    1.4    99/04/06 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package components;
import java.io.DataOutput;
import java.io.IOException;
import vm.Const;
import util.DataFormatException;

/*
 * An ExceptionEntry represents a range of Java bytecode PC values,
 * a Java exception type, and an action to take should that exception be
 * thrown in that range.
 *
 * Exception entries are read by components.MethodInfo, though perhaps
 * that code should be moved here. At least we know how to write ourselves
 * out.
 */

public
class ExceptionEntry
{
    public ClassConstant catchType;

    public int startPC, endPC;
    public int handlerPC;

    public static final int size = 8; // bytes in class files

    ExceptionEntry( int s, int e, int h, ClassConstant c ){
    startPC = s;
    endPC = e;
    handlerPC= h;
    catchType = c;
    }

    public void write( DataOutput o ) throws IOException {
    o.writeShort( startPC );
    o.writeShort( endPC );
    o.writeShort( handlerPC );
    o.writeShort( (catchType==null) ? 0 : catchType.index );
    }

    /*
     * A class referenced from an ExceptionEntry
     * is in the local constant pool, not the shared one.
     *
     * Thus it must not be externalized, but must be counted.
     * These decisions could be exposed at a higher level, for some
     * savings in performance, and should be when I have the
     * courage of my convictions.
     */
    public void externalize( ConstantPool p ){
    // do nothing.
    }

    public void countConstantReferences( ){
    if ( catchType != null )
        catchType.incReference();
    }
}
