/*
 *    Set.java    1.8    03/01/14 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/*
 * Set-of-objects
 * Add, delete, query, enumerate.
 * Dead simple.
 * This set implementation is best for small, often
 * empty sets, as used by the dependenceAnalysis classes.
 * It appears that a good fraction ( 1/3 to 1/2 ) of those
 * sets are empty, and most of the rest are pretty small.
 * That is the target use of this implementation.
 * 
 */
package util;
import java.util.Enumeration;
import java.util.Vector;

public class Set {

    static private final int defaultInitial = 10;

    int      ninitial;     // initial allocation for this one.
    Vector setData;

    public Set( int nin ){
    ninitial = nin;
    }

    public Set(){
    ninitial = defaultInitial;
    }

    public boolean isIn( Object o ){
    return (setData==null)?false:setData.contains( o );
    }

    public void addElement( Object o ){
    // see if its already in. If not,
    // add at end.
    if ( isIn( o ) ) return;

    // now add, extending if necessary.
    if ( setData == null ){
        setData = new Vector( ninitial );
    }
    setData.addElement( o );
    }

    public void add( Object o ){ addElement( o ); }

    public Enumeration elements(){
    return (setData==null)?EmptyEnumeration.instance:setData.elements();
    }

    public void deleteAllElements(){
    setData = null;
    }

    public int size(){ return (setData==null)?0:setData.size(); } 

}
