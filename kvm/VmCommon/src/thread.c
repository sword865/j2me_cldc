/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Thread (concurrency) management
 * FILE:      thread.c
 * OVERVIEW:  This file defines the structures and operations
 *            that are needed for Java-style multithreading.
 * AUTHOR:    Antero Taivalsaari, Sun Labs, 1998
 *            Edited by Doug Simon 11/1998
 *            Edited by Nik Shaylor 09/1999 to allow asynchronous I/O
 *            Frank Yellin (new monitors), Bill Pittore (debugging)
 *=======================================================================*/

/*=========================================================================
 * COMMENTS:
 * Read the description of thread and monitor structures in thread.h
 *=======================================================================*/

/*=========================================================================
 * Local include files
 *=======================================================================*/

#include <global.h>
#include <execute.h>

/*=========================================================================
 * Global variables needed for multitasking
 *=======================================================================*/

THREAD CurrentThread;   /* Current thread pointer */
THREAD MainThread;      /* Global so debugger code can create a name */

THREAD AllThreads;      /* List of all threads */
THREAD RunnableThreads;  /* Runnable thread list */

/* NOTE:
 * RunnableThreads is a circular queue of threads.  RunnableThreads
 * is either NULL or points to the >>last<< element of the list.
 * This makes it easier to add to either end of the list *
 */

int AliveThreadCount;   /* Number of alive threads in AllThreads.  This count
                         * does >>not<< include threads that haven't yet been
                         * started */

int Timeslice;          /* Time slice counter for multitasking */

/*=========================================================================
 * Static declarations needed for this file
 *=======================================================================*/

#define QUEUE_ADDED TRUE
#define QUEUE_REMOVED FALSE

#if INCLUDEDEBUGCODE
static void
TraceMonitor(THREAD thread, MONITOR monitor, THREAD *queue, bool_t added);
static void TraceThread(THREAD thread, char *what);
#endif

/* Internal monitor operations */
static void addMonitorWait(MONITOR monitor, THREAD thread);
static void addCondvarWait(MONITOR monitor, THREAD thread);
static void removeMonitorWait(MONITOR monitor);
static void removeCondvarWait(MONITOR monitor, bool_t notifyAll);
static void removePendingAlarm(THREAD thread);

/* Internal queue manipulation operations */
typedef enum { AT_START, AT_END } queueWhere;
static void   addThreadToQueue(THREAD *queue, THREAD, queueWhere where);
static THREAD removeQueueStart(THREAD *queue);
static bool_t removeFromQueue(THREAD *queue, THREAD waiter);
static int    queueLength(THREAD queue);

static void   monitorWaitAlarm(THREAD thread);

/*=========================================================================
 * Global multitasking operations
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      storeExecutionEnvironment()
 *                loadExecutionEnvironment()
 * TYPE:          private operations
 * OVERVIEW:      These functions are used for storing and restoring
 *                virtual machine registers upon task switching.
 * INTERFACE:
 *   parameters:  thread pointer
 *   returns:     <nothing>
 *=======================================================================*/

void storeExecutionEnvironment(THREAD thisThread)
{
    /* Save the current thread execution environment
     * (virtual machine registers) to the thread structure.
     */
    thisThread->fpStore = getFP();
    thisThread->spStore = getSP();
    thisThread->ipStore = getIP();
}

void loadExecutionEnvironment(THREAD thisThread)
{
    /* Restore the thread execution environment */
    /* (VM registers) from the thread structure */
    setFP(thisThread->fpStore);
    setLP(FRAMELOCALS(getFP()));
    setCP(getFP()->thisMethod->ofClass->constPool);
    setSP(thisThread->spStore);
    setIP(thisThread->ipStore);
}

/*=========================================================================
 * FUNCTION:      SwitchThread()
 * TYPE:          public global operations
 * OVERVIEW:      Fundamental task switching operation:
 *                switches to the next thread to be executed.
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     Boolean showing if there is now a current thread
 *
 * NOTE:
 *
 * On entry to this routine there are three states the current thread
 * pointer can be in:
 *
 * CurrentThread == NIL
 *
 *     There is no current thread because suspendThread() was called.
 *
 * CurrentThread != NIL && CurrentThread->state == THREAD_ACTIVE
 *
 *     This is the normal case when the current thread's timeslice has
 *     run out.
 *
 * CurrentThread != NIL && CurrentThread->state != THREAD_ACTIVE
 *
 *     This is an atypical condition when suspendThread() was called
 *     while we were in supervisor mode. In this case the routine
 *     must spin until CurrentThread->state == THREAD_ACTIVE.
 *
 *=======================================================================*/

bool_t SwitchThread(void)
{
    THREAD threadToAdd = NIL;

    if (CurrentThread != NIL) {

        /* Sanity test: any pending exception must have already been thrown */
        if (CurrentThread->pendingException != NIL) {
            fatalError(KVM_MSG_BAD_PENDING_EXCEPTION);
        }

        if (CurrentThread->state == THREAD_ACTIVE) {
            /* If there is only one thread, or we can't switch threads then
               just return */
            if (RunnableThreads == NULL) {
                /* Nothing else to run */
                Timeslice = CurrentThread->timeslice;
                return TRUE;
            } else {
                /* Save the VM registers.  Indicate that this thread is to */
                /* to be added to the dormant thread queue */
                storeExecutionEnvironment(CurrentThread);
                threadToAdd = CurrentThread;
                CurrentThread = NIL;
            }
        } else {
            fatalError(KVM_MSG_ATTEMPTING_TO_SWITCH_TO_INACTIVE_THREAD);
        }
    }

    /* Try and find a thread */
    CurrentThread = removeQueueStart(&RunnableThreads);

    /* If there was a thread to add then do so */
    if (threadToAdd != NIL) {
        addThreadToQueue(&RunnableThreads, threadToAdd, AT_END);
    }

    /* If nothing can run then return FALSE */
    if (CurrentThread == NIL) {
        return FALSE;
    }

#if ENABLEPROFILING
    ThreadSwitchCounter++;
#endif
    /* Load the VM registers of the new thread */
    loadExecutionEnvironment(CurrentThread);

#if INCLUDEDEBUGCODE
    if (tracethreading) {
        /* Diagnostics */
        TraceThread(CurrentThread, "Switching to this thread");
    }
#endif

    /* Load new time slice */
    Timeslice = CurrentThread->timeslice;

    /* If there's a pending exception, throw it immediately */
    if (CurrentThread->pendingException != NIL) {
        char* pending = CurrentThread->pendingException;
        CurrentThread->pendingException = NIL;
        raiseException(pending);
    }

    return TRUE;
}

/*=========================================================================
 * Public operations on thread instances
 *=======================================================================*/

/*=========================================================================
 * Constructors and destructors
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      BuildThread()
 * TYPE:          public global operation
 * OVERVIEW:      Build a new VM-level thread by instantiating and
 *                initializing the necessary structures.
 * INTERFACE:
 *   returns:     a new thread instance
 * NOTE:          The created thread is an internal structure that does
 *                not map one-to-one with Java class 'Thread.class'.
 *                One-to-one mapping is implemented by structure
 *                JAVATHREAD.
 *=======================================================================*/

static THREAD BuildThread(JAVATHREAD_HANDLE javaThreadH)
{
    THREAD newThread;
    JAVATHREAD javaThread;
    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(THREAD, newThreadX,
                               (THREAD)callocObject(SIZEOF_THREAD, GCT_THREAD));
        STACK newStack = (STACK)callocObject(sizeof(struct stackStruct)
                                                     / CELL, GCT_EXECSTACK);
        /* newStack->next = NULL;  Already zero from calloc */
        newStack->size = STACKCHUNKSIZE;
        newThreadX->stack = newStack;

#if INCLUDEDEBUGCODE
        if (tracethreading) {
            TraceThread(newThreadX, "Created");
        }
        if (tracestackchunks) {
            fprintf(stdout,
                "Created a new stack (thread: %lx, first chunk: %lx, chunk size: %ld\n",
                (long)newThreadX, (long)newStack, (long)STACKCHUNKSIZE);
        }
#endif /* INCLUDEDEBUGCODE */

        newThread = newThreadX;
    END_TEMPORARY_ROOTS

    /* Time slice will be initialized to default value */
    newThread->timeslice = BASETIMESLICE;

    /* Link the THREAD to the JAVATHREAD */
    javaThread = unhand(javaThreadH);
    newThread->javaThread = javaThread;
    javaThread->VMthread = newThread;

    /* Initialize the state */
    newThread->state = THREAD_JUST_BORN;
#if ENABLE_JAVA_DEBUGGER
    newThread->nextOpcode = NOP;
#endif

    /* Add to the alive thread list  */
    newThread->nextAliveThread = AllThreads;
    AllThreads = newThread;

    return(newThread);
}

/*=========================================================================
 * FUNCTION:      DismantleThread()
 * TYPE:          public global operation
 * OVERVIEW:      Free off the thread's resources
 * INTERFACE:
 *   parameters:  The thread pointer
 *   returns:     <none>
 *=======================================================================*/

void
DismantleThread(THREAD thisThread)
{
    START_TEMPORARY_ROOTS
        IS_TEMPORARY_ROOT(thisThread, thisThread);

#if ENABLE_JAVA_DEBUGGER
        if (vmDebugReady) {
            CEModPtr cep;
            storeExecutionEnvironment(thisThread);
            cep = GetCEModifier();
            cep->threadID = getObjectID((OBJECT)thisThread->javaThread);
            setEvent_ThreadDeath(cep);
            FreeCEModifier(cep);
        }
#endif /* ENABLE_JAVA_DEBUGGER */

    /*
     * A side effect of setevent is that the state could change, let's
     * hammer it home that this thread is DEAD
     */
    thisThread->state = THREAD_DEAD;
    if (AllThreads == thisThread) {
        AllThreads = AllThreads->nextAliveThread;
    } else {
        THREAD prevThread = AllThreads;
        /* Remove from alive thread list */
        while (prevThread->nextAliveThread != thisThread) {
            prevThread = prevThread->nextAliveThread;
        }
        prevThread->nextAliveThread = thisThread->nextAliveThread;
    }
    thisThread->nextAliveThread = NULL;
    thisThread->stack = NULL;
    thisThread->fpStore = NULL;
    thisThread->spStore = NULL;
    if (inTimerQueue(thisThread)) {
        removePendingAlarm(thisThread);
    }
    END_TEMPORARY_ROOTS
}

/*=========================================================================
 * FUNCTION:      InitializeThreading()
 * TYPE:          public global operation
 * OVERVIEW:      Create the first low-level system thread and
 *                initialize it properly.  Initialize VM registers
 *                accordingly.
 * INTERFACE:
 *   parameters:  method: The main(String[]) method to call
 *                arguments:  String[] argument
 *   returns:     <nothing>
 *=======================================================================*/

static void
initInitialThreadBehaviorFromThread(FRAME_HANDLE);

void InitializeThreading(INSTANCE_CLASS mainClass, ARRAY argumentsArg)
{
    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(ARRAY, arguments, argumentsArg);
        DECLARE_TEMPORARY_ROOT(JAVATHREAD, javaThread,
                               (JAVATHREAD)instantiate(JavaLangThread));
        
        int unused; /* Needed for creating name char array */
        makeGlobalRoot((cell **)&MainThread);
        MainThread = NULL;
        MonitorCache = NULL;
        makeGlobalRoot((cell **)&CurrentThread);
        makeGlobalRoot((cell **)&RunnableThreads);
        makeGlobalRoot((cell **)&TimerQueue);

        /* Initialize the field of the Java-level thread structure */
        javaThread->priority = 5;

        /* Initialize the name of the system thread (since CLDC 1.1) */
        javaThread->name = createCharArray("Thread-0", 8, &unused, FALSE);

        MainThread = BuildThread(&javaThread);

        /* AllThreads is initialized to NULL by the garbage collector.
         *
         * Ensure that the thread list is properly initialized
         * and set mainThread as the active (CurrentThread) thread
         */
        MainThread->nextThread = NIL;
        AliveThreadCount = 1;
        Timeslice = BASETIMESLICE;
        MainThread->state = THREAD_ACTIVE;

        /* Initialize VM registers */
        CurrentThread = MainThread;
        RunnableThreads = NULL;
        TimerQueue = NULL;

        setSP((MainThread->stack->cells - 1));
        setFP(NULL);
        setIP(KILLTHREAD);

        /* We can't create a frame consisting of "method", since its class
         * has not yet been initialized, (and this would mess up the
         * garbage collector).  So we set up a pseudo-frame, and arrange
         * for the interpreter to do the work for us.
         */
        pushFrame(RunCustomCodeMethod);
        pushStackAsType(CustomCodeCallbackFunction,
                        initInitialThreadBehaviorFromThread);
        /* We want to push method, but that would confuse the GC, since
         * method is a pointer into the middle of a heap object.  Only
         * heap objects, and things that are clearly not on the heap, can
         * be pushed onto the stack.
         */
        pushStackAsType(INSTANCE_CLASS, mainClass);
        pushStackAsType(ARRAY, arguments);
        initializeClass(mainClass);
    END_TEMPORARY_ROOTS
}

static void
initInitialThreadBehaviorFromThread(FRAME_HANDLE exceptionFrameH) {
    INSTANCE_CLASS thisClass;
    METHOD thisMethod;
    if (exceptionFrameH != NULL) {
        /* We have no interest in dealing with exceptions. */
        return;
    }
    thisClass = secondStackAsType(INSTANCE_CLASS);

    thisMethod = getSpecialMethod(thisClass, mainNameAndType);
    if (thisMethod == NULL) {
        AlertUser(KVM_MSG_CLASS_DOES_NOT_HAVE_MAIN_FUNCTION);
        stopThread();
    } else if ((thisMethod->accessFlags & ACC_PUBLIC) == 0) {
        AlertUser(KVM_MSG_MAIN_FUNCTION_MUST_BE_PUBLIC);
        stopThread();
    } else {
        START_TEMPORARY_ROOTS
            DECLARE_TEMPORARY_ROOT(ARRAY, arguments, topStackAsType(ARRAY));
            /* Reinitialize the thread for the new method */
            setSP((CurrentThread->stack->cells - 1) + thisMethod->argCount);
            setFP(NULL);
            setIP(KILLTHREAD);
            pushFrame(thisMethod);
            ((ARRAY *)getLP())[0] = arguments;
        END_TEMPORARY_ROOTS
        if (thisMethod->accessFlags & ACC_SYNCHRONIZED) {
            getFP()->syncObject = (OBJECT)thisClass;
            monitorEnter((OBJECT)thisClass);
        } else {
            getFP()->syncObject = NULL;
        }
    }
}

/*=========================================================================
 * FUNCTION:      getVMthread()
 * TYPE:          public auxiliary operation
 * OVERVIEW:      Given a Java-level thread, ensure that the internal
 *                VM-level thread structure has been created.
 * INTERFACE:
 *   parameters:  JAVATHREAD pointer
 *   returns:     THREAD pointer
 *=======================================================================*/

THREAD getVMthread(JAVATHREAD_HANDLE javaThreadH)
{
    /* Create the VM-level thread structure if necessary */
    THREAD VMthread = unhand(javaThreadH)->VMthread;
    if (!VMthread) {
        VMthread = BuildThread(javaThreadH);
    }
    return VMthread;
}

/*=========================================================================
 * Thread activation and deactivation operations
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      initThreadBehavior()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Set the behavior of a thread without the context of
 *                a current execution stack (i.e. don't use sp).
 * INTERFACE:
 *   parameters:  thread pointer, pointer to a method object and the sync
 *                object that will be used if this is a synchronized method.
 *   returns:     <nothing>
 * NOTE:          You have to explicitly construct the local stack of
 *                this thread after returning from this call. Don't
 *                forget that 'this' must be the first local is this
 *                thisMethod is virtual.
 *                There are no safety checks here, so do not activate
 *                the same thread twice.
 *=======================================================================*/

static void
initThreadBehaviorFromThread(FRAME_HANDLE);

void initThreadBehavior(THREAD thisThread, METHOD thisMethod, OBJECT syncObjectArg)
{
    START_TEMPORARY_ROOTS
        /* Protect the syncObject argument from garbage collection */
        DECLARE_TEMPORARY_ROOT(OBJECT, syncObject, syncObjectArg);

        /* Note, thisThread == CurrentThread when booting.  This code below is
         * slightly inefficient, but it's not worth the hassle of fixing for
         * a one-time case.
         */
        THREAD current = CurrentThread;
        if (current != NULL) {
            storeExecutionEnvironment(current);
        }
        CurrentThread = thisThread;

        setSP((thisThread->stack->cells - 1) + thisMethod->argCount);
        setFP(NULL);
        setIP(KILLTHREAD);
        pushFrame(thisMethod);

        if (thisMethod->accessFlags & ACC_SYNCHRONIZED) {
            getFP()->syncObject = syncObject;
            pushFrame(RunCustomCodeMethod);
            pushStackAsType(CustomCodeCallbackFunction,
                            initThreadBehaviorFromThread);
        } else {
            getFP()->syncObject = NULL;
        }

        storeExecutionEnvironment(thisThread);

        if (current) {
            loadExecutionEnvironment(current);
        }
        CurrentThread = current;
    END_TEMPORARY_ROOTS
}

/*=========================================================================
 * FUNCTION:      initThreadBehaviorFromThread()
 * TYPE:          private initialization of a class
 * OVERVIEW:      This function performs any thread initialization that can
 *                only be performed by the thread itself after it has
 *                begun running, and that cannot be performed by the 
 *                creator of the thread.
 *
 * Notes:         This is a "CustomCode" callback function.
 *
 *                Currently, it only handles the method being synchronized,
 *                and needing to enter the monitor.  It may be extended to
 *                do more.
 * INTERFACE:
 *   parameters:  exceptionFrameHandle:
 *                  - NULL if called from the interpreter.
 *                  - A handle to the current exception if called from
 *                    the error handler.
 *   returns:     <nothing>
 *=======================================================================*/

static void
initThreadBehaviorFromThread(FRAME_HANDLE exceptionFrameH) {
    if (exceptionFrameH == NULL) {
        popFrame();
        if (getFP()->syncObject) {
            monitorEnter(getFP()->syncObject);
        }
    } else {
        /* We have no interest in dealing with exceptions */
    }
}

/*=========================================================================
 * FUNCTION:      startThread()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Make a thread alive, but suspended.
 * INTERFACE:
 *   parameters:  thread pointer
 *   returns:     <nothing>
 * NOTE:          There are no safety checks here, so please
 *                do not activate the same thread twice.
 *=======================================================================*/

void startThread(THREAD thisThread)
{
#if INCLUDEDEBUGCODE
    if (tracethreading) {
        TraceThread(thisThread, "Starting");
    }
#endif

    /* Add one to the count of alive Java threads */
    thisThread->state = THREAD_SUSPENDED;
    AliveThreadCount++;

#if ENABLE_JAVA_DEBUGGER
    START_TEMPORARY_ROOTS
        IS_TEMPORARY_ROOT(thisThread, thisThread);
    if (vmDebugReady) {
        CEModPtr cep = GetCEModifier();
        cep->threadID = getObjectID((OBJECT)thisThread->javaThread);
        cep->eventKind = JDWP_EventKind_THREAD_START;
        insertDebugEvent(cep);
    }
   END_TEMPORARY_ROOTS
#endif /* ENABLE_JAVA_DEBUGGER */
}

/*=========================================================================
 * FUNCTION:      interruptThread()
 * TYPE:          public instance-level operation
 * OVERVIEW:      interrupt a thread that is sleeping or waiting
 * INTERFACE:
 *   parameters:  thread pointer
 *   returns:     <nothing>
 *=======================================================================*/

void interruptThread(THREAD thread) {
    bool_t sleeping = FALSE;
    bool_t waiting = FALSE;
    if (inTimerQueue(thread)) {
        removePendingAlarm(thread);
        sleeping = TRUE;
    }
    if (thread->state & THREAD_CONVAR_WAIT) {
        MONITOR monitor = thread->monitor;
        removeFromQueue(&monitor->condvar_waitq, thread);
        addMonitorWait(monitor, thread);
        waiting = TRUE;
    }
    if (sleeping || waiting) { 
        thread->pendingException = (char*)InterruptedException;
        if (!waiting) { 
            resumeThread(thread);
        }
    } else { 
        thread->isPendingInterrupt = TRUE;
    }
}
    

/*=========================================================================
 * FUNCTION:      handlePendingInterrupt();
 * TYPE:          public instance-level operation
 * OVERVIEW:      Called by sleep() or wait() if isPendingInterrupt is 
 *                set
 * INTERFACE:
 *   parameters:  thread pointer
 *   returns:     <nothing>
 *=======================================================================*/

void handlePendingInterrupt() { 
    THREAD thisThread = CurrentThread;
    thisThread->pendingException = (char*)InterruptedException;
    thisThread->isPendingInterrupt = FALSE;
}

/*=========================================================================
 * FUNCTION:      stopThread()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Stop a thread and free its resources
 * INTERFACE:
 *   parameters:  thread pointer
 *   returns:     <nothing>
 * NOTE:          There are no safety checks here, so please
 *                do not stopThread the same thread twice.
 *=======================================================================*/

void stopThread(void)
{
    THREAD thisThread = CurrentThread;

#if INCLUDEDEBUGCODE
    if (tracethreading) {
        TraceThread(thisThread, "Stopping");
    }
#endif

    /* First suspend the thread */
    suspendThread();

    /* Always force the end of the current thread */
    CurrentThread = NIL;

    /* Make the thread be no longer alive */
    thisThread->state = THREAD_DEAD;
    AliveThreadCount--;

    /* Notify any waiters that we are dying */
    if (OBJECT_MHC_TAG(thisThread->javaThread) == MHC_MONITOR) {
        removeCondvarWait(OBJECT_MHC_MONITOR(thisThread->javaThread), TRUE);
    }

    /* Then kill it off */
    DismantleThread(thisThread);
}

/*=========================================================================
 * FUNCTION:      suspendThread()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Suspend the currently executing thread.
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 * NOTE:          When this function is used with an asynchronous
 *                callback routine that calls resumeThread(), it
 *                is vital that this routine is called *BEFORE*
 *                the callback routine can execute. **NS**
 *=======================================================================*/

void suspendThread(void)
{
    if (CurrentThread == NULL) {
        /* This shouldn't happen, but don't crash */
        return;
    }

#if INCLUDEDEBUGCODE
    if (tracethreading) {
        TraceThread(CurrentThread, "Suspending");
    }
#endif

    if (!(CurrentThread->state & THREAD_SUSPENDED)) {
        /* Save the VM registers of the currently executing thread */
        storeExecutionEnvironment(CurrentThread);

        /* Signal time to reschedule the interpreter */
        signalTimeToReschedule();
    }

    /* Set the flag to show that it is inactive */
    CurrentThread->state |= THREAD_SUSPENDED;

#if ENABLE_JAVA_DEBUGGER
    /* If we are running the debugger, we must send any pending events
     * that this thread knows about before we set CurrentThread to NIL.
     */
     __checkDebugEvent();
#endif

    /* Show that there is no current thread anymore */
    CurrentThread = NIL;

}

#if ENABLE_JAVA_DEBUGGER
void suspendSpecificThread(THREAD thread)
{
    if (thread == NULL) {
        /* This shouldn't happen, but don't crash */
        return;
    }

    if (thread == CurrentThread) {
        if (!(CurrentThread->state & THREAD_SUSPENDED)) {
            /* Save the VM registers of the currently executing thread */
            storeExecutionEnvironment(CurrentThread);

            /* Signal time to reschedule the interpreter */
            signalTimeToReschedule();
        }

        /* Show that there is no current thread anymore */
        /* Only do this if we are not in supervisor mode */
        CurrentThread = NIL;
    } else {

        if (!(thread->state & (THREAD_SUSPENDED | THREAD_DEAD))) {
            removeFromQueue(&RunnableThreads, thread);
        }
    }
    thread->state |= THREAD_DBG_SUSPENDED;
    thread->debugSuspendCount++;
}
#endif /* ENABLE_JAVA_DEBUGGER */

/*=========================================================================
 * FUNCTION:      resumeThread()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Resume a thread by placing it from the dormant
 *                thread chain.
 * INTERFACE:
 *   parameters:  thread pointer
 *   returns:     <nothing>
 *=======================================================================*/

void resumeThread(THREAD thisThread)
{
#if INCLUDEDEBUGCODE
    if (tracethreading) {
        TraceThread(thisThread, "Resuming");
    }
#endif

    if (!(thisThread->state & THREAD_SUSPENDED)) {
        fatalError(KVM_MSG_ATTEMPTING_TO_RESUME_NONSUSPENDED_THREAD);
    }

#if ENABLE_JAVA_DEBUGGER
    if (thisThread->state & THREAD_DBG_SUSPENDED) {
        thisThread->state &= ~THREAD_SUSPENDED;
        return;
    }
#endif

    /* Set the flag to show that it is now active */
    thisThread->state = THREAD_ACTIVE;

    /* Check if we are trying to restart CurrentThread
     * Normally this is an error, but if we are
     * spinning in supervisor mode it is normal
     */
    if (thisThread == CurrentThread) {
        fatalError(KVM_MSG_ATTEMPTING_TO_RESUME_CURRENT_THREAD);
        /* We don't need to do anything else because SwitchThread will be
         * spinning on the thread state, so we just have to get out
         * of whatever sort of callback routine we are in, and it will then
         * be able to proceed */
    } else {
        /* If the new thread has higher priority then */
        /* add to head of the wait queue and signal that */
        /* it is time to reschedule the processor */
        addThreadToQueue(&RunnableThreads, thisThread, AT_END);
    }
}

#if ENABLE_JAVA_DEBUGGER

void resumeSpecificThread(THREAD thisThread)
{
    if (--thisThread->debugSuspendCount <= 0) {
        thisThread->state &= ~THREAD_DBG_SUSPENDED;
        thisThread->debugSuspendCount = 0;
        if (!(thisThread->state & THREAD_SUSPENDED) &&
            !(thisThread->state & THREAD_JUST_BORN)) {
            thisThread->state |= THREAD_SUSPENDED;
            resumeThread(thisThread);
        }
    }
}

#endif /* ENABLE_JAVA_DEBUGGER */

/*=========================================================================
 * FUNCTION:      activeThreadCount
 * TYPE:          public global operations
 * OVERVIEW:      Returns the number of threads ready to run.
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     int
 *=======================================================================*/

int activeThreadCount() {
    return (CurrentThread ? 1 : 0) + queueLength(RunnableThreads);
}

/*=========================================================================
 * FUNCTION:      isActivated()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Check if a thread is active
 * INTERFACE:
 *   parameters:  thread pointer
 *   returns:     TRUE if the task is active, FALSE otherwise.
 *=======================================================================*/

int isActivated(THREAD thread)
{
    if (thread == NIL) {
        return FALSE;
    } else {
        int state = thread->state;
        return (state & THREAD_ACTIVE) || (state & THREAD_SUSPENDED);
    }
}

/*=========================================================================
 * FUNCTION:      removeFirstRunnableThread()
 * OVERVIEW:      Return (and remove) the first runnable
 *                thread of the queue.
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     A THREAD object.
 *=======================================================================*/

#if ENABLE_JAVA_DEBUGGER

THREAD removeFirstRunnableThread()
{
    return removeQueueStart(&RunnableThreads);
}

#endif /* ENABLE_JAVA_DEBUGGER */

/*=========================================================================
 * Asynchronous threading operations (optional)
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      asyncFunctionProlog()
 * TYPE:          Public link routine for asynchronous native methods
 * OVERVIEW:      Call an asynchronous native method
 * INTERFACE:
 *   parameters:  thread pointer, native function pointer
 *   returns:     <nothing>
 *
 * Note: This is just an example implementation for demonstration purposes.
 *       It does not actually work very well because eventually the WIN32
 *       program seems to run out of thread resources and no new threads
 *       seem to run. Typically no error code is ever returned!
 *=======================================================================*/

#if ASYNCHRONOUS_NATIVE_FUNCTIONS

static int AsyncThreadCount   = 0;      /* Number of asynchronously */
                                        /* running I/O threads */
static int collectorIsRunning = FALSE;  /* Flag to show the GC is active */

void asyncFunctionProlog() {
   /*
    * The garbage collector cannot be running because the calling thread is
    * the main system thread and the GC is only ever run on this thread
    * so the following is just a safety check.
    */
    if (collectorIsRunning) {
        fatalError(KVM_MSG_COLLECTOR_IS_RUNNING_ON_ENTRY_TO_ASYNCFUNCTIONPROLOG);
    }

    suspendThread(); /* Suspend the current thread */

    START_CRITICAL_SECTION
        AsyncThreadCount++;
    END_CRITICAL_SECTION
}

#endif /* ASYNCHRONOUS_NATIVE_FUNCTIONS */

/*=========================================================================
 * FUNCTION:      asyncFunctionEpilog()
 * TYPE:          Public link routine for asynchronous native methods
 * OVERVIEW:      Spin until all asynchronous I/O is finished
 * INTERFACE:
 *   parameters:  A THREAD
 *   returns:     <nothing>
 *=======================================================================*/

#if ASYNCHRONOUS_NATIVE_FUNCTIONS

void asyncFunctionEpilog(THREAD thisThread) {
    resumeThread(thisThread);

    START_CRITICAL_SECTION
        --AsyncThreadCount;
        NATIVE_FUNCTION_COMPLETED();
    END_CRITICAL_SECTION
}

#endif /* ASYNCHRONOUS_NATIVE_FUNCTIONS */

/*=========================================================================
 * FUNCTION:      decrementAsyncCount()
 * TYPE:          Public link routine for asynchronous native methods
 * OVERVIEW:      Allow the garbage collector to run during async I/O
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

#if ASYNCHRONOUS_NATIVE_FUNCTIONS

void decrementAsyncCount() {
    START_CRITICAL_SECTION
        --AsyncThreadCount;
    END_CRITICAL_SECTION
}

#endif /* ASYNCHRONOUS_NATIVE_FUNCTIONS */

/*=========================================================================
 * FUNCTION:      incrementAsyncCount()
 * TYPE:          Public link routine for asynchronous native methods
 * OVERVIEW:      Disallow the garbage collector to run during async I/O
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

#if ASYNCHRONOUS_NATIVE_FUNCTIONS

void incrementAsyncCount() {
    bool_t yield = TRUE;
    while (yield) {
        START_CRITICAL_SECTION
            if(!collectorIsRunning) {
                ++AsyncThreadCount;
                yield = FALSE;
            }
        END_CRITICAL_SECTION
        if(yield) {
            Yield_md();
        }
    }
}

#endif /* ASYNCHRONOUS_NATIVE_FUNCTIONS */

/*=========================================================================
 * FUNCTION:      RundownAsynchronousFunctions()
 * TYPE:          Public routine for garbage collector
 * OVERVIEW:      Spin until all asynchronous I/O is finished
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

#if ASYNCHRONOUS_NATIVE_FUNCTIONS

void RundownAsynchronousFunctions(void) {

    for (;;) {

        if (collectorIsRunning) {
            fatalError(KVM_MSG_COLLECTOR_RUNNING_IN_RUNDOWNASYNCHRONOUSFUNCTIONS);
        }

        START_CRITICAL_SECTION
            if (AsyncThreadCount == 0) {
                collectorIsRunning = TRUE;
            }
        END_CRITICAL_SECTION

        if (collectorIsRunning) {
            break;
        }

        /* Wait */
        Yield_md();
    }

}

#endif /* ASYNCHRONOUS_NATIVE_FUNCTIONS */

/*=========================================================================
 * FUNCTION:      RestartAsynchronousFunctions()
 * TYPE:          Public routine for garbage collector
 * OVERVIEW:      Allow asynchronous I/O to continue
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

#if ASYNCHRONOUS_NATIVE_FUNCTIONS

void RestartAsynchronousFunctions(void) {
    if (!collectorIsRunning) {
        fatalError(KVM_MSG_COLLECTOR_NOT_RUNNING_ON_ENTRY_TO_RESTARTASYNCHRONOUSFUNCTIONS);
    }
    collectorIsRunning = FALSE;
}

#endif /* ASYNCHRONOUS_NATIVE_FUNCTIONS */

/*=========================================================================
 * Thread debugging operations
 *=======================================================================*/

#if INCLUDEDEBUGCODE

/*=========================================================================
 * FUNCTION:      printThreadStatus()
 * TYPE:          public thread instance-level operation
 * OVERVIEW:      Print the status of given thread for debugging purposes.
 * INTERFACE:
 *   parameters:  thread pointer
 *   returns:     <nothing>
 *=======================================================================*/

static void
printThreadStatus(THREAD thisThread)
{
    fprintf(stdout, "Thread %lx status:\n", (long)thisThread);

    if (thisThread == CurrentThread) {
        fprintf(stdout, "ACTIVE (CurrentThread)\n");
        /* To print most up-to-date information we need to do this */
        storeExecutionEnvironment(CurrentThread);
    } else {
        fprintf(stdout, "%s\n",
                isActivated(thisThread) ? "ACTIVE (DORMANT)" : "SUSPENDED");
    }

    fprintf(stdout,
            "Timeslice: %ld, Priority: %ld\n",
            thisThread->timeslice, thisThread->javaThread->priority);
    fprintf(stdout,
            "Instruction pointer: %lx\n", (long)thisThread->ipStore);

    fprintf(stdout,
            "Execution stack location: %lx\n", (long)thisThread->stack);
    fprintf(stdout,
            "Execution stack pointer: %lx \n",
            (long)(thisThread->spStore));

    fprintf(stdout,
            "Frame pointer: %lx \n", (long)(thisThread->fpStore));

    fprintf(stdout,
            "JavaThread: %lx VMthread: %lx\n",
            (long)thisThread->javaThread, (long)thisThread->javaThread->VMthread);

    if (thisThread->monitor != NULL) {
        fprintf(stdout,
                "Waiting on monitor %lx %lx\n",
                (long)thisThread->monitor, (long)thisThread->monitor->object);
    }

    if (inTimerQueue(thisThread)) {
        fprintf(stdout, "In timer queue\n");
    }

    fprintf(stdout, "\n\n");
}

/*=========================================================================
 * FUNCTION:      printFullThreadStatus()
 * TYPE:          public thread instance-level operation
 * OVERVIEW:      Print the status of all the active threads in the system.
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

void printFullThreadStatus(void)
{
    THREAD thisThread;
    int    threadCount = 0;

    for (thisThread = AllThreads; thisThread != NULL;
           thisThread = thisThread->nextAliveThread) {
        fprintf(stdout, "Thread #%d\n", ++threadCount);
        printThreadStatus(thisThread);
    }
}

#endif /* INCLUDEDEBUGCODE */

/*=========================================================================
 * Timer implementation
 *=======================================================================*/

/* Threads waiting for a timer interrupt */
THREAD TimerQueue;

/*=========================================================================
 * Timer operations
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      registerAlarm()
 * TYPE:          timer queue
 * OVERVIEW:      Perform a callback on this thread
 * INTERFACE:
 *   parameters:  The thread pointer
 *                The time in the future at which the callback should be called
 *                The callback function
 *   returns:     no value
 *=======================================================================*/

void
registerAlarm(THREAD thread, long64 delta, void (*wakeupCall)(THREAD))
{
#if NEED_LONG_ALIGNMENT
    Java8 tdub;
#endif

    THREAD q, prev_q;
    ulong64 wakeupTime;

    /*
     * First see if this thread is on the queue already.  This can happen
     * if the thread was suspended by the debugger code, and there was a
     * wakeup pending.  Then if the user resumes this thread it could try to
     * register another wakeup before this one happens.  This is rather
     * simple minded since what if the second call is registering a different
     * callback?  This whole subsystem should be re-written slightly to
     * handle a list of callbacks I suppose.
     */
    q = TimerQueue;
    while (q != NULL) {
        if (q == thread)
            return; /* already on the queue so leave  */
        q = q->nextAlarmThread;
    }
    /* set wakeupTime to now + delta.  Save this value in the thread */
    wakeupTime = CurrentTime_md();
    ll_inc(wakeupTime, delta);      /* wakeUp += delta */

    SET_ULONG(thread->wakeupTime, wakeupTime);

#if INCLUDEDEBUGCODE
    if (tracethreading) {
        TraceThread(thread, "Adding to timer queue");
    }
#endif

    /* Save the callback function in the thread */
    thread->wakeupCall = wakeupCall;

    /* insert this value into the clock queue, which is sorted by wakeup time.*/
    q = TimerQueue;
    prev_q = NULL;
    while (q != NULL) {
        ulong64 qtime = GET_ULONG(q->wakeupTime);
        if (ll_compare_ge(qtime, wakeupTime)) {
            break;
        }
        prev_q = q;
        q = q->nextAlarmThread;
    }
    if (prev_q != NULL) {
        /* This is the first item in the queue. */
        prev_q->nextAlarmThread = thread;
        thread->nextAlarmThread = q;
    } else {
        thread->nextAlarmThread = TimerQueue;
        TimerQueue = thread;
    }
}

/*=========================================================================
 * FUNCTION:      checkTimerQueue()
 * TYPE:          timer queue
 * OVERVIEW:      Remove objects, as necessary, from the timer queue and
 *                call their callback function.
 * INTERFACE:
 *   parameters:  A pointer to a ulong64.  This value will be set to the delta
 *                time at which the next clock event will occur.
 *   returns:     no value
 *=======================================================================*/

void
checkTimerQueue(ulong64 *nextTimerDelta)
{
#if NEED_LONG_ALIGNMENT
    Java8 tdub;
#endif
    ulong64 now = CurrentTime_md();
    if (TimerQueue != NULL) {
        do {
            ulong64 firstTime = GET_ULONG(TimerQueue->wakeupTime);
            if (ll_compare_le(firstTime, now)) {
                /* Remove this item from the queue, and resume it */
                THREAD thread = TimerQueue;
                void (*wakeupCall)() = thread->wakeupCall;

#if INCLUDEDEBUGCODE
                if (tracethreading) {
                    TraceThread(thread, "Removing from timer queue");
                }
#endif

                TimerQueue = thread->nextAlarmThread;
                thread->nextAlarmThread = NULL;
                thread->wakeupCall = NULL; /* signal that not on queue */
                wakeupCall(thread);
            } else {
                break;
            }
        } while (TimerQueue != NULL);
    }

    /* Now indicate when the next timer wakeup should happen */
    if (TimerQueue == NULL) {
        ll_setZero(*nextTimerDelta);
    } else {
        ulong64 nextwakeup = GET_ULONG(TimerQueue->wakeupTime);
        if (ll_compare_le(nextwakeup, now)) {
            ll_setZero(*nextTimerDelta);
        } else {
                ll_dec(nextwakeup, now);
            *nextTimerDelta = nextwakeup;
        }
    }
}

/*=========================================================================
 * FUNCTION:      removePendingAlarm();
 * TYPE:          timer queue
 * OVERVIEW:      Remove thread from the timer queue.
 * INTERFACE:
 *   parameters:  The thread to remove from the timer queue.
 *   returns:     no value
 *=======================================================================*/

static void
removePendingAlarm(THREAD thread) {
    THREAD q, prev_q;

#if INCLUDEDEBUGCODE
    if (tracethreading) {
        TraceThread(thread, "Purging from timer queue");
    }
#endif

    for (q = TimerQueue, prev_q = NULL;
               q != NULL; prev_q = q, q = q->nextAlarmThread) {
        if (q == thread) {
            if (prev_q) {
                prev_q->nextAlarmThread = q->nextAlarmThread;
            } else {
                TimerQueue = q->nextAlarmThread;
            }
            q->nextAlarmThread = NULL;
            q->wakeupCall = NULL; /* indicate not on queue anymore */
            break;
        }
    }
}

/*=========================================================================
 * FUNCTION:      inTimerQueue();
 * TYPE:          timer queue
 * OVERVIEW:      Determine if a thread is in the timer queue.
 * INTERFACE:
 *   parameters:  The thread
 *   returns:     TRUE if the thread is in the timer queue.
 *=======================================================================*/

bool_t inTimerQueue(THREAD thread) {
    return thread->wakeupCall != NULL;
}

/*=========================================================================
 * Monitor implementation
 *=======================================================================*/

MONITOR MonitorCache;

/*=========================================================================
 * Monitor operations
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      addMonitorWait
 * TYPE:          Monitor handler
 * OVERVIEW:      Called to indicate a thread is waiting on a specific monitor
 * INTERFACE:
 *   parameters:  monitor: Monitor
 *                thread:  Thread waiting on the monitor
 *=======================================================================*/

static void
addMonitorWait(MONITOR monitor, THREAD thread) {
    /* Add to the wait queue */
    addThreadToQueue(&monitor->monitor_waitq, thread, AT_END);
    thread->monitor = monitor;
    thread->state |= THREAD_MONITOR_WAIT;

#if INCLUDEDEBUGCODE
    if (tracemonitors) {
        TraceMonitor(thread, monitor, &monitor->monitor_waitq, QUEUE_ADDED);
    }
#endif

    /* If the wait queue has no owner, perhaps we should try to run this */
    if (monitor->owner == NULL) {
        removeMonitorWait(monitor);
    }
}

/*=========================================================================
 * FUNCTION:      removeMonitorWait
 * TYPE:          Monitor handler
 * OVERVIEW:      Called to indicate a new thread can take ownership of the
 *                  monitor
 * INTERFACE:
 *   parameters:  monitor: Monitor
 *                thread:  Thread waiting to be notified
 *=======================================================================*/

static void
removeMonitorWait(MONITOR monitor)
{
    THREAD waiter = removeQueueStart(&monitor->monitor_waitq);

    if (waiter != NULL) {
        /* Set the monitor's ownership and restore the entry count */
        monitor->owner = waiter;
        monitor->depth = waiter->monitor_depth;

        /* Zero this, just to be sure */
        waiter->monitor = NULL;
        waiter->monitor_depth = 0;

#if INCLUDEDEBUGCODE
        if (tracemonitors) {
            TraceMonitor(waiter, monitor, &monitor->monitor_waitq, QUEUE_REMOVED);
        }
#endif

        resumeThread(waiter);
    } else {
        /* No one wants this monitor.  Mark it as unused */
        monitor->owner = NULL;
        monitor->depth = 0;
    }
}

/*=========================================================================
 * FUNCTION:      addCondvarWait
 * TYPE:          Monitor handler
 * OVERVIEW:      Called to indicate a thread is waiting to be notified
 * INTERFACE:
 *   parameters:  monitor: Monitor
 *                thread:  Thread waiting to be notified
 *=======================================================================*/

static void
addCondvarWait(MONITOR monitor, THREAD thread) {
    if (monitor->owner != thread) {
        fatalVMError(KVM_MSG_BAD_CALL_TO_ADDCONDVARWAIT);
    }

    addThreadToQueue(&monitor->condvar_waitq, thread, AT_END);
    thread->monitor = monitor;
    thread->state |= THREAD_CONVAR_WAIT;

#if INCLUDEDEBUGCODE
    if (tracemonitors) {
        TraceMonitor(thread, monitor, &monitor->condvar_waitq, QUEUE_ADDED);
    }
#endif

    /* Save our entry count, so we can restore it when we regain control of
     * the monitor */
    thread->monitor_depth = monitor->depth;

    /* We're relinquishing the monitor. Get the next guy off the queue */
    removeMonitorWait(monitor);
}

/*=========================================================================
 * FUNCTION:      removeCondvarWait
 * TYPE:          Monitor handler
 * OVERVIEW:      Called to notify one thread waiting to be notified
 * INTERFACE:
 *   parameters:  monitor: Monitor
 *=======================================================================*/

static void
removeCondvarWait(MONITOR monitor, bool_t notifyAll) {
    do {

        THREAD waiter = removeQueueStart(&monitor->condvar_waitq);
        if (waiter == NULL) {
            break;
        }

        /* This thread now has to wait to get ownership of the monitor */
#if INCLUDEDEBUGCODE
        if (tracemonitors) {
            TraceMonitor(waiter, monitor, &monitor->condvar_waitq, FALSE);
        }
#endif

        /* Just in case we were set to get an alarm */
        removePendingAlarm(waiter);

        /* We can run as soon as we get the monitor again */
        addMonitorWait(monitor, waiter);

    } /* Once through if !notifyAll.  Until waiter == NULL, otherwise */
    while (notifyAll);
}

static bool_t
allocateFastLock(THREAD thread, OBJECT object, int depth, long hashCode) {
    if (IS_EXTENDED_LOCK_FREE(thread)) {
        /* thisLock->object = object; */
        thread->extendedLock.depth = depth;
        thread->extendedLock.hashCode = hashCode;
        SET_OBJECT_EXTENDED_LOCK(object, thread);
        return TRUE;
    } else {
        return FALSE;
    }
}

void
clearObjectMonitor(OBJECT object) {
    long hashCode;
    switch (OBJECT_MHC_TAG(object)) {
        case MHC_UNLOCKED:
            /* Do nothing */
            return;

        case MHC_SIMPLE_LOCK:
            hashCode = 0;
            break;

        case MHC_EXTENDED_LOCK: {
            THREAD thread = OBJECT_MHC_EXTENDED_THREAD(object);
            hashCode = thread->extendedLock.hashCode;
            FREE_EXTENDED_LOCK(thread);
            break;
        }

        default:
            hashCode = OBJECT_MHC_MONITOR(object)->hashCode;
            break;
    }
    SET_OBJECT_HASHCODE(object, hashCode);
}

static MONITOR
upgradeToRealMonitor(OBJECT object)
{
    enum MHC_Tag_TypeCode tag = OBJECT_MHC_TAG(object);
    MONITOR monitor;

    if (tag == MHC_MONITOR) {
        /* We already have a monitor, so there is no need to do anything */
        return OBJECT_MHC_MONITOR(object);
    }

    if (MonitorCache != NULL) {
        monitor = MonitorCache;
        MonitorCache = (MONITOR)monitor->owner;
        /* Make sure that the appropriate slots are zero.  We know that
         * the queues are zero, since otherwise the monitor wouldn't have
         * been freed in the first placed */
        monitor->owner = NULL;
        monitor->hashCode = 0;
        monitor->depth = 0;
    } else {
        START_TEMPORARY_ROOTS
            /* Create the monitor, while protecting the object from GC */
            DECLARE_TEMPORARY_ROOT(OBJECT, tObject, object);
            monitor = (MONITOR)callocObject(SIZEOF_MONITOR, GCT_MONITOR);
            object = tObject;
        END_TEMPORARY_ROOTS
    }

#if INCLUDEDEBUGCODE
    monitor->object = object;
#endif

    switch(tag) {
        case MHC_UNLOCKED:
            monitor->hashCode = object->mhc.hashCode;
            break;

        case MHC_SIMPLE_LOCK:
            monitor->owner = OBJECT_MHC_SIMPLE_THREAD(object);
            monitor->depth = 1;
            break;

        case MHC_EXTENDED_LOCK: {
            THREAD thread = OBJECT_MHC_EXTENDED_THREAD(object);
            monitor->owner = thread;
            monitor->depth = thread->extendedLock.depth;
            monitor->hashCode = thread->extendedLock.hashCode;
            /* Free this fast lock, since it is no longer in use */
            FREE_EXTENDED_LOCK(thread);
            break;

       default:                /* Needed to keep compiler happy */
            break;
        }
    }
    SET_OBJECT_MONITOR(object, monitor);
    return monitor;
}

long*
monitorHashCodeAddress(OBJECT object) {
    switch (OBJECT_MHC_TAG(object)) {
        case MHC_SIMPLE_LOCK: {
            THREAD thisThread = OBJECT_MHC_SIMPLE_THREAD(object);
            if (allocateFastLock(thisThread, object, 1, 0)) {
                return &thisThread->extendedLock.hashCode;
            } else {
                MONITOR monitor = upgradeToRealMonitor(object);
                /* object may be trash, because of a GC, but doesn't matter */
                return &monitor->hashCode;
            }
        }

        case MHC_EXTENDED_LOCK: {
            THREAD thread = OBJECT_MHC_EXTENDED_THREAD(object);
            return &thread->extendedLock.hashCode;
        }

        default:                /* Keep compiler happy */
            /* Do nothing */
            return NULL;
    }
}

/*=========================================================================
 * FUNCTION:      monitorEnter
 * TYPE:          Monitor handler
 * OVERVIEW:      Try to grab the monitor of an object
 * INTERFACE:
 *   parameters:  object: Any Java object
 *   returns:     MonitorStatusOwn:     own the monitor
 *                MonitorStatusWaiting: waiting for the monitor
 *=======================================================================*/

enum MonitorStatusType
monitorEnter(OBJECT object)
{
    THREAD thisThread = CurrentThread;
    long value = object->mhc.hashCode;
    MONITOR monitor;

#if INCLUDEDEBUGCODE
    const char *format =
        "Thread %lx enter %s monitor %lx (owner=%lx, depth=%lx)\n";
#endif

    switch (OBJECT_MHC_TAG(object)) {
        case MHC_UNLOCKED:
            if (value == MHC_UNLOCKED) {
                /* Our hash code is still 0.  We can just create a simple
                 * lock, which has implicit depth of 1 and an implicit hash
                 * code of 0.
                 */
                SET_OBJECT_SIMPLE_LOCK(object, thisThread);
#if INCLUDEDEBUGCODE
                if (tracemonitors) {
                    fprintf(stdout, format,
                            (long)thisThread, "simple", (long)object, 0L, 0L);
                }
#endif /* INCLUDEDEBUGCODE */

                return MonitorStatusOwn;
            } else if (allocateFastLock(thisThread, object, 1, value)) {
                /* We need a fast lock, since we already have a hash code.
                 * If the above allocation succeeds, then we're done.
                 */
#if INCLUDEDEBUGCODE
                if (tracemonitors) {
                    fprintf(stdout, format,
                            (long)thisThread, "fast", (long)object,
                            (long)thisThread, (long)0);
                }
#endif /* INCLUDEDEBUGCODE */

                return MonitorStatusOwn;
            }
            /* We have to do it the hard way */
            goto upgradeToRealMonitor;

        case MHC_SIMPLE_LOCK:
            if (OBJECT_MHC_SIMPLE_THREAD(object) == thisThread) {
                if (allocateFastLock(thisThread, object, 2, 0)) {
                    /* We need to upgrade from a simple lock to a fast lock,
                     * since the depth is now 2.  Indicate that the hash code
                     * is still implicitly 0.
                     */

#if INCLUDEDEBUGCODE
                    if (tracemonitors) {
                        fprintf(stdout, format,
                                (long)thisThread, "fast", (long)object,
                                (long)thisThread, (long)1);
                    }
#endif /* INCLUDEDEBUGCODE */

                    return MonitorStatusOwn;
                }
            }
            /* Either some other thread owns the lock, or no more fast locks */
            goto upgradeToRealMonitor;

        case MHC_EXTENDED_LOCK:
            if (OBJECT_MHC_EXTENDED_THREAD(object) == thisThread) {
                thisThread->extendedLock.depth++;

#if INCLUDEDEBUGCODE
                if (tracemonitors) {
                    fprintf(stdout, format,
                            (long)thisThread, "fast", (long)object,
                            (long)thisThread, (long)thisThread->extendedLock.depth);
                }
#endif /* INCLUDEDEBUGCODE */

                return MonitorStatusOwn;
            }
            goto upgradeToRealMonitor;

        upgradeToRealMonitor:
            /* Calling upgradeToRealMonitor may cause GC */
            monitor = upgradeToRealMonitor(object);
            thisThread = CurrentThread; /* in case of GC */
            break;

        default: /* case MHC_MONITOR: */
            monitor = OBJECT_MHC_MONITOR(object);
            break;

    }

#if INCLUDEDEBUGCODE
    if (tracemonitors) {
        fprintf(stdout, format,
                (long)thisThread, "slow", (long)object,
                monitor->owner, monitor->depth);
    }
#endif /* INCLUDEDEBUGCODE */

    /* If we reach this point, the monitor of the object is "monitor",
     * and thisThread contains the current thread */
    if (monitor->owner == NULL) {
        /* The monitor is unowned.  Make ourselves be the owner */
        monitor->owner = thisThread;
        monitor->depth = 1;
        return MonitorStatusOwn;
    } else if (monitor->owner == thisThread) {
        /* We already own the monitor.  Increase the entry count. */
        monitor->depth++;
        return MonitorStatusOwn;
    } else {
        /* Add ourselves to the wait queue.  Indicate that when we are
         * woken, the monitor's depth should be set to 1 */
        thisThread->monitor_depth = 1;
        addMonitorWait(monitor, thisThread);
        suspendThread();
        return MonitorStatusWaiting;
    }
}

/*=========================================================================
 * FUNCTION:      monitorExit
 * TYPE:          Monitor handler
 * OVERVIEW:      Relinquish ownership of a monitor
 * INTERFACE:
 *   parameters:  object: Any Java object
 *   returns:     MonitorStatusOwnMonitor:     still own the monitor
 *                MonitorStatusReleaseMonitor: releasing the monitor
 *                MonitorStatusError:          some exception occurred
 *=======================================================================*/

enum MonitorStatusType
monitorExit(OBJECT object, char** exceptionName)
{
    THREAD thisThread = CurrentThread;

#if INCLUDEDEBUGCODE
    const char *format =
        "Thread %lx exit %s monitor %lx (owner=%lx, depth=%lx)\n";
#endif

    *exceptionName = NULL;

    switch (OBJECT_MHC_TAG(object)) {
        case MHC_SIMPLE_LOCK:
            if (OBJECT_MHC_SIMPLE_THREAD(object) != thisThread) {
                break;
            }

            /* Releasing a simple lock is easy */
#if INCLUDEDEBUGCODE
            if (tracemonitors) {
                fprintf(stdout, format,
                        (long)thisThread, "simple", (long)object,
                        (long)thisThread, 1);
            }
#endif /* INCLUDEDEBUGCODE */

            SET_OBJECT_HASHCODE(object, 0);
            return MonitorStatusRelease;

        case MHC_EXTENDED_LOCK:
            if (OBJECT_MHC_EXTENDED_THREAD(object) != thisThread) {
                break;
            } else {
                int newDepth;

#if INCLUDEDEBUGCODE
                if (tracemonitors) {
                    fprintf(stdout, format,
                            (long)thisThread, "fast", (long)object,
                            (long)thisThread, (long)thisThread->extendedLock.depth);
                }
#endif /* INCLUDEDEBUGCODE */

                newDepth = --thisThread->extendedLock.depth;
                if (newDepth == 0) {
                    /* Release the fast lock.  No one is waiting for it */
                    SET_OBJECT_HASHCODE(object, thisThread->extendedLock.hashCode);
                    FREE_EXTENDED_LOCK(thisThread);
                    return MonitorStatusRelease;
                } else {
                    if (newDepth == 1 && thisThread->extendedLock.hashCode==0){
                        /* Simplify this to a simple lock */
                        FREE_EXTENDED_LOCK(thisThread);
                        SET_OBJECT_SIMPLE_LOCK(object, thisThread);
                    }
                    return MonitorStatusOwn;
                }
            }

        case MHC_MONITOR: {
            MONITOR monitor = OBJECT_MHC_MONITOR(object);
            if (monitor->owner != thisThread) {
                break;
            }

#if INCLUDEDEBUGCODE
            if (tracemonitors) {
                fprintf(stdout, format,
                        (long)thisThread, "slow", (long)object,
                        (long)monitor->owner, (long)monitor->depth);
            }
#endif /* INCLUDEDEBUGCODE */

            if (--monitor->depth == 0) {
                /* Let someone else have the monitor */
                removeMonitorWait(monitor);
                /* Is this now a dead monitor */
                if (   monitor->owner == NULL
                    && monitor->monitor_waitq == NULL
                    && monitor->condvar_waitq == NULL) {
                    /* Remove the monitor from the object */
                    SET_OBJECT_HASHCODE(object, monitor->hashCode);

                    /* Use the "owner" slot to keep a list
                     * of available monitors */
                    monitor->owner = (THREAD)MonitorCache;
                    MonitorCache = monitor;
                }
                return MonitorStatusRelease;
            } else {
                return MonitorStatusOwn;
            }
        }

        case MHC_UNLOCKED:
            break;
    }
    *exceptionName = (char*)IllegalMonitorStateException;
    return MonitorStatusError;
}

/*=========================================================================
 * FUNCTION:      monitorWait()
 * TYPE:          Monitor handler
 * OVERVIEW:      Wait for notification on the object
 * INTERFACE:
 *   parameters:  object: Any Java object
 *   returns:     MonitorStatusWaiting: waiting on the condvar
 *                MonitorStatusError:   some exception occurred
 *=======================================================================*/

enum MonitorStatusType
monitorWait(OBJECT object, long64 delta)
{
    /* For monitor wait, we always need a real monitor object. */
    MONITOR monitor = upgradeToRealMonitor(object);

#if INCLUDEDEBUGCODE
    if (tracemonitors) {
        fprintf(stdout, "Thread %lx waiting on monitor/condvar %lx\n",
                (long)CurrentThread, (long)object);
    }
#endif /* INCLUDEDEBUGCODE */

    if (monitor->owner != CurrentThread) {
        raiseException(IllegalMonitorStateException);
        return MonitorStatusError;
    }
    
    if (CurrentThread->isPendingInterrupt) { 
        handlePendingInterrupt();
        return MonitorStatusError;
    }

    if (ll_zero_gt(delta)) {
        registerAlarm(CurrentThread, delta, monitorWaitAlarm);
    }

    addCondvarWait(monitor, CurrentThread);
    suspendThread();

    return MonitorStatusWaiting;
}

/* Callback function if a thread's timer expires while waiting to be notified */

static void monitorWaitAlarm(THREAD thread)
{
    MONITOR monitor = thread->monitor;
    if (monitor != NULL) {
        if (removeFromQueue(&monitor->condvar_waitq, thread)) {
            addMonitorWait(monitor, thread);
        } else {
            fatalError(KVM_MSG_THREAD_NOT_ON_CONDVAR_QUEUE);
        }
    }
}

/*=========================================================================
 * FUNCTION:      monitorNotify()
 * TYPE:          Monitor handler
 * OVERVIEW:      Notify one process
 * INTERFACE:
 *   parameters:  object: Any Java object
 *                notifyAll:  true if waking up all waiters.
 *   returns:     MonitorStatusOwnMonitor: normal
 *                MonitorStatusError:      some exception occurred
 *=======================================================================*/

enum MonitorStatusType
monitorNotify(OBJECT object, bool_t notifyAll)
{
    long value = object->mhc.hashCode;
    void *address = (void *)(value & ~0x3);
    MONITOR monitor;

    switch (OBJECT_MHC_TAG(object)) {
        case MHC_SIMPLE_LOCK:
        case MHC_EXTENDED_LOCK:
            if (address != CurrentThread) {
                break;
            }
            /* We can't have any waiters, since this is a simple lock */
            return MonitorStatusOwn;

        case MHC_MONITOR: 
            /* Make sure current thread is the owner of the monitor */
            monitor = OBJECT_MHC_MONITOR(object);
            if (monitor->owner != CurrentThread) {
                break;
            }
#if INCLUDEDEBUGCODE
            if (tracemonitors) {
                fprintf(stdout, "Thread %lx %s monitor %lx\n",
                        (long)CurrentThread,
                        (notifyAll ? "notifying all in" : "notifying"),
                        (long)object);
            }
#endif /* INCLUDEDEBUGCODE */

            removeCondvarWait(OBJECT_MHC_MONITOR(object), notifyAll);
            return MonitorStatusOwn;

        case MHC_UNLOCKED:
            break;
    }
    raiseException(IllegalMonitorStateException);
    return MonitorStatusError;
}

/*=========================================================================
 * Queue manipulation functions (internal to this file)
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      addThreadToQueue()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Add a thread to the indicated queue
 * INTERFACE:
 *   parameters:  queue:      A pointer to the queue
                  thisThread: thread to add to the queue
                  where:      AT_START or AT_END
 *   returns:     <nothing>
 * NOTE:          There are no safety checks here, so please
 *                do not suspend the same thread twice.
 *=======================================================================*/

static void
addThreadToQueue(THREAD *queue, THREAD thisThread, queueWhere where)
{
    START_CRITICAL_SECTION
        if (*queue == NIL) {
            *queue = thisThread;
            thisThread->nextThread = thisThread;
        } else {
            /* Add thisThread after *queue, which always points
             * to the last element of the list */
            thisThread->nextThread = (*queue)->nextThread;
            (*queue)->nextThread = thisThread;
            if (where == AT_START) {
                /* We are already the first element of the queue */
                ;
            } else {
                /* make this thread be the last thread */
                *queue = thisThread;
            }
        }
    END_CRITICAL_SECTION
}

/*=========================================================================
 * FUNCTION:      removeQueueStart()
 * TYPE:          private instance-level operation
 * OVERVIEW:      Remove a thread by from start of the indicated queue
 * INTERFACE:
 *   parameters:  queue:  A pointer to the queue
 *   returns:     A THREAD pointer or NIL
 *=======================================================================*/

static THREAD removeQueueStart(THREAD *queue)
{
    THREAD thisThread;

    START_CRITICAL_SECTION
        if (*queue == NULL) {
            thisThread = NULL;
        } else {
            /* (*queue) points to the last item on a circular list */
            thisThread = (*queue)->nextThread;
            if (thisThread == *queue) {
                /* We were the only item on the list */
                *queue = NULL;
            } else {
                /* Remove us from the front of the queue */
                (*queue)->nextThread = thisThread->nextThread;
            }
            thisThread->nextThread = NIL;
        }
    END_CRITICAL_SECTION

    return thisThread;
}

/*=========================================================================
 * FUNCTION:      removeFromQueue()
 * TYPE:          private instance-level operation
 * OVERVIEW:      Remove a thread from the queue
 * INTERFACE:
 *   parameters:  queue:  A pointer to the queue
 *                waiter: The item to remove
 *   returns:     TRUE if the item was found on the queue, FALSE otherwise
 *=======================================================================*/

static bool_t
removeFromQueue(THREAD *queue_p, THREAD waiter)
{
    bool_t result;
    THREAD queue = *queue_p;
    START_CRITICAL_SECTION
    if (queue == NULL) {
        result = FALSE;
    } else {
        THREAD prev_q = queue;
        THREAD q = queue->nextThread;

        while (q != queue && q != waiter) {
            /* We start at the first item on the queue, and circle until
             * we reach the last item */
            prev_q = q;
            q = q->nextThread;
        }

        /* Note, q == queue is ambiguous.  Either we were searching for the
         * last item on the list, or the item can't be found */

        if (q != waiter) {
            result = FALSE;
        } else {
            prev_q->nextThread = q->nextThread;
            q->nextThread = NULL;
            if (q == queue) {
                /* We were deleting the final item on the list */
                *queue_p = (prev_q == q)
                    ? NULL  /* Deleted last item on the list */
                    : prev_q; /* Still more items on the list */
            }
            result = TRUE;
        }
    }
    END_CRITICAL_SECTION
    return (result);
}

static int
queueLength(THREAD queue) {
    int length = 0;
    START_CRITICAL_SECTION
    if (queue == NULL) {
        ;
    } else {
        THREAD thread = queue;
        do {
            length++;
            thread = thread->nextThread;
        } while (thread != queue);
    }
    END_CRITICAL_SECTION
    return (length);
}

/*=========================================================================
 * Debugging and printing operations
 *=======================================================================*/

#if INCLUDEDEBUGCODE

static void
TraceThread(THREAD thread, char* what) {
    fprintf(stdout, "Thread %lx: %s\n", (long)thread, what);
}

static void
TraceMonitor(THREAD thread, MONITOR monitor, THREAD *queuep, bool_t added) {
    int count = 0;
    THREAD queue = *queuep;
    if (queue != NULL) {
        THREAD q = queue;
        do {
            q = q->nextThread;
            count++;
        } while (q != queue);
    }
    fprintf(stdout,
            "Thread %lx %s %s queue %lx (queue length now %d)\n",
            (long)thread,
            added ? "added to" : "removed from",
            queuep == &monitor->monitor_waitq ? "monitor" : "condvar",
            (long)monitor->object,
            count);
}

static void
printMonitor(OBJECT object) {
    switch (OBJECT_MHC_TAG(object)) {
        case MHC_UNLOCKED:
            break;

        case MHC_SIMPLE_LOCK: {
            THREAD thread = OBJECT_MHC_SIMPLE_THREAD(object);
            fprintf(stdout, "Object %lx simple lock on thread %lx\n",
                   (long)object, (long)thread);
            break;
        }

        case MHC_EXTENDED_LOCK: {
            THREAD thread = OBJECT_MHC_EXTENDED_THREAD(object);
            fprintf(stdout, "Object %lx fast lock on thread %lx, depth=%lx\n",
                   (long)object, (long)thread, (long)thread->extendedLock.depth);
            break;
        }

        case MHC_MONITOR: {
            MONITOR monitor = OBJECT_MHC_MONITOR(object);
            fprintf(stdout, "Object %lx full monitor on thread %lx, depth=%lx\n",
                   (long)object, (long)monitor->owner, (long)monitor->depth);
            break;
        }
    }
}

void
printAllMonitors(void)
{
    HASHTABLE stringTable;
    cell* scanner = CurrentHeap;
    cell* heapSpaceTop = CurrentHeapEnd;
    for ( ; scanner < heapSpaceTop; scanner += SIZE(*scanner) + HEADERSIZE) {
        int type = TYPE(*scanner);
        if (type == GCT_INSTANCE
               || type == GCT_ARRAY || type == GCT_OBJECTARRAY) {
            OBJECT object = (OBJECT)(scanner + HEADERSIZE);
            printMonitor(object);
        }
    }

    stringTable = InternStringTable;
    if (ROMIZING || stringTable != NULL) {
        int count = stringTable->bucketCount;
        while (--count >= 0) {
            INTERNED_STRING_INSTANCE instance =
                (INTERNED_STRING_INSTANCE)stringTable->bucket[count];
            printMonitor((OBJECT)instance);
        }
    }

    if (ROMIZING || ClassTable != NULL) {
        FOR_ALL_CLASSES(clazz)
            printMonitor((OBJECT)clazz);
        END_FOR_ALL_CLASSES
    }
}

#endif /* INCLUDEDEBUGCODE */

