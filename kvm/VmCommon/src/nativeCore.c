/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Native function interface
 * FILE:      nativeCore.c
 * OVERVIEW:  This file defines the native functions needed
 *            by the Java virtual machine. The implementation
 *            is _not_ based on JNI (Java Native Interface),
 *            because JNI seems too expensive for small devices.
 * AUTHOR:    Antero Taivalsaari, Sun Labs, 1998
 *            Edited by Doug Simon 11/1998
 *            Frank Yellin
 *            Support for CLDC 1.1 weak references, A. Taivalsaari 3/2002
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>

/*=========================================================================
 * FUNCTION:      getClass()Ljava/lang/Class (VIRTUAL)
 * CLASS:         java.lang.Object
 * TYPE:          virtual native function (reflection)
 * OVERVIEW:      Return the class object of the given object
 * INTERFACE (operand stack manipulation):
 *   parameters:  an object instance ('this')
 *   returns:     a class object
 * NOTE:          Current implementation does not create array
 *                classes yet, so arrays return JavaLangObject.
 *=======================================================================*/

void Java_java_lang_Object_getClass(void)
{
    topStackAsType(CLASS) = topStackAsType(OBJECT)->ofClass;
}

/*=========================================================================
 * FUNCTION:      hashCode()I, identityHashCode()I (VIRTUAL)
 * CLASS:         java.lang.Object, java.lang.System
 * TYPE:          virtual native function
 * OVERVIEW:      Generate a unique hash code for an object.
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     hash code
 *=======================================================================*/

void Java_java_lang_Object_hashCode(void)
{
    /* The following must be two lines:
     * objectHashCode can GC, and a compiler is allowed to assume that the
     * value of sp hasn't changed if it is all on one line.
     */
    OBJECT object = popStackAsType(OBJECT);
    long result = objectHashCode(object); /* this may GC */
    pushStack(result);
}

void Java_java_lang_System_identityHashCode(void)
{
    OBJECT object = popStackAsType(OBJECT);
    long result = object == NULL ? 0 : objectHashCode(object); /* this may GC */
    pushStack(result);
}

/*=========================================================================
 * FUNCTION:      notify()V (VIRTUAL)
 * CLASS:         java.lang.Object
 * TYPE:          virtual native function
 * OVERVIEW:      Release the first thread in the object's monitor.
 * INTERFACE (operand stack manipulation):
 *   parameters:  an object instance ('this')
 *   returns:     <nothing>
 * NOTE:          This operation does nothing
 *                if the current thread is not the owner of the monitor.
 *=======================================================================*/

void Java_java_lang_Object_notify(void)
{
    OBJECT object = popStackAsType(OBJECT);
    monitorNotify(object, FALSE);
}

/*=========================================================================
 * FUNCTION:      notifyAll()V (VIRTUAL)
 * CLASS:         java.lang.Object
 * TYPE:          virtual native function
 * OVERVIEW:      Release all threads in the object's monitor.
 * INTERFACE (operand stack manipulation):
 *   parameters:  an object instance ('this')
 *   returns:     <nothing>
 * NOTE:          This operation does nothing
 *                if the current thread is not the owner of the monitor.
 *=======================================================================*/

void Java_java_lang_Object_notifyAll(void)
{
    OBJECT object = popStackAsType(OBJECT);
    monitorNotify(object, TRUE);
}

/*=========================================================================
 * FUNCTION:      wait()V (VIRTUAL)
 * CLASS:         java.lang.Object
 * TYPE:          virtual native function
 * OVERVIEW:      Wait in the object's monitor forever.
 * INTERFACE (operand stack manipulation):
 *   parameters:  an object instance ('this').
 *   returns:     <nothing>
 * NOTE:          This operation should throw IllegalMonitorState-
 *                Exception if the current thread is not the owner of
 *                the monitor. Other possible exceptions are:
 *                IllegalArgumentException, InterruptedException.
 *=======================================================================*/

void Java_java_lang_Object_wait(void)
{
    long64  period;
    OBJECT object;

    popLong(period);
    object = popStackAsType(OBJECT);

    /* only block if the time period is not zero */
    if (ll_zero_ge(period)) {
        monitorWait(object, period);
    } else {
        raiseException(IllegalArgumentException);
    }
}

/*=========================================================================
 * Native functions of class java.lang.Math
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      randomInt()I (STATIC)
 * CLASS:         java.lang.Math
 * TYPE:          static native function
 * OVERVIEW:      Return a random integer value
 * INTERFACE (operand stack manipulation):
 *   parameters:  none
 *   returns:     random integer
 *=======================================================================*/

void Java_java_lang_Math_randomInt(void)
{
    long result = RandomNumber_md();
    pushStack(result);
}

/*=========================================================================
 * Native functions of class java.lang.Class
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      isInterface()B (VIRTUAL)
 * CLASS:         java.lang.Class
 * TYPE:          virtual native function
 * OVERVIEW:      Return true if the class is an interface class.
 * INTERFACE (operand stack manipulation):
 *   parameters:  'this' pointer
 *   returns:     Boolean indicating if the class is an interface.
 *=======================================================================*/

void Java_java_lang_Class_isInterface(void)
{
    CLASS clazz = topStackAsType(CLASS);
    topStack = ((clazz->accessFlags & ACC_INTERFACE) != 0);
}

/*=========================================================================
 * FUNCTION:      isPrimitive()B (VIRTUAL)
 * CLASS:         java.lang.Class
 * TYPE:          virtual native function
 * OVERVIEW:      Return true if the class is a primitive class.
 * INTERFACE (operand stack manipulation):
 *   parameters:  'this' pointer
 *   returns:     Boolean indicating if the class is a primitive class.
 *=======================================================================*/

void Java_java_lang_Class_isPrimitive(void)
{
    topStack = FALSE; /* We do not support this feature */
}

/*=========================================================================
 * FUNCTION:      forName(Ljava/lang/String;)Ljava/lang/Class; (STATIC)
 * CLASS:         java.lang.Class
 * TYPE:          static native function (reflection)
 * OVERVIEW:      Given a string containing a class name (e.g.,
 *                "java.lang.Thread", return the corresponding
 *                class object.
 * INTERFACE (operand stack manipulation):
 *   parameters:  a string object
 *   returns:     a class object
 *=======================================================================*/

void Java_java_lang_Class_forName(void)
{
    STRING_INSTANCE string = topStackAsType(STRING_INSTANCE);
    if (string == NULL) {
        raiseException(NullPointerException);
    } else {
        START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(const char*,className,
               getStringContents((STRING_INSTANCE)string));
        CLASS thisClass = NULL;
        if (!strchr(className, '/')) {
            replaceLetters((char*)className,'.','/');
            if (isValidName(className, LegalClass)) {
                thisClass = getRawClassX(&className,0,strlen(className));
                if (!IS_ARRAY_CLASS(thisClass)) {
                    INSTANCE_CLASS iclass = (INSTANCE_CLASS)thisClass;
                    /*
                     * If the class is already in error, then
                     * NoClassDefFoundError must be thrown immediately.
                     */
                    if (iclass->status == CLASS_ERROR) {
                        raiseException(NoClassDefFoundError);
                    }
                    if (iclass->status == CLASS_RAW) {
                        /* Load the class; this may throw exceptions */
                        loadedReflectively = TRUE;
                        loadClassfile(iclass, TRUE);
                    }
                    /* The specification does not require that the current
                     * class have "access" to thisClass */
                    topStackAsType(INSTANCE_CLASS) = iclass;

                    if (!CLASS_INITIALIZED(iclass)) {
                        /* Initialize the class; this may push another frame */
                        initializeClass(iclass);
                    }
                } else {
                    topStackAsType(CLASS) = thisClass;
                }
            }
        }
        if (thisClass == NULL)
            raiseException(ClassNotFoundException);
        END_TEMPORARY_ROOTS
    }
}

/*=========================================================================
 * FUNCTION:      newInstance()Ljava/lang/Object; (VIRTUAL)
 * CLASS:         java.lang.Class
 * TYPE:          virtual native function (reflection)
 * OVERVIEW:      Instantiate an object of the given class
 * INTERFACE (operand stack manipulation):
 *   parameters:  'this'
 *   returns:     an instance of the class represented by 'this'
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      newInstance()Ljava/lang/Object; (VIRTUAL)
 * CLASS:         java.lang.Class
 * TYPE:          virtual native function (reflection)
 * OVERVIEW:      Instantiate an object of the given class
 * INTERFACE (operand stack manipulation):
 *   parameters:  'this'
 *   returns:     an instance of the class represented by 'this'
 * NOTE:          Current implementation does not create array
 *                classes properly.
 *=======================================================================*/

static void
newInstanceReturnObject(FRAME_HANDLE exceptionFrameH)
{
    if (exceptionFrameH == NULL) {
        popFrame();
    } else {
        /* We have no interest in dealing with exceptions */
    }
}

void Java_java_lang_Class_newInstance(void)
{
    INSTANCE_CLASS currentClass = getFP()->thisMethod->ofClass;
    CLASS clazz = topStackAsType(CLASS);

    if (IS_ARRAY_CLASS(clazz)
        || ((clazz->accessFlags & (ACC_INTERFACE | ACC_ABSTRACT)) != 0)) {
        raiseException(InstantiationException);
        return;
    }

    if (classHasAccessToClass(currentClass, clazz)) {
        METHOD method = lookupMethod(clazz, initNameAndType, currentClass);
        if (   (method != NULL)
            && (method->ofClass == (INSTANCE_CLASS)clazz)
               /*
                * We're not allowed access to a protected constructor (even if
                * CurrentClass is a subclass of clazz). Protected constructors
                * can only be accessed from within the constructor of a direct
                * subclass of clazz and they cannot be accessed via
                * reflection (even if we are currently in the constructor
                * of a direct subclass of clazz).
                */
            && classHasAccessToMember(currentClass,
                   (method->accessFlags & ~ACC_PROTECTED),
                   (INSTANCE_CLASS)clazz, (INSTANCE_CLASS)clazz)

            )
        {
            START_TEMPORARY_ROOTS
                DECLARE_TEMPORARY_ROOT(INSTANCE, object,
                                       instantiate((INSTANCE_CLASS)clazz));
                if (object != NULL) {
                    /*  Put the result back on the stack */
                    topStackAsType(INSTANCE) = object; /* Will get the result */
                    /* We now need to call the initializer.  We'd like to just
                     * push a second copy of the object onto the stack, and then
                     * do pushFrame(method).  But we can't, because that would
                     * not necessarily coincide with the stack map of the
                     * current method.
                     */
                    pushFrame(RunCustomCodeMethod);
                    pushStackAsType(CustomCodeCallbackFunction,
                                    newInstanceReturnObject);
                    pushStackAsType(INSTANCE, object);
                    /* pushFrame may signal a stack overflow.  */
                    pushFrame(method);
                } else {
                    /* We will already have thrown an appropriate error */
                }
            END_TEMPORARY_ROOTS
            return;
        }
    }
    raiseException(IllegalAccessException);
    return;
}

/*=========================================================================
 * FUNCTION:      getName()Ljava/lang/String; (VIRTUAL)
 * CLASS:         java.lang.Class
 * TYPE:          virtual native function
 * OVERVIEW:      Return the name of the class as a string.
 * INTERFACE (operand stack manipulation):
 *   parameters:  'this' pointer
 *   returns:     Name of the class as a string.
 *=======================================================================*/

void Java_java_lang_Class_getName(void)
{
    CLASS clazz = topStackAsType(CLASS);
    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(char*, name, getClassName((CLASS)clazz));
        STRING_INSTANCE result;
        char *ptr = strchr(name, '/');
        /* Replace all slashes in the string with dots */
        while (ptr != NULL) {
            *ptr = '.';
            ptr = strchr(ptr + 1, '/');
        }
        result = instantiateString(name, strlen(name));
        topStackAsType(STRING_INSTANCE) = result;
    END_TEMPORARY_ROOTS
}

/*=========================================================================
 * FUNCTION:      isArray()Z (VIRTUAL)
 * CLASS:         java.lang.Class
 * TYPE:          virtual native function
 * OVERVIEW:      true if the class is an array class
 * INTERFACE (operand stack manipulation):
 *   parameters:  'this' pointer
 *   returns:     true if the object is an instance.
 *=======================================================================*/

void Java_java_lang_Class_isArray(void)
{
    CLASS clazz = topStackAsType(CLASS);
    topStack = !!IS_ARRAY_CLASS(clazz);
    /* Double negation is intentional */
}

/*=========================================================================
 * FUNCTION:      isInstance(Ljava/lang/Object;)Z (VIRTUAL)
 * CLASS:         java.lang.Class
 * TYPE:          virtual native function
 * OVERVIEW:      true if the object is an instance of "this" class
 * INTERFACE (operand stack manipulation):
 *   parameters:  'this' pointer, object
 *   returns:     a boolean
 *=======================================================================*/

void Java_java_lang_Class_isInstance(void)
{
    OBJECT object = popStackAsType(OBJECT);
    CLASS thisClass = topStackAsType(CLASS);
    topStack = (object != NULL) && isAssignableTo(object->ofClass, thisClass);
}

/*=========================================================================
 * FUNCTION:      isAssignableFrom(Ljava/lang/Class;)Z (VIRTUAL)
 * CLASS:         java.lang.Class
 * TYPE:          virtual native function
 * OVERVIEW:      true if objects of the argument class can always be
 *                  assigned to variables of the "this" class.
 *   parameters:  'this' pointer, object
 *   returns:     a boolean
 *=======================================================================*/

void Java_java_lang_Class_isAssignableFrom(void)
{
    CLASS argClass = popStackAsType(CLASS);
    CLASS thisClass = topStackAsType(CLASS);
    if (argClass == NULL) {
        raiseException(NullPointerException);
    } else {
        topStack = !!isAssignableTo(argClass, thisClass);
        /* Double negation is intentional */
    }
}

/*=========================================================================
 * FUNCTION:      static byte[] getResourceAsStream0(String s);
 * CLASS:         java.lang.class
 * TYPE:          virtual native function
 * OVERVIEW:      Internal system function
 * INTERFACE
 *   parameters:
 *   returns:
 *
 * This is just a stub function, until the real definition is written
 *=======================================================================*/

void Java_java_lang_Class_getResourceAsStream0(void)
{
    topStackAsType(cell *) = NULL;
}

/*=========================================================================
 * Native functions of class java.lang.Thread
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      activeCount()I; (STATIC)
 * CLASS:         java.lang.Thread
 * TYPE:          static native function
 * OVERVIEW:      Return the number of threads alive.
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   returns:     the number of threads alive
 *=======================================================================*/

void Java_java_lang_Thread_activeCount(void)
{
    pushStack(activeThreadCount());
}

/*=========================================================================
 * FUNCTION:      currentThread()Ljava/lang/Thread; (STATIC)
 * CLASS:         java.lang.Thread
 * TYPE:          static native function
 * OVERVIEW:      Return pointer to the currently executing Java thread.
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   returns:     pointer to a JAVATHREAD object (instance of Thread.class)
 *=======================================================================*/

void Java_java_lang_Thread_currentThread(void)
{
    pushStackAsType(JAVATHREAD, CurrentThread->javaThread);
}

/*=========================================================================
 * FUNCTION:      yield()V (STATIC)
 * CLASS:         java.lang.Thread
 * TYPE:          static native function
 * OVERVIEW:      Force a task switch.
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

void Java_java_lang_Thread_yield(void)
{
    signalTimeToReschedule();
}

/*=========================================================================
 * FUNCTION:      sleep(J)V (STATIC)
 * CLASS:         java.lang.Thread
 * TYPE:          static native function
 * OVERVIEW:      Pause execution of the thread for a timer period
 * INTERFACE (operand stack manipulation):
 *   parameters:  Long of the number of ms to sleep for
 *=======================================================================*/

void Java_java_lang_Thread_sleep(void)
{
    long64  period;
    THREAD thisThread = CurrentThread;

    popLong(period);
    if (ll_zero_lt(period)) { 
        raiseException(IllegalArgumentException);
    } else  if (thisThread->isPendingInterrupt) { 
        handlePendingInterrupt();
    } else if (ll_zero_gt(period)) {
        /* Suspend the current thread before we add the timer */
        suspendThread();

        /* Now add the timer (this is the safe way) */
        registerAlarm(thisThread, period, resumeThread);
    } else if (ll_zero_eq(period)) {
        signalTimeToReschedule();
    }
}

/*=========================================================================
 * FUNCTION:      waitForIO()V (STATIC)
 * CLASS:         com.sun.cldc.io.Waiter
 * TYPE:          static native function
 * OVERVIEW:      Pause execution of the thread for a period
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

/*
 * The following native function is called by the I/O protocol classes to
 * suspend execution of a Java thread that is waiting for I/0. (This is only
 * done when the only way to achieve the effect of asynchronous I/O is
 * to poll an I/O stream.) The options here are to either put the Java thread
 * into a set period of sleep, or to simply yield to another Java thread. The
 * former option can be used (in conjunction with the regular thread scheduling
 * code) to force a task to sleep for a while in order to conserve power. The
 * latter option will produce a smaller latency restarting the thread then the
 * data is available.
 *
 * By default we simply yield
 */

#ifndef GENERIC_IO_WAIT_TIME
#define GENERIC_IO_WAIT_TIME 0
#endif

void Java_com_sun_cldc_io_Waiter_waitForIO(void)
{
#if GENERIC_IO_WAIT_TIME > 0
    /* Suspend the current thread for GENERIC_IO_WAIT_TIME milliseconds */
    THREAD thisThread = CurrentThread;
    suspendThread();
    registerAlarm(thisThread, (long64)GENERIC_IO_WAIT_TIME, resumeThread);
#else
    /* Try to switch to another thread */
    signalTimeToReschedule();
#endif
}

/*=========================================================================
 * FUNCTION:      start()V (VIRTUAL)
 * CLASS:         java.lang.Thread
 * TYPE:          virtual native function
 * OVERVIEW:      Start executing the 'run' method of the target
 *                object given to the thread.
 * INTERFACE (operand stack manipulation):
 *   parameters:  'this' pointer (implicitly)
 *   returns:     <nothing>
 *=======================================================================*/

void Java_java_lang_Thread_start(void)
{
    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(JAVATHREAD, javaThread,
                               popStackAsType(JAVATHREAD));
        DECLARE_TEMPORARY_ROOT(THREAD, VMthread, getVMthread(&javaThread));
        /*  Check if a separate Runnable object has been provided */
        INSTANCE target =
            (javaThread->target) ? javaThread->target : (INSTANCE)javaThread;
        METHOD thisMethod;

        /*  Ensure that the thread is not already active */
        if (VMthread->state != THREAD_JUST_BORN) {
            raiseException(IllegalThreadStateException);
            goto done;
        }

        /*  Find the 'run' method and set the thread to execute it */
        thisMethod =
            lookupMethod((CLASS)target->ofClass, runNameAndType,
                         target->ofClass);

        if (thisMethod == NULL) {
            raiseException("java/lang/Error");
            goto done;
        }

        /* May call pushFrame(), which can cause GC */
        initThreadBehavior(VMthread, thisMethod, (OBJECT)target);

        /*  Since 'run' is a virtual method, it must have  */
        /*  'this' in its first local variable */
        *(INSTANCE *)&VMthread->stack->cells[0] = target;

        /* Make the thread alive, and tell it that it can run */
        startThread(VMthread);
        resumeThread(VMthread);
 done:
    END_TEMPORARY_ROOTS
}

/*=========================================================================
 * FUNCTION:      isAlive()Z (VIRTUAL)
 * CLASS:         java.lang.Thread
 * TYPE:          virtual native function
 * OVERVIEW:      Check if a thread is active.
 * INTERFACE (operand stack manipulation):
 *   parameters:  'this' pointer (implicitly)
 *   returns:     true if thread is active, false otherwise
 *=======================================================================*/

void Java_java_lang_Thread_isAlive(void)
{
    topStack = isActivated(topStackAsType(JAVATHREAD)->VMthread);
}

/*=========================================================================
 * FUNCTION:      setPriority0(I)V (VIRTUAL)
 * CLASS:         java.lang.Thread
 * TYPE:          virtual native function
 * OVERVIEW:      Set the priority of the current thread.
 * INTERFACE (operand stack manipulation):
 *   parameters:  'this' pointer (implicitly), new priority
 *   returns:     <nothing>
 *=======================================================================*/

void Java_java_lang_Thread_setPriority0(void)
{
    int priority = popStack();
    THREAD VMthread;

    /*  Ensure that the internal thread execution */
    /*  structure has been created */
    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(JAVATHREAD, javaThread,
                               popStackAsType(JAVATHREAD));
        javaThread->priority = (priority > MAX_PRIORITY ? MAX_PRIORITY :
                   (priority < MIN_PRIORITY ? MIN_PRIORITY : priority));
        VMthread = getVMthread(&javaThread);
        /* The actual VM-level timeslice of the thread is calculated by
         * multiplying the given priority */
        VMthread->timeslice = javaThread->priority * TIMESLICEFACTOR;
    END_TEMPORARY_ROOTS
}

/*=========================================================================
 * FUNCTION:      interrupt0()V (VIRTUAL)
 * CLASS:         java.lang.Thread
 * TYPE:          virtual native function
 * OVERVIEW:      Interrupts the thread.
 * INTERFACE (operand stack manipulation):
 *   parameters:  'this' pointer (implicitly).  The 'this' pointer
 *                points to the Java thread to be interrupted.
 *   returns:     <nothing>
 *=======================================================================*/

void Java_java_lang_Thread_interrupt0(void)
{
    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(JAVATHREAD, javaThread,
                               popStackAsType(JAVATHREAD));
        DECLARE_TEMPORARY_ROOT(THREAD, VMthread, getVMthread(&javaThread));
        if (VMthread->state != THREAD_JUST_BORN &&
            VMthread->state != THREAD_DEAD) {
            interruptThread(VMthread);
        }
    END_TEMPORARY_ROOTS
}

/*=========================================================================
 * Native functions of class java.lang.Runtime
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      exit(I)V (VIRTUAL)
 * CLASS:         java.lang.Runtime
 * TYPE:          virtual native function
 * OVERVIEW:      Stop the VM and exit immediately.
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

void Java_java_lang_Runtime_exitInternal(void)
{
    long value = popStack();
    oneLess; /* Discard runtime object */
    VM_EXIT(value);
}

/*=========================================================================
 * FUNCTION:      freeMemory()J (VIRTUAL)
 * CLASS:         java.lang.Runtime
 * TYPE:          virtual native function
 * OVERVIEW:      Return the amount of free memory in the dynamic heap.
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   return:      the amount of free dynamic heap memory as long
 *=======================================================================*/

void Java_java_lang_Runtime_freeMemory(void)
{
    long result = memoryFree();
    ulong64 result64;
    ll_uint_to_long(result64, result);
    oneLess; /* Discard runtime object */
    pushLong(result64);
}

/*=========================================================================
 * FUNCTION:      totalMemory()J (VIRTUAL)
 * CLASS:         java.lang.Runtime
 * TYPE:          virtual native function
 * OVERVIEW:      Return the total amount of dynamic heap memory.
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   return:      the total amount of dynamic heap memory as long
 *=======================================================================*/

void Java_java_lang_Runtime_totalMemory(void)
{
    long result = getHeapSize();
    ulong64 result64;
    ll_uint_to_long(result64, result);

    oneLess; /* Discard runtime object */
    pushLong(result64);
}

/*=========================================================================
 * FUNCTION:      gc() (VIRTUAL)
 * CLASS:         java.lang.Runtime
 * TYPE:          virtual native function
 * OVERVIEW:      Invoke the garbage collector.
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   return:      <nothing>
 *=======================================================================*/

void Java_java_lang_Runtime_gc(void)
{
    oneLess; /* Discard runtime object */

    /*  Garbage collect now, keeping the heap size the same as currently */
    garbageCollect(0);
}

/*=========================================================================
 * Native functions of class java.lang.System
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      arraycopy()B (STATIC)
 * CLASS:         java.lang.System
 * TYPE:          static native function
 * OVERVIEW:      Copies a region of one array to another.
 *                Copies a region of one array, src, beginning at the
 *                array cell at src_position, to another array, dst,
 *                beginning at the dst_position.length cells are copied.
 *                No memory is allocated for the destination array, dst.
 *                This memory must already be allocated.
 *
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   return:      <none>
 *=======================================================================*/

void Java_java_lang_System_arraycopy(void)
{
    long length  = popStack();
    long dstPos  = popStack();
    ARRAY dst    = popStackAsType(ARRAY);
    long srcPos  = popStack();
    ARRAY src    = popStackAsType(ARRAY);
    ARRAY_CLASS srcClass, dstClass;

    long srcEnd, dstEnd;

    if ((src == NULL) || (dst == NULL)) {
        raiseException(NullPointerException);
        return;
    }
    srcClass = src->ofClass;
    dstClass = dst->ofClass;

    if (    (!IS_ARRAY_CLASS((CLASS)srcClass))
         || (!IS_ARRAY_CLASS((CLASS)dstClass))
         /* both must be arrays of Objects, or arrays of primitives */
         || (srcClass->gcType != dstClass->gcType)
         /* If arrays of primitives, must be the same primitive */
         || (srcClass->gcType == GCT_ARRAY
               && (srcClass->u.elemClass != dstClass->u.elemClass))
            ) {
        raiseException(ArrayStoreException);
        return;
    }

    srcEnd = length + srcPos;
    dstEnd = length + dstPos;

    if (     (length < 0) || (srcPos < 0) || (dstPos < 0)
          || (length > 0 && (srcEnd < 0 || dstEnd < 0))
          || (srcEnd > (long)src->length)
          || (dstEnd > (long)dst->length)) {
        raiseException(ArrayIndexOutOfBoundsException);
        return;
    }

    if (src->ofClass->gcType == GCT_ARRAY) {
        /* We've already determined that src and dst are primitives of the
         * same type */
        long itemSize = src->ofClass->itemSize;
        memmove(&((BYTEARRAY)dst)->bdata[dstPos * itemSize],
                &((BYTEARRAY)src)->bdata[srcPos * itemSize],
                itemSize * length);
    } else {
        CLASS srcElementClass = srcClass->u.elemClass;
        CLASS dstElementClass = dstClass->u.elemClass;
        if (!isAssignableTo(srcElementClass, dstElementClass)) {
            /* We have to check each element to make sure it can be
             * put into an array of dstElementClass */
            long i;
            for (i = 0; i < length; i++) {
                OBJECT item = (OBJECT)src->data[srcPos + i].cellp;
                if ((item != NULL) && !isAssignableTo(item->ofClass, dstElementClass)) {
                    raiseException(ArrayStoreException);
                    break;
                } else {
                    dst->data[dstPos + i].cellp = (cell *)item;
                }
            }
        } else {
            memmove(&dst->data[dstPos], &src->data[srcPos],
                    length << log2CELL);
        }
    }
}

/*=========================================================================
 * FUNCTION:      currentTimeMillis()I (STATIC)
 * CLASS:         java.lang.System
 * TYPE:          static native function
 * OVERVIEW:      Return current time in milliseconds.
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   returns:     current time in centisecs in operand stack as integer.
 *=======================================================================*/

#ifdef BEDETERMINISTIC
ulong64 last;

void Java_java_lang_System_currentTimeMillis(void)
{
    /* Debugging use only.  The value
     * has no meaning in the real world.
     */
    last += 1000;
    pushLong(last);
}

#else

void Java_java_lang_System_currentTimeMillis(void)
{
    ulong64 result = CurrentTime_md();
    pushLong(result);
}

#endif /* BEDETERMINISTIC */

/*=========================================================================
 * FUNCTION:      getProperty0(Ljava.lang.String)Ljava.lang.String (STATIC)
 * CLASS:         java.lang.System
 * TYPE:          static native function
 * OVERVIEW:      Return a property
 * INTERFACE (operand stack manipulation):
 *   parameters:  A string
 *   returns:     A string
 *=======================================================================*/

void Java_java_lang_System_getProperty0(void)
{
    STRING_INSTANCE string = topStackAsType(STRING_INSTANCE);
    STRING_INSTANCE result = NULL;
    if (string->length < STRINGBUFFERSIZE - 1) {
        char* key = getStringContents(string);
        char* value = getSystemProperty(key);
        if (value != NULL) {
            result = instantiateString(value, strlen(value));
        }
    }
    topStackAsType(STRING_INSTANCE) = result;
}

/*=========================================================================
 * Native functions in java.lang.ref (since CLDC 1.1)
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      initializeWeakReference()V (VIRTUAL)
 * CLASS:         java.lang.ref.WeakReference
 * TYPE:          virtual native function
 * OVERVIEW:      Initialize a weak reference
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

void Java_java_lang_ref_WeakReference_initializeWeakReference(void)
{
    /* This implementation is very KVM-specific: */

    /* We've added a new garbage collection (GCT) */
    /* type specifically for weak references. */

    /* We need to convert the object from a regular */
    /* instance (GCT_INSTANCE) to GCT_WEAKREFERENCE. */
    /* We do this by updating the header word of the */
    /* garbage collector. */

    /* First, read the 'this' pointer */
    cell* instance = popStackAsType(cell*);

    /* Find the garbage collector header word location */
    cell* header = instance-HEADERSIZE;

    /* Read the header and blow away old GCT type info */
    cell headerData = *header & ~TYPEMASK;

    /* Store new GCT type info into the header */
    *header = headerData | (GCT_WEAKREFERENCE << TYPE_SHIFT);
}

/*=========================================================================
 * Native functions in com.sun.cldc.io.*
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      write(C)V (STATIC)
 * CLASS:         com/sun/kvm/io/ConsoleOutputStream
 * TYPE:          static native function
 * OVERVIEW:      Print a byte to the stdout file
 * INTERFACE (operand stack manipulation):
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

void Java_com_sun_cldc_io_ConsoleOutputStream_write(void)
{
    int c = popStack();
    putchar(c);
#ifdef POCKETPC
    fprintf(stdout, "%c", c);
#endif
}

/*=========================================================================
 * Native functions of class java.util.Calendar
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      init
 * CLASS:         java.util.Calendar
 * TYPE:          virtual native function
 * OVERVIEW:      Initialize Calendar object by calling a platform
 *                specific initialization function.
 * INTERFACE (operand stack manipulation):
 *   parameters:  see java.util.Calendar
 *   returns:     ditto
 *====================================================================*/

void Java_java_util_Calendar_init(void)
{
    unsigned long *date;

    ARRAY dateArray = popStackAsType(ARRAY);
    oneLess; /* Ignore "this" */

    /* initialize the dateArray */
    memset(&dateArray->data[0], 0,
           MAXCALENDARFLDS*sizeof(dateArray->data[0]));

    date = Calendar_md();

    /* copy over the date array back */
    memcpy(&dateArray->data[0], date,
           MAXCALENDARFLDS*sizeof(dateArray->data[0]));
}

/*=========================================================================
 * FUNCTION:      fillInStackTrace
 * CLASS:         java.lang.Throwable
 * TYPE:          virtual native function
 * OVERVIEW:      save information about the current stack
 *                including the method pointers for each stack frame
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *   returns:     this
 *====================================================================*/

void Java_java_lang_Throwable_fillInStackTrace() {
#if PRINT_BACKTRACE
    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(THROWABLE_INSTANCE, exception,
                               topStackAsType(THROWABLE_INSTANCE));
        fillInStackTrace(&exception);
    END_TEMPORARY_ROOTS
#else
    lessStack(1);
#endif
}

/*=========================================================================
 * FUNCTION:      printStackTrace0
 * CLASS:         java.lang.Throwable
 * TYPE:          virtual native function
 * OVERVIEW:      print class, message and stack trace
 * INTERFACE (operand stack manipulation):
 *   parameters:  this
 *                stream to print to (unused)
 *   returns:     void
 *====================================================================*/

void Java_java_lang_Throwable_printStackTrace0() {
    START_TEMPORARY_ROOTS
        OBJECT unused = popStackAsType(OBJECT);
        DECLARE_TEMPORARY_ROOT(THROWABLE_INSTANCE, throwable,
                               popStackAsType(THROWABLE_INSTANCE));
        unused = unused;
        /* If PRINT_BACKTRACE is 0, this will print a message saying
         * that stack tracing isn't available.
         */
        printExceptionStackTrace(&throwable);
    END_TEMPORARY_ROOTS
}

/*=========================================================================
 * Native functions of classes java.util.String and StringBuffer
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      charAt()
 * CLASS:         java.lang.String
 * TYPE:          virtual native function
 * OVERVIEW:      returns the character at a particular index
 * INTERFACE
 *   parameters:  this, index
 *   returns:     character
 *====================================================================*/

void Java_java_lang_String_charAt() {
    long index = popStack();
    STRING_INSTANCE this = popStackAsType(STRING_INSTANCE);
    unsigned long length = this->length;

    if (index < 0 || index >= length) {
        raiseException(StringIndexOutOfBoundsException);
    } else {
        long result =
            ((unsigned short *)(this->array->sdata))[this->offset + index];
        pushStack(result);
    }
}

/*=========================================================================
 * FUNCTION:      string_indexOf
 * CLASS:         java.lang.String
 * TYPE:          helper function
 * OVERVIEW:      this function is common code used by both
 *                  String.indexOf(ch) and String.indexOf(ch, from)
 * INTERFACE
 *   parameters:  fromIndex
 *   returns:     index
 *====================================================================*/

static void string_indexOf(long fromIndex) {
    long ch = popStack();
    STRING_INSTANCE this = popStackAsType(STRING_INSTANCE);
    long offset = this->offset;
    long length = this->length;
    long result = -1;
    long i;

    if (fromIndex < length) {
        SHORTARRAY array = this->array;
        /* Max is the largest position in the underlying char array */
        long max = offset + length;
        for (i = offset + fromIndex; i < max; i++) {
            if (((unsigned short *)(array->sdata))[i] == ch) {
                /* i is the position in the underlying char array.  We must
                 * normalize it to the position in the String
                 */
                result = i - offset;
                break;
            }
        }
    }
    pushStack(result);
}

/*=========================================================================
 * FUNCTION:      indexOf(I)
 * CLASS:         java.lang.String
 * TYPE:          virtual native function
 * OVERVIEW:      returns index of a character
 *   parameters:  this
 *                character
 *   returns:     index of the first occurrence of that character
 *====================================================================*/

void Java_java_lang_String_indexOf__I(void) {
    string_indexOf(0);
}

/*=========================================================================
 * FUNCTION:      indexOf(II)
 * CLASS:         java.lang.String
 * TYPE:          virtual native function
 * OVERVIEW:      returns index of a character
 *   parameters:  this
 *                character
 *                fromIndex:  first location to look for that character
 *   returns:     index of the first occurrence of that character
 *====================================================================*/

void Java_java_lang_String_indexOf__II(void) {
    long fromIndex = popStackAsType(long);
    if (fromIndex < 0) {
        fromIndex = 0;
    }
    string_indexOf(fromIndex);
}

/*=========================================================================
 * FUNCTION:      equals(Object)
 * CLASS:         java.lang.String
 * TYPE:          virtual native function
 * OVERVIEW:      returns index of a character
 *   parameters:  this
 *                otherObject
 *   returns:     returns TRUE if the other object is a String with
 *                the identical characters
 *==================================================================== */

void Java_java_lang_String_equals() {
    OBJECT otherX = popStackAsType(OBJECT);
    STRING_INSTANCE this = popStackAsType(STRING_INSTANCE);
    bool_t result = FALSE;

    if ((OBJECT)this == otherX) {
        /* This case happens often enough that we should handle it quickly */
        result = TRUE;
    } else if ((otherX != NULL) && (otherX->ofClass == (CLASS)JavaLangString)){
        /* The other object must be a java.lang.String */
        STRING_INSTANCE other = (STRING_INSTANCE)otherX;
        unsigned long length = this->length;
        if (length == other->length) {
            /* Both objects must have the same length */
            if (0 == memcmp(&this->array->sdata[this->offset],
                            &other->array->sdata[other->offset],
                            length * sizeof(this->array->sdata[0]))) {
                /* Both objects must have the same characters */
                result = TRUE;
            }
        }
    }
    pushStack(result);
}

/*=========================================================================
 * FUNCTION:      intern()
 * CLASS:         java.lang.String
 * TYPE:          virtual native function
 * OVERVIEW:      returns a canonical representation for the string object
 *   parameters:  this
 *   returns:     the interned string
 *==================================================================== */

void Java_java_lang_String_intern() {
    STRING_INSTANCE this = popStackAsType(STRING_INSTANCE); /* volatile! */
    INTERNED_STRING_INSTANCE result;
    int this_offset = this->offset;
    int this_length = this->length;
    START_TEMPORARY_ROOTS
      DECLARE_TEMPORARY_ROOT(SHORTARRAY, this_array, this->array);
      int utflen = unicode2utfstrlen((unsigned short *)&this_array->sdata[this_offset],
                                     this_length) + 1;
      DECLARE_TEMPORARY_ROOT(char*, buf, mallocBytes(utflen));
      unicode2utf((unsigned short *)&this_array->sdata[this_offset], this_length, buf, utflen);
      result = internString(buf, strlen(buf));
    END_TEMPORARY_ROOTS
    pushStackAsType(INTERNED_STRING_INSTANCE, result);
}

/*=========================================================================
 * FUNCTION:      append(String)
 * CLASS:         java.lang.StringBuffer
 * TYPE:          virtual native function
 * OVERVIEW:      appends a String to a StringBuffer
 *   parameters:  this
 *                String
 *   returns:     the "this" argument
 *==================================================================== */

typedef struct stringBufferInstanceStruct {
    COMMON_OBJECT_INFO(INSTANCE_CLASS)
    SHORTARRAY array;
    long count;
    bool_t shared;
} *STRINGBUFFER_INSTANCE, **STRINGBUFFER_HANDLE;

static void expandStringBufferCapacity(STRINGBUFFER_HANDLE, int size);

void Java_java_lang_StringBuffer_append__Ljava_lang_String_2(void) {
    STRING_INSTANCE string = popStackAsType(STRING_INSTANCE);
    STRINGBUFFER_INSTANCE this = popStackAsType(STRINGBUFFER_INSTANCE);
    unsigned long count, newCount, stringLength;

    if (string == NULL) {
        START_TEMPORARY_ROOTS
            /* This seems to be the right thing to do */
            DECLARE_TEMPORARY_ROOT(STRINGBUFFER_INSTANCE, thisX, this);
            string = (STRING_INSTANCE)internString("null", 4);
            this = thisX;
        END_TEMPORARY_ROOTS
    }

    count = this->count;
    stringLength = string->length;
    newCount = count + stringLength;
    if (newCount > this->array->length) {
        /* We have to expand the string buffer array.  This may GC */
        START_TEMPORARY_ROOTS
            DECLARE_TEMPORARY_ROOT(STRING_INSTANCE, stringX, string);
            DECLARE_TEMPORARY_ROOT(STRINGBUFFER_INSTANCE, thisX, this);
            expandStringBufferCapacity(&thisX, newCount);
            string = stringX;
            this = thisX;
        END_TEMPORARY_ROOTS
    }
    if (newCount <= this->array->length) {
        memcpy(&this->array->sdata[count],
               &string->array->sdata[string->offset],
               stringLength * sizeof(short));
        this->count = newCount;
        pushStackAsType(STRINGBUFFER_INSTANCE, this);
    } else {
        /* We will have gotten an OutOfMemoryException from the
         * expandStringBufferCapacity
         */
    }
}

/*=========================================================================
 * FUNCTION:      append(int)
 * CLASS:         java.lang.StringBuffer
 * TYPE:          virtual native function
 * OVERVIEW:      appends an integer to a string buffer
 *   parameters:  this
 *                int
 *   returns:     the "this" argument
 *==================================================================== */

void
Java_java_lang_StringBuffer_append__I(void) {
    long value = popStack();
    STRINGBUFFER_INSTANCE this = popStackAsType(STRINGBUFFER_INSTANCE);

    unsigned long count, newCount, stringLength;
    char buffer[16];            /* 12 is enough, but I'm paranoid */
    long i;

    /* Put the value into a buffer */
    sprintf(buffer, "%ld", value);

    count = this->count;
    stringLength = strlen(buffer);
    newCount = count + stringLength;
    if (newCount > this->array->length) {
        /* Need to expand the string buffer */
        START_TEMPORARY_ROOTS
            DECLARE_TEMPORARY_ROOT(STRINGBUFFER_INSTANCE, thisX, this);
            expandStringBufferCapacity(&thisX, newCount);
            this = thisX;
        END_TEMPORARY_ROOTS
    }
    if (newCount <= this->array->length) {
        for (i = stringLength; --i >= 0; ) {
            this->array->sdata[count + i] = ((unsigned char *)buffer)[i];
        }
        this->count = newCount;
        pushStackAsType(STRINGBUFFER_INSTANCE, this);
    } else {
        /* We will have gotten an OutOfMemoryException from the
         * expandStringBufferCapacity
         */
    }
}

/*=========================================================================
 * FUNCTION:      toString()
 * CLASS:         java.lang.StringBuffer
 * TYPE:          virtual native function
 * OVERVIEW:      returns the String representation of a StringBuffer
 *   parameters:  this
 *   returns:     the corresponding String
 *==================================================================== */

void Java_java_lang_StringBuffer_toString(void) {
    STRINGBUFFER_INSTANCE this = popStackAsType(STRINGBUFFER_INSTANCE);
    STRING_INSTANCE result;
    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(STRINGBUFFER_INSTANCE, thisX, this);
        result = (STRING_INSTANCE)instantiate(JavaLangString);
        this = thisX;
    END_TEMPORARY_ROOTS

    if (result != NULL) {
        this->shared = TRUE;

        result->offset = 0;
        result->length = this->count;
        result->array  = this->array;

        pushStackAsType(STRING_INSTANCE, result);
    } else {
        /* We will already have thrown an appropriate error */
    }
}

static void expandStringBufferCapacity(STRINGBUFFER_HANDLE sbH, int newLength)
{
    STRINGBUFFER_INSTANCE sb = unhand(sbH);
    long oldLength = sb->array->length;
    SHORTARRAY newArray;

    if (newLength < oldLength * 2) {
        newLength = oldLength * 2;
    }
    newArray =
        (SHORTARRAY)instantiateArray(PrimitiveArrayClasses[T_CHAR], newLength);

    if (newArray != NULL) {
        sb = unhand(sbH);           /* in case of GC */
        memcpy(&newArray->sdata[0], &sb->array->sdata[0],
               sb->count * sizeof(short));
        sb->array = newArray;
        sb->shared = FALSE;
    }
}

