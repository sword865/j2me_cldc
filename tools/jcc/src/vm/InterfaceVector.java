/*
 *    InterfaceVector.java    1.5    03/01/14 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package vm;
import components.ClassInfo;
/*
 * A class's InterfaceMethodTable is an array of InterfaceVector,
 * giving the correspondence between the methods of the interface and the
 * methods of the containing class.
 * This could be implemented as an array-of-short, except that we
 * want to do sharing at runtime, so must tag each such array with an "owner",
 * for naming purposes.
 */

public class
InterfaceVector {
    public ClassClass    parent;
    public ClassInfo    intf;
    public short    v[];
    public boolean    generated; // for use of output writer.
    public int        offset;    // for use of output writer.

    public InterfaceVector( ClassClass p, ClassInfo i, short vec[] ){
    parent = p;
    intf = i;
    v = vec;
    generated = false;
    offset = -1; // clearly a bogus value.
    }
}
