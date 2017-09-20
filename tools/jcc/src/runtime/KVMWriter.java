/*
 *        KVMWriter.java        1.35        01/05/08        SMI
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
import java.util.Hashtable;
import java.io.OutputStream;

/*
 * The CoreImageWriter for the Embedded VM
 */

public class KVMWriter implements CoreImageWriter, Const, EVMConst {
    KVMStringTable stringTable = new KVMStringTable();
    KVMNameTable nameTable;
    KVMClassTable classTable;
    KVMStackMap stackMapUtil;

    /* INSTANCE DATA */
    protected  String     outputFileName;
    protected  Exception failureMode = null; // only interesting on failure

    protected OutputStream          xxx;
    CCodeWriter out;

    protected     vm.VMClassFactory     classMaker = new vm.EVMClassFactory();

    //CConstants sc;
    boolean        formatError = false;
    boolean        verbose     = false;
    boolean        classDebug  = false;

    boolean        buildingRelocationTable = false;
    boolean        relocatableROM = false;

    protected static final String           staticStoreName = "KVM_staticData";
    protected static final String           masterStaticStoreName = "KVM_masterStaticData";

    // In ROMjava.c we need to make several "forward static" declarations,
    // where the forward declaration has no initializer and the later
    // declaration has an initializer. Some compilers allow you to specify
    // simply "static" in both places. Other compiler treat this as a 
    // redefinition error and require the "forward" declaration to be
    // "external". So, we will emit the identifier
    // "FORWARD_STATIC_DECLARATION", and each specific compiler will
    // have to define this to be what is appropriate for that compiler.

    protected ClassnameFilterList           nativeTypes;

    protected boolean                       classLoading = true; // by default.

    /* for statistics only */
    int  ncodebytes;
    int  ncatchframes;
    int  nmethods;
    int  nfields;
    int  nconstants;
    int  njavastrings;

    public static final int ACC_ARRAY_CLASS        = 0x1000;
    public static final int ACC_ROM_CLASS          = 0x2000;
    public static final int ACC_ROM_NON_INIT_CLASS = 0x4000;
    public static final int ACC_POINTER            = Const.ACC_REFERENCE;
    public static final int ACC_DOUBLE             = Const.ACC_DOUBLEWORD;

    MethodInfo runCustomCodeMethod;
    MethodConstant runCustomCodeConstant;

    public KVMWriter( ){ 
        nameTable = new KVMNameTable(); 
        classTable = new KVMClassTable(nameTable);
        nameTable.classTable = classTable;
    stackMapUtil = new KVMStackMap(this, nameTable);
    }

    public void init( boolean classDebug, ClassnameFilterList nativeTypes, 
                      boolean verbose, int maxSegmentSize ){
        this.verbose = verbose;
        this.classDebug = classDebug;
        this.nativeTypes = nativeTypes;
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
        if (verbose) { 
            System.out.println(Localizer.getString("cwriter.writing_classes"));
        }
        try {

            initialPass(classes);

        runCustomCodeConstant = 
        new MethodConstant(new ClassConstant(
                    new UnicodeConstant("java/lang/Class")),
                   new NameAndTypeConstant(
                    new UnicodeConstant("runCustomCode"),
                    new UnicodeConstant("()V")));
        runCustomCodeMethod = runCustomCodeConstant.find();

        if (!buildingRelocationTable) { 
        // write out some constant pool stuff here,
        writeProlog();

        // Print out declarations for each of the classes
        writeAllClassDeclarations(classes);

        out.println("\014");

        // Print out the string table

        if (relocatableROM) { 
            out.println("void *StringSectionHeader = &StringSectionHeader;");
        }
        njavastrings = stringTable.writeStrings(this, "InternStringTable");
        if (relocatableROM) { 
            out.println("void *StringSectionTrailer = &StringSectionTrailer;");
        }
        
        /* Write out the static store */
        out.println("\014");
        writeStaticStore(classes);

        /* Write out the UTF table */
        out.println("\014");
        if (relocatableROM) { 
            out.println("void *UTFSectionHeader = &UTFSectionHeader;");
        }
        nameTable.writeTable(out, "UTFStringTable");
        if (relocatableROM) { 
            out.println("void *UTFSectionTrailer = &UTFSectionTrailer;");
        }
        /* Write out class definitions */
        out.println("\014");
        writeAllClassDefinitions(classes); 

        out.println("\014");
        writeEpilog();
        } else { 
        writeRelocationFile(classes);
        }
    } catch (DataFormatException e) { 
            out.flush();
            System.out.println(e);
            e.printStackTrace(System.out);
            formatError = true;
        } catch (RuntimeException e) { 
            out.flush();
            System.out.println(e);
            e.printStackTrace(System.out);
            formatError = true;
        }
        return (!formatError) && (! out.checkError());
    }

    protected void initialPass(ClassClass classes[]) { 
        for (int i = 0; i < classes.length; i++) { 
            EVMClass cc = (EVMClass) classes[i];
            if (cc.isPrimitiveClass()) { 
                /* Do nothing */
            } else if (cc.isArrayClass()) {             
                /* Make sure this is in the table */
                classTable.addArrayClass(cc.ci.className);   
            } else { 
                /* Make sure this is in the table */
                classTable.getClassKey(cc.ci.className);

                EVMMethodInfo m[] = cc.methods;
                FieldInfo      f[] = cc.ci.fields;
                ConstantObject cpool[] = cc.ci.constants;
                ClassConstant  intfs[] = cc.ci.interfaces;
        
                int methodCount = m.length;
                int fieldCount =  f.length;
                int constantCount = cpool.length;
                int intfCount = intfs.length;

                for (int index = 0; index < methodCount; index++) { 
            EVMMethodInfo meth = m[index];
            MethodInfo mi = meth.method;
                    classTable.getNameAndTypeKey(mi);
            stackMapUtil.initialPass(meth);
                }
                for (int index = 0; index < fieldCount; index++) { 
                    classTable.getNameAndTypeKey(f[index]);
            if (f[index].isStaticMember() && 
            f[index].value instanceof StringConstant) { 
                        stringTable.intern( (StringConstant) f[index].value);
            }
        }
                for (int index = 1; index < constantCount;) {
                    ConstantObject obj = cpool[index];
                    if (obj instanceof StringConstant){
                        stringTable.intern( (StringConstant) obj);
                    }
                    index += obj.nSlots;
                }
            }

        }
        for (int i = 0; i < publicNameTypes.length; i++) {
            String triple[] = publicNameTypes[i];
            String globalName = triple[0];
            String methodName = triple[1];
            String methodType = triple[2];
        classTable.getNameAndTypeKey(methodName, methodType);
        }

        stringTable.stringHashTable.closed = true;
        classTable.closed = true;
        nameTable.closed = true;
    stackMapUtil.showStatistics();
    }

    protected void writeAllClassDeclarations(ClassClass classes[]) 
               throws DataFormatException { 
    out.println("struct AllClassblocks_Struct {");
    Vector names = new Vector();

        for (Enumeration e = classTable.enumerate(); e.hasMoreElements(); ) { 
            String name = ((KVMClassName)e.nextElement()).toString();
            ClassInfo ci = ClassInfo.lookupClass(name);
            if (ci == null) { 
                if (name.charAt(0) == '[') { 
                    /* Let's create an array class for this thing */
                    ci = new ArrayClassInfo(false, name);
                    new EVMClass(ci);
                } else { 
                    String myName = Util.convertToClassName(name);
                    out.println("\tstruct instanceClassStruct " + myName + ";");
                    continue;
                }
            }
            EVMClass cc = (EVMClass)ci.vmClass;
            String myName = cc.getNativeName();
        names.addElement(myName);
            if (cc.isPrimitiveClass()) { 
                // We ignore these 
            } else if (cc.isArrayClass()) { 
                out.println("\tstruct arrayClassStruct " + myName + ";");
            } else { 
        out.println("\tstruct instanceClassStruct " + myName + ";");
        }
        }
    out.println("};");
    out.println();
    out.println("FORWARD_STATIC_DECLARATION");
    out.println("struct AllClassblocks_Struct AllClassblocks;");
    out.println();
    }

    /*
     * Merge the static store for all classes into one area.
     * Sort static cells, refs first, and assign offsets.
     * Write the resulting master data glob and arrange for
     * it to be copied to writable at startup.
     */
    void writeStaticStore (ClassClass classes[]) {
        int nclass = classes.length;
        int nStaticWords = 1; // 1 header word assumed
        int nRef    = 0;

        /*
         * Count all statics.
         * Count refs, and count number of words.
         */
        for (int cno = 0; cno < nclass; cno++ ){
            EVMClass c = (EVMClass)(classes[cno]);
            c.orderStatics();
            nRef += c.nStaticRef;
            nStaticWords += c.nStaticWords;
            FieldInfo f[] = c.statics;
        }

        /*
         * Assign offsets and at the same time
         * write any initial values.
         */
        int refOff = 1;
        int scalarOff = refOff + nRef;

    final ConstantObject staticInitialValue[] = 
        new ConstantObject[nStaticWords];

        for ( int cno = 0; cno < nclass; cno++ ){
            EVMClass c = (EVMClass)(classes[cno]);
            FieldInfo f[] = c.statics;
            if ((f == null) || (f.length == 0 )) { 
        continue; // this class has none
        }
            int nFields = f.length;
            for ( int i = 0; i < nFields; i++ ){
                FieldInfo fld = f[i];
                char toptype =  fld.type.string.charAt(0);
                if ( (toptype == 'L') || (toptype=='[')){
                    fld.instanceOffset = refOff;
            staticInitialValue[refOff] = fld.value;
                    refOff += 1;
                } else {
                    fld.instanceOffset = scalarOff;
            staticInitialValue[scalarOff] = fld.value;
                    scalarOff += fld.nSlots;
        }
        }
    }

    ConstantObject zero = new SingleValueConstant(0);
    staticInitialValue[0] = new SingleValueConstant(nRef);
    ArrayPrinter ap = 
        new ArrayPrinter() { 
            DoubleValueConstant previous;
        public void print(int index) { 
            ConstantObject value = staticInitialValue[index];
            if (value != null) { 
            out.print("ROM_STATIC_");
            if (value.nSlots == 1) { 
                writeConstant(value, false);
            } else { 
                DoubleValueConstant dval = 
                (DoubleValueConstant) value;
                String name = (value.tag == Const.CONSTANT_LONG)
                       ? "LONG" : "DOUBLE";
                out.print(name + "(");
                writeIntegerValue(dval.highVal);
                previous = dval;
            }
            } else if (previous != null) { 
            writeIntegerValue(previous.lowVal);
            out.print(")");
            previous = null;
            } else { 
            writeIntegerValue(0);
            }
        }
        };

        out.println("long "+staticStoreName+"["+nStaticWords+"];");
    out.println("struct {");
    out.println("\tlong count;");
    out.println("\tINSTANCE roots[" + nRef + "];");
    out.println("\tlong nonRoots[" + (nStaticWords - nRef - 1) + "];");
    out.println("} " + masterStaticStoreName + "= {");
    out.println("\t" + nRef + ",");
    out.println("\t{");
    writeArray(1, nRef + 1, 8, "\t\t", ap);
    out.println("\t},");
    out.println("\t{");
    writeArray(nRef + 1, nStaticWords, 4, "\t\t", ap);
    out.println("\t}");
        out.println("};\n");
    }

    protected void writeAllClassDefinitions(ClassClass classes[]) 
    throws DataFormatException
    {

        Vector instanceClasses = new Vector();
        for (Enumeration e = classTable.enumerate(); e.hasMoreElements(); ) { 
            String name = ((KVMClassName)e.nextElement()).toString();
            ClassInfo ci = ClassInfo.lookupClass(name);
            if (ci != null) { 
                EVMClass cc = (EVMClass)ci.vmClass;
                if (!cc.isPrimitiveClass() && !cc.isArrayClass()) { 
                    String nativeName = cc.getNativeName();
                    instanceClasses.addElement(cc);
                    instanceClasses.addElement(cc.getNativeName());
                }
            }
        }

    if (buildingRelocationTable) { 
        writeAllMethodDefinitions(instanceClasses);
        return;
    } 

    writeAllByteCodes(instanceClasses);
    out.println("\014");
    writeAllHandlers(instanceClasses);
    out.println("\014");
        writeAllStackMaps(instanceClasses);
        out.println("\014");

    writeAllMethodDefinitions(instanceClasses);
        out.println("\014");
        writeAllFieldDefinitions(instanceClasses);
        out.println("\014");
        writeAllConstantPoolDefinitions(instanceClasses);
        out.println("\014");
        writeAllInterfaceTableDefinitions(instanceClasses);
        
    if (relocatableROM) { 
        out.println("void *ClassDefinitionSectionHeader = &ClassDefinitionSectionHeader;");
    }

    out.println("static struct AllClassblocks_Struct AllClassblocks = {");

        for (Enumeration e = classTable.enumerate(); e.hasMoreElements(); ) { 
            String name = ((KVMClassName)e.nextElement()).toString();
            ClassInfo ci = ClassInfo.lookupClass(name);
            if (ci == null) { 
                writeRawClassDefinition(name);
            } else { 
                EVMClass cc = (EVMClass)ci.vmClass;
                if (cc.isPrimitiveClass()) { 
                    /* ignore */
                } else if (cc.isArrayClass()) { 
                    writeArrayClassDefinition(cc);
                } else { 
                    writeNormalClassDefinition(cc);
                }
            }
        }
    out.println("};");

        /* Write out the Class Table */
        out.println("\014");
        classTable.writeTable(out, "ClassTable");
        /* Write the primitive array table */
        out.println("\014");
        writePrimitiveArrayTable(classes);

    if (relocatableROM) { 
        out.println("void *ClassDefinitionSectionTrailer = &ClassDefinitionSectionTrailer;");
    }
    }

    protected void writeNormalClassDefinition(EVMClass c) {
        int methodCount = c.methods.length;
        int fieldCount =  c.ci.fields.length;
        int constantCount = c.ci.constants.length;
        int intfCount =  c.ci.interfaces.length;
        // This will be clear later.  The only important thing is that it
        // becomes zero after we've output the last thing.
        int extras = methodCount + fieldCount + constantCount + intfCount;
        int access = c.ci.access + ACC_ROM_CLASS;
        if (!c.hasStaticInitializer) { 
            /* This class doesn't need to be inited */
            access += ACC_ROM_NON_INIT_CLASS;
        }
        String nativeName = c.getNativeName();
        writeBasicClassInfo(c, "instanceClassStruct", "INSTANCE_INFO", access);
        out.print("\t\t" + c.instanceSize() + ", ");
        out.println(c.hasStaticInitializer ? "CLASS_VERIFIED, \\" : "CLASS_READY, \\");
        MethodInfo mi = c.findFinalizer();
        if (mi != null) {
            out.println("\t\t" + mi.getNativeName(true) + ", \\");
        } else {
            out.println("\t\tNULL, \\");
        }
        if (c.ci.superClass == null) {
            out.println("\t\tNULL, \\");
        } else {
            ClassInfo sci = ClassInfo.lookupClass(c.ci.superClass.name.string);
            String superName = ((EVMClass)(sci.vmClass)).getNativeName();
            out.println("\t\t&AllClassblocks." + superName + ", \\");
        }
        out.println("\t\t" 
                    + ((methodCount==0) ? "NULL" 
                 : ("&AllMethods." + nativeName + "_MethodSection." + 
                nativeName))
                    + ", \\");
        out.println("\t\t" 
                    + ((fieldCount==0) ? "NULL" : ("&AllFields." + nativeName))
                    + ", \\");
        out.println("\t\t" 
                    + ((constantCount==0) ? "NULL" : ("&AllConstantPools." + nativeName))
                    + ", \\");
        out.println("\t\t" 
                    + ((intfCount == 0) ? "NULL" : ("&AllInterfaces." + nativeName))
                    + " ),");
        // out.println("};\n");
    }

    protected void writeRawClassDefinition(String className){
        String nativeName = Util.convertToClassName(className);
        KVMClassName cn = new KVMClassName(className);
        writeBasicClassInfo(nativeName, cn, 
                            "instanceClassStruct", "RAW_CLASS_INFO", 0);
        out.println("\t\tNULL),");
        // out.println("};");
    }

    protected void writeArrayClassDefinition(EVMClass c){
        KVMClassName cn = new KVMClassName(c.ci.className);
        int access = c.ci.access + ACC_ARRAY_CLASS + ACC_ROM_CLASS; 
        ConstantObject cp = c.ci.constants[1];
        if (cp.tag == Const.CONSTANT_CLASS) {   
            ArrayClassInfo aci = (ArrayClassInfo)c.ci;
            ClassInfo elemClass = 
                ClassInfo.lookupClass(((ClassConstant)cp).name.string);
            String elemName = ((EVMClass)(elemClass.vmClass)).getNativeName();
            writeBasicClassInfo(c, "arrayClassStruct", 
                                "ARRAY_OF_OBJECT", access);
        out.println("\t\tAllClassblocks." + elemName + "),");
        } else {
            String baseName = ((ArrayClassInfo)(c.ci)).baseName.toUpperCase();
            writeBasicClassInfo(c, "arrayClassStruct", 
                                "ARRAY_OF_PRIMITIVE", access);
            
            //out.println("\t\t" + baseName + ")};\n");
        out.println("\t\t" + baseName + "),");
        }
    }
    
    protected void writeBasicClassInfo(EVMClass c,
                                     String type, String macro, int access) {
        writeBasicClassInfo(c.getNativeName(), new KVMClassName(c.ci.className),
                            type, macro, access);
    }

    protected void writeBasicClassInfo(String nativeName, 
                                     KVMClassName cn,
                                     String type, String macro, int access) {
        String fullBaseName = cn.getFullBaseName();
        String packageName = cn.getPackageName();
        int packageKey = -1;
        if (packageName != null) { 
            packageKey = classTable.getNameKey(packageName);
            nameTable.declareUString(out, packageName);
        }
        int baseKey = classTable.getNameKey(fullBaseName);      
        nameTable.declareUString(out, fullBaseName);

        int classKey = classTable.getClassKey(cn);

        out.println("\t" + macro + "( \\");
        if (packageKey == -1) {
            out.println("\t\tNULL, ");
        } else { 
            out.println("\t\t" + nameTable.getUString(packageName) 
                        + ",  /* " + packageName + " */ \\");
        }
        out.println("\t\t" + nameTable.getUString(fullBaseName) 
                    + ",  /* " + fullBaseName + " */ \\");
        KVMClassName nextName = (KVMClassName)classTable.getNext(cn);
        if (nextName == null) { 
            out.print("\t\tNULL, ");
        } else {
            EVMClass next = nextName.getEVMClass();
            String nextNativeName = 
                (next != null) ? next.getNativeName()
                               : Util.convertToClassName(nextName.toString());
            out.print("\t\t&AllClassblocks." + nextNativeName + ", ");
        }
        out.printHexInt(classKey);
        out.print(", ");
        out.printHexInt(access);
        out.println(", \\");
    }

    protected void writeAllMethodDefinitions(Vector instanceClasses) {
        Vector todo = new Vector();
        out.println("struct AllMethods_Struct { ");
    SectionCounter sectionCounter = 
        new SectionCounter(relocatableROM 
                   /* Each method is roughly 40 bytes */
                   ? 50000 / 40 
                   : Integer.MAX_VALUE);
    
    sectionCounter.startFirstSection(SectionCounter.Definition);
        for (Enumeration e = instanceClasses.elements(); e.hasMoreElements();) {
            EVMClass cc = (EVMClass)e.nextElement();
            String nativeName = (String)e.nextElement();
            int length = cc.methods.length;
        sectionCounter.maybeNextSection(length);
            if (length > 0) { 
                todo.addElement(cc);
        todo.addElement(nativeName);
                out.println("\t\tstruct {");
        out.println("#define " + nativeName + "_MethodSection " + 
                "section" + sectionCounter.getSection());
        if (buildingRelocationTable) { 
            out.println("#define " + nativeName + 
                "_MethodSectionNumber " + 
                sectionCounter.getSection());
        }
        out.println("\t\t\tlong length;");
                out.println("\t\t\tstruct methodStruct methods[" 
                             + length+ "];");
                out.println("\t\t} " + nativeName + ";");
            }
        }
    sectionCounter.endLastSection();
    out.println("};");
    if (buildingRelocationTable) { 
        return;
    }
    
    out.println("static CONST struct AllMethods_Struct AllMethods = {");
    sectionCounter.startFirstSection(SectionCounter.Initialization);
    for (Enumeration e = todo.elements(); e.hasMoreElements(); ) { 
        EVMClass cc = (EVMClass)e.nextElement();
            String nativeName = (String)e.nextElement();
            int length = cc.methods.length;
        sectionCounter.maybeNextSection(length);
        out.println("\t\t{");
        out.println("\t\t\t/* " + cc.ci.className + " */");
        writeMethods(cc);
        out.println("\t\t},");
    }
    sectionCounter.endLastSection();
    out.println("};\n");
    if (!relocatableROM) { 
        // We need to coercion to remove the CONST-ness
        out.print("METHOD RunCustomCodeMethod = (METHOD) ROM_CLINIT_");
        writeConstant(runCustomCodeConstant, true);
        out.println(";\n");
    } else { 
        sectionCounter.printRelocationInfo("AllMethods", "METHODTABLE");
    }
    
    }

    protected void writeAllFieldDefinitions(Vector instanceClasses) { 
        Vector todo = new Vector();
        out.println("struct AllFields_Struct { ");
        for (Enumeration e = instanceClasses.elements(); e.hasMoreElements();) {
            EVMClass cc = (EVMClass)e.nextElement();
            String nativeName = (String)e.nextElement();
            int length = cc.ci.fields.length;
            if (length > 0) { 
                todo.addElement(cc);
                out.println("\tstruct {");
                out.println("\t\tlong length;");
                out.println("\t\tstruct fieldStruct fields[" + length+ "];");
                out.println("\t} " + nativeName + ";");
            }
        }
        out.println("};");
    out.println("static CONST struct AllFields_Struct AllFields = { ");
        for (Enumeration e = todo.elements(); e.hasMoreElements();) {
            EVMClass cc = (EVMClass)e.nextElement();
            out.println("\t{");
            out.println("\t\t/* " + cc.ci.className + " */");
            writeFields(cc);
            out.println("\t},");
        }
        out.println("};\n");
    }

    protected void writeAllConstantPoolDefinitions(Vector instanceClasses) {
        Vector todo = new Vector();
        out.println("struct AllConstantPools_Struct { ");
        for (Enumeration e = instanceClasses.elements(); e.hasMoreElements();) {
            EVMClass cc = (EVMClass)e.nextElement();
            String nativeName = (String)e.nextElement();
            int length = cc.ci.constants.length;
            if (length > 0) { 
                todo.addElement(cc);
                out.println("\tstruct {");
                out.println("\t\tunion ROMconstantPoolEntryStruct entries[" + length + "];");
                out.println("\t\tunsigned char tags[" + length + "];");
                out.println("\t} " + nativeName + ";");
            }
        }
        out.println("};");
    out.println("static CONST struct AllConstantPools_Struct AllConstantPools = {");
        for (Enumeration e = todo.elements(); e.hasMoreElements();) {
            EVMClass cc = (EVMClass)e.nextElement();
            out.println("\t{");
            out.println("\t\t/* " + cc.ci.className + " */");
            writeConstantPool(cc);
            out.println("\t},");
        }
        out.println("};\n");
    }

    protected void writeAllInterfaceTableDefinitions(Vector instanceClasses) {
        Vector todo = new Vector();
        out.println("struct AllInterfaces_Struct { ");
        for (Enumeration e = instanceClasses.elements(); e.hasMoreElements();) {
            EVMClass cc = (EVMClass)e.nextElement();
            String nativeName = (String)e.nextElement();
            int length = cc.ci.interfaces.length;
            if (length > 0) { 
                todo.addElement(cc);
                out.println("\tstruct {");
                out.println("\t\tunsigned short length;");
                out.println("\t\tunsigned short index[" + length + "];");
                out.println("\t} " + nativeName + ";");
            }
        }
    out.println("};");
    out.println();
        out.println("static CONST struct AllInterfaces_Struct AllInterfaces = { ");
        for (Enumeration e = todo.elements(); e.hasMoreElements();) {
            EVMClass cc = (EVMClass)e.nextElement();
            out.println("\t{");
            out.println("\t\t/* " + cc.ci.className + " */");
            writeInterfaces(cc);
            out.println("\t},");
        }
        out.println("};\n");
    }
    

    protected void writeAllByteCodes(Vector instanceClasses) 
          throws DataFormatException{ 
        /* Find the values of the two special methods */
    Vector todo = new Vector();
    Vector natives = new Vector();
    SectionCounter sectionCounter = 
        new SectionCounter(relocatableROM ? 50000 : Integer.MAX_VALUE);
    
        out.println("struct AllCode_Struct { ");
    int padCount = 0;    // just so each padding will have a name

    sectionCounter.startFirstSection(SectionCounter.Definition);

        for (Enumeration e = instanceClasses.elements(); e.hasMoreElements();) {
            EVMClass cc = (EVMClass)e.nextElement();
            String classNativeName = (String)e.nextElement(); // ignored
            EVMMethodInfo  m[] = cc.methods;
            int nmethod = m.length;
            for (int j = 0; j < nmethod; j++){
                EVMMethodInfo meth = m[j];
                MethodInfo mi = meth.method;
                if ((mi.access & Const.ACC_ABSTRACT) != 0) { 
                    /* Do nothing */
                } else if ((mi.access & Const.ACC_NATIVE) != 0) { 
            natives.addElement(meth);
                } else { 
            String methodNativeName = meth.getNativeName();
                    int alignment = meth.alignment();
            int padding = 
            alignment - (sectionCounter.getOffset() % alignment);
            if (padding != alignment) { 
            sectionCounter.notNextSection(padding);
            out.println("\t\tBYTE padding" + (++padCount) + "["
                    + padding + "];");
            todo.addElement(new Integer(padding));
            }
            sectionCounter.maybeNextSection(mi.code.length);
            out.println("#define " + methodNativeName + "_CodeSection "
                + "section" + sectionCounter.getSection());
            out.println("\t\t/* " +  prettyName(mi) + " */");
            out.println("\t\tBYTE " + methodNativeName 
                + "[" + mi.code.length + "];" );
            todo.addElement(meth);
            todo.addElement(methodNativeName);
        }
        }
    }
    sectionCounter.endLastSection();
    ncodebytes = sectionCounter.getTotalLength();
        out.println("};");
    out.println();

    out.println("#if !ENABLEFASTBYTECODES && !ENABLE_JAVA_DEBUGGER");
    out.println("CONST");
    out.println("#endif");
    out.println("static struct AllCode_Struct AllCode = {");
    sectionCounter.startFirstSection(SectionCounter.Initialization); 
    for (Enumeration e = todo.elements(); e.hasMoreElements(); ) { 
        Object nextElement = e.nextElement();
        if (nextElement instanceof Integer) { 
        int padding = ((Integer)nextElement).intValue();
        sectionCounter.notNextSection(padding);
        out.println("\t\t{ 0 }, /* padding size " + padding + " */");
        continue;
        }
            EVMMethodInfo meth = (EVMMethodInfo)nextElement;
        String methodNativeName = (String)e.nextElement();
            final MethodInfo mi = meth.method;
        sectionCounter.maybeNextSection(mi.code.length);
            out.println("\t\t{ /* " + mi.parent.className + ": " 
                      + prettyName(mi) + " */");
        writeArray(mi.code.length, 10, "\t\t\t", 
               new ArrayPrinter() { 
                          public void print(int index) { 
                  if (index == 0 && (mi == runCustomCodeMethod)) {
                  out.print("CUSTOMCODE");
                  } else { 
                  int value = mi.code[index] & 0xFF;
                  out.printHexInt(value);
                  }
              }
                    });
        out.println("\t\t},");
    }
    sectionCounter.endLastSection();
        out.println("};\n");

    if (!relocatableROM) { 
        for (Enumeration e = natives.elements(); e.hasMoreElements(); ) {
        EVMMethodInfo meth = (EVMMethodInfo)e.nextElement();
        String jniName = meth.method.getNativeName(true);
        out.println("extern void " + jniName + "(void);");
        }
    }
    if (relocatableROM) { 
        sectionCounter.printRelocationInfo("AllCode", "CODE");
    }
    }

    protected void writeAllHandlers(Vector instanceClasses) { 
        Vector todo = new Vector();
        out.println("struct AllHandlers_Struct { ");
    ExceptionEntry empty[] = new ExceptionEntry[0];
        for (Enumeration e = instanceClasses.elements(); e.hasMoreElements();) {
            EVMClass cc = (EVMClass)e.nextElement();
            String classNativeName = (String)e.nextElement(); // ignored
            EVMMethodInfo  m[] = cc.methods;
            int nmethod = m.length;
            for (int j = 0; j < nmethod; j++){
                EVMMethodInfo meth = m[j];
                MethodInfo mi = meth.method;
        if (mi.exceptionTable == null) { 
            mi.exceptionTable = empty;
        }
                int tryCatches = mi.exceptionTable.length;
                if (tryCatches > 0) { 
                    String methodNativeName = meth.getNativeName();
                    todo.addElement(meth);
                    ncatchframes += tryCatches;
                    out.println("\tstruct { /* " 
                + mi.parent.className + ": " 
                + prettyName(mi) + "*/");
                    out.println("\t\tlong length;");
                    out.println("\t\tstruct exceptionHandlerStruct handlers[" + 
                                     tryCatches + "];");
                    out.println("\t} " + methodNativeName + ";");

                } 
        }
        }
    out.println("};");
    out.println();

    out.println("static CONST struct AllHandlers_Struct AllHandlers = {");
        for (Enumeration e = todo.elements(); e.hasMoreElements(); ) { 
            EVMMethodInfo meth = (EVMMethodInfo)e.nextElement();
            MethodInfo mi = meth.method;
            ExceptionEntry[] exceptions = mi.exceptionTable;
            out.println("\t{ /* " + mi.parent.className + ": " 
             + prettyName(mi) + "*/");
            out.println("\t\t" + exceptions.length + ",");
            out.println("\t\t{");
            int tryCatches = exceptions.length;
            for (int k = 0; k < tryCatches; k++) { 
                ExceptionEntry ee = mi.exceptionTable[k];
                out.println("\t\t\tHANDLER_ENTRY(" + ee.startPC + ", " + 
                            ee.endPC + ", " + ee.handlerPC + ", " + 
                            (ee.catchType == null ? 0 : ee.catchType.index)
                            + ")" + ((k == tryCatches - 1) ? "" : ","));
            }
            out.println("\t\t}");
            out.println("\t},\n");
        }
        out.println("};");
    }

    protected void writeAllStackMaps(Vector instanceClasses) { 
        Vector todo = new Vector();
    StackMapFrame empty[] = new StackMapFrame[0];

        out.println("struct AllStackMaps_Struct { ");
        for (Enumeration e = instanceClasses.elements(); e.hasMoreElements();) {
            EVMClass cc = (EVMClass)e.nextElement();
            String classNativeName = (String)e.nextElement(); // ignored
            EVMMethodInfo  m[] = cc.methods;
            int nmethod = m.length;
            for (int j = 0; j < nmethod; j++){
                EVMMethodInfo meth = m[j];
                MethodInfo mi = meth.method;
        if (mi.stackMapTable == null) { 
            mi.stackMapTable = empty;
        }
        if (mi.stackMapTable.length > 0) { 
                    todo.addElement(meth);
            stackMapUtil.printDeclaration(out, meth, "\t");
                } 
        }
        }
    out.println("};");
    out.println();

        out.println("static CONST struct AllStackMaps_Struct AllStackMaps = {");
        for (Enumeration e = todo.elements(); e.hasMoreElements(); ) { 
            EVMMethodInfo meth = (EVMMethodInfo)e.nextElement();
        stackMapUtil.printDefinition(out, meth, "\t");
    }
        out.println("};");
    }

    protected void writeMethods(EVMClass c) {
        EVMMethodInfo m[] = c.methods;
        int methodCount =  m.length;
        out.println("\t\t\t" + methodCount + ",");
    out.println("\t\t\t{");
        for (int i = 0; i < methodCount; i++) { 
            EVMMethodInfo meth = m[i];  
            MethodInfo mi = meth.method;
        int access = mi.access;
        String typeString = mi.type.string;
        switch (typeString.charAt(typeString.indexOf(')') + 1)) { 
        case 'L': case '[':
            access |= ACC_POINTER; break;
        case 'J': case 'D': 
            access |= ACC_DOUBLE; break;
        case 'V': 
            access |= (ACC_DOUBLE | ACC_POINTER); break;
            case 'B': case 'C': case 'S': case 'I': case 'F': case 'Z':
            break;
        default:
            System.out.println("Bad type string " + typeString);
        }

        String type = ((access & Const.ACC_ABSTRACT) != 0) ? "ABSTRACT_"
                : ((access & Const.ACC_NATIVE) != 0)   ? "NATIVE_"
                : "";
            out.println("\t\t\t\t" + type + "METHOD_INFO( /* " + 
            prettyName(mi) + " */ \\");
            out.println("\t\t\t\t\t" + c.getNativeName() + ", \\");
        if ((access & Const.ACC_ABSTRACT) != 0) { 
        // do nothing
        } else if ((access & Const.ACC_NATIVE)  != 0) { 
        if (relocatableROM) { 
            out.println("\t\t\t\t\tNULL, \\");
        } else { 
            out.println("\t\t\t\t\t" + mi.getNativeName(true) + ", \\");
        }
        } else { 
        String methodNativeName = meth.getNativeName();
        out.println("\t\t\t\t\tAllCode." 
                + methodNativeName + "_CodeSection."
                + methodNativeName + ", \\");
                if (mi.exceptionTable.length > 0) { 
                    out.println("\t\t\t\t\t&AllHandlers." + methodNativeName
                + ", \\");
        } else { 
            out.println("\t\t\t\t\t0, \\");
        }
                if (mi.stackMapTable.length > 0) { 
                    out.println("\t\t\t\t\t&AllStackMaps." + methodNativeName
                + ", \\");
        } else { 
            out.println("\t\t\t\t\t0, \\");
        }
        }
            out.print("\t\t\t\t\t");
            out.printHexInt(access);
            out.print(", ");
            out.print(mi.argsSize + ", ");
            if ((access & (Const.ACC_ABSTRACT + Const.ACC_NATIVE)) != 0) { 
                // out.println(mi.argsSize + ", 0, 0, \\");
            } else {
        int locals = mi.locals;
        int stackSize = mi.stack;
        int codeLength = mi.code.length;
        if (mi == runCustomCodeMethod) { 
            out.println(locals + ", RunCustomCodeMethod_MAX_STACK_SIZE, " + codeLength + ", \\");
        } else { 
            out.println(locals + ", " + stackSize + ", " + codeLength + ", \\");
        }
        out.print("\t\t\t\t\t");
            }
            out.print(classTable.getNameAndTypeKey(mi));
            out.println((i == methodCount - 1) ? ")" : "),");
        }
    out.println("\t\t\t}");
        nmethods += methodCount;
    }

    void writeFields(EVMClass c) {
        // Some day, I'll have to deal with > 255 fields.
        // Today is not that day. Just do the simple case.
        FieldInfo ff[] = c.ci.fields;
        int nfields = ff == null ? 0 : ff.length;
        out.println("\t\t" + nfields + ",");
    out.println("\t\t{");
        for ( int i = 0; i < nfields; i++ ){
            FieldInfo f = ff[i];
        if (f.isStaticMember()) { 
        out.print("\t\t\tSTATIC_FIELD_INFO( ");
        } else { 
        out.print("\t\t\tFIELD_INFO( ");
        }
        out.println("/* " + prettyName(f) + " */ \\");
            out.print("\t\t\t\t" + c.getNativeName() + ", ");
            /* Note that access already includes ACC_DOUBLE, ACC_POINTER */
            out.printHexInt(f.access);
            out.print(", " + f.instanceOffset + ", " 
              +  classTable.getNameAndTypeKey(f) + ")");
        out.println(  (i == nfields - 1) ? "" : ","  );
        }
    out.println("\t\t}");
        this.nfields += nfields;
    }

    protected void writeConstantPool(EVMClass c) { 
        final ConstantObject cpool[] = c.ci.constants;
        int length = cpool.length;
    final int[] tags = new int[length];

    out.println("\t\t{");
        out.println("\t\t\tROM_CPOOL_LENGTH(" + length + "),");
    for (int index = 1; index < length; ) { 
        ConstantObject cp = cpool[index];
        int tag = cp.tag;
        switch (tag) { 
            case Const.CONSTANT_INTEGER: case Const.CONSTANT_STRING:
            case Const.CONSTANT_LONG:   case Const.CONSTANT_DOUBLE:
            tags[index] = tag;
            break;
            default:
            tags[index] = tag | (cp.isResolved() ? 0x80:0);
            break;
        }
        out.print("\t\t\tROM_CPOOL_");
            writeConstant(cpool[index], true);
        index += cp.nSlots;
        out.println(index < length ? "," : "");
    }
    out.println("\t\t},");
        out.println("\t\t{");
        writeArray(length, 12, "\t\t\t", 
                   new ArrayPrinter() { 
                       public void print(int index) { out.print(tags[index]); }
                   });
        nconstants += length;
        out.println("\t\t}");
    }

    protected void writeInterfaces(final EVMClass c) { 
        int length = c.ci.interfaces.length;
        out.println("\t\t" + length + ", /* interfaces */");
        out.println("\t\t{");
        writeArray(length, 12, "\t\t\t", 
                new ArrayPrinter() { 
                   public void print(int index) { 
                       out.print(c.ci.interfaces[index].index);
                   } });
        out.println("\t\t}");
    }

    protected void writePrimitiveArrayTable(ClassClass classes[]) { 
        final EVMClass[] table = new EVMClass[12];
        for (int i = 0; i < classes.length; i++) { 
            EVMClass cc = (EVMClass) classes[i];
            if (cc.isArrayClass()) { 
                ArrayClassInfo aci = (ArrayClassInfo)cc.ci;
                if (aci.constants[1].tag == Const.CONSTANT_INTEGER) { 
                   int base = ((SingleValueConstant)aci.constants[1]).value;
                   table[base] = cc;
                }
            }
        }
        out.println("ARRAY_CLASS PrimitiveArrayClasses[12] = {");
        writeArray(0, 12, 1, "\t", new ArrayPrinter() {
            public void print(int index) { 
                EVMClass entry = table[index];
                if (entry == null) { 
                    out.print("NULL");
                } else { 
                    out.print("&AllClassblocks." + entry.getNativeName());
                } 
            }
        });
        out.println("};\n");
    }

    protected void
    writeIntegerValue( int v ){
         // little things make gcc happy.
         if (v==0x80000000)
            out.print( "(long)0x80000000" );
        else
             out.print( v );
    }

    protected void
    writeLongValue( String tag, int highval, int lowval ){
        out.print( tag );
        out.write( '(' );
        writeIntegerValue( highval );
        out.print( ", " );
        writeIntegerValue( lowval );
        out.write( ')' );
    }

    protected void
    writeConstant(ConstantObject value, boolean verbose){
        switch ( value.tag ){
        case Const.CONSTANT_INTEGER:
        case Const.CONSTANT_FLOAT:
        out.print("INT(");
        writeIntegerValue(((SingleValueConstant)value).value);
        out.print(")");
        break;

        case Const.CONSTANT_UTF8:
             System.out.println("Cannot write UTF entry: \"" + 
                                ((UnicodeConstant)value).string + "\"");
         out.print("UNKNOWN()");
             formatError = true;
             break;

        case Const.CONSTANT_STRING:
        out.print("STRING(");
            stringTable.writeString(out, (StringConstant)value);
        out.print(")");
        if (verbose) { 
        out.print(" /* ");
        out.printSafeString(((StringConstant)value).str.string);
        out.print(" */");
        }
            break;

        case Const.CONSTANT_LONG: {
        DoubleValueConstant dval = (DoubleValueConstant) value;
        writeLongValue( "LONG",  dval.highVal, dval.lowVal );
        break;
    }

        case Const.CONSTANT_DOUBLE: {
            DoubleValueConstant dval = (DoubleValueConstant) value;
            writeLongValue( "DOUBLE",  dval.highVal, dval.lowVal );
            break;
    }

        case Const.CONSTANT_CLASS:
            if (value.isResolved()) {
                ClassInfo ci = ((ClassConstant)value).find();
                EVMClass c = (EVMClass)(ci.vmClass);
                out.print("CLASS(" + c.getNativeName() + ")");
            } else {
                System.out.println("Unresolved class constant: "+value.toString() );
        formatError = true;
        out.print("UNKNOWN()");
            }
            break;

        case Const.CONSTANT_METHOD:
        case Const.CONSTANT_INTERFACEMETHOD:
            if ( value.isResolved() ){
                MethodInfo mi = ((MethodConstant)value).find();
                ClassInfo  ci = mi.parent;
                EVMClass   EVMci = (EVMClass)(ci.vmClass);
        out.print("METHOD(" + EVMci.getNativeName() 
              + ", " + mi.index + ")");
        if (verbose) { 
            out.print(" /* " + prettyName(mi) + " */");
        }
            } else {
        formatError = true;
        out.print("UNKNOWN()");
                System.out.println("Unresolved method constant: "+value.toString() );
            }
            break;

        case Const.CONSTANT_FIELD:
            if ( value.isResolved() ){
                FieldInfo fi = ((FieldConstant)value).find();
                ClassInfo ci = fi.parent;
                String className = ((EVMClass)(ci.vmClass)).getNativeName();
        out.print("FIELD(" + className + ", " + fi.index + ")");
        if (verbose) { 
            out.print(" /* " + prettyName(fi) + " */");
        }
        } else {
        formatError = true;
        out.print("UNKNOWN()");
                System.out.print("Unresolved field constant: "+value.toString() );
            }
            break;

        default:
        formatError = true;
        out.print("UNKNOWN()");
            System.out.print("ERROR: constant " + value.tag);
        }
    }

    /*
     * These are macros to encapsulate some of the messiness
     * of data definitions. They are, perhaps, compiler-specific.
     */
    protected static String stdHeader[] = {
        "#define COMPILING_ROMJAVA 1",
        "",
        "#include <global.h>",
        "#include \"rom.h\"",
        "",
        "#if ROMIZING",
        ""
    };

    protected void writeProlog(){
    java.util.Date date = new java.util.Date();
        out.println("/* This is a generated file.  Do not modify.");
        out.println(" * Generated on " + date);
        out.println(" */\n");
    out.println();
    out.println("#define ROM_GENERATION_DATE \"" + date + "\"");
    out.println();
        for ( int i = 0; i < stdHeader.length; i++ ){
            out.println( stdHeader[i] );
        }
    }
    
    String publicClasses[][] = 
              { {"JavaLangObject",    "java/lang/Object"   },
                {"JavaLangClass",     "java/lang/Class"    }, 
                {"JavaLangString",    "java/lang/String"   }, 
                {"JavaLangThread",    "java/lang/Thread"   },
                {"JavaLangSystem",    "java/lang/System"   },
                {"JavaLangThrowable", "java/lang/Throwable"},
                {"JavaLangError",     "java/lang/Error"},
              };
                           
    String publicNameTypes[][] = 
              { { "initNameAndType",   "<init>",   "()V"   },
                { "clinitNameAndType", "<clinit>", "()V"   },
                { "runNameAndType",    "run",      "()V"   },
                { "mainNameAndType",   "main",     "([Ljava/lang/String;)V"}
              };

    protected void writeEpilog() {
                           
        String epilogCode[] = {
            "void InitializeROMImage() { ",
            "    memcpy(" + staticStoreName + ", &" + masterStaticStoreName + ", sizeof(KVM_staticData));",
            "}",
            "",
            "void FinalizeROMImage() { ",
            "    finalizeROMHashTable(UTFStringTable, offsetof(struct UTF_Hash_Entry, next));", 
            "    finalizeROMHashTable(InternStringTable, offsetof(struct internedStringInstanceStruct, next));",
            "    finalizeROMHashTable(ClassTable, offsetof(struct classStruct, next));", 
            "    KVM_staticData[0] = 0;",
            "}", 
        "", 
            "#if INCLUDEDEBUGCODE",
        "bool_t isROMString(void *x) { ",
        "    if (x == (void *)(&stringCharArrayInternal.ofClass)) { ", 
        "        return TRUE;", 
        "    } else { ", 
        "        void *start = (void *)&stringArrayInternal[0];", 
        "        void *end = (void *)((char *)start + sizeof(stringArrayInternal));", 
        "        return x >= start && x < end;",
        "    }", 
        "}", 
        "", 
        "bool_t isROMClass(void *x) { ", 
        "    void *start = (void *)&AllClassblocks;", 
        "    void *end  = (void *)((char *)start + sizeof(AllClassblocks));",
        "    return x >= start && x < end;", 
        "}", 
            "#endif", 
        "", 
            "#endif"
        };
        
        for (int i = 0; i < publicClasses.length; i++) {
            String[] pair = publicClasses[i];
            String globalName = pair[0];
            String className = pair[1];
            ClassInfo ci = ClassInfo.lookupClass(className);
            EVMClass cc = (EVMClass)ci.vmClass;
            String type = cc.isArrayClass() ? "ARRAY_CLASS" : "INSTANCE_CLASS";
            out.println(type + " " + globalName + " = (" + type + ")&" +
            "AllClassblocks." + cc.getNativeName() + ";");
        }
        for (int i = 0; i < publicNameTypes.length; i++) {
            String triple[] = publicNameTypes[i];
            String globalName = triple[0];
            String methodName = triple[1];
            String methodType = triple[2];
            out.println("NameTypeKey " + globalName + " = " 
                        + classTable.getNameAndTypeKey(methodName, methodType) + ";");
        }
        out.println();
        for (int i = 0; i < epilogCode.length; i++) {
            out.println(epilogCode[i]);
        }
        
    }

    public void 
    writeArray(int start, int length, 
           int columns, String indent, ArrayPrinter ap) { 
        CCodeWriter out = this.out;
        int column = 0;
        out.print(indent);  
        for ( int i = start; ; i++ ){
            if (column >= columns) { 
                out.print("\n" + indent);
                column = 1;
            } else {
                column += 1;
            }
            ap.print(i);
            if (i < (length - 1)) { 
                out.print(", ");
            } else { 
                out.println(ap.finalComma() ? ", " : "");
                break;
            }
        }
    }

    public void 
    writeArray(int length, int columns, String indent, ArrayPrinter ap) { 
    writeArray(0, length, columns, indent, ap);
    }
    
    public String prettyName(MethodInfo mi) { 
    String name = mi.name.string;
    String type = mi.type.string;
    int lastParen = type.lastIndexOf(')');
    StringBuffer result = new StringBuffer();
    
    /* First, put in the result type */
    if (!name.equals("<init>") && !name.equals("<clinit>")) { 
        typeName(type, lastParen + 1, result);
        result.append(' ');
    }
    result.append(name).append('(');
    for (int index = 1; index < lastParen; ) { 
        if (index > 1) { 
        result.append(", ");
        }
            index = typeName(type, index, result);
        }
    return result.append(')').toString();
    }

    public String prettyName(FieldInfo fi) { 
    String name = fi.name.string;
    String type = fi.type.string;

    StringBuffer result = new StringBuffer();
    typeName(type, 0, result);

    return result.append(' ').append(name).toString();
    }

    private int
    typeName(String type, int index, StringBuffer result) {
    int end;
    switch(type.charAt(index)) { 
        case 'V':  result.append("void");    return index + 1;
        case 'Z':  result.append("boolean"); return index + 1;
        case 'B':  result.append("byte");    return index + 1;
        case 'S':  result.append("short");   return index + 1;
          case 'C':  result.append("char");    return index + 1;
        case 'I':  result.append("int");     return index + 1;
        case 'J':  result.append("long");    return index + 1;
        case 'F':  result.append("float");   return index + 1;
        case 'D':  result.append("double");  return index + 1; 

        case 'L': { 
        end = type.indexOf(';', index) + 1;
        String className = type.substring(index + 1, end - 1);
        if (className.startsWith("java/lang/")) { 
            className = className.substring(10);
        }
        result.append(className.replace('/', '.'));
        return end;
        }

        case '[': { 
        end = typeName(type, index + 1, result);
        result.append("[]");
        return end;
        }
        default:
        System.err.println("Unrecognized character");
        result.append("XXXX");
        return index + 1;
    }
    }

    public void printSpaceStats( java.io.PrintStream o ){
        ClassClass classes[] = ClassClass.getClassVector( classMaker );
        o.println(Localizer.getString("cwriter.total_classes", Integer.toString(classes.length)));
        o.println(Localizer.getString("cwriter.method_blocks", Integer.toString(nmethods)));
        o.println(Localizer.getString("cwriter.bytes_java_code", Integer.toString(ncodebytes)));
        o.println(Localizer.getString("cwriter.catch_frames", Integer.toString(ncatchframes)));
        o.println(Localizer.getString("cwriter.field_blocks", Integer.toString(nfields)));
        o.println(Localizer.getString("cwriter.constant_pool_entries", Integer.toString(nconstants)));
        o.println(Localizer.getString("cwriter.java_strings",Integer.toString(njavastrings)));
    }
    
    abstract public static class ArrayPrinter { 
        abstract void print(int index);
        boolean finalComma() { return false; }
    }

    public static class ArrayEqual { 
        Object array;
        ArrayEqual(byte[] x) { this.array = x; }

        public int hashCode() { 
            byte[] a = (byte[])array;
            int total = 0;
            for (int i = 0; i < a.length; i++) { 
                total = (total * 3) + a[i];
            }
            return total;
        }

        public boolean equals(Object y) { 
            if (this.getClass() != y.getClass()) { 
                return false;
            }
            byte[] a = (byte[])array;
            byte[] b = (byte[])((ArrayEqual)y).array;
            if (a.length != b.length) { 
                return false;
            }
            for (int i = 0; i < a.length; i++) { 
                if (a[i] != b[i]) { 
                    return false;
                }
            }
            return true;
        }
    }

    // Allow code and methods to be broken up into smaller pieces, if
    // necessary
    class SectionCounter { 
    int maxOffset;
    int offset, totalLength;
    boolean inDefinitions;
    int section;

    SectionCounter(int maxOffset) { 
        this.maxOffset = maxOffset; 
    }
    
    public static final boolean Definition = true;
    public static final boolean Initialization = false;

    public void startFirstSection(boolean inDefinitions) { 
        this.inDefinitions = inDefinitions;
        offset = 0;
        totalLength = 0;
        section = 1;
        startSection();
    }
    
    public void endLastSection() { 
        endSection(true);
    }

    public void notNextSection(int thisLength) { 
        // Indicate that we're adding a small item, that
        // for padding purposes or otherwise, must remain with the
        // previous section.
        offset += thisLength;
    }

    public void maybeNextSection(int thisLength) { 
        // Start a new section, if necessary
        if (offset + thisLength > maxOffset) { 
        endSection(false);
        section++;
        startSection();
        }
        offset += thisLength;
    }

    private void startSection() { 
        // Start a new section
        out.println(inDefinitions ? "\tstruct {" : "\t{");
    }

    private void endSection(boolean last) { 
        // End the current section.  What is the purpose of "dummy"?
        // 
        // PalmMain.c adds two bytes to the end of each resource, and
        // modifies the size to reflect this.  There might be some
        // ambiguity as to whether a specific address is the end of
        // one resource, or the beginning of the next.  So we put a
        // little bit of space between the resources to make sure that
        // there is no ambiguity.
        if (inDefinitions) { 
        out.println("\t} section" + section + ";");
        if (!last) { 
            out.println("\tlong dummy" + section + ";");
        }
        } else { 
        String info = 
            "/* section " + section + " (size " + offset + ") */";
        if (!last) { 
            out.println("\t}, " + info);
            out.println("\t0, /* dummy separator */");
        } else { 
            out.println("\t} " + info);
        }
        }
        totalLength += offset;
        offset = 0;
    }
    
    int getSection() { return section; }
    int getOffset() { return offset; }
    int getTotalLength() { return totalLength; }

    void printRelocationInfo(String structure, String name) { 
        out.println("#define NUMBER_OF_" + name + "_RESOURCES " + section);
        out.println("#define ALL_" + name + "_RESOURCES  \\");
        for (int i = 1; i <= section; i++) { 
        out.println("\t\tSTRUCTURE_ENTRY(" + structure + ".section" 
                + i + ")" + (i == section ? "" : ", \\"));
        }
    }
    }

    // Here so that PALMWriter can override it.
    protected void writeRelocationFile(ClassClass classes[]) { 
    }

}
