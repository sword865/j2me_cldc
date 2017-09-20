/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * KVM
 *=========================================================================
 * SYSTEM:    KVM
 * FILE:      machine_md.h (for Windows CE)
 * OVERVIEW:  This file is included in every compilation.  It contains
 *            definitions that are specific to the Windows port of KVM.
 * AUTHOR:    Frank Yellin
 *            Ioi Lam
 *            Richard Berlin
 *            Kinsley Wong
 * NOTE:      This file overrides many of the default compilation
 *            flags and macros defined in VmCommon/h/main.h.
 *=======================================================================*/

/*=========================================================================
 * Platform definition
 *=======================================================================*/

#define WINDOWS 1
#define POCKETPC 1

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <setjmp.h>

#include <windef.h> 

/*=========================================================================
 * Platform-specific datatype definitions
 *=======================================================================*/

typedef long             Long;
typedef char             Boolean;

typedef __int64          long64;  /* 64-bit signed integer type */
typedef unsigned __int64 ulong64; /* 64-bit unsigned integer type */

typedef __int64          jlong;   /* Added for KNI */

#ifndef Calendar_md
unsigned long *Calendar_md();
#endif

/*=========================================================================
 * Compilation flags and macros that override values defined in main.h
 *=======================================================================*/

/* This is a little-endian target platform */
#define LITTLE_ENDIAN 1

/* Make the VM run a little faster (can afford the extra space) */
#define ENABLEFASTBYTECODES 1

/* Use asynchronous native functions on Windows */
#define ASYNCHRONOUS_NATIVE_FUNCTIONS 1

/* This port uses the environment classpath */
#define USES_CLASSPATH      1
#define JAR_FILES_USE_STDIO 1
#define PATH_SEPARATOR      ';'

#define SLEEP_FOR(delta) Sleep((long)delta)

/* If we are using the assembly interpreter, then Java-level  */
/* debugging support is not available.  Also, we must use      */
/* the SPLITINFREQUENTBYTECODES mode to compile the interpreter */
#if ALTERNATIVE_FAST_INTERPRETER

#define ENABLE_JAVA_DEBUGGER 0
#define SPLITINFREQUENTBYTECODES 1

#endif /* ALTERNATIVE_FAST_INTERPRETER */

/*=========================================================================
 * Platform-specific macros and function prototypes
 *=======================================================================*/

#define InitializeFloatingPoint()
#define InitializeVM()
#define FinalizeVM()

#define freeHeap(x) free(x)
#define RandomNumber_md() rand()

ulong64 sysTimeMillis(void);

/*
 * Thread processing routines
 */
void releaseAsyncThread(void);
void processAcyncThread(void);
void Yield_md(void);

/*=========================================================================
 * The following are used in several different places, and its worthwhile
 * to define them just once
 *=======================================================================*/

void* allocateVirtualMemory_md(long size);
void  freeVirtualMemory_md(void *address, long size);
enum { PVM_NoAccess, PVM_ReadOnly, PVM_ReadWrite };
void  protectVirtualMemory_md(void *address, long size, int protection);
int  getpagesize();

/*=========================================================================
 * FUNCTION:      The stub of GetAndStoreNextKVMEvent
 * TYPE:          event handler
 * OVERVIEW:      Wait for an external event to occur, and store
 *                the event to the KVM event queue.
 *                On a real device, the implementation of this
 *                function should try to conserve battery if 
 *                the routine is called with the 'forever'
 *                flag turned on, or with a 'waitUntil' parameter
 *                larger than zero.
 * INTERFACE
 *   parameters:  forever: a boolean flag that tells whether the
 *                KVM has anything to do or not.  When the KVM calls
 *                this routine with the forever flag turned on, the
 *                VM has no threads to run and this function can call
 *                machine-specific battery conservation/hibernation
 *                facilities.  If the forever flag is off, the function
 *                should try to return immediately, or at least after
 *                'waitUntil' milliseconds, since there are other threads
 *                that need to continue running.
 *                waitUntil: the amount of milliseconds this routine
 *                should wait in case the 'forever' flag is off.
 *   returns:     nothing directly, but the event should be stored
 *                in the event queue of the KVM (see events.c and
 *                the porting guide for details).
 *=======================================================================*/

#define GetAndStoreNextKVMEvent(x,y)
#define _NOT_IMPLEMENTED_GetAndStoreNextKVMEvent
