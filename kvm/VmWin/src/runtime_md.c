/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * KVM
 *=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Windows-specific functions needed by the virtual machine
 * FILE:      runtime_md.c
 * AUTHOR:    Frank Yellin, Antero Taivalsaari
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>
#include <async.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>

/*=========================================================================
 * Functions
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      alertUser()
 * TYPE:          error handling operation
 * OVERVIEW:      Show an alert dialog to the user and wait for
 *                confirmation before continuing execution.
 * INTERFACE:
 *   parameters:  message string
 *   returns:     <nothing>
 *=======================================================================*/

void AlertUser(const char* message)
{
    fprintf(stderr, "ALERT: %s\n", message);
}

/*=========================================================================
 * FUNCTION:      allocateHeap()
 * TYPE:          allocates memory
 * OVERVIEW:      Show an alert dialog to the user and wait for
 *                confirmation before continuing execution.
 * INTERFACE:
 *   parameters:  *sizeptr:  INPUT:   Pointer to size of heap to allocate
 *                           OUTPUT:  Pointer to actually size of heap
 *                *realresultptr: Returns pointer to actual pointer than
 *                           was allocated, before any adjustments for
 *                           memory alignment.  This is the value that
 *                           must be passed to "free()"
 *
 *  returns:      pointer to aligned memory.
 *
 *  Note that "sizeptr" reflects the size of the "aligned" memory, and
 *  note the actual size of the memory that has been allocated.
 *=======================================================================*/

cell *allocateHeap(long *sizeptr, void **realresultptr) {
        void *space = malloc(*sizeptr + sizeof(cell) - 1);
        *realresultptr = space;
        return (void *) ((((long)space) + (sizeof(cell) - 1)) & ~(sizeof(cell) - 1));
}

/* Virtual memory allocation and protection operations. */
/* Used for testing the correctness of the garbage collector */

int getpagesize() {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return sysInfo.dwPageSize;
}

void*
allocateVirtualMemory_md(long size) {
    return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
}

void
freeVirtualMemory_md(void *address, long size) {
    VirtualFree(address, size, MEM_RELEASE);
}

void
protectVirtualMemory_md(void *address, long size, int protection)
{
    long o;                     /* old protection value */
    int flag;
    switch (protection) {
        case PVM_NoAccess:  flag = PAGE_NOACCESS; break;
        case PVM_ReadOnly:  flag = PAGE_READONLY; break;
        case PVM_ReadWrite: flag = PAGE_READWRITE; break;
        default:            fatalError("Bad argument");
    }
    VirtualProtect(address, size, flag, &o);
}

/*=========================================================================
 * FUNCTION:      CurrentTime_md
 * TYPE:          Public link routine
 * OVERVIEW:      Get the system time in milliseconds since 1970
 * INTERFACE:
 *   parameters:  none
 *   returns:     time
 *=======================================================================*/

ulong64
CurrentTime_md(void) {
    return sysTimeMillis();
}

/*=========================================================================
 * FUNCTION:      InitializeFloatingPoint
 * TYPE:          initialization
 * OVERVIEW:      Called at startup to setup the FPU.
 * 
 * INTERFACE:
 *   parameters:  none
 *   returns:     none
 *=======================================================================*/
void InitializeFloatingPoint() {
}

/*=========================================================================
 * FUNCTION:      InitializeNativeCode
 * TYPE:          initialization
 * OVERVIEW:      Called at start up to perform machine-specific
 *                initialization.
 * INTERFACE:
 *   parameters:  none
 *   returns:     none
 *=======================================================================*/

void InitializeNativeCode() {
    extern void runtime2_md_init(void);
    runtime2_md_init();
}

/*=========================================================================
 * FUNCTION:      nativeFinalization
 * TYPE:          initialization
 * OVERVIEW:      Called at start up to perform machine-specific
 *                finalization.
 * INTERFACE:
 *   parameters:  none
 *   returns:     none
 *=======================================================================*/

void FinalizeNativeCode() {
}

/*=========================================================================
 * Asynchronous (non-blocking) I/O Routines
 *=======================================================================*/

#if ASYNCHRONOUS_NATIVE_FUNCTIONS

/*
 * Parameters to processAcyncThread
 */
static ASYNCIOCB *nextIocb;
static void (*nextRoutine)(ASYNCIOCB *);

/*
 * Routine called from runtime2_md.c when a thread is unblocked
 */
void processAcyncThread(void) {

    /*
     * Pick up parameters
     */
    ASYNCIOCB *iocb          = nextIocb;
    void (*rtn)(ASYNCIOCB *) = nextRoutine;

    /*
     * Clear input queue
     */
    nextRoutine = 0;
    nextIocb    = 0;

    /*
     * Sanity check
     */
    if (iocb == 0 || rtn == 0) {
        fatalError("processAcyncThread problem");
    }

    /*
     * Execute async function
     */
    (rtn)(iocb);
}

/*=========================================================================
 * FUNCTION:      CallAsyncNativeFunction_md()
 * TYPE:          Public link routine for asynchronous native methods
 * OVERVIEW:      Call an asynchronous native method
 * INTERFACE:
 *   parameters:  thread pointer, native function pointer
 *   returns:     <nothing>
 *
 * Note: This is just an example implementation for demonstration purposes.
 *       and does not actually work very well because eventually the WIN32
 *       program seems to run out of thread resources and no new threads
 *       seem to run. Typically no error code is ever returned!
 *=======================================================================*/

void CallAsyncNativeFunction_md(ASYNCIOCB *iocb, void (*afp)(ASYNCIOCB *)) {

    /*
     * Sanity check
     */
    if (nextRoutine != 0 || nextIocb != 0 || iocb == 0 || afp == 0) {
        fatalError("CallAsyncNativeFunction_md problem");
    }

    /*
     * Save the thread parameters
     */
    nextRoutine = afp;
    nextIocb    = iocb;

    /*
     * Release a thread. This function will cause
     * processAcyncThread to be called
     */
    releaseAsyncThread();

    /*
     * Wait until it is running and has taken the queued request
     */
    while(nextIocb != 0) {
        Yield_md();
    }
}

#endif /* ASYNCHRONOUS_NATIVE_FUNCTIONS */
