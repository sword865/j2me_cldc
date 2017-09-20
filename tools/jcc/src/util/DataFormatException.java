/*
 *    DataFormatException.java    1.5    03/01/14 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package util;

/*
 * An exception we throw when we don't like what we find in data 
 * structures, especially those coming from .class files.
 */

public class DataFormatException extends java.io.IOException {
    public DataFormatException( ){
    super( );
    }
    public DataFormatException( String msg ){
    super( msg );
    }
}
