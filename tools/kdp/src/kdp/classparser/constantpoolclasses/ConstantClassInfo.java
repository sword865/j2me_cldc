/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package kdp.classparser.constantpoolclasses;

import java.io.*;

/**
 * Encapsulates a Constant_Class item in the Java class
 * file constant pool. 
 *
 * @author             Aaron Dietrich
 * @version            $Id: ConstantClassInfo.java,v 1.1.1.1 2000/07/07 13:34:24 jrv Exp $
 *
 * Revision History
 *   $Log: ConstantClassInfo.java,v $
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
public class ConstantClassInfo extends ConstantPoolInfo
  {
   /** index into constant pool table containing a UTF8Info structure
       that specifies a class name */
   private int        nameIndex;

   /**
    * Constructor.  Reads appropriate fields from the specified data
    * input stream.
    *
    * @param             iStream        input stream to read from
    *
    * @exception         IOException    just pass exceptions up
    */
   public ConstantClassInfo (DataInputStream iStream) throws IOException
     {
      tag = ConstantPoolInfo.CONSTANT_Class;

      nameIndex = iStream.readUnsignedShort ();
     }

   /**
    * Retrieve the constant pool index of the class name this
    * ConstantClassInfo structure represents.
    *
    * @return            int            constant pool index containing
    *                                   UTF8Info structure with class's name
    */
   public int getNameIndex ()
     {
      return nameIndex;
     }
     
   /**
    * Return this ConstantClassInfo structure's fields as a string.
    */ 
   public String toString ()
     {
      return ("CONSTANT_Class" + "\t\tnameIndex=" + nameIndex);
     }
  }
