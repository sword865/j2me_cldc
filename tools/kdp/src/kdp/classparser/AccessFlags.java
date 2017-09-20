/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package kdp.classparser;

import java.io.*;

/**
 * Encapsulates the access flags for a class file.
 *
 * @author             Aaron Dietrich
 * @version            $Id: AccessFlags.java,v 1.1.1.1 2000/07/07 13:34:23 jrv Exp $
 *
 * Revision History
 *   $Log: AccessFlags.java,v $
 *   Revision 1.1.1.1  2000/07/07 13:34:23  jrv
 *   Initial import of kdp code
 *
 *   Revision 1.1.1.1  2000/05/31 19:14:47  ritsun
 *   Initial import of kvmdt to CVS
 *
 *   Revision 1.1  2000/04/25 00:36:43  ritsun
 *   Initial revision
 *
 */
public class AccessFlags
  {
   /** access flag if this class is public */
   private static final int        ACC_PUBLIC        = 0x0001;
   /** access flag if this class is final */
   private static final int        ACC_FINAL         = 0x0010;
   /** access flag if this class is "super" */
   private static final int        ACC_SUPER        = 0x0020;
   /** access flag if this class is an interface */
   private static final int        ACC_INTERFACE    = 0x0200;
   /** access flag if this class is abstract */
   private static final int        ACC_ABSTRACT    = 0x0400;
   /** access flag if this class is an Array class  */
   private static final int        ACC_ARRAY    = 0x1000;

   /** access permissions to and properties of this class */
   private int                 flags;

   /**
    * Constructor.  Reads the access flags from the
    * specified DataInputStream.
    *
    * @param        iStream            input stream attached
    *                                to the class file
    */
   public AccessFlags (DataInputStream iStream) throws IOException
     {
      flags = iStream.readUnsignedShort ();
     }

   /**
    * Converts the access flags to an easy to read format
    *
    * @return        String            string representation
    *                            of the access flags
    */
   public String toString ()
     {
      String            s = new String ("Class Access Flags:\t");

      //just go through all the possibilities and build a string
      //of the access flags
      if ((flags & ACC_PUBLIC) > 0)
        s = s + "public ";

      if ((flags & ACC_FINAL) > 0)
        s = s + "final ";

      if ((flags & ACC_SUPER) > 0)
        s = s + "super ";

      if ((flags & ACC_INTERFACE) > 0)
        s = s + "interface ";

      if ((flags & ACC_ABSTRACT) > 0)
        s = s + "abstract";

      return s;
     }

    public byte getJDWPTypeTag() {
    if ((flags & ACC_ARRAY) > 0)
        return 3;
    else if ((flags & ACC_INTERFACE) > 0)
        return 2;
    else
        return 1;
    }

    public int getRawAccessFlags() {
    return (flags);
    }
  }
