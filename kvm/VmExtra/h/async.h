/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: KVM
 * FILE:      async.h
 * OVERVIEW:  Definitions to support (and not support) asynchronous I/O
 * AUTHOR:    Nik Shaylor, Sun C&E
 *=======================================================================*/

#ifndef KVM_ASYNC_H
#define KVM_ASYNC_H

/*
 * Data structure used in all cases
 */
typedef struct asynciocb {
    struct asynciocb   *nextFree;
    THREAD              thread;
    INSTANCE            instance;
    BYTEARRAY           array;
    char               *exception;
} ASYNCIOCB;

#if !ASYNCHRONOUS_NATIVE_FUNCTIONS

/*
 * NO ASYNCHRONOUS_NATIVE_FUNCTIONS
 */

#define ASYNC_FUNCTION(x)                                               \
void x(void) {                                                          \
    static void internal ## x (ASYNCIOCB *);                            \
    ASYNCIOCB anaiocb;                                                  \
    ASYNCIOCB *aiocb = &anaiocb;                                        \
    aiocb->thread       = CurrentThread;                                \
    aiocb->instance     = 0;                                            \
    aiocb->array        = 0;                                            \
    aiocb->exception    = 0;                                            \
    internal ## x (aiocb);                                              \
}                                                                       \
static void internal ## x (ASYNCIOCB *aiocb)

#define ASYNC_popStack            popStack
#define ASYNC_pushStack           pushStack
#define ASYNC_popStackAsType      popStackAsType
#define ASYNC_pushStackAsType     pushStackAsType
#define ASYNC_popLong             popLong
#define ASYNC_pushLong            pushLong
#define ASYNC_raiseException(x)   (aiocb->exception = (x))
#define ASYNC_oneLess             oneLess;
#define ASYNC_resumeThread()      if(aiocb->exception != 0) raiseException(aiocb->exception)

#define ASYNC_enableGarbageCollection() /**/
#define ASYNC_disableGarbageCollection() /**/

#else

/*
 * ASYNCHRONOUS_NATIVE_FUNCTIONS
 */

ASYNCIOCB *AcquireAsyncIOCB(void);
void       ReleaseAsyncIOCB(ASYNCIOCB *);
void       AbortAsyncIOCB(ASYNCIOCB *);

/*
 * The number if I/O control blocks in the system
 */
#ifndef ASYNC_IOCB_COUNT
#define ASYNC_IOCB_COUNT 5
#endif

/*
 * The transfer buffer size. By default it is large enough
 * to hold one standard ethernet datagram. Note that this
 * buffer is allocated on the stack of the I/O threads.
 */
#ifndef ASYNC_BUFFER_SIZE
#define ASYNC_BUFFER_SIZE 1500
#endif

#define ASYNC_FUNCTION(x)                                               \
void x(void) {                                                          \
    static void internal ## x (ASYNCIOCB *);                            \
    ASYNCIOCB *aiocb = AcquireAsyncIOCB();                              \
    CallAsyncNativeFunction_md(aiocb, internal ## x);                   \
}                                                                       \
static void internal ## x (ASYNCIOCB *aiocb)

#define ASYNC_popStack()                      popStackForThread(aiocb->thread)
#define ASYNC_pushStack(x)                    pushStackForThread(aiocb->thread,x)
#define ASYNC_popStackAsType(_type_)          popStackAsTypeForThread(aiocb->thread, _type_)
#define ASYNC_pushStackAsType(_type_, x)      pushStackAsTypeForThread(aiocb->thread,_type_,x)
#define ASYNC_popLong(x)                      popLongForThread(aiocb->thread,x)
#define ASYNC_pushLong(x)                     pushLongForThread(aiocb->thread,x)
#define ASYNC_raiseException(x)               (aiocb->thread->pendingException = (x))
#define ASYNC_oneLess                         oneLessForThread(aiocb->thread)
#define ASYNC_resumeThread()                  ReleaseAsyncIOCB(aiocb)

extern void decrementAsyncCount();
extern void incrementAsyncCount();

#if EXCESSIVE_GARBAGE_COLLECTION

extern void RequestGarbageCollection();

#define ASYNC_enableGarbageCollection()  {                              \
    extern int VersionOfTheWorld;                                       \
    int _verson_ = VersionOfTheWorld;                                   \
    decrementAsyncCount();                                              \
    RequestGarbageCollection(); /* only when testing */                 \

#define ASYNC_disableGarbageCollection()                                \
    if (_verson_ != VersionOfTheWorld) {                                \
        /* This is not the same system that we started with so          \
           just release the IOCB and exit from the native method */     \
        AbortAsyncIOCB(aiocb);                                          \
        return;                                                         \
    }                                                                   \
    incrementAsyncCount();                                              \
    RequestGarbageCollection(); /* only when testing */                 \
}

#else

#define ASYNC_enableGarbageCollection()  {                              \
    extern int VersionOfTheWorld;                                       \
    int _verson_ = VersionOfTheWorld;                                   \
    decrementAsyncCount();                                              \

#define ASYNC_disableGarbageCollection()                                \
    if (_verson_ != VersionOfTheWorld) {                                \
        /* This is not the same system that we started with so          \
           just release the IOCB and exit from the native method */     \
        AbortAsyncIOCB(aiocb);                                          \
        return;                                                         \
    }                                                                   \
    incrementAsyncCount();                                              \
}

#endif /* EXCESSIVE_GARBAGE_COLLECTION */

#endif /* !ASYNCHRONOUS_NATIVE_FUNCTIONS */

#define ASYNC_FUNCTION_START(x)  ASYNC_FUNCTION(x) {
#define ASYNC_FUNCTION_END       ASYNC_resumeThread(); }

#endif /* KVM_ASYNC_H */

