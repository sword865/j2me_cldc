/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Interpreter stack frames
 * FILE:      frame.c
 * OVERVIEW:  This file defines the internal operations for
 *            manipulating stack frames & performing exception
 *            handling.
 * AUTHOR:    Antero Taivalsaari, Sun Labs, 1998
 *            Edited by Doug Simon 11/1998 (simplified exception handling)
 *            Sheng Liang (chunky stacks), Frank Yellin
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>

/*=========================================================================
 * Global variables and definitions
 *=======================================================================*/

/* Shortcuts to the errors/exceptions that the VM may throw */
/* These come directly from the CLDC Specification */

const char ArithmeticException[]
           = "java/lang/ArithmeticException";
const char ArrayIndexOutOfBoundsException[]
           = "java/lang/ArrayIndexOutOfBoundsException";
const char ArrayStoreException[]
           = "java/lang/ArrayStoreException";
const char ClassCastException[]
           = "java/lang/ClassCastException";
const char ClassNotFoundException[]
            = "java/lang/ClassNotFoundException";
const char IllegalAccessException[]
           = "java/lang/IllegalAccessException";
const char IllegalArgumentException[]
           = "java/lang/IllegalArgumentException";
const char IllegalMonitorStateException[]
           = "java/lang/IllegalMonitorStateException";
const char IllegalThreadStateException[]
           = "java/lang/IllegalThreadStateException";
const char IndexOutOfBoundsException[]
           = "java/lang/IndexOutOfBoundsException";
const char InstantiationException[]
           = "java/lang/InstantiationException";
const char InterruptedException[]
           = "java/lang/InterruptedException";
const char NegativeArraySizeException[]
           = "java/lang/NegativeArraySizeException";
const char NullPointerException[]
           = "java/lang/NullPointerException";
const char NumberFormatException[]
           = "java/lang/NumberFormatException";
const char RuntimeException[]
           = "java/lang/RuntimeException";
const char SecurityException[]
           = "java/lang/SecurityException";
const char StringIndexOutOfBoundsException[]
           = "java/lang/StringIndexOutOfBoundsException";

const char IOException[]
           = "java/io/IOException";

const char NoClassDefFoundError[]
           = "java/lang/NoClassDefFoundError";
const char OutOfMemoryError[]
           = "java/lang/OutOfMemoryError";
const char VirtualMachineError[]
           = "java/lang/VirtualMachineError";

/* The following classes would be needed for full JLS/JVMS compliance */
/* However, they are not part of the CLDC Specification. */

const char AbstractMethodError[]
           = "java/lang/AbstractMethodError";
const char ClassCircularityError[]
           = "java/lang/ClassCircularityError";
const char ClassFormatError[]
           = "java/lang/ClassFormatError";
const char IllegalAccessError[]
           = "java/lang/IllegalAccessError";
const char IncompatibleClassChangeError[]
           = "java/lang/IncompatibleClassChangeError";
const char InstantiationError[]
           = "java/lang/InstantiationError";
const char NoSuchFieldError[]
           = "java/lang/NoSuchFieldError";
const char NoSuchMethodError[]
           = "java/lang/NoSuchMethodError";
const char VerifyError[]
           = "java/lang/VerifyError";

/*
const char InternalError[]
           = "java/lang/InternalError";
const char ExceptionInInitializerError[]
           = "java/lang/ExceptionInInitializerError";
const char LinkageError[]
           = "java/lang/LinkageError";
const char StackOverflowError[]
           = "java/lang/StackOverflowError";
const char UnknownError[]
           = "java/lang/UnknownError";
const char UnsatisfiedLinkError[]
           = "java/lang/UnsatisfiedLinkError";
const char UnsupportedClassVersionError[]
           = "java/lang/UnsupportedClassVersionError";
*/

/*=========================================================================
 * Static function prototypes (for functions used only in this file)
 *=======================================================================*/

#if INCLUDEDEBUGCODE

static void exceptionThrownTracing(INSTANCE_HANDLE exceptionH);
static void exceptionCaughtTracing(THROWABLE_INSTANCE_HANDLE exceptionH, HANDLER handler);

#else

#define exceptionThrownTracing(exception)
#define exceptionCaughtTracing(exception, handler)

#endif /* INCLUDEDEBUGCODE */

/*=========================================================================
 * Operations on stack frames
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      pushFrame()
 * TYPE:          constructor (kind of)
 * OVERVIEW:      Creates a new execution stack frame for the currently
 *                executing thread. The operation is used for invoking
 *                methods upon message sending to objects.  Allocates
 *                new stack chunks as necessary ("chunky stacks").
 *                This allows the KVM to support a large number of
 *                Java threads in a limited heap space.
 * INTERFACE:
 *   parameters:  method pointer
 *   returns:     <nothing>
 *   throws:      OutOfMemoryError if the system runs out of memory
 *
 * COMMENTS:      Note: this function operates in the context of
 *                currently executing thread. All VM registers must
 *                be initialized correctly before allocating frames.
 *
 *                Remember that the zeroeth local variable '*lp'
 *                must contain the 'this' (self) pointer
 *                in virtual/special/interface method calls.
 *
 *                Also remember that unlike most other Java VMs,
 *                KVM does not create stack frames for native
 *                function calls (see native.c and kni.h/c for 
 *                more information on native function calls)
 *=======================================================================*/

void pushFrame(METHOD thisMethod)
{
    int thisFrameSize = thisMethod->frameSize;
    int thisArgCount = thisMethod->argCount;
    int thisLocalCount = thisFrameSize - thisArgCount;
    
    STACK stack = getFP() ? getFP()->stack : CurrentThread->stack;
    int thisMethodHeight = thisLocalCount + thisMethod->u.java.maxStack + 
                           SIZEOF_FRAME + RESERVEDFORNATIVE;
    FRAME newFrame;
    int i;
    cell* prev_sp = getSP() - thisArgCount; /* Very volatile! */
    
    /* Check if there is enough space in the current stack chunk */
    if (getSP() - stack->cells + thisMethodHeight >= stack->size) {
        /* Not enough space in the current stack chunk. */
        /* We need to create a new one or reuse an existing one */
        STACK newstack;
        thisMethodHeight += thisArgCount;

        /* Check if we can reuse an existing stack chunk */
        if (stack->next && thisMethodHeight > stack->next->size) {
            stack->next = NULL;
        }

        /* If next is NULL, we need to create a new stack chunk */
        if (stack->next == NULL) {
            int size = thisMethodHeight > STACKCHUNKSIZE 
                     ? thisMethodHeight : STACKCHUNKSIZE;
            int stacksize = sizeof(struct stackStruct) / CELL + 
                           (size - STACKCHUNKSIZE);
            START_TEMPORARY_ROOTS
                DECLARE_TEMPORARY_ROOT(STACK, stackX, stack);
                newstack = (STACK)mallocHeapObject(stacksize, GCT_EXECSTACK); 
                stack = stackX;
                prev_sp = getSP() - thisArgCount;
            END_TEMPORARY_ROOTS
            if (newstack == NULL) {
                THROW(StackOverflowObject);
            }

#if INCLUDEDEBUGCODE
            /* In debug mode, initialize the new stack chunk to zeros */
            memset(newstack, 0, stacksize << log2CELL);
#endif

            newstack->next = NULL;
            newstack->size = size;
            stack->next = newstack;

#if INCLUDEDEBUGCODE
            if (traceframes || tracestackchunks) {
                fprintf(stdout,
                    "Created a new stack chunk (thread: %lx, new chunk: %lx, prev: %lx, stack depth: %ld)\n", 
                    (long)CurrentThread, (long)newstack,
                    (long)stack, (long)frameDepth());
            }
#endif
        } else {
            /* Can reuse an existing, unused stack chunk */
            newstack = stack->next;
        }

        /* The actual pushFrame operation for a new stack chunk happens here */
        for (i = 0; i < thisArgCount; i++) {
            newstack->cells[i] = prev_sp[i + 1];
        }
        setLP(newstack->cells);
        newFrame = (FRAME)(getLP() + thisFrameSize);
        newFrame->stack = newstack;
    } else {

        /* This is the more common case; pushing a frame without */
        /* having to create or reuse another stack chunk */
        ASSERTING_NO_ALLOCATION
            /* Set the local variable pointer to point to the start */
            /* of the local variables in the execution stack */
            setLP(prev_sp + 1);

            /* Add space for local variables, and */
            /* initialize the new frame pointer */
            newFrame = (FRAME)(getSP() + thisLocalCount + 1);
            newFrame->stack = stack;
        END_ASSERTING_NO_ALLOCATION
    }

#if ENABLE_JAVA_DEBUGGER
    /*
     * Although the GC doesn't need to zero out the locations, the debugger
     * code needs to have unallocated objects zeroed out on the stack, else
     * it will try to dereference them when the debugger asks for the local
     * variables.
     */
    if (vmDebugReady)
        memset(getLP() + thisArgCount, 0, thisLocalCount << log2CELL);
#endif

    /* Fill out the remaining fields in the stack frame */
    ASSERTING_NO_ALLOCATION
        /* Initialize info needed for popping the stack frame later on */
        newFrame->previousSp = prev_sp;
        newFrame->previousIp = getIP();
        newFrame->previousFp = getFP();

        /* Initialize the frame to execute the given method */
        newFrame->thisMethod = thisMethod;
        newFrame->syncObject = NIL; /* Initialized later if necessary */

        /* Change virtual machine registers to execute the new method */
        setFP(newFrame);
        setSP((cell*)(newFrame + 1) - 1);
        setIP(thisMethod->u.java.code);
        setCP(thisMethod->ofClass->constPool);

#if INCLUDEDEBUGCODE
        if (tracemethodcalls || tracemethodcallsverbose) {
            frameTracing(thisMethod, "=>", 0);
        }

        if (traceframes) {
            fprintf(stdout, 
                "Pushed a stack frame (thread: %lx, fp: %lx, sp: %lx, depth: %ld, stack: %lx)\n",
                (long)CurrentThread, (long)getFP(),
                (long)getSP(), (long)frameDepth(), (long)stack);
        }
#endif /* INCLUDEDEBUGCODE */

    END_ASSERTING_NO_ALLOCATION
}

/*=========================================================================
 * FUNCTION:      popFrame()
 * TYPE:          destructor (kind of)
 * OVERVIEW:      Deletes an execution stack frame and resumes the
 *                execution of the method that called the currently
 *                executing method.
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     Nothing directly, but field 'previousIp' holds
 *                information about what to do after the frame has
 *                been popped (whether to continue interpreter execution,
 *                to kill the thread, or to return to C/C++ code).
 *                Also, the field 'syncObject' holds a monitor object
 *                to be released in case this was a synchronized method
 *                call (see the RETURN bytecodes in Interpret.cpp)
 *=======================================================================*/

void popFrame()
{
    /* Unallocate a stack frame and restore virtual */ 
    /* machine registers to continue the execution */
    /* of the previous method. */

#if INCLUDEDEBUGCODE
        if (tracemethodcalls || tracemethodcallsverbose) {
            frameTracing(getFP()->thisMethod, "<=", 0);
        }
#endif /* INCLUDEDEBUGCODE */

    /* See frame.h */
    POPFRAMEMACRO

#if INCLUDEDEBUGCODE
    if (traceframes) {
        fprintf(stdout,
            "Popped a stack frame (thread: %lx, fp: %lx, sp: %lx, depth: %ld, stack: %lx)\n",
            (long)CurrentThread, (long)getFP(), 
            (long)getSP(), (long)frameDepth(), (long)getFP()->stack);
    }
#endif /* INCLUDEDEBUGCODE */

#if ENABLE_JAVA_DEBUGGER
    if (vmDebugReady)
        setEvent_FramePop();
#endif
}

/*=========================================================================
 * Exception handling operations
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      findHandler()
 * TYPE:          exception handler table lookup operation
 * OVERVIEW:      Find a possible handler in the given exception handler
 *                table that catches the given exception in given
 *                code location.
 * INTERFACE:
 *   parameters:  handler table, exception object, instruction pointer offset
 *   returns:     handler or NIL if no matching handler is found
 *=======================================================================*/

static HANDLER
findHandler(INSTANCE_CLASS thisClass, HANDLERTABLE handlerTable, 
            THROWABLE_INSTANCE_HANDLE exceptionH, unsigned short ipOffset)
{
    HANDLER result = NULL;
    ASSERTING_NO_ALLOCATION    
        FOR_EACH_HANDLER(thisHandler, handlerTable)
            if (ipOffset < thisHandler->startPC) { 
                continue;
            } else if (ipOffset >= thisHandler->endPC) { 
                continue;
            } else { 
                if (thisHandler->exception == 0) {
                    result = thisHandler;
                    break;
                } else { 
                    unsigned short thisException = 
                        (unsigned short)thisHandler->exception;
                    CLASS handlerClass = 
                        resolveClassReference(thisClass->constPool,
                                              thisException,
                                              thisClass);
                    if (isAssignableTo((CLASS)(unhand(exceptionH)->ofClass), 
                                       (CLASS)handlerClass)) {
                        /* Matching exception handler has been found */
                        result = thisHandler;
                        break;
                    }
                }
            }
        END_FOR_EACH_HANDLER
    END_ASSERTING_NO_ALLOCATION
    return result;
}

/*=========================================================================
 * FUNCTION:      throwException()
 * TYPE:          internal exception handling operation
 * OVERVIEW:      Throw an exception (to be handled by a matching
 *                stack frame exception handler.  This operation may be
 *                called both from bytecode (by primitive ATHROW) or
 *                internally from the VM whenever an exceptional
 *                situation occurs (e.g., an array range exception).
 *
 *                Handle a runtime exception by inspecting exception
 *                handlers in execution stack frames, and unwinding the
 *                execution stack if necessary in order to try to find
 *                a matching handler.  If we find a matching handler
 *                set the interpreter to execute its code.
 * INTERFACE:
 *   parameters:  pointer to an exception object
 *   returns:     <nothing>
 * NOTE:          When invoking this operation from inside the VM,
 *                keep in mind that the exception handler is not
 *                executed until the VM returns back to the interpreter.
 *                Thus, you should not put any C/C++ code after calls
 *                to 'throwException' because that code would be called
 *                "too early" (without having executed any of the
 *                the exception handler bytecode.
 *=======================================================================*/

void throwException(THROWABLE_INSTANCE_HANDLE exceptionH) {
#if ENABLE_JAVA_DEBUGGER
    START_TEMPORARY_ROOTS
    DECLARE_TEMPORARY_FRAME_ROOT(saveFp, (getFP()));
    DECLARE_TEMPORARY_FRAME_ROOT(thisFP, (getFP()));
    BYTE *saveIp;
#else
    FRAME thisFP;
#endif
    BYTE* thisIP;
    OBJECT synchronized;
    unsigned short ipOffset = 0;

    /* The KVM has already updated the program counter in the saved pc of
     * method calls.  So we need to correct the pc for all frames except
     * the top one, and the ones just below RunCustomCode
     * 
     * But if we're running a native method, then there's a missing frame
     * at the top (we don't push frames for native methods), so we need
     * the correction.
     */
    int ipCorrection = (CurrentNativeMethod == NULL) ? 0 : 1;

restartOnError:
    thisFP = getFP();
    thisIP = getIP();
#if ENABLE_JAVA_DEBUGGER
    saveIp = thisIP;
    saveFp = thisFP;
#endif
    if (CurrentThread == NULL) {
        /* If we don't have any active threads in the system, there is no point
         * in trying to unwind stack frames. Print the detailed error message
         * contained in the second slot of the exception option and then exit.
         * (this code should get executed only if there is an exception before
         * the VM has started properly).
         */
        STRING_INSTANCE string = unhand(exceptionH)->message;
        if (string != NULL) {
            fatalError(getStringContents(string));
        } else {
            fatalError(getClassName((CLASS)unhand(exceptionH)->ofClass));
        }
    }

#if INCLUDEDEBUGCODE
    if (traceexceptions) {
        Log->throwException(unhand(exceptionH));
    }
#endif

    while (thisFP != NULL) {
        /* Check if the current execution frame/method */
        /* has an exception handler table */
        METHOD thisMethod = thisFP->thisMethod;
        HANDLERTABLE handlerTable = thisMethod->u.java.handlers;
        if (handlerTable != NULL) { 
            HANDLER thisHandler;
            START_TEMPORARY_ROOTS
                INSTANCE_CLASS thisClass = thisFP->thisMethod->ofClass ;
                DECLARE_TEMPORARY_FRAME_ROOT(thisFPx, thisFP);
                ipOffset = thisIP - thisMethod->u.java.code;
                thisHandler = findHandler(thisClass, handlerTable, exceptionH,
                                          (unsigned short)(ipOffset - ipCorrection));
                thisFP = thisFPx;
            END_TEMPORARY_ROOTS
            if (thisHandler != NULL) {
            
#if INCLUDEDEBUGCODE
                if ((tracemethodcalls || tracemethodcallsverbose) && getFP() != thisFP) {
                    exceptionCaughtTracing(exceptionH, thisHandler);
                }
#endif /* INCLUDEDEBUGCODE */
               
                setFP(thisFP);
                setIP(thisMethod->u.java.code + thisHandler->handlerPC);
                setLP(FRAMELOCALS(thisFP));  
                setCP(thisFP->thisMethod->ofClass->constPool);
                setStackHeight(1);
                topStackAsType(THROWABLE_INSTANCE) = unhand(exceptionH);
#if ENABLE_JAVA_DEBUGGER
                START_TEMPORARY_ROOTS
                DECLARE_TEMPORARY_ROOT(THREAD, thisThread, CurrentThread);
                if (vmDebugReady) {
                    CEModPtr cep = GetCEModifier();
                    cep->exp.classID = 
                       GET_CLASS_DEBUGGERID(&((THROWABLE_INSTANCE)unhand(exceptionH))->ofClass->clazz);
                    cep->exp.sig_caught = TRUE;
                    cep->exp.sig_uncaught = FALSE;
                    cep->threadID = getObjectID((OBJECT)thisThread->javaThread);
                    setEvent_Exception(exceptionH, saveFp,
                        saveIp, thisMethod,
                        (unsigned long)thisHandler->handlerPC, cep);
                    FreeCEModifier(cep);
                }
                END_TEMPORARY_ROOTS
#endif /* ENABLE_JAVA_DEBUGGER */
                goto leave;
            } 
        } else if (thisMethod == RunCustomCodeMethod) {
            START_TEMPORARY_ROOTS
                DECLARE_TEMPORARY_FRAME_ROOT(thisFPx, thisFP); 
                void **bottomStack = (void **)(thisFPx + 1);
                CustomCodeCallbackFunction func = 
                    (CustomCodeCallbackFunction)(bottomStack[0]);
                if (func != NULL) {
                    /* Overwrite the slot contain callback function with error */
                    bottomStack[0] = unhand(exceptionH);
                    func(&thisFPx); /* may GC */
                    thisFP = thisFPx;
                    bottomStack = (void **)(thisFP + 1); /* in case of GC  */
                    unhand(exceptionH) = bottomStack[0];
                }
            END_TEMPORARY_ROOTS
        }

        /* If we got here, then current stack frame had no matching   */
        /* exception handler.  We will unwind (remove) the stack frame. */
        /* Before we can do that, we need to release the possible monitor */
        /* that is held by the stack frame. */
        synchronized = thisFP->syncObject;
        if (synchronized != NULL) {
            char *errorIfFailure;
            enum MonitorStatusType result = 
                monitorExit(synchronized, &errorIfFailure);
            thisFP->syncObject = NULL;
            if (result == MonitorStatusError) { 
                setFP(thisFP);
                setIP(thisIP);
                setStackHeight(0); /* GC's may be confused if we don't adjust */
                unhand(exceptionH) = 
                    (THROWABLE_INSTANCE)instantiate((INSTANCE_CLASS)getClass(errorIfFailure));
                goto restartOnError;
            }
        }
        thisIP = thisFP->previousIp;
        thisFP = thisFP->previousFp;
        ipCorrection = (thisMethod == RunCustomCodeMethod) ? 0 : 1;
    }

    /* If we've gotten here, then there was no exception handler */
    /* in any of the stack frames on the stack.  In debug mode,  */
    /* we'll report an uncaught exception. */
#if INCLUDEDEBUGCODE
    Log->uncaughtException(unhand(exceptionH));
#endif

#if ENABLE_JAVA_DEBUGGER
    START_TEMPORARY_ROOTS
    DECLARE_TEMPORARY_ROOT(THREAD, thisThread, CurrentThread);
    if (vmDebugReady) {
        CEModPtr cep = GetCEModifier();
        cep->exp.classID = 
            GET_CLASS_DEBUGGERID(&((THROWABLE_INSTANCE)unhand(exceptionH))->ofClass->clazz);
        cep->exp.sig_caught = FALSE;
        cep->exp.sig_uncaught = TRUE;
        cep->threadID = getObjectID((OBJECT)thisThread->javaThread);
        setEvent_Exception(exceptionH, saveFp, saveIp, NIL, 
                           (unsigned long)0, cep);
        FreeCEModifier(cep);
    }
    if (CurrentThread == NIL) {
        /*
         * suspended via the debugger when the event was sent.
         */
        DismantleThread(thisThread);
    } else {
        stopThread();

        /* If the VM has no more threads to run, exit immediately */
        if (AliveThreadCount == 0 && AllThreads == NULL) {
            VM_EXIT(FATAL_ERROR_EXIT_CODE);
        }
    }
    END_TEMPORARY_ROOTS
leave:
    END_TEMPORARY_ROOTS         /* this matches the one at the very top */
#else
#if INCLUDEDEBUGCODE
    printExceptionStackTrace(exceptionH);
#endif
        
    stopThread();

    /* If the VM has no more threads to run, exit immediately */
    if (AliveThreadCount == 0 && AllThreads == NULL) {
        VM_EXIT(FATAL_ERROR_EXIT_CODE);
    }

leave:
    ;
#endif /* ENABLE_JAVA_DEBUGGER */

    /* After throwing an exception, we need to make sure     */
    /* that CurrentNativeMethod is NULL, because the C-level */
    /* THROW-CATCH mechanism will make sure that we will no  */
    /* longer go back to 'invokeNativeFunction()' (native.c) */
    /* if an exception was thrown inside a native method.    */
    /* This line of code is specific to the new class loader */
    /* introduced in CLDC 1.1. */
    CurrentNativeMethod = NULL;
}

/*=========================================================================
 * Operations for raising exceptions and errors from within the VM
 * (see the detailed comparison of these operations in frame.h)
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      getExceptionInstance()
 * TYPE:          internal exception handling operation
 * OVERVIEW:      Get a throwable instance. This method should only be used
 *                as a helper by the raiseException* functions as this
 *                functions assumes that the named class exists. It is a
 *                fatalError if it doesn't.  This operation has been added
 *                so that we can gradually bring in more error classes 
 *                as the CLDC Specification evolves.
 * INTERFACE:
 *   parameters:  exception class name
 *   returns:     <nothing>
 *=======================================================================*/

static const char* UnfoundException = NULL;
static const char* UnfoundExceptionMsg = NULL;

static THROWABLE_INSTANCE getExceptionInstance(const char* name,
       const char* msg) {

    INSTANCE_CLASS clazz;
    THROWABLE_INSTANCE exception;

    /* 
     * For CLDC 1.0/1.1, we will hardcode the names of some of the 
     * error classes that we don't support in CLDC.  This will prevent
     * the KVM from trying to load error classes that don't actually
     * exist.  This code should be removed if we ever decide to add the
     * missing error classes to the KVM/CLDC.
     */
    if (name == AbstractMethodError          ||
        name == ClassCircularityError        ||
        name == ClassFormatError             ||
     /* name == ExceptionInInitializerError  || */
        name == IllegalAccessError           ||
        name == IncompatibleClassChangeError ||
        name == InstantiationError           ||
        name == NoSuchMethodError            ||
        name == NoSuchFieldError             ||
        name == VerifyError) {

        /* The VM is about to die; don't bother allocating memory from the heap */
        char buffer[STRINGBUFFERSIZE];

        sprintf(buffer, "%s", name);
        if (msg != NULL) {
            sprintf(buffer + strlen(buffer),": %s.", msg);
        } else {
            strcpy(buffer + strlen(buffer),".");
        }
        fatalError(buffer);
    }

    /*
     * This piece of code is executed when the VM has already tried to
     * throw an error or exception earlier, but couldn't find the error class.
     * The code basically converts raiseException semantics into fatalError 
     * semantics.
     */
    if (UnfoundException != NULL) {
        /* The VM is about to die; don't bother allocating memory from the heap */
        char buffer[STRINGBUFFERSIZE];

        if (name == NoClassDefFoundError || name == ClassNotFoundException) {
            sprintf(buffer,"%s", UnfoundException);
        } 
        else {
            sprintf(buffer,"%s while loading exception class %s",
                    name, UnfoundException);
            UnfoundExceptionMsg = msg;
        }

        if (UnfoundExceptionMsg != NULL) {
            sprintf(buffer + strlen(buffer),": %s.",
                UnfoundExceptionMsg);
        } else {
            strcpy(buffer + strlen(buffer),".");
        }
        fatalError(buffer);
    }
    
    UnfoundException = name;
    UnfoundExceptionMsg = msg;
    
    /*
     * Load the actual exception/error class.
     * Any exceptions thrown during this call will be handled above.
     */
    clazz = (INSTANCE_CLASS)getClass(name);
    
    START_TEMPORARY_ROOTS
        IS_TEMPORARY_ROOT(exception, (THROWABLE_INSTANCE)instantiate(clazz));
        /* The exception object instantiation is successful otherwise we
         * will have already thrown an OutOfMemoryError */
#if PRINT_BACKTRACE
        fillInStackTrace(&exception);
#endif
    END_TEMPORARY_ROOTS

    UnfoundException = NULL;
    UnfoundExceptionMsg = NULL;
    
    return exception;
}

/*=========================================================================
 * FUNCTION:      raiseException()
 * TYPE:          internal exception handling operation
 * OVERVIEW:      Raise an exception.  This operation is intended
 *                to be used from within the VM only.  It should
 *                be used for reporting only those exceptions and
 *                errors that are known to be "safe" and "harmless",
 *                i.e., the internal consistency and integrity of
 *                the VM should not be endangered in any way.
 *
 *                Upon execution, the operation tries to load the
 *                exception class with the given name, instantiate an
 *                instance of that class, and
 *                passes the object to the exception handler routines
 *                of the VM. The operation will fail if the class can't
 *                be found or there is not enough memory to load it or
 *                create an instance of it.
 * INTERFACE:
 *   parameters:  exception class name
 *   returns:     <nothing>
 * NOTE:          Since this operation needs to allocate two new objects
 *                it should not be used for reporting memory-related
 *                problems.
 *=======================================================================*/

void raiseException(const char* exceptionClassName)
{
#if INCLUDEDEBUGCODE
    /* Turn off the allocation guard */
    NoAllocation = 0;
#endif /* INCLUDEDEBUGCODE */
    THROW(getExceptionInstance(exceptionClassName, NULL))
}

/*=========================================================================
 * FUNCTION:      raiseExceptionWithMessage()
 * TYPE:          internal exception handling operations
 * OVERVIEW:      Raise an exception with a message.  This operation is 
 *                intended to be used from within the VM only.  It should
 *                be used for reporting only those exceptions and
 *                errors that are known to be "safe" and "harmless",
 *                i.e., the internal consistency and integrity of
 *                the VM should not be endangered in any way.
 *
 *                Upon execution, the operation tries to load the
 *                exception class with the given name, instantiate an
 *                instance of that class, and
 *                passes the object to the exception handler routines
 *                of the VM. The operation will fail if the class can't
 *                be found or there is not enough memory to load it or
 *                create an instance of it.
 * INTERFACE:
 *   parameters:  exception class name
 *                String message as a C string
 *   returns:     <nothing>
 * NOTE:          Since this operation needs to allocate two new objects
 *                it should not be used for reporting memory-related
 *                problems.
 *=======================================================================*/

void raiseExceptionWithMessage(const char* exceptionClassName, const char* msg)
{
    STRING_INSTANCE stringInstance = NULL;
#if INCLUDEDEBUGCODE
    /* Turn off the allocation guard */
    NoAllocation = 0;
#endif /* INCLUDEDEBUGCODE */
    
    stringInstance = instantiateString(msg, strlen(msg));
    if (stringInstance != NULL) {
        START_TEMPORARY_ROOTS
            DECLARE_TEMPORARY_ROOT(STRING_INSTANCE, string, stringInstance);
            THROWABLE_INSTANCE exception =
                getExceptionInstance(exceptionClassName, msg);
            /* Store the string object into the exception */
            exception->message = string;
            THROW(exception);
        END_TEMPORARY_ROOTS
     }
     raiseException(exceptionClassName);
}

/*=========================================================================
 * FUNCTION:      fatalVMError()
 * TYPE:          internal error handling operation
 * OVERVIEW:      Report a fatal error indicating that some severe
 *                unexpected situation has been encountered by the VM.
 *                This may be due to a bug in the VM.  VM execution will
 *                be stopped. This operation should be called only from
 *                inside the VM.
 * INTERFACE:
 *   parameters:  error message string
 *   returns:     <nothing>
 *=======================================================================*/

void fatalVMError(const char* errorMessage)
{
    fatalError(errorMessage);
}

/*=========================================================================
 * FUNCTION:      fatalError()
 * TYPE:          internal error handling operation
 * OVERVIEW:      Report a fatal error indicating that the execution
 *                of erroneous Java code might have endangered the
 *                integrity of the VM. VM will be stopped. This
 *                operation should be called only the from inside the VM.
 * INTERFACE:
 *   parameters:  error message string.
 *   returns:     <nothing>
 *=======================================================================*/

void fatalError(const char* errorMessage)
{
    if (INCLUDEDEBUGCODE) {
        printVMstatus();
    }
    AlertUser(errorMessage);
    VM_EXIT(FATAL_ERROR_EXIT_CODE);
}

/*=========================================================================
 * Stack tracing operations
 *=======================================================================*/

#if INCLUDEDEBUGCODE

/*=========================================================================
 * FUNCTION:      frameDepth();
 * OVERVIEW:      Returns the current stack depth of the Java stack.
 * INTERFACE:
 *   parameters:  none
 *   returns:     depth
 *
 * COMMENTS:      Do not call this function is fp == 0.
 *=======================================================================*/

static int
frameDepth() {
    int depth = 0;
    FRAME thisFP = getFP();
    while (thisFP->previousIp != KILLTHREAD) {
        depth++;
        thisFP = thisFP->previousFp;
    }
    return depth;
}

/*=========================================================================
 * FUNCTION:      frameTracing();
 * OVERVIEW:      Print a message to stdout indicating that a frame has
 *                been entered or exited.
 * INTERFACE:
 *   parameters:  method:  Method we're calling
 *                glyph:   "=>" or "<="
 *                offset:  Actual depth is frameDepth() + offset
 *=======================================================================*/

void frameTracing(METHOD method, char *glyph, int offset) {    
    ASSERTING_NO_ALLOCATION
        char className[256];
        char methodSignature[256];
        int depth = frameDepth() + offset;
        int counter = depth;
        char *methodName = methodName(method);

        getClassName_inBuffer((CLASS)method->ofClass, className);
        change_Key_to_MethodSignature_inBuffer(method->nameTypeKey.nt.typeKey, 
                                               methodSignature);

        /* Print a bar indicating stack frame depth */
        while (--counter >= 0) {
            fprintf(stdout, "|");
        }

        /* Print name of the class, method name & signature */
        fprintf(stdout, " (%ld) %s %s.%s%s ", (long)depth, glyph, 
                className, methodName, methodSignature);

        /* Print thread id */
        fprintf(stdout, "(Thread %lx)\n", (long)CurrentThread);
    END_ASSERTING_NO_ALLOCATION
}

/*=========================================================================
 * FUNCTION:      exceptionThrownTracing
 * OVERVIEW:      Print a message indicating that an exception isn't being
 *                caught in the same stack frame that it was called.
 * INTERFACE:
 *   parameters:  exception:  Exception being thrown
 *=======================================================================*/

#if UNUSED_CODE

/* This isn't currently being used.  But it's worth leaving 
 * the definition here, in case we ever want to bring it back.
 */
static void
exceptionThrownTracing(INSTANCE_HANDLE exceptionH) {
    ASSERTING_NO_ALLOCATION
        int depth = frameDepth();
        char className[256];
        getClassName_inBuffer((CLASS)unhand(exceptionH)->ofClass, className);
        fprintf(stdout, "%lx: (%ld) ", (long)CurrentThread, (long)depth);
        while (--depth >= 0) {
            fprintf(stdout, "|");
        }
        fprintf(stdout, " Throwing %s\n", className);
    END_ASSERTING_NO_ALLOCATION
}

#endif /* UNUSED_CODE */

/*=========================================================================
 * FUNCTION:      exceptionCaughtTracing
 * OVERVIEW:      Print a message indicating that an exception has been
 *                caught in a frame other than the one that threw it.
 * INTERFACE:
 *   parameters:  exception:  Exception being thrown
 *                handler:    Exception handler
 *=======================================================================*/

static void
exceptionCaughtTracing(THROWABLE_INSTANCE_HANDLE exceptionH, HANDLER handler) {
    ASSERTING_NO_ALLOCATION
        int depth = frameDepth();
        char className[256];
        getClassName_inBuffer((CLASS)unhand(exceptionH)->ofClass, className);
        fprintf(stdout, "%lx: (%ld) ", (long)CurrentThread, (long)depth);
        while (--depth >= 0) {
            fprintf(stdout, "|");
        }
        fprintf(stdout, " Caught %s%s\n", className,
                handler->exception == 0 ? " (finally)" : "");
    END_ASSERTING_NO_ALLOCATION
}

#endif /* INCLUDEDEBUGCODE */

/*=========================================================================
 * FUNCTION:      fillInStackTrace, printExceptionStackTrace
 * TYPE:          internal local function
 * OVERVIEW:      save / print information about the current stack
 *                including the method pointers for each stack frame
 * INTERFACE (operand stack manipulation):
 *   parameters:  an Exception
 *   returns:     void
 *====================================================================*/

#if PRINT_BACKTRACE

void fillInStackTrace(THROWABLE_INSTANCE_HANDLE exceptionH) {
    ARRAY backtrace;
    int i;
    int depth;
    FRAME thisFp;
    BYTE* thisIp;

    /* Can't do much if we are in VM startup... */
    if (CurrentThread == NULL) return;

    /* Get the number of frames that need to be saved less the current frame */
    for ( depth = 1, thisFp = getFP(); 
          thisFp->previousIp != KILLTHREAD; 
          depth++, thisFp = thisFp->previousFp) 
        ; /* intentionally empty */

    /* We are essentially doing an instantiateArray here, but we don't 
     * want it to throw an error if it runs out of memory.  For now, we
     * have to roll our own, but this may become its own function, someday */
    backtrace = (ARRAY)mallocHeapObject(SIZEOF_ARRAY(2 * depth), GCT_ARRAY);
    unhand(exceptionH)->backtrace = backtrace;
    if (backtrace != NULL) { 
        ASSERTING_NO_ALLOCATION
            /* Make sure all headers are cleared. */
            memset(backtrace, 0, offsetof(struct arrayStruct, data[0]));
            backtrace->ofClass = PrimitiveArrayClasses[T_INT];
            backtrace->length = depth * 2;
            
            thisIp = getIP();
            thisFp = getFP();
            for (i = 0; i < depth; i++) {
                backtrace->data[i*2].cellp = (cell*)thisFp->thisMethod;
                backtrace->data[i*2+1].cell = 
                    thisIp - thisFp->thisMethod->u.java.code;
                thisIp = thisFp->previousIp;
                thisFp = thisFp->previousFp;
            }
        END_ASSERTING_NO_ALLOCATION
    }
}
#endif /* PRINT_BACKTRACE */

void printExceptionStackTrace(THROWABLE_INSTANCE_HANDLE exceptionH) { 
#if PRINT_BACKTRACE
    START_TEMPORARY_ROOTS 
        DECLARE_TEMPORARY_ROOT(ARRAY, backtrace, unhand(exceptionH)->backtrace);
        DECLARE_TEMPORARY_ROOT(char*, className, NULL);
        int length = (backtrace == NULL ? 0 : backtrace->length);
        int i;

        for (i = 0; i < length; i += 2) {
            METHOD method = (METHOD)(backtrace->data[i].cellp);
            char *ptr;
            className = getClassName((CLASS)method->ofClass);
            ptr = strchr(className, '/');
            /* Replace all slashes in the string with dots */
            while (ptr != NULL) {
                *ptr = '.';
                ptr = strchr(ptr + 1, '/');
            }
            fprintf(stdout, "\tat %s.%s(+%ld)\n", className,
                    methodName(method), backtrace->data[i+1].cell);
        }
    END_TEMPORARY_ROOTS
#else 
    fprintf(stdout, "Stack trace data not available\n");
#endif /* PRINT_BACKTRACE */
}
    
/*=========================================================================
 * Stack frame printing and debugging operations
 *=======================================================================*/

#if INCLUDEDEBUGCODE

/*=========================================================================
 * FUNCTION:      printFrame()
 * TYPE:          public instance-level function on stack frames
 * OVERVIEW:      Prints the contents of a specific execution stack
 *                frame for debugging purposes.
 * INTERFACE:
 *   parameters:  frame pointer
 *   returns:     <nothing>
 *=======================================================================*/

static void
printFrame(FRAME currentFP, unsigned char *currentIP, cell *currentSP)
{
    METHOD thisMethod = currentFP->thisMethod;
    unsigned char *prevIP = currentFP->previousIp;
    FRAME prevFP = currentFP->previousFp;
    char buffer[256];

    int argCount, i;
    cell* pointer;

    /* This is ugly, but this gets called during fatal errors, and we
     * may not have any memory to allocate a className buffer, etc()
     */
    getClassName_inBuffer((CLASS)thisMethod->ofClass, buffer);
    fprintf(stdout, "Method............: %lx '%s.%s (%s)' \n",
            (long)thisMethod,
            buffer, methodName(thisMethod),
            ((thisMethod->accessFlags & ACC_STATIC) ? "static" : "virtual"));
    fprintf(stdout, "Stack Chunk.......: %lx\n", (long)currentFP->stack);
    fprintf(stdout, "Frame Pointer.....: %lx\n", (long)currentFP);
    if (currentIP == NULL) {
        fprintf(stdout, "Bytecode..........: %lx\n",
                (long)thisMethod->u.java.code);
    } else {
        fprintf(stdout, "Current IP........: %lx = %lx + offset %ld\n",
                (long)currentIP,
                (long)thisMethod->u.java.code,
                (long)(currentIP - thisMethod->u.java.code));
    }
    fprintf(stdout, "Previous Frame....: %lx\n", (long)prevFP);
    fprintf(stdout, "Previous IP.......: %lx", (long)prevIP);
    if (prevFP != NULL) {
        int offset = prevIP - prevFP->thisMethod->u.java.code;
        fprintf(stdout, " (offset %d)", offset);
    }
    fprintf(stdout, "\n");

    fprintf(stdout,"Frame size........: %d (%d arguments, %d local variables)\n",
            thisMethod->frameSize, thisMethod->argCount,
           (thisMethod->frameSize - thisMethod->argCount));

    /* Print the parameters and local variables */
    argCount = thisMethod->argCount;
    for (i = 0, pointer = FRAMELOCALS(currentFP); pointer < (cell*)currentFP;
         i++, pointer++) {
        char *format = (i < argCount) ? "Argument[%d].......: %lx\n"
                                      : "Local[%d]..........: %lx\n";
        fprintf(stdout, format, i, *pointer);
    }
    for (i = 1, pointer = (cell*)(currentFP + 1); pointer <= currentSP;
            pointer++, i++) {
        fprintf(stdout,"Operand[%d]........: %lx\n", i, *pointer);
    }
}

/*=========================================================================
 * FUNCTION:      printStackTrace()
 * TYPE:          public debugging operation
 * OVERVIEW:      Prints the contents of all the stack frames
 *                for debugging purposes.
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 * NOTE:          This operation destroys all the stack frames
 *                so VM execution must be terminated after printing.
 *=======================================================================*/

static void printStackTraceRange(int lower, int count)
{
    FRAME          currentFP = getFP();
    unsigned char* currentIP = getIP();
    cell         * currentSP = getSP();
    int upper = lower + count;
    int i;
    for (i = 0; currentFP != NULL && i < upper; i++) { 
        if (i >= lower) {  
            printFrame(currentFP, currentIP, currentSP);
            fprintf(stdout,"\n");
        }
        if (currentFP->previousIp == KILLTHREAD) {
            break;
        }
        currentIP = currentFP->previousIp;
        currentSP = currentFP->previousSp;
        currentFP = currentFP->previousFp;
    }
}

void printStackTrace()
{
    printStackTraceRange(0, 1000);
}

/*=========================================================================
 * FUNCTION:      printExecutionStack()
 * TYPE:          public debugging operation
 * OVERVIEW:      Prints the contents of the whole execution stack
 *                for debugging purposes.
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

void printExecutionStack()
{
    cell* i;
    STACK stack;

    if (CurrentThread == NULL) { 
        return;
    }
    for (i = 0, stack = CurrentThread->stack; 
         stack != NULL; 
         stack = stack->next
        ) {
        if (STACK_CONTAINS(stack, getSP())) {
            i += getSP() - stack->cells + 1;
            break;
        } else {
            i += stack->size;
        }
    }

    fprintf(stdout, "Execution stack contains %ld items: \n", (long)i);

    for (stack = CurrentThread->stack; stack != NULL; stack = stack->next) {
        if (STACK_CONTAINS(stack, getSP())) {
            for (i = stack->cells; i <= getSP(); i++)
                fprintf(stdout, "%lx  \n", *i);
            break;
        } else {
            for (i = stack->cells; i < stack->cells + stack->size; i++)
                fprintf(stdout, "%lx  \n", *i);
        }
    }
    fprintf(stdout, "\n");
}

#endif /* INCLUDEDEBUGCODE */

/*=========================================================================
 * Exception handler debugging and tracing operations
 *=======================================================================*/

#if INCLUDEDEBUGCODE

/*=========================================================================
 * FUNCTION:      printHandler()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Print the contents of the given exception handler
 *                for debugging purposes.
 * INTERFACE:
 *   parameters:  pointer to an exception handler object.
 *   returns:     <nothing>
 *=======================================================================*/

static void
printHandler(HANDLER thisHandler) {
    fprintf(stdout, "Exception handler at %lx:\n", (long)thisHandler);
    fprintf(stdout, "\tStart PC..: %ld\n", (long)thisHandler->startPC);
    fprintf(stdout, "\tEnd PC....: %ld\n", (long)thisHandler->endPC);
    fprintf(stdout, "\tHandler PC: %ld\n", (long)thisHandler->handlerPC);
    fprintf(stdout, "\tException.: %ld \n", (long)thisHandler->exception);
    if (thisHandler->exception == 0) fprintf(stdout, "(finally)\n");
    fprintf(stdout, "\n");
}

/*=========================================================================
 * FUNCTION:      printExceptionHandlerTable()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Print the contents of the given exception handler
 *                table for debugging purposes.
 * INTERFACE:
 *   parameters:  pointer to an exception handler table
 *   returns:     <nothing>
 *=======================================================================*/

void printExceptionHandlerTable(HANDLERTABLE handlerTable) {

    FOR_EACH_HANDLER(thisHandler, handlerTable)
        printHandler(thisHandler);
    END_FOR_EACH_HANDLER
}

#endif /* INCLUDEDEBUGCODE */

