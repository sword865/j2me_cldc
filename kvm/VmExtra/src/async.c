/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: KVM
 * FILE:      async.c
 * OVERVIEW:  Functions to support asynchronous I/O
 * AUTHOR:    Nik Shaylor, Sun C&E
 *=======================================================================*/
 
/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>
#include <async.h>

/*=========================================================================
 * Variables
 *=======================================================================*/

ASYNCIOCB  IocbRoots[ASYNC_IOCB_COUNT];
ASYNCIOCB *IocbFreeList = 0;

/*=========================================================================
 * Functions
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      AcquireAsyncIOCB()
 * TYPE:          Public routine for asynchronous native methods
 * OVERVIEW:      Allocate an Async I/O control block
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     An ASYNCIOCB
 *=======================================================================*/

ASYNCIOCB *AcquireAsyncIOCB(void) {
    ASYNCIOCB *result = 0;
    while (result == 0) {
        START_CRITICAL_SECTION
            if (IocbFreeList != 0) {
                result       = IocbFreeList;
                IocbFreeList = IocbFreeList->nextFree;
                result->nextFree = 0;
            }
        END_CRITICAL_SECTION
        if (result == 0) {
            Yield_md();
        }
    }
    if (result->thread != 0) {
        fatalError(KVM_MSG_PROBLEM_IN_ACQUIRE_ASYNC_IOCB);
    }
    result->thread = CurrentThread;
    asyncFunctionProlog();
    return result;
}

/*=========================================================================
 * FUNCTION:      FreeAsyncIOCB()
 * TYPE:          Private routine for asynchronous native methods
 * OVERVIEW:      Free an Async I/O control block
 * INTERFACE:
 *   parameters:  An ASYNCIOCB
 *   returns:     <nothing>
 *=======================================================================*/

static void FreeAsyncIOCB(ASYNCIOCB *aiocb) {
    aiocb->thread       = 0;
    aiocb->instance     = 0;
    aiocb->array        = 0;
    aiocb->exception    = 0;
    aiocb->nextFree     = IocbFreeList;
    IocbFreeList        = aiocb;
}

/*=========================================================================
 * FUNCTION:      AbortAsyncIOCB()
 * TYPE:          Public routine for asynchronous native methods
 * OVERVIEW:      Free an Async I/O control block
 * INTERFACE:
 *   parameters:  An ASYNCIOCB
 *   returns:     <nothing>
 *=======================================================================*/

void AbortAsyncIOCB(ASYNCIOCB *aiocb) {
    START_CRITICAL_SECTION
        FreeAsyncIOCB(aiocb);
    END_CRITICAL_SECTION
}

/*=========================================================================
 * FUNCTION:      ReleaseAsyncIOCB()
 * TYPE:          Public routine for asynchronous native methods
 * OVERVIEW:      Free an Async I/O control block
 * INTERFACE:
 *   parameters:  An ASYNCIOCB
 *   returns:     <nothing>
 *=======================================================================*/

void ReleaseAsyncIOCB(ASYNCIOCB *aiocb) {
    if (aiocb->thread == 0) {
        fatalError(KVM_MSG_PROBLEM_IN_RELEASE_ASYNC_IOCB);
    }
    asyncFunctionEpilog(aiocb->thread);
    AbortAsyncIOCB(aiocb);
}

/*=========================================================================
 * FUNCTION:      ActiveAsyncOperations()
 * TYPE:          Private routine for asynchronous native methods
 * OVERVIEW:      Return the number of active IOCBs
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     Number of active IOCBs
 *=======================================================================*/

static int ActiveAsyncOperations() {
    int active = ASYNC_IOCB_COUNT;
    ASYNCIOCB *aiocb = IocbFreeList;
    while (aiocb != 0) {
        aiocb = aiocb->nextFree;
        active--;
    }
    return active;
}

/*=========================================================================
 * FUNCTION:      InitalizeAsynchronousIO()
 * TYPE:          Public routine for asynchronous native methods
 * OVERVIEW:      Initialize the system
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

/*
 * A brief explanation of "VersionOfTheWorld"
 *
 * This variable is incremented every time InitalizeAsynchronousIO is called.
 * The first time the code will initialize the queue of IOCBs. The subsequent
 * times the code will wait for any I/O activity to end before returning.
 *
 * The only systems that need this feature are those who restart the VM after
 * it has ended, These are only the test suites used by CLDC and MIDP.
 *
 * The implication is that these systems must correctly rundown all async I/O
 * before starting up a new VM. This is typically done by the native function
 * finalization code which will typically do something like close a socket.
 * This in turn should wake up any threads that are waiting for I/O. These will
 * either complete by throwing an exception, or, if the VersionOfTheWorld
 * has been incremented, will release there IOCB and silently exit.This is
 * an automatic feature built in to the ASYNC_enableGarbageCollection
 * and ASYNC_disableGarbageCollection macros.
 *
 */

int VersionOfTheWorld = 0;

void InitalizeAsynchronousIO(void) {
    if (VersionOfTheWorld++ == 0) {
        int i;
        for (i = 0 ; i < ASYNC_IOCB_COUNT ; i++) {
            ASYNCIOCB *aiocb = &IocbRoots[i];
            FreeAsyncIOCB(aiocb);
        }
    } else {
        while (ActiveAsyncOperations() > 0) {
            Yield_md();
        }
    }
}

/*=========================================================================
 * FUNCTION:      RequestGarbageCollection()
 * TYPE:          Public routine for asynchronous native methods
 * OVERVIEW:      Try and find GC unsafe code
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

#if EXCESSIVE_GARBAGE_COLLECTION
bool_t veryExcessiveGCrequested = FALSE;

void RequestGarbageCollection() {
    veryExcessiveGCrequested = TRUE;
}
#endif

