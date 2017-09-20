/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Memory management
 * FILE:      garbage.c
 * OVERVIEW:  Memory manager/garbage collector for the KVM.
 * AUTHOR:    Antero Taivalsaari, Sun Labs, 1998
 *            Edited by Doug Simon 11/1998
 *            Frank Yellin (exact collection, compaction)
 *            Compaction based on code written by Matt Seidl
 *=======================================================================*/

/*=========================================================================
 * For detailed explanation of the memory system, see Garbage.h
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>

/*=========================================================================
 * Variables
 *=======================================================================*/

int TemporaryRootsLength;
int GlobalRootsLength;
int gcInProgress;

union cellOrPointer TemporaryRoots[MAXIMUM_TEMPORARY_ROOTS];
union cellOrPointer GlobalRoots[MAXIMUM_GLOBAL_ROOTS];
POINTERLIST         CleanupRoots;

/*=========================================================================
 * Functions
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      garbageCollect
 * TYPE:          public garbage collection function
 * OVERVIEW:      Perform mark-and-sweep garbage collection.
 * INTERFACE:
 *   parameters:  int moreMemory: the amount by which the heap
 *                size should be grown during garbage collection
 *                (this feature is not supported in the mark-and-sweep
 *                collector).
 *   returns:     <nothing>
 *=======================================================================*/

void
garbageCollect(int moreMemory)
{
#if INCLUDEDEBUGCODE
    int beforeCollection = 0;
    int afterCollection = 0;
#endif

    if (gcInProgress != 0) {
        /* 
         * Circular invocation of GC should not be allowed
         */
        fatalVMError(KVM_MSG_CIRCULAR_GC_INVOCATION);
    }

    gcInProgress++;

    RundownAsynchronousFunctions();

    if (ENABLEPROFILING && INCLUDEDEBUGCODE) {
        checkHeap();
    }
#if INCLUDEDEBUGCODE
    if ((tracegarbagecollection || tracegarbagecollectionverbose)
        && !TERSE_MESSAGES) {
            Log->startGC();
    }
#endif

#if INCLUDEDEBUGCODE
    if (ENABLEPROFILING || tracegarbagecollection
        || tracegarbagecollectionverbose) {

        beforeCollection = memoryFree();
    }
#endif

    MonitorCache = NULL;        /* Clear any temporary monitors */

    /* Store virtual machine registers of the currently active thread before
     * garbage collection (must be  done to enable execution stack scanning).
    */
    if (CurrentThread) {
        storeExecutionEnvironment(CurrentThread);
    }

    garbageCollectForReal(moreMemory);

    if (CurrentThread) {
        loadExecutionEnvironment(CurrentThread);
    }

#if INCLUDEDEBUGCODE
    if (ENABLEPROFILING || tracegarbagecollection
        || tracegarbagecollectionverbose) {

        afterCollection = memoryFree();

#if ENABLEPROFILING
        GarbageCollectionCounter += 1;
        DynamicDeallocationCounter += (afterCollection - beforeCollection);
#endif

        if (tracegarbagecollection || tracegarbagecollectionverbose) {

            Log->endGC(afterCollection - beforeCollection, afterCollection, getHeapSize());
        }
    }
#endif /* INCLUDEDEBUGCODE */

    RestartAsynchronousFunctions();
    /*
     * Reset to indicate end of garbage collection
     */
    gcInProgress = 0;
}

#if INCLUDEDEBUGCODE
void verifyTemporaryRootSpace(int size)
{
    if (TemporaryRootsLength + size > MAXIMUM_TEMPORARY_ROOTS) {
        fatalError(KVM_MSG_TEMPORARY_ROOT_OVERFLOW);
    }
}
#endif /* INCLUDEDEBUGCODE */

void
makeGlobalRoot(cell** object)
{
    int index = GlobalRootsLength;
    if (index >= MAXIMUM_GLOBAL_ROOTS) {
        fatalError(KVM_MSG_GLOBAL_ROOT_OVERFLOW);
    }
    GlobalRoots[index].cellpp = object;
    GlobalRootsLength = index + 1;
}

/*=========================================================================
 * Memory allocation operations
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      mallocObject, callocObject
 * TYPE:          public memory allocation operation
 * OVERVIEW:      Allocate a contiguous area of n cells in the dynamic heap.
 * INTERFACE:
 *   parameters:  size: the requested object size in cells,
 *                type: garbage collection type of the object
 *   returns:     pointer to newly allocated area.
 *                Raises an exception if space cannot be allocated.
 *
 * COMMENTS:      callocObject zeros the space before returning it.
 *=======================================================================*/

cell* mallocObject(long size, GCT_ObjectType type)
/*  Remember: size is given in CELLs rather than bytes */
{
    cell* result = mallocHeapObject(size, type);
    if (result == NULL) {
        THROW(OutOfMemoryObject);
    }

    return result;
}

cell* callocObject(long size, GCT_ObjectType type)
{
    cell* result = mallocHeapObject(size, type);
    if (result == NULL) {
        THROW(OutOfMemoryObject);
    }

    /* Initialize the area to zero */
    memset(result, 0, size << log2CELL);

    return result;
}

/*=========================================================================
 * FUNCTION:      mallocBytes
 * TYPE:          public memory allocation operation
 * OVERVIEW:      Allocate a contiguous area of n bytes in the dynamic heap.
 * INTERFACE:
 *   parameters:  size: the requested object size in bytes
 *   returns:     pointer to newly allocated area, or
 *                NIL is allocation fails.
 * COMMENTS:      mallocBytes allocates areas that are treated as
 *                as byte data only. Any possible heap references
 *                within such areas are ignored during garbage
 *                collection.
 *
 *                The actual size of the data area is rounded
 *                up to the next four-byte aligned memory address,
 *                so the area may contain extra bytes.
 *=======================================================================*/

char* mallocBytes(long size)
{
    /*  The size is given in bytes, so it must be */
    /*  rounded up to next long word boundary */
    /*  and converted to CELLs */
    cell* dataArea = mallocObject((size + CELL - 1) >> log2CELL,
                                  GCT_NOPOINTERS);
    return (char*)dataArea;
}

/*=========================================================================
 * FUNCTION:      printContents, getObjectTypeStr
 * TYPE:          public debugging operation
 * OVERVIEW:      Print the contents of the heap. Intended for
 *                debugging and heap consistency checking.
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

#if INCLUDEDEBUGCODE

static char*
getObjectTypeStr(GCT_ObjectType type) {
    static char* typeNames[] = GCT_TYPENAMES;
    if ((type >= 0) && (type <= GCT_LASTVALIDTAG)) {
        return typeNames[type];
    } else {
        return "<unknown>";
    }
}

void
printObject(cell *object) {
    cell *header = object - HEADERSIZE;
    char buffer[256];
    GCT_ObjectType gctype = getObjectType(object);

    switch(gctype) {
        case GCT_INSTANCE: {
            /*  The object is a Java object instance.  Mark pointer fields */
            INSTANCE thisObject = (INSTANCE)object;
            INSTANCE_CLASS thisClass = thisObject->ofClass;
            if (thisClass != NULL) {
                getClassName_inBuffer((CLASS)thisClass, buffer);
                Log->heapObject((long)object, getObjectTypeStr(gctype),
                                 getObjectSize(object),
                                 (*header & MARKBIT),
                                 buffer, 0);
                break;
            }
            /* Fall Through */
        }

        default:
            Log->heapObject((long)object, getObjectTypeStr(gctype),
                             getObjectSize(object),
                             (*header & MARKBIT),
                             NULL, 0);
    }
}

#endif /* INCLUDEDEBUGCODE */

/*=========================================================================
 * Auxiliary memory management operations (both static and dynamic heap)
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      getObjectSize()
 * TYPE:          public helper function
 * OVERVIEW:      Returns the size of the given heap object (in CELLs).
 * INTERFACE:
 *   parameters:  an object pointer
 *   returns:     the size of the object in cells (excluding the header).
 *=======================================================================*/

unsigned long getObjectSize(cell* object)
{
    /*  Calculate the address of the object header */
    cell* header = object-HEADERSIZE;

    /*  Get the size of object by shifting the type  */
    /*  information away from the first header word */
    return ((unsigned long)*header) >> TYPEBITS;
}

/*=========================================================================
 * FUNCTION:      getObjectType()
 * TYPE:          public helper function
 * OVERVIEW:      Returns the garbage collection type (GCT_*)
 *                of the given heap object.
 * INTERFACE:
 *   parameters:  an object pointer
 *   returns:     GC type
 *=======================================================================*/

GCT_ObjectType getObjectType(cell* object)
{
    /*  Calculate the address of the object header */
    cell* header = object-HEADERSIZE;

    /*  Get the type of object by masking away the */
    /*  size information and the flags */
    return TYPE(*header);
}

/*=========================================================================
 * Global garbage collection operations
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      InitializeGlobals()
 * TYPE:          public global operation
 * OVERVIEW:      Initialize all the globals variables. This ensures that
 *                they have correct values in each invocation of the VM.
 *                ** DON'T rely on static initializers, because they 
 *                will not work if you launch the VM again inside the 
 *                same native process!! ** 
 *                Any globals (not initialized elsewhere) added to the 
 *                KVM should be initialized here.
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

void InitializeGlobals(void)
{
    loadedReflectively = FALSE;
}

/*=========================================================================
 * FUNCTION:      InitializeMemoryManagement()
 * TYPE:          public global operation
 * OVERVIEW:      Initialize the system memory heaps.
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

void InitializeMemoryManagement(void)
{
    int index;
    gcInProgress = 0;
    InitializeHeap();

    index = 0;
    GlobalRoots[index++].cellpp = (cell **)&AllThreads;
    GlobalRoots[index++].cellpp = (cell **)&CleanupRoots;
    GlobalRootsLength = index;
    TemporaryRootsLength = 0;

    CleanupRoots =
          (POINTERLIST)callocObject(SIZEOF_POINTERLIST(CLEANUP_ROOT_SIZE),
                                    GCT_POINTERLIST);
}

/*=========================================================================
 * FUNCTION:      FinalizeMemoryManagement()
 * TYPE:          public global operation
 * OVERVIEW:      Finalize the system memory heaps. As a side-effect,
 *                this routine also runs the native finalizers that
 *                may have been registered earlier (by function
 *                'registerCleanup').
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

void FinalizeMemoryManagement(void) {
    int i;
    cellOrPointer *ptr, *endPtr;

    /* When memory finalization is being performed */
    /* the VM no longer has a CurrentThread.       */
    /* We need to fake it in order to support the  */
    /* code below. */
    if (CurrentThread == NULL) {
        CurrentThread = MainThread;
    }

    /* Check each known root to see if there are any instances
     * to callback this callback (cb)
     */
    for (i = CleanupRoots->length - 1; i >= 0; i--) {
        WEAKPOINTERLIST list = (WEAKPOINTERLIST)CleanupRoots->data[i].cellp;
        if (list != NULL) {
            void (*finalizer)(INSTANCE_HANDLE) = list->finalizer;
            if (finalizer != NULL) {
                ptr = &list->data[0];
                endPtr = ptr + list->length;
                for ( ; ptr < endPtr; ptr++) {
                    INSTANCE object = (INSTANCE)(ptr->cellp);
                    if (object != NULL) {
                        /* In KVM, making the 'this' pointer available   */ 
                        /* in native finalizers is rather painful.       */
                        /* KVM doesn't create stack frames for native    */ 
                        /* methods, and therefore the stack may not      */
                        /* contain anything to which 'nativeLp' could    */ 
                        /* point to create access to the 'this' pointer. */ 
                        /* Therefore, it is necessary to set 'nativeLp'  */
                        /* to point to a 'fake' location each time we    */
                        /* need to call a native finalizer.              */
                        CurrentThread->nativeLp = (cell *)&object;

                        /* In KVM 1.0.4 and 1.1, we pass the object */
                        /* to the finalizer as a handle. This makes */
                        /* it possible to define native finalizers  */
                        /* as KNI functions that take a 'jobject'   */
                        /* parameter. */
                        finalizer((INSTANCE_HANDLE)&object);
                    }
                }
                /* Reset 'nativeLp' after calling all finalizers */ 
                CurrentThread->nativeLp = NULL;
            }
        }
    }

    ptr = &GlobalRoots[0];
    endPtr = ptr + GlobalRootsLength;

    for ( ; ptr < endPtr; ptr++) {
        *(ptr->cellpp) = NULL;
    }

    FinalizeHeap();

    if (USESTATIC) {
        FinalizeStaticMemory();
    }
}

/*=========================================================================
 * FUNCTION:      registerCleanup()
 * TYPE:          public global operation
 * OVERVIEW:      This function is new in KVM 1.0.3. It allows
 *                a person doing a KVM port to register special
 *                cleanup routines that get called automatically
 *                by the KVM upon exiting the VM.
 *
 *                Register a cleanup function for an instance.
 *                The roots of the already registered functions
 *                are in CleanupRoots.  Each reference is to a
 *                weak/final object that contains the function
 *                pointer and a list of instances.
 * INTERFACE:
 *   parameters:
 *                instance - object to be registered
 *                function - to be called with the instance pointer
 *                when the object is freed.
 *   returns:     <nothing>
 *
 * NOTE:          In KVM 1.0.4 and 1.1, the types 'INSTANCE_HANDLE' and
 *                the KNI type 'jobject' are identical.  Therefore, it
 *                is acceptable to use a jobject in these cleanup
 *                routines whenever an INSTANCE_HANDLE is required.
 *=======================================================================*/

void registerCleanup(INSTANCE_HANDLE instanceH, CleanupCallback cb) {
    int i;
    WEAKPOINTERLIST list = NULL;
    cell  **ptr, **endptr;

    /* Check each known root to see if it is for this callback (cb) */
    for (i = CleanupRoots->length - 1; i >= 0; i--) {
        list = (WEAKPOINTERLIST)CleanupRoots->data[i].charp;
        if (list->finalizer == cb) {
            break;
        }
    }
    if (i < 0) {
        int size;
        /* Didn't find an array for this function, allocate a new one */
        i = CleanupRoots->length;
        if (i >= CLEANUP_ROOT_SIZE) {
            /* TBD: expand roots if not enough */
            fatalError(KVM_MSG_ERROR_TOO_MANY_CLEANUP_REGISTRATIONS);
        }
        size = SIZEOF_WEAKPOINTERLIST(CLEANUP_ARRAY_SIZE);
        list = (WEAKPOINTERLIST)callocObject(size, GCT_WEAKPOINTERLIST);
        list->length = CLEANUP_ARRAY_SIZE;
        list->finalizer = cb;
        list->data[CLEANUP_ARRAY_SIZE - 1].cellp = (cell *)unhand(instanceH);

        /* Insert the function pointer into this new array */
        CleanupRoots->data[i].cellp = (cell *)list;
        CleanupRoots->length = i + 1;
        return;
    }

    /* We want to insert "instance" into the i-th element of CleanupRoots,
     * which happens to be "list".
     */

    /* Skip over 0th entry used by GC, 1st is function pointer */
    ptr = &list->data[0].cellp;
    endptr = ptr + list->length;

    for ( ; ptr < endptr; ptr++) {
        if (*ptr == NULL) {
            *ptr = (cell *)unhand(instanceH);
            if (EXCESSIVE_GARBAGE_COLLECTION) { 
                /* Make sure people realize that this function really
                 * can cause an allocation to happen */
                garbageCollect(0);
            }
            return;
        }
    }

    /* We did not find an empty list.  We need to make the list larger and
     * copy the old one into the new one
     */
    {
        int oldLength = list->length;
        int newLength = oldLength + CLEANUP_ARRAY_GROW;
        WEAKPOINTERLIST newList =
            (WEAKPOINTERLIST)callocObject(SIZEOF_WEAKPOINTERLIST(newLength),
                                          GCT_WEAKPOINTERLIST);
        newList->length = newLength;
        newList->finalizer = cb;
        list = (WEAKPOINTERLIST)CleanupRoots->data[i].cellp; /*may have moved*/
        CleanupRoots->data[i].cellp = (cell *)newList;

        /* Update original elements of list, and insert the new element
         * at the end.
         */
        memcpy(newList->data, list->data, oldLength << log2CELL);
        newList->data[newLength - 1].cellp = (cell *)unhand(instanceH);
    }
}

