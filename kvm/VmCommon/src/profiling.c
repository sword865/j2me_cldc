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
 * FILE:      profiling.c
 * OVERVIEW:  The variables and operations defined in this file 
 *            execution allow the of the execution of the virtual
 *            machine to be profiled at runtime.
 * AUTHOR:    Antero Taivalsaari, Frank Yellin
 * NOTE:      The code in this file was originally in
 *            interpret.c, and moved to a separate file later.
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>

/*=========================================================================
 * Profiling variables
 *=======================================================================*/

#if ENABLEPROFILING

int InstructionCounter;         /* Number of bytecodes executed */
int ThreadSwitchCounter;        /* Number of thread switches */

int DynamicObjectCounter;       /* Number of dynamic objects allocated */
int DynamicAllocationCounter;   /* Bytes of dynamic memory allocated */
int DynamicDeallocationCounter; /* Bytes of dynamic memory deallocated */
int GarbageCollectionCounter;   /* Number of garbage collections done */
int TotalGCDeferrals;           /* Total number of GC objects deferred */
int MaximumGCDeferrals;         /* Maximum number of GC objects deferred */
int GarbageCollectionRescans;   /* Number of extra scans of GC heap */

#if ENABLEFASTBYTECODES
int InlineCacheHitCounter;      /* Number of inline cache hits */
int InlineCacheMissCounter;     /* Number of inline cache misses */
int MaxStackCounter;            /* Maximum amount of stack space needed */
#endif

#if USESTATIC
int StaticObjectCounter;        /* Number of static objects allocated */
int StaticAllocationCounter;    /* Bytes of static memory allocated */
#endif

/*=========================================================================
 * Profiling operations
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      InitializeProfiling
 * TYPE:          Profiling
 * OVERVIEW:      Reset all profiling counters to zero
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

void InitializeProfiling()
{ 
    /* Zero any profiling counters */
    InstructionCounter         = 0;
    ThreadSwitchCounter        = 0;
    DynamicObjectCounter       = 0;
    DynamicAllocationCounter   = 0;
    DynamicDeallocationCounter = 0;
    GarbageCollectionCounter   = 0;

    TotalGCDeferrals           = 0;
    MaximumGCDeferrals         = 0;
    GarbageCollectionRescans   = 0;

#if ENABLEFASTBYTECODES
    InlineCacheHitCounter      = 0;
    InlineCacheMissCounter     = 0;
    MaxStackCounter            = 0;
#endif

#if USESTATIC
    StaticObjectCounter        = 0;
    StaticAllocationCounter    = 0;
#endif
}

/*=========================================================================
 * FUNCTION:      printProfileInfo()
 * TYPE:          public debugging operation
 * OVERVIEW:      Print the summary of the contents of all the profiling 
 *                variables for debugging purposes.
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

void printProfileInfo()
{
    /* Note: the Palm wants ints printed with %ld, while every
     * other machine in the known universe prints them with %d.
     * To keep everyone happy, we use long's exclusively.
     */
    fprintf(stdout, "Execution completed.\n");
    fprintf(stdout, "%ld bytecodes executed\n", (long)InstructionCounter);
    fprintf(stdout, "%ld thread switches\n", (long)ThreadSwitchCounter);
    fprintf(stdout, "%ld classes in the system (including system classes)\n", 
            (long)(ClassTable->count));
    fprintf(stdout, "%ld dynamic objects allocated (%ld bytes)\n", 
            (long)DynamicObjectCounter, (long)DynamicAllocationCounter);
    fprintf(stdout, "%ld garbage collections ",
            (long)GarbageCollectionCounter);
    fprintf(stdout, "(%ld bytes collected)\n",
            (long)DynamicDeallocationCounter);

/* This info is too detailed for most users:
    fprintf(stdout, "%ld objects deferred in GC\n", (long)TotalGCDeferrals);
    fprintf(stdout, "%ld (maximum) objects deferred at any one time\n", 
            (long)MaximumGCDeferrals);
    fprintf(stdout, "%ld rescans of heap because of deferral overflow\n", 
            (long)GarbageCollectionRescans);
*/
}

#endif /* ENABLEPROFILING */

