/*
 *    CodeHacker.java    1.17    01/04/27 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package vm;
import components.*;
import java.io.PrintStream;
import vm.Const;
import vm.EVMConst;
import util.DataFormatException;
import util.SignatureIterator;

/*
 * This class "quickens" Java bytecodes.
 * That is, it looks at the instructions having symbolic references,
 * such as get/putfield and methodcalls, attempts to resolve the references
 * and re-writes the instructions in their JVM-specific, quick form.
 *
 * Resolution failures are not (any longer) considered fatal errors, they
 * just result in code that cannot be considered as read-only, as more
 * resolution will be required at runtime, as we refuse to re-write any
 * instructions having unresolved references.
 *
 * The following is an exception to the above rule (and an unfortunate hack):
 * References to array classes (having names starting in "[") are 
 * quickened anyway, as we have confidence that these classes, though not
 * instantiated when this code is run, will be instantiated in time.
 * (Perhaps this should be under control of a flag?)
 * 
 */
public
class CodeHacker {

    ConstantPool    pool;
    boolean        warn;
    boolean        verbose;
    PrintStream        log;
    boolean        useLosslessOpcodes = false;
    ClassInfo        java_lang_Object;

    private boolean    success;
    private byte        newopcode;

    public CodeHacker( ConstantPool p, boolean lossless, boolean w, boolean v ){
    pool = p;
    warn = w;
    verbose = v;
    log = System.out;
    java_lang_Object = ClassInfo.lookupClass("java/lang/Object");
    if ( java_lang_Object == null ){
        log.println("CodeHacker: could not find java/lang/Object");
    }
    useLosslessOpcodes = lossless;
    }

    private void lookupFailed( ClassConstant me, FMIrefConstant target ){
    log.println("Quickening "+me.name.string+": lookup failed for "+target);
    success = false;
    }

    private static int getInt( byte code[], int w ){
    return    (  (int)code[w]   << 24 ) |
        (( (int)code[w+1] &0xff ) << 16 ) |
        (( (int)code[w+2] &0xff ) << 8 ) |
         ( (int)code[w+3] &0xff );
    }

    private static int getUnsignedShort( byte code[], int w ){
    return    (( (int)code[w] &0xff ) << 8 ) | ( (int)code[w+1] &0xff );
    }

    private static void putShort( byte code[], int w, short v ){
    code[w]   = (byte)(v>>>8);
    code[w+1] = (byte)v;
    }

    private FMIrefConstant dup( FMIrefConstant c ){
    return (FMIrefConstant) pool.dup( c );
    }

    /*
     * Returns true if the reference was fully quickened.
     * Returns false if the reference could not be deleted, either because
     *   the lookup failed, or because we had to use the wide variant.
     * Sets this.success to false if the lookup failed or there was no
     * usable variant.
     */
    private boolean quickenFieldAccess(
    ClassConstant me,
    byte code[],
    int loc,
    boolean isStatic,
    ConstantObject c[],
    int oneWord, int twoWords, int refReference)
    {
    FieldConstant fc = (FieldConstant)c[ getUnsignedShort( code, loc+1 ) ];
    FieldInfo  fi = fc.find();
    if ( fi == null ){
        // never even try to quicken anything we cannot resolve.
        lookupFailed( me, fc );
        return false;
    }
    byte newCode;
    switch (fc.sig.type.string.charAt(0) ){
    case '[':
    case 'L':
        newCode = (byte)refReference;
        break;
    case 'D':
    case 'J':
        newCode = (byte)twoWords;
        break;
    default:
        newCode = (byte)oneWord;
        break;
    }
    if ( isStatic ) {
        code[loc] = newCode;
        return false; // still references constant pool!
    } else {
        int fieldOffset = fi.instanceOffset;
        code[loc+1] =  (byte)((fieldOffset >> 8) & 0xFF);
        code[loc+2] =  (byte) (fieldOffset & 0xFF);
        code[loc] = newCode;
        return true;
    }
    }

    /*
     * Should  a call to the specified methodblock be turned into
     * an invokesuper_fast instead of a invokenonvirtual_fast?
     *
     * The fourTHREE conditions that have to be satisfied:
     *    The method isn't private
     *    The method isn't an <init> method
     *    XXXXThe ACC_SUPER flag is set in the current class
     *    The method's class is a superclass (and not equal) to 
     *         the current class.
     */
    private boolean
    isSpecialSuperCall( MethodInfo me, MethodInfo callee ){
    String name = callee.name.string;
    if ( (callee.access&Const.ACC_PRIVATE) != 0 ) return false;
    if ( name.equals("<init>") ) return false;
    ClassInfo myclass = me.parent;
    ClassInfo hisclass = callee.parent;
    if ( myclass == hisclass ) return false;
    // walk up my chain, looking for other's class
    while ( myclass != null ){
        myclass = myclass.superClassInfo;
        if ( myclass == hisclass ) return true;
    }
    return false;
    }

    private boolean quickenCode( MethodInfo m, ConstantObject c[] ) throws DataFormatException {
    byte code[] = m.code;
    int list[] = m.ldcInstructions;
    ConstantObject        co;
    FieldConstant        fc;
    MethodConstant        mc;
    NameAndTypeConstant    nt;
    ClassConstant        cc;
    String            t;
    ClassConstant        me = m.parent.thisClass;
    MethodInfo        mi;
    ClassInfo        ci;

    success = true;
    if (verbose){
        log.println("Q>>METHOD "+m.name.string );
    }
    if ( list != null ){
        for ( int i = 0; i < list.length; i++ ){
        int loc = list[i];
        if ( loc < 0 ) continue;
        switch( (int)code[loc]&0xff ){
        case Const.opc_ldc:
            //
            // no danger of lookup failure here,
            // so don't even examine the referenced object.
            //
            co = c[(int)code[loc+1]&0xff];
            break;
        default:
            throw new DataFormatException( "unexpected opcode in ldc="+
            ((int)code[loc]&0xff)+" at loc="+loc+
            " in "+m.qualifiedName() );
        }
        }
    }
    list = m.wideConstantRefInstructions;
    if ( list != null ){
        MethodInfo[] tList = null;
        int tli = 0;        // index into tList
        if (VMMethodInfo.SAVE_TARGET_METHODS) {
        tList = new MethodInfo[list.length];
        }
        for ( int i = 0; i < list.length; i++ ){
        int loc = list[i];
        if ( loc < 0 ) continue;
        co = c[ getUnsignedShort(code, loc+1) ];
        if ( ! co.isResolved() ){
            //
            // don't try to quicken unresolved references.
            // this is not fatal!
            //
            // Do quicken if its a reference to an array!!
            // What a hack!
            if ( (co instanceof ClassConstant ) &&
             ((ClassConstant)co).name.string.charAt(0) == Const.SIGC_ARRAY ){
            ((ClassConstant)co).forget(); // never mind, we'll fix later...
            } else {
            if ( warn ){
                log.println("Warning: could not quicken reference from " + m.qualifiedName() + " to "+co );
            }
            continue;
            }
        }
        switch( (int)code[loc]&0xff ){
        case Const.opc_ldc_w:
            co.incldcReference();
            break;
        case Const.opc_ldc2_w:
            break;
        case Const.opc_getstatic:
            quickenFieldAccess( me, code, loc, true, c,
            Const.opc_getstatic_fast,
            Const.opc_getstatic2_fast, Const.opc_getstaticp_fast);
            break;
        case Const.opc_putstatic:
            quickenFieldAccess( me, code, loc, true, c,
            Const.opc_putstatic_fast,
            Const.opc_putstatic2_fast, 
            Const.opc_putstatic_fast);
            break;
        case Const.opc_getfield:
            if (quickenFieldAccess( me, code, loc, false, c,
              Const.opc_getfield_fast,
              Const.opc_getfield2_fast, 
              Const.opc_getfieldp_fast )) {
            // doesn't reference constant pool any more
            list[i] = -1;
            }
            break;
        case Const.opc_putfield:
            if (quickenFieldAccess( me, code, loc, false, c,
              Const.opc_putfield_fast,
              Const.opc_putfield2_fast,
              Const.opc_putfield_fast)) {
            // doesn't reference constant pool any more
            list[i] = -1;
            }
            break;

        case Const.opc_invokevirtual:
                    mc = (MethodConstant)c[ getUnsignedShort( code, loc+1 ) ];
                    mi = mc.find();
                    if (mi != null){
                        if (mi.isPrivateMember() 
                            || ((mi.access & Const.ACC_FINAL) != 0)
                            || ((mi.parent.access & Const.ACC_FINAL) != 0)) { 
                            code[loc] = (byte)Const.opc_invokespecial_fast;
                        }
                    }
                    break;

        case Const.opc_invokeinterface:
            break;

        case Const.opc_invokestatic:
            code[loc] = (byte)Const.opc_invokestatic_fast;
            break;

        case Const.opc_new:
            code[loc] = (byte)Const.opc_new_fast;
            break;

        case Const.opc_anewarray:
            // code[loc] = (byte)Const.opc_anewarray_fast;
            break;

        case Const.opc_checkcast:
            code[loc] = (byte)Const.opc_checkcast_fast;
            break;

        case Const.opc_instanceof:
            code[loc] = (byte)Const.opc_instanceof_fast;
            break;

        case Const.opc_multianewarray:
            code[loc] = (byte)Const.opc_multianewarray_fast;
            break;

        case Const.opc_invokespecial:
            code[loc] = (byte)Const.opc_invokespecial_fast;
            break;

        default: 
            throw new DataFormatException( "unexpected opcode in wideConstantRef="+
            ((int)code[loc]&0xff)+" at loc="+loc+
            " in "+m.qualifiedName() );
        }
        }
        // Alloc and copy to new targetMethods array
        if (VMMethodInfo.SAVE_TARGET_METHODS) {
        m.targetMethods = new MethodInfo[tli];
        System.arraycopy(tList, 0, m.targetMethods, 0, tli);
        }
    }
    return success;
    }

    public boolean
    quickenCode( ClassInfo c ){
    ConstantObject constants[] = c.constants;
    MethodInfo     method[]= c.methods;
    int n = method.length;
    int i = 0;
    boolean result = true;
    try {
        for ( i = 0; i < n; i++ ){
        if ( ! quickenCode( method[i], constants ) )
            result = false;
        }
    } catch( DataFormatException e ){
        System.err.println("Quickening "+method[i].qualifiedName()+" got exception:");
        e.printStackTrace();
        return false;
    }
    return result;
    }

}
