/*
 *    InterfaceMethodFactory.java    1.4    03/01/14 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package vm;
/*
 * Provide a way to call back to make an InterfaceMethodTable
 * Annoying artifact of Java.
 * See InterfaceMethodTable and its target-specific subclasses, such
 * as coreimage.SPARCInterfaceMethodTable
 */
public interface InterfaceMethodFactory {
    public  InterfaceMethodTable newInterfaceMethodTable( ClassClass c, String n, InterfaceVector v[] );
}
