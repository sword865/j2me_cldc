/*
 *    FileCompare.java    1.3    03/01/14 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package util;
import java.io.*;

public class FileCompare{

public static boolean debug = false;

private static void debugPrint( File a, File b, String msg ){
    System.err.print(a.getPath());
    System.err.print(", ");
    System.err.print(b.getPath());
    System.err.print(" ");
    System.err.println( msg );
}

public static boolean cmp( File a, File b ) {
    if ( !a.isFile() || !b.isFile()){
    if ( debug ){
        debugPrint(a, b, "not both plain files");
    }
    return false; // NotAFile compares unequal.
    }
    if ( a.length() != b.length() ){
    if ( debug ){
        debugPrint(a, b, "different lengths");
    }
    return false; // different lengthes cannot be the same.
    }
    try {
    FileInputStream astream = null;
    FileInputStream bstream = null;
    try {
        astream = new FileInputStream( a );
        bstream = new FileInputStream( b );
        long flength = a.length(); // == b.length(), remember?
        int bufsize = (int)Math.min( flength, 1024 );
        byte abuf[] = new byte[ bufsize ];
        byte bbuf[] = new byte[ bufsize ];
        long n = 0;
        while ( n < flength ){
        int naread = astream.read( abuf );
        int nbread = bstream.read( bbuf );
        if ( naread != nbread ) return false; // oops.
        for ( int i = 0; i < naread; i++ ){
            if ( abuf[i] != bbuf[i] ){
            if ( debug ){
                debugPrint(a, b, "differ at byte "+(n+i) );
            }
            return false;
            }
        }
        n += naread;
        }
    } finally {
        if ( astream != null ) astream.close();
        if ( bstream != null ) bstream.close();
    }
    } catch ( IOException e ){
    e.printStackTrace();
    return false;
    }
    if ( debug ){
    debugPrint(a, b, "are the same");
    }
    return true;
}

public static boolean cpy( File a, File b ) {
    try {
    FileInputStream astream  = null;
    FileOutputStream bstream = null;
    try {
        astream = new FileInputStream( a );
        bstream = new FileOutputStream( b );
        long flength = a.length();
        int bufsize = (int)Math.min( flength, 1024 );
        byte buf[] = new byte[ bufsize ];
        long n = 0;
        while ( n < flength ){
        int naread = astream.read( buf );
        bstream.write( buf, 0, naread );
        n += naread;
        }
    } finally {
        if ( astream != null ) astream.close();
        if ( bstream != null ) bstream.close();
    }
    } catch ( IOException e ){
    e.printStackTrace();
    return false;
    }
    return true;
}

public static void conditionalCopy( File fromFile, File toFile ){
    if ( ! cmp( fromFile, toFile ) )
    cpy( fromFile, toFile );
}

public static void main( String args[] ){
    debug = true;
    conditionalCopy( new File( args[0] ), new File( args[1] ) );
}

}
