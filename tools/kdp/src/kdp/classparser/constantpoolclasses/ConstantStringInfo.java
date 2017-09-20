/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package kdp.classparser.constantpoolclasses;

import java.io.*;

/**
 * Encapsulates a CONSTANT_String item a Java class file 
 * constant pool. 
 *
 * @author             Aaron Dietrich
 * @version            $Id: ConstantStringInfo.java,v 1.1.1.1 2000/07/07 13:34:24 jrv Exp $
 *
 * Revision History
 *   $Log: ConstantStringInfo.java,v $
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
public class ConstantStringInfo extends ConstantPoolInfo
  {
   /** index into constant pool containing a UTF8Info structure */
   private int        stringIndex;

   /**
    * Constructor.  Creates a ConstantStringInfo object.
    *
    * @param        iStream        input stream to read from
    *
    * @exception    IOException    just pass IOExceptions up
    */
   public ConstantStringInfo (DataInputStream iStream) throws IOException
     {
      tag = ConstantPoolInfo.CONSTANT_String;
      
      stringIndex = iStream.readUnsignedShort ();
     }

   /**
    * Returns this ConstantStringInfo's information as a string.
    *
    * @return       String         info as a string
    */
   public String toString ()
     {
      return ("CONSTANT_String\t" + "stringIndex=\t" + stringIndex);
     }

   /**
    * Returns the string referenced by this ConstantStringInfo object
    * from the constant pool.
    *
    * @param        constantPool   class's constant pool
    *
    * @return       String         referenced string
    */
   public String toString (final ConstantPoolInfo[] constantPool)
     {
      ConstantUtf8Info        utf8Info = (ConstantUtf8Info) constantPool[stringIndex];

      return ("CONSTANT_String=\t" + utf8Info.toString ());
     }
  }
