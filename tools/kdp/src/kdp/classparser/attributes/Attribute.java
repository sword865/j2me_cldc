/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package kdp.classparser.attributes;
import kdp.classparser.constantpoolclasses.*;

import java.io.*;

/**
 * Base class for representing attributes of fields and methods
 * in a Java class file.
 *
 * @author             Aaron Dietrich
 * @version            $Id: Attribute.java,v 1.1.1.1 2000/07/07 13:34:23 jrv Exp $
 *
 * Revision History
 *   $Log: Attribute.java,v $
 *   Revision 1.1.1.1  2000/07/07 13:34:23  jrv
 *   Initial import of kdp code
 *
 *   Revision 1.1.1.1  2000/05/31 19:14:47  ritsun
 *   Initial import of kvmdt to CVS
 *
 *   Revision 1.1  2000/04/25 00:30:39  ritsun
 *   Initial revision
 *
 */
public abstract class Attribute
  {
   /** 
    * Returns the attribute in a nice easy to read format as a String.
    *
    * @param        constantPool        the constant pool of the class file
    *
    * @return        String            the attribute as a nice easy to read
    *                            String.
    */
   public abstract String toString (final ConstantPoolInfo[] constantPool);         
  }
  
  
