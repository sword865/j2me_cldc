/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
/**
 * Represents an individual item in the LocalVariableTable of a
 * Java class file.
 *
 * @author             Aaron Dietrich
 * @version            $Id: LocalVariableTable.java,v 1.1.1.1 2000/07/07 13:34:24 jrv Exp $
 *
 * Revision History
 *   $Log: LocalVariableTable.java,v $
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
package kdp.classparser.attributes;

import kdp.classparser.attributes.*;

public class LocalVariableTable
  {
   /** index into code array that begins the range where
       a local variable has a value */
   public int        startPC;
   /** index into code array, startPC + length specifies
       the position where the local variable ceases to
       have a value */
   public int        length;
   /** index into constant pool table containing the name
       of the local variable as a simple name */
   public int        nameIndex;
   /** index into constant pool table containing the
       encoded data type of the local variable */
   public int        descriptorIndex;
   /** local variable must be at index in the local 
       variable array of the current frame */
   public int        index;
  }
