/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Bytecode interpreter
 * FILE:      interpret.c
 * OVERVIEW:  This file defines the general routines used by the
 *            Java bytecode interpreter.
 * AUTHOR:    Antero Taivalsaari, Sun Labs, 1998
 *            Major reorganization by Nik Shaylor 9/5/2000
 * NOTE:      In KVM 1.0.2, the interpreter has been restructured for
 *            better performance, but without sacrificing portability.
 *            The high-level interpreter loop is now defined in file
 *            execute.c.  Actual bytecode definitions are in file
 *            bytecodes.c.  Various high-level compilation flags
 *            for the interpreter have been documented in main.h 
 *            and in the KVM Porting Guide.
 *=======================================================================*/

/*=========================================================================
 * Local include files
 *=======================================================================*/

#include <global.h>

/*=========================================================================
 * Virtual machine global registers (see description in Interpreter.h)
 *=======================================================================*/

struct GlobalStateStruct GlobalState;

#define ip ip_global
#define fp fp_global
#define sp sp_global
#define lp lp_global
#define cp cp_global

/*=========================================================================
 * Bytecode table for debugging purposes (included
 * only if certain tracing modes are enabled).
 *=======================================================================*/

#if INCLUDEDEBUGCODE
const char* const byteCodeNames[] = BYTE_CODE_NAMES;
#endif /* INCLUDEDEBUGCODE */

/*=========================================================================
 * Interpreter tracing & profiling functions
 *=======================================================================*/

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
 * FUNCTION:      printRegisterStatus()
 * TYPE:          public debugging operation
 * OVERVIEW:      Print all the VM registers of the currently executing
 *                thread for debugging purposes.
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

#if INCLUDEDEBUGCODE
void printRegisterStatus() {
    cell* i;

    fprintf(stdout,"VM status:\n");
    fprintf(stdout,"Instruction pointer.: %lx ", (long)ip);
    if (fp)
        fprintf(stdout, "(offset within invoking method: %d)",
                ip - fp->thisMethod->u.java.code);
    if (ip) {
        fprintf(stdout, "\nNext instruction....: 0x%x", *ip);
#if INCLUDEDEBUGCODE
        if (tracebytecodes) {
            fprintf(stdout, " (%s)", getByteCodeName(*ip));
        }
#endif        
    }

    fprintf(stdout,"\nFrame pointer.......: %lx\n", (long)fp);
    fprintf(stdout,"Local pointer.......: %lx\n", (long)lp);

    if (CurrentThread != NULL) {
        STACK stack;
        int size = 0;
        for (stack = CurrentThread->stack; stack != NULL; stack = stack->next) {
            size += stack->size;
        }

        fprintf(stdout,"Stack size..........: %d; sp: %lx; ranges: ",
                size, (long)sp);
        for (stack = CurrentThread->stack; stack != NULL; stack = stack->next) {
            fprintf(stdout, "%lx-%lx;", (long)stack->cells,
                    (long)(stack->cells + stack->size));
        }
        fprintf(stdout, "\n");
    }

    if (fp) {
        fprintf(stdout,"Contents of the current stack frame:\n");
        for (i = lp; i <= sp; i++) {
        fprintf(stdout,"    %lx: %lx", (long)i, (long)*i);
            if (i == lp) fprintf(stdout," (lp)");
            if (i == (cell*)fp) fprintf(stdout," (fp)");
            if (i == (cell*)fp + SIZEOF_FRAME - 1) {
                fprintf(stdout," (end of frame)");
            }
            if (i == sp) fprintf(stdout," (sp)");
            fprintf(stdout,"\n");
        }
    }
}
#endif /* INCLUDEDEBUGCODE */

/*=========================================================================
 * FUNCTION:      printVMstatus()
 * TYPE:          public debugging operation
 * OVERVIEW:      Print all the execution status of the VM
 *                for debugging purposes.
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

#if INCLUDEDEBUGCODE
void printVMstatus() {
    printStackTrace();
    printRegisterStatus();
    printExecutionStack();
    printProfileInfo();
}
#endif /* INCLUDEDEBUGCODE */

/*=========================================================================
 * FUNCTION:      checkVMStatus()
 * TYPE:          public debugging operation (consistency checking)
 * OVERVIEW:      Ensure that VM registers are pointing to meaningful
 *                locations and that stacks have not over/underflown.
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 * NOTE:          This operation is only called when profiling
 *                is enabled.
 *=======================================================================*/

#if INCLUDEDEBUGCODE && ENABLEPROFILING
static void checkVMstatus() {
    STACK stack;
    int valid;

    if (CurrentThread == NIL) {
        return;
    }

    /* Catch stack over/underflows */
    for (valid = 0, stack = CurrentThread->stack; stack; stack = stack->next) {
        if (STACK_CONTAINS(stack, sp)) {
            valid = 1;
            break;
        }
    }
    if (!valid) {
        fatalVMError(KVM_MSG_STACK_POINTER_CORRUPTED);
    }

    /* Frame pointer should always be within stack range */
    for (valid = 0, stack = CurrentThread->stack; stack; stack = stack->next) {
        if (STACK_CONTAINS(stack, (cell*)fp) && (cell*)fp <= sp) {
            valid = 1;
            break;
        }
    }
    if (!valid) {
        fatalVMError(KVM_MSG_FRAME_POINTER_CORRUPTED);
    }

    /* Locals pointer should always be within stack range */
    for (valid = 0, stack = CurrentThread->stack; stack; stack = stack->next) {
        if (STACK_CONTAINS(stack, lp) && lp <= sp) {
            valid = 1;
            break;
        }
    }
    if (!valid) {
        fatalVMError(KVM_MSG_LOCALS_POINTER_CORRUPTED);
    }

    /* Check the number of active threads; this should always be > 1 */
    if (!areActiveThreads()) { 
        fatalVMError(KVM_MSG_ACTIVE_THREAD_COUNT_CORRUPTED);
   }
}
#else

#define checkVMstatus()

#endif /* INCLUDEDEBUGCODE && ENABLEPROFILING */

/*=========================================================================
 * FUNCTION:      fatalSlotError()
 * TYPE:          public debugging operation (consistency checking)
 * OVERVIEW:      Report missing field/method and exit
 * INTERFACE:
 *   parameters:  CONSTANTPOOL constantPool, int cpIndex
 *   returns:     <nothing>
 *   throws:      NoSuchMethodError or NoSuchFieldError
 *                (not supported by CLDC 1.0 or 1.1)
 *=======================================================================*/

void fatalSlotError(CONSTANTPOOL constantPool, int cpIndex) {
    CONSTANTPOOL_ENTRY thisEntry = &constantPool->entries[cpIndex];
    unsigned char thisTag   = CONSTANTPOOL_TAG(constantPool, cpIndex);

    if (thisTag & CP_CACHEBIT) {
        if (thisTag == CONSTANT_Fieldref) {
            sprintf(str_buffer, KVM_MSG_NO_SUCH_FIELD_2STRPARAMS,
                    fieldName((FIELD)(thisEntry->cache)),
                    getFieldSignature((FIELD)thisEntry->cache));
        } else {
            sprintf(str_buffer, KVM_MSG_NO_SUCH_METHOD_2STRPARAMS,
                    methodName((METHOD)thisEntry->cache),
                    getMethodSignature((METHOD)thisEntry->cache));
        }
    } else {
        int nameTypeIndex       = thisEntry->method.nameTypeIndex;
        NameTypeKey nameTypeKey = 
            constantPool->entries[nameTypeIndex].nameTypeKey;
        NameKey nameKey         = nameTypeKey.nt.nameKey;
        MethodTypeKey typeKey   = nameTypeKey.nt.typeKey;

        if (thisTag == CONSTANT_Fieldref) {
            sprintf(str_buffer, KVM_MSG_NO_SUCH_FIELD_2STRPARAMS,
                    change_Key_to_Name(nameKey, NULL),
                    change_Key_to_FieldSignature(typeKey));
        } else {
            sprintf(str_buffer, KVM_MSG_NO_SUCH_METHOD_2STRPARAMS,
                    change_Key_to_Name(nameKey, NULL),
                    change_Key_to_MethodSignature(typeKey));
        }
    }

    if (thisTag == CONSTANT_Fieldref) {
        raiseExceptionWithMessage(NoSuchFieldError, str_buffer);
    } else {
        raiseExceptionWithMessage(NoSuchMethodError, str_buffer);
    }
}

/*=========================================================================
 * FUNCTION:      fatalIcacheMethodError()
 * TYPE:          public debugging operation (consistency checking)
 * OVERVIEW:      Report unable to find appropriate method for
 *                  invokevirtual_fat, invokeinterface_fast
 * INTERFACE:
 *   parameters:  Icache entry containing method
 *   returns:     <nothing>
 *=======================================================================*/

void fatalIcacheMethodError(ICACHE thisICache) {
    METHOD thisMethod = (METHOD)thisICache->contents;
    sprintf(str_buffer, KVM_MSG_NO_SUCH_METHOD_2STRPARAMS,
            methodName(thisMethod),
            getMethodSignature(thisMethod));
    fatalError(str_buffer);
}

/*=========================================================================
 * FUNCTION:      InstructionProfile()
 * OVERVIEW:      Profile an instruction
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

#if ENABLEPROFILING

void InstructionProfile() {

#if INCLUDEDEBUGCODE
    /* Check that the VM is doing ok */
    checkVMstatus();
#endif /*INCLUDEDEBUGCODE*/

    /* Increment the instruction counter */
    /* for profiling purposes */
    InstructionCounter++;
}
#endif /* ENABLEPROFILING */

/*=========================================================================
 * FUNCTION:      InstructionTrace()
 * OVERVIEW:      Trace an instruction
 * INTERFACE:
 *   parameters:  instruction pointer
 *   returns:     <nothing>
 *=======================================================================*/

#if INCLUDEDEBUGCODE

void InstructionTrace(BYTE *traceip) {
    int token = *traceip;

    ASSERTING_NO_ALLOCATION

        /* Print stack contents */
        {
            /* mySP points to just the first stack word. */
            cell *mySP = (cell *)((char *)fp + sizeof(struct frameStruct));
            fprintf(stdout, "       Operands (%ld items): ", 
                    (long)(sp - mySP) + 1);
            for ( ; mySP <= sp; mySP++) {
                fprintf(stdout, "%lx, ", *mySP);
            }
            fprintf(stdout, "\n");
        }

#if (TERSE_MESSAGES)
        fprintf(stdout, "    %ld: %s\n",
                (long)((traceip) - fp_global->thisMethod->u.java.code),
                getByteCodeName(token));
#else /* TERSE_MESSAGES */
        { 
            char buffer[256];
            getClassName_inBuffer(((CLASS)fp->thisMethod->ofClass), buffer);
            fprintf(stdout, "    %d: %s (in %s::%s)\n", 
                    (traceip) - fp->thisMethod->u.java.code,
                    getByteCodeName(token), buffer, methodName(fp->thisMethod));
        }
#endif /* TERSE_MESSAGES */

    END_ASSERTING_NO_ALLOCATION
}

#endif /* INCLUDEDEBUGCODE */

