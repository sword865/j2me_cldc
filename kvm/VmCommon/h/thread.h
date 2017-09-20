/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Thread (concurrency) management
 * FILE:      thread.h
 * OVERVIEW:  This file defines the structures that are needed
 *            for Java-style multithreading.
 * AUTHOR:    Antero Taivalsaari, Sun Labs, 1998
 *            Edited by Doug Simon 11/1998
 *            Edited by Nik Shaylor 09/1999 to allow asynchronous I/O
 *            Frank Yellin (new monitors), Bill Pittore (debugging)
 *=======================================================================*/

/*=========================================================================
 * ORIGINAL COMMENTS (PRE-KVM 1.0)
 *
 * KVM has a machine-independent, portable, pre-emptive
 * threading model that is capable of operating independently
 * of the Java language.
 *
 * A simple round-robin style scheduling model is used, in which
 * all the active threads in the system are stored in a circular
 * linked list.
 *
 * Threads in the list are given execution time one after each other
 * based on the Java-level priority of each task.  At the implementation
 * level thread priority is simply an integer that tells to the
 * interpreter how many primitives the thread may execute until the
 * next thread switch takes place. After each bytecode execution,
 * the "timeslice" counter of the thread is decremented.  When
 * timeslice becomes zero, a thread switch is forced. Some I/O
 * primitives may also initiate a thread switch.
 *=======================================================================*/

/*=========================================================================
 * NEW COMMENTS (KVM 1.0)
 * ======================
 *
 * The thread model described above has been changed a little to allow
 * asynchronous (non-blocking) I/O to be supported by the VM.
 *
 * An additional goal was to introduce the ability to easily implement
 * alternative scheduling mechanisms, but without changing the existing
 * system too radically.
 *
 * In the original system context switching occurred in a number of places.
 * Now it is only ever done at the top of the interpreter dispatch loop.
 * In the original version, runnable threads were kept in a circular list
 * with the currently executing one. Now each thread is removed from the
 * circular list of runnable threads when the thread starts executing and
 * is returned there when it is blocked.
 *
 * The variable UP has been renamed CurrentThread and it points to the
 * currently executing thread.  A new variable RunnableThreads points to
 * the circular list of runnable threads.
 *
 * NOTE: RunnableThreads points to the >>last<< item on the circular queue.
 * This makes it easy to add items to either the front or back of the queue.
 *
 * Important routine changes:
 *
 * suspendThread()
 * ---------------
 *
 * This used to have a parameter of which thread to suspend, but there is no
 * case in which it cannot be UP so the parameter has been removed. This
 * routine used to perform a context switch, but now it just zeros
 * CurrentThread and calls a new function called signalTimeToReschedule()
 * which signals to the interpreter that it should reschedule the VM before
 * the next bytecode is executed.
 *
 * A new function isTimeToReshedule() is provided for the interpreter to
 * test this condition, and a new function reschedule() is called to do
 * this. The code at the top of the interpreter bytecode dispatch loop is now:
 *
 *    if (isTimeToReshedule()) {
 *        reschedule();
 *    }
 *
 * Because of the time critical nature of this part of the VM these two
 * functions are implemented as macros. The current version of
 * isTimeToReshedule() expands to "Timeslice-- == 0" which is identical
 * to the code in the original threading system.  The function
 * signalTimeToReschedule() simply sets Timeslice to zero.
 *
 * startThread()
 * -------------
 *
 * This makes a thread alive, but suspended (ie. doesn't start
 * execution immediately).
 *
 * resumeThread()
 * ---------------
 *
 * This function places a thread back on the runnable thread list
 * (RunnableThreads) and will also call signalTimeToReschedule() if the
 * priority of the activated thread is higher than the priority of
 * currently executing thread.
 *
 * The new thread is put in the RunnableThreads list in a position where
 * it will be the next one to be executed. Unlike the earlier thread
 * scheduling mechanism, this will allow higher priority threads to
 * get switched to immediately in an interrupt driven environment.
 *
 * The intention of this change is to implement a more conventional
 * event driven priority scheduling mechanism. This was the primary
 * motivation for removing the active thread from the runnable queue
 * when it is executing. We can now completely change the format of
 * the runnable queue without affecting the rest of the system.
 *
 * BuildThread()
 * -------------
 *
 * This now adds all new threads to another list of all alive threads called
 * AllThreads. This list is used by the garbage collector.
 *
 * DismantleThread()
 * -----------------
 *
 * Removes the thread from the AllThreads list.
 *
 * stopThread()
 * ------------
 *
 * This function is the logical opposite to BuildThread(). It suspends the
 * thread and calls DismantleThread().
 *
 * resumeThread()
 * ---------------
 *
 * This replaces the use of activateThread() to restart a suspended thread.
 *
 * isActivated()
 * -------------
 *
 * It used to be necessary to test that CurrentThread was non-null before
 * calling this routine.
 *
 * HandleEvent()
 * -------------
 *
 * Because the original HandleEvent routine called activateThread it did
 * not require any changes (except for the calling convention for
 * isActivated()).  However, the meaning of the return parameter is
 * now slightly different in that is indicates that a new thread was
 * added to RunnableThreads and not that it just switched context.
 *
 * SwitchThread()
 * ---------------
 *
 * SwitchThread is now only called from reschedule(). CurrentThread may or
 * may not point to a thread. If it does we rotate the thread through the
 * RunnableThreads list as before. If it is null and the RunnableThreads
 * list is empty then the function returns FALSE and reschedule() must
 * decide what to do.
 *
 * JLT_Yield()
 * -----------
 *
 * Now only has to call signalTimeToReschedule()
 *
 *
 * Asynchronous I/O (optional feature)
 * ===================================
 *
 * The basic thread management strategy of KVM is unchanged in that
 * all the Java code is executed by the same native thread, and all
 * Java thread context switching is done internally using the
 * original 'green thread' concept. This is generally the preferred
 * multithreading mechanism for KVM.
 *
 * Asynchronous I/O is an optional KVM feature that can be used
 * if the underlying operating system provides the necessary
 * support, e.g., in the form of interrupts, callback routines,
 * or native threads. However, be aware that the use of
 * asynchronous I/O involves a substantial amount of additional
 * work and complexity.
 *
 * When the author of a Java native function wants to allow the
 * interpreter to execute while the I/O is being performed, he
 * should call suspendThread(), then set up the appropriate
 * platform dependent mechanism to be notified of the I/O
 * completion, and then return. >>> It is very important that
 * the call to suspendThread() occurs before the callback
 * routine is run. <<<
 *
 * When the callback routine executes, it will almost always wants
 * to push some result on the stack of the suspended thread, and
 * then call resumeThread().
 *
 * When a native function is called in KVM, it is generally running
 * in the context of the calling thread. This is not the case when
 * a callback routine (or whatever mechanism you are using to
 * report the results of an asynchronous call) is called. Therefore,
 * the normal stack access functions and macros (popStack() etc.)
 * cannot be used when writing callback routines. To solve this
 * problem, special new versions of these stack manipulation
 * functions are provided. These new routines all have an additional
 * "FromThread" parameter to indicate the requested context. For
 * instance, popStack() becomes popStackFromThread(THREAD* FromThread).
 * Using these new, alternative macros, the thread's stack can
 * be set up with the appropriate parameters prior to calling
 * activateThread().
 *
 * An additional problem with asynchronous I/O is the possibility
 * of a race condition. To prevent a race condition corrupting the
 * RunnableThreads queue, which links asynchronous world outside
 * the VM with its internal rescheduling functions, all accesses
 * to this queue much be interlocked with a critical section.
 * Therefore, two new platform dependent functions have been
 * defined:
 *
 *     enterSystemCriticalSection()
 *
 * and
 *
 *     exitSystemCriticalSection()
 *
 *=======================================================================*/

/*=========================================================================
 * NEW COMMENTS (KVM 1.0.3)
 * ========================
 *
 * KVM 1.0.3 has a new monitor/synchronization implementation.
 * Read the description later in this file.  Otherwise, there
 * are no significant multithreading changes between KVM 1.0
 * and KVM 1.0.3.
 *=======================================================================*/

/*=========================================================================
 * Global definitions and variables needed for multitasking
 *=======================================================================*/

#ifndef __THREAD_H__
#define __THREAD_H__

extern THREAD CurrentThread;    /* Current thread */
extern THREAD MainThread;       /* For debugger code to access */

extern THREAD AllThreads;       /* List of all threads */
extern THREAD RunnableThreads;  /* Queue of all threads that can be run */

extern int AliveThreadCount;    /* Number of alive threads */

extern int Timeslice;           /* Time slice counter for multitasking */

#define areActiveThreads() (CurrentThread != NULL || RunnableThreads != NULL)
#define areAliveThreads()  (AliveThreadCount > 0)

#define MAX_PRIORITY  10        /* These constants must be the same */
#define NORM_PRIORITY 5         /* as those defined in Thread.java */
#define MIN_PRIORITY  1

/*=========================================================================
 * Global multitasking operations
 *=======================================================================*/

void storeExecutionEnvironment(THREAD thisThread); /* Context save */
void loadExecutionEnvironment(THREAD thisThread);  /* Context restore */
bool_t SwitchThread(void);

/*=========================================================================
 * Thread data structures and operations
 *=======================================================================*/

/*=========================================================================
 * Thread instance data structures:
 *
 * One of the structures below is instantiated
 * for every thread (task) in the system:
 *
 * - THREAD is an internal (VM-level) structure that is used for storing
 *   the necessary information for implementing preemptive task
 *   switching independently of Java-level stuff. Active THREADs
 *   are singly-linked to a linear list that contains all the
 *   active threads in the system.
 *
 * - JAVATHREAD is mapped one to one to the instance structure
 *   defined in Java class 'Thread.java'. It is referred to from
 *   THREAD.
 *
 * Why do we need two separate structures? Well, since class 'Thread.java'
 * may be subclassed, we cannot add the necessary implementation-level
 * fields directly to JAVATHREAD, but we must use a separate struct instead!
 * Also, we wanted to keep the thread system implementation independent
 * of the Java language for portability reasons.
 *
 * Note that a JAVATHREAD must be a full-fledged Java object instance.
 * Thus its beginning must be identical with that of INSTANCE.
 *
 * IMPORTANT: Do not change the order of the data elements below
 * unless you do the corresponding modifications also in the data
 * fields of class java.lang.Thread!!
 *
 *=======================================================================*/

/* THREAD */
struct threadQueue {
    THREAD nextAliveThread;  /* Queue of threads that are alive; */
    THREAD nextThread;       /* Queue of runnable or waiting threads */
    JAVATHREAD javaThread;   /* Contains Java-level thread information */
    long   timeslice;        /* Calculated from Java-level thread priority */
    STACK stack;             /* The execution stack of the thread */

    /* The following four variables are used for storing the */
    /* virtual machine registers of the thread while the thread */
    /* is runnable (= active but not currently given processor time). */
    /* In order to facilitate garbage collection, we store pointers */
    /* to execution stacks as offsets instead of real pointers. */
    BYTE* ipStore;           /* Program counter temporary storage (pointer) */
    FRAME fpStore;           /* Frame pointer temporary storage (pointer) */
    cell* spStore;           /* Execution stack pointer temp storage (ptr) */
    cell* nativeLp;          /* Used in KNI calls to access local variables */

    MONITOR monitor;         /* Monitor whose queue this thread is on */
    short monitor_depth;

    THREAD nextAlarmThread;  /* The next thread on this queue */
    long   wakeupTime[2];    /* We can't demand 8-byte alignment of heap
                                objects  */
    void (*wakeupCall)(THREAD); /* Callback when thread's alarm goes off */
    struct {
        int depth;
        long hashCode;
    } extendedLock;          /* Used by an object with a FASTLOCK */

    char* pendingException;  /* Name of an exception (class) that the */
                             /* thread is about to throw */
    char* exceptionMessage;  /* Message string for the exception */

    enum {
        THREAD_JUST_BORN = 1,     /* Not "start"ed yet */
        THREAD_ACTIVE = 2,        /* Currently running, or on Run queue */
        THREAD_SUSPENDED = 4,     /* Waiting for monitor or alarm */
        THREAD_DEAD = 8,          /* Thread has quit */
        THREAD_MONITOR_WAIT = 16,
        THREAD_CONVAR_WAIT = 32,
        THREAD_DBG_SUSPENDED = 64
    } state;
    bool_t isPendingInterrupt;    /* Don't perform next sleep or wait */

#if ENABLE_JAVA_DEBUGGER
    bool_t isStepping;       /* Single stepping this thread */
    bool_t isAtBreakpoint;
    bool_t needEvent;
    CEModPtr debugEvent;

    ByteCode nextOpcode;
    struct singleStep_mod stepInfo;
    int debugSuspendCount; /* Number of times debugger suspended thread */
#endif /* ENABLE_JAVA_DEBUGGER */

}; /* End of THREAD data structure definition */

#define SIZEOF_THREAD        StructSizeInCells(threadQueue)

/* JAVATHREAD
 *
 * DANGER: This structure is mapped one-to-one with Java thread instances
 *         The fields should be the same as in 'java.lang.Thread'
*/
struct javaThreadStruct {    /* (A true Java object instance) */

    COMMON_OBJECT_INFO(INSTANCE_CLASS)
    long       priority;     /* Current priority of the thread */
    THREAD     VMthread;     /* Backpointer to internal THREAD. */
    INSTANCE   target;       /* Pointer to the object whose 'run' */
    SHORTARRAY name;         /* Name of the thread (new in CLDC 1.1) */
                             /* The name is stored as a Java char array */
};

#define SIZEOF_JAVATHREAD    StructSizeInCells(javaThreadStruct)

/*=========================================================================
* Public operations on thread instances
 *=======================================================================*/

/*     Constructors and destructors */
void   InitializeThreading(INSTANCE_CLASS, ARRAY);
THREAD getVMthread(JAVATHREAD_HANDLE);

/*     Thread activation and deactivation operations */
void   initThreadBehavior(THREAD, METHOD, OBJECT);
void   startThread(THREAD);
void   stopThread(void);
void   interruptThread(THREAD);
void   handlePendingInterrupt(void);
void   DismantleThread(THREAD);
void   suspendThread(void);
void   suspendSpecificThread(THREAD);
void   resumeThread(THREAD);
void   resumeSpecificThread(THREAD);
int    activeThreadCount(void);
int    isActivated(THREAD);
THREAD removeFirstRunnableThread(void);

/*     Asynchronous threading operations (optional) */
#if ASYNCHRONOUS_NATIVE_FUNCTIONS
void   asyncFunctionProlog(void);
void   asyncFunctionEpilog(THREAD);
void   RundownAsynchronousFunctions(void);
void   RestartAsynchronousFunctions(void);
#else
#define RundownAsynchronousFunctions() /**/
#define RestartAsynchronousFunctions() /**/
#endif

#if ASYNCHRONOUS_NATIVE_FUNCTIONS
void enterSystemCriticalSection(void);
void exitSystemCriticalSection(void);
#define START_CRITICAL_SECTION { enterSystemCriticalSection();
#define END_CRITICAL_SECTION   exitSystemCriticalSection(); }
#else
#define START_CRITICAL_SECTION {
#define END_CRITICAL_SECTION   }
#endif

#ifndef NATIVE_FUNCTION_COMPLETED
#define NATIVE_FUNCTION_COMPLETED()  /* do nothing by default */
#endif

/* Thread debugging operations */
#if INCLUDEDEBUGCODE
void printFullThreadStatus(void);
#else
#define printFullThreadStatus()
#endif

/*=========================================================================
 * Timer data structures and operations
 *=======================================================================*/

/* Threads waiting for a timer interrupt */
extern THREAD TimerQueue;

/*=========================================================================
 * Timer operations
 *=======================================================================*/

void   registerAlarm(THREAD thread, long64 delta, void (*)(THREAD));
void   checkTimerQueue(ulong64 *nextTimerDelta);
bool_t inTimerQueue(THREAD thread);

/*=========================================================================
 * Monitor data structures and operations
 *=======================================================================*/

extern MONITOR MonitorCache;

/*=========================================================================
 * COMMENTS:
 * KVM has a general, machine-independent monitor implementation
 * that is used when executing synchronized method calls and
 * MONITORENTER/MONITOREXIT bytecodes.
 *
 * Monitors are always attached to individual object instances
 * (or class instances when executing synchronized static method
 * calls). Each monitor structure holds a wait queue in which
 * those threads who are waiting for the monitor to be released
 * are stored. A democratic FCFS (first-come-first-serve) scheduling
 * model is used for controlling wait queues. Threads are
 * automatically suspended while they wait in a monitor.
 *
 *=========================================================================
 * NEW COMMENTS FOR 1.0.3:
 *
 * KVM 1.0.3 has a new monitor system that allocates real
 * monitor objects only when they are really needed.
 * If a monitor is uncontended (only one thread needs
 * to acquire the monitor), no monitor object is allocated
 * at all. Rather, the mch (monitorOrHashCode) field of the
 * object is used for storing information about the monitor
 * that is associated with that object.  The mch field can
 * be in four different states depending on the status of
 * the system.  The different mch states are explained below.
 *=======================================================================*/

/* These are the possible meanings of the low two bits of an object's .mhc
 * field.  If the upper 30 bits point to a thread or monitor, the low two
 * bits are implicitly zero.
 */
enum MHC_Tag_TypeCode {
    /* The upper 30 bits are the hash code. 0 means no hash code */
    MHC_UNLOCKED  = 0,

    /* The upper 30 bits are the thread that locks this object.
     * This object implicitly has no hash code, and the locking
     * depth is 1.  There is no other contention for this object */
    MHC_SIMPLE_LOCK  = 1,

    /* The upper 30 bits are the thread that locks this object.
     * thread->extendedLock.hashCode & thread->extendedLock->depth
     * contain this object's hashCode and and locking depth.
     * There is no other contention for this object. */
    MHC_EXTENDED_LOCK = 2,

    /* The upper 30 bits point to a MONITOR object */
    MHC_MONITOR  = 3
};

/* These macros are used to extract the .mhc field of the object in a
 * meaningful way.
 */
#define OBJECT_MHC_TAG(obj) ((enum MHC_Tag_TypeCode)((obj)->mhc.hashCode & 0x3))
#define OBJECT_MHC_MONITOR(obj)  \
             ((MONITOR)(((char *)(obj)->mhc.address) - MHC_MONITOR))
#define OBJECT_MHC_SIMPLE_THREAD(obj) \
             ((THREAD)(((char *)(obj)->mhc.address) - MHC_SIMPLE_LOCK))
#define OBJECT_MHC_EXTENDED_THREAD(obj) \
             ((THREAD)(((char *)(obj)->mhc.address) - MHC_EXTENDED_LOCK))

/* Utility macros for checking and setting lock values */
#define IS_EXTENDED_LOCK_FREE(thread) (thread->extendedLock.depth == 0)
#define FREE_EXTENDED_LOCK(thread)    (thread->extendedLock.depth = 0)

/* MONITOR */
struct monitorStruct {
    THREAD owner;          /* Current owner of the monitor */
    THREAD monitor_waitq;
    THREAD condvar_waitq;
    long   hashCode;       /* Hash code of original object */
    long   depth;          /* Nesting depth: how many times the */
                           /* monitor has been entered recursively */
#if INCLUDEDEBUGCODE
    OBJECT object;
#endif
};

#define SIZEOF_MONITOR     StructSizeInCells(monitorStruct)

/* These are the possible return values from monitorEnter, monitorExit,
 * monitorWait, and monitorNotify.
 */
enum MonitorStatusType {
    MonitorStatusOwn,       /* The process owns the monitor */
    MonitorStatusRelease,   /* The process has released the monitor */
    MonitorStatusWaiting,   /* The process is waiting for monitor/condvar */
    MonitorStatusError      /* An error has occurred */
};

/*=========================================================================
 * Monitor operations
 *=======================================================================*/

enum MonitorStatusType  monitorEnter(OBJECT object);
enum MonitorStatusType  monitorExit(OBJECT object, char **exceptionName);
enum MonitorStatusType  monitorWait(OBJECT object, long64 time);
enum MonitorStatusType  monitorNotify(OBJECT object, bool_t wakeAll);

/* Remove any monitor associated with this object */
void clearObjectMonitor(OBJECT object);

/* If this object has a monitor or monitor-like structure associated with it,
 * then return the address of its "hash code" field.  This function may GC
 * if it returns a non-NULL value.
 */
long* monitorHashCodeAddress(OBJECT object);

#if INCLUDEDEBUGCODE
void printAllMonitors(void);
#endif

#endif /* __THREAD_H__ */

