/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * KVM
 *=========================================================================
 * SYSTEM:    KVM
 * FILE:      machine_md.h (for UNIX/Solaris/Linux)
 * OVERVIEW:  This file is included in every compilation.  It contains
 *            definitions that are specific to the Solaris/Linux ports 
 *            of KVM.
 * AUTHOR:    Frank Yellin
 *            Andreas Heilwagen, Kinsley Wong (Linux port)
 * NOTE:      This file overrides many of the default compilation 
 *            flags and macros defined in VmCommon/h/main.h.
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <setjmp.h>

#ifdef LINUX
#include <sys/time.h>
#include <fpu_control.h>
#endif

/*=========================================================================
 * Platform-specific datatype definitions
 *=======================================================================*/

typedef long long           long64;    /* 64-bit signed integer type */
typedef unsigned long long  ulong64;   /* 64-bit unsigned integer type */
typedef long long           jlong;     /* Added for KNI */

#ifndef Calendar_md
unsigned long * Calendar_md(void);
#endif

/*=========================================================================
 * Compilation flags and macros that override values defined in main.h
 *=======================================================================*/

/* By default we are assume that if we are not using a i386 machine the 
 * target platform is big-endian.
 * Solaris requires 8-byte alignment of longs and doubles */

#if !defined(i386) && !defined(__arm__)
#define BIG_ENDIAN 1
#define NEED_LONG_ALIGNMENT 1
#define NEED_DOUBLE_ALIGNMENT 1
#else
#undef BIG_ENDIAN
#undef LITTLE_ENDIAN
#undef NEED_LONG_ALIGNMENT 
#define LITTLE_ENDIAN 1
#define NEED_LONG_ALIGNMENT 0 
#endif

#if defined(i386)
/* This definition is used for compiling in */
/* the x86-specific strictfp operations */
#define PROCESSOR_ARCHITECTURE_X86 1
#endif

/* Make the VM run a little faster (can afford the extra space) */
#define ENABLEFASTBYTECODES 1

/* Override the sleep function defined in main.h */
#define SLEEP_FOR(delta)                                     \
    {                                                        \
           struct timeval timeout;                           \
           timeout.tv_sec = delta / 1000;                    \
           timeout.tv_usec = (delta % 1000) * 1000;          \
           select(0, NULL, NULL, NULL, &timeout);            \
        }

/*=========================================================================
 * Platform-specific function prototypes
 *=======================================================================*/

#define InitializeVM()
#define FinalizeVM()

#define freeHeap(heap) free(heap)
#define RandomNumber_md() rand()

void InitializeWindowSystem();
void FinalizeWindowSystem(void);

/*=========================================================================
 * The following are used in several different places, and its worthwhile
 * to define them just once
 *=======================================================================*/

void* allocateVirtualMemory_md(long size);
void  freeVirtualMemory_md(void *address, long size);

enum { PVM_NoAccess, PVM_ReadOnly, PVM_ReadWrite };
void  protectVirtualMemory_md(void *address, long size, int protection);

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
