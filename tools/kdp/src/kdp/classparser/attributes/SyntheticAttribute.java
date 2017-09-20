/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package kdp.classparser.attributes;

import kdp.classparser.attributes.*;
import kdp.classparser.constantpoolclasses.*;
import java.io.*;

/**
 * Encapsulates the Synthetic attribute of a Java class file.
 *
 * This class has never been thoroughly tested.
 *
 * @author             Aaron Dietrich
 * @version            $Id: SyntheticAttribute.java,v 1.1.1.1 2000/07/07 13:34:24 jrv Exp $
 *
 * Revision History
 *   $Log: SyntheticAttribute.java,v $
 *   Revision 1.1.1.1  2000/07/07 13:34:24  jrv
 *   Initial import of kdp code
 *
 *   Revision 1.1.1.1  2000/05/31 19:14:48  ritsun
 *   Initial import of kvmdt to CVS
 *
 *   Revision 1.1  2000/04/25 00:30:39  ritsun
 *   Initial revision
 *
 */
public class SyntheticAttribute extends Attribute
  {
   /** index into the constant pool table containing the name 
       of this class */
   private int                attributeNameIndex;
   /** length of this attribute in bytes */
   private int                attributeLength;
   
   /**
    * Constructor.  Reads the SyntheticAttribute attribute from
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
   public SyntheticAttribute (DataInputStream iStream,
                              int attributeNameIndex, int attributeLength)
                                                              throws IOException
     {
      this.attributeNameIndex = attributeNameIndex;
      this.attributeLength = attributeLength;
     }

   /**
    * Returns the SyntheticAttribute attribute in a nice easy to
    * read format as a string.
    *
    * @param        constantPool        constant pool of the class file
    *
    * @return         String            the attribute as a nice easy to
    *                            read String
    */
   public String toString (final ConstantPoolInfo[] constantPool)
     {
      ConstantUtf8Info            utf8Info;
      String                        s = new String ("");

      utf8Info = (ConstantUtf8Info) constantPool[attributeNameIndex];
      s = s + utf8Info.toString () + "\n";

      return s;
     }
  }
