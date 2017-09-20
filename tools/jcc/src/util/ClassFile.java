/*
 *    ClassFile.java    1.9    99/04/06 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package util;

import components.*;
import vm.Const;

import java.io.DataInput;
import java.io.DataInputStream;
import java.io.DataOutput;
import java.io.IOException;
import java.io.PrintStream;
import java.io.InputStream;

import java.util.Enumeration;
import java.util.Hashtable;

/*
 * Reading of class files.
 * Which is different from reading a class from a multi-class file.
 * But not much.
 * Much of the real work is done in components.ClassInfo.
 *
 * There would be stuff for writing class files here, too, but we've
 * never been called upon to do that operation.
 *
 * Also includes a small test program for reading and dumping
 * a class file.
 */

public
class ClassFile
{
    // Input file name
    public String     fileName;
    public boolean     didRead = false;
    public Exception     failureMode; // only for didRead == false.
    public boolean    verbose = false;
    public ClassInfo    clas;
    PrintStream       log = System.out;
    InputStream        istream;

    // Identifies the classfile format
    int            minorID,
            majorID;

    public ClassFile(String name, boolean v ) {
    fileName = name;
    verbose     = v;
    clas     = new ClassInfo( v );
    istream  = null;
    }

    public ClassFile(String name, InputStream i, boolean v ) {
    fileName = name;
    verbose     = v;
    clas     = new ClassInfo( v );
    istream  = i;
    }

    // Read in the entire class file
    public boolean
    readClassFile( ConstantPool t ) {
    DataInputStream in = null;
    failureMode = null;
    done:
    try {
        int magic;
        if ( istream == null ){
        istream = new java.io.BufferedInputStream( new java.io.FileInputStream( fileName ) );
        }
        in = new DataInputStream( istream );
        if ((magic = in.readInt()) != Const.JAVA_MAGIC) {
        failureMode = new IOException(Localizer.getString("classfile.bad_magic.ioexception"));
        break done; // bad magic -- bail immediately.
        }

        minorID = in.readShort();
        majorID = in.readShort();

            /* Ignore the major and minor versions for JDK 1.4 compatibility */

            /*
        if (minorID != Const.JAVA_MINOR_VERSION) {
        failureMode = new IOException(Localizer.getString("classfile.bad_minor_version_number.ioexception"));
        break done; // bad magic -- bail immediately.
        }

        if (majorID != Const.JAVA_VERSION) {
        failureMode = new IOException(Localizer.getString("classfile.bad_major_version_number.ioexception"));
        break done; // bad magic -- bail immediately.
        }
            */

        if(verbose){
        log.println(Localizer.getString("classfile._file", fileName)+
                Localizer.getString("classfile._magic/major/minor",
                                        Integer.toHexString( magic ),
                                        Integer.toHexString( majorID ),
                                        Integer.toHexString( minorID )));
        }
        clas.read( in, true, t );
        didRead = true;
    } catch ( Exception e ){
        failureMode = e;
    }
    if ( in != null ){
        try{
        in.close();
        } catch ( IOException e ){
        if (didRead==true){
            didRead=false;
            failureMode=e;
        }
        }
    }
    return didRead;
    }

    public void
    dump( PrintStream o ){
    o.println(Localizer.getString("classfile.file", fileName));
    if ( ! didRead ){
        o.println(Localizer.getString(
              "classfile.could_not_be_read_because_of", 
              failureMode));
        if ( failureMode != null ) // stupid.
        failureMode.printStackTrace( o );
        return;
    }
    clas.dump( o );
    }

    /*
     * Simple test program.
     * Parameters: list of class files to read and process.
     */
    public static void main( String clist[] ){
    for( int i = 0; i < clist.length; i++ ){
        ClassFile f = new ClassFile( clist[i], true );
        if (f.readClassFile( null ) != true ){
        System.out.println(Localizer.getString(
                        "classfile.read_of",
                        clist[i]));
        f.dump( System.out );
        continue;
        }
        f.clas.countReferences( true );
        f.dump( System.out );
    }
    }

}
