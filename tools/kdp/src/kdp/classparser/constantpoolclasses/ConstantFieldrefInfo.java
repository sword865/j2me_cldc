/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package kdp.classparser.constantpoolclasses;

import java.io.*;

/**
 * Encapsulates a Constant_Fieldref item of a Java class
 * file constant pool.
 *
 * @author             Aaron Dietrich
 * @version            $Id: ConstantFieldrefInfo.java,v 1.1.1.1 2000/07/07 13:34:24 jrv Exp $
 *
 * Revision History
 *   $Log: ConstantFieldrefInfo.java,v $
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
public class ConstantFieldrefInfo extends ConstantPoolInfo
  {
   /** constant pool index containing ConstantClassInfo structure
       identifying class of this field. */
   private int        classIndex;
   /** constant pool index describing the type of this field */
   private int        nameAndTypeIndex;
   
   /**
    * Constructor.   Creates the ConstantFieldInfo object
    *
    * @param        iStream        input stream to read from
    *
    * @exception    IOException    just pass IOExceptions up
    */
   public ConstantFieldrefInfo (DataInputStream iStream) throws IOException
     {
      tag = ConstantPoolInfo.CONSTANT_Fieldref;
      
      classIndex = iStream.readUnsignedShort ();
      nameAndTypeIndex = iStream.readUnsignedShort ();
     }

   /**
    * Returns the fields of this ConstantFieldrefInfo in a string for
    * displaying.
    */
   public String toString ()
     {
      return ("CONSTANT_Fieldref" + "\t\tclassIndex=" + classIndex + 
                "\tnameAndTypeIndex=" + nameAndTypeIndex);
     }
  }
