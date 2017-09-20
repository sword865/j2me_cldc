/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Bytecode interpreter
 * FILE:      execute.h
 * OVERVIEW:  This file defines macros for the Java interpreter
 *            execution loop.  These macros are needed by the
 *            redesigned interpreter loop.
 * AUTHOR:    Nik Shaylor 9/5/2000
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

/*=========================================================================
 * Additional build options (standard options are defined in main.h)
 *=======================================================================*/

/* The following options are intended for interpreter debugging
 * and customization.  Normally you should not change these
 * definitions, and that's why they are here instead of
 * main.h where the user-level customization options are.
 */

/* COMMONBRANCHING effects the BRANCHIF macro. The original macro
 * would load the ip in the body of the macro. This option causes a
 * branch to a common place where this is done. There seems no
 * disadvantage to doing it this way.
 */
#ifndef COMMONBRANCHING
#define COMMONBRANCHING 1
#endif

/* Private instrumentation code that is useful for 
 * measuring changes to the interpreter.
 */
#ifndef INSTRUMENT
#define INSTRUMENT 0
#endif

/* Turn this on to do a GC between every bytecode
 * (Painfully/ridiculously slow...)
 */
#ifndef VERY_EXCESSIVE_GARBAGE_COLLECTION
#define VERY_EXCESSIVE_GARBAGE_COLLECTION 0
#endif

/*=========================================================================
 * Setup default local register values if LOCALVMREGISTERS is enabled
 *=======================================================================*/

#if LOCALVMREGISTERS

/* IP = Instruction pointer */
#ifndef IPISLOCAL
#define IPISLOCAL 1
#endif

/* SP = (Operand) Stack pointer */
#ifndef SPISLOCAL
#define SPISLOCAL 1
#endif

/* LP = Locals Pointer */
#ifndef LPISLOCAL
#define LPISLOCAL 0
#endif

/* FP = Frame Pointer */
#ifndef FPISLOCAL
#define FPISLOCAL 0
#endif

/* CP = Constant Pool Pointer */
#ifndef CPISLOCAL
#define CPISLOCAL 0
#endif

#else

/*=========================================================================
 * Setup default local register values if LOCALVMREGISTERS is not enabled
 *=======================================================================*/

#ifdef IPISLOCAL
#undef IPISLOCAL
#endif

#ifdef SPISLOCAL
#undef SPISLOCAL
#endif

#ifdef LPISLOCAL
#undef LPISLOCAL
#endif

#ifdef FPISLOCAL
#undef FPISLOCAL
#endif

#ifdef CPISLOCAL
#undef CPISLOCAL
#endif

#define IPISLOCAL 0
#define FPISLOCAL 0
#define SPISLOCAL 0
#define LPISLOCAL 0
#define CPISLOCAL 0

#endif /* LOCALVMREGISTERS */

/*=========================================================================
 * Extreme debug option to call the garbage collector before every bytecode
 *=======================================================================*/

#if VERY_EXCESSIVE_GARBAGE_COLLECTION
#define DO_VERY_EXCESSIVE_GARBAGE_COLLECTION {  \
    VMSAVE                                      \
    garbageCollect(0);                          \
    VMRESTORE                                   \
}
#else
#if ASYNCHRONOUS_NATIVE_FUNCTIONS && EXCESSIVE_GARBAGE_COLLECTION
#define DO_VERY_EXCESSIVE_GARBAGE_COLLECTION {  \
    extern bool_t veryExcessiveGCrequested;     \
    if (veryExcessiveGCrequested) {             \
        VMSAVE                                  \
        garbageCollect(0);                      \
        VMRESTORE                               \
        veryExcessiveGCrequested = FALSE;       \
    }                                           \
}
#else
#define DO_VERY_EXCESSIVE_GARBAGE_COLLECTION /**/
#endif
#endif

/*=========================================================================
 * Instrumentation macros
 *=======================================================================*/

#if INSTRUMENT
#define INC_CALLS     calls++;
#define INC_RESHED    reshed++;
#define INC_BYTECODES bytecodes++;
#define INC_SLOWCODES slowcodes++;
#define INC_BRANCHES  branches++;
#else
#define INC_CALLS     /**/
#define INC_RESHED    /**/
#define INC_BYTECODES /**/
#define INC_SLOWCODES /**/
#define INC_BRANCHES  /**/
#endif /* INSTRUMENT */

/*=========================================================================
 * SELECT - Macros to define bytecode(s)
 *=======================================================================*/

#define SELECT(l1)                      case l1: {
#define SELECT2(l1, l2)                 case l1: case l2: {
#define SELECT3(l1, l2, l3)             case l1: case l2: case l3: {
#define SELECT4(l1, l2, l3, l4)         case l1: case l2: case l3: case l4: {
#define SELECT5(l1, l2, l3, l4, l5)     case l1: case l2: case l3: case l4: case l5: {
#define SELECT6(l1, l2, l3, l4, l5, l6) case l1: case l2: case l3: case l4: case l5: case l6: {

/*=========================================================================
 * DONE - To end a bytecode definition and increment ip
 *=======================================================================*/

#define DONE(n)    } goto next##n;

/*=========================================================================
 * DONEX - To end a bytecode definition without goto
 *=======================================================================*/

#define DONEX      }

/*=========================================================================
 * DONE_R - To end a bytecode definition and test for thread rescheduling
 *=======================================================================*/

#define DONE_R     } goto reschedulePoint;

/*=========================================================================
 * CHECKARRAY - To check for valid array access
 *=======================================================================*/

#define CHECKARRAY(thisArray, index)                                 \
    if (thisArray) {                                                 \
        /* Check that the given index is within array boundaries */  \
        if (index >= 0 && index < (long)thisArray->length) {

/*=========================================================================
 * ENDCHECKARRAY - Finish the check for valid array access
 *=======================================================================*/

#define ENDCHECKARRAY                                     \
        } else goto handleArrayIndexOutOfBoundsException; \
    } else goto handleNullPointerException;               \

/*=========================================================================
 * CALL_VIRTUAL_METHOD - Branch to common code for Invokevirtual
 *=======================================================================*/

#define CALL_VIRTUAL_METHOD   {                 \
    goto callMethod_virtual;                    \
}

/*=========================================================================
 * CALL_STATIC_METHOD - Branch to common code for Invokestatic
 *=======================================================================*/

#define CALL_STATIC_METHOD  {                   \
    goto callMethod_static;                     \
}

/*=========================================================================
 * CALL_SPECIAL_METHOD - Branch to common code for Invokespecial
 *=======================================================================*/

#define CALL_SPECIAL_METHOD  {                  \
    goto callMethod_special;                    \
}

/*=========================================================================
 * CALL_INTERFACE_METHOD - Branch to common code for Invokeinterface
 *=======================================================================*/

#define CALL_INTERFACE_METHOD  {                \
    goto callMethod_interface;                  \
}

/*=========================================================================
 * CHECK_NOT_NULL - Throw an exception of an object is null
 *
 * Use the following macro to place null checks where a NullPointerException
 * should be thrown (e.g. invoking a virtual method on a null object).
 * As this macro executes a continue statement, it must only be used where
 * the continue will effect a jump to the start of the interpreter loop.
 *=======================================================================*/

#define CHECK_NOT_NULL(object)                  \
    if (object == NIL) {                        \
        goto handleNullPointerException;        \
}

/*=========================================================================
 * TRACE_METHOD_ENTRY - Macro for tracing
 *=======================================================================*/

#if INCLUDEDEBUGCODE
#define TRACE_METHOD_ENTRY(method, what) {                      \
   ASSERTING_NO_ALLOCATION                                      \
   if (tracemethodcallsverbose) Log->enterMethod(method, what); \
   END_ASSERTING_NO_ALLOCATION                                  \
}
#else
#define TRACE_METHOD_ENTRY(method, what) /**/
#endif

/*=========================================================================
 * TRACE_METHOD_EXIT - Macro for tracing
 *=======================================================================*/

#if INCLUDEDEBUGCODE
#define TRACE_METHOD_EXIT(method) {                      \
   ASSERTING_NO_ALLOCATION                               \
   if (tracemethodcallsverbose) Log->exitMethod(method); \
   END_ASSERTING_NO_ALLOCATION                           \
}
#else
#define TRACE_METHOD_EXIT(method)        /**/
#endif

/*=========================================================================
 * POP_FRAME - Macro to pop a stack frame
 *=======================================================================*/

#if INCLUDEDEBUGCODE
#define POP_FRAME {                                \
       VMSAVE                                      \
       popFrame();                                 \
       VMRESTORE                                   \
}
#else
#define POP_FRAME POPFRAMEMACRO
#endif

/*=========================================================================
 * INFREQUENTROUTINE - Have bytecode executed by the infrequent routine
 *=======================================================================*/

#if SPLITINFREQUENTBYTECODES
#define INFREQUENTROUTINE(x) case x: { goto callSlowInterpret; }
#else
#define INFREQUENTROUTINE(x) /**/
#endif

/*=========================================================================
 * INSTRUCTIONPROFILE - Instruction profiling
 *=======================================================================*/

#if ENABLEPROFILING
#define INSTRUCTIONPROFILE  {                   \
    VMSAVE                                      \
    InstructionProfile();                       \
    VMRESTORE                                   \
}
#else
#define INSTRUCTIONPROFILE /**/
#endif

/*=========================================================================
 * INSTRUCTIONTRACE - Instruction tracing
 *=======================================================================*/

#if INCLUDEDEBUGCODE
#define INSTRUCTIONTRACE  {                             \
        VMSAVE                                          \
        if (tracebytecodes) InstructionTrace(ip_global);\
        VMRESTORE                                       \
}
#else
#define INSTRUCTIONTRACE /**/
#endif

/*=========================================================================
 * RESCHEDULE - Thread rescheduling
 *=======================================================================*/

#if ENABLE_JAVA_DEBUGGER

#define __getNextToken()                                                \
        if (CurrentThread->isAtBreakpoint) {           \
            CurrentThread->isAtBreakpoint = FALSE;                      \
            token = CurrentThread->nextOpcode;                          \
        } else                                                          \
            token = *ip;

#define __doSingleStep()                                                \
        if (CurrentThread && CurrentThread->isStepping) {               \
            THREAD __thread__ = CurrentThread;                          \
            THREAD __tp__;                                              \
            VMSAVE                                                      \
            if (handleSingleStep(__thread__, &__tp__)) {                \
                __tp__->isAtBreakpoint = TRUE;                          \
                __tp__->nextOpcode = token;                             \
                VMRESTORE                                               \
                goto reschedulePoint;                                   \
            }                                                           \
            VMRESTORE                                                   \
        }

#define __checkDebugEvent()                                             \
        if (vmDebugReady && CurrentThread && CurrentThread->needEvent) {  \
            checkDebugEvent(CurrentThread);                             \
        }

/* This routine/macro is called from inside the interpreter */
/* to check if it is time to perform thread switching and   */
/* possibly to check for external events.  This routine is  */
/* very performance critical!  Most of the function calls   */
/* inside this macro definition turn into null statements   */
/* in production builds when debugger support is turned off */

#define RESCHEDULE {                            \
    INC_RESHED                                  \
    checkRescheduleValid();                     \
    if (isTimeToReschedule()) {                 \
        VMSAVE                                  \
        __checkDebugEvent()                     \
        reschedule();                           \
        VMRESTORE                               \
    }                                           \
    if (vmDebugReady && CurrentThread) {        \
      __getNextToken()                          \
      __doSingleStep()                          \
    } else                                      \
      token = *ip;                              \
}

#else

/* This routine/macro is called from inside the interpreter */
/* to check if it is time to perform thread switching and   */
/* possibly to check for external events.  This routine is  */
/* very performance critical!  Most of the function calls   */
/* inside this macro definition turn into null statements   */
/* in production builds when debugger support is turned off */

#define RESCHEDULE {                            \
    INC_RESHED                                  \
    checkRescheduleValid();                     \
    if (isTimeToReschedule()) {                 \
        VMSAVE                                  \
        reschedule();                           \
        VMRESTORE                               \
    }                                           \
}

#endif /* ENABLE_JAVA_DEBUGGER */

/*=========================================================================
 * BRANCHIF - Macro to cause a branch if a condition is true
 *=======================================================================*/

#if COMMONBRANCHING
#define BRANCHIF(cond) { if(cond) { goto branchPoint; } else { goto next3; } }
#else
#define BRANCHIF(cond) { ip += (cond) ? getShort(ip + 1) : 3; goto reschedulePoint; }
#endif

/*=========================================================================
 * NOTIMPLEMENTED - Macro to pad out the jump table as an option
 *=======================================================================*/

#if PADTABLE
#define NOTIMPLEMENTED(x) case x: { goto notImplemented; }
#else
#define NOTIMPLEMENTED(x) /**/
#endif

/*=========================================================================
 * CLEAR - Zero a variable for safety only
 *=======================================================================*/

#if INCLUDEDEBUGCODE
#define CLEAR(x) { (x) = 0; }
#else
#define CLEAR(x) /**/
#endif

/*=========================================================================
 * SAVEIP/RESTOREIP - Macros to spill and load the ip machine register
 *=======================================================================*/

#if IPISLOCAL
#define SAVEIP    ip_global = ip; CLEAR(ip);
#define RESTOREIP ip = ip_global; CLEAR(ip_global);
#else
#define SAVEIP    /**/
#define RESTOREIP /**/
#endif

/*=========================================================================
 * SAVEFP/RESTOREFP - Macros to spill and load the fp machine register
 *=======================================================================*/

#if FPISLOCAL
#define SAVEFP    fp_global = fp; CLEAR(fp);
#define RESTOREFP fp = fp_global; CLEAR(fp_global);
#else
#define SAVEFP    /**/
#define RESTOREFP /**/
#endif

/*=========================================================================
 * SAVESP/RESTORESP - Macros to spill and load the sp machine register
 *=======================================================================*/

#if SPISLOCAL
#define SAVESP    sp_global = sp; CLEAR(sp);
#define RESTORESP sp = sp_global; CLEAR(sp_global);
#else
#define SAVESP    /**/
#define RESTORESP /**/
#endif

/*=========================================================================
 * SAVELP/RESTORELP - Macros to spill and load the lp machine register
 *=======================================================================*/

#if LPISLOCAL
#define SAVELP    lp_global = lp; CLEAR(lp);
#define RESTORELP lp = lp_global; CLEAR(lp_global);
#else
#define SAVELP    /**/
#define RESTORELP /**/
#endif

/*=========================================================================
 * SAVECP/RESTORECP - Macros to spill and load the cp machine register
 *=======================================================================*/

#if CPISLOCAL
#define SAVECP    cp_global = cp; CLEAR(cp);
#define RESTORECP cp = cp_global; CLEAR(cp_global);
#else
#define SAVECP    /**/
#define RESTORECP /**/
#endif

