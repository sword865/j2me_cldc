/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package kdp.classparser.constantpoolclasses;

import java.io.*;

/**
 * Encapsulates a CONSTANT_Float in a Java class file
 * constant pool. 
 *
 * @author             Aaron Dietrich
 * @version            $Id: ConstantFloatInfo.java,v 1.1.1.1 2000/07/07 13:34:24 jrv Exp $
 *
 * Revision History
 *   $Log: ConstantFloatInfo.java,v $
 *   Revision 1.1.1.1  2000/07/07 13:34:24  jrv
 *   Initial import of kdp code
 *
 *   Revision 1.1.1.1  2000/05/31 19:14:48  ritsun
 *   Initial import of kvmdt to CVS
 *
 *   Revision 1.1  2000/04/25 00:34:06  ritsun
 *   Initial revision
 *
 */
public class ConstantFloatInfo extends ConstantPoolInfo
  {
   /** the bytes that make up this field */
   private int            bytes;

   /**
    * Constructor.  Creates the constant float info object.
    *
    * @param             iStream        input stream to read from
    *
    * @exception         IOException    just pass IOExceptions up.
    */
   public ConstantFloatInfo (DataInputStream iStream) throws IOException
     {
      tag = ConstantPoolInfo.CONSTANT_Float;

      bytes = iStream.readInt ();
     }

   /**
    * Returns this ConstantFloatInfo as a string for displaying.
    * Converted to a float as specified in the JVM Specification
    *
    * @return            String         the float as a string.
    */
   public String toString ()
     {
      if (bytes == 0x7f800000)
        return ("CONSTANT_Float=\t" + "Positive Infinity");

      if (bytes == 0xff800000)
        return ("CONSTANT_Float=\t" + "Negative Infinity");

      if (((bytes >= 0x7f800001) && (bytes <= 0x7fffffff)) ||
          ((bytes >= 0xff800001) && (bytes <= 0xffffffff)))
        return ("CONSTANT_Float=\t" + "NaN");

      int s = ((bytes >> 31) == 0) ? 1 : -1;
      int e = ((bytes >> 23) & 0xff);
      int m = (e == 0) ? (bytes & 0x7fffff) << 1 :
                         (bytes & 0x7fffff) | 0x800000;

      float    value = s * m * (2^(e - 150));

      return ("CONSTANT_Float=\t" + value);
     }
  }
