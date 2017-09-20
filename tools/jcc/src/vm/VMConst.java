/*
 *    VMConst.java    1.3%    03/01/14 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package vm;

/*
 * Constants used by the Java linker which are not exported in any
 * direct way from the Java runtime or compiler.
 * These are constants of the VM implementation.
 * Some day, I'll figure out how to do separate JVM and EVM versions.
 * For the time being, this is the EVM Version.
 * This does not include class-file-related constants, which are in
 * ClassFileConst, or the opcode values or names, which are in the
 * derived file OpcodeConst.
 */

public interface VMConst
{

    public static int sysLockFlag =        1;
    public static int resolvedFlag =       2;
    public static int errorFlag =          4;
    public static int softRefFlag =      0x8;
    public static int initializedFlag =  0x10;
    public static int linkedFlag =       0x20;
    public static int verifiedFlag =     0x40;
    public static int primitiveFlag =    0x100;
    public static int referencedFlag =   0x200;
    public static int stickyFlag =       0x400;
    public static int scannableFlag =    0x1000;
    public static int hasFinalizerFlag = 0x2000;

    public static int classFlags = 
        sysLockFlag | initializedFlag | linkedFlag | resolvedFlag | stickyFlag;

    // For now, perform no inlining of methods
    public static final int noinlineFlag = 0x1 << 24;

    public static final int OBJ_FLAG_CLASS_ARRAY =  1;
    public static final int OBJ_FLAG_BYTE_ARRAY  =  3;
    public static final int OBJ_FLAG_SHORT_ARRAY =  5;
    public static final int OBJ_FLAG_INT_ARRAY   =  7;
    public static final int OBJ_FLAG_LONG_ARRAY  =  9;
    public static final int OBJ_FLAG_FLOAT_ARRAY =  11;
    public static final int OBJ_FLAG_DOUBLE_ARRAY = 13;
    public static final int OBJ_FLAG_BOOLEAN_ARRAY = 15;
    public static final int OBJ_FLAG_CHAR_ARRAY    = 17;

    /**
     * From src/share/java/include/tree.h
     */
    public static final int ACC_DOUBLEWORD = 0x4000;
    public static final int ACC_REFERENCE  = 0x8000;

    public static final byte ROM_Initializing         = 0x01;
    public static final byte ROM_Initialized          = 0x02;

    public static final byte INVOKE_JAVA_METHOD               = 0;
    public static final byte INVOKE_SYNCHRONIZED_JAVA_METHOD       = 1;
    public static final byte INVOKE_NATIVE_METHOD           = 2;
    public static final byte INVOKE_SYNCHRONIZED_NATIVE_METHOD       = 3;
    public static final byte INVOKE_JNI_NATIVE_METHOD           = 4;
    public static final byte INVOKE_JNI_SYNCHRONIZED_NATIVE_METHOD = 5;
    public static final byte INVOKE_LAZY_NATIVE_METHOD           = 6;
    public static final byte INVOKE_ABSTRACT_METHOD           = 7;
    public static final byte INVOKE_COMPILED_METHOD           = 8;
    public static final byte INVOKE_UNSAFE               = 9;

}
