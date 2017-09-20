/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package kdp.classparser.attributes;

import kdp.classparser.attributes.*;
import kdp.classparser.constantpoolclasses.*;

import java.io.*;

/**
 * Encapsulates the ConstantValue attribute of a Java class file.
 *
 * @author             Aaron Dietrich
 * @version            $Id: ConstantValueAttribute.java,v 1.1.1.1 2000/07/07 13:34:23 jrv Exp $
 *
 * Revision History
 *   $Log: ConstantValueAttribute.java,v $
 *   Revision 1.1.1.1  2000/07/07 13:34:23  jrv
 *   Initial import of kdp code
 *
 *   Revision 1.1.1.1  2000/05/31 19:14:48  ritsun
 *   Initial import of kvmdt to CVS
 *
 *   Revision 1.1  2000/04/25 00:30:39  ritsun
 *   Initial revision
 *
 */
public class ConstantValueAttribute extends Attribute
  {
   /** index into the constant pool table containing the name 
       of this class */
   private int                attributeNameIndex;
   /** length of this attribute in bytes */
   private int                attributeLength;
   /** the constant pool entry at that index gives the constant 
       value represented by this attribute */
   private int                constantValueIndex;
   
   /**
    * Constructor.  Reads the ConstantValue attribute from
    * the class file.
    *
    * @param        iStream            the input stream on which to
    *                            read the class file
    * @param        attributeNameIndex    attributeNameIndex member of
    *                            attribute structure.
    * @param        attributeLength    attributeLength member of
    *                            attribute structure.
    *
    * @exception    IOException        pass IOExceptions up
    */
   public ConstantValueAttribute (DataInputStream iStream,
                                  int attributeNameIndex, int attributeLength)
                                                              throws IOException
     {
      this.attributeNameIndex = attributeNameIndex;
      this.attributeLength = attributeLength;

      //read the constantValueIndex from the class file
      constantValueIndex = iStream.readUnsignedShort ();
     }

   /**
    * Returns the ConstantValue attribute in a nice easy to
    * read format as a string.
    *
    * @param        constantPool        constant pool of the class file
    *
    * @return         String                the attribute as a nice easy to
    *                                read String
    */
   public String toString (final ConstantPoolInfo[] constantPool)
     {
      String                s = new String ("");

      ConstantUtf8Info    utf8Info = (ConstantUtf8Info)
                                             constantPool[attributeNameIndex];
      s = s + utf8Info.toString () + "\n\t\t\t";

      ConstantPoolInfo   cpInfo = (ConstantPoolInfo)
                                             constantPool[constantValueIndex];

      switch (cpInfo.getTag())
        {
         case ConstantPoolInfo.CONSTANT_Long :
           {
            ConstantLongInfo cpInfo2 = (ConstantLongInfo) cpInfo;
            s = s + cpInfo2.toString ();
            break;
           }

         case ConstantPoolInfo.CONSTANT_Float :
           {
            ConstantFloatInfo cpInfo2 = (ConstantFloatInfo) cpInfo;
            s = s + cpInfo2.toString ();
            break;
           }

         case ConstantPoolInfo.CONSTANT_Double :
           {
            ConstantDoubleInfo cpInfo2 = (ConstantDoubleInfo) cpInfo;
            s = s + cpInfo2.toString ();
            break;
           }

         case ConstantPoolInfo.CONSTANT_Integer :
           {
            ConstantIntegerInfo cpInfo2 = (ConstantIntegerInfo) cpInfo;
            s = s + cpInfo2.toString ();
            break;
           }

         case ConstantPoolInfo.CONSTANT_String :
           {
            ConstantStringInfo      cpInfo2 = (ConstantStringInfo) cpInfo;
            s = s + cpInfo2.toString (constantPool);
            break;
           }
        }
      return s;
     }
  }
