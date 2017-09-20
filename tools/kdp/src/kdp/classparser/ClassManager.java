/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package kdp.classparser;

import java.io.*;
import java.util.*;
import kdp.Log;
import kdp.VMConstants;

public class ClassManager {

    List classes;
    SearchPath path;
    public static Map classMap = new HashMap(128, 0.75f);

    public ClassManager() {
        classes = new Vector();
        path = null;
    }

    public ClassManager( SearchPath path ) {
        this();
        this.path = path;
    }

    private ClassFile loadClass(String className, 
                                FileReference file, 
                                byte typetag) {

        Log.LOGN(3, "loadclass: " + file );

        ClassFile cf = new ClassFile(file, className, typetag);

        try { 
            cf.readClassFile();
            synchronized(classes) {
                classes.add( cf );
            }
        }
        catch ( Exception e ) { 
            Log.LOGN(2,  "Error loading: " + file );
            cf = null;
        }

        return cf; 
    }

    public List getAllClasses() {
        return Collections.unmodifiableList( classes );
    }

    public ClassFile findClass( byte typetag, String className ) {
        ClassFile cf=null;

    synchronized(classes) {
            Iterator iter = classes.iterator();

            while ( iter.hasNext() ) {
                cf = ( ClassFile )iter.next();
                if ( cf.equals( className ) )
                    return cf;
            }
        }

    if (typetag == VMConstants.TYPE_TAG_ARRAY) {
            Log.LOGN(4,  "findclass: Array class " + className );
            cf = new ClassFile(null, className, typetag);
            synchronized(classes) {
                classes.add( cf );
            }
        return cf;
    }
        if ( path != null ) {
            FileReference file;

            Log.LOGN(4,  "findclass: finding " + className );
            if ( ( file = path.resolve( className ) ) != null ) {
                return loadClass(className, file, typetag);
            } 
        }
        return null;
    }
} // ClassManager
