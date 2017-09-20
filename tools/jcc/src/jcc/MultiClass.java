/*
 *    MultiClass.java    1.12    03/01/14 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package jcc;

import java.io.DataOutput;
import java.io.DataInput;
import java.io.DataInputStream;
import java.io.IOException;
import java.io.PrintStream;
import java.io.InputStream;
import java.util.Vector;
import components.*;
import util.*;

/*
 * Reading and writing multi-class files.
 * Which is a lot like reading class files, but with some
 * shared symbol-table stuff at the front.
 *
 * Since this looks so much like a class file, most of the real
 * work is done in components.ClassInfo
 */

public
class MultiClass{

    public String     fileName;
    public boolean     didRead = false;
    public Exception     failureMode; // only for didRead == false.
    public boolean    verbose = false;
    PrintStream       log = System.out;
    InputStream        istream;

    public ConstantPool    consts;
    public ClassInfo    classes[];

    public final static int   MULTICLASS_MAGIC = 0xCafeBeef;
    final static short MAJOR = 0;
    final static short MINOR = 1;

    // create from a collection of classes we read elsewhere
    public MultiClass( ConstantPool p, ClassInfo c[], boolean v ){
    consts  = p;
    classes = c;
    verbose = v;
    }

    // create in anticipation of reading classes from file.
    public MultiClass( String name, ConstantPool p, boolean v ){
    fileName = name;
    consts   = p;
    verbose  = v;
    istream  = null;
    }

    // create in anticipation of reading classes from file.
    public MultiClass( String name, InputStream i, ConstantPool p, boolean v ){
    fileName = name;
    consts   = p;
    verbose  = v;
    istream  = i;
    }

    void readHeader( DataInput in ) throws IOException{
    int    magic;
    int    minor;
    int    major;
    if ((magic = in.readInt()) != MULTICLASS_MAGIC) {
        throw new IOException(Localizer.getString("multiclass.bad_magic.ioexception"));
    }
    minor = in.readShort();
    major = in.readShort();
    if (minor != MINOR) {
        throw new IOException(Localizer.getString("multiclass.bad_minor_version_number.ioexception"));
    }
    if (major != MAJOR) {
        throw new IOException(Localizer.getString("multiclass.bad_major_version_number.ioexception"));
    }
    if(verbose){
        log.println(Localizer.getString(
               "multiclass.multiclass", fileName)+
            Localizer.getString("multiclass._magic/major/minor",
                        Integer.toHexString( magic ),
                        Integer.toHexString( major ),
                        Integer.toHexString( minor )));
    }
    }

    static void writeHeader( DataOutput o ) throws IOException{
    o.writeInt( MULTICLASS_MAGIC );
    o.writeShort( MINOR );
    o.writeShort( MAJOR );
    }

    public boolean
    readMultiClassFile( ) {
    boolean myConstPool = false;
    DataInputStream in = null;
    failureMode = null;
    didRead = false;
    if ( consts == null ){
        consts = new ConstantPool();
        myConstPool = true;
    }
    try {
        if ( istream == null ){
        istream = new java.io.BufferedInputStream( new java.io.FileInputStream( fileName ) );
        }
        in = new DataInputStream( istream );
        readHeader( in );
        int n = consts.read( in );
        if ( verbose ){
        System.out.println(Localizer.getString(
                   "multiclass.symbols_into_constant_pool", 
                   Integer.toString(n)));
        }
        //consts.dump( System.out );//DEBUG
        if ( myConstPool )
        consts.lock();
        n = in.readUnsignedShort( );
        if (verbose){
        System.out.println(Localizer.getString(
                   "multiclass.reading_classes",
                    Integer.toString(n)));
        }
        classes = new ClassInfo[n];
        for( int i = 0; i < n; i++ ){
        classes[i] = new ClassInfo( verbose );
            classes[i].readMultiClass( in, true, consts );
        }
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
    o.println(Localizer.getString("multiclass.file", fileName));
    if ( failureMode != null ){
        o.println(Localizer.getString("multiclass.could_not_be_read_because_of"));
        failureMode.printStackTrace( o );
        return;
    }
    o.println(Localizer.getString("multiclass.shared_constant_table"));
    consts.dump(o);
    o.println(Localizer.getString("multiclass.classes"));
    for( int i = 0; i < classes.length; i++ ){
        classes[i].dump(o);
    }
    }

    public void outputMultiClass(java.io.DataOutputStream out)
     throws IOException {
    // write header info
    writeHeader( out );
    // write constant pool
    consts.write( out );
    int  nclasses = classes.length;
    // write classes
    out.writeShort(nclasses);
    for( int i = 0; i < nclasses; i++ ){
        classes[i].write(out);
    }
    out.close();
    }

    public boolean writeMultiClass( String fileName ){
    java.io.DataOutputStream out;
    try {
        out = new java.io.DataOutputStream( new java.io.BufferedOutputStream( new java.io.FileOutputStream( fileName ) ) );
        outputMultiClass( out );
        return true;
    } catch ( IOException e ){
        System.err.println(Localizer.getString("multiclass.writing",
                fileName));
        e.printStackTrace();
        return false;
    }
    }
}
