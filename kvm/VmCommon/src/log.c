/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Profiling
 * FILE:      log.c
 * OVERVIEW:  Operations for gathering and printing out diagnostic
 *            virtual machine execution information at runtime.
 * AUTHOR:    Daniel Blaukopf, based on code by Antero Taivalsaari
 *            and Doug Simon
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>

/*=========================================================================
 * Functions
 *=======================================================================*/

/*=========================================================================
 * NOTE:
 * Documentation for each of the functions below is provided in log.h.
 *=======================================================================*/

static void 
log_uncaughtException(THROWABLE_INSTANCE exception) 
{
    /* We are about to exit the VM, we can't malloc from the heap
     * since we may have gotten here via an outofmemory exception
     * Hence we use the native 'malloc' to get a buffer.
     */
    CLASS clazz = (CLASS)exception->ofClass;
    UString UPackageName = clazz->packageName;
    UString UBaseName    = clazz->baseName;
    int     baseLength = UBaseName->length;
    int     packageLength = UPackageName == 0 ? 0 : UPackageName->length;
    char *result = malloc(baseLength + packageLength + 5);

    if (result != NULL) {
        getClassName_inBuffer(clazz, result);
    } else {
        /* ugly, but what can we do, there's no memory */
        result = UBaseName->string;
    }
    fprintf(stderr, "Uncaught exception %s", result);
    if (exception->message != NULL) {
        fprintf(stderr,": %s", getStringContents(exception->message));
    }
    fprintf(stderr, ".\n");
}

/* The following functions are included only when really needed */
#if INCLUDEDEBUGCODE

static void log_enterMethod(METHOD method, const char *type) 
{ 
    ASSERTING_NO_ALLOCATION
        char className[256];
        char *methodName = methodName(method);
        getClassName_inBuffer((CLASS)method->ofClass, className);
        if (TERSE_MESSAGES) {
            fprintf(stdout, "=> %s.%s\n", className, methodName);
        } else {
            char methodSignature[256];
            unsigned short key = method->nameTypeKey.nt.typeKey;
            change_Key_to_MethodSignature_inBuffer(key, methodSignature);
            fprintf(stdout, "Invoking %s method '%s::%s%s'\n", 
                    type, className, methodName, methodSignature);
        }
    END_ASSERTING_NO_ALLOCATION
}

/*=========================================================================
 * FUNCTION:      TraceMethodExit()
 * OVERVIEW:      Trace a method exit
 * INTERFACE:
 *   parameters:  METHOD method
 *   returns:     <nothing>
 *=======================================================================*/

static void log_exitMethod(METHOD method) 
{
    ASSERTING_NO_ALLOCATION
        char className[256];
        char *methodName = methodName(method);
        getClassName_inBuffer((CLASS)method->ofClass, className);
        if (TERSE_MESSAGES) {
            fprintf(stdout, "<= %s.%s\n", className, methodName);
        } else { 
            fprintf(stdout, "Leaving %s::%s\n", className, methodName);
        }
    END_ASSERTING_NO_ALLOCATION
}

static void log_allocateObject(long pointer,
                               long cells,
                               int type,
                               long ID,
                               long memoryFree) {
    fprintf(stdout, "Allocated %ld bytes, address: %lx, type: %lx, free: %ld\n",
            cells*CELL, pointer, (long)type, memoryFree);
}

static void log_freeObject(long pointer,
                           INSTANCE_CLASS clazz,
                           long cells) {
  ASSERTING_NO_ALLOCATION
    fprintf(stdout, "Freed %ld bytes, address: %lx\n", cells*CELL, pointer);
  END_ASSERTING_NO_ALLOCATION
}

static void log_allocateHeap(long heapSize,
                             long heapBottom,
                             long heapTop) {
    fprintf(stdout, "Allocated a dynamic heap of %ld bytes\n", heapSize);
    fprintf(stdout, "Heap bottom: %lx, heap top: %lx\n",
            heapBottom, heapTop);
}

static void log_heapObject(long object,
                           const char *type,
                           long size,
                           int isMarked,
                           const char *className,
                           int isClass) {
    fprintf(stdout, "%lx: %s, size=%ld%s",
            object, type, size,
            isMarked ? ", MARKED" : "");
    if (className==NULL) {
        fprintf(stdout, "\n");
    } else if (isClass) {
        fprintf(stdout, ", class %s\n", className);
    } else {
        fprintf(stdout, ", instance of %s\n", className);
    }
}

static void log_heapSummary(int objectCount, int freeCount,
                            int liveByteCount, int freeByteCount,
                            int largestFreeObjectSize,
                            long heapBottom, long heapTop) {
    fprintf(stdout, "%d objects in heap (%d alive/%d free)\n",
            objectCount+freeCount, objectCount, freeCount);
    fprintf(stdout, "%d live bytes; %d bytes free (%d total)\n",
            liveByteCount, freeByteCount, liveByteCount+freeByteCount);
    fprintf(stdout, "Largest free object: %d bytes\n",
            largestFreeObjectSize);
    fprintf(stdout, "Heap bottom: %lx, heap top: %lx\n",
            heapBottom, heapTop);
}

static void log_horizontalRule(void) {
    fprintf(stdout, "\n====================\n");
}

#define log_startHeapScan log_horizontalRule
#define log_endHeapScan log_horizontalRule

static void log_heapWarning (long freeListCount,
                             long byteCount) {
    fprintf(stdout, "WARNING: %ld objects in the free list (%ld bytes)\n",
            freeListCount, byteCount);
}

static void log_startGC(void) {
    fprintf(stdout, "Garbage collecting...\n");
}

static void log_endGC (int collected, int memoryFree, int heapSize) {
    fprintf(stdout,
            "Collected %ld bytes of garbage (%ld/%ld bytes free)\n",
            collected, memoryFree, heapSize);
}

static void log_loadClass(const char *className) {
    fprintf(stdout, "Loading class '%s'\n", className);
}
    
static void 
log_throwException(THROWABLE_INSTANCE exception) {
    fprintf(stdout, "Exception: %s\n", \
            getClassName((CLASS)exception->ofClass));
}

#endif /* INCLUDEDEBUGCODE */

static const struct KVMLogInterface_ logImplementation = {
    fprintf
    ,log_uncaughtException
#if INCLUDEDEBUGCODE
    ,log_enterMethod
    ,log_exitMethod
    ,log_allocateObject
    ,log_freeObject
    ,log_allocateHeap
    ,log_heapObject
    ,log_heapSummary
    ,log_startHeapScan
    ,log_endHeapScan
    ,log_heapWarning
    ,log_loadClass
    ,log_startGC
    ,log_endGC
    ,log_throwException
#endif /* INCLUDEDEBUGCODE */
};

const KVMLogInterface Log = (KVMLogInterface) &logImplementation;

