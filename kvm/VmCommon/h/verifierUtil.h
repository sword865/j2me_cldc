/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Class file verifier
 * FILE:      verifierUtil.h
 * OVERVIEW:  Implementation-specific parts of the KVM verifier.
 * AUTHOR:    Nik Shaylor, 2000
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>

/* ------------------------------------------------------------------------ *\
 *                               General types                              *
\* ------------------------------------------------------------------------ */

/*
 * In order to split the verifier into a KVM part and a VM
 * independent part the verifier uses a variable typing 
 * system that is slightly different from the KVM.
 *
 * The following types are use and have the same meaning as they do in KVM
 *
 *      CLASS           A reference to a class data structure
 *      METHOD          A reference to a method data structure
 *      CONSTANTPOOL    A reference to a constant pool data structure
 *      TRUE            A boolean value
 *      FALSE           A boolean value
 */

/*
 * The following are type definitions that are only used by the
 * verifier core.
 *
 * In KVM a VERIFIERTYPE and a CLASSKEY are fairly similar.
 * The difference lies in the encoding of primitive data types.
 * For example, a CLASSKEY for an Integer is an ASCII 'I', while
 * a VERIFIERTYPE for an Integer is ITEM_Integer. In both
 * cases values less than 256 are primitive data types and
 * those above 256 are reference types and array types.
 * In KVM the encoding above 256 are the same for CLASSKEY
 * and VERIFIERTYPE. The are two conversion functions
 * Vfy_toVerifierType() and Vfy_toClassKey() that will
 * convert between these types. Currently verifierCore
 * never needs to convert primitive data types this way,
 * so these functions simply pass the input to the output
 *
 * Another small complexity exists with VERIFIERTYPE.
 * This is that the value pushed by the NEW bytecode
 * is an encoding of the IP address of the NEW instruction
 * that will create the object when executed (this encoding
 * is the same as that done in the stack maps). This is done in
 * order to distinguish initialized from uninitialized
 * references of the same type. For instance:
 *
 *      new Foo
 *      new Foo
 *      invokespecial <init>
 *
 * This results in the stack containing one initialized Foo
 * and one uninitialized Foo. The verifier needs to keep track
 * of these so after the invokespecial <init> the verifier
 * changes all occurences of the second Foo from being type
 * "New from bytecode-n" to being type "Foo".
 */

/*
 * This represents a class type in the VM. It could be the same
 * as a CLASS, but in KVM it is a 16 bit data structure that
 * contains an encoded key that represents a CLASS. Its format is
 * described in the function change_FieldSignature_to_Key().
 */
#define CLASSKEY unsigned short

/*
 * This represents a class type in the verifier. Again it could
 * the same as a CLASS and/or CLASSKEY, but in KVM it is another
 * 16 bit data structure that matches the format the loader produces
 * for the stack map tables.
 */
#define VERIFIERTYPE unsigned short

/*
 * This represents the name of a method. It could be the same as METHOD,
 * but in KVM it is 16 value that is used get the name from a table of
 * interned UTF8 strings.
 */
#define METHODNAMEKEY unsigned short

/*
 * This represents the type of a method. It could be the same as METHOD,
 * but in KVM it is 16 value that is used get the name from a table of
 * interned strings that have the method signature encoded as described
 * in the function change_MethodSignature_to_Key()
 */
#define METHODTYPEKEY unsigned short

/*
 * Index in to a bytecode array
 */
#define IPINDEX unsigned short

/*
 * Index into the local variable array maintained by the verifier
 */
#define SLOTINDEX int

/*
 * Index into the constant pool
 */
#define POOLINDEX int

/*
 * Tag from the constant pool
 */
#define POOLTAG unsigned char

/* ------------------------------------------------------------------------ *\
 *                           Manifest Class types                           *
\* ------------------------------------------------------------------------ */

/*
 * All these functions produce a VERIFIERTYPE corresponding to an array type.
 */

#define Vfy_getBooleanArrayVerifierType()   ((1 << FIELD_KEY_ARRAY_SHIFT) + 'Z')
#define Vfy_getByteArrayVerifierType()      ((1 << FIELD_KEY_ARRAY_SHIFT) + 'B')
#define Vfy_getCharArrayVerifierType()      ((1 << FIELD_KEY_ARRAY_SHIFT) + 'C')
#define Vfy_getShortArrayVerifierType()     ((1 << FIELD_KEY_ARRAY_SHIFT) + 'S')
#define Vfy_getIntArrayVerifierType()       ((1 << FIELD_KEY_ARRAY_SHIFT) + 'I')
#define Vfy_getLongArrayVerifierType()      ((1 << FIELD_KEY_ARRAY_SHIFT) + 'J')

#if IMPLEMENTS_FLOAT
#define Vfy_getFloatArrayVerifierType()     ((1 << FIELD_KEY_ARRAY_SHIFT) + 'F')
#define Vfy_getDoubleArrayVerifierType()    ((1 << FIELD_KEY_ARRAY_SHIFT) + 'D')
#endif

#define Vfy_getObjectVerifierType()         (JavaLangObject->clazz.key)
#define Vfy_getStringVerifierType()         (JavaLangString->clazz.key)
#define Vfy_getThrowableVerifierType()      (JavaLangThrowable->clazz.key)
#define Vfy_getObjectArrayVerifierType()    ((VERIFIERTYPE)(Vfy_getObjectVerifierType() + (1 << FIELD_KEY_ARRAY_SHIFT)))

/* ------------------------------------------------------------------------ *\
 *                           Dynamic Class types                            *
\* ------------------------------------------------------------------------ */

/*
 * Get a class array from a CLASSKEY data type that some kind of object
 * reference. It is used for ANEWARRAY
 */
extern VERIFIERTYPE Vfy_getClassArrayVerifierType(CLASSKEY typeKey);

/*
 * Gets the VERIFIERTYPE that represents the element type of an array
 */
extern VERIFIERTYPE Vfy_getReferenceArrayElementType(VERIFIERTYPE arrayTypeKey);

/*
 * Gets the VERIFIERTYPE that represents the element type of an array
 */
extern bool_t   Vfy_isArrayClassKey(CLASSKEY typeKey, int dim);

/* ------------------------------------------------------------------------ *\
 *                          "NEW" object data types                         *
\* ------------------------------------------------------------------------ */

/*
 * This will encode an IP address into a special kind of VERIFIERTYPE such
 * that it will not be confused with other types.
 */
#define Vfy_createVerifierTypeForNewAt(ip)      Vfy_toVerifierType((CLASSKEY)(ENCODE_NEWOBJECT(ip)))

/*
 * Tests a VERIFIERTYPE to see if it was created using
 * Vfy_createVerifierTypeForNewAt()
 */
#define Vfy_isVerifierTypeForNew(verifierType)  (verifierType & ITEM_NewObject_Flag)

/*
 * Translates a VERIFIERTYPE encoded using Vfy_createVerifierTypeForNewAt()
 * back into the IP address that was used to create it.
 */
#define Vfy_retrieveIpForNew(verifierType)      DECODE_NEWOBJECT(verifierType)

/* ------------------------------------------------------------------------ *\
 *                               Error codes                                *
\* ------------------------------------------------------------------------ */

#define VE_STACK_OVERFLOW               1
#define VE_STACK_UNDERFLOW              2
#define VE_STACK_EXPECT_CAT1            3
#define VE_STACK_BAD_TYPE               4
#define VE_LOCALS_OVERFLOW              5
#define VE_LOCALS_BAD_TYPE              6
#define VE_LOCALS_UNDERFLOW             7
#define VE_TARGET_BAD_TYPE              8
#define VE_BACK_BRANCH_UNINIT           9
#define VE_SEQ_BAD_TYPE                10
#define VE_EXPECT_CLASS                11
#define VE_EXPECT_THROWABLE            12
#define VE_BAD_LOOKUPSWITCH            13
#define VE_BAD_LDC                     14
#define VE_BALOAD_BAD_TYPE             15
#define VE_AALOAD_BAD_TYPE             16
#define VE_BASTORE_BAD_TYPE            17
#define VE_AASTORE_BAD_TYPE            18
#define VE_FIELD_BAD_TYPE              19
#define VE_EXPECT_METHODREF            20
#define VE_ARGS_NOT_ENOUGH             21
#define VE_ARGS_BAD_TYPE               22
#define VE_EXPECT_INVOKESPECIAL        23
#define VE_EXPECT_NEW                  24
#define VE_EXPECT_UNINIT               25
#define VE_BAD_INSTR                   26
#define VE_EXPECT_ARRAY                27
#define VE_MULTIANEWARRAY              28
#define VE_EXPECT_NO_RETVAL            29
#define VE_RETVAL_BAD_TYPE             30
#define VE_EXPECT_RETVAL               31
#define VE_RETURN_UNINIT_THIS          32
#define VE_BAD_STACKMAP                33
#define VE_FALL_THROUGH                34
#define VE_EXPECT_ZERO                 35
#define VE_NARGS_MISMATCH              36
#define VE_INVOKESPECIAL               37
#define VE_BAD_INIT_CALL               38
#define VE_EXPECT_FIELDREF             39
#define VE_FINAL_METHOD_OVERRIDE       40
#define VE_MIDDLE_OF_BYTE_CODE         41
#define VE_BAD_NEW_OFFSET              42
#define VE_BAD_EXCEPTION_HANDLER_RANGE 43
#define VE_EXPECTING_OBJ_OR_ARR_ON_STK 44

/*
 * If you add new errors to this list, be sure to also add
 * an explanation to KVM_MSG_VERIFIER_STATUS_INFO_INITIALIZER in
 * messages.h.
 */

/* ------------------------------------------------------------------------ *\
 *                              Stack map flags                             *
\* ------------------------------------------------------------------------ */

/*
 * Flags used to control how to match two stack maps.
 * One of the stack maps is derived as part of the type checking process.
 * The other stack map is recorded in the class file.
 */
#define SM_CHECK 1           /* Check if derived types match recorded types. */
#define SM_MERGE 2           /* Merge recorded types into derived types. */
#define SM_EXIST 4           /* A recorded type attribute must exist. */

/* ------------------------------------------------------------------------ *\
 *                         Global data definitions                          *
\* ------------------------------------------------------------------------ */

/*
 * This is always the method being verified.
 */
extern METHOD methodBeingVerified;

/*
 * Pointer to the bytecodes for the above method
 */
extern unsigned char *bytecodesBeingVerified;

/*
 * ByteCode table for debugging purposes (included
 * only if certain tracing modes are enabled).
 *
 */
extern const char* const byteCodeNames[];

/* ------------------------------------------------------------------------ *\
 *                             General functions                            *
\* ------------------------------------------------------------------------ */

/*
 * Print fatal error message and exit
 */
#define Vfy_fatal(message)  fatalError(message)

/*
 * Get an signed byte from the bytecode array
 */
#define Vfy_getByte(ip)     (((signed char *)bytecodesBeingVerified)[ip])

/*
 * Get an unsigned byte from the bytecode array
 */
#define Vfy_getUByte(ip)    (bytecodesBeingVerified[ip])

/*
 * Get an signed short from the bytecode array
 */
#define Vfy_getShort(ip)    getShort(bytecodesBeingVerified+(ip))

/*
 * Get an unsigned short from the bytecode array
 */
#define Vfy_getUShort(ip)   getUShort(bytecodesBeingVerified+(ip))

/*
 * Get an signed word from the bytecode array
 */
#define Vfy_getCell(ip)     getCell(bytecodesBeingVerified+(ip))

/*
 * Get an opcode from the bytecode array. (
 */
extern int Vfy_getOpcode(IPINDEX ip);

/* ------------------------------------------------------------------------ *\
 *                              Type checking                               *
\* ------------------------------------------------------------------------ */

#if INCLUDEDEBUG

/*
 * Convert a VERIFIERTYPE to a CLASSKEY
 *
 * In KVM the types are the same when the value is above 256 and the
 * verifier core never needs to convert values lower then this so it
 * is simply enough to check that this assumption is true.
 *
 * @param classKey  A CLASSKEY
 * @return          A VERIFIERTYPE
 */
#define Vfy_toVerifierType(classKey) \
    ((classKey > 255) ? classKey : (fatalError("Bad class key"), 0))

/*
 * Convert a CLASSKEY to a VERIFIERTYPE
 *
 * In KVM the types are the same when the value is above 256 and the
 * verifier core never needs to convert values lower then this so it
 * is simply enough to check that this assumption is true.
 *
 * @param verifierType  A VERIFIERTYPE
 * @return              A CLASSKEY
 */
#define Vfy_toClassKey(verifierType) \
    ((verifierType > 255) ? verifierType : (fatalError("Bad verifier type"), 0))

#else
#define Vfy_toVerifierType(classKey) classKey
#define Vfy_toClassKey(verifierType) verifierType
#endif

/*
 * Test to see if a type is a primitive type
 *
 * @param  type     A VERIFIERTYPE
 * @return bool_t   TRUE is type is a primitive
 */
#define Vfy_isPrimitive(type)  ((type != ITEM_Null) && (type < 256))

/*
 * Test to see if a type is an array
 *
 * @param  type     A VERIFIERTYPE
 * @return bool_t   TRUE is type is an array
 */
#define Vfy_isArray(type)  ((type >> FIELD_KEY_ARRAY_SHIFT) != 0)

/*
 * Test to see if a type is an array or null
 *
 * @param  type     A VERIFIERTYPE
 * @return bool_t   TRUE is type is an array or null
 */
#define Vfy_isArrayOrNull(type)  (Vfy_isArray(type) || type == ITEM_Null)

/*
 * Test to see if one type can be assigned to another type
 *
 * @param fromKey   A VERIFIERTYPE to assign from
 * @param toKey     A VERIFIERTYPE to assign to
 * @return bool_t   TRUE if the assignment is legal
 */
#define Vfy_isAssignable(fromKey, toKey)    vIsAssignable(fromKey, toKey, NULL)

/*
 * Test to see if a field of a class is "protected"
 *
 * @param clazz     A CLASS
 * @param index     The field index (offset in words)
 * @return          TRUE if field is protected
 */
#define Vfy_isProtectedField(clazz, index) vIsProtectedAccess((INSTANCE_CLASS)clazz, index, FALSE)

/*
 * Test to see if a method of a class is "protected"
 *
 * @param clazz     A CLASS
 * @param index     The method index (offset in words)
 * @return          TRUE if method is protected
 */
#define Vfy_isProtectedMethod(clazz, index) vIsProtectedAccess((INSTANCE_CLASS)clazz, index, TRUE)

/*
 * Check if the current state of the local variable and stack contents match
 * that of a stack map at a specific IP address
 *
 * @param targetIP  The IP address for the stack map
 * @param flags     One or more of SM_CHECK, SM_MERGE, and SM_EXIST
 * @param throwCode A verification error code
 * @throw           If the match is not correct
 */
#define Vfy_matchStackMap(targetIP, flags, throwCode) {                                                 \
    if (!matchStackMap(methodBeingVerified,  targetIP, flags)) {                                        \
        Vfy_throw(throwCode);                                                                           \
    }                                                                                                   \
}

/*
 * Check stack maps can be merged at the current location
 *
 * @param targetIP      The IP address for the stack map
 * @param noControlFlow TRUE if the location cannot be reached from the previous instruction
 * @throw               If the match is not correct
 */
#define Vfy_checkCurrentTarget(current_ip, noControlFlow) {                                             \
        Vfy_matchStackMap(                                                                              \
                           current_ip,                                       /* target ip */            \
                           SM_MERGE | (noControlFlow ? SM_EXIST : SM_CHECK), /* flags */                \
                           VE_SEQ_BAD_TYPE                          /* Vfy_throw code if not matched */ \
                         );                                                                             \
}

/*
 * Check stack maps match at an exception handler entry point
 *
 * @param target_ip     The IP address of the exception handler
 * @throw               If the match is not correct
 */
#define Vfy_checkHandlerTarget(target_ip) {                                                             \
    Vfy_trace1("Check exception handler at %ld:\n", (long)target_ip);                                   \
    Vfy_matchStackMap(target_ip, SM_CHECK | SM_EXIST, VE_TARGET_BAD_TYPE);                              \
}

/*
 * Check stack maps match at a jump target
 *
 * @param current_ip    The current IP address
 * @param target_ip     The IP address of the jump target
 * @throw               If the match is not correct
 */
#define Vfy_checkJumpTarget(this_ip, target_ip) {                                                       \
    Vfy_trace1("Check jump target at %ld\n", target_ip);                                                \
    Vfy_matchStackMap(target_ip, SM_CHECK | SM_EXIST, VE_TARGET_BAD_TYPE);                              \
    if (!checkNewObject(this_ip, target_ip)) {                                                          \
        Vfy_throw(VE_BACK_BRANCH_UNINIT);                                                               \
    }                                                                                                   \
}

/*
 * Check stack maps match at a jump target
 *
 * @param current_ip    The current IP address
 * @param target_ip     The IP address of the jump target
 * @throw               If the match is not correct
 */
#define Vfy_markNewInstruction(ip, length) {                                                            \
    if (NEWInstructions == NULL) {                                                                      \
        NEWInstructions = callocNewBitmap(length);                                                      \
    }                                                                                                   \
    SET_NTH_BIT(NEWInstructions, ip);                                                                   \
}

/*
 * Get the type of a local variable
 *
 * @param i             The slot index of the local variable
 * @param t             The VERIFIERTYPE that must match
 * @return              The actual VERIFIERTYPE found
 * @throw               If the match is not correct or index is bad
 */
extern VERIFIERTYPE Vfy_getLocal(SLOTINDEX  i, VERIFIERTYPE t);

/*
 * Set the type of a local variable
 *
 * @param i             The slot index of the local variable
 * @param t             The new VERIFIERTYPE of the slot
 * @throw               If index is bad
 */
extern void Vfy_setLocal(SLOTINDEX  i, VERIFIERTYPE t);

/*
 * Push a type
 *
 * @param t             The new VERIFIERTYPE to be pushed
 * @throw               If stack overflows
 */
extern void Vfy_push(VERIFIERTYPE t);

/*
 * Pop a type
 *
 * @param t             The VERIFIERTYPE that must match
 * @throw               If the match is not correct or stack underflow
 */
extern VERIFIERTYPE Vfy_pop(VERIFIERTYPE t);

/*
 * Pop category1 type (ITEM_Null, ITEM_Integer, ITEM_Float, <init> object,
 *                      reference type, or array type
 *
 * @throw               If the match is not correct or stack underflow
 */
extern VERIFIERTYPE Vfy_popCategory1();

/*
 * Pop the first word of a category2 type
 *
 * @throw               If the match is not correct or stack underflow
 */
extern VERIFIERTYPE Vfy_popCategory2_firstWord();

/*
 * Pop the second word of a category2 type
 *
 * @throw               If the match is not correct or stack underflow
 */
extern VERIFIERTYPE Vfy_popCategory2_secondWord();

/*
 * Pop the first word of a DoubleWord type
 *
 * @throw               If the match is not correct or stack underflow
 */
#define Vfy_popDoubleWord_firstWord()  Vfy_popCategory2_firstWord()

/*
 * Pop the second word of a DoubleWord type
 *
 * @throw               If the match is not correct or stack underflow
 */
#define Vfy_popDoubleWord_secondWord() Vfy_popCategory2_secondWord()

/*
 * Verify a void return
 *
 * @throw               If the method is not if type void, if the method
 *                      is <init> and super() was not called.
 */
extern void Vfy_returnVoid();

/*
 * Pop a type and verify a return
 *
 * @param returnType    The VERIFIERTYPE to be returned
 * @throw               If the return type is incorrect
 */
extern void Vfy_popReturn(VERIFIERTYPE returnType);

/*
 * Push a CLASSKEY
 *
 * @param returnType    The CLASSKEY to be pushed
 * @throw               If stack overflow, bad type.
 */
extern void Vfy_pushClassKey(CLASSKEY fieldType);

/*
 * Pop a CLASSKEY
 *
 * @param returnType    The CLASSKEY to be popped
 * @throw               If stack underflow, bad type.
 */
extern void Vfy_popClassKey(CLASSKEY fieldType);

/*
 * Setup the callee context (to process an invoke)
 *
 * @param methodTypeKey The METHODTYPEKEY the method type
 */
extern void Vfy_setupCalleeContext(METHODTYPEKEY methodTypeKey);

/*
 * Pop the callee arguments from the stack
 *
 * @throw               If argument are not correct
 */
extern int Vfy_popInvokeArguments();

/*
 * Push the callee result
 *
 * @throw               If result is not correct
 */
extern void Vfy_pushInvokeResult();

/*
 * Test to see if a method name start "<"
 *
 * @param methodNameKey The METHODNAMEKEY of the method
 * @return              TRUE if the method name starts with '<'
 */
extern bool_t Vfy_MethodNameStartsWithLeftAngleBracket(METHODNAMEKEY methodNameKey);

/*
 * Test to see if a method name is "<init>"
 *
 * @param methodNameKey The METHODNAMEKEY of the method
 * @return              TRUE if the method name is "<init>"
 */
extern bool_t Vfy_MethodNameIsInit(METHODNAMEKEY methodNameKey);

/*
 * Change all local and stack instances of one type to another
 *
 * @param fromType      The VERIFIERTYPE to convert from
 * @param fromType      The VERIFIERTYPE to convert to
 */
extern void Vfy_ReplaceTypeWithType(VERIFIERTYPE fromType, VERIFIERTYPE toType);

/* ------------------------------------------------------------------------ *\
 *                             Stack state management                       *
\* ------------------------------------------------------------------------ */

/*
 * Save the state of the type stack
 */
extern void Vfy_saveStackState();

/*
 * Restore the state of the type stack
 */
extern void Vfy_restoreStackState();

/* ------------------------------------------------------------------------ *\
 *                             Utility Functions                            *
\* ------------------------------------------------------------------------ */

/*
 * Initialize the local variable types
 */
extern void Vfy_initializeLocals();

/*
 * Abort the verification
 *
 * @param code          One of the VE_xxx codes
 */
extern void Vfy_throw(int code);

#if INCLUDEDEBUGCODE

/*
 * Write a trace line with one parameter
 *
 * @param msg           printf format string
 * @param p1            32 bit parameter
 */
extern void Vfy_trace1(char *msg, long p1);

/*
 * Save the ip somewhere the error printing routine can find it
 *
 * @param ip            The IP address
 */
extern void Vfy_setErrorIp(int ip);

/*
 * Output a trace statement at the head of the dispatch loop
 *
 * @param vMethod       The METHOD being verified
 * @param ip            The IP address
 */
extern void Vfy_printVerifyLoopMessage(METHOD vMethod, IPINDEX ip);

#else

#define Vfy_trace1(x, y)                    /**/
#define Vfy_setErrorIp(x)                   /**/
#define Vfy_printVerifyLoopMessage(x, y)    /**/

#endif /* INCLUDEDEBUGCODE */

/* ------------------------------------------------------------------------ *\
 *                           Class Accessor Methods                         *
\* ------------------------------------------------------------------------ */

/*
 * Return TRUE is the class is java.lang.Object
 *
 * @param vClass        The CLASS to be accessed
 * @return              TRUE if the the class is java.lang.Object
 */
#define Cls_isJavaLangObject(vClass)    ((INSTANCE_CLASS)vClass == JavaLangObject)

/*
 * Return the superclass of the class
 *
 * @param vClass        The CLASS to be accessed
 * @return              The CLASS of its superclass
 */
#define Cls_getSuper(vClass)            ((CLASS)(((INSTANCE_CLASS)vClass)->superClass))

/*
 * Return the constant pool for the class
 *
 * @param vClass        The CLASS to be accessed
 * @return              Its CONSTANTPOOL
 */
#define Cls_getPool(vClass)             (((INSTANCE_CLASS)vClass)->constPool)

/*
 * Lookup a method
 *
 * @param vClassContext The CLASS of the lookup context (e.g. calling class)
 * @param vClassTarget  The CLASS to be accessed (e.g. called class)
 * @param vMethod       The METHOD who's name should be used to do the lookup
 * @return              The METHOD if it exists else NULL
 */
#define Cls_lookupMethod(vClassContext, vClassTarget, vMethod) \
    lookupMethod(vClassTarget, vMethod->nameTypeKey, (INSTANCE_CLASS)vClassContext)

/*
 * Lookup a field 
 *
 * @param vClass        The CLASS of the lookup context (e.g. calling class)
 * @param vNameTypeKey  The nameTypeKey which is being searched in field table
 * @return              The FIELD if it exists else NULL
 */
#define Cls_lookupField(vClass, vNameTypeKey) \
    lookupField(vClass, vNameTypeKey)

/*
 * Get a class's key
 *
 * @param vClass        The CLASS to be accessed
 * @return              Its CLASSKEY
 */
#define Cls_getKey(vClass)  (vClass->key)

/* ------------------------------------------------------------------------ *\
 *                            Pool Accessor Methods                         *
\* ------------------------------------------------------------------------ */

/*
 * Get the tag of the pool entry at index.
 *
 * @param vPool         The CONSTANTPOOL to be accessed
 * @param index         The index to be accessed
 * @return              Its POOLTAG
 * @throw               If index out of bounds
 */
#define Pol_getTag(vPool, index)                            \
    (index >= CONSTANTPOOL_LENGTH(vPool)) ?                 \
        (Vfy_throw(120), 0)                                 \
        :                                                   \
        (CONSTANTPOOL_TAG(vPool, index) & CP_CACHEMASK)     \

/*
 * Check the tag at index is the same as "tag".
 *
 * @param vPool         The CONSTANTPOOL to be accessed
 * @param index         The index to be accessed
 * @param tag           The POOLTAG that should be there
 * @param errorcode     The error to throw if it is not
 * @throw               If tag is wrong
 */
#define Pol_checkTagIs(vPool, index, tag, errorcode) {      \
    if (Pol_getTag(vPool, index) != tag) {                  \
        Vfy_throw(errorcode);                               \
    }                                                       \
}

/*
 * Check the tag at index is the same as "tag"or "tag2".
 *
 * @param vPool         The CONSTANTPOOL to be accessed
 * @param index         The index to be accessed
 * @param tag           The POOLTAG that should be there
 * @param tag2          The POOLTAG that should be there
 * @param errorcode     The error to throw if it is not
 * @throw               If both tags are wrong
 */
#define Pol_checkTag2Is(vPool, index, tag, tag2, errorcode) {   \
    POOLTAG t = Pol_getTag(vPool, index);                       \
    if (t != tag && t != tag2) {                                \
        Vfy_throw(errorcode);                                   \
    }                                                           \
}

/*
 * Check the tag at index is CONSTANT_Class.
 *
 * @param vPool         The CONSTANTPOOL to be accessed
 * @param index         The index to be accessed
 * @throw               If the pool tag is not CONSTANT_Class
 */
#define Pol_checkTagIsClass(vPool, index)                   \
     Pol_checkTagIs(vPool, index, CONSTANT_Class, VE_EXPECT_CLASS)

/*
 * Get the CLASSKEY for the pool entry at index
 *
 * @param vPool         The CONSTANTPOOL to be accessed
 * @param index         The index to be accessed
 * @return              The CLASS of the CONSTANT_Class at that index
 */
#define Pol_getClassKey(vPool, index)                       \
     Cls_getKey(vPool->entries[index].clazz)

/* For CONSTANT_FieldRef_info, CONSTANT_MethodRef_info, CONSTANT_InterfaceMethodRef_info  */

/*
 * Get the class index for the pool entry at index
 *
 * @param vPool         The CONSTANTPOOL to be accessed
 * @param index         The index to be accessed
 * @return              The POOLINDEX in the class_index field
 */
#define Pol_getClassIndex(vPool, index)                     \
    (vPool->entries[index].method.classIndex)

/*
 * Get the class for the pool entry at index 
 *
 * @param vPool         The CONSTANTPOOL to be accessed
 * @param index         The index to be accessed
 * @return              The POOLINDEX in the name_and_type_index field
 */
#define Pol_getClass(vPool, fieldClassIndex)               \
    (vPool->entries[fieldClassIndex].clazz)

/*
 * Get the Name-and-type index for the pool entry at index
 *
 * @param vPool         The CONSTANTPOOL to be accessed
 * @param index         The index to be accessed
 * @return              The POOLINDEX in the name_and_type_index field
 */
#define Pol_getNameAndTypeIndex(vPool, index)               \
    (vPool->entries[index].method.nameTypeIndex)

/*
 * Get the Name-and-type key for the pool entry at index
 *
 * @param vPool         The CONSTANTPOOL to be accessed
 * @param index         The index to be accessed
 * @return              The nameTypeKey 
 */
#define Pol_getNameTypeKey(vPool, index)                    \
    (vPool->entries[index].nameTypeKey)

/* For CONSTANT_NameAndType_info, */

/*
 * Get the type key of a Name-and-type for the pool entry at index
 *
 * @param vPool             The CONSTANTPOOL to be accessed
 * @param nameAndTypeIndex The index to be accessed
 * @return                  The METHODTYPEKEY of that entry
 */
#define Pol_getTypeKey(vPool, nameAndTypeIndex)             \
    (vPool->entries[nameAndTypeIndex].nameTypeKey.nt.typeKey)

/*
 * Get the descriptor key of a Name-and-type for the pool entry at index
 *
 * @param vPool             The CONSTANTPOOL to be accessed
 * @param nameAndTypeIndex The index to be accessed
 * @return                  The METHODNAMEKEY of that entry
 */
#define Pol_getDescriptorKey(vPool, nameAndTypeIndex)       \
    (vPool->entries[nameAndTypeIndex].nameTypeKey.nt.nameKey)

/* ------------------------------------------------------------------------ *\
 *                      Method/Field Accessor Methods                       *
\* ------------------------------------------------------------------------ */

/*
 * Get the CLASS of a METHOD
 *
 * @param vMethod       The METHOD to be accessed
 * @return              The CLASS of the method
 */
#define Mth_getClass(vMethod)  ((CLASS)vMethod->ofClass)

/*
 * Get the CLASS of a FIELD
 *
 * @param vField        The FIELD to be accessed
 * @return              The CLASS of the field
 */
#define Fld_getClass(vField)  ((CLASS)vField->ofClass)

/*
 * Get the address of the bytecode array
 *
 * @param vMethod       The METHOD to be accessed
 * @return              The address of the bytecodes
 */
#define Mth_getBytecodes(vMethod) (vMethod->u.java.code)

/*
 * Get the length of the bytecode array
 *
 * @param vMethod       The METHOD to be accessed
 * @return              The length of the bytecodes
 */
#define Mth_getBytecodeLength(vMethod) (vMethod->u.java.codeLength)

/*
 * Return TRUE if it is a static method
 *
 * @param vMethod       The METHOD to be accessed
 * @return              TRUE if it is a static method
 */
#define Mth_isStatic(vMethod) (vMethod->accessFlags & ACC_STATIC)

/*
 * Return TRUE if it is a final method
 *
 * @param vMethod       The METHOD to be accessed
 * @return              TRUE if it is a final method
 */
#define Mth_isFinal(vMethod) (vMethod->accessFlags & ACC_FINAL)

/*
 * Return the number of locals in used by the method
 *
 * @param vMethod       The METHOD to be accessed
 * @return              The number of local variable slots
 */
#define Mth_frameSize(vMethod) (vMethod->frameSize)

/*
 * Get the start ip address for the ith exception table entry
 *
 * @param vMethod       The METHOD to be accessed
 * @param i             The exception table index
 * @return              The startPC
 */
#define Mth_getExceptionTableStartPC(vMethod, i)        \
    ((vMethod->u.java.handlers)->handlers[i].startPC)

/*
 * Get the end ip address for the ith exception table entry
 *
 * @param vMethod       The METHOD to be accessed
 * @param i             The exception table index
 * @return              The endPC
 */
#define Mth_getExceptionTableEndPC(vMethod, i)          \
    ((vMethod->u.java.handlers)->handlers[i].endPC)

/*
 * Get the handler ip address for the ith exception table entry
 *
 * @param vMethod       The METHOD to be accessed
 * @param i             The exception table index
 * @return              The handlerPC
 */
#define Mth_getExceptionTableHandlerPC(vMethod, i)      \
    ((vMethod->u.java.handlers)->handlers[i].handlerPC)

/*
 * Get the exception class for the ith exception table entry
 *
 * @param vMethod       The METHOD to be accessed
 * @param i             The exception table index
 * @return              The CLASSKEY of the exception
 */
#define Mth_getExceptionTableCatchType(vMethod,i)       \
    ((vMethod->u.java.handlers)->handlers[i].exception)

/*
 * Get the ip address for the ith entry in the stack map table
 *
 * @param vMethod       The METHOD to be accessed
 * @param i             The index into the stack map table
 * @return              The IP address for that entry
 */
extern int Mth_getStackMapEntryIP(METHOD vMethod, int i);

/*
 * Check the stackmap offset for the ith entry in the stack map table
 *
 * @param vMethod       The METHOD to be accessed
 * @param stackmapIndex The index into the stack map table
 * @return              true if ok, false if not valid.
 */
extern bool_t Mth_checkStackMapOffset(METHOD vMethod, int stackmapIndex);

/*
 * Get the length of the stack map table
 *
 * @param vMethod       The METHOD to be accessed
 * @return              The number of entries in the  stack map table
 */
extern int Mth_getExceptionTableLength(METHOD vMethod);

/*
 * The following are not a part of this interface, but defined the
 * other variables and routines used from verifier.c
 */

#define IS_NTH_BIT(var, bit)    (var[(bit) >> 5] & (1 <<  ((bit) & 0x1F)))
#define SET_NTH_BIT(var, bit)   (var[(bit) >> 5] |= (1 << ((bit) & 0x1F)))
#define callocNewBitmap(size) \
        ((unsigned long *) callocObject((size + 31) >> 5, GCT_NOPOINTERS))

extern unsigned short* vStack;
extern unsigned short* vLocals;
extern unsigned long *NEWInstructions;
extern unsigned short  vSP;
extern bool_t vNeedInitialization;

extern bool_t   vIsAssignable(CLASSKEY fromKey, CLASSKEY toKey, CLASSKEY *mergedKeyP);
extern bool_t   matchStackMap(METHOD thisMethod, IPINDEX target_ip, int flags);
extern bool_t   checkNewObject(IPINDEX this_ip, IPINDEX target_ip);

extern bool_t   vIsProtectedAccess(INSTANCE_CLASS thisClass, POOLINDEX index, bool_t isMethod);
extern int      change_Field_to_StackType(CLASSKEY fieldType, CLASSKEY* stackTypeP);
extern int      change_Arg_to_StackType(unsigned char** sigP, CLASSKEY* typeP);

#if CACHE_VERIFICATION_RESULT
bool_t checkVerifiedClassList(INSTANCE_CLASS);
void appendVerifiedClassList(INSTANCE_CLASS);
#else 
#define checkVerifiedClassList(class) FALSE
#define appendVerifiedClassList(class)
#endif

