/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Profiling
 * FILE:      log.h
 * OVERVIEW:  Definitions for gathering and printing out diagnostic
 *            virtual machine execution information at runtime.
 * AUTHOR:    Daniel Blaukopf, based on code by Antero Taivalsaari
 *            and Doug Simon
 *=======================================================================*/

/*=========================================================================
 * COMMENT: This file contains the declaration of a structure type, and
 * an instance of it. This instance contains pointers to functions that
 * are called for by other modules in the KVM in order to provide
 * diagnostic information.
 *=======================================================================*/

#ifndef __log_h
#define __log_h

/*=========================================================================
 * Logging makes use of the "fprintf" function, the first parameter of
 * which is expected to be of type "LOGFILEPTR". If LOGFILEPTR has not
 * already been defined for a platform (for example, the Palm source 
 * defines it in machine_md.h) then we assume that logging will be to
 * "stdout" using the C output routines. Thus, we define LOGFILEPTR
 * appropriately below.
 *=======================================================================*/

#ifndef LOGFILEPTR
#define LOGFILEPTR FILE*
#endif

/*=========================================================================
 * The following is the LogInterface structure, which contains pointers to
 * the functions to be called for each type of logged event.
 *=======================================================================*/

struct KVMLogInterface_ {

/*=========================================================================
* FUNCTION:      fprintf
* TYPE:          instrumentation operation
* OVERVIEW:      Record a text string. By default this function maps to
*                the fprintf() function declared in <stdio.h>.
*
* INTERFACE:
*   parameters:  as defined in <stdio.h>,
*                stream: the output stream (note: defined as a LOGFILEPTR)
*                format: the text and formatting information
*                ...: optional arguments
*   returns:     The number of characters written, or a negative value
*                if an error was encountered.
*=======================================================================*/

    int (*fprintf) (LOGFILEPTR stream, const char *format, ...);
    
/*=========================================================================
* FUNCTION:      uncaughtException
* TYPE:          instrumentation operation
* OVERVIEW:      Record the fact than an exception was thrown, but
*                no appropriate handler was found to handle it.
* 
* INTERFACE:
*   parameters:  exception: the exception object
*   returns:     <nothing>
*=======================================================================*/

    void (*uncaughtException)(THROWABLE_INSTANCE exception);

#if INCLUDEDEBUGCODE    

/*=========================================================================
* FUNCTION:      enterMethod
* TYPE:          instrumentation operation
* OVERVIEW:      Record the fact that a method has been called
*
* INTERFACE:
*   parameters:  type: any extra words describing this method
*                className: the name of the class containing the method
*                methodName: the name of the method
*                signature: the method's signature (e.g. (II)V)
*   returns:     <nothing>
*=======================================================================*/

    void (*enterMethod) (METHOD method, const char *type);

/*=========================================================================
* FUNCTION:      exitMethod
* TYPE:          instrumentation operation
* OVERVIEW:      Record the fact that a method has returned
*
* INTERFACE:
*   parameters:  className: the name of the class containing the method
*                methodName: the name of the method
*   returns:     <nothing>
*=======================================================================*/

    void (*exitMethod) (METHOD method);

/*=========================================================================
* FUNCTION:      allocateObject
* TYPE:          instrumentation operation
* OVERVIEW:      Record the fact that an object has been allocated
*
* INTERFACE:
*   parameters:  object: a pointer to the object, casted to a long
*                cells: the number of cells occupied by the object
*                type: the type of the object
*                ID: a long integer uniquely identifying the object
*                memoryFree: the amount of free memory after the
*                        allocation
*   returns:     <nothing>
*=======================================================================*/

    void (*allocateObject) (long object,
                            long cells,
                            int type,
                            long ID,
                            long memoryFree);

/*=========================================================================
* FUNCTION:      freeObject
* TYPE:          instrumentation operation
* OVERVIEW:      Record the fact that an object has been allocated
*
* INTERFACE:
*   parameters:  object: a pointer to the object, cast to a long
*                clazz: the class of the object
*                cells: the number of cells occupied by the object
*   returns:     <nothing>
*=======================================================================*/

    void (*freeObject) (long object,
                        INSTANCE_CLASS clazz,
                        long cells);

/*=========================================================================
* FUNCTION:      allocateHeap
* TYPE:          instrumentation operation
* OVERVIEW:      Record the fact that the heap has been initialized
*
* INTERFACE:
*   parameters:  heapSize: the size of the heap, in cells
*                heapBottom: the base address of the heap, casted
*                        to a long
*                heapTop: the address of the top of the heap,casted
*                        to a long
*   returns:     <nothing>
*=======================================================================*/

    void (*allocateHeap) (long heapSize,
                          long heapBottom,
                          long HeapTop);

/*=========================================================================
* FUNCTION:      heapObject
* TYPE:          instrumentation operation
* OVERVIEW:      Update data about a particular object on the heap
*
* INTERFACE:
*   parameters:  object: a pointer to the object, casted to a long
*                type: the type of the object
*                size: the size of the object, in bytes
*                isMarked: 1 is this object has been marked by the
*                        garbage collector, 0 otherwise
*                className:
*                        i) If this is a class object, then the name of
*                           the class it represents.
*                       ii) If this is an instance of a class, then the
*                           name of that class
*                      iii) If className is NULL, then no class
*                           information will be recorded
*                isClass: 1 if the object represents a class, 0 otherwise
*   returns:     <nothing>
*=======================================================================*/

    void (*heapObject) (long object,
                        const char *type,
                        long size,
                        int isMarked,
                        const char *className,
                        int isClass);

/*=========================================================================
* FUNCTION:      heapSummary
* TYPE:          instrumentation operation
* OVERVIEW:      Record data concerning the state of the heap
*
* INTERFACE:
*   parameters:  objectCount: the total number of objects in the heap
*                freeCount: the number of free objects in the heap
*                liveByteCount: the number of bytes of data in the heap
*                        that may not be garbage collected
*                freeByteCount: the number of bytes of data in the heap
*                        that may be garbage collected
*                largestFreeObjectSize: the size of the largest free
*                        object in the heap, in bytes
*                heapBottom: the base address of the heap, casted
*                        to a long
*                heapTop: the address of the top of the heap,casted
*                        to a long
*   returns:     <nothing>
*=======================================================================*/

    void (*heapSummary) (int objectCount,int freeCount,
                         int liveByteCount,int freeByteCount,
                         int largestFreeObjectSize,
                         long heapBottom, long heapTop);

/*=========================================================================
* FUNCTION:      startHeapScan
* TYPE:          instrumentation operation
* OVERVIEW:      Record the fact that a scan of the heap has begun
*
* INTERFACE:
*   parameters:  <none>
*   returns:     <nothing>
*=======================================================================*/

    void (*startHeapScan) (void);

/*=========================================================================
* FUNCTION:      endHeapScan
* TYPE:          instrumentation operation
* OVERVIEW:      Record the fact that a scan of the heap has ended
*
* INTERFACE:
*   parameters:  <none>
*   returns:     <nothing>
*=======================================================================*/

    void (*endHeapScan) (void);

/*=========================================================================
* FUNCTION:      heapWarning
* TYPE:          instrumentation operation
* OVERVIEW:      Record a warning if the free list does not match the
*                heap contents
*
* INTERFACE:
*   parameters:  freeListCount: number of objects that may be garbage
*                        collected
*                byteCount: number of bytes that may be garbage collected
*   returns:     <nothing>
*=======================================================================*/

    void (*heapWarning) (long freeListCount, long byteCount);

/*=========================================================================
* FUNCTION:      loadClass
* TYPE:          instrumentation operation
* OVERVIEW:      Record the fact that a class has been loaded
*
* INTERFACE:
*   parameters:  className: the name of the loaded class
*   returns:     <nothing>
*=======================================================================*/

    void (*loadClass) (const char *className);

/*=========================================================================
* FUNCTION:      startGC
* TYPE:          instrumentation operation
* OVERVIEW:      Record the fact that a garbage collection has been started 
*
* INTERFACE:
*   parameters:  <none>
*   returns:     <nothing>
*=======================================================================*/

    void (*startGC) (void);

/*=========================================================================
* FUNCTION:      endGC
* TYPE:          instrumentation operation
* OVERVIEW:      Record the fact that a garbage collection has been ended
*
* INTERFACE:
*   parameters:  collected: the amount of memory that has been garbage
*                        collected, in bytes
*                memoryFree: the amount of free memory after the garbage
*                        collection, in bytes
*                heapSize: the size of the heap, in bytes
*   returns:     <nothing>
*=======================================================================*/

    void (*endGC) (int collected, int memoryFree, int heapSize);

/*=========================================================================
* FUNCTION:      throwException
* TYPE:          instrumentation operation
* OVERVIEW:      Record the fact than an exception is being thrown.  This
*                function is called just before the KVM attempts to 
*                find a handler for this exception.
* INTERFACE:
*   parameters:  exception: the exception object
*   returns:     <nothing>
*=======================================================================*/

    void (*throwException)(THROWABLE_INSTANCE exception);

#endif /* INCLUDEDEBUGCODE */

}; /* end of struct KVMLogInterface_ */

typedef struct KVMLogInterface_ *KVMLogInterface;

extern const KVMLogInterface Log;

#endif /* __log_h */

