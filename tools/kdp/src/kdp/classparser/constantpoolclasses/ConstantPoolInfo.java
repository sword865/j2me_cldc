/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package kdp.classparser.constantpoolclasses;

import java.io.*;

/**
 * Base class for representing an item in the 
 * constant pool of a Java class file.
 *
 * @author             Aaron Dietrich
 * @version            $Id: ConstantPoolInfo.java,v 1.1.1.1 2000/07/07 13:34:24 jrv Exp $
 *
 * Revision History
 *   $Log: ConstantPoolInfo.java,v $
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
public abstract class ConstantPoolInfo
  {
   /** All the possible types of constant pool entries as specified by
       the JVM Specification. */
   public static final byte    CONSTANT_Class                = 7;
   public static final byte    CONSTANT_Fieldref            = 9;
   public static final byte    CONSTANT_Methodref            = 10;
   public static final byte    CONSTANT_InterfaceMethodref     = 11;
   public static final byte    CONSTANT_String            = 8;
   public static final byte    CONSTANT_Integer            = 3;
   public static final byte    CONSTANT_Float                = 4;
   public static final byte    CONSTANT_Long                = 5;
   public static final byte    CONSTANT_Double            = 6;
   public static final byte     CONSTANT_NameAndType        = 12;
   public static final byte    CONSTANT_Utf8                = 1;

   /** Stores the type of constant pool entry this is */
   protected byte            tag;

   public byte getTag() {
       return tag;
   }

   /** All subclasses must implement the toString class to display their
       individual attributes */
   public abstract String toString ();
  }
  
