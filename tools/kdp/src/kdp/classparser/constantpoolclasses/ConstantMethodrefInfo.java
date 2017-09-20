/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package kdp.classparser.constantpoolclasses;

import java.io.*;

/**
 * Encapsulates a CONSTANT_Methodref item in a Java class
 * file constant pool.
 *
 * @author             Aaron Dietrich
 * @version            $Id: ConstantMethodrefInfo.java,v 1.1.1.1 2000/07/07 13:34:24 jrv Exp $
 *
 * Revision History
 *   $Log: ConstantMethodrefInfo.java,v $
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
public class ConstantMethodrefInfo extends ConstantPoolInfo
  {
   /** constant pool index specifying this method's name */
   private int        classIndex;
   /** constant pool index specifying this method's signature */
   private int        nameAndTypeIndex;

   /**
    * Constructor.  Creates a ConstantMethodrefInfo structure.
    *
    * @param        iStream        input stream to read from
    *
    * @exception   IOException    just pass IOExceptions up
    */
   public ConstantMethodrefInfo (DataInputStream iStream) throws IOException
     {
      tag = ConstantPoolInfo.CONSTANT_Methodref;

      classIndex = iStream.readUnsignedShort ();
      nameAndTypeIndex = iStream.readUnsignedShort ();
     }

   /**
    * Return this ConstantMethodrefInfo object as a string for displaying.
    *
    * @return       String         info as a string
    */  
   public String toString ()
     {
      return ("CONSTANT_Methodref" + "\tclassIndex=" + 
              classIndex + "\tnameAndTypeIndex=" + nameAndTypeIndex);
     }
  }
