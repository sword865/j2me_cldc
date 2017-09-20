/*
 *    EVMConst.java    1.4    99/06/22 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package vm;

public interface EVMConst {

    /* 
     * Class, field, and method access and modifier flags. Some of these
     * values are different from the red book so they'll fit in one byte
     */

    public static final int EVM_CLASS_ACC_PUBLIC        = 0x01; /* visible to everyone */
    public static final int EVM_CLASS_ACC_FINAL         = 0x10; /* no further subclassing */
    public static final int EVM_CLASS_ACC_SUPER         = 0x20; /* funky handling of invokespecial */
    public static final int EVM_CLASS_ACC_INTERFACE     = 0x40; /* class is an interface */
    public static final int EVM_CLASS_ACC_ABSTRACT      = 0x80; /* may not be instantiated */

    public static final int EVM_FIELD_ACC_PUBLIC        = 0x01; /* visible to everyone */
    public static final int EVM_FIELD_ACC_PRIVATE       = 0x02; /* visible only to defining class */
    public static final int EVM_FIELD_ACC_PROTECTED     = 0x04; /* visible to subclasses */
    public static final int EVM_FIELD_ACC_STATIC        = 0x08; /* instance variable is static */
    public static final int EVM_FIELD_ACC_FINAL         = 0x10; /* no subclassing/overriding */
    public static final int EVM_FIELD_ACC_VOLATILE      = 0x40; /* cannot cache in registers */
    public static final int EVM_FIELD_ACC_TRANSIENT     = 0x80; /* not persistant */

    public static final int EVM_METHOD_ACC_PUBLIC       = 0x01; /* visible to everyone */
    public static final int EVM_METHOD_ACC_PRIVATE      = 0x02; /* visible only to defining class */
    public static final int EVM_METHOD_ACC_PROTECTED    = 0x04; /* visible to subclasses */
    public static final int EVM_METHOD_ACC_STATIC       = 0x08; /* method is static */
    public static final int EVM_METHOD_ACC_FINAL        = 0x10; /* no further overriding */
    public static final int EVM_METHOD_ACC_SYNCHRONIZED = 0x20; /* wrap method call in monitor lock */
    public static final int EVM_METHOD_ACC_NATIVE       = 0x40; /* implemented in C */
    public static final int EVM_METHOD_ACC_ABSTRACT     = 0x80; /* no definition provided */

    /* EVMConstantPoolType */
    public static final int EVM_CONSTANT_Utf8        = 1;
    public static final int EVM_CONSTANT_Unicode     = 2;
    public static final int EVM_CONSTANT_Integer    = 3;
    public static final int EVM_CONSTANT_Float        = 4;
    public static final int EVM_CONSTANT_Long        = 5;      
    public static final int EVM_CONSTANT_Double        = 6;
    public static final int EVM_CONSTANT_Class        = 7;
    public static final int EVM_CONSTANT_String        = 8;
    public static final int EVM_CONSTANT_Fieldref    = 9;
    public static final int EVM_CONSTANT_Methodref    = 10;
    public static final int EVM_CONSTANT_InterfaceMethodref= 11;
    public static final int EVM_CONSTANT_NameAndType    = 12;

    public static final int EVM_CONSTANT_POOL_ENTRY_RESOLVED = 0x80;
    public static final int EVM_CONSTANT_POOL_ENTRY_RESOLVING = 0x40;
    public static final int EVM_CONSTANT_POOL_ENTRY_TYPEMASK = 0x3F;

    /*
     * Method invoker indices. Stored in EVMMethodArray.invokerIdx field.
     * Used to determine what code will handle a method's invocation.
     *
     * WARNING: The exact order and values of these macros may be important.
     * [ see javavm/include/classes.h ]
     */
    public static final int EVM_INVOKE_JAVA_METHOD    = 0;
    public static final int EVM_INVOKE_JAVA_SYNC_METHOD    = 1;
    public static final int EVM_INVOKE_EVM_METHOD    = 2;
    public static final int EVM_INVOKE_JNI_METHOD    = 3;
    public static final int EVM_INVOKE_JNI_SYNC_METHOD    = 4;
    public static final int EVM_INVOKE_ABSTRACT_METHOD    = 5;

    /*
     * Implementation details we must know to compute
     * field offsets.
     * Note that header size is the minimum. Alternative GC's
     * might require more, so will require a change here!!!!!
     */
    public static final int ObjHeaderWords        = 2;
    public static final int BytesPerWord        = 4;

}
