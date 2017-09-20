/*
 *        KVMNativeWriter.java
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package runtime;
import vm.Const;
import vm.EVMConst;
import components.*;
import vm.*;
import jcc.Util;
import util.*;
import java.util.Vector;
import java.util.Enumeration;
import java.io.OutputStream;

/*
 * The CoreImageWriter for the Embedded VM
 */

public class KVM_NATIVEWriter implements CoreImageWriter, Const, EVMConst {
    KVMStringTable stringTable = new KVMStringTable();
    KVMNameTable nameTable;
    KVMClassTable classTable;

    /* INSTANCE DATA */
    protected  String     outputFileName;
    protected  Exception failureMode = null; // only interesting on failure

    protected OutputStream          xxx;
    CCodeWriter out;

    protected     vm.VMClassFactory     classMaker = new vm.EVMClassFactory();

    boolean formatError;

    public KVM_NATIVEWriter( ){ 
    }

    public void init( boolean classDebug, ClassnameFilterList nativeTypes, 
                      boolean verbose, int maxSegmentSize ){
    }

    public boolean setAttribute( String attribute ){
        return false; 
    }

    public boolean open( String filename ){
        if ( out != null ) { 
            close();
        }
        outputFileName = filename;
        if ( filename == null){
            xxx = System.out;
            out = new CCodeWriter( xxx );
        } else {
            try {
                xxx = new java.io.FileOutputStream( filename );
                out = new CCodeWriter( xxx );
            } catch ( java.io.IOException e ){
                failureMode = e;
                return false;
            }
        }
        return true;
    }

    public void close(){
        if (out != null) { 
            out.close();
            outputFileName = null;
            out = null;
        }
    }

    public boolean writeClasses( ConstantPool consts ) {
        return writeClasses(consts, null);
    }

    public void printError( java.io.PrintStream o ){
        if ( failureMode != null ){
            failureMode.printStackTrace( o );
        } else {
            if ( out != null && out.checkError() )
                o.println(outputFileName+": Output write error");
        }
    }

    public boolean writeClasses( ConstantPool consts, 
                                 ConstantPool sharedconsts ){
        ClassClass classes[] = ClassClass.getClassVector( classMaker );
        ClassClass.setTypes();
        
        // write out some constant pool stuff here,
        writeProlog();
        
        try {
        writeAllNativeTables(classes);
        } catch (RuntimeException e) { 
            out.flush();
            System.out.println(e);
            e.printStackTrace(System.out);
            formatError = true;
        }
    writeEpilog();
        return (!formatError) && (! out.checkError());
    }
    

    protected void writeAllNativeTables(ClassClass classes[]) { 
    Vector nativeClasses = new Vector();
        for (int i = 0; i < classes.length; i++){
        boolean classHasNatives = false;
            EVMClass cc = (EVMClass)classes[i];
            if (cc.isPrimitiveClass() || cc.isArrayClass()) { 
                continue;
            }
            EVMMethodInfo  m[] = cc.methods;
            int nmethod = (m == null) ? 0 : m.length;
            for (int j = 0; j < nmethod; j++){
                EVMMethodInfo meth = m[j];
                MethodInfo mi = meth.method;
        if ((mi.access & Const.ACC_NATIVE) == 0) { 
            continue;
        }
        if (!classHasNatives) { 
            classHasNatives = true;
            nativeClasses.addElement(cc);
        }
        out.println("extern void " + 
                mi.getNativeName(true) + "(void);");
        }
    }
    out.println();
    out.println();

    for (Enumeration e = nativeClasses.elements(); e.hasMoreElements();){
            EVMClass cc = (EVMClass)e.nextElement();
            EVMMethodInfo  m[] = cc.methods;
            int nmethod = (m == null) ? 0 : m.length;

        out.println("const NativeImplementationType " + 
                       cc.getNativeName() + "_natives[] = " + "{");
            for (int j = 0; j < nmethod; j++){
                EVMMethodInfo meth = m[j];
                MethodInfo mi = meth.method;
        if ((mi.access & Const.ACC_NATIVE) == 0) { 
            continue;
        }
        out.print("\t{ \"" + mi.name.string + "\"," 
                               + spaces(20 - mi.name.string.length()));                            
                if (isOverloadedNative(mi)) { 
                    out.print("\"" + mi.type.string + "\", ");
                } else { 
                    out.print("NULL, ");
                }
                out.println(mi.getNativeName(true) + "},");
        }
        out.println("\tNATIVE_END_OF_LIST");
        out.println("};");
        out.println();
    }

    out.println(
       "const ClassNativeImplementationType nativeImplementations[] = {");

        for (Enumeration e = nativeClasses.elements(); e.hasMoreElements();){
        EVMClass cc = (EVMClass)e.nextElement();
        String className = cc.ci.className;
            String packageName, baseName;
            int lastSlash = className.lastIndexOf('/');
            if (lastSlash == -1) { 
                baseName = className;
        packageName = null;
        } else { 
                baseName = className.substring(lastSlash + 1);
                packageName = className.substring(0, lastSlash);
        }
        out.print("\t{ ");
        if (packageName == null) { 
        out.print("0" + spaces(26));
        } else {
        out.print("\"" + packageName + "\"," 
                   + spaces(25 - packageName.length()));
        }
        out.print("\"" + baseName + "\"," + spaces(25 - baseName.length()));
        out.println(cc.getNativeName() + "_natives },");
    }
    out.println("NATIVE_END_OF_LIST");
    out.println("};");
    }

    protected void writeProlog(){
        out.println("/* This is a generated file.  Do not modify.");
        out.println(" * Generated on " + new java.util.Date());
        out.println(" */\n");
    out.println();
    out.println("#include <global.h>");
    out.println();
    out.println("#if !ROMIZING");
    }
    
    protected void writeEpilog() {
    out.println("#endif");
    }

    public void printSpaceStats(java.io.PrintStream stream) {}

    private String spaces(int length) { 
    if (length <= 1) { 
        return " ";
    } if (length <= 10) { 
        return "          ".substring(0, length);
    } else { 
        return spaces(10) + spaces(length - 10);
    }
    }

    private boolean isOverloadedNative(MethodInfo mi) { 
        ClassInfo ci = mi.parent;
    int nmethods = ci.methods.length;
    for (int j = 0; j < nmethods; j ++ ){
        MethodInfo m = ci.methods[j];
        if (    (m != mi) 
                 && ((m.access&Const.ACC_NATIVE) != 0 )
                 && (m.name.equals(mi.name))) { 
                return true;
            }
    } 
        return false;
    }

}
