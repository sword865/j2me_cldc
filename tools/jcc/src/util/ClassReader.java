/*
 *    ClassReader.java    1.7    01/09/21 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package util;

import components.*;

import java.io.DataInputStream;
import java.io.IOException;
import java.io.File;
import java.io.InputStream;
import java.io.FileInputStream;
import java.io.BufferedInputStream;
import java.io.ByteArrayInputStream;
import java.util.Enumeration;
import java.util.Vector;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import vm.Const;
import jcc.MultiClass;

/*
 * ClassReader reads classes from a variety of sources
 * and, in all cases, creates ClassInfo structures for them.
 * It can read single class files, mclass files, entire zip
 * files or a member of a zip file.
 */
public class ClassReader {

    int verbosity;
    ConstantPool t;

    public ClassReader( ConstantPool cp, int verb ){
    t = cp;
    verbosity = verb;
    }

    private int
    getMagic( InputStream file ) throws IOException {
    DataInputStream data = new DataInputStream( file );
    int n = 0;
    file.mark( 4 );
    try {
        n = data.readInt();
    } finally {
        file.reset();
    }
    return n;
    }

    /*
     * To read the contents of a single, named class or mclass file.
     * The two cases are distinguished by magic number.
     * The return value is the number of classes read.
     * ClassInfo classes for the newly read classes are added
     * to the argument Vector.
     */
    public int
    readFile (String fileName, Vector done) throws IOException
    {
    InputStream infile;
    infile = new BufferedInputStream(new FileInputStream( fileName ) );
    return readStream(fileName, infile, done);

    }

    /*
     * To read the contents of an entire named zip (or Jar) file.
     * Each element that is a class or mclass file is read.
     * Others are silently ignored.
     * The return value is the number of classes read.
     * ClassInfo classes for the newly read classes are added
     * to the argument Vector.
     */
    public int
    readZip (String fileName, Vector done) throws IOException
    { 
    int i = 0;

        ZipFile zf = new ZipFile(fileName);
        Enumeration enum = zf.entries();
        while (enum.hasMoreElements()) {
            ZipEntry ent = (ZipEntry)enum.nextElement();
        String name = ent.getName();
        if (!ent.isDirectory() && 
        (name.endsWith(".class") || name.endsWith(".mclass"))) {
        int length = (int)ent.getSize();
        byte buffer[] = new byte[length];
        try { 
                    new DataInputStream(zf.getInputStream(ent)).readFully(buffer);
            i += readStream(name, new ByteArrayInputStream(buffer), 
                    done);
        } catch (IOException e) { 
            System.out.println(Localizer.getString("classreader.failed_on", name));
        }
        }
    }
    return i;
    }

    /*
     * To read the contents of a class or mclass file from
     * a given input stream.
     * The two cases are distinguished by magic number.
     * The return value is the number of classes read.
     * ClassInfo classes for the newly read classes are added
     * to the argument Vector.
     * The inputName argument is used only for error messages.
     * This can be used for reading a zip file element or
     * from single opened file.
     */
    public int
    readStream( String inputName, InputStream infile, Vector done ) throws IOException {
    int magicNumber = getMagic( infile );

    int ndone = 0;
    if ( magicNumber == Const.JAVA_MAGIC ){
        /*
         * We have a solo class.
         */
        ClassFile f = new ClassFile( inputName, infile, verbosity>=2 );
        if ( verbosity != 0 )
        System.out.println(
            Localizer.getString("classreader.reading_classfile", 
                                inputName));
        if (f.readClassFile( t ) ){
        f.clas.externalize( t );
        done.addElement( f.clas );
        ndone+=1;
        } else {
        throw new DataFormatException(
            Localizer.getString("classreader.read_of_class_file", 
                                 inputName));
        }
    } else if ( magicNumber == MultiClass.MULTICLASS_MAGIC ){
        /*
         * We have an mclass file.
         */
        MultiClass m = new MultiClass( inputName, infile, null, verbosity>=2 );
        if (m.readMultiClassFile() ){
        ClassInfo c[] = m.classes;
        int n = c.length;
        for ( int i = 0 ; i < n; i++ ){
            c[i].externalize( t );
            done.addElement( c[i] );
        }
        ndone += n;
        } else {
        throw new DataFormatException(
            Localizer.getString("classreader.read_of_multi-class_file", inputName));
        }
    } else {
        throw new DataFormatException(
        Localizer.getString("classreader.file_has_bad_magic_number", 
                     inputName, Integer.toString(magicNumber)));
    }
    try {
        infile.close();
    } catch ( Throwable x ){ }
    return ndone;
    }

    /*
     * To read a single, named class.
     * The return value is the number of classes read.
     * ClassInfo classes for the newly read class is added
     * to the argument Vector.
     * The class to be read is found using the ClassFileFinder,
     * which, if successful, will give us an InputStream, either
     * of a file from its search path, or of a zip file element
     * from its search path.
     */
    public int readClass( String classname, ClassFileFinder finder, Vector done ) 
    {
    InputStream f = finder.findClassFile( classname );
    if ( f == null ) return 0;
    try {
        return readStream( classname, f, done );
    } catch ( IOException e ){
        e.printStackTrace();
        return 0;
    }
    }

}
