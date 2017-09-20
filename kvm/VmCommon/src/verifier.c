/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*
 * NOTICE:
 * This file is considered "Shared Part" under the CLDC SCSL
 * and commercial licensing terms, and is therefore not to
 * be modified by licensees.  For definition of the term
 * "Shared Part", please refer to the CLDC SCSL license
 * attachment E, Section 2.1 (Definitions) paragraph "e",
 * or your commercial licensing terms as applicable.
 */

/*=========================================================================
 * KVM
 *=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Class file verifier (runtime part)
 * FILE:      verifier.c
 * OVERVIEW:  KVM has a two-phase class file verifier.  In order to
 *            run in the KVM, class files must first be processed with
 *            a special "pre-verifier" tool. This phase is typically
 *            done on the development workstation.  During execution,
 *            the runtime verifier (defined in this file) of the KVM
 *            performs the actual class file verification based on
 *            both runtime information and pre-verification information.
 * AUTHORS:   Sheng Liang, Frank Yellin, restructured by Nik Shaylor
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include "verifierUtil.h"

/*=========================================================================
 * Functions
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      Vfy_verifyMethodOrAbort
 * TYPE:          private operation on methods.
 * OVERVIEW:      Perform byte-code verification of a given method.
 *
 * INTERFACE:
 *   parameters:  vMethod: method to be verified.
 *   returns:     Normally if verification succeeds, otherwise longjmp
 *                is called (in Vfy_throw) to return control to a handler
 *                in the calling code.
 *=======================================================================*/

void Vfy_verifyMethodOrAbort(const METHOD vMethod) {

   /*
    * The following variables are constant in this function
    */
    const CLASS vClass       = Mth_getClass(vMethod);
    const CLASS vSuperClass  = Cls_getSuper(vClass);
    const int codeLength     = Mth_getBytecodeLength(vMethod);
    const int handlerCount   = Mth_getExceptionTableLength(vMethod);
    const CONSTANTPOOL vPool = Cls_getPool(vClass);

   /*
    * The virtual IP used by the verifier
    */
    IPINDEX ip = 0;

   /*
    * The following is set to TRUE where is no direct control
    * flow from current instruction to the next instruction
    * in sequence.
    */
    bool_t noControlFlow = FALSE;

   /*
    * Pointer to the "current" stackmap entry
    */
    int currentStackMapIndex = 0;

   /*
    * Check that a virtual method does not subclass a "final"
    * method higher up the inheritance chain
    *
    * Was bug 4336036
    */
    if (!Mth_isStatic(vMethod)) {

       /*
        * Dont check methods in java.lang.Object.
        */
        if (!Cls_isJavaLangObject(vClass)) {

           /*
            * Lookup the method being verified in the superclass
            */
            METHOD superMethod = Cls_lookupMethod(vClass, vSuperClass, vMethod);

           /*
            * If it exists then it must not be final
            */
            if (superMethod != NULL && Mth_isFinal(superMethod)) {
                Vfy_throw(VE_FINAL_METHOD_OVERRIDE);
            }
        }
    }

   /*
    * Verify that all exception handlers have a reasonable exception type.
    */
    if (handlerCount > 0) {
        int i;
        VERIFIERTYPE exceptionVerifierType;

       /*
        * Iterate through the handler table
        */
        for (i = 0; i < handlerCount; i++) {

           /*
            * Get the catch type from the exception table
            */
            POOLINDEX catchTypeIndex = Mth_getExceptionTableCatchType(vMethod, i);

           /*
            * If the catch type index is zero then this is a try/finally entry
            * and there is no  exception type, If it is not zero then it needs
            * to be checked.
            */
            if (catchTypeIndex != 0) {

               /*
                * Check that the entry is there and that it is a CONSTANT_Class
                */
                Pol_checkTagIsClass(vPool, catchTypeIndex);

               /*
                * Get the class key
                */
                exceptionVerifierType = Vfy_toVerifierType(Pol_getClassKey(vPool, catchTypeIndex));

               /*
                * Check that it is subclass of java.lang.Throwable
                */
                if (!Vfy_isAssignable(exceptionVerifierType, Vfy_getThrowableVerifierType())) {
                    Vfy_throw(VE_EXPECT_THROWABLE);
                }
            }
        }
    }

   /*
    * Initialize the local variable type table with the method argument types
    */
    Vfy_initializeLocals();

   /*
    * The main loop
    */
    while (ip < codeLength) {

        IPINDEX original_ip = ip;

       /*
        * Used to hold the current opcode
        */
        int opcode;

       /*
        * Used to hold IP address of the next entry in the stackmap table
        */
        IPINDEX nextStackMapIP;

       /*
        * Setup the ip used for error messages in case it is needed later
        */
        Vfy_setErrorIp(ip);

       /*
        * Output the debug trace message
        */
        Vfy_printVerifyLoopMessage(vMethod, ip);

       /*
        * Check that stackmaps are ordered according to offset and that
        * every offset in stackmaps point to the beginning to an instruction.
        */
        nextStackMapIP = Mth_getStackMapEntryIP(vMethod, currentStackMapIndex);
        if (nextStackMapIP == ip) {
            currentStackMapIndex++;         /* This offset is good. */
        }
        if (nextStackMapIP < ip) {
            Vfy_throw(VE_BAD_STACKMAP);     /* ip should have met offset. */
        }

       /*
        * Trace the instruction ip
        */
        Vfy_trace1("Check instruction %ld\n", ip);

       /*
        * Merge with the next instruction (note how noControlFlow is used here).
        */
        Vfy_checkCurrentTarget(ip, noControlFlow);

       /*
        * Set noControlFlow to its default state
        */
        noControlFlow = FALSE;

       /*
        * Look for a possible jump target in one or more exception handlers.
        * If found the current local variable types must be the same as those
        * of the handler entry point.
        */
        if (handlerCount > 0) {
            int i;

           /*
            * Iterate through the handler table
            */
            for (i = 0 ; i < handlerCount ; i++) {

               /*
                * Get the start and end points of the handler entry
                */
                IPINDEX startPC = Mth_getExceptionTableStartPC(vMethod, i);
                IPINDEX endPC   = Mth_getExceptionTableEndPC(vMethod, i);

               /*
                * Check to see if the "current" ip falls in between these pointers
                */
                if (ip >= startPC && ip < endPC) {
                    VERIFIERTYPE exceptionVerifierType;

                   /*
                    * Get the ip offset for the exception handler and the catch type index
                    */
                    IPINDEX handlerPC        = Mth_getExceptionTableHandlerPC(vMethod, i);
                    POOLINDEX catchTypeIndex = Mth_getExceptionTableCatchType(vMethod, i);

                   /*
                    * If the catch type index is zero then this is a try/finally entry.
                    * Unlike J2SE (1.3) there are no jsr/ret instructions in J2SE. The
                    * code in a finally block will be copied in to the end of the try
                    * block and an anonymous handler entry is make that points to another
                    * copy of the code. This will only be called if an exception is thrown
                    * in which case there will be an exception object of some kind on the
                    * stack. On the other hand if catch type index is non-zero then it
                    * will indicate a specific throwable type.
                    */
                    if (catchTypeIndex != 0) {
                        exceptionVerifierType = Vfy_toVerifierType(Pol_getClassKey(vPool, catchTypeIndex));
                    } else {
                        exceptionVerifierType = Vfy_getThrowableVerifierType();
                    }

                   /*
                    * Save the current stack types somewhere and re-initialize the stack.
                    */
                    Vfy_saveStackState();

                   /*
                    * Push the exception type and check the target has just this one type
                    * on the stack along with the current local variable types.
                    */
                    Vfy_push(exceptionVerifierType);
                    Vfy_checkHandlerTarget(handlerPC);

                   /*
                    * Restore the old stack types
                    */
                    Vfy_restoreStackState();
                }
            }
        }

       /*
        * Get the next bytecode
        */
        opcode = Vfy_getOpcode(ip);

        switch (opcode) {

            case NOP: {
                ip++;
                break;

            }

            case ACONST_NULL: {
                Vfy_push(ITEM_Null);
                ip++;
                break;

            }

            case ICONST_M1:
            case ICONST_0:
            case ICONST_1:
            case ICONST_2:
            case ICONST_3:
            case ICONST_4:
            case ICONST_5: {
                Vfy_push(ITEM_Integer);
                ip++;
                break;

            }

            case LCONST_0:
            case LCONST_1: {
                Vfy_push(ITEM_Long);
                Vfy_push(ITEM_Long_2);
                ip++;
                break;

            }

#if IMPLEMENTS_FLOAT

            case FCONST_0:
            case FCONST_1:
            case FCONST_2: {
                Vfy_push(ITEM_Float);
                ip++;
                break;

            }

            case DCONST_0:
            case DCONST_1: {
                Vfy_push(ITEM_Double);
                Vfy_push(ITEM_Double_2);
                ip++;
                break;

            }

#endif /* IMPLEMENTS_FLOAT */

            case BIPUSH: {
                Vfy_push(ITEM_Integer);
                ip += 2;
                break;
            }

            case SIPUSH: {
                ip++;
                Vfy_push(ITEM_Integer);
                ip += 2;
                break;

            }

            case LDC:
            case LDC_W:
            case LDC2_W: {
                POOLTAG tag;
                POOLINDEX index;

               /*
                * Get the constant pool index and advance the ip to the next instruction
                */
                if (opcode == LDC) {                        /* LDC */
                    index = Vfy_getUByte(ip + 1);
                    ip += 2;
                } else {                                    /* LDC_W or LDC2_W */
                    index = Vfy_getUShort(ip + 1);
                    ip += 3;
                }

               /*
                * Get the tag
                */
                tag = Pol_getTag(vPool, index);

               /*
                * Check for the right kind of LDC and push the required type
                */

                if (opcode == LDC2_W) {                     /* LDC2_W */
                    if (tag == CONSTANT_Long) {
                        Vfy_push(ITEM_Long);
                        Vfy_push(ITEM_Long_2);
                        break;
                    }
#if IMPLEMENTS_FLOAT
                    if (tag == CONSTANT_Double) {
                        Vfy_push(ITEM_Double);
                        Vfy_push(ITEM_Double_2);
                        break;
                    }
#endif
                } else {                                    /* LDC or LDC_W */
                    if (tag == CONSTANT_String) {
                        Vfy_push(Vfy_getStringVerifierType());
                        break;
                    }

                    if (tag == CONSTANT_Integer) {
                        Vfy_push(ITEM_Integer);
                        break;
                    }
#if IMPLEMENTS_FLOAT
                    if (tag == CONSTANT_Float) {
                        Vfy_push(ITEM_Float);
                        break;
                    }
#endif
                }
                Vfy_throw(VE_BAD_LDC);
            }

            case ILOAD_0:
            case ILOAD_1:
            case ILOAD_2:
            case ILOAD_3:
            case ILOAD: {
                SLOTINDEX index;
                if (opcode == ILOAD) {
                    index = Vfy_getUByte(ip + 1);
                    ip += 2;
                } else {
                    index = opcode - ILOAD_0;
                    ip++;
                }
                Vfy_getLocal(index, ITEM_Integer);
                Vfy_push(ITEM_Integer);
                break;
            }

            case LLOAD_0:
            case LLOAD_1:
            case LLOAD_2:
            case LLOAD_3:
            case LLOAD: {
                SLOTINDEX index;
                if (opcode == LLOAD) {
                    index = Vfy_getUByte(ip + 1);
                    ip += 2;
                } else {
                    index = opcode - LLOAD_0;
                    ip++;
                }
                Vfy_getLocal(index,     ITEM_Long);
                Vfy_getLocal(index + 1, ITEM_Long_2);
                Vfy_push(ITEM_Long);
                Vfy_push(ITEM_Long_2);
                break;

            }

#if IMPLEMENTS_FLOAT

            case FLOAD_0:
            case FLOAD_1:
            case FLOAD_2:
            case FLOAD_3:
            case FLOAD: {
                SLOTINDEX index;
                if (opcode == FLOAD) {
                    index = Vfy_getUByte(ip + 1);
                    ip += 2;
                } else {
                    index = opcode - FLOAD_0;
                    ip++;
                }
                Vfy_getLocal(index, ITEM_Float);
                Vfy_push(ITEM_Float);
                break;
            }

            case DLOAD_0:
            case DLOAD_1:
            case DLOAD_2:
            case DLOAD_3:
            case DLOAD: {
                SLOTINDEX index;
                if (opcode == DLOAD) {
                    index = Vfy_getUByte(ip + 1);
                    ip += 2;
                } else {
                    index = opcode - DLOAD_0;
                    ip++;
                }
                Vfy_getLocal(index, ITEM_Double);
                Vfy_getLocal(index + 1, ITEM_Double_2);
                Vfy_push(ITEM_Double);
                Vfy_push(ITEM_Double_2);
                break;

            }

#endif /* IMPLEMENTS_FLOAT */

            case ALOAD_0:
            case ALOAD_1:
            case ALOAD_2:
            case ALOAD_3:
            case ALOAD: {
                SLOTINDEX    index;
                VERIFIERTYPE refType;
                if (opcode == ALOAD) {
                    index = Vfy_getUByte(ip + 1);
                    ip += 2;
                } else {
                    index = opcode - ALOAD_0;
                    ip++;
                }
                refType = Vfy_getLocal(index, ITEM_Reference);
                Vfy_push(refType);
                break;

            }

            case IALOAD: {
                Vfy_pop(ITEM_Integer);
                Vfy_pop(Vfy_getIntArrayVerifierType());
                Vfy_push(ITEM_Integer);
                ip++;
                break;

            }

            case BALOAD: {
                VERIFIERTYPE dummy     = Vfy_pop(ITEM_Integer);
                VERIFIERTYPE arrayType = Vfy_pop(Vfy_getObjectVerifierType());
                if (arrayType != Vfy_getByteArrayVerifierType() &&
                    arrayType != Vfy_getBooleanArrayVerifierType() &&
                    arrayType != ITEM_Null) {
                    Vfy_throw(VE_BALOAD_BAD_TYPE);
                }
                Vfy_push(ITEM_Integer);
                ip++;
                (void)dummy;
                break;
            }

            case CALOAD: {
                Vfy_pop(ITEM_Integer);
                Vfy_pop(Vfy_getCharArrayVerifierType());
                Vfy_push(ITEM_Integer);
                ip++;
                break;

            }

            case SALOAD: {
                Vfy_pop(ITEM_Integer);
                Vfy_pop(Vfy_getShortArrayVerifierType());
                Vfy_push(ITEM_Integer);
                ip++;
                break;

            }

            case LALOAD: {
                Vfy_pop(ITEM_Integer);
                Vfy_pop(Vfy_getLongArrayVerifierType());
                Vfy_push(ITEM_Long);
                Vfy_push(ITEM_Long_2);
                ip++;
                break;
            }

#if IMPLEMENTS_FLOAT

            case FALOAD: {
                Vfy_pop(ITEM_Integer);
                Vfy_pop(Vfy_getFloatArrayVerifierType());
                Vfy_push(ITEM_Float);
                ip++;
                break;

            }

            case DALOAD: {
                Vfy_pop(ITEM_Integer);
                Vfy_pop(Vfy_getDoubleArrayVerifierType());
                Vfy_push(ITEM_Double);
                Vfy_push(ITEM_Double_2);
                ip++;
                break;

            }

#endif /* IMPLEMENTS_FLOAT */

            case AALOAD: {

                VERIFIERTYPE dummy     = Vfy_pop(ITEM_Integer);
                VERIFIERTYPE arrayType = Vfy_pop(Vfy_getObjectVerifierType());
                VERIFIERTYPE arrayElementType;
                if (!Vfy_isAssignable(arrayType, Vfy_getObjectArrayVerifierType())) {
                    Vfy_throw(VE_AALOAD_BAD_TYPE);
                }

/* Alternative implementation:
                VERIFIERTYPE dummy     = Vfy_pop(ITEM_Integer);
                VERIFIERTYPE arrayType = Vfy_pop(Vfy_getObjectArrayVerifierType());
                VERIFIERTYPE arrayElementType;
*/

                arrayElementType = Vfy_getReferenceArrayElementType(arrayType);
                Vfy_push(arrayElementType);
                ip++;
                (void)dummy;
                break;
            }

            case ISTORE_0:
            case ISTORE_1:
            case ISTORE_2:
            case ISTORE_3:
            case ISTORE: {
                SLOTINDEX index;
                if (opcode == ISTORE) {
                    index = Vfy_getUByte(ip + 1);
                    ip += 2;
                } else {
                    index = opcode - ISTORE_0;
                    ip++;
                }
                Vfy_pop(ITEM_Integer);
                Vfy_setLocal(index, ITEM_Integer);
                break;

            }

            case LSTORE_0:
            case LSTORE_1:
            case LSTORE_2:
            case LSTORE_3:
            case LSTORE: {
                SLOTINDEX index;
                if (opcode == LSTORE) {
                    index = Vfy_getUByte(ip + 1);
                    ip += 2;
                } else {
                    index = opcode - LSTORE_0;
                    ip++;
                }
                Vfy_pop(ITEM_Long_2);
                Vfy_pop(ITEM_Long);
                Vfy_setLocal(index + 1, ITEM_Long_2);
                Vfy_setLocal(index, ITEM_Long);
                break;
            }

#if IMPLEMENTS_FLOAT

            case FSTORE_0:
            case FSTORE_1:
            case FSTORE_2:
            case FSTORE_3:
            case FSTORE: {
                SLOTINDEX index;
                if (opcode == FSTORE) {
                    index = Vfy_getUByte(ip + 1);
                    ip += 2;
                } else {
                    index = opcode - FSTORE_0;
                    ip++;
                }
                Vfy_pop(ITEM_Float);
                Vfy_setLocal(index, ITEM_Float);
                break;
            }

            case DSTORE_0:
            case DSTORE_1:
            case DSTORE_2:
            case DSTORE_3:
            case DSTORE: {
                SLOTINDEX index;
                if (opcode == DSTORE) {
                    index = Vfy_getUByte(ip + 1);
                    ip += 2;
                } else {
                    index = opcode - DSTORE_0;
                    ip++;
                }
                Vfy_pop(ITEM_Double_2);
                Vfy_pop(ITEM_Double);
                Vfy_setLocal(index + 1, ITEM_Double_2);
                Vfy_setLocal(index, ITEM_Double);
                break;
            }

#endif /* IMPLEMENTS_FLOAT */

            case ASTORE_0:
            case ASTORE_1:
            case ASTORE_2:
            case ASTORE_3:
            case ASTORE: {
                SLOTINDEX index;
                VERIFIERTYPE arrayElementType;
                if (opcode == ASTORE) {
                    index = Vfy_getUByte(ip + 1);
                    ip += 2;
                } else {
                    index = opcode - ASTORE_0;
                    ip++;
                }
                arrayElementType = Vfy_pop(ITEM_Reference);
                Vfy_setLocal(index, arrayElementType);
                break;
            }

            case IASTORE: {
                Vfy_pop(ITEM_Integer);
                Vfy_pop(ITEM_Integer);
                Vfy_pop(Vfy_getIntArrayVerifierType());
                ip++;
                break;

            }

            case BASTORE: {
                VERIFIERTYPE dummy1    = Vfy_pop(ITEM_Integer);
                VERIFIERTYPE dummy2    = Vfy_pop(ITEM_Integer);
                VERIFIERTYPE arrayType = Vfy_pop(Vfy_getObjectVerifierType());
                if (arrayType != Vfy_getByteArrayVerifierType() &&
                    arrayType != Vfy_getBooleanArrayVerifierType() &&
                    arrayType != ITEM_Null) {
                    Vfy_throw(VE_BASTORE_BAD_TYPE);
                }
                ip++;
                (void)dummy1;
                (void)dummy2;
                break;
            }

            case CASTORE: {
                Vfy_pop(ITEM_Integer);
                Vfy_pop(ITEM_Integer);
                Vfy_pop(Vfy_getCharArrayVerifierType());
                ip++;
                break;

            }

            case SASTORE: {
                Vfy_pop(ITEM_Integer);
                Vfy_pop(ITEM_Integer);
                Vfy_pop(Vfy_getShortArrayVerifierType());
                ip++;
                break;

            }

            case LASTORE: {
                Vfy_pop(ITEM_Long_2);
                Vfy_pop(ITEM_Long);
                Vfy_pop(ITEM_Integer);
                Vfy_pop(Vfy_getLongArrayVerifierType());
                ip++;
                break;

            }

#if IMPLEMENTS_FLOAT

            case FASTORE: {
                Vfy_pop(ITEM_Float);
                Vfy_pop(ITEM_Integer);
                Vfy_pop(Vfy_getFloatArrayVerifierType());
                ip++;
                break;

            }

            case DASTORE: {
                Vfy_pop(ITEM_Double_2);
                Vfy_pop(ITEM_Double);
                Vfy_pop(ITEM_Integer);
                Vfy_pop(Vfy_getDoubleArrayVerifierType());
                ip++;
                break;

            }

#endif /* IMPLEMENTS_FLOAT */

            case AASTORE: {
                VERIFIERTYPE value            = Vfy_pop(Vfy_getObjectVerifierType());
                VERIFIERTYPE dummy            = Vfy_pop(ITEM_Integer);
                VERIFIERTYPE arrayType        = Vfy_pop(Vfy_getObjectArrayVerifierType());
                VERIFIERTYPE arrayElementType;

               /*
                * The value to be stored must be some kind of object and the
                * array must be some kind of reference array.
                */
                if (
                    !Vfy_isAssignable(value,     Vfy_getObjectVerifierType()) ||
                    !Vfy_isAssignable(arrayType, Vfy_getObjectArrayVerifierType())
                   ) {
                    Vfy_throw(VE_AASTORE_BAD_TYPE);
                }

               /*
                * Get the actual element type of the array
                */
                arrayElementType = Vfy_getReferenceArrayElementType(arrayType);

               /*
                * This part of the verifier is far from obvious, but the logic
                * appears to be as follows:
                *
                * 1, Because not all stores into a reference array can be
                *    statically checked then they never are in the case
                *    where the array is of one dimension and the object
                *    being inserted is a non-array, The verifier will
                *    ignore such errors and they will all be found at runtime.
                *
                * 2, However, if the array is of more than one dimension or
                *    the object being inserted is some kind of an array then
                *    a check is made by the verifier and errors found at
                *    this time (statically) will cause the method to fail
                *    verification. Presumable not all errors will will be found
                *    here and so some runtime errors can occur in this case
                *    as well.
                */
                if (!Vfy_isArray(arrayElementType) && !Vfy_isArray(value)) {
                    /* Success */
                } else if (Vfy_isAssignable(value, arrayElementType)) {
                    /* Success */
                } else {
                    Vfy_throw(VE_AASTORE_BAD_TYPE);
                }
                ip++;
                (void)dummy;
                break;
            }

            case POP: {
                Vfy_popCategory1();
                ip++;
                break;

            }

            case POP2: {
                Vfy_popCategory2_secondWord();
                Vfy_popCategory2_firstWord();
                ip++;
                break;

            }

            case DUP: {
                VERIFIERTYPE type = Vfy_popCategory1();
                Vfy_push(type);
                Vfy_push(type);
                ip++;
                break;

            }

            case DUP_X1:{
                VERIFIERTYPE type1 = Vfy_popCategory1();
                VERIFIERTYPE type2 = Vfy_popCategory1();
                Vfy_push(type1);
                Vfy_push(type2);
                Vfy_push(type1);
                ip++;
                break;
            }

            case DUP_X2: {
                VERIFIERTYPE cat1type = Vfy_popCategory1();
                VERIFIERTYPE second   = Vfy_popDoubleWord_secondWord();
                VERIFIERTYPE first    = Vfy_popDoubleWord_firstWord();
                Vfy_push(cat1type);
                Vfy_push(first);
                Vfy_push(second);
                Vfy_push(cat1type);
                ip++;
                break;
            }

            case DUP2: {
                VERIFIERTYPE second = Vfy_popDoubleWord_secondWord();
                VERIFIERTYPE first  = Vfy_popDoubleWord_firstWord();
                Vfy_push(first);
                Vfy_push(second);
                Vfy_push(first);
                Vfy_push(second);
                ip++;
                break;
            }

            case DUP2_X1: {
                VERIFIERTYPE second   = Vfy_popDoubleWord_secondWord();
                VERIFIERTYPE first    = Vfy_popDoubleWord_firstWord();
                VERIFIERTYPE cat1type = Vfy_popCategory1();
                Vfy_push(first);
                Vfy_push(second);
                Vfy_push(cat1type);
                Vfy_push(first);
                Vfy_push(second);
                ip++;
                break;
            }

            case DUP2_X2: {
                VERIFIERTYPE item1second = Vfy_popDoubleWord_secondWord();
                VERIFIERTYPE item1first  = Vfy_popDoubleWord_firstWord();
                VERIFIERTYPE item2second = Vfy_popDoubleWord_secondWord();
                VERIFIERTYPE item2first  = Vfy_popDoubleWord_firstWord();
                Vfy_push(item1first);
                Vfy_push(item1second);
                Vfy_push(item2first);
                Vfy_push(item2second);
                Vfy_push(item1first);
                Vfy_push(item1second);
                ip++;
                break;
            }

            case SWAP: {
                VERIFIERTYPE type1 = Vfy_popCategory1();
                VERIFIERTYPE type2 = Vfy_popCategory1();
                Vfy_push(type1);
                Vfy_push(type2);
                ip++;
                break;
            }

            case IADD:
            case ISUB:
            case IMUL:
            case IDIV:
            case IREM:
            case ISHL:
            case ISHR:
            case IUSHR:
            case IOR:
            case IXOR:
            case IAND: {
                Vfy_pop(ITEM_Integer);
                Vfy_pop(ITEM_Integer);
                Vfy_push(ITEM_Integer);
                ip++;
                break;
            }

            case INEG:
                Vfy_pop(ITEM_Integer);
                Vfy_push(ITEM_Integer);
                ip++;
                break;

            case LADD:
            case LSUB:
            case LMUL:
            case LDIV:
            case LREM:
            case LAND:
            case LOR:
            case LXOR: {
                Vfy_pop(ITEM_Long_2);
                Vfy_pop(ITEM_Long);
                Vfy_pop(ITEM_Long_2);
                Vfy_pop(ITEM_Long);
                Vfy_push(ITEM_Long);
                Vfy_push(ITEM_Long_2);
                ip++;
                break;

            }

            case LNEG: {
                Vfy_pop(ITEM_Long_2);
                Vfy_pop(ITEM_Long);
                Vfy_push(ITEM_Long);
                Vfy_push(ITEM_Long_2);
                ip++;
                break;
            }

            case LSHL:
            case LSHR:
            case LUSHR: {
                Vfy_pop(ITEM_Integer);
                Vfy_pop(ITEM_Long_2);
                Vfy_pop(ITEM_Long);
                Vfy_push(ITEM_Long);
                Vfy_push(ITEM_Long_2);
                ip++;
                break;

            }

#if IMPLEMENTS_FLOAT

            case FADD:
            case FSUB:
            case FMUL:
            case FDIV:
            case FREM: {
                Vfy_pop(ITEM_Float);
                Vfy_pop(ITEM_Float);
                Vfy_push(ITEM_Float);
                ip++;
                break;

            }

            case FNEG: {
                Vfy_pop(ITEM_Float);
                Vfy_push(ITEM_Float);
                ip++;
                break;
            }

            case DADD:
            case DSUB:
            case DMUL:
            case DDIV:
            case DREM: {
                Vfy_pop(ITEM_Double_2);
                Vfy_pop(ITEM_Double);
                Vfy_pop(ITEM_Double_2);
                Vfy_pop(ITEM_Double);
                Vfy_push(ITEM_Double);
                Vfy_push(ITEM_Double_2);
                ip++;
                break;

            }

            case DNEG: {
                Vfy_pop(ITEM_Double_2);
                Vfy_pop(ITEM_Double);
                Vfy_push(ITEM_Double);
                Vfy_push(ITEM_Double_2);
                ip++;
                break;
            }

#endif /* IMPLEMENTS_FLOAT */

            case IINC: {
                SLOTINDEX index = Vfy_getUByte(ip + 1);
                ip += 3;
                Vfy_getLocal(index, ITEM_Integer);
                Vfy_setLocal(index, ITEM_Integer);
                break;

            }

            case I2L: {
                Vfy_pop(ITEM_Integer);
                Vfy_push(ITEM_Long);
                Vfy_push(ITEM_Long_2);
                ip++;
                break;

            }

            case L2I: {
                Vfy_pop(ITEM_Long_2);
                Vfy_pop(ITEM_Long);
                Vfy_push(ITEM_Integer);
                ip++;
                break;

            }

#if IMPLEMENTS_FLOAT

            case I2F: {
                Vfy_pop(ITEM_Integer);
                Vfy_push(ITEM_Float);
                ip++;
                break;

            }

            case I2D: {
                Vfy_pop(ITEM_Integer);
                Vfy_push(ITEM_Double);
                Vfy_push(ITEM_Double_2);
                ip++;
                break;

            }

            case L2F: {
                Vfy_pop(ITEM_Long_2);
                Vfy_pop(ITEM_Long);
                Vfy_push(ITEM_Float);
                ip++;
                break;

            }

            case L2D: {
                Vfy_pop(ITEM_Long_2);
                Vfy_pop(ITEM_Long);
                Vfy_push(ITEM_Double);
                Vfy_push(ITEM_Double_2);
                ip++;
                break;

            }

            case F2I: {
                Vfy_pop(ITEM_Float);
                Vfy_push(ITEM_Integer);
                ip++;
                break;

            }

            case F2L: {
                Vfy_pop(ITEM_Float);
                Vfy_push(ITEM_Long);
                Vfy_push(ITEM_Long_2);
                ip++;
                break;

            }

            case F2D: {
                Vfy_pop(ITEM_Float);
                Vfy_push(ITEM_Double);
                Vfy_push(ITEM_Double_2);
                ip++;
                break;

            }

            case D2I: {
                Vfy_pop(ITEM_Double_2);
                Vfy_pop(ITEM_Double);
                Vfy_push(ITEM_Integer);
                ip++;
                break;

            }

            case D2L: {
                Vfy_pop(ITEM_Double_2);
                Vfy_pop(ITEM_Double);
                Vfy_push(ITEM_Long);
                Vfy_push(ITEM_Long_2);
                ip++;
                break;

            }

            case D2F: {
                Vfy_pop(ITEM_Double_2);
                Vfy_pop(ITEM_Double);
                Vfy_push(ITEM_Float);
                ip++;
                break;

            }

#endif /* IMPLEMENTS_FLOAT */

            case I2B:
            case I2C:
            case I2S: {
                Vfy_pop(ITEM_Integer);
                Vfy_push(ITEM_Integer);
                ip++;
                break;

            }

            case LCMP: {
                Vfy_pop(ITEM_Long_2);
                Vfy_pop(ITEM_Long);
                Vfy_pop(ITEM_Long_2);
                Vfy_pop(ITEM_Long);
                Vfy_push(ITEM_Integer);
                ip++;
                break;

            }

#if IMPLEMENTS_FLOAT

            case FCMPL:
            case FCMPG: {
                Vfy_pop(ITEM_Float);
                Vfy_pop(ITEM_Float);
                Vfy_push(ITEM_Integer);
                ip++;
                break;

            }

            case DCMPL:
            case DCMPG: {
                Vfy_pop(ITEM_Double_2);
                Vfy_pop(ITEM_Double);
                Vfy_pop(ITEM_Double_2);
                Vfy_pop(ITEM_Double);
                Vfy_push(ITEM_Integer);
                ip++;
                break;

            }

#endif /* IMPLEMENTS_FLOAT */

            case IF_ICMPEQ:
            case IF_ICMPNE:
            case IF_ICMPLT:
            case IF_ICMPGE:
            case IF_ICMPGT:
            case IF_ICMPLE: {
                Vfy_pop(ITEM_Integer);
                Vfy_pop(ITEM_Integer);
                Vfy_checkJumpTarget(ip, (IPINDEX)(ip + Vfy_getShort(ip + 1)));
                ip += 3;
                break;
            }

            case IFEQ:
            case IFNE:
            case IFLT:
            case IFGE:
            case IFGT:
            case IFLE: {
                Vfy_pop(ITEM_Integer);
                Vfy_checkJumpTarget(ip, (IPINDEX)(ip + Vfy_getShort(ip + 1)));
                ip += 3;
                break;
            }

            case IF_ACMPEQ:
            case IF_ACMPNE: {
                Vfy_pop(ITEM_Reference);
                Vfy_pop(ITEM_Reference);
                Vfy_checkJumpTarget(ip, (IPINDEX)(ip + Vfy_getShort(ip + 1)));
                ip += 3;
                break;
            }

            case IFNULL:
            case IFNONNULL:
                Vfy_pop(ITEM_Reference);
                Vfy_checkJumpTarget(ip, (IPINDEX)(ip + Vfy_getShort(ip + 1)));
                ip += 3;
                break;

            case GOTO: {
                Vfy_checkJumpTarget(ip, (IPINDEX)(ip + Vfy_getShort(ip + 1)));
                ip += 3;
                noControlFlow = TRUE;
                break;
            }

            case GOTO_W: {
                Vfy_checkJumpTarget(ip, (IPINDEX)(ip + Vfy_getCell(ip + 1)));
                ip += 5;
                noControlFlow = TRUE;
                break;
            }

            case TABLESWITCH:
            case LOOKUPSWITCH: {
                IPINDEX lpc = ((ip + 1) + 3) & ~3; /* Advance one byte and align to the next word boundary */
                IPINDEX lptr;
                int keys;
                int k, delta;

               /*
                * Pop the switch argument
                */
                Vfy_pop(ITEM_Integer);

               /*
                * Get the number of keys and the delta between each entry
                */
                if (opcode == TABLESWITCH) {
                    keys = Vfy_getCell(lpc + 8) - Vfy_getCell(lpc + 4) + 1;
                    delta = 4;
                } else {
                    keys = Vfy_getCell(lpc + 4);
                    delta = 8;
                    /*
                     * Make sure that the tableswitch items are sorted
                     */
                    for (k = keys - 1, lptr = lpc + 8 ; --k >= 0 ; lptr += 8) {
                        long this_key = Vfy_getCell(lptr);
                        long next_key = Vfy_getCell(lptr + 8);
                        if (this_key >= next_key) {
                            Vfy_throw(VE_BAD_LOOKUPSWITCH);
                        }
                    }
                }

               /*
                * Check the default case
                */
                Vfy_checkJumpTarget(ip, (IPINDEX)(ip + Vfy_getCell(lpc)));

               /*
                * Check all the other cases
                */
                for (k = keys, lptr = lpc + 12 ; --k >= 0; lptr += delta) {
                    Vfy_checkJumpTarget(ip, (IPINDEX)(ip + Vfy_getCell(lptr)));
                }

               /*
                * Advance the virtual ip to the next instruction
                */
                ip = lptr - delta + 4;
                noControlFlow = TRUE;

                break;
            }

            case IRETURN: {
                Vfy_popReturn(ITEM_Integer);
                ip++;
                noControlFlow = TRUE;
                break;

            }

            case LRETURN: {
                Vfy_pop(ITEM_Long_2);
                Vfy_popReturn(ITEM_Long);
                ip++;
                noControlFlow = TRUE;
                break;

            }

#if IMPLEMENTS_FLOAT

            case FRETURN: {
                Vfy_popReturn(ITEM_Float);
                ip++;
                noControlFlow = TRUE;
                break;

            }

            case DRETURN: {
                Vfy_pop(ITEM_Double_2);
                Vfy_popReturn(ITEM_Double);
                ip++;
                noControlFlow = TRUE;
                break;
            }

#endif /* IMPLEMENTS_FLOAT */

            case ARETURN: {
                Vfy_popReturn(Vfy_getObjectVerifierType());
                ip++;
                noControlFlow = TRUE;
                break;

            }

            case RETURN: {
                Vfy_returnVoid();
                ip++;
                noControlFlow = TRUE;
                break;
            }

            case GETSTATIC:
            case PUTSTATIC:
            case GETFIELD:
            case PUTFIELD: {
                POOLINDEX fieldNameAndTypeIndex;
                CLASSKEY fieldTypeKey;

               /*
                * Get the index into the constant pool where the field is defined
                */
                POOLINDEX fieldIndex = Vfy_getUShort(ip + 1);

               /*
                * Check that the tag is correct
                */
                Pol_checkTagIs(vPool, fieldIndex, CONSTANT_Fieldref, VE_EXPECT_FIELDREF);

               /*
                * Get the indexto the name-and-type entry
                */
                fieldNameAndTypeIndex = Pol_getNameAndTypeIndex(vPool, fieldIndex);

               /*
                * Get the field type key ("I", "J, "Ljava.lang.Foobar" etc)
                */
                fieldTypeKey = Pol_getTypeKey(vPool, fieldNameAndTypeIndex);

                if (opcode == GETFIELD || opcode == PUTFIELD) {

                   /*
                    * Get the index to the class of the field
                    */
                    POOLINDEX fieldClassIndex = Pol_getClassIndex(vPool, fieldIndex);

                   /*
                    * Verification of access to protected fields is done here because
                    * the superclass must be loaded at this time (other checks are
                    * done at runtime in order to implement lazy class loading.)
                    *
                    * In order for the access to a protected field to be legal the
                    * class of this method must be a direct descendant of the class
                    * who's field is being accessed. By placing the method's class
                    * in place of the class specified in the constant pool this
                    * will be checked.
                    */
                    VERIFIERTYPE targetVerifierType;

                    if (Vfy_isProtectedField(vClass, fieldIndex)) {
                        targetVerifierType = Vfy_toVerifierType(Cls_getKey(vClass));
                    } else {
                        targetVerifierType = Vfy_toVerifierType(Pol_getClassKey(vPool, fieldClassIndex));
                    }

                   /*
                    * GETFIELD / PUTFIELD
                    */
                    if (opcode == GETFIELD) {
                        Vfy_pop(targetVerifierType);
                        Vfy_pushClassKey(fieldTypeKey);
                    } else {
                        VERIFIERTYPE obj_type;
                        Vfy_popClassKey(fieldTypeKey);

                        /* 2nd edition JVM allows field initializations
                         * before the super class initializer if the
                         * field is defined within the current class.
                         */
                        obj_type = Vfy_popCategory1(); 
                        if (obj_type == ITEM_InitObject) {
                            /* special case for uninitialized objects */ 
                            NameTypeKey nameTypeKey = 
                              Pol_getNameTypeKey(vPool, fieldNameAndTypeIndex);

                            INSTANCE_CLASS clazz = 
                              (INSTANCE_CLASS) Pol_getClass(vPool, fieldClassIndex);
                            FIELD thisField = 
                              Cls_lookupField((INSTANCE_CLASS) clazz, nameTypeKey);
                            if (thisField == NULL || 
                                (INSTANCE_CLASS)Fld_getClass(thisField) != clazz) {
                                Vfy_throw(VE_EXPECTING_OBJ_OR_ARR_ON_STK);
                            }
                        } else {
                            Vfy_push(obj_type);
                            Vfy_pop(targetVerifierType);
                        }
                    }
                } else {

                   /*
                    * GETSTATIC / PUTSTATIC
                    */
                    if (opcode == GETSTATIC) {
                        Vfy_pushClassKey(fieldTypeKey);
                    } else {
                        Vfy_popClassKey(fieldTypeKey);
                    }
                }

                ip += 3;
                break;
            }

            case INVOKEVIRTUAL:
            case INVOKESPECIAL:
            case INVOKESTATIC:
            case INVOKEINTERFACE: {
                POOLINDEX      methodNameAndTypeIndex;
                POOLINDEX      methodClassIndex;
                METHODNAMEKEY  methodNameKey;
                METHODTYPEKEY  methodTypeKey;
                int            nwords;

               /*
                * Get the index into the constant pool where the method is defined
                */
                POOLINDEX methodIndex = Vfy_getUShort(ip + 1);

               /*
                * Check that the tag is correct
                */
                Pol_checkTag2Is(vPool,
                                methodIndex,
                                CONSTANT_Methodref,           /* this or...*/
                                CONSTANT_InterfaceMethodref,  /* this */
                                VE_EXPECT_METHODREF);

               /*
                * Get the index to the class of the method
                */
                methodClassIndex = Pol_getClassIndex(vPool, methodIndex);;

               /*
                * Get the indexto the name-and-type entry
                */
                methodNameAndTypeIndex = Pol_getNameAndTypeIndex(vPool, methodIndex);

               /*
                * Get the method type key ("(I)J", "(III)D", "(Ljava.lang.Foobar)V" etc.)
                */
                methodTypeKey = Pol_getTypeKey(vPool, methodNameAndTypeIndex);

               /*
                * Get the method name
                */
                methodNameKey = Pol_getDescriptorKey(vPool, methodNameAndTypeIndex);

               /*
                * Setup the callee context
                */
                Vfy_setupCalleeContext(methodTypeKey);

               /*
                * Pop the arguments from the stack
                */
                nwords = Vfy_popInvokeArguments();

               /*
                * Check that the only method called that starts with "<" is <init>
                * and that it is only called via an INVOKESPECIAL
                */
                if (Vfy_MethodNameStartsWithLeftAngleBracket(methodNameKey)) {
                    if (opcode != INVOKESPECIAL || !Vfy_MethodNameIsInit(methodNameKey)) {
                        Vfy_throw(VE_EXPECT_INVOKESPECIAL);
                    }
                }

               /*
                * Receiver type processing
                */
                if (opcode != INVOKESTATIC) {
                    CLASSKEY methodClassKey = Pol_getClassKey(vPool, methodClassIndex);
                    CLASSKEY targetClassKey;

                   /*
                    * Special processing for calls to <init>
                    */
                    if (Vfy_MethodNameIsInit(methodNameKey)) {

                       /*
                        * Pop the receiver type
                        */
                        VERIFIERTYPE receiverType = Vfy_popCategory1();

                        if (Vfy_isVerifierTypeForNew(receiverType)) {
                           /*
                            * This for is a call to <init> that is made to an object that has just been
                            * created with a NEW bytecode. The address of the new was encoded
                            * as a VERIFIERTYPE by the NEW bytecode using a function called
                            * Vfy_createVerifierTypeForNewAt(). This is converted back into
                            * an IP address and is used to lookup the receiver CLASSKEY
                            */

                            POOLINDEX newIndex;

                           /*
                            * receiverType is a special encoding of the ip offset into the
                            * method where a NEW instruction should be. Get the index
                            * and check this out.
                            */
                            IPINDEX newIP = Vfy_retrieveIpForNew(receiverType);
                            if (newIP > codeLength - 3 || Vfy_getOpcode(newIP) != NEW) {
                                Vfy_throw(VE_EXPECT_NEW);
                            }

                           /*
                            * Get the pool index after the NEW bytecode and check that
                            * is a CONSTANT_class
                            */
                            newIndex = Vfy_getUShort(newIP + 1);
                            Pol_checkTagIsClass(vPool, newIndex);

                           /*
                            * Get the CLASSKEY for is entry
                            */
                            targetClassKey = Pol_getClassKey(vPool, newIndex);

                           /*
                            * The method must be an <init> method of the indicated class
                            */
                            if (targetClassKey != methodClassKey) {
                                Vfy_throw(VE_BAD_INIT_CALL);
                            }

                        } else if (receiverType == ITEM_InitObject) {
                          int i;

                           /*
                            * This for is a call to <init> that is made as a result of
                            * a constructor calling another constructor (e.g. this() or super())
                            * The receiver CLASSKEY must be the same as the one for the class
                            * of the method being verified or its superclass
                            */
                            targetClassKey = Cls_getKey(vClass);

                            /*
                             * Check that it is this() or super()
                             */
                            if ((methodClassKey != targetClassKey && 
                                  methodClassKey != Cls_getKey(vSuperClass))
                                || !vNeedInitialization) {
                                Vfy_throw(VE_BAD_INIT_CALL);
                            }

                            /* Make sure that the current opcode is not
                             * inside any exception handlers.
                             */
                            for (i = 0 ; i < handlerCount ; i++) {
                                /*
                                 * Get the start and end points of the
                                 handler entry 
                                 */
                                IPINDEX startPC = 
                                    Mth_getExceptionTableStartPC(vMethod, i);
                                IPINDEX endPC = 
                                    Mth_getExceptionTableEndPC(vMethod, i);
                                if (ip >= startPC && ip < endPC) {
                                    Vfy_throw(VE_BAD_INIT_CALL);
                                }
                            }

                            vNeedInitialization = FALSE;

                        } else {

                            Vfy_throw(VE_EXPECT_UNINIT);

                        }

                       /*
                        * Replace all the <init> receiver type with the real target type
                        */
                        Vfy_ReplaceTypeWithType(receiverType, Vfy_toVerifierType(targetClassKey));

                    } else {

                       /*
                        * This is for the non INVOKESTATIC case where <init> is not being called.
                        *
                        * Check that an INVOKESPECIAL is either to the method being verified
                        * or a superclass.
                        */
                        if (opcode == INVOKESPECIAL && methodClassKey != Cls_getKey(vClass)) {
                            CLASS superClass = vSuperClass;
                            while (superClass != NULL && Cls_getKey(superClass) != methodClassKey) {
                                superClass = Cls_getSuper(superClass);
                            }
                            if (superClass == NULL) {
                                Vfy_throw(VE_INVOKESPECIAL);
                            }
                        }

                       /*
                        * Verification of access to protected methods is done here because
                        * the superclass must be loaded at this time (other checks are
                        * done at runtime in order to implement lazy class loading.)
                        *
                        * In order for the access to a protected method to be legal the
                        * class of this method must be a direct descendant of the class
                        * who's field is being accessed. By placing the method's class
                        * in place of the class specified in the constant pool this
                        * will be checked.
                        */
                        if (
                             (opcode == INVOKESPECIAL || opcode == INVOKEVIRTUAL) &&
                             (Vfy_isProtectedMethod(vClass, methodIndex))
                           ) {
                            Vfy_pop(Cls_getKey(vClass));
                        } else {
                            Vfy_pop(methodClassKey);
                        }
                    }
                }

               /*
                * Push the result type.
                */
                Vfy_pushInvokeResult();

               /*
                * Test a few INVOKEINTERFACE things and increment the ip
                */
                if (opcode == INVOKEINTERFACE) {
                    if (Vfy_getUByte(ip + 3) != nwords + 1) {
                        Vfy_throw(VE_NARGS_MISMATCH);
                    }
                    if (Vfy_getUByte(ip + 4) != 0) {
                        Vfy_throw(VE_EXPECT_ZERO);
                    }
                    ip += 5;
                } else {
                    ip += 3;
                }
                break;
            }

            case NEW: {
                CLASSKEY typeKey;

               /*
                * Get the pool index and check that it is a CONSTANT_Class
                */
                POOLINDEX index = Vfy_getUShort(ip + 1);
                Pol_checkTagIsClass(vPool, index);

               /*
                * Get the class and check that is it not an array class
                */
                typeKey = Pol_getClassKey(vPool, index);
                if (Vfy_isArrayClassKey(typeKey, 1)) {
                    Vfy_throw(VE_EXPECT_CLASS);
                }

               /*
                * Convert the IP address to a VERIFIERTYPE that indicates the
                * result of executing a NEW
                */
                Vfy_push(Vfy_createVerifierTypeForNewAt(ip));

               /*
                * Mark the new instruction as existing. This is used later to check
                * that all the stackmap ITEM_new entries were valid.
                */
                Vfy_markNewInstruction(ip, codeLength);

               /*
                * Advance the IP to next instruction
                */
                ip += 3;
                break;
            }

            case NEWARRAY: {
                CLASSKEY typeKey;

               /*
                * Get the pool tag and convert it a VERIFIERTYPE
                */
                POOLTAG tag = Vfy_getUByte(ip + 1);
                switch (tag) {
                    case T_BOOLEAN: typeKey = Vfy_getBooleanArrayVerifierType(); break;

                    case T_CHAR:    typeKey = Vfy_getCharArrayVerifierType();    break;
#if IMPLEMENTS_FLOAT
                    case T_FLOAT:   typeKey = Vfy_getFloatArrayVerifierType();   break;

                    case T_DOUBLE:  typeKey = Vfy_getDoubleArrayVerifierType();  break;
#endif
                    case T_BYTE:    typeKey = Vfy_getByteArrayVerifierType();    break;

                    case T_SHORT:   typeKey = Vfy_getShortArrayVerifierType();   break;

                    case T_INT:     typeKey = Vfy_getIntArrayVerifierType();     break;

                    case T_LONG:    typeKey = Vfy_getLongArrayVerifierType();    break;

                    default:        Vfy_throw(VE_BAD_INSTR);                     break;
                }

               /*
                * Pop the length and push the array type
                */
                Vfy_pop(ITEM_Integer);
                Vfy_push(typeKey);

               /*
                * Advance the IP to next instruction
                */
                ip += 2;
                break;
            }

            case ANEWARRAY: {
                CLASSKEY     arrayKey;
                VERIFIERTYPE arrayType;

               /*
                * Get the pool index and check it is CONSTANT_Class
                */
                POOLINDEX index = Vfy_getUShort(ip + 1);
                Pol_checkTagIsClass(vPool, index);

               /*
                * Get the CLASSKEY
                */
                arrayKey = Pol_getClassKey(vPool, index);

               /*
                * Convert the CLASSKEY to a VERIFIERTYPE of an array of typeKey elements
                */
                arrayType = Vfy_getClassArrayVerifierType(arrayKey);

               /*
                * Pop the length and push the array type
                */
                Vfy_pop(ITEM_Integer);
                Vfy_push(arrayType);

               /*
                * Advance the IP to next instruction
                */
                ip += 3;
                break;
            }

            case ARRAYLENGTH: {

               /*
                * Pop some kind of array
                */
                VERIFIERTYPE arrayType = Vfy_pop(Vfy_getObjectVerifierType());

               /*
                * Check that this is an array of at least one dimension
                */
                if (!Vfy_isArrayOrNull(arrayType)) {
                    Vfy_throw(VE_EXPECT_ARRAY);
                }

               /*
                * Push the integer result and advance the IP to the next instruction
                */
                Vfy_push(ITEM_Integer);
                ip += 1;
                break;

            }

            case CHECKCAST: {
                CLASSKEY typeKey;

               /*
                * Get the pool index and check it is CONSTANT_Class
                */
                POOLINDEX index = Vfy_getUShort(ip + 1);
                Pol_checkTagIsClass(vPool, index);

               /*
                * Pop the type casting from and push the type casting to
                */
                typeKey = Pol_getClassKey(vPool, index);
                Vfy_pop(Vfy_getObjectVerifierType());
                Vfy_push(Vfy_toVerifierType(typeKey));

               /*
                * Advance the IP to next instruction
                */
                ip += 3;
                break;

            }

            case INSTANCEOF: {

               /*
                * Get the pool index and check it is CONSTANT_Class
                */
                POOLINDEX index = Vfy_getUShort(ip + 1);
                Pol_checkTagIsClass(vPool, index);

               /*
                * Pop the type casting from and push a boolean (int)
                */
                Vfy_pop(Vfy_getObjectVerifierType());
                Vfy_push(ITEM_Integer);

               /*
                * Advance the IP to next instruction
                */
                ip += 3;
                break;

            }

            case MONITORENTER:
            case MONITOREXIT: {

               /*
                * Pop some kind of object and advance the IP to next instruction
                */
                Vfy_pop(Vfy_getObjectVerifierType());
                ip++;
                break;

            }

            case MULTIANEWARRAY: {
                CLASSKEY typeKey;
                int dim, i;

               /*
                * Get the pool index and check it is CONSTANT_Class
                */
                POOLINDEX index = Vfy_getUShort(ip + 1);
                Pol_checkTagIsClass(vPool, index);

               /*
                * Get the CLASSKEY and the number of dimensions
                */
                typeKey = Pol_getClassKey(vPool, index);
                dim = Vfy_getUByte(ip + 3);

               /*
                * Check that the number of dimensions is not zero and that
                * the number specified in the array type is at least as great
                */
                if (dim == 0 || !Vfy_isArrayClassKey(typeKey, dim)) {
                    Vfy_throw(VE_MULTIANEWARRAY);
                }

               /*
                * Pop all the dimension sizes
                */
                for (i = 0; i < dim; i++) {
                    Vfy_pop(ITEM_Integer);
                }

               /*
                * Push the array type and advance the IP to next instruction
                */
                Vfy_push(typeKey);
                ip += 4;
                break;
            }

            case ATHROW: {
                Vfy_pop(Vfy_getThrowableVerifierType());
                ip++;
                noControlFlow = TRUE;
                break;
            }

            case WIDE: {
                SLOTINDEX index;
                switch (Vfy_getUByte(ip + 1)) {

                    case IINC: {
                        index = Vfy_getUShort(ip + 2);
                        ip += 6;
                        Vfy_getLocal(index, ITEM_Integer);
                        Vfy_setLocal(index, ITEM_Integer);
                        break;
                    }

                    case ILOAD: {
                        index = Vfy_getUShort(ip + 2);
                        ip += 4;
                        Vfy_getLocal(index, ITEM_Integer);
                        Vfy_push(ITEM_Integer);
                        break;
                    }

                    case ALOAD: {
                        CLASSKEY refType;
                        index = Vfy_getUShort(ip + 2);
                        ip += 4;
                        refType = Vfy_getLocal(index, ITEM_Reference);
                        Vfy_push(refType);
                        break;
                    }

                    case LLOAD: {
                        index = Vfy_getUShort(ip + 2);
                        ip += 4;
                        Vfy_getLocal(index,     ITEM_Long);
                        Vfy_getLocal(index + 1, ITEM_Long_2);
                        Vfy_push(ITEM_Long);
                        Vfy_push(ITEM_Long_2);
                        break;
                    }

                    case ISTORE: {
                        index = Vfy_getUShort(ip + 2);
                        ip += 4;
                        Vfy_pop(ITEM_Integer);
                        Vfy_setLocal(index, ITEM_Integer);
                        break;
                    }

                    case ASTORE: {
                        CLASSKEY arrayElementType;
                        index = Vfy_getUShort(ip + 2);
                        ip += 4;
                        arrayElementType = Vfy_pop(ITEM_Reference);
                        Vfy_setLocal(index, arrayElementType);
                        break;
                    }

                    case LSTORE: {
                        index = Vfy_getUShort(ip + 2);
                        ip += 4;
                        Vfy_pop(ITEM_Long_2);
                        Vfy_pop(ITEM_Long);
                        Vfy_setLocal(index + 1, ITEM_Long_2);
                        Vfy_setLocal(index, ITEM_Long);
                        break;
                    }

#if IMPLEMENTS_FLOAT

                    case FLOAD: {
                        index = Vfy_getUShort(ip + 2);
                        ip += 4;
                        Vfy_getLocal(index, ITEM_Float);
                        Vfy_push(ITEM_Float);
                        break;

                    }

                    case DLOAD: {
                        index = Vfy_getUShort(ip + 2);
                        ip += 4;
                        Vfy_getLocal(index, ITEM_Double);
                        Vfy_getLocal(index + 1, ITEM_Double_2);
                        Vfy_push(ITEM_Double);
                        Vfy_push(ITEM_Double_2);
                        break;

                    }

                    case FSTORE: {
                        index = Vfy_getUShort(ip + 2);
                        ip += 4;
                        Vfy_pop(ITEM_Float);
                        Vfy_setLocal(index, ITEM_Float);
                        break;

                    }

                    case DSTORE: {
                        index = Vfy_getUShort(ip + 2);
                        ip += 4;
                        Vfy_pop(ITEM_Double_2);
                        Vfy_pop(ITEM_Double);
                        Vfy_setLocal(index + 1, ITEM_Double_2);
                        Vfy_setLocal(index, ITEM_Double);
                        break;
                    }

#endif /* IMPLEMENTS_FLOAT */

                    default: {
                        Vfy_throw(VE_BAD_INSTR);
                    }
                }
                break;
            }

            default: {
                Vfy_throw(VE_BAD_INSTR);
                break;
            }
        } /* End of switch statement */

        /* Make sure that there is no exception handler whose start or
         * end is between original_ip and ip, because that would be 
         * an illegal value
         */
        if (handlerCount > 0 && ip > original_ip + 1) {
            int i;
            for (i = 0 ; i < handlerCount ; i++) {
                IPINDEX startPC = Mth_getExceptionTableStartPC(vMethod, i);
                IPINDEX endPC   = Mth_getExceptionTableEndPC(vMethod, i);
                if (   (startPC > original_ip && startPC < ip)
                    || (endPC > original_ip && endPC < ip)) {
                    Vfy_throw(VE_BAD_EXCEPTION_HANDLER_RANGE); 
                }
            }
        }
    }

    /* Make sure that the bytecode offset within 
     * stackmaps does not exceed code size
     */

    if (Mth_checkStackMapOffset(vMethod, currentStackMapIndex) == 0) {
        Vfy_throw(VE_BAD_STACKMAP);
    }

    /* Make sure that control flow does not fall through 
     * the end of the method. 
     */

    if (ip > codeLength) {
        Vfy_throw(VE_MIDDLE_OF_BYTE_CODE);
    }

    if (!noControlFlow) {
        Vfy_throw(VE_FALL_THROUGH);
    }

    /* All done */
}

