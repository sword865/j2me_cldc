/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package kdp.classparser.constantpoolclasses;

import java.io.*;

/**
 * Encapsulates a CONSTANT_NameAndType member of a Java
 * class file constant pool.
 *
 * @author             Aaron Dietrich
 * @version            $Id: ConstantNameAndTypeInfo.java,v 1.1.1.1 2000/07/07 13:34:24 jrv Exp $
 *
 * Revision History
 *   $Log: ConstantNameAndTypeInfo.java,v $
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
public class ConstantNameAndTypeInfo extends ConstantPoolInfo
  {
   /** constant pool index of a name, varies by use of this
       ConstantNameAndTypeInfo object */
   private int        nameIndex;
   /** constant pool index of a descriptor, varies by use of this
       ConstantNameAndTypeInfo object. */
   private int        descriptorIndex;
   
   /**
    * Constructor.  Creates a ConstantNameAndTypeInfo object.
    *
    * @param        iStream        input stream to read from
    *
    * @exception    IOException    just pass IOExceptions up
    */
   public ConstantNameAndTypeInfo (DataInputStream iStream) throws IOException
     {
      tag = ConstantPoolInfo.CONSTANT_NameAndType;

      nameIndex = iStream.readUnsignedShort ();
      descriptorIndex = iStream.readUnsignedShort ();
     }

   /**
    * Returns this ConstantNameAndTypeInfo object's info as a string for
    * displaying.
    *
    * @return       String         info as a string
    */     
   public String toString ()
     {
      return ("CONSTANT_NameAndType" + "\tnameIndex=" + 
              nameIndex + "\tdescriptorIndex=" + descriptorIndex);
     }
  }
