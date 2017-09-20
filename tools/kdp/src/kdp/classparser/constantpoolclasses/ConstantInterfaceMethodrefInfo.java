/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package kdp.classparser.constantpoolclasses;

import java.io.*;

/**
 * Encapsulates a CONSTANT_InterfaceMethodref item a Java class
 * file constant pool.
 *
 * @author             Aaron Dietrich
 * @version            $Id: ConstantInterfaceMethodrefInfo.java,v 1.1.1.1 2000/07/07 13:34:24 jrv Exp $
 *
 * Revision History
 *   $Log: ConstantInterfaceMethodrefInfo.java,v $
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
public class ConstantInterfaceMethodrefInfo extends ConstantPoolInfo
  {
   /** index into constant pool table containing name of this interface */
   private int        classIndex;
   /** index into constant pool table containing type of this interface */
   private int        nameAndTypeIndex;

   /**
    * Constructor.  Creates a ConstantInterfaceMethodrefInfo object.
    *
    * @param        iStream        input stream to read from
    *
    * @exception    IOException    just pass io exceptions up
    */
   public ConstantInterfaceMethodrefInfo (DataInputStream iStream) throws IOException
     {
      tag = ConstantPoolInfo.CONSTANT_InterfaceMethodref;

      classIndex = iStream.readUnsignedShort ();
      nameAndTypeIndex = iStream.readUnsignedShort ();
     }

   /**
    * Return this ConstantInterfaceMethodrefInfo as a string
    *
    * @return       String         info as a string
    */
   public String toString ()
     {
      return ("CONSTANT_InterfaceMethodref" + "\tclassIndex=" +
              classIndex + "\tnameAndTypeIndex=" + nameAndTypeIndex);
     }
  }
