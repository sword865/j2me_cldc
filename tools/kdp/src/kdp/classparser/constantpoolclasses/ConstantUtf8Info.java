/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package kdp.classparser.constantpoolclasses;

import java.io.*;

/**
 * Encapsulates a CONSTANT_Utf8 member of a Java class file
 * constant pool.
 *
 * @author             Aaron Dietrich
 * @version            $Id: ConstantUtf8Info.java,v 1.1.1.1 2000/07/07 13:34:24 jrv Exp $
 *
 * Revision History
 *   $Log: ConstantUtf8Info.java,v $
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
public class ConstantUtf8Info extends ConstantPoolInfo
  {
   /** length of this string */
   private int            length;
   /** value of the string */
   private String        bytes;

   /**
    * Constructor.  Creates a ConstantUtf8Info object.
    *
    * @param        iStream        input stream to read from
    *
    * @exception    IOException    just pass IOExceptions up
    */
   public ConstantUtf8Info (DataInputStream iStream) throws IOException
     {
      tag = ConstantPoolInfo.CONSTANT_Utf8;
      
      bytes = new String (iStream.readUTF ());
      
      length = bytes.length ();
     }

   /**
    * Returns the string represented by this ConstantUtf8Info object.
    *
    * @return       String         the string this encapsulates
    */
   public String toString ()
     {
      return bytes;
     }
  }
