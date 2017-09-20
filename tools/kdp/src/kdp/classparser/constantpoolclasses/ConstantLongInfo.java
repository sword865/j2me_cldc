/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package kdp.classparser.constantpoolclasses;

import java.io.*;

/**
 * Encapsulates a CONSTANT_Long in a Java class file
 * constant pool.
 *
 * @author             Aaron Dietrich
 * @version            $Id: ConstantLongInfo.java,v 1.1.1.1 2000/07/07 13:34:24 jrv Exp $
 *
 * Revision History
 *   $Log: ConstantLongInfo.java,v $
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
public class ConstantLongInfo extends ConstantPoolInfo
  {
   /** the high bytes of the long */
   private int            highBytes;
   /** the low bytes of the long */
   private int            lowBytes;

   /**
    * Constructor.  Creates the ConstantLongInfo object.
    *
    * @param        iStream        input stream to read from
    *
    * @return       IOException    just pass IOExceptions up
    */
   public ConstantLongInfo (DataInputStream iStream) throws IOException
     {
      tag = ConstantPoolInfo.CONSTANT_Long;

      highBytes = iStream.readInt ();
      lowBytes = iStream.readInt ();
     }

   /**
    * Returns this ConstantLongInfo object as a string for displaying.
    *
    * @return       String         string representing this ConstantLongInfo
    *                              object.
    */
   public String toString ()
     {
      long     value = ((long) highBytes << 32) + lowBytes;

      return ("CONSTANT_Long=\t" + value);
     }
  }
