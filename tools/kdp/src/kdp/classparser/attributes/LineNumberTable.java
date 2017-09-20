/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
/**
 * Stores the items of an individual line number table
 * in a Java class file.
 *
 * @author             Aaron Dietrich
 * @version            $Id: LineNumberTable.java,v 1.1.1.1 2000/07/07 13:34:24 jrv Exp $
 *
 * Revision History
 *   $Log: LineNumberTable.java,v $
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

public class LineNumberTable
  {
   /** indicate the index into the code array at which the code 
       for a new line in the original source file begins */
   public int            startPC;
   /** the corresponding line number in the original source file */
   public int            lineNumber;
  }
