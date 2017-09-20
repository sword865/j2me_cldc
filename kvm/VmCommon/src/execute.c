/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Bytecode interpreter
 * FILE:      execute.c
 * OVERVIEW:  This file defines the interpreter execution loop.
 *            The interpreter was completely restructured for
 *            KVM 1.0.2.  This file defines the heart of the
 *            interpreter, while the actual bytecode definitions
 *            are located in file bytecodes.c.
 * AUTHOR:    Major reorganization by Nik Shaylor 9/5/2000
 *=======================================================================*/

/*=========================================================================
 * Local include files
 *=======================================================================*/

#include <global.h>
#include <execute.h>

#if IMPLEMENTS_FLOAT
#include <fp_math.h>
#endif

/*=========================================================================
 * Redefine the Virtual Machine global registers
 *=======================================================================*/

#define ip ip_global
#define fp fp_global
#define sp sp_global
#define lp lp_global
#define cp cp_global

#undef  getIP
#define getIP()  (ip)
#undef  getFP
#define getFP()  (fp)
#undef  getSP
#define getSP()  (sp)
#undef  getLP
#define getLP()  (lp)
#undef  getCP
#define getCP()  (cp)

#undef  setIP
#define setIP(x) (ip = (x))
#undef  setFP
#define setFP(x) (fp = (x))
#undef  setSP
#define setSP(x) (sp = (x))
#undef  setLP
#define setLP(x) (lp = (x))
#undef  setCP
#define setCP(x) (cp = (x))

/*=========================================================================
 * Instrumentation counters
 *=======================================================================*/

#if INSTRUMENT
long calls;
long reshed;
long bytecodes;
long slowcodes;
long branches;
#endif

/*************************************************************************
 *                  Start of the "secondary" interpreter                 *
 *************************************************************************/

OBJECT thisObjectGCSafe = NULL;

#if SPLITINFREQUENTBYTECODES

/*=========================================================================
 * VMSAVE - Save the VM state
 *=======================================================================*/

#define VMSAVE    /**/

/*=========================================================================
 * VMRESTORE - Restore the VM state
 *=======================================================================*/

#define VMRESTORE /**/

/*=========================================================================
 * TOKEN - Macro to get the current bytecode
 *=======================================================================*/

#define TOKEN token

/*=========================================================================
 * FUNCTION:      SlowInterpret()
 * OVERVIEW:      This is the "secondary" interpreter that executes
 *                "infrequently" used Java bytecodes.  By separating
 *                the infrequent bytecodes to a separate routine,
 *                the C compiler can often do a better job in 
 *                optimizing the primary interpreter loop (FastInterpret).
 * INTERFACE:
 *   parameters:  The bytecode to be executed
 *   returns:     <nothing>
 *
 * NOTES:
 * This function is called by the main Interpret() loop when the
 * main loop does not contain the code for a specific bytecode.
 * There are two reasons for this. First, the main loop runs using
 * local copies of the main interpreter variables (ip, sp, etc.) and
 * when a routine needs to be called that requires access to these
 * variables via global memory, then the local copies are written
 * back to their global counterpoints, and this routine is called
 * to re-dispatch the bytecode in global context. Second, any
 * infrequently executed bytecodes are normally put here to make
 * the main interpreter loop as small as possible -- by doing this
 * the C compiler will often optimize the main loop better.
 *=======================================================================*/

void SlowInterpret(ByteCode token) {
    METHOD   thisMethod;
    OBJECT   thisObject;
    int      invokerSize;

#if NEED_LONG_ALIGNMENT || NEED_DOUBLE_ALIGNMENT
    Java8 tdub;
#endif

    INC_SLOWCODES

    switch (token) {

/*=======================================================================*/
/* Include bytecode definitions that we need for secondary interpreter   */
/*=======================================================================*/
/* Note: There is some serious macro-hackery here.  The actual           */
/* Java bytecode definitions are in file 'bytecodes.c'.  The macro       */
/* definitions below determine which bytecodes from 'bytecodes.c         */
/* get included into the secondary interpreter.                          */
/*=======================================================================*/

#if !ALTERNATIVE_FAST_INTERPRETER
#define STANDARDBYTECODES 0
#define FLOATBYTECODES    IMPLEMENTS_FLOAT
#define FASTBYTECODES     0
#define INFREQUENTSTANDARDBYTECODES 1
#else
#define STANDARDBYTECODES 0
#define FLOATBYTECODES    IMPLEMENTS_FLOAT
#define FASTBYTECODES     ENABLEFASTBYTECODES
#define INFREQUENTSTANDARDBYTECODES 1
#endif

#include "bytecodes.c"

#undef STANDARDBYTECODES
#undef FLOATBYTECODES
#undef FASTBYTECODES
#undef INFREQUENTSTANDARDBYTECODES

/*=======================================================================*/

#if COMMONBRANCHING
        branchPoint: {
            INC_BRANCHES
            ip += getShort(ip + 1);
            goto reschedulePoint;
        }
#endif

        callMethod_interface: {
            invokerSize = 5;        /* Size of the bytecode */
            goto callMethod_general;
        }

        callMethod_virtual:
        callMethod_static:
        callMethod_special:
            invokerSize = 3;        /* Size of the bytecode */

        callMethod_general: {

            INC_CALLS

            /* Check if the method is a native method */
            if (thisMethod->accessFlags & ACC_NATIVE) {
                ip += invokerSize;
                invokeNativeFunction(thisMethod);
                TRACE_METHOD_EXIT(thisMethod)
                goto reschedulePoint;
            }

            /* Check if this is an abstract method */
            if (thisMethod->accessFlags & ACC_ABSTRACT) {
                raiseExceptionWithMessage(AbstractMethodError, methodName(thisMethod));
            }

            /* Create an execution frame for executing a Java method */
            thisObjectGCSafe = thisObject;
            pushFrame(thisMethod);

            /* Advance to the next instruction on return */
            fp->previousIp += invokerSize;

            /* Check if this is a synchronized method */
            if (thisMethod->accessFlags & ACC_SYNCHRONIZED) {
                monitorEnter(thisObjectGCSafe);
                fp->syncObject = thisObjectGCSafe;
            }

            thisObjectGCSafe = NULL;
            goto reschedulePoint;
        }

        handleNullPointerException: {
            raiseException(NullPointerException);
            goto reschedulePoint;
        }

        handleArrayIndexOutOfBoundsException: {
            raiseException(ArrayIndexOutOfBoundsException);
            goto reschedulePoint;
        }

        handleArithmeticException: {
            raiseException(ArithmeticException);
            goto reschedulePoint;
        }

        handleArrayStoreException: {
            raiseException(ArrayStoreException);
            goto reschedulePoint;
        }

        handleClassCastException: {
            raiseException(ClassCastException);
            goto reschedulePoint;
        }

#if PADTABLE
        notImplemented:
#endif

        default: {
            sprintf(str_buffer, KVM_MSG_ILLEGAL_BYTECODE_1LONGPARAM,
                    (long)token);
            fatalError(str_buffer);
            break;
        }

    }

    fatalError(KVM_MSG_SLOWINTERPRETER_STOPPED);

next3:  ip++;
next2:  ip++;
next1:  ip++;
next0:
reschedulePoint:
    return;
}

/*=========================================================================
 * Undefine the macros we needed just for this function
 *=======================================================================*/

#undef VMSAVE
#undef VMRESTORE
#undef TOKEN

#endif /* SPLITINFREQUENTBYTECODES */

/*************************************************************************
 *                  End of the "secondary" interpreter                   *
 *************************************************************************/

/*************************************************************************
 *                 Start of the primary interpreter loop                  *
 *************************************************************************/

/*=========================================================================
 * VMSAVE - Save the VM state
 *=======================================================================*/

#define VMSAVE  {               \
    SAVEIP                      \
    SAVEFP                      \
    SAVESP                      \
    SAVELP                      \
    SAVECP                      \
}

/*=========================================================================
 * VMRESTORE - Restore the VM state
 *=======================================================================*/

#define VMRESTORE {             \
    RESTOREIP                   \
    RESTOREFP                   \
    RESTORESP                   \
    RESTORELP                   \
    RESTORECP                   \
}

/*=========================================================================
 * TOKEN - Macro to get the current bytecode
 *=======================================================================*/

#if ENABLE_JAVA_DEBUGGER
#define TOKEN token
#else
#define TOKEN (*ip)
#endif

/*=========================================================================
 * FUNCTION:      FastInterpret()
 * OVERVIEW:      This is the primary interpreter loop.  All the most 
 *                commonly needed bytecodes are defined here.  To 
 *                improve performance, we instruct the C compiler 
 *                to put the VM registers into hardware registers.
 *                The primary interpreter loop can be replaced by
 *                an alternative assembly interpreter loop in some
 *                KVM versions.
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *
 * NOTES:         Remember to initialize all virtual machine registers
 *                and the call stack before entering the interpreter.
 *=======================================================================*/

#if !ALTERNATIVE_FAST_INTERPRETER
void FastInterpret() {

   /*
    * Define as locals those machine registers that are appropriate.
    * Typically you get a lot of bang for the buck simply by making
    * ip and sp local, but if you can make all the VM registers
    * (ip, sp, lp, fp and cp) local, that's even better.
    *
    * Note: whenever you exit the primary interpreter loop,
    * you MUST call the VMSAVE-VMRESTORE operations to make
    * sure that the rest of the VM can use the correct values
    * of the VM registers!!
    */

#if IPISLOCAL
#undef ip
    register BYTE* ip; /* Instruction pointer (program counter) */
#endif

#if SPISLOCAL
#undef sp
    register cell* sp; /* Execution stack pointer */
#endif

#if LPISLOCAL
#undef lp
    register cell* lp; /* Local variable pointer */
#endif

#if FPISLOCAL
#undef fp
    register FRAME fp; /* Current frame pointer */
#endif

#if CPISLOCAL
#undef cp
    register CONSTANTPOOL  cp; /* Constant pool pointer */
#endif

#if ENABLE_JAVA_DEBUGGER
    register BYTE token;
#endif

   /*
    * Define other local variables needed by the interpreter
    */
    METHOD   thisMethod;
    OBJECT   thisObject;
    int      invokerSize;
    const char *exception;

#if NEED_LONG_ALIGNMENT || NEED_DOUBLE_ALIGNMENT
    Java8 tdub;
#endif

    VMRESTORE  /** Restore virtual machine registers to local variables **/

    goto reschedulePoint;

/*************************************************************************
 *              Top of the actual primary interpreter loop               *
 *************************************************************************/

   /*
    * If RESCHEDULEATBRANCH is not defined then we always test for thread
    * scheduling before every bytecode. This is like in KVM 1.0.
    */

#if !RESCHEDULEATBRANCH
next3:  ip++;
next2:  ip++;
next1:  ip++;
next0:
reschedulePoint:
    RESCHEDULE
#endif

   /*
    * If RESCHEDULEATBRANCH is defined then we only test for thread
    * scheduling when reschedulePoint is called.
    */

#if RESCHEDULEATBRANCH
reschedulePoint:
    RESCHEDULE
#if ENABLE_JAVA_DEBUGGER
    goto next0a;
#else
    goto next0;
#endif

next3:  ip++;
next2:  ip++;
next1:  ip++;
next0:
#if ENABLE_JAVA_DEBUGGER
    token = *ip;
    /*
     * Be careful, there is a goto in this following macro that jumps back
     * up to a label in the RESCHEDULE macro;  ugly, I know
     */
    __doSingleStep()
next0a:
#endif
#endif /* RESCHEDULEATBRANCH */

   /*
    * Profile the instruction
    */
    INSTRUCTIONPROFILE

   /*
    * Trace the instruction here if the option is enabled
    */
    INSTRUCTIONTRACE

   /*
    * Increment the bytecode counter
    */
    INC_BYTECODES

   /*
    *  Extreme debug option to call the garbage collector before every bytecode
    */
    DO_VERY_EXCESSIVE_GARBAGE_COLLECTION

   /*
    * Dispatch the bytecode
    */
#if ENABLE_JAVA_DEBUGGER
    switch (token) {
#else
    switch (((unsigned char)*ip)) {
#endif

/*=======================================================================*/
/* Include bytecode definitions that we need for main interpreter loop   */
/*=======================================================================*/
/* Note: There is some serious macro-hackery here.  The actual           */
/* Java bytecode definitions are in file 'bytecodes.c'.  The macro       */
/* definitions below determine which bytecodes from 'bytecodes.c         */
/* get included into the primary interpreter loop.                       */
/*=======================================================================*/

#define STANDARDBYTECODES 1
#define FASTBYTECODES     ENABLEFASTBYTECODES

#if SPLITINFREQUENTBYTECODES
#define INFREQUENTSTANDARDBYTECODES 0
#define FLOATBYTECODES    0
#else
#define INFREQUENTSTANDARDBYTECODES 1
#define FLOATBYTECODES    IMPLEMENTS_FLOAT
#endif

#include "bytecodes.c"

#undef STANDARDBYTECODES
#undef FLOATBYTECODES
#undef FASTBYTECODES
#undef INFREQUENTSTANDARDBYTECODES

/*=======================================================================*/

   /*
    * If COMMONBRANCHING is defined then include the code to load ip
    * See the BRANCHIF macro
    */
#if COMMONBRANCHING
        branchPoint: {
            INC_BRANCHES
            ip += getShort(ip + 1);
            goto reschedulePoint;
        }
#endif

        callMethod_interface:
            invokerSize = 5;       /* Size of the bytecode */
            goto callMethod_general;

        callMethod_virtual:
        callMethod_static:
        callMethod_special:
            invokerSize = 3;       /* Size of the bytecode */

        callMethod_general: {

            INC_CALLS

            /* Check if the method is a native method */
            if (thisMethod->accessFlags & ACC_NATIVE) {
                ip += invokerSize;
                VMSAVE
                invokeNativeFunction(thisMethod);
                VMRESTORE
                TRACE_METHOD_EXIT(thisMethod);
                goto reschedulePoint;
            }

            /* Check if this is an abstract method */
            if (thisMethod->accessFlags & ACC_ABSTRACT) {
                VMSAVE
                raiseExceptionWithMessage(AbstractMethodError, methodName(thisMethod));
                VMRESTORE
            }

            thisObjectGCSafe = thisObject;
            VMSAVE
            pushFrame(thisMethod);
            VMRESTORE

            /* Advance to the next instruction on return */
            fp->previousIp += invokerSize;

            /* Check if this is a synchronized method */
            if (thisMethod->accessFlags & ACC_SYNCHRONIZED) {
                VMSAVE
                monitorEnter(thisObjectGCSafe);
                VMRESTORE
                fp->syncObject = thisObjectGCSafe;
            }

            thisObjectGCSafe = NULL;
            goto reschedulePoint;
        }

#if SPLITINFREQUENTBYTECODES
        callSlowInterpret: {
            int __token = TOKEN;
            VMSAVE
            SlowInterpret(__token);
            VMRESTORE
            goto reschedulePoint;        /* test for rescheduling */
        }
#endif

        handleNullPointerException: {
            exception = NullPointerException;
            goto handleException;
        }

        handleArrayIndexOutOfBoundsException: {
            exception = ArrayIndexOutOfBoundsException;
            goto handleException;
        }

        handleArithmeticException: {
            exception = ArithmeticException;
            goto handleException;
        }

        handleArrayStoreException: {
            exception = ArrayStoreException;
            goto handleException;
        }

        handleClassCastException: {
            exception = ClassCastException;
            goto handleException;
        }

        handleException: {
            VMSAVE
            raiseException(exception);
            VMRESTORE
            goto reschedulePoint;
        }

#if PADTABLE
        notImplemented:
#endif

        default: {
            sprintf(str_buffer, KVM_MSG_ILLEGAL_BYTECODE_1LONGPARAM,
                    (long)TOKEN);
            fatalError(str_buffer);
            break;
        }
    }
    fatalError(KVM_MSG_INTERPRETER_STOPPED);
}

#endif /* ALTERNATIVE_FAST_INTERPRETER */

/*************************************************************************
 *                  End of the primary interpreter loop                  *
 *************************************************************************/

/*=========================================================================
 * FUNCTION:      Interpret()
 * OVERVIEW:      Top-level bytecode interpreter.  Calls the primary
 *                interpreter loop, and optionally prints out 
 *                instrumentation results.
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

void Interpret() {

    /* When the VM has handled an exception, it returns here */
startTry:

    CurrentNativeMethod = NULL;

    TRY {

        START_TEMPORARY_ROOTS
            IS_TEMPORARY_ROOT(thisObjectGCSafe, NULL);
            FastInterpret();
        END_TEMPORARY_ROOTS

    } CATCH (e) {

        /*
         * Exceptions in the FastInterpret() loop must be thrown within 
         * a VMSAVE/VMRESTORE pair.  Otherwise, throwException below
         * will not to get the right values for virtual machine registers
         */
        START_TEMPORARY_ROOTS
            IS_TEMPORARY_ROOT(e, e);
            throwException(&e);
        END_TEMPORARY_ROOTS

    } END_CATCH_AND_GOTO(startTry)

#if INSTRUMENT
    fprintf(stdout,"bytecodes      =\t%ld\n", bytecodes);
    fprintf(stdout,"slowcodes      =\t%ld\t(%ld)%%\n", slowcodes, slowcodes/(bytecodes/100));
    fprintf(stdout,"calls          =\t%ld\t(%ld)%%\n", calls,         calls/(bytecodes/100));
    fprintf(stdout,"branches taken =\t%ld\t(%ld)%%\n", branches,   branches/(bytecodes/100));
    fprintf(stdout,"rescheduled    =\t%ld\t(%ld)%%\n", reshed,       reshed/(bytecodes/100));
#endif /* INSTRUMENT */
}

