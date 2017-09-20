/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Stackmap based pointer calculation
 * FILE:      stackmap.c
 * OVERVIEW:  This file defines stackmap calculation operations
 *            that are needed by the exact garbage collector.
 * AUTHOR:    Frank Yellin
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>

/*=========================================================================
 * Definitions and declarations
 *=======================================================================*/

#define BIT_SET(bit)   map[(bit) >> 3] |= (((unsigned)1) << ((bit) & 7))
#define BIT_CLR(bit)   map[(bit) >> 3] &= ~(((unsigned)1) << ((bit) & 7))

/*=========================================================================
 * Static variables and functions (private to this file)
 *=======================================================================*/

static unsigned int
getInitialRegisterMask(METHOD, unsigned int *offsetP, char *map);

static void
setBits(char *map, unsigned int bit, unsigned int count, unsigned char *values);

static void
getBits(char *map, unsigned int bit, unsigned int count, unsigned char *results);

#if INCLUDEDEBUGCODE

static void
printStackMap(long offset, const char *string,
              unsigned int localsCount, char *map, int stackSize);

#endif /* INCLUDEDEBUGCODE */

/*=========================================================================
 * Functions
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      getGCRegisterMask
 * TYPE:          Important GC function
 * OVERVIEW:      Returns a bitmap indicating the live size of the stack
 * INTERFACE:
 *   parameters:  thisMethod:  The current method
 *                targetIP:    An IP somewhere in the code.  We want to find
 *                             the register state just before this instruction
 *                map:         A bitmap to be filled in
 *   returns:     The size of the stack before the indicated instruction
 *                is executed.
 *
 * NOTE: The important side effect of this function is that it fills in
 * map with a bitmap indicating the locals and stack that contain pointers.
 * The first thisMethod->frameSize bits are the locals.  The <returnValue>
 * bits following are the state of the stack.
 *=======================================================================*/

static unsigned int
getInitialRegisterMask(METHOD, unsigned int *targetOffset, char *map);

unsigned int
getGCRegisterMask(METHOD thisMethod, unsigned char *targetIP,  char *map)
{
    CONSTANTPOOL cp = NULL;
    unsigned char *code = NULL;
    unsigned int codeOffset = 0;
    unsigned int localsCount = 0;
    unsigned int stackSize = 0;
    unsigned char *thisIP = NULL;
    unsigned char dupValues[6];

    ByteCode token;
    unsigned int index = 0;
    METHOD method;
    FIELD field;

#if INCLUDEDEBUGCODE
    int junk = 0;

    if (tracestackmaps) {
        junk = (printMethodName(thisMethod, stdout), 0);
    }
#endif

    cp = thisMethod->ofClass->constPool;
    code = thisMethod->u.java.code;
    codeOffset = targetIP - code;
    localsCount = thisMethod->frameSize;

    stackSize =
        /* This modifies codeOffset to be a location less than or
         * equal to its original value. */
        getInitialRegisterMask(thisMethod, &codeOffset, map);

    thisIP = code + codeOffset;

    /* Our calculations are much simplified if we let stackSize also
     * include the localsCount bits at the beginning of the stack map.
     */
    stackSize += localsCount;

#if INCLUDEDEBUGCODE
    if (tracestackmaps) {
        printStackMap(thisIP - code, "Start", localsCount, map, stackSize);
    }
    junk++;                     /* Avoid a compiler warning */
#endif

    while (thisIP < targetIP) {
#if INCLUDEDEBUGCODE
    unsigned char *initialIP=NULL;
    if (tracestackmaps) {
        initialIP = thisIP;
    }    
#endif
        /* Get the next token, and dispatch on it. */
     token = *thisIP++;

#if ENABLE_JAVA_DEBUGGER
again:
#endif

        switch(token) {
            /* Store a single non pointer from the stack to a register */
            case ISTORE: case FSTORE:
                index = *thisIP++;
            storeInt:
                BIT_CLR(index);
                stackSize--;
                break;

            /* Store a single pointer from the stack to a register */
            case ASTORE:
                index = *thisIP++;
            storePointer:
                BIT_SET(index);
                stackSize--;
                break;

            /* Store a double or long from the stack to a register */
            case LSTORE: case DSTORE:
                index = *thisIP++;
            storeDouble:
                BIT_CLR(index);
                BIT_CLR(index + 1);
                stackSize -= 2;
                break;

            case ISTORE_0: case ISTORE_1: case ISTORE_2: case ISTORE_3:
                index = token - ISTORE_0;
                goto storeInt;

            case LSTORE_0: case LSTORE_1: case LSTORE_2: case LSTORE_3:
                index = token - LSTORE_0;
                goto storeDouble;

            case FSTORE_0: case FSTORE_1: case FSTORE_2: case FSTORE_3:
                index = (token - FSTORE_0);
                goto storeInt;

            case DSTORE_0: case DSTORE_1: case DSTORE_2: case DSTORE_3:
                index = (token - DSTORE_0);
                goto storeDouble;

            case ASTORE_0: case ASTORE_1: case ASTORE_2: case ASTORE_3:
                index = (token - ASTORE_0);
                goto storePointer;

                /* These leave any pointers on the stack as pointers, and
                 * any nonpointers on the stack as nonpointers */
            case GETFIELDP_FAST: /* Ptr => Ptr */
            case IINC:
            case CHECKCAST: case CHECKCAST_FAST:
                thisIP += 2;
            case NOP:
            case INEG: case LNEG: case FNEG: case DNEG:
            case I2F:  case L2D:  case F2I:  case D2L:
            case I2B:  case I2C:  case I2S:
                break;

                /* These push a non-pointer onto the stack */
            case SIPUSH:
            case GETSTATIC_FAST:
                thisIP++;
            case ILOAD:  case FLOAD:  case BIPUSH:
                thisIP++;
            case ACONST_NULL:
            case ICONST_M1: case ICONST_0: case ICONST_1:
            case ICONST_2:  case ICONST_3: case ICONST_4: case ICONST_5:
            case FCONST_0:  case FCONST_1: case FCONST_2:
            case ILOAD_0:   case ILOAD_1:  case ILOAD_2:  case ILOAD_3:
            case FLOAD_0:   case FLOAD_1:  case FLOAD_2:  case FLOAD_3:
            case I2L: case I2D: case F2L: case F2D:
            pushInt:
                BIT_CLR(stackSize);
                stackSize++;
                break;

                /* These push two non-pointers onto the stack */
            case GETSTATIC2_FAST: case LDC2_W:
                thisIP++;
            case LLOAD:    case DLOAD:
                thisIP++;
            case LCONST_0: case LCONST_1: case DCONST_0: case DCONST_1:
            case LLOAD_0:  case LLOAD_1:  case LLOAD_2:  case LLOAD_3:
            case DLOAD_0:  case DLOAD_1:  case DLOAD_2:  case DLOAD_3:
            pushDouble:
                BIT_CLR(stackSize);
                stackSize++;
                BIT_CLR(stackSize);
                stackSize++;
                break;

                /* These push a pointer onto the stack */
            case NEW: case NEW_FAST:
            case GETSTATICP_FAST:
                thisIP++;
            case ALOAD:
                thisIP++;
            case ALOAD_0:  case ALOAD_1:  case ALOAD_2: case ALOAD_3:
            pushPointer:
                BIT_SET(stackSize);
                stackSize++;
                break;

                /* These pop an item off the stack */
            case IFEQ: case IFNE: case IFLT: case IFGE:
            case IFGT: case IFLE: case IFNULL: case IFNONNULL:
            case PUTSTATIC_FAST:
                thisIP += 2;
            case POP:  case IADD: case FADD: case ISUB: case FSUB:
            case IMUL: case FMUL: case IDIV: case FDIV: case IREM:
            case FREM: case ISHL: case LSHL: case ISHR: case LSHR:
            case IUSHR: case LUSHR:
            case IAND: case IOR: case IXOR:
            case L2I:  case L2F: case D2I: case D2F:
            case FCMPL: case FCMPG:
            case MONITORENTER: case MONITOREXIT:
            case AALOAD:        /* Ptr Int => Ptr */
                stackSize--;
                break;

                /* These pop an item off the stack, and then push a pointer */
            case ANEWARRAY:
            case ANEWARRAY_FAST:
                thisIP++;
            case NEWARRAY:
                thisIP++;
                stackSize--;
                goto pushPointer;

                /* These pop an item off the stack, and then push an int */
            case INSTANCEOF:
            case INSTANCEOF_FAST:
            case GETFIELD_FAST:
                thisIP += 2;
            case ARRAYLENGTH:
                stackSize--;
                goto pushInt;

            /* These pop an item off the stack, and then put two nonptrs */
            case GETFIELD2_FAST:
                thisIP += 2;
                BIT_SET(stackSize-1);
                BIT_SET(stackSize);
                stackSize--;
                goto pushDouble;

                /* These pop two items off the stack */
            case IF_ICMPEQ: case IF_ICMPNE: case IF_ICMPLT: case IF_ICMPGE:
            case IF_ICMPGT: case IF_ICMPLE: case IF_ACMPEQ: case IF_ACMPNE:
            case PUTFIELD_FAST:
            case PUTSTATIC2_FAST:
                thisIP += 2;
            case POP2:
            case LADD: case DADD: case LSUB: case DSUB: case LMUL: case DMUL:
            case LDIV: case DDIV: case LREM: case DREM:
            case LAND: case LOR: case LXOR:
                stackSize -= 2;
                break;

                /* These pop two items off, and then push non-pointer */
            case IALOAD: case FALOAD: case BALOAD: case CALOAD: case SALOAD:
                stackSize -= 2;
                goto pushInt;

                /* These pop two items off, and then push two non pointers */
            case DALOAD: case LALOAD:
                stackSize -= 2;
                goto pushDouble;

                /* These pop three items off the stack. */
            case PUTFIELD2_FAST:
                thisIP += 2;
            case IASTORE: case FASTORE: case AASTORE: case BASTORE:
            case CASTORE: case SASTORE:
            case LCMP:    case DCMPL:   case DCMPG:
                stackSize -= 3;
                break;

                /* These pop four items off the stack. */
            case LASTORE:
            case DASTORE:
                stackSize -= 4;
                break;

                /* These either load a pointer or an integer */
            case LDC:
                index = *thisIP++;
                goto handleLoadConstant;
            case LDC_W:
                index = getUShort(thisIP);
                thisIP += 2;
            handleLoadConstant:
                if (CONSTANTPOOL_TAG(cp, index) == CONSTANT_String) {
                    goto pushPointer;
                } else {
                    goto pushInt;
                }

                /* These involve doing bit twiddling */
            case DUP:
                getBits(map, stackSize - 1, 1, dupValues);
                if (dupValues[0]) {
                    goto pushPointer;
                } else {
                    goto pushInt;
                }

            case DUP_X1:
                getBits(map, stackSize - 2, 2, dupValues + 1);
                dupValues[0] = dupValues[2];
                setBits(map, stackSize - 2, 3, dupValues);
                stackSize++;
                break;

            case DUP_X2:
                getBits(map, stackSize - 3, 3, dupValues + 1);
                dupValues[0] = dupValues[3];
                setBits(map, stackSize - 3, 4, dupValues);
                stackSize++;
                break;

            case DUP2:
                getBits(map, stackSize - 2, 2, dupValues);
                setBits(map, stackSize, 2, dupValues);
                stackSize += 2;
                break;

            case DUP2_X1:
                getBits(map, stackSize - 3, 3, dupValues + 2);
                dupValues[0] = dupValues[3];
                dupValues[1] = dupValues[4];
                setBits(map, stackSize - 3, 5, dupValues);
                stackSize  += 2;
                break;

            case DUP2_X2:
                getBits(map, stackSize - 4, 4, dupValues + 2);
                dupValues[0] = dupValues[4];
                dupValues[1] = dupValues[5];
                setBits(map, stackSize - 4, 6, dupValues);
                stackSize += 2;
                break;

            case SWAP:
                getBits(map, stackSize - 2, 2, dupValues + 1);
                dupValues[0] = dupValues[2];
                setBits(map, stackSize - 2, 2, dupValues);
                break;

                /* Get a field.  We don't know what type of object we
                 * are getting.
                 */
            case GETFIELD:
                /* Remove the pointer to the object */
                stackSize -= 1;
            case GETSTATIC:
                index = getUShort(thisIP);
                thisIP += 2;

                if (CONSTANTPOOL_TAG(cp, index) & CP_CACHEBIT) {
                    field = (FIELD)cp->entries[index].cache;
                } else {
                    fatalError(KVM_MSG_EXPECTED_RESOLVED_FIELD);
                }

#if INCLUDEDEBUGCODE
                if (tracestackmaps) {
                    fprintf(stdout, "\t\t");
                    printFieldName(field, stdout);
                }
#endif

                /* Determine what needs to be pushed onto the stack,
                 * on the basis of the flags in the field */
                if (field->accessFlags & ACC_POINTER) {
                    goto pushPointer;
                } else if (field->accessFlags & ACC_DOUBLE) {
                    goto pushDouble;
                } else {
                    goto pushInt;
                }

                /* Set a field value from the stack */
            case PUTFIELD:
                /* Remove the pointer to the object */
                stackSize -= 1;
            case PUTSTATIC:
                index = getUShort(thisIP);
                thisIP += 2;

                if (CONSTANTPOOL_TAG(cp, index) & CP_CACHEBIT) {
                    field = (FIELD)cp->entries[index].cache;
                } else {
                    fatalError(KVM_MSG_EXPECTED_RESOLVED_FIELD);
                }

#if INCLUDEDEBUGCODE
                if (tracestackmaps) {
                    fprintf(stdout, "\t\t");
                    printFieldName(field, stdout);
                }
#endif

                /* We are either removing a single word or two words.  We
                 * don't care if it is a pointer or not.
                 */
                stackSize -= (field->accessFlags & ACC_DOUBLE) ? 2 : 1;
                break;

                /* We remove thisIP[2] dimensions, and then we push
                 * the result */
            case MULTIANEWARRAY:
            case MULTIANEWARRAY_FAST:
                stackSize -= thisIP[2];
                thisIP += 3;
                goto pushPointer;

            case WIDE:
                token = *thisIP++;
                index = getUShort(thisIP); thisIP += 2;
                switch (token) {
                    case ILOAD: case FLOAD:
                        goto pushInt;
                    case LLOAD: case DLOAD:
                        goto pushDouble;
                    case ALOAD:
                        goto pushPointer;
                    case LSTORE: case DSTORE:
                        goto storeDouble;
                    case ISTORE: case FSTORE:
                        goto storeInt;
                    case ASTORE:
                        goto storePointer;
                    case IINC:
                        thisIP += 2; break;
                    default:    /* RET shouldn't happen! */
                        fatalError(KVM_MSG_UNEXPECTED_BYTECODE);
                        break;
                }
                break;

#if ENABLEFASTBYTECODES
            case INVOKEVIRTUAL_FAST:
            case INVOKEINTERFACE_FAST:
            {
                unsigned int iCacheIndex = getUShort(thisIP);
                ICACHE thisICache = getInlineCache(iCacheIndex);
                method = (METHOD)thisICache->contents;
                thisIP += (token == INVOKEINTERFACE_FAST) ? 4 : 2;
                goto handleMethodCall;
            }
#endif

#if ENABLEFASTBYTECODES
            case INVOKESPECIAL_FAST:
            case INVOKESTATIC_FAST:
#endif
            case INVOKEVIRTUAL:
            case INVOKESPECIAL:
            case INVOKESTATIC:
            case INVOKEINTERFACE: {
                unsigned int cpIndex = getUShort(thisIP);
                CONSTANTPOOL_ENTRY thisEntry = &cp->entries[cpIndex];
                thisIP += (token == INVOKEINTERFACE) ? 4 : 2;

                if (CONSTANTPOOL_TAG(cp, cpIndex) & CP_CACHEBIT) {
                    method = (METHOD)thisEntry->cache;
                } else {
                    fatalError(KVM_MSG_EXPECTED_RESOLVED_METHOD);
                }
           }

#if ENABLEFASTBYTECODES
            handleMethodCall:
#endif
#if INCLUDEDEBUGCODE
                if (tracestackmaps) {
                    fprintf(stdout, "\t\t");
                    printMethodName(method, stdout);
                }
#endif
                /* Remove the arguments. */
                stackSize -= method->argCount;
                if (INCLUDEDEBUGCODE && stackSize < localsCount) {
                    fatalError(KVM_MSG_ARGUMENT_POPPING_FAILED);
                }
                /* Push the return result */
                switch (method->accessFlags & (ACC_POINTER | ACC_DOUBLE)) {
                    case ACC_DOUBLE:
                        goto pushDouble;
                    case 0:     /* int */
                        goto pushInt;
                    case ACC_POINTER | ACC_DOUBLE:   /* void */
                        break;
                    case ACC_POINTER:
                        goto pushPointer;
                }
                break;

#if ENABLE_JAVA_DEBUGGER
            case BREAKPOINT:
                token = getVerifierBreakpointOpcode(thisMethod, (unsigned short)((thisIP - 1) - code));
                goto again;
#endif

            default:
                fatalError(KVM_MSG_UNEXPECTED_BYTECODE);
        }

        if (stackSize < localsCount) {
            fatalError(KVM_MSG_ILLEGAL_STACK_SIZE);
        }

#if INCLUDEDEBUGCODE
        if (tracestackmaps) {
            extern const char* const byteCodeNames[];
            printStackMap(initialIP - thisMethod->u.java.code,
                          byteCodeNames[*initialIP],
                          localsCount, map, stackSize);
        }
#endif

    } /* end of while */

    if (thisIP > targetIP) {
        fatalError(KVM_MSG_STRANGE_VALUE_OF_THISIP);
    }

    return stackSize - localsCount;
}

/*=========================================================================
 * FUNCTION:      getInitialRegisterMask
 * TYPE:          Important GC function
 * OVERVIEW:      Returns a bitmap indicating the live size of the stack
 * INTERFACE:
 *   parameters:  thisMethod:    The current method
 *                targetOffsetP: Pointer to offset into the method for which
 *                               we want a stack map
 *                map            Map to fill in with the stack map information.
 *
 *   returns:     The stack size.  It also fills in *map and *targetOffsetP.
 *
 * This function will determine a stack map either for *targetOffsetP or for
 * some offset before *targetOffsetP.  It will update *targetOffsetP with the
 * offset that it actually knows about.
 *
 * This function has two sources of knowing a stack map for a particular
 * location.
 *      the method's stackmap field.
 *      the method's signature, if there is no stackmap, or all stackmaps
 *                 are for an address after the current one.
 *=======================================================================*/

/* Helper functions. . . . */

static void
getRegisterMaskFromMethodSignature(METHOD thisMethod, char *map);

static unsigned int
getInitialRegisterMask(METHOD thisMethod,
                       unsigned int *targetOffsetP, char *map)
{
    unsigned int localsCount = thisMethod->frameSize; /* number of locals */
    unsigned int maxStack = thisMethod->u.java.maxStack;
    unsigned int targetOffset = *targetOffsetP;
    struct stackMapEntryStruct *firstFrame, *thisFrame, *lastFrame;

    STACKMAP stackMaps = thisMethod->u.java.stackMaps.pointerMap;

    /* Zero out the map to start with, so we only need to set
     * appropriate bits    */
    memset(map, 0, (localsCount + maxStack + 7) >> 3);

    if (stackMaps != NULL) {
        int entryCount = stackMaps->nEntries;
        int mask;
        if (entryCount & STACK_MAP_SHORT_ENTRY_FLAG) {
            entryCount &= STACK_MAP_ENTRY_COUNT_MASK;
            mask = STACK_MAP_SHORT_ENTRY_OFFSET_MASK;
        } else {
            mask = ~0;          /* Use whole word */
        }

        /* Find the first stack map frame with an entry >>larger<< than
         * the offset we are seeking */
        firstFrame = &stackMaps->entries[0];
        lastFrame = firstFrame + entryCount;
        for (thisFrame = firstFrame; ; thisFrame++) {
            if (thisFrame == lastFrame ||
                          targetOffset < (thisFrame->offset & mask)) {
                if (thisFrame == firstFrame) {
                    /* We're smaller than the first offset, so just fall
                     * through to getting the frame from the signature */
                    break;
                }
                thisFrame -= 1;
                *targetOffsetP = thisFrame->offset & mask;
                if (mask == -1) {
                    int stackmapLength;
                    char *stackmap =
                        change_Key_to_Name(thisFrame->stackMapKey,
                                           &stackmapLength);
                    /* The first byte is the size of the stack.  The
                       remaining bytes can just be copied over.
                    */
                    memcpy(map, stackmap + 1, stackmapLength - 1);
                    return stackmap[0];
                } else {
                    memcpy(map, &thisFrame->stackMapKey, 2);
                    return thisFrame->offset >> 12;
                }
            }
        }
    }

    /* Either no stack map, or else we are less than the first entry in
     * the stackmap.  So we just get the assumed stack map from the
     * signature and offset = 0.
     */
    getRegisterMaskFromMethodSignature(thisMethod, map);
    *targetOffsetP = 0;
    return 0;           /* no stack */
}

static void
getRegisterMaskFromMethodSignature(METHOD thisMethod, char *map)
{
    unsigned char *codedSignature = (unsigned char *)
        change_Key_to_Name(thisMethod->nameTypeKey.nt.typeKey, NULL);
    unsigned char *from = codedSignature;
    int localVar, argCount;

    argCount = *from++;         /* the first byte is the arg count */

    if (thisMethod->accessFlags & ACC_STATIC) {
        localVar = 0;
    } else {
        BIT_SET(0);
        localVar = 1;
    }
    for ( ; argCount > 0; argCount--) {
        unsigned char tag = *from++;
        if (tag == 'L') {
            BIT_SET(localVar);
            localVar++;
            from += 2;
        } else if (tag >= 'A' && tag <= 'Z') {
            localVar += (tag == 'D' || tag == 'J') ? 2 : 1;
        } else {
            BIT_SET(localVar);
            localVar++;
            from++;
        }
    }
}

/*=========================================================================
 * FUNCTION:      rewriteVerifierStackMapsAsPointerMaps
 * TYPE:          Important GC function
 * OVERVIEW:      Converts the stack map from its "verifier" format
 *                to its pointer map format.
 * INTERFACE:
 *   parameters:  thisMethod:    The current method
 *
 * This function converts the stack map from its verifier format (in which
 * we need to know the type of every stack location and local variable) to
 * pointer map format (in which we just need to know what's a pointer and
 * what isn't).
 *
 * This transformation is performed as the last step of verifying a method.
 *=======================================================================*/

STACKMAP
rewriteVerifierStackMapsAsPointerMaps(METHOD thisMethod) {
    STACKMAP result;            /* Holds the result, to be stored in method */
    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(POINTERLIST, verifierStackMaps, 
                               thisMethod->u.java.stackMaps.verifierMap);
        unsigned int localsCount = thisMethod->frameSize;
        unsigned int maxStack = thisMethod->u.java.maxStack;
        long stackMapCount = verifierStackMaps->length;
        long stackMapIndex, i;
        bool_t useLongFormat;
        char tempSpace[8];          /* Almost always enough space */
        char *map = tempSpace;

        unsigned int maxMapLength;

        int resultSize = sizeof(struct stackMapStruct) +  
            (stackMapCount - 1) * sizeof(struct stackMapEntryStruct);

        result = 
            (STACKMAP)callocPermanentObject(ByteSizeToCellSize(resultSize));
        useLongFormat = FALSE;
        for (stackMapIndex = 0; stackMapIndex < stackMapCount; stackMapIndex++){
            short *verifierStackMap =
                (short *)verifierStackMaps->data[stackMapIndex].cellp;
            unsigned short registers = verifierStackMap[0];
            unsigned short stackSize = verifierStackMap[registers + 1];
            unsigned short offset =
                   verifierStackMaps->data[stackMapIndex + stackMapCount].cell;

            if (stackSize > STACK_MAP_SHORT_ENTRY_MAX_STACK_SIZE
                || offset > STACK_MAP_SHORT_ENTRY_MAX_OFFSET
                || (stackSize + localsCount) > 16) {
                   useLongFormat = TRUE;
                   break;
            }
        }

        if (useLongFormat) {
            result->nEntries = (unsigned short)stackMapCount;
            maxMapLength = (localsCount + maxStack + 7 + 8) >> 3;
            if (maxMapLength > sizeof(tempSpace)) {
                IS_TEMPORARY_ROOT(map, mallocBytes(maxMapLength));
            }

        } else {
            result->nEntries = (unsigned short)stackMapCount 
                             | STACK_MAP_SHORT_ENTRY_FLAG;
            maxMapLength = 4;   /* actually, 3 */
        }

        for (stackMapIndex = 0; stackMapIndex < stackMapCount; stackMapIndex++){
            short *verifierStackMap =
                   (short *)verifierStackMaps->data[stackMapIndex].cellp;
            unsigned short offset = (unsigned short)
                 verifierStackMaps->data[stackMapIndex + stackMapCount].cell;
            unsigned short registers, stackSize;

            /* This isn't a duplicate.  So calculate the stack map. */
            memset(map, 0, maxMapLength);

            /* Read the registers and the stack */
            registers = *verifierStackMap++; /* number of registers */
            for (i = 0; i < registers; i++) {
                unsigned short type = *verifierStackMap++;
                if (type > 255 || type == ITEM_InitObject) {
                    BIT_SET(8 + i);
                }
            }
            stackSize = *verifierStackMap++;
            for (i = 0; i < stackSize; i++) {
                unsigned short type = *verifierStackMap++;
                if (type > 255 || type == ITEM_InitObject) {
                    BIT_SET(8 + i + localsCount);
                }
            }

            result->entries[stackMapIndex].offset = offset;
            if (useLongFormat) { 
                map[0] = stackSize;
                /* We can delete all the zero words at the end, as long as we
                 * save at least one word at the end. 
                 */
                for (i = maxMapLength - 1; i > 0; i--) {
                    if (map[i]) break;
                }
                /* Fill in the key */
                result->entries[stackMapIndex].stackMapKey =
                    change_Name_to_Key((CONST_CHAR_HANDLE)&map, 0, i+1);
            } else { 
                result->entries[stackMapIndex].offset = 
                    offset + (stackSize << 12);
                memcpy(&result->entries[stackMapIndex].stackMapKey, map+1, 2);
            }
         }
     END_TEMPORARY_ROOTS
     return result;
}

/*=========================================================================
 * FUNCTION:      getBits, setbits
 * TYPE:          private stack map operations
 * OVERVIEW:      Sets/Gets zero or more bits in a bit map
 * INTERFACE:
 *   parameters:  map:    A table of bits
 *                bit:    The starting index of bits to set
 *                count:  The number of bits to change
 *                result/values: Array of bytes.  For getBits, the result
 *                    is placed into the array.  For setBits, it's the
 *                    value(s) to be put into the bitmap.  Non-zero = TRUE
 *=======================================================================*/

static void
getBits(char *map, unsigned int bit, unsigned int count, unsigned char *result)
{
    int i;
    for (i = 0; i < count; i++, bit++) {
        int offset = bit >> 3;
        int mask = (((unsigned)1) << (bit & 7));
        result[i] = map[offset] & mask;
    }
}

static void
setBits(char *map, unsigned int bit, unsigned int count, unsigned char *values)
{
    int i;
    for (i = 0; i < count; i++, bit++) {
        int offset = bit >> 3;
        int mask = (((unsigned)1) << (bit & 7));
        if (values[i] != 0) {
            map[offset] |= mask;
        } else {
            map[offset] &= ~mask;
        }
    }
}

/*=========================================================================
 * FUNCTION:      printStackMap
 * TYPE:          debugging/printing operation
 * OVERVIEW:      Print a stack map for debugging purposes
 * INTERFACE:
 *=======================================================================*/

#if INCLUDEDEBUGCODE

static void
printStackMap(long offset, const char *string,
              unsigned int localsCount, char *map, int stackSize)
{
    unsigned char x[1];
    int i;
    fprintf(stdout, "%4ld %20s: ", offset, string);
    for (i = 0; i < localsCount; i++) {
        getBits(map, i, 1, x);
        fprintf(stdout,x[0] ? "*" : ".");
    }
    fprintf(stdout, "//");
    for (i = localsCount; i < stackSize; i++) {
        getBits(map, i, 1, x);
        fprintf(stdout,x[0] ? "*" : ".");
    }
    fprintf(stdout, "\n");
}

#endif /* INCLUDEDEBUGCODE */

