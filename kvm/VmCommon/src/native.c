/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Native function interface
 * FILE:      native.c
 * OVERVIEW:  This file defines the interface for plugging in
 *            the native functions needed by the Java Virtual
 *            Machine. The implementation here is _not_ based
 *            on JNI (Java Native Interface), because JNI seems
 *            too expensive for small devices.
 * AUTHOR:    Antero Taivalsaari, Sun Labs, 1998
 *            Edited by Doug Simon 11/1998
 *            Frank Yellin
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>

/*=========================================================================
 * Variables
 *=======================================================================*/

/* This variable is set to point to the current native */
/* method during native function calls.  The variable  */
/* is used for adjusting exception handler ranges in   */
/* frame.c, as well as helping access local variables  */
/* in KNI calls.  It is important that this variable   */
/* is kept NULL when the VM is not executing a native  */
/* method */
METHOD CurrentNativeMethod;

/*=========================================================================
 * Operations on native functions
 *=======================================================================*/

int xstrcmp(const char *s1, const char *s2) {
    if (s1 == NULL) {
        s1 = "";
    }
    if (s2 == NULL) {
       s2 = "";
    }
    return strcmp(s1, s2);
}

/*=========================================================================
 * FUNCTION:      getNativeFunction()
 * TYPE:          lookup operation
 * OVERVIEW:      Given a function name as a string, try to find
 *                a corresponding native function and return a
 *                a pointer to it if found.
 * INTERFACE:
 *   parameters:  class name, method name
 *   returns:     function pointer or NIL if not found.
 *=======================================================================*/

NativeFunctionPtr
getNativeFunction(INSTANCE_CLASS clazz, const char* methodName, 
                                        const char *methodSignature)
{
#if !ROMIZING
    const ClassNativeImplementationType *cptr;
    const NativeImplementationType *mptr;
    UString UBaseName    = clazz->clazz.baseName;
    UString UPackageName = clazz->clazz.packageName;
    char* baseName;
    char* packageName;

    /* Package names can be NULL -> must do an explicit check */
    /* to ensure that string comparison below will not fail */
    if (UPackageName == NULL) {
        packageName = "";
    } else {
        packageName = UStringInfo(UPackageName);
    }

    if (UBaseName == NULL) {
        baseName = "";
    } else {
        baseName = UStringInfo(UBaseName);
    }

    for (cptr = nativeImplementations; cptr->baseName != NULL ; cptr++) {                 
        if (   (xstrcmp(cptr->packageName, packageName) == 0) 
            && (xstrcmp(cptr->baseName, baseName) == 0)) { 
            break;
        }
    }

    for (mptr = cptr->implementation; mptr != NULL ; mptr++) {
        const char *name = mptr->name;
        if (name == NULL) {
            return NULL;
        }
        if (strcmp(name, methodName) == 0) {
            const char *signature = mptr->signature;
            /* The signature is NULL for non-overloaded native methods. */
            if (signature == NULL || (xstrcmp(signature, methodSignature) == 0)){
                return mptr->implementation;
            }
        }
    }
#endif /* !ROMIZING */

    return NULL;
}

/*=========================================================================
 * FUNCTION:      invokeNativeFunction()
 * TYPE:          private operation
 * OVERVIEW:      Invoke a native function, resolving the native
 *                function reference if necessary.
 * INTERFACE:
 *   parameters:  method pointer
 *   returns:     <nothing>
 * NOTE:          Native functions are automatically synchronized, i.e.,
 *                they cannot be interrupted by other threads unless
 *                the native code happens to invoke the Java interpreter.
 *=======================================================================*/

void invokeNativeFunction(METHOD thisMethod)
{
#if INCLUDEDEBUGCODE
    int saved_TemporaryRootsLength;
#endif
    NativeFunctionPtr native = thisMethod->u.native.code;

    if (native == NULL) {
        /* Native function not found; throw error */

        /* The GC may get confused by the arguments on the stack */
        setSP(getSP() - thisMethod->argCount);

        START_TEMPORARY_ROOTS
            DECLARE_TEMPORARY_ROOT(char*, className, 
                     getClassName((CLASS)(thisMethod->ofClass)));
            sprintf(str_buffer,
                    KVM_MSG_NATIVE_METHOD_NOT_FOUND_2STRPARAMS,
                    className, methodName(thisMethod));
        END_TEMPORARY_ROOTS
        fatalError(str_buffer);
    }

#if INCLUDEDEBUGCODE
    if (tracemethodcalls || tracemethodcallsverbose) {
        frameTracing(thisMethod, "=>", +1);
    }
    saved_TemporaryRootsLength = TemporaryRootsLength;
#endif

#if USE_KNI
    /* Note: Unlike many other Java VMs, KVM does not create      */
    /* stack frames for native function calls.  This makes native */
    /* function calls faster.  However, at the same time it means */
    /* that the native function programmer must be careful not to */
    /* screw up the stack pointer when popping and pushing items  */
    /* to and from the operand stack.                             */

    /* Because of the lack of stack frames for native functions,  */
    /* KNI calls need special assistance to access the parameters */
    /* of a native method.  Basically, we create a new "pseudo-   */
    /* local pointer" called "nativeLp" to point to the locals of */
    /* the native method.  Old-style (pre-KNI) native methods can */
    /* simply ignore this new variable.                           */
    if ((thisMethod->accessFlags & ACC_STATIC) && CurrentThread) {
        CurrentThread->nativeLp = getSP() - (thisMethod->argCount);
    } else {
        CurrentThread->nativeLp = getSP() - (thisMethod->argCount-1);
    }
#endif /* USE_KNI */

    /* Call the native function we are supposed to call */
    CurrentNativeMethod = thisMethod;
    native();

#if USE_KNI
    if (CurrentThread) {
        /* Remember to reset the native LP so that garbage collector */
        /* doesn't have to do redundant work                         */        
        CurrentThread->nativeLp = NULL;

        /* Check for pending exceptions (KNI_Throw) */
        if (CurrentThread->pendingException) {
            const char* pending = CurrentThread->pendingException;
            CurrentThread->pendingException = NULL;
            if (CurrentThread->exceptionMessage) {
                const char* message = CurrentThread->exceptionMessage;
                CurrentThread->exceptionMessage = NULL;
                raiseExceptionWithMessage(pending, message);
            } else {
                raiseException(pending);
            }
        }
    }
#endif /* USE_KNI */

    /* Reset the global native function pointer. */
    /* This cannot be done until we have thrown */
    /* the possible exception/error above.     */
    CurrentNativeMethod = NULL;

#if INCLUDEDEBUGCODE
    if (TemporaryRootsLength != saved_TemporaryRootsLength) { 
        START_TEMPORARY_ROOTS
            DECLARE_TEMPORARY_ROOT(char*, className, 
                     getClassName((CLASS)(thisMethod->ofClass)));
            sprintf(str_buffer,
                    KVM_MSG_NATIVE_METHOD_BAD_USE_OF_TEMPORARY_ROOTS,
                    className, methodName(thisMethod));
        END_TEMPORARY_ROOTS
        fatalError(str_buffer);
    }

    if (tracemethodcalls || tracemethodcallsverbose) {
        frameTracing(thisMethod, "<=", +1);
    }

#endif
    
}

