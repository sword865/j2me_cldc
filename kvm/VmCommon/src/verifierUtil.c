/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Class file verifier (runtime part)
 * FILE:      verifierUtil.c
 * OVERVIEW:  Implementation-specific parts of the KVM verifier.
 * AUTHORS:   Sheng Liang, Frank Yellin, restructured by Nik Shaylor
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <stddef.h>
#include "verifierUtil.h"

/*=========================================================================
 * Global variables and definitions
 *=======================================================================*/

/*
 * This is always the method being verified. (The verifier cannot be called
 * recursively)
 */
METHOD methodBeingVerified;

/*
 * Pointer to the bytecodes
 */
unsigned char *bytecodesBeingVerified;

/*
 * Holds the return signature
 */
static unsigned char *returnSig;

/*
 * Global variables used by the verifier.
 */
VERIFIERTYPE *vStack;
VERIFIERTYPE *vLocals;
unsigned long *NEWInstructions;
bool_t vNeedInitialization;      /* must call a this.<init> or super.<init> */
unsigned short  vMaxStack;
unsigned short  vFrameSize;
unsigned short  vSP;

#if INCLUDEDEBUGCODE
static int vErrorIp;
#endif

/*=========================================================================
 * Old verifier functions (inherited from KVM 1.0.3)
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      initializeVerifier
 * TYPE:          public global operation
 * OVERVIEW:      Initialize the bytecode verifier
 *
 * INTERFACE:
 *   parameters:  <nothing>
 *   returns:     <nothing>
 *=======================================================================*/

void InitializeVerifier(void) {
}

/*=========================================================================
 * FUNCTION:      vIsAssignable
 * TYPE:          private operation on type keys
 * OVERVIEW:      Check if a value of one type key can be converted to a
 *                value of another type key.
 *
 * INTERFACE:
 *   parameters:  fromKey: from type
 *                toKey: to type
 *                mergedKeyP: pointer to merged type
 *   returns:     TRUE: can convert.
 *                FALSE: cannot convert.
 *=======================================================================*/

bool_t vIsAssignable(VERIFIERTYPE fromKey, VERIFIERTYPE toKey, VERIFIERTYPE *mergedKeyP) {
    if (mergedKeyP) {
        /* Most of the time merged type is toKey */
        *mergedKeyP = toKey;
    }
    if (fromKey == toKey)     /* trivial case */
        return TRUE;
    if (toKey == ITEM_Bogus)
        return TRUE;
    if (toKey == ITEM_Reference) {
        return fromKey == ITEM_Null ||
            fromKey > 255 ||
            fromKey == ITEM_InitObject ||
            (fromKey & ITEM_NewObject_Flag);
    }
    if ((toKey & ITEM_NewObject_Flag) || (fromKey & ITEM_NewObject_Flag)) {
        return FALSE;
    }
    if (fromKey == ITEM_Null && toKey > 255) {
        return TRUE;
    }
    if (fromKey > 255 && toKey > 255) {
        CLASS fromClass = change_Key_to_CLASS(Vfy_toClassKey(fromKey));
        CLASS toClass   = change_Key_to_CLASS(Vfy_toClassKey(toKey));
        bool_t res = isAssignableTo(fromClass, toClass);
        /* Interfaces are treated like java.lang.Object in the verifier. */
        if (toClass->accessFlags & ACC_INTERFACE) {
            /* Set mergedKey to fromKey for interfaces */
            if (mergedKeyP) {
                *mergedKeyP = Vfy_getObjectVerifierType();
            }
            return TRUE;
        }
        return res;
    }
    return FALSE;
}

/*=========================================================================
 * FUNCTION:      vIsProtectedAccess
 * TYPE:          private operation on type keys
 * OVERVIEW:      Check if the access to a method or field is to
 *                a protected field/method of a superclass.
 *
 * INTERFACE:
 *   parameters:  thisClass:   The class in question
 *                index:       Constant pool entry of field or method
 *                isMethod:    true for method, false for field.
 *=======================================================================*/

bool_t vIsProtectedAccess(INSTANCE_CLASS thisClass, POOLINDEX index, bool_t isMethod) {
    CONSTANTPOOL constPool = thisClass->constPool;
    unsigned short memberClassIndex =
        constPool->entries[index].method.classIndex;
    INSTANCE_CLASS memberClass =
        (INSTANCE_CLASS)constPool->entries[memberClassIndex].clazz;
    INSTANCE_CLASS tClass;
    unsigned short nameTypeIndex;
    unsigned long nameTypeKey;

    /* if memberClass isn't a superclass of this class, then we don't worry
     * about this case */
    for (tClass = thisClass; ; tClass = tClass->superClass) {
        if (tClass == NULL) {
            return FALSE;
        } else if (tClass == memberClass) {
            break;
        }
    }

    nameTypeIndex = constPool->entries[index].method.nameTypeIndex;
    nameTypeKey = constPool->entries[nameTypeIndex].nameTypeKey.i;

    while (memberClass != NULL) {
        if (isMethod) {
            FOR_EACH_METHOD(method, memberClass->methodTable)
                if (method->nameTypeKey.i == nameTypeKey) {
                    return (((method->accessFlags & ACC_PROTECTED) != 0)
                          && (memberClass->clazz.packageName !=
                                thisClass->clazz.packageName));
                }
            END_FOR_EACH_METHOD
        } else {
            FOR_EACH_FIELD(field, memberClass->fieldTable)
                if (field->nameTypeKey.i == nameTypeKey) {
                    return (((field->accessFlags & ACC_PROTECTED) != 0)
                          && (memberClass->clazz.packageName !=
                                thisClass->clazz.packageName));
                }
            END_FOR_EACH_FIELD
        }
        memberClass = memberClass->superClass;
    }
    return FALSE;
}

/*=========================================================================
 * FUNCTION:      Vfy_getReferenceArrayElementType
 * TYPE:          private operation on type keys
 * OVERVIEW:      Obtain the element type key of a given object reference type.
 *
 * INTERFACE:
 *   parameters:  typeKey: object array type key.
 *   returns:     element type key.
 *
 * DANGER:
 *   Before calling this function, make sure that typeKey in fact is an
 *   object array.
 *=======================================================================*/

VERIFIERTYPE Vfy_getReferenceArrayElementType(VERIFIERTYPE arrayType) {
    CLASSKEY arrayTypeKey = Vfy_toClassKey(arrayType);
    int n = (arrayTypeKey >> FIELD_KEY_ARRAY_SHIFT);
    if (arrayTypeKey == ITEM_Null) {
        return ITEM_Null;
    } else if (n < MAX_FIELD_KEY_ARRAY_DEPTH) {
        return ((n - 1) << FIELD_KEY_ARRAY_SHIFT) + (arrayTypeKey & 0x1FFF);
    } else {
        CLASS arrayClass           = change_Key_to_CLASS(Vfy_toClassKey(arrayTypeKey));
        const char* baseNameString = arrayClass->baseName->string;
        UString baseName           = getUStringX(&baseNameString, 1, arrayClass->baseName->length - 1);
        UString packageName        = arrayClass->packageName;
        CLASS elemClass            = change_Name_to_CLASS(packageName, baseName);
        return Vfy_toVerifierType(elemClass->key);
    }
}

/*=========================================================================
 * FUNCTION:      Vfy_getClassArrayVerifierType
 * TYPE:          private operation on type keys
 * OVERVIEW:      Obtain the object array type key whose element type is the
 *                given element type key.
 *
 * INTERFACE:
 *   parameters:  typeKey: element type key.
 *   returns:     object array type key.
 *
 * DANGER:
 *   Before calling this function, make sure that typeKey in fact is an
 *   object type.
 *=======================================================================*/

VERIFIERTYPE Vfy_getClassArrayVerifierType(CLASSKEY typeKey) {
    CLASSKEY res;
    int n = (typeKey >> FIELD_KEY_ARRAY_SHIFT);
    if (n < MAX_FIELD_KEY_ARRAY_DEPTH - 1) {
        res = (1 << FIELD_KEY_ARRAY_SHIFT) + typeKey;
    } else {
        CLASS elemClass = change_Key_to_CLASS(typeKey);
        ARRAY_CLASS arrayClass = getObjectArrayClass(elemClass);
        res = arrayClass->clazz.key;
    }
    return Vfy_toVerifierType(res);
}

/*=========================================================================
 * FUNCTION:      Vfy_isArrayClassKey
 * TYPE:          private operation on type keys
 * OVERVIEW:      Check if the given type key denotes an array type of the
 *                given dimension.
 *
 * INTERFACE:
 *   parameters:  typeKey: a type key.
 *                dim: dimension.
 *   returns:     TRUE if the type key is an array of dim dimension, FALSE
 *                otherwise.
 *=======================================================================*/

bool_t Vfy_isArrayClassKey(CLASSKEY typeKey, int dim) {
    if (typeKey == ITEM_Null) {
        return TRUE;
    } else {
        int n = (typeKey >> FIELD_KEY_ARRAY_SHIFT);
        /* n is the dimension of the class.  It has the value
         * 0..MAX_FIELD_KEY_ARRAY_DEPTH, in which the last value means
         * MAX_FIELD_KEY_ARRAY_DEPTH or more. . .
         */
        if (dim <= MAX_FIELD_KEY_ARRAY_DEPTH) {
            return n >= dim;
        } if (n < MAX_FIELD_KEY_ARRAY_DEPTH) {
            /* We are looking for at more than MAX_FIELD_KEY_ARRAY_DEPTH,
             * dimensions, so n needs to be that value. */
            return FALSE;
        } else {
            CLASS arrayClass = change_Key_to_CLASS(typeKey);
            char *baseName = arrayClass->baseName->string;
            /* The first dim characters of baseName must be [ */
            for ( ; dim > 0; --dim) {
                if (*baseName++ != '[') {
                    return FALSE;
                }
            }
            return TRUE;
        }
    }
}

/*=========================================================================
 * FUNCTION:      change_Field_to_StackType
 * TYPE:          private operation on type keys
 * OVERVIEW:      Change an individual field type to a stack type
 *
 * INTERFACE:
 *   parameters:  fieldType: field type
 *                stackTypeP: pointer for placing the corresponding stack type(s)
 *   returns:     number of words occupied by the resulting type.
 *=======================================================================*/

int change_Field_to_StackType(CLASSKEY fieldType, VERIFIERTYPE* stackTypeP) {
    switch (fieldType) {
        case 'I':
        case 'B':
        case 'Z':
        case 'C':
        case 'S':
            *stackTypeP++ = ITEM_Integer;
            return 1;
#if IMPLEMENTS_FLOAT
        case 'F':
            *stackTypeP++ = ITEM_Float;
            return 1;
        case 'D':
            *stackTypeP++ = ITEM_Double;
            *stackTypeP++ = ITEM_Double_2;
            return 2;
#endif
        case 'J':
            *stackTypeP++ = ITEM_Long;
            *stackTypeP++ = ITEM_Long_2;
            return 2;
        default:
            *stackTypeP++ = fieldType;
            return 1;
    }
}

/*=========================================================================
 * FUNCTION:      change_Arg_to_StackType
 * TYPE:          private operation on type keys
 * OVERVIEW:      Change an individual method argument type to a stack type
 *
 * INTERFACE:
 *   parameters:  sigP: pointer to method signature.
 *                type: pointer for placing the corresponding stack type(s)
 *   returns:     number of words occupied by the resulting type.
 *=======================================================================*/

int change_Arg_to_StackType(unsigned char** sigP, VERIFIERTYPE* typeP) {
    unsigned char *sig = *sigP;
    unsigned short hi, lo;

    hi = *sig++;

    if (hi == 'L') {
        hi = *sig++;
        lo = *sig++;
        *sigP = sig;
        return change_Field_to_StackType((unsigned short)((hi << 8) + lo), typeP);
    }
    if (hi < 'A' || hi > 'Z') {
        lo = *sig++;
        *sigP = sig;
        return change_Field_to_StackType((unsigned short)((hi << 8) + lo), typeP);
    }
    *sigP = sig;
    return change_Field_to_StackType(hi, typeP);
}

/*=========================================================================
 * FUNCTION:      getStackType
 * TYPE:          private operation on type keys
 * OVERVIEW:      Get the recorded stack map of a given target ip.
 *
 * INTERFACE:
 *   parameters:  thisMethod: method being verified.
 *                this_ip: current ip (unused for now).
 *                target_ip: target ip (to look for a recored stack map).
 *   returns:     a stack map
 *=======================================================================*/

static unsigned short *getStackMap(METHOD thisMethod, IPINDEX target_ip) {
    POINTERLIST stackMaps = thisMethod->u.java.stackMaps.verifierMap;
    unsigned short i;
    if (stackMaps == NULL) {
        return NULL;
    } else {
        long length = stackMaps->length; /* number of entries */
        for (i = 0; i < length; i++) {
            if (target_ip == stackMaps->data[i + length].cell) {
                return (unsigned short*)stackMaps->data[i].cellp;
            }
        }
    }
    return NULL;
}

/*=========================================================================
 * FUNCTION:      matchStackMap
 * TYPE:          private operation on type keys
 * OVERVIEW:      Match two stack maps.
 *
 * INTERFACE:
 *   parameters:  thisMethod: method being verified.
 *                target_ip: target ip (to look for a recored stack map).
 *                flags: bit-wise or of the SM_* flags.
 *   returns:     TRUE if match, FALSE otherwise.
 *=======================================================================*/

bool_t matchStackMap(METHOD thisMethod, IPINDEX target_ip, int flags) {
    bool_t result = TRUE;  /* Assume result is TRUE */
    bool_t target_needs_initialization;
    unsigned short nstack;
    unsigned short nlocals;
    unsigned short i;

    /* Following is volatile, and will disappear at first GC */
    unsigned short *stackMapBase = getStackMap(thisMethod, target_ip);

#if INCLUDEDEBUGCODE
    if (traceverifier) {
        fprintf(stdout, "Match stack maps (this=%ld, target=%ld)%s%s%s\n",
                (long)vErrorIp, (long)target_ip,
            (flags & SM_CHECK) ? " CHECK" : "",
            (flags & SM_MERGE) ? " MERGE" : "",
            (flags & SM_EXIST) ? " EXIST" : "");
    }
#endif

    if (stackMapBase == NULL) {
#if INCLUDEDEBUGCODE
        if (traceverifier) {
            fprintf(stdout, "No recorded stack map at %ld\n", (long)target_ip);
        }
#endif
        return !(flags & SM_EXIST);
    }

  START_TEMPORARY_ROOTS
    DECLARE_TEMPORARY_ROOT_FROM_BASE(unsigned short*, stackMap,
                                     stackMapBase, stackMapBase);

#if INCLUDEDEBUGCODE
    if (traceverifier) {
        static void printStackMap(METHOD thisMethod, IPINDEX ip);
        printStackMap(thisMethod, target_ip);
    }
#endif

    nlocals = *stackMap++;
    /* We implicitly need to still perform initialization if at least
     * one local or stack variable contains ITEM_InitObject */
    target_needs_initialization = FALSE;

    for (i = 0; i < nlocals; i++) {
        unsigned short ty = *stackMap++;
        unsigned short mergedType = ty;
        if (ty == ITEM_InitObject) { 
            target_needs_initialization = TRUE;
        }
        if ((SM_CHECK & flags) && !vIsAssignable(vLocals[i], ty, &mergedType)) {
            result = FALSE;
            goto done;
        }
        if (SM_MERGE & flags) {
            vLocals[i] = mergedType;
        }
    }
    if (SM_MERGE & flags) {
        for (i = nlocals; i < vFrameSize; i++) {
            vLocals[i] = ITEM_Bogus;
        }
    }

    nstack = *stackMap++;
    if ((SM_CHECK & flags) && nstack != vSP) {
        result = FALSE;
        goto done;
    }
    if (SM_MERGE & flags) {
        vSP = nstack;
    }
    for (i = 0; i < nstack; i++) {
        unsigned short ty = *stackMap++;
        unsigned short mergedType = ty;
        if (ty == ITEM_InitObject) { 
            target_needs_initialization = TRUE;
        }
        if ((SM_CHECK & flags) && !vIsAssignable(vStack[i], ty, &mergedType)) {
            result = FALSE;
            goto done;
        }
        if (SM_MERGE & flags) {
            vStack[i] = mergedType;
        }
    }

    if (methodBeingVerified->nameTypeKey.nt.nameKey 
                                     == initNameAndType.nt.nameKey) {
        if (SM_CHECK & flags) { 
            if (vNeedInitialization && !target_needs_initialization) { 
                /* We still need to perform initialization, but we are
                 * merging into a location that doesn't.
                 */
                result = FALSE;
                goto done;
            }
        }

        if (SM_MERGE & flags) { 
            vNeedInitialization = target_needs_initialization;
        }
    }

 done:
  END_TEMPORARY_ROOTS
  return result;
}

/*=========================================================================
 * FUNCTION:      checkNewObject
 * TYPE:          private operation on type keys
 * OVERVIEW:      Check if uninitialized objects exist on backward branches.
 *
 * INTERFACE:
 *   parameters:  this_ip: current ip
 *                target_ip: branch target ip
 *   returns:     TRUE if no uninitialized objects exist, FALSE otherwise.
 *=======================================================================*/

bool_t checkNewObject(IPINDEX this_ip, IPINDEX target_ip) {
    if (target_ip < this_ip) {
        int i;
        for (i = 0; i < vFrameSize; i++) {
            if (vLocals[i] & ITEM_NewObject_Flag) {
                return FALSE;
            }
        }
        for (i = 0; i < vSP; i++) {
            if (vStack[i] & ITEM_NewObject_Flag) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

/*=========================================================================
 * FUNCTION:      verifyClass
 * TYPE:          public operation on classes.
 * OVERVIEW:      Perform byte-code verification of a given class. Iterate
 *                through all methods.
 *
 * INTERFACE:
 *   parameters:  thisClass: class to be verified.
 *   returns:     0 if verification succeeds, error code if verification
 *                fails.
 *=======================================================================*/

int verifyClass(INSTANCE_CLASS thisClass) {
    static int Vfy_verifyMethod(METHOD vMethod);
    int i;
    int result = 0;
#if USESTATIC
    CONSTANTPOOL cp = thisClass->constPool;
#endif
    if (thisClass->methodTable) {
        if (!checkVerifiedClassList(thisClass)) {
            /* Verify all methods */
            for (i = 0; i < thisClass->methodTable->length; i++) {
                METHOD thisMethod = &thisClass->methodTable->methods[i];

                /* Skip special synthesized methods. */
                if (thisMethod == RunCustomCodeMethod) {
                    continue;
                }
                /* Skip abstract and native methods. */
                if (thisMethod->accessFlags & (ACC_NATIVE | ACC_ABSTRACT)) {
                    continue;
                }

               /*
                * Call the core routine
                */
                result = Vfy_verifyMethod(thisMethod);
                if (result != 0) {
                    break;
                }
            }

            if (result == 0) {
                /* Add this newly verified class to verified class list. */
                appendVerifiedClassList(thisClass);
            }
        }

        /* Rewrite the stack maps */
        for (i = 0; i < thisClass->methodTable->length; i++) {
            METHOD thisMethod = &thisClass->methodTable->methods[i];
            if (thisMethod->u.java.stackMaps.verifierMap != NULL) {
                STACKMAP newStackMap = (result == 0)
                     ? rewriteVerifierStackMapsAsPointerMaps(thisMethod)
                     :  NULL;
#if !USESTATIC
                thisMethod->u.java.stackMaps.pointerMap = newStackMap;
#else
                int offset = (char *)&thisMethod->u.java.stackMaps.pointerMap
                           - (char *)cp;
                modifyStaticMemory(cp, offset, &newStackMap, sizeof(STACKMAP));
#endif
            }
        }
    }
    if (result == 0) {
        thisClass->status = CLASS_VERIFIED;
    } else {
        START_TEMPORARY_ROOTS
            DECLARE_TEMPORARY_ROOT(char*, className, getClassName((CLASS)thisClass));
            thisClass->status = CLASS_ERROR;
            if (className) {
                raiseExceptionWithMessage(VerifyError, className);
            } else {
                raiseException(VerifyError);
            }
        END_TEMPORARY_ROOTS
    }

    return result;
}

/*=========================================================================
 * Debugging and printing operations
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      printStackType, printStackMap
 * TYPE:          debugging operations
 * OVERVIEW:      Print verifier-related information
 *
 * INTERFACE:
 *   parameters:  pretty obvious
 *   returns:
 *=======================================================================*/

#if INCLUDEDEBUGCODE

/* Print a string, and add padding. */
static void
print_str(char *str)
{
    int i;
    fprintf(stdout, "%s", str);
    i = 30 - strlen(str);
    while (i > 0) {
        fprintf(stdout, " ");
        i--;
    }
}

/* Print a type key */
static void
printStackType(METHOD thisMethod, unsigned short key)
{
    char buf[128];
    switch (key) {
    case 0xFFF:
        print_str("-");
        return;
    case ITEM_Bogus:
        print_str("*");
        return;
    case ITEM_Integer:
        print_str("I");
        return;
#if IMPLEMENTS_FLOAT
    case ITEM_Float:
        print_str("F");
        return;
    case ITEM_Double:
        print_str("D");
        return;
#endif
    case ITEM_Long:
        print_str("J");
        return;
    case ITEM_Null:
        print_str("null");
        return;
    case ITEM_InitObject:
        print_str("this");
        return;
    case ITEM_Long_2:
        print_str("J2");
        return;
#if IMPLEMENTS_FLOAT
    case ITEM_Double_2:
        print_str("D2");
        return;
#endif
    default: /* ITEM_Object and ITEM_NewObject */
        if (key & ITEM_NewObject_Flag) {
            sprintf(buf, "new %ld", (long)DECODE_NEWOBJECT(key));
            print_str(buf);
        } else {
            print_str(change_Key_to_FieldSignature(key));
        }
        return;
    }
}

#define MAX(a,b) ((a) > (b) ? (a) : (b))

/* Print the derived stackmap and recorded stackmap (if it exists) at
 * a given ip.
 */
static void
printStackMap(METHOD thisMethod, unsigned short ip)
{
  START_TEMPORARY_ROOTS
    unsigned short* stackMapX = getStackMap(thisMethod, ip);
    DECLARE_TEMPORARY_ROOT_FROM_BASE(unsigned short*, stackMap,
                                     stackMapX, stackMapX);
    int nlocals, nstack, i;
    int lTop;

    for (lTop = vFrameSize; lTop > 0; lTop--) {
        if (vLocals[lTop - 1] != ITEM_Bogus) {
            break;
        }
    }

    fprintf(stdout, "__SLOT__DERIVED_______________________");
    if (stackMap) {
        fprintf(stdout, "RECORDED______________________\n");
    } else {
        fprintf(stdout, "\n");
    }

    nlocals = stackMap ? (*stackMap++) : 0;

    for (i = 0; i < MAX(lTop, nlocals); i++) {
        unsigned short ty;
        if (i < 100) {
            fprintf(stdout, " ");
            if (i < 10) {
                fprintf(stdout, " ");
            }
        }
        fprintf(stdout, "L[%ld]  ", (long)i);
        if (i < lTop) {
            ty = vLocals[i];
        } else {
            ty = 0xFFF;
        }
        printStackType(thisMethod, ty);
        if (stackMap) {
            if (i < nlocals) {
                ty = *stackMap++;
            } else {
                ty = 0xFFF;
            }
            printStackType(thisMethod, ty);
        }
        fprintf(stdout, "\n");
    }

    nstack = stackMap ? (*stackMap++) : 0;

    for (i = 0; i < MAX(vSP, nstack); i++) {
        unsigned short ty;
        if (i < 100) {
            fprintf(stdout, " ");
            if (i < 10) {
                fprintf(stdout, " ");
            }
        }
        fprintf(stdout, "S[%ld]  ", (long)i);
        if (i < vSP) {
            ty = vStack[i];
        } else {
            ty = 0xFFF;
        }
        printStackType(thisMethod, ty);

        if (stackMap) {
            if (i < nstack) {
                ty = *stackMap++;
            } else {
                ty = 0xFFF;
            }
            printStackType(thisMethod, ty);
        }
        fprintf(stdout, "\n");
    }
  END_TEMPORARY_ROOTS
}

#endif /* INCLUDEDEBUGCODE */

/*=========================================================================
 * New verifier functions
 *=======================================================================*/

/* ------------------------------------------------------------------------ *\
 *                              Initialization                              *
\* ------------------------------------------------------------------------ */

/*=========================================================================
 * FUNCTION:      Vfy_initializeLocals
 * TYPE:          private operation
 * OVERVIEW:      Initialize the local variable types from the method
 *                signature
 *
 * INTERFACE:
 *   parameters:  None
 *   returns:     Nothing
 *=======================================================================*/

void Vfy_initializeLocals() {
    /* Fill in the initial derived stack map with argument types. */
    unsigned char *sig = (unsigned char*) change_Key_to_Name(methodBeingVerified->nameTypeKey.nt.typeKey, NULL);

    int nargs = *sig++;
    int i;
    int n;

    n = 0;
    vNeedInitialization = FALSE;
    if (!Mth_isStatic(methodBeingVerified)) {
        /* add one extra argument for instance methods */
        n++;
        if (methodBeingVerified->nameTypeKey.nt.nameKey == initNameAndType.nt.nameKey &&
            methodBeingVerified->ofClass->clazz.key != Vfy_getObjectVerifierType()) {
            vLocals[0] = ITEM_InitObject;
            vNeedInitialization = TRUE;
        } else {
            vLocals[0] = methodBeingVerified->ofClass->clazz.key;
        }
    }
    for (i = 0; i < nargs; i++) {
        n += change_Arg_to_StackType(&sig, &vLocals[n]);
    }

    returnSig = sig;
}

/* ------------------------------------------------------------------------ *\
 *                             General functions                            *
\* ------------------------------------------------------------------------ */

/*=========================================================================
 * FUNCTION:      Vfy_getOpcode
 * TYPE:          private operation
 * OVERVIEW:      Get an opcode from the bytecode array.
 *                If it is a BREAKPOINT then get
 *                the real instruction from the debugger
 *
 * INTERFACE:
 *   parameters:  IPINDEX of the bytecode
 *   returns:     The opcode
 *=======================================================================*/

int Vfy_getOpcode(IPINDEX ip) {
    int opcode = Vfy_getUByte(ip);
#if ENABLE_JAVA_DEBUGGER
        if (opcode == BREAKPOINT) {
                opcode = getVerifierBreakpointOpcode(methodBeingVerified, ip);
        }
#endif
    return opcode;
}

/* ------------------------------------------------------------------------ *\
 *                          Stack state management                          *
\* ------------------------------------------------------------------------ */

static unsigned short vSP_bak;
static VERIFIERTYPE   vStack0_bak;

/*=========================================================================
 * FUNCTION:      Vfy_saveStackState
 * TYPE:          private operation
 * OVERVIEW:      Save and initialize the type stack
 *
 * INTERFACE:
 *   parameters:  None
 *   returns:     Nothing
 *=======================================================================*/

void Vfy_saveStackState() {
    vSP_bak = vSP;
    vStack0_bak = vStack[0];
    vSP = 0;
}

/*=========================================================================
 * FUNCTION:      Vfy_restoreStackState
 * TYPE:          private operation
 * OVERVIEW:      Restore the saved type stack
 *
 * INTERFACE:
 *   parameters:  None
 *   returns:     Nothing
 *=======================================================================*/

void Vfy_restoreStackState() {
    vStack[0] = vStack0_bak;
    vSP = vSP_bak;
}

/* ------------------------------------------------------------------------ *\
 *                              Type checking                               *
\* ------------------------------------------------------------------------ */

/*=========================================================================
 * FUNCTION:      Vfy_getLocal
 * TYPE:          private operation on the virtual local frame
 * OVERVIEW:      Get a type key from the virtual local frame maintained by
 *                the verifier. Performs index check and type check.
 *
 * INTERFACE:
 *   parameters:  index: the local index
 *                typeKey: the type expected.
 *   returns:     The actual type from the slot
 *=======================================================================*/

VERIFIERTYPE Vfy_getLocal(SLOTINDEX index, VERIFIERTYPE typeKey) {
    VERIFIERTYPE k;
    if (index >= vFrameSize) {
        Vfy_throw(VE_LOCALS_OVERFLOW);
    }
    k = vLocals[index];
    if (!vIsAssignable(k, typeKey, NULL)) {
        Vfy_throw(VE_LOCALS_BAD_TYPE);
    }
    return k;
}

/*=========================================================================
 * FUNCTION:      Vfy_setLocal
 * TYPE:          private operation on the virtual local frame
 * OVERVIEW:      Set a type key in the virtual local frame maintained by
 *                the verifier. Performs index check and type check.
 *
 * INTERFACE:
 *   parameters:  index: local index.
 *                typeKey: the supplied type.
 *   returns:     Nothing
 *=======================================================================*/

void Vfy_setLocal(SLOTINDEX index, VERIFIERTYPE typeKey) {
    if (index >= vFrameSize) {
        Vfy_throw(VE_LOCALS_OVERFLOW);
    }

    if (vLocals[index] == ITEM_Long_2
#if IMPLEMENTS_FLOAT
        || vLocals[index] == ITEM_Double_2
#endif
        ) {
        if (index < 1) {
            Vfy_throw(VE_LOCALS_UNDERFLOW);
        }
        vLocals[index - 1] = ITEM_Bogus;
    }

    if (vLocals[index] == ITEM_Long
#if IMPLEMENTS_FLOAT
        || vLocals[index] == ITEM_Double
#endif
        ) {
        if (index >= vFrameSize - 1) {
            Vfy_throw(VE_LOCALS_OVERFLOW);
        }
        vLocals[index + 1] = ITEM_Bogus;
    }
    vLocals[index] = typeKey;
}

/*=========================================================================
 * FUNCTION:      vPushStack
 * TYPE:          private operation on the virtual stack
 * OVERVIEW:      Push a type key onto the virtual stack maintained by
 *                the verifier. Performs stack overflow check.
 *
 * INTERFACE:
 *   parameters:  typeKey: the type to be pushed.
 *   returns:     Nothing
 *=======================================================================*/

void Vfy_push(VERIFIERTYPE typeKey) {
    if (vSP >= vMaxStack) {
        Vfy_throw(VE_STACK_OVERFLOW);
    }
    vStack[vSP++] = typeKey;
}

/*=========================================================================
 * FUNCTION:      Vfy_pop
 * TYPE:          private operation on the virtual stack
 * OVERVIEW:      Pop an item from the virtual stack maintained by
 *                the verifier. Performs stack underflow check and type check.
 *
 * INTERFACE:
 *   parameters:  typeKey: The expected type
 *   returns:     The actual type popped
 *=======================================================================*/

VERIFIERTYPE Vfy_pop(VERIFIERTYPE typeKey) {
    VERIFIERTYPE resultKey;

    if (typeKey == ITEM_DoubleWord || typeKey == ITEM_Category2 || typeKey == ITEM_Category1) {
        fatalError(KVM_MSG_V_BAD_POPSTACK_TYPE);
    }

    if (vSP == 0) { /* vSP is unsigned, See bug 4323211 */
        Vfy_throw(VE_STACK_UNDERFLOW);
    }

    resultKey = vStack[vSP - 1];
    vSP--;

    if (!vIsAssignable(resultKey, typeKey, NULL)) {
        Vfy_throw(VE_STACK_BAD_TYPE);
    }
    return resultKey;
}

/*=========================================================================
 * FUNCTION:      Vfy_popCategory2_secondWord
 * TYPE:          private operation on the virtual stack
 * OVERVIEW:      Pop an the second word of an ITEM_DoubleWord or ITEM_Category2
 *                from the virtual stack maintained by the verifier.
 *                Performs stack underflow check and type check.
 *                (This is always called before vPopStackCategory2_firstWord)
 *
 * INTERFACE:
 *   parameters:  None.
 *   returns:     The actual type popped
 *=======================================================================*/

VERIFIERTYPE Vfy_popCategory2_secondWord() {
    VERIFIERTYPE resultKey;
    if (vSP <= 1) {
        Vfy_throw(VE_STACK_UNDERFLOW);
    }

    resultKey = vStack[vSP - 1];
    vSP--;

    return resultKey;
}

/*=========================================================================
 * FUNCTION:      Vfy_popCategory2_firstWord
 * TYPE:          private operation on the virtual stack
 * OVERVIEW:      Pop an the first word of an ITEM_DoubleWord or ITEM_Category2
 *                from the virtual stack maintained by the verifier.
 *                Performs stack underflow check and type check.
 *
 * INTERFACE:
 *   parameters:  None.
 *   returns:     The actual type popped
 *=======================================================================*/

VERIFIERTYPE Vfy_popCategory2_firstWord() {
    VERIFIERTYPE resultKey;
    if (vSP <= 0) {
        Vfy_throw(VE_STACK_UNDERFLOW);
    }

    resultKey = vStack[vSP - 1];
    vSP--;

   /*
    * The only think known about this operation is that it
    * cannot result in an ITEM_Long_2 or ITEM_Double_2 being
    * popped.
    */

    if ((resultKey == ITEM_Long_2) || (resultKey == ITEM_Double_2)) {
        Vfy_throw(VE_STACK_BAD_TYPE);
    }

    return resultKey;
}

/*=========================================================================
 * FUNCTION:      Vfy_popCategory1
 * TYPE:          private operation on the virtual stack
 * OVERVIEW:      Pop a ITEM_Category1 from the virtual stack maintained by
 *                the verifier. Performs stack underflow check and type check.
 *
 * INTERFACE:
 *   parameters:  None.
 *   returns:     The actual type popped
 *=======================================================================*/

VERIFIERTYPE Vfy_popCategory1() {
    VERIFIERTYPE resultKey;
    if (vSP == 0) { /* vSP is unsigned, See bug 4323211 */
        Vfy_throw(VE_STACK_UNDERFLOW);
    }
    resultKey = vStack[vSP - 1];
    vSP--;

    if (resultKey == ITEM_Integer ||
#if IMPLEMENTS_FLOAT
        resultKey == ITEM_Float ||
#endif
        resultKey == ITEM_Null ||
        resultKey > 255 ||
        resultKey == ITEM_InitObject ||
       (resultKey & ITEM_NewObject_Flag)) {
       /* its okay */
    } else {
        Vfy_throw(VE_STACK_EXPECT_CAT1);
    }
    return resultKey;
}

/*=========================================================================
 * FUNCTION:      Vfy_returnVoid
 * TYPE:          private operation
 * OVERVIEW:      Check that a return is valid for this method. If the
 *                method is <init> make sure that 'this' was initialized
 *
 * INTERFACE:
 *   parameters:  None.
 *   returns:     Nothing.
 *=======================================================================*/

void Vfy_returnVoid() {
    if (returnSig[0] != 'V') {
        Vfy_throw(VE_EXPECT_RETVAL);
    }
    if (methodBeingVerified->nameTypeKey.nt.nameKey 
                       == initNameAndType.nt.nameKey) {
        if (vNeedInitialization) { 
            Vfy_throw(VE_RETURN_UNINIT_THIS);
        }
    }
}

/*=========================================================================
 * FUNCTION:      Vfy_popReturn
 * TYPE:          private operation
 * OVERVIEW:      Check that a return is valid for this method.
 *
 * INTERFACE:
 *   parameters:  returnType: the type to be returned.
 *   returns:     Nothing.
 *=======================================================================*/

void Vfy_popReturn(VERIFIERTYPE returnType) {

    VERIFIERTYPE ty[2];
    unsigned char *sig = returnSig;

    returnType = Vfy_pop(returnType);

    if (sig[0] == 'V') {
        Vfy_throw(VE_EXPECT_NO_RETVAL);
    }
    change_Arg_to_StackType(&sig, ty);
    if (!Vfy_isAssignable(returnType, ty[0])) {
        Vfy_throw(VE_RETVAL_BAD_TYPE);
    }

   /*
    * Is this needed here as well as in Vfy_popReturnVoid()?
    */

    if (methodBeingVerified->nameTypeKey.nt.nameKey == initNameAndType.nt.nameKey) {
        fatalError(KVM_MSG_VFY_UNEXPECTED_RETURN_TYPE);
     /*
     //   if (vLocals[0] == ITEM_InitObject) {
     //       Vfy_throw(VE_RETURN_UNINIT_THIS);
     //   }
     */
    }
}

/*=========================================================================
 * FUNCTION:      Vfy_pushClassKey
 * TYPE:          private operation on type keys
 * OVERVIEW:      Push the equivalent VERIFIERTYPES for CLASSKEY
 *
 * INTERFACE:
 *   parameters:  fieldType: CLASSKEY
 *   returns:     Nothing
 *=======================================================================*/

void Vfy_pushClassKey(CLASSKEY fieldType) {
    switch (fieldType) {
        case 'I':
        case 'B':
        case 'Z':
        case 'C':
        case 'S': {
            Vfy_push(ITEM_Integer);
            break;
        }
#if IMPLEMENTS_FLOAT
        case 'F': {
            Vfy_push(ITEM_Float);
            break;
        }
        case 'D': {
            Vfy_push(ITEM_Double);
            Vfy_push(ITEM_Double_2);
            break;
        }
#endif
        case 'J': {
            Vfy_push(ITEM_Long);
            Vfy_push(ITEM_Long_2);
            break;
        }
        default: {
            Vfy_push(Vfy_toVerifierType(fieldType));
            break;
        }
    }
}

/*=========================================================================
 * FUNCTION:      Vfy_popClassKey
 * TYPE:          private operation on type keys
 * OVERVIEW:      Pop the equivalent VERIFIERTYPES for CLASSKEY
 *
 * INTERFACE:
 *   parameters:  fieldType: CLASSKEY
 *   returns:     Nothing
 *=======================================================================*/

void Vfy_popClassKey(CLASSKEY fieldType) {
    switch (fieldType) {
        case 'I':
        case 'B':
        case 'Z':
        case 'C':
        case 'S': {
            Vfy_pop(ITEM_Integer);
            break;
        }
#if IMPLEMENTS_FLOAT
        case 'F': {
            Vfy_pop(ITEM_Float);
            break;
        }
        case 'D': {
            Vfy_pop(ITEM_Double_2);
            Vfy_pop(ITEM_Double);
            break;
        }
#endif
        case 'J': {
            Vfy_pop(ITEM_Long_2);
            Vfy_pop(ITEM_Long);
            break;
        }
        default: {
            Vfy_pop(Vfy_toVerifierType(fieldType));
            break;
        }
    }
}

/*=========================================================================
 * FUNCTION:      Vfy_setupCalleeContext
 * TYPE:          private operation
 * OVERVIEW:      Pop the arguments of the callee context
 *
 * INTERFACE:
 *   parameters:  Class key for the methodTypeKey
 *   returns:     Nothing.
 *=======================================================================*/

METHODTYPEKEY calleeContext;
unsigned char *sigResult;

void Vfy_setupCalleeContext(METHODTYPEKEY methodTypeKey) {
    calleeContext = methodTypeKey;
}

/*=========================================================================
 * FUNCTION:      Vfy_popInvokeArguments
 * TYPE:          private operation on stack
 * OVERVIEW:      Pop the arguments of the callee context
 *
 * INTERFACE:
 *   parameters:  None
 *   returns:     The number of words popped
 *=======================================================================*/

int Vfy_popInvokeArguments() {

    unsigned char* sig = (unsigned char*)change_Key_to_Name(calleeContext, NULL);
    unsigned char* sig_bak;
    unsigned short nargs;
    int nwords = 0;
    CLASSKEY ty[2];
    int j,k;
    unsigned short i;

    nargs = *sig++;

    sig_bak = sig;

    for (i = 0; i < nargs; i++) {
        nwords += change_Arg_to_StackType(&sig, ty);
    }
    sig = sig_bak;

   /*
    * Are there the required number of words on the stack?
    */
    if (vSP < nwords) {
        Vfy_throw(VE_ARGS_NOT_ENOUGH);
    }
    vSP -= nwords;

    k = 0;
    for (i = 0; i < nargs; i++) {
        int n = change_Arg_to_StackType(&sig, ty);
        for (j = 0; j < n; j++) {
            VERIFIERTYPE arg = vStack[vSP + k];
            if (!vIsAssignable(arg, ty[j], NULL)) {
                Vfy_throw(VE_ARGS_BAD_TYPE);
            }
            k++;
        }
    }

   sigResult = sig;

   return nwords;
}

/*=========================================================================
 * FUNCTION:      Vfy_pushInvokeResult
 * TYPE:          private operation on stack
 * OVERVIEW:      Push the callee's result
 *
 * INTERFACE:
 *   parameters:  None
 *   returns:     Nothing
 *=======================================================================*/

void Vfy_pushInvokeResult() {

   /*
    * Push the result type.
    */
    if (*sigResult != 'V') {
        CLASSKEY ty[2];
        int i;
        int n = change_Arg_to_StackType(&sigResult, ty);
        for (i = 0 ; i < n; i++) {
            Vfy_push(ty[i]);
        }
    }
}

bool_t Vfy_MethodNameStartsWithLeftAngleBracket(METHODNAMEKEY methodNameKey) {
    char *methodName = change_Key_to_Name(methodNameKey, NULL);
    return methodName[0] == '<';
}

bool_t Vfy_MethodNameIsInit(METHODNAMEKEY methodNameKey) {
    return methodNameKey == initNameAndType.nt.nameKey;
}

void Vfy_ReplaceTypeWithType(VERIFIERTYPE fromType, VERIFIERTYPE toType) {
    int i;
    for (i = 0; i < vSP; i++) {
        if (vStack[i] == fromType) {
            vStack[i] = toType;
        }
    }
    for (i = 0; i < Mth_frameSize(methodBeingVerified); i++) {
        if (vLocals[i] == fromType) {
            vLocals[i] = toType;
        }
    }

}

/* ------------------------------------------------------------------------ *\
 *                          Method Accessor Methods                         *
\* ------------------------------------------------------------------------ */

/*=========================================================================
 * FUNCTION:      Mth_getStackMapEntryIP
 * TYPE:          private operation
 * OVERVIEW:      Get the ip address of an entry in the stack map table
 *
 * INTERFACE:
 *   parameters:  vMethod: the method, stackMapIndex: the index
 *   returns:     The ip address of the indexed entry. Or 0x7FFFFFFF if
 *                the index is off the end of the table.
 *=======================================================================*/

int Mth_getStackMapEntryIP(METHOD vMethod, int stackMapIndex) {
    METHOD thisMethod = (METHOD)vMethod;
    POINTERLIST stackMaps = thisMethod->u.java.stackMaps.verifierMap;
    if (stackMaps && stackMapIndex < stackMaps->length) {
        return stackMaps->data[stackMapIndex + stackMaps->length].cell;
    } else {
        return 0x7FFFFFFF;
    }
}

/*=========================================================================
 * FUNCTION:      Mth_checkStackMapOffset
 * TYPE:          private operation
 * OVERVIEW:      Validates the stackmap offset to ensure that it does
 *                not exceed code size. 
 *
 * INTERFACE:
 *   parameters:  vMethod: the method, stackMapIndex: the index
 *   returns:     true if ok, false otherwise. 
 *=======================================================================*/

bool_t Mth_checkStackMapOffset(METHOD vMethod, int stackMapIndex) {
    METHOD thisMethod = (METHOD)vMethod;
    POINTERLIST stackMaps = thisMethod->u.java.stackMaps.verifierMap;

    if (stackMaps && stackMapIndex != stackMaps->length) {
        return FALSE; 
    } 
    return TRUE;
}

/*=========================================================================
 * FUNCTION:      Mth_getExceptionTableLength
 * TYPE:          private operation
 * OVERVIEW:      Get the number of entries in the exception table
 *
 * INTERFACE:
 *   parameters:  vMethod: the method
 *   returns:     The number of entries
 *=======================================================================*/

int Mth_getExceptionTableLength(METHOD vMethod) {
    HANDLERTABLE handlers = vMethod->u.java.handlers;
    if (handlers != 0) {
        return handlers->length;
    } else {
        return 0;
    }
}

/* ------------------------------------------------------------------------ *\
 *                                 Printing                                 *
\* ------------------------------------------------------------------------ */

#if INCLUDEDEBUGCODE

/*=========================================================================
 * FUNCTION:      Vfy_setErrorIp
 * TYPE:          private operation
 * OVERVIEW:      Set the ip address for error printing
 *
 * INTERFACE:
 *   parameters:  ip: the IP address
 *   returns:     Nothing
 *=======================================================================*/

/*
 * Set to ip for error messages
 */
void Vfy_setErrorIp(int ip) {
    vErrorIp = ip;
#if EXCESSIVE_GARBAGE_COLLECTION
    garbageCollect(0);
#endif
}

/*=========================================================================
 * FUNCTION:      getByteCodeName()
 * TYPE:          public debugging operation
 * OVERVIEW:      Given a bytecode value, get the mnemonic name of
 *                the bytecode.
 * INTERFACE:
 *   parameters:  bytecode as an integer
 *   returns:     constant string containing the name of the bytecode
 *=======================================================================*/

#if INCLUDEDEBUGCODE
static const char*
getByteCodeName(ByteCode token) {
    if (token >= 0 && token <= LASTBYTECODE)
         return byteCodeNames[token];
    else return "<INVALID>";
}
#else
#  define getByteCodeName(token) ""
#endif

/*=========================================================================
 * FUNCTION:      Vfy_printVerifyStartMessage
 * TYPE:          private operation
 * OVERVIEW:      Print trace info
 *
 * INTERFACE:
 *   parameters:  vMethod: the method
 *   returns:     Nothing
 *=======================================================================*/

static void Vfy_printVerifyStartMessage(METHOD vMethod) {
    if (traceverifier) {
        START_TEMPORARY_ROOTS
            DECLARE_TEMPORARY_ROOT(char*, className,
                                   getClassName(&(vMethod->ofClass->clazz)));
            unsigned short nameKey = vMethod->nameTypeKey.nt.nameKey;
            unsigned short typeKey = vMethod->nameTypeKey.nt.typeKey;
            DECLARE_TEMPORARY_ROOT(char*, signature,
                                   change_Key_to_MethodSignature(typeKey));
            fprintf(stdout, "Verifying method %s.%s%s\n",
                    className, change_Key_to_Name(nameKey, NULL), signature);
        END_TEMPORARY_ROOTS
    }
}

/*=========================================================================
 * FUNCTION:      Vfy_printVerifyLoopMessage
 * TYPE:          private operation
 * OVERVIEW:      Print trace info
 *
 * INTERFACE:
 *   parameters:  vMethod: the method,ip: the IP address
 *   returns:     Nothing
 *=======================================================================*/

void Vfy_printVerifyLoopMessage(METHOD vMethod, IPINDEX ip) {
    if (traceverifier) {
        unsigned char *code = Mth_getBytecodes(vMethod);
        ByteCode token = code[ip];
        fprintf(stdout, "Display derived/recorded stackmap at %ld:\n", (long)ip);
        printStackMap(vMethod, ip);
        fprintf(stdout, "%ld %s\n", (long)ip, getByteCodeName(token));
    }
}

/*=========================================================================
 * FUNCTION:      Vfy_printVerifyEndMessage
 * TYPE:          private operation
 * OVERVIEW:      Print trace info
 *
 * INTERFACE:
 *   parameters:  vMethod: the method
 *   returns:     Nothing
 *=======================================================================*/

static void Vfy_printVerifyEndMessage(METHOD vMethod, int result) {
    static const char* verifierStatusInfo[] =
        KVM_MSG_VERIFIER_STATUS_INFO_INITIALIZER; /* See messages.h */
    if (result != 0) {
        const int  count = sizeof(verifierStatusInfo)/sizeof(verifierStatusInfo[0]);
        const char *info = (result > 0 && result < count)
                                 ? verifierStatusInfo[result]
                                 : "Unknown problem";
        fprintf(stderr, "Error verifying method ");
        printMethodName(vMethod, stderr);
        fprintf(stderr, "Approximate bytecode offset %ld: %s\n", (long)vErrorIp, info);
    }
}

/*=========================================================================
 * FUNCTION:      Vfy_trace1
 * TYPE:          private operation
 * OVERVIEW:      Print trace info
 *
 * INTERFACE:
 *   parameters:  msg: a printf format string, p1: a parameter
 *   returns:     Nothing
 *=======================================================================*/

void Vfy_trace1(char *msg, long p1) {
    if (traceverifier) {
        fprintf(stdout, msg, p1);
    }
}

#endif /* INCLUDEDEBUGCODE */

 /* ------------------------------------------------------------------------ *\
  *                               Entry / Exit                               *
 \* ------------------------------------------------------------------------ */

 static jmp_buf vJumpBuffer;

 /*
  * Main verity routine
  */

/*=========================================================================
 * FUNCTION:      Vfy_verifyMethod
 * TYPE:          private operation
 * OVERVIEW:      Verify a method
 *
 * INTERFACE:
 *   parameters:  vMethod: the method
 *   returns:     Nothing
 *=======================================================================*/

 static int Vfy_verifyMethod(METHOD vMethod) {

    void Vfy_verifyMethodOrAbort(const METHOD vMethod);
    static bool_t Vfy_checkNewInstructions(METHOD vMethod);
    int res;

#if INCLUDEDEBUGCODE
    Vfy_printVerifyStartMessage(vMethod);
#endif

    START_TEMPORARY_ROOTS

        vMaxStack = vMethod->u.java.maxStack;
        vFrameSize = vMethod->frameSize;
        IS_TEMPORARY_ROOT(vStack,  (VERIFIERTYPE*) callocObject(ByteSizeToCellSize(vMaxStack  * sizeof(VERIFIERTYPE)), GCT_NOPOINTERS));
        IS_TEMPORARY_ROOT(vLocals, (VERIFIERTYPE*) callocObject(ByteSizeToCellSize(vFrameSize * sizeof(VERIFIERTYPE)), GCT_NOPOINTERS));

       /*
        * This bitmap keeps track of all the NEW instructions that we have
        * already seen.
        */
        IS_TEMPORARY_ROOT(NEWInstructions, NULL);

        vSP = 0;                   /* initial stack is empty. */

       /*
        * All errors from Vfy_verifyMethodOrAbort are longjmped here. If no error
        * is thrown then res will be zero from the call to setjmp
        */
        res = setjmp(vJumpBuffer);
        if (res == 0) {
           /*
            * Setup the verification context and call the core function.
            */
            methodBeingVerified    = vMethod;
            bytecodesBeingVerified = Mth_getBytecodes(vMethod);

            Vfy_verifyMethodOrAbort(vMethod);
            if (!Vfy_checkNewInstructions(vMethod)) {
                Vfy_throw(VE_BAD_NEW_OFFSET);
            }
        }

    END_TEMPORARY_ROOTS

#if INCLUDEDEBUGCODE
    Vfy_printVerifyEndMessage(vMethod, res);
#endif

    return res;
}

/*=========================================================================
 * FUNCTION:      Vfy_throw
 * TYPE:          private operation
 * OVERVIEW:      Throw a verification exception
 *
 * INTERFACE:
 *   parameters:  code: the VE_xxxx code
 *   returns:     Does a longjmp back to Vfy_verifyMethod
 *=======================================================================*/

void Vfy_throw(int code) {
     longjmp(vJumpBuffer, code);
 }

/*=========================================================================
 * FUNCTION:      Vfy_checkNewInstructions
 * TYPE:          private operation
 * OVERVIEW:      ITEM_new were all used
 *
 * INTERFACE:
 *   parameters:  thisMethod: the method
 *   returns:     FALSE if there is an error
 *=======================================================================*/

static bool_t Vfy_checkNewInstructions(METHOD thisMethod) {

   /* We need to double check that all of the ITEM_new's that
    * we stack in the stack map were legitimate.
    */
    const int codeLength = thisMethod->u.java.codeLength;
    int i, j;
    POINTERLIST stackMaps = thisMethod->u.java.stackMaps.verifierMap;
    if (stackMaps != NULL) {
        for (i = stackMaps->length; --i >= 0;) {
            unsigned short *stackMap =
                (unsigned short*)stackMaps->data[i].cellp;
            for (j = 0; j < 2; j++) {
                /* Do this loop twice: locals and stack */
                unsigned short count = *stackMap++;
                while (count-- > 0) {
                    unsigned short typeKey = *stackMap++;
                    if (typeKey & ITEM_NewObject_Flag) {
                        int index = DECODE_NEWOBJECT(typeKey);
                        if (index >= codeLength
                               || NEWInstructions == NULL
                               || !IS_NTH_BIT(NEWInstructions, index)) {
                            return FALSE;
                        }
                    }
                }
            }
        }
    }
    return TRUE;
}

