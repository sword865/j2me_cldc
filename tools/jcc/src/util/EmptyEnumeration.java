/*
 *    EmptyEnumeration.java    1.2    03/01/14 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package util;

public
class EmptyEnumeration implements java.util.Enumeration {
    public boolean hasMoreElements(){ return false; }
    public Object nextElement(){ throw new java.util.NoSuchElementException(); }

    public static java.util.Enumeration instance = new EmptyEnumeration();
}
