/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 */

/*=========================================================================
 * KVM 
 *=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Profiling
 * FILE:      profiling.h
 * OVERVIEW:  The variables and operations defined in this file 
 *            execution allow the of the execution of the virtual
 *            machine to be profiled at runtime.
 * AUTHOR:    Antero Taivalsaari, Frank Yellin
 * NOTE:      The code in this file was originally in
 *            interpret.h, and moved to a separate file later.
 *=======================================================================*/

/*=========================================================================
 * Profiling variables
 *=======================================================================*/

#if ENABLEPROFILING

extern int InstructionCounter;       /* Number of bytecodes executed */
extern int ThreadSwitchCounter;      /* Number of thread switches */

extern int DynamicObjectCounter;     /* Number of dynamic objects allocated */
extern int DynamicAllocationCounter; /* Bytes of dynamic memory allocated */
extern int DynamicDeallocationCounter; /* Bytes of dynamic memory deallocated */
extern int GarbageCollectionCounter; /* Number of garbage collections done */
extern int TotalGCDeferrals;         /* Total number of GC objects deferred */
extern int MaximumGCDeferrals;       /* Maximum number of GC objects deferred */
extern int GarbageCollectionRescans; /* Number of extra scans of GC heap */

#if ENABLEFASTBYTECODES
extern int InlineCacheHitCounter;    /* Number of inline cache hits */
extern int InlineCacheMissCounter;   /* Number of inline cache misses */
extern int MaxStackCounter;          /* Maximum amount of stack space needed */
#endif

#if USESTATIC
extern int StaticObjectCounter;      /* Number of static objects allocated */
extern int StaticAllocationCounter;  /* Bytes of static memory allocated */
#endif

/*=========================================================================
 * Profiling operations
 *=======================================================================*/

void InitializeProfiling(void);
void printProfileInfo(void);

#else 

#define InitializeProfiling()
#define printProfileInfo()

#endif /* ENABLEPROFILING */

