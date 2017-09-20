/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Memory management
 * FILE:      collector.c
 * OVERVIEW:  Exact, compacting garbage collector.
 * AUTHOR:    Antero Taivalsaari, Sun Labs, 1998
 *            Edited by Doug Simon 11/1998 (made collection more precise)
 *            Frank Yellin (removed excessive recursion)
 *            Frank Yellin, Matt Seidl (exact, compacting GC)
 *            Weak reference support for CLDC 1.1, A. Taivalsaari 3/2002
 *=======================================================================*/

/*=========================================================================
 * For detailed explanation of the memory system, see Garbage.h
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>

/*=========================================================================
 * Definitions and declarations
 *=======================================================================*/

#define PTR_OFFSET(ptr, offset) ((void *)((char *)ptr + offset))
#define PTR_DELTA(ptr1, ptr2) ((char *)(ptr1) - (char *)ptr2)

#define CELL_AT_OFFSET(ptr, offset) (*(cell *)PTR_OFFSET(ptr, offset))

#if ENABLE_HEAP_COMPACTION
# define ISKEPT(n) ((n) & MARKBIT)
#else
# define ISKEPT(n) ((n) & (MARKBIT|STATICBIT))
#endif

typedef struct breakTableStruct {
    int length;                 /* in entries */
    struct breakTableEntryStruct *table;
} breakTableStruct;

typedef struct breakTableEntryStruct {
    cell *address;
    int offset;
} breakTableEntryStruct;

/*=========================================================================
 * Variables
 *=======================================================================*/

void* TheHeap;
long  VMHeapSize;               /* Heap size */

cell* AllHeapStart;             /* Heap bottom */
cell* CurrentHeap;              /* Same as AllHeapStart*/
cell* CurrentHeapEnd;           /* End of heap */
cell* AllHeapEnd;

WEAKPOINTERLIST WeakPointers;   /* List of weak pointers during GC */
WEAKREFERENCE   WeakReferences; /* List of weak refs during GC (CLDC 1.1) */

CHUNK FirstFreeChunk;

#if ENABLE_HEAP_COMPACTION
cell* PermanentSpaceFreePtr;
#endif

#define DEFERRED_OBJECT_TABLE_SIZE 40
static cell *deferredObjectTable[DEFERRED_OBJECT_TABLE_SIZE];
#define endDeferredObjectTable (deferredObjectTable + DEFERRED_OBJECT_TABLE_SIZE)
static cell **startDeferredObjects,  **endDeferredObjects;
static int deferredObjectCount;
static int deferredObjectTableOverflow;

/*=========================================================================
 * Static functions (private to this file)
 *=======================================================================*/

static void putDeferredObject(cell *c);
static cell* getDeferredObject(void);
static void initializeDeferredObjectTable(void);
static void markChildren(cell* object, cell* limit, int remainingDepth);
static void checkMonitorAndMark(OBJECT object);

static cell* allocateFreeChunk(long size);

static CHUNK sweepTheHeap(long *maximumFreeSizeP);

#if ENABLE_HEAP_COMPACTION
static cell* compactTheHeap(breakTableStruct *currentTable, CHUNK);

static breakTableEntryStruct*
slideObject(cell* deadSpace, cell *object, int objectSize, int extraSize,
            breakTableEntryStruct *table, int tableLength, int *lastRoll);

static void sortBreakTable(breakTableEntryStruct *, int length);
static void updateRootObjects(breakTableStruct *currentTable);
static void updateHeapObjects(breakTableStruct *currentTable, cell *endScan);
static void updatePointer(void *address, breakTableStruct *currentTable);
static void updateMonitor(OBJECT object, breakTableStruct *currentTable);
static void updateThreadAndStack(THREAD thread, breakTableStruct *currentTable);

void printBreakTable(breakTableEntryStruct *table, int length) ;

#endif /* ENABLE_HEAP_COMPACTION */

#if INCLUDEDEBUGCODE
static void checkValidHeapPointer(cell *number);
int NoAllocation = 0;
#else
#define checkValidHeapPointer(number)
#define NoAllocation 0
#endif

#if ASYNCHRONOUS_NATIVE_FUNCTIONS
#include <async.h>
extern ASYNCIOCB IocbRoots[];
#endif

static void markRootObjects(void);
static void markNonRootObjects(void);
static void markWeakPointerLists(void);
static void markWeakReferences(void);

static void markThreadStack(THREAD thisThread);

/*=========================================================================
 * Helper macros
 *=======================================================================*/

#define OBJECT_HEADER(object) ((cell *)(object))[-HEADERSIZE]

#define MARK_OBJECT(object) \
    if (inHeapSpaceFast(object)) { OBJECT_HEADER((object)) |= MARKBIT; }

#define MARK_OBJECT_IF_NON_NULL(object) \
    if (inHeapSpaceFast((object))) { OBJECT_HEADER((object)) |= MARKBIT; }

#define inHeapSpaceFast(ptr) \
     (((cell *)(ptr) >= heapSpace) && ((cell *)(ptr) < heapSpaceEnd))

/*=========================================================================
 * Heap initialization operations
 *=======================================================================*/

void InitializeHeap(void)
{
    VMHeapSize = RequestedHeapSize;
    AllHeapStart = allocateHeap(&VMHeapSize, &TheHeap);

    if (TheHeap == NIL) {
        fatalVMError(KVM_MSG_NOT_ENOUGH_MEMORY);
    }

    /* Initially, don't create any permanent space.  It'll grow as needed */
    CurrentHeap    = AllHeapStart;
    CurrentHeapEnd = PTR_OFFSET(AllHeapStart, VMHeapSize);

#if !CHUNKY_HEAP
    FirstFreeChunk = (CHUNK)CurrentHeap;
    FirstFreeChunk->size =
           (CurrentHeapEnd -CurrentHeap - HEADERSIZE) << TYPEBITS;
    FirstFreeChunk->next = NULL;
#endif

    /* Permanent space goes from CurrentHeapEnd to AllHeapEnd.  It currently
     * has zero size.
     */
#if ENABLE_HEAP_COMPACTION
    PermanentSpaceFreePtr = CurrentHeapEnd;
#endif
    AllHeapEnd            = CurrentHeapEnd;

#if INCLUDEDEBUGCODE
    if (tracememoryallocation) {
        Log->allocateHeap(VMHeapSize, (long)AllHeapStart, (long)AllHeapEnd);
    }
    NoAllocation = 0;
#endif
}

void FinalizeHeap(void)
{
    freeHeap(TheHeap);
}

/*=========================================================================
 * Memory allocation operations
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      mallocHeapObject()
 * TYPE:          public memory allocation operation
 * OVERVIEW:      Allocate a contiguous area of n cells in the dynamic heap.
 * INTERFACE:
 *   parameters:  size: the requested object size in cells,
 *                type: garbage collection type of the object
 *   returns:     pointer to newly allocated area, or
 *                NIL is allocation fails.
 * COMMENTS:      Allocated data area is not initialized, so it
 *                may contain invalid heap references. If you do
 *                not intend to initialize data area right away,
 *                you must use 'callocObject' instead (or otherwise
 *                the garbage collector will get confused next time)!
 *=======================================================================*/

cell* mallocHeapObject(long size, GCT_ObjectType type)
/* Remember: size is given in CELLs rather than bytes */
{
    cell* thisChunk;
    long realSize;

    if (size == 0) size = 1;
    realSize = size + HEADERSIZE;

#if INCLUDEDEBUGCODE
    if (NoAllocation > 0) {
        fatalError(KVM_MSG_CALLED_ALLOCATOR_WHEN_FORBIDDEN);
    }
#endif /* INCLUDEDEBUGCODE */

    if (EXCESSIVE_GARBAGE_COLLECTION) {
        garbageCollect(0);
    }

    thisChunk = allocateFreeChunk(realSize);
    if (thisChunk == NULL) {
        garbageCollect(realSize); /* So it knows what we need */
        thisChunk = allocateFreeChunk(realSize);
        if (thisChunk == NULL) {
            return NULL;
        }
    }

    /* Augment the object header with gc type information */
    /* NOTE: This operation does not set the size of the object!! */
    /* You must initialize the size elsewhere, or otherwise the */
    /* memory system will be corrupted! */
    *thisChunk |= (type << TYPE_SHIFT);

#if INCLUDEDEBUGCODE
    if (tracememoryallocation) {
        Log->allocateObject((long)thisChunk,
                            (long)size+HEADERSIZE,
                            (int)type,
                            (long)thisChunk, memoryFree());
    }
#endif /* INCLUDEDEBUGCODE */

#if ENABLEPROFILING
    DynamicObjectCounter     += 1;
    DynamicAllocationCounter += (size+HEADERSIZE)*CELL;
#endif /* ENABLEPROFILING */

    return thisChunk + HEADERSIZE;
}

static cell* allocateFreeChunk(long size)
{
    CHUNK thisChunk = FirstFreeChunk;
    CHUNK* nextChunkPtr = &FirstFreeChunk;
    cell* dataArea  = NIL;

    for (thisChunk = FirstFreeChunk, nextChunkPtr = &FirstFreeChunk;
         thisChunk != NULL;
         nextChunkPtr = &thisChunk->next, thisChunk = thisChunk->next) {

        /* Calculate how much bigger or smaller the */
        /* chunk is than the requested size */
        long overhead = SIZE(thisChunk->size) + HEADERSIZE - size;

        if (overhead > HEADERSIZE) {
            thisChunk->size = (overhead - HEADERSIZE) << TYPEBITS;
            dataArea = (cell *)thisChunk + overhead;
            *dataArea = (size - HEADERSIZE) << TYPEBITS;
            return dataArea;
        } else if (overhead >= 0) {
            /* There was an exact match or overhead is too small to be useful.
             * Remove this chunk from the free list, and if there is extra
             * space at the end of the chunk, it becomes wasted space for the
             * lifetime of the allocated object
             */
            *nextChunkPtr = thisChunk->next;
            dataArea = (cell *)thisChunk;
            /* Store the size of the object in the object header */
            *dataArea = (size + overhead - HEADERSIZE) << TYPEBITS;
            return dataArea;
        }
    }
    /* If we got here, there was no chunk with enough memory available */
    return NULL;
}

/*=========================================================================
 * FUNCTION:      callocPermanentObject()
 * TYPE:          public memory allocation operation
 * OVERVIEW:      Allocate a contiguous area of n cells in the permanent heap
 * INTERFACE:
 *   parameters:  size: the requested object size in cells,
 *   returns:     pointer to newly allocated area,
 *=======================================================================*/

cell*
callocPermanentObject(long size) {
    cell* result;
#if ENABLE_HEAP_COMPACTION
    result = PermanentSpaceFreePtr - size;
    PermanentSpaceFreePtr = result;
    if (result < CurrentHeapEnd) {
        /* We have to expand permanent memory */
        cell* newPermanentSpace = CurrentHeapEnd;
        do {
            newPermanentSpace = PTR_OFFSET(newPermanentSpace, -0x800);
        } while (newPermanentSpace > result);
        /* We need to expand permanent memory to include the memory between
         * newPermanentSpace and CurrentHeapEnd
         */

        /* We pass GC a request that is larger than it could possibly
         * fulfill, so as to force a GC.
         */
        garbageCollect(AllHeapEnd - AllHeapStart);

        if (newPermanentSpace < (cell*)FirstFreeChunk + 2 * HEADERSIZE) {
            raiseExceptionWithMessage(OutOfMemoryError,
                KVM_MSG_UNABLE_TO_EXPAND_PERMANENT_MEMORY);
        } else {
            int newFreeSize =
                (newPermanentSpace - (cell*)FirstFreeChunk - HEADERSIZE);
            memset(newPermanentSpace, 0,
                   PTR_DELTA(CurrentHeapEnd, newPermanentSpace));
            CurrentHeapEnd = newPermanentSpace;
            FirstFreeChunk->size = newFreeSize << TYPEBITS;
        }
    }
    return result;
#else /* ENABLE_HEAP_COMPACTION */
    result = callocObject(size, GCT_NOPOINTERS);
    OBJECT_HEADER(result) |= STATICBIT;
    return result;
#endif /* ENABLE_HEAP_COMPACTION */
}

/*=========================================================================
 * Garbage collection operations
 *=======================================================================*/

void
garbageCollectForReal(int realSize)
{
    CHUNK firstFreeChunk;
    long maximumFreeSize;

    /* The actual high-level GC algorithm is here */
    markRootObjects();
    markNonRootObjects();
    markWeakPointerLists();
    markWeakReferences();
    firstFreeChunk = sweepTheHeap(&maximumFreeSize);
#if ENABLE_HEAP_COMPACTION
    if (realSize > maximumFreeSize) {
        /* We need to compact the heap. */
        breakTableStruct currentTable;
        cell* freeStart = compactTheHeap(&currentTable, firstFreeChunk);
        if (currentTable.length > 0) {
            updateRootObjects(&currentTable);
            updateHeapObjects(&currentTable, freeStart);
        }
        if (freeStart < CurrentHeapEnd - 1) {
            firstFreeChunk = (CHUNK)freeStart;
            firstFreeChunk->size =
                (CurrentHeapEnd - freeStart - HEADERSIZE) << TYPEBITS;
            firstFreeChunk->next = NULL;
        } else {
            /* We are so utterly hosed.
             * Memory is completely full, and there is no free space whatsoever.
             */
            firstFreeChunk = NULL;
        }
    }
#endif
    FirstFreeChunk = firstFreeChunk;
}

/*=========================================================================
 * FUNCTION:      markRootObjects
 * TYPE:          private garbage collection operation
 * OVERVIEW:      Marks the root objects of garbage collection
 * INTERFACE:
 *   parameters:  none
 *   returns:     none
 *=======================================================================*/

static void
markRootObjects(void)
{
    cell *heapSpace = CurrentHeap;
    cell *heapSpaceEnd = CurrentHeapEnd;
    cellOrPointer *ptr, *endptr;

    HASHTABLE stringTable;
    THREAD thread;

    ptr = &GlobalRoots[0];
    endptr = ptr + GlobalRootsLength;
    for ( ; ptr < endptr; ptr++) {
        MARK_OBJECT_IF_NON_NULL(*(ptr->cellpp));
    }

    ptr = &TemporaryRoots[0];
    endptr = ptr + TemporaryRootsLength;
    for ( ; ptr < endptr; ptr++) {
        cellOrPointer location = *ptr;
        if (location.cell == -1) {
            /* Actual Location is ptr[1], base is ptr[2] */
            MARK_OBJECT_IF_NON_NULL(ptr[2].cellpp);
            ptr += 2;
        } else {
            MARK_OBJECT_IF_NON_NULL(*(location.cellpp));
        }
    }

#if ASYNCHRONOUS_NATIVE_FUNCTIONS
{
    int i;
    for (i = 0 ; i < ASYNC_IOCB_COUNT ; i++) {
        ASYNCIOCB *aiocb = &IocbRoots[i];
        MARK_OBJECT_IF_NON_NULL(aiocb->thread);
        MARK_OBJECT_IF_NON_NULL(aiocb->instance);
        MARK_OBJECT_IF_NON_NULL(aiocb->array);
    }
}
#endif

#if ROMIZING
    {
        /* In RELOCATABLE_ROM builds, we have a pointer to the static data.
         * In !RELOCATABLE_ROM builds, we have the actual array.
         */
#if RELOCATABLE_ROM
        long *staticPtr = KVM_staticDataPtr;
#else
        long *staticPtr = KVM_staticData;
#endif
        int refCount = staticPtr[0];
        for( ; refCount > 0; refCount--) {
            MARK_OBJECT_IF_NON_NULL((cell *)staticPtr[refCount]);
        }
    }
#endif

    stringTable = InternStringTable;
    if (ROMIZING || stringTable != NULL) {
        int count = stringTable->bucketCount;
        while (--count >= 0) {
            INTERNED_STRING_INSTANCE instance =
                (INTERNED_STRING_INSTANCE)stringTable->bucket[count];
            for ( ; instance != NULL; instance = instance->next) {
                checkMonitorAndMark((OBJECT)instance);
            }
        }
    }

    if (ROMIZING || ClassTable != NULL) {
        FOR_ALL_CLASSES(clazz)
            checkMonitorAndMark((OBJECT)clazz);
            if (!IS_ARRAY_CLASS(clazz)) {
                INSTANCE_CLASS iclazz = (INSTANCE_CLASS)clazz;
                POINTERLIST statics = iclazz->staticFields;
                METHODTABLE  methodTable = iclazz->methodTable;
                MARK_OBJECT_IF_NON_NULL(iclazz->initThread);

                if (clazz->accessFlags & ACC_ROM_CLASS) {
                    continue;
                }

                if (USESTATIC) {
                    MARK_OBJECT_IF_NON_NULL(iclazz->constPool);
                    MARK_OBJECT_IF_NON_NULL(iclazz->ifaceTable);
                    MARK_OBJECT_IF_NON_NULL(iclazz->fieldTable);
                    MARK_OBJECT_IF_NON_NULL(iclazz->methodTable);
                }

                if (statics != NULL) {
                    int count = statics->length;
                    while (--count >= 0) {
                        MARK_OBJECT_IF_NON_NULL(statics->data[count].cellp);
                    }
                }

                if (iclazz->status == CLASS_VERIFIED) {
                    continue;
                }

                FOR_EACH_METHOD(thisMethod, methodTable)
                    /* Mark the bytecode object alive for non-native methods */
                    if (!(thisMethod->accessFlags & ACC_NATIVE)) {
                        checkValidHeapPointer((cell *)
                               thisMethod->u.java.stackMaps.verifierMap);
                        MARK_OBJECT_IF_NON_NULL(thisMethod->u.java.stackMaps.verifierMap);
                    }
                END_FOR_EACH_METHOD
            }
        END_FOR_ALL_CLASSES
    }

    for (thread = AllThreads; thread != NULL; thread = thread->nextAliveThread){
        MARK_OBJECT(thread);
        if (thread->javaThread != NULL) {
            MARK_OBJECT(thread->javaThread);
        }
        if (thread->stack != NULL) {
            markThreadStack(thread);
        }
    }
}

#if ENABLE_HEAP_COMPACTION

static void
updateRootObjects(breakTableStruct *currentTable)
{
    HASHTABLE stringTable;
    cellOrPointer *ptr, *endptr;

    ptr = &GlobalRoots[0];
    endptr = ptr + GlobalRootsLength;
    for ( ; ptr < endptr; ptr++) {
        updatePointer(ptr->cellpp, currentTable);
    }

    ptr = &TemporaryRoots[0];
    endptr = ptr + TemporaryRootsLength;
    for ( ; ptr < endptr; ptr++) {
        cellOrPointer location = *ptr;
        if (location.cell == -1) {
            /* Actual Location is ptr[1], base is ptr[2] */
            long offset = *(ptr[1].charpp) - ptr[2].charp;
            updatePointer(&ptr[2], currentTable);
            *(ptr[1].charpp) = ptr[2].charp + offset;
            ptr += 2;
        } else {
            updatePointer(location.cellpp, currentTable);
        }
    }

#if ASYNCHRONOUS_NATIVE_FUNCTIONS
{
    int i;
    for (i = 0 ; i < ASYNC_IOCB_COUNT ; i++) {
        ASYNCIOCB *aiocb = &IocbRoots[i];
        updatePointer(&aiocb->thread, currentTable);
        updatePointer(&aiocb->instance, currentTable);
        updatePointer(&aiocb->array, currentTable);
    }
}
#endif

#if ROMIZING
    {
        /* In RELOCATABLE_ROM builds, we have a pointer to the static data.
         * In !RELOCATABLE_ROM builds, we have the actual array.
         */
#if RELOCATABLE_ROM
        long *staticPtr = KVM_staticDataPtr;
#else
        long *staticPtr = KVM_staticData;
#endif
        int refCount = staticPtr[0];
        for( ; refCount > 0; refCount--) {
            updatePointer(&staticPtr[refCount], currentTable);
        }
    }
#endif /* ROMIZING */

    stringTable = InternStringTable;
    if (ROMIZING || stringTable != NULL) {
        int count = stringTable->bucketCount;
        while (--count >= 0) {
            INTERNED_STRING_INSTANCE instance =
                (INTERNED_STRING_INSTANCE)stringTable->bucket[count];
            for ( ; instance != NULL; instance = instance->next) {
                updateMonitor((OBJECT)instance, currentTable);
            }
        }
    }

    if (ROMIZING || ClassTable != NULL) {
        FOR_ALL_CLASSES(clazz)
            updateMonitor((OBJECT)clazz, currentTable);
            if (!IS_ARRAY_CLASS(clazz)) {
                INSTANCE_CLASS iclazz = (INSTANCE_CLASS)clazz;
                POINTERLIST statics = iclazz->staticFields;
                THREAD initThread = iclazz->initThread;

                if (initThread != NULL) {
                    updatePointer(&initThread, currentTable);
                    setClassInitialThread(iclazz, initThread);
                }

                if (clazz->accessFlags & ACC_ROM_CLASS) {
                    continue;
                }

                if (USESTATIC) {
                    updatePointer(&iclazz->constPool, currentTable);
                    updatePointer(&iclazz->ifaceTable, currentTable);
                    updatePointer(&iclazz->fieldTable, currentTable);
                    updatePointer(&iclazz->methodTable, currentTable);
                }

                if (statics != NULL) {
                    int count = statics->length;
                    while (--count >= 0) {
                        updatePointer(&statics->data[count].cellp, currentTable);
                    }
                }

                if (iclazz->status == CLASS_VERIFIED) {
                    continue;
                }

                FOR_EACH_METHOD(thisMethod, iclazz->methodTable)
                    if (!(thisMethod->accessFlags & ACC_NATIVE)) {
                        if (!USESTATIC || inCurrentHeap(thisMethod)) {
                            updatePointer(&thisMethod->u.java.stackMaps.verifierMap, currentTable);
                        } else {
                            POINTERLIST oldValue = thisMethod->u.java.stackMaps.verifierMap;
                            POINTERLIST newValue = oldValue;
                            updatePointer(&newValue, currentTable);
                            if (oldValue != newValue) {
                                CONSTANTPOOL cp = iclazz->constPool;
                                int offset = (char *)&thisMethod->u.java.stackMaps.pointerMap
                                            - (char *)cp;
                                modifyStaticMemory(cp, offset, &newValue, sizeof(POINTERLIST));
                            }
                        }
                    }
                END_FOR_EACH_METHOD
            }
        END_FOR_ALL_CLASSES
    }
}

#endif /* ENABLE_HEAP_COMPACTION */

/*=========================================================================
 * FUNCTION:      markNonRootObjects
 * TYPE:          private garbage collection operation
 * OVERVIEW:      Scan the heap looking for objects that are reachable only
 *                from other heap objects.
 * INTERFACE:
 *   parameters:  None
 *   returns:     <nothing>
 * COMMENTS:      This code >>tries<< to do all its work in a single pass
 *                through the heap.  For each live object in the heap, it
 *                calls the function markChildren().  This latter function
 *                sets the variable deferredObjectTableOverflow if it was
 *                ever unable to completely following the children of an
 *                object because of recursion overflow.  In this rare case,
 *                we just rescan the heap.  We're guaranteed to get further
 *                to completion on each pass.
 *=======================================================================*/

#define MAX_GC_DEPTH 4

static void
markNonRootObjects(void) {
    /* Scan the entire heap, looking for badly formed headers */
    cell* scanner;
    cell* endScanPoint = CurrentHeapEnd;
    int scans = 0;
    do {
        WeakPointers = NULL;
        WeakReferences = NULL;
        initializeDeferredObjectTable();
        for (scanner = CurrentHeap;
                scanner < endScanPoint;
                scanner += SIZE(*scanner) + HEADERSIZE) {
            if (ISMARKED(*scanner)) {
                cell *object = scanner + 1;
                /* See markChildren() for comments on the arguments */
                markChildren(object, object, MAX_GC_DEPTH);
            }
        }
        if (ENABLEPROFILING) {
            scans++;
        }
        /* This loop runs exactly once in almost all cases */
    } while (deferredObjectTableOverflow);
#if ENABLEPROFILING
    GarbageCollectionRescans += (scans - 1);
#endif
}

/*=========================================================================
 * FUNCTION:      markChildren()
 * TYPE:          private garbage collection operation
 * OVERVIEW:      Scan the contents of an individual object,
 *                recursively marking all those objects that
 *                are reachable from it.
 * INTERFACE:
 *   parameters:  child: an object pointer
 *                limit: the current location of the scanner in
 *                          markNonRootObjects
 *                depth: The number of times further for which this function
 *                          can recurse, before we start adding objects to
 *                          the deferral queue.
 *   returns:     <nothing>
 *
 * This function tries >>very<< hard not to be recursive.  It uses several
 * tricks:
 *   1) It will only recursively mark objects whose location in the heap
 *      is less than the value "limit".  It marks but doesn't recurse on
 *      objects "above the limit" because markNonRootObjects() will
 *      eventually get to them.
 *
 *   2) The variable nextObject is set in those cases in which we think, or
 *      hope that only a single child of the current node needs to be
 *      followed.  In these cases, we iterate (making explicit the tail
 *      recursion), rather than calling ourselves recursively
 *
 *   3) We only allow the stack to get to a limited depth.  If the depth
 *      gets beyond that, we save the sub object on a "deferred object"
 *      queue, and then iterate on these deferred objects once the depth
 *      gets back down to zero.
 *
 *   4) If the deferred object queue overflows, we just throw up our
 *      hands, and scan the stack multiple times.  We're guaranteed to make
 *      some progress in each pass through the stack.  It's hard to
 *      create a case that needs more than one pass through the stack.
 *=======================================================================*/

static void
markChildren(cell* object, cell* limit, int remainingDepth)
{
    cell *heapSpace = CurrentHeap;
    cell *heapSpaceEnd = CurrentHeapEnd;

/* Call this macro to mark a child, when we don't think that there is
 * any useful reason to try for tail recursion (i.e. there's something
 * later in the object that's better to tail recurse on.
 */
#define MARK_AND_RECURSE(child) \
    if (inHeapSpaceFast(child)) {                            \
        cell _tmp_ = OBJECT_HEADER(child);                   \
        if (!ISKEPT(_tmp_)) {                                \
            OBJECT_HEADER(child) = _tmp_ | MARKBIT;          \
            if ((cell*)child < limit) {                      \
                RECURSE((cell *)child);                      \
            }                                                \
        }                                                    \
    }

/* Call this macro to mark a child, when we think tail recursion might
 * be useful on this field.
 * This macro does the right thing, even if we might have already picked
 * another child on which to tail recurse.
 *
 * [We use this macro when looking at elements of an array, or fields of
 * an instance, since we want to tail recurse on one of the children.
 * We just don't know which ones will be good to recurse on
 */
#define MARK_AND_TAIL_RECURSE(child)                         \
    if (inHeapSpaceFast(child)) {                            \
        cell _tmp_ = OBJECT_HEADER(child);                   \
        if (!ISKEPT(_tmp_)) {                                \
            OBJECT_HEADER(child) = _tmp_ | MARKBIT;          \
            if ((cell*)child < limit) {                      \
                if (nextObject != NULL) {                    \
                    RECURSE(nextObject);                     \
                }                                            \
                nextObject = (cell *)(child);                \
            }                                                \
        }                                                    \
    }

/* This is the same as MARK_AND_TAIL_RECURSE, except we only call it when we
 * absolutely know that we have not yet called MARK_AND_TAIL_RECURSE, so
 * we know that we have not yet picked a child upon which to tail recurse.
 */

#define MARK_AND_TAIL_RECURSEX(child)                        \
    if (inHeapSpaceFast(child)) {                            \
        cell _tmp_ = OBJECT_HEADER(child);                   \
        if (!ISKEPT(_tmp_))  {                               \
            OBJECT_HEADER(child) = _tmp_ | MARKBIT;          \
            if ((cell*)child < limit) {                      \
                nextObject = (cell *)(child);                \
            }                                                \
        }                                                    \
    }

/* Used only in the macros above.  Either recurse on the child, or
 * save it in the deferred items list.
 *
 * Note that remainingDepth will have already been decremented from the value
 * that it originally had when the function was called
 */

#define RECURSE(child)                                       \
    if (remainingDepth < 0) {                                \
       putDeferredObject(child);                             \
    } else {                                                 \
       markChildren(child, limit, remainingDepth);           \
    }

    /* If non-NULL, then it holds the value of object for the next iteration
     * through the loop.  Used to implement tail recursion.
     */
    cell *nextObject = NULL;

    remainingDepth -= 1;        /* remaining depth for any subcalls */

    for (;;) {
        cell *header = object - HEADERSIZE;
        GCT_ObjectType gctype = TYPE(*header);

#if INCLUDEDEBUGCODE
        if (tracegarbagecollectionverbose) {
            fprintf(stdout, "GC Mark: ");
            printObject(object);
        }
#endif
        switch (gctype) {
            int length;
            cell **ptr;

        case GCT_INSTANCE: {
            /* The object is a Java object instance.  Mark pointer fields */
            INSTANCE instance = (INSTANCE)object;
            INSTANCE_CLASS clazz = instance->ofClass;

            /* Mark the possible monitor object alive */
            checkMonitorAndMark((OBJECT)instance);

            while (clazz) {
                /* This is the tough part: walk through all the fields */
                /* of the object and see if they contain pointers */
                FOR_EACH_FIELD(thisField, clazz->fieldTable)
                    /* Is this a non-static pointer field? */
                    if ((thisField->accessFlags & (ACC_POINTER | ACC_STATIC))
                               == ACC_POINTER) {
                        int offset = thisField->u.offset;
                        cell* subobject = instance->data[offset].cellp;
                        MARK_AND_TAIL_RECURSE(subobject);
                    }
                END_FOR_EACH_FIELD
                clazz = clazz->superClass;
            }
            break;
        }

        case GCT_ARRAY:
            /* The object is a Java array with primitive values. */
            checkMonitorAndMark((OBJECT)object);
            break;

        case GCT_POINTERLIST: {
            POINTERLIST list = (POINTERLIST)object;
            length = list->length;
            ptr = &list->data[0].cellp;
            goto markArray;
        }

        case GCT_WEAKPOINTERLIST:
            /* Don't mark children, we only want to update the pointers
             * if the objects are marked because of other references
             */
            /* Push this object onto the linked list of weak pointers */
            ((WEAKPOINTERLIST)object)->gcReserved = WeakPointers;
            WeakPointers = (WEAKPOINTERLIST)object;
            break;

        case GCT_OBJECTARRAY:
            /* The object is a Java array with object references. */
            checkMonitorAndMark((OBJECT)object);
            length = ((ARRAY)object)->length;
            ptr = &((ARRAY)object)->data[0].cellp;
            /* FALL THROUGH */

        markArray:
            /* Keep objects in the array alive. */
            while (--length >= 0) {
                cell *subobject = *ptr++;
                MARK_AND_TAIL_RECURSE(subobject);
            }
            break;

        /* Added for java.lang.ref.WeakReference support in CLDC 1.1 */
        case GCT_WEAKREFERENCE: {
            /* Mark the possible monitor object alive */
            checkMonitorAndMark((OBJECT)object);

            /* Push this object onto the linked list of weak refs */
            ((WEAKREFERENCE)object)->gcReserved = WeakReferences;
            WeakReferences = (WEAKREFERENCE)object;

            /* NOTE: We don't mark the 'referent' field of the */
            /* weak reference object here, because we want to */
            /* keep the referent alive only if there are */
            /* strong references to it. */
            break;
        }

        case GCT_METHODTABLE:
            FOR_EACH_METHOD(thisMethod, ((METHODTABLE)object))
                if ((thisMethod->accessFlags & ACC_NATIVE) == 0) {
                    MARK_OBJECT(thisMethod->u.java.code);
                    MARK_OBJECT_IF_NON_NULL(thisMethod->u.java.handlers);
                }
            END_FOR_EACH_METHOD
            break;

        case GCT_MONITOR:
            /* The only pointers in a monitor will get handled elsewhere */
            break;

        case GCT_THREAD:
            /* Scanning of all live threads is done as part of the root set */
            break;

        case GCT_NOPOINTERS:
            /* The object does not have any pointers in it */
            break;

        case GCT_EXECSTACK:
            /* Scanning of executing stack is done as part of the root set */
            break;

        default:
            /* We should never get here as isValidHeapPointer should */
            /* guarantee that the header tag of this object is in the */
            /* range GCT_FIRSTVALIDTAG && GCT_LASTVALIDTAG */
            fatalVMError(KVM_MSG_BAD_DYNAMIC_HEAP_OBJECTS_FOUND);

        } /* End of switch statement */

        if (nextObject != NULL) {
            /* We can perform a tail recursive call. */
            object = nextObject;
            nextObject = NULL;
            /* continue */
        } else if (remainingDepth == MAX_GC_DEPTH - 1
                         && deferredObjectCount > 0) {
            /* We're at the outer level of recursion and there is a deferred
             * object for us to look up.  Note that remainingDepth has
             * already been decremented, so we don't compare against
             * MAX_GC_DEPTH
             */
            object = getDeferredObject();
            /* continue */
        } else {
            break;              /* finish "for" loop. */
        }
    } /* end of for(ever) loop */
}

/*=========================================================================
 * FUNCTION:      markThreadStack
 * TYPE:          private garbage collection operation
 * OVERVIEW:      Scan the execution stack of the given thread, and
 *                mark all the objects that are reachable from it.
 * INTERFACE:
 *   parameters:  pointer to the thread whose stack is to be scanned.
 *   returns:     <nothing>
 *=======================================================================*/

static void
markThreadStack(THREAD thisThread)
{
    /* Perform a complete stack trace, looking for pointers  */
    /* inside the stack and marking the corresponding objects. */

    cell *heapSpace = CurrentHeap;
    cell *heapSpaceEnd = CurrentHeapEnd;

    FRAME  thisFP = thisThread->fpStore;
    cell*  thisSP = thisThread->spStore;
    BYTE*  thisIP = thisThread->ipStore;
    char   map[(MAXIMUM_STACK_AND_LOCALS + 7) >> 3];
    long   i;

    STACK stack = thisThread->stack;

    if (thisFP == NULL) {
        MARK_OBJECT_IF_NON_NULL(stack);
        return;
    }

    thisFP->stack->next = NULL; /* we're the end of the stack chain. */

    while (thisFP != NULL) {
        /* Get a pointer to the method of the current frame */
        METHOD method = thisFP->thisMethod;
        cell* localVars = FRAMELOCALS(thisFP);
        cell *operandStack = (cell*)thisFP + SIZEOF_FRAME;
        int localsCount = method->frameSize;
        long realStackSize = thisSP - (cell*)thisFP - SIZEOF_FRAME + 1;
        unsigned int totalSize = realStackSize + localsCount;

        /* Mark the possible synchronization object alive */
        MARK_OBJECT_IF_NON_NULL(thisFP->syncObject);
        MARK_OBJECT_IF_NON_NULL(thisFP->stack);

        if (method == RunCustomCodeMethod) {
            memset(map, -1, (realStackSize + 7) >> 3);
        } else {
            unsigned int expectedStackSize =
                getGCRegisterMask(method, thisIP, map);
            if ((unsigned int)realStackSize > expectedStackSize) {
                /* If the garbage collector gets called from */
                /* inside KNI method calls, the execution stack */
                /* may contain more elements than the stack map */
                /* indicates => normalize the stack size for */
                /* this situation */
                totalSize = expectedStackSize + localsCount;
            }
        }

        for (i = 0; i < totalSize; i++) {
            if (map[i >> 3] & (1 << (i & 7))) {
                cell *arg;
                if (i < localsCount) {
                    arg = *(cell **)&localVars[i];
                } else {
                    arg = *(cell **)&operandStack[i - localsCount];
                }
                if (INCLUDEDEBUGCODE && method != RunCustomCodeMethod) {
                    checkValidHeapPointer(arg);
                }
                MARK_OBJECT_IF_NON_NULL(arg);
            }
        }

        /* This frame is now done.  Go to the previous one */
        /* using the same algorithm as in 'popFrame()'. */
        thisSP = thisFP->previousSp;
        thisIP = thisFP->previousIp;
        thisFP = thisFP->previousFp;
    }
}

static void checkMonitorAndMark(OBJECT object)
{
    /* We only need to mark real monitors.  We don't need to mark threads'
     * in the monitor/hashcode slot since they will be marked elsewhere */
    if (OBJECT_HAS_REAL_MONITOR(object)) {
        cell *heapSpace = CurrentHeap;
        cell *heapSpaceEnd = CurrentHeapEnd;
        MONITOR monitor = OBJECT_MHC_MONITOR(object);
        /* A monitor doesn't contain any subobjects that won't be marked
         * elsewhere */
        MARK_OBJECT(monitor);
    }
}

static void
markWeakPointerLists()
{
    WEAKPOINTERLIST list;
    /* save the current native method pointer */
    cell* currentNativeLp = NULL;
    if (CurrentThread) {
        currentNativeLp = CurrentThread->nativeLp;
    }

    for (list = WeakPointers; list != NULL; list = list->gcReserved) {
        void (*finalizer)(INSTANCE_HANDLE) = list->finalizer;
        cellOrPointer *ptr = &list->data[0];
        cellOrPointer *endPtr = ptr + list->length;

        for (; ptr < endPtr; ptr++) {
            cell* object = ptr->cellp;
            if (object != NULL) {
                if (!ISKEPT((object)[-HEADERSIZE])) {
                    ptr->cellp = NULL;
                    if (finalizer) {
                        /* In KVM, making the 'this' pointer available   */ 
                        /* in native finalizers is rather painful.       */
                        /* KVM doesn't create stack frames for native    */ 
                        /* methods, and therefore the stack may not      */
                        /* contain anything to which 'nativeLp' could    */ 
                        /* point to create access to the 'this' pointer. */ 
                        /* Therefore, it is necessary to set 'nativeLp'  */
                        /* to point to a 'fake' location each time we    */
                        /* need to call a native finalizer.              */
                        if (CurrentThread) {
                            CurrentThread->nativeLp = (cell *)&object;
                        }

                        /* In KVM 1.0.4 and 1.1, we pass the object  */
                        /* to the finalizer as a handle.  This makes */
                        /* it possible to define native finalizers   */
                        /* as KNI functions that take a 'jobject'    */
                        /* parameter. */
                        finalizer((INSTANCE_HANDLE)&object);
                    }

                }
            }
        }
    }

    /* Restore the original 'nativeLp' */    
    if (CurrentThread) {
        CurrentThread->nativeLp = currentNativeLp;
    }
}

/*=========================================================================
 * FUNCTION:      markWeakReferences
 * OVERVIEW:      Handle the weak reference objects (CLDC 1.1).
 *                The 'referent' field of a weak reference object
 *                needs to be cleared if the referent object is
 *                not referred to by at least one strong
 *                reference.
 * INTERFACE:
 *   parameters:  <none>
 * NOTE:          The name of this routine is a bit misleading, since
 *                we aren't really "marking" anything.  Rather, we are
 *                clearing references to those objects that are referred
 *                to by weak references only.
 *=======================================================================*/

static void
markWeakReferences()
{
    WEAKREFERENCE thisRef;

    /* Walk through the list of weak reference objects that was */
    /* created earlier during the markChildren phase.  This list */
    /* contains all the weak reference objects that exist in the heap */
    for (thisRef = WeakReferences; thisRef != NULL; thisRef = thisRef->gcReserved) {

        /* If the referent object is not marked, clear the weak reference */
        cell* referent = (cell*)thisRef->referent;
        if (referent != NULL && !ISKEPT((referent)[-HEADERSIZE]))
            thisRef->referent = NULL;
    }
}

#if ENABLE_HEAP_COMPACTION

static void
updateHeapObjects(breakTableStruct *currentTable, cell* endScanPoint)
{
    cell* scanner;
    for (   scanner = CurrentHeap;
            scanner < endScanPoint;
            scanner += SIZE(*scanner) + HEADERSIZE) {
        cell *header = scanner;
        cell *object = scanner + 1;
        GCT_ObjectType gctype = TYPE(*header);
        switch (gctype) {
            int length;
            cell **ptr;

        case GCT_INSTANCE:
        case GCT_WEAKREFERENCE: {
            /* The object is a Java object instance.  Mark pointer fields */
            INSTANCE instance = (INSTANCE)object;
            INSTANCE_CLASS clazz = instance->ofClass;
            updateMonitor((OBJECT)instance, currentTable);
            while (clazz) {
                FOR_EACH_FIELD(thisField, clazz->fieldTable)
                    /* Is this a non-static pointer field? */
                    if ((thisField->accessFlags & (ACC_POINTER | ACC_STATIC))
                               == ACC_POINTER) {
                       updatePointer(&instance->data[thisField->u.offset].cellp,
                                     currentTable);
                    }
                END_FOR_EACH_FIELD
                clazz = clazz->superClass;
            }
            break;
        }

        case GCT_ARRAY: {
            /* The object is a Java array with primitive values. */
            /* Only the possible monitor will have to be marked alive. */
            updateMonitor((OBJECT)object, currentTable);
        }
        break;

        case GCT_POINTERLIST: {
            POINTERLIST list = (POINTERLIST)object;
            length = list->length;
            ptr = &list->data[0].cellp;
            goto markArray;
        }

        case GCT_WEAKPOINTERLIST: {
            WEAKPOINTERLIST list = (WEAKPOINTERLIST)object;
            length = list->length;
            ptr = &list->data[0].cellp;
            goto markArray;
        }

        case GCT_OBJECTARRAY: {
            /* The object is a Java array with object references. */
            /* The contents of the array and the possible monitor  */
            /* will have to be scanned. */
            ARRAY array = (ARRAY)object;
            updateMonitor((OBJECT)array, currentTable);

            length = array->length;
            ptr = &array->data[0].cellp;
            /* FALL THROUGH */
        }

        markArray:
            /* Keep objects in the array alive. */
            while (--length >= 0) {
                updatePointer(ptr, currentTable);
                ptr++;
            }
            break;

        case GCT_MONITOR: {
            MONITOR monitor  = (MONITOR)object;
            updatePointer(&monitor->owner, currentTable);
            updatePointer(&monitor->monitor_waitq, currentTable);
            updatePointer(&monitor->condvar_waitq, currentTable);
#if INCLUDEDEBUGCODE
            updatePointer(&monitor->object, currentTable);
#endif
            break;
        }

        case GCT_THREAD: {
            THREAD thread  = (THREAD)object;
            updatePointer(&thread->nextAliveThread, currentTable);
            updatePointer(&thread->nextThread, currentTable);
            updatePointer(&thread->javaThread, currentTable);
            updatePointer(&thread->monitor, currentTable);
            updatePointer(&thread->nextAlarmThread, currentTable);
            updatePointer(&thread->stack, currentTable);

#if  ENABLE_JAVA_DEBUGGER
          {
            updatePointer(&thread->stepInfo.fp, currentTable);
          }
#endif
            if (thread->fpStore != NULL) {
                updateThreadAndStack(thread, currentTable);
            }
            break;
        }

        case GCT_METHODTABLE:
            FOR_EACH_METHOD(thisMethod, ((METHODTABLE)object))
                if ((thisMethod->accessFlags & ACC_NATIVE) == 0) {
                    updatePointer(&thisMethod->u.java.code, currentTable);
                    updatePointer(&thisMethod->u.java.handlers, currentTable);
                }
            END_FOR_EACH_METHOD
            break;

        case GCT_NOPOINTERS:
            break;

        case GCT_EXECSTACK:
            /* This is handled by the thread that the stack belongs to. */
            break;

        default:
            /* We should never get here as isValidHeapPointer should */
            /* guarantee that the header tag of this object is in the */
            /* range GCT_FIRSTVALIDTAG && GCT_LASTVALIDTAG */
            fatalError(KVM_MSG_BAD_DYNAMIC_HEAP_OBJECTS_FOUND);

        } /* End of switch statement */

    } /* End of for statement */
}

static void
updateMonitor(OBJECT object, breakTableStruct *currentTable)
{
    if (OBJECT_HAS_MONITOR(object)) {
        /* The monitor slot contains either a MONITOR or a THREAD,
         * incremented by a small constant, so we will be pointing into the
         * middle of the first word.
         *
         * In this collector, updatePointer() correctly handles pointers into
         * the middle of an object, so the following code is simpler then it
         * would be with other collectors.
         */
        void *oldTemp = object->mhc.address;
        void *newTemp = oldTemp;
        updatePointer((cell **)&newTemp, currentTable);
        if (!USESTATIC || (oldTemp != newTemp)) {
            SET_OBJECT_MHC_ADDRESS(object, (char *)newTemp);
        }
    }
}

static void
updateThreadAndStack(THREAD thread, breakTableStruct *currentTable)
{
    char   map[(MAXIMUM_STACK_AND_LOCALS + 7) >> 3];
    long   delta;
    FRAME  thisFP;
    cell*  thisSP;
    BYTE*  thisIP;
    STACK  oldStack, newStack, stack;
    int    i;

    updatePointer(&thread->fpStore, currentTable);
    updatePointer(&thread->spStore, currentTable);

    /* Added for KNI */
    updatePointer(&thread->nativeLp, currentTable);

    thisSP = thread->spStore;   /* new address */
    thisFP = thread->fpStore;   /* new address*/
    thisIP = thread->ipStore;

    oldStack = stack = thisFP->stack;
    updatePointer(&stack, currentTable);
    newStack = stack;
    delta = PTR_DELTA(newStack, oldStack);

    for (;;) {
        /* thisSP and thisFP are pointers to the current frame
         *       in new space.
         * oldStack and newStack contain the old and new addresses of the
         *       stack containing the current frame.
         */
        METHOD method = thisFP->thisMethod;
        cell* localVars = FRAMELOCALS(thisFP);
        cell *operandStack = (cell*)thisFP + SIZEOF_FRAME;
        int localsCount = method->frameSize;
        long realStackSize = thisSP - (cell*)thisFP - SIZEOF_FRAME + 1;
        unsigned int totalSize = realStackSize + localsCount;
        FRAME previousFp;

        /* Mark the possible synchronization object alive */
        updatePointer(&thisFP->syncObject, currentTable);

        if (method == RunCustomCodeMethod) {
            memset(map, -1, (realStackSize + 7) >> 3);
        } else {
            getGCRegisterMask(method, thisIP, map);
        }
        for (i = 0; i < totalSize; i++) {
            if (map[i >> 3] & (1 << (i & 7))) {
                cell **argP;
                if (i < localsCount) {
                    argP = (cell **)&localVars[i];
                } else {
                    argP = (cell **)&operandStack[i - localsCount];
                }
                updatePointer(argP, currentTable);
            }
        }
        if (INCLUDEDEBUGCODE && thisFP->stack != oldStack) {
            fatalError(KVM_MSG_BAD_STACK_INFORMATION);
        }
        thisFP->stack = newStack;

        previousFp = thisFP->previousFp; /* old address of previousFP */
        if (previousFp == NULL) {
            break;
        } else {
            STACK prevOldStack, prevNewStack;
            updatePointer(&previousFp, currentTable);
            prevOldStack = previousFp->stack;  /* old address */
            if (prevOldStack == oldStack) {
                thisFP->previousSp = PTR_OFFSET(thisFP->previousSp, delta);
                thisFP->previousFp = PTR_OFFSET(thisFP->previousFp, delta);
            } else {
                updatePointer(&thisFP->previousFp, currentTable);
                updatePointer(&thisFP->previousSp, currentTable);

                stack = prevOldStack;
                updatePointer(&stack, currentTable);
                prevNewStack = stack;
                prevNewStack->next = newStack;

                oldStack = prevOldStack;
                newStack = prevNewStack;
                delta = PTR_DELTA(newStack, oldStack);
            }
        }
        /* This frame is now done.  Go to the previous one. */
        thisSP = thisFP->previousSp;
        thisIP = thisFP->previousIp;
        thisFP = thisFP->previousFp;
    }
}

#endif /* ENABLE_HEAP_COMPACTION */

/*=========================================================================
 * Deferred objects, as part of markChildren.
 *
 * These functions create a queue of objects that are waiting to be
 * scanned so that their children can be marked.  The active part of the
 * starts at startDeferredObjects and ends just before endDeferredObjects.
 * This queue is circular.
 *
 * The two main functions are putDeferredObject() and getDeferredObject()
 * which add and remove objects from the queue.  putDeferredObject()
 * correctly handles overflow.  getDeferredObject() presumes that the user
 * has first checked that there is an object to be gotten.
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      initializeDeferredObjectTable
 * TYPE:          scanning the heap
 * OVERVIEW:      Initialize the deferred object table so that it correctly
 *                behaves like a queue.
 * INTERFACE:
 *   parameters:  <none>
 *=======================================================================*/

static void initializeDeferredObjectTable(void) {
    startDeferredObjects = endDeferredObjects = deferredObjectTable;
    deferredObjectCount = 0;
    deferredObjectTableOverflow = FALSE;
}

/*=========================================================================
 * FUNCTION:      putDeferredObject
 * TYPE:          scanning the heap
 * OVERVIEW:      Add an object to the end of the queue.  If the queue is
 *                full, just set the variable deferredObjectTableOverflow
 *                and ignore the request.
 * INTERFACE:
 *   parameters:  value: An object to put on the queue
 *=======================================================================*/

static void
putDeferredObject(cell *value)
{
    if (deferredObjectCount >= DEFERRED_OBJECT_TABLE_SIZE) {
        deferredObjectTableOverflow = TRUE;
    } else {
        if (endDeferredObjects == endDeferredObjectTable) {
            endDeferredObjects = deferredObjectTable;
        }
        *endDeferredObjects++ = value;
        deferredObjectCount++;
    }
#if ENABLEPROFILING
    if (deferredObjectCount > MaximumGCDeferrals) {
        MaximumGCDeferrals = deferredObjectCount;
    }
    TotalGCDeferrals++;
#endif
}

/*=========================================================================
 * FUNCTION:      getDeferredObject
 * TYPE:          scanning the heap
 * OVERVIEW:      Get the next object from the front of the queue.
 *                The user should check the value of deferredObjectCount
 *                before calling this function.
 * INTERFACE:
 *   parameters:  none
 *   returns:     The next object on the queue.
 *=======================================================================*/

static cell*
getDeferredObject(void)
{
    if (startDeferredObjects == endDeferredObjectTable) {
        startDeferredObjects = deferredObjectTable;
    }
    deferredObjectCount--;
    return *startDeferredObjects++;
}

static CHUNK
sweepTheHeap(long *maximumFreeSizeP)
{
    CHUNK firstFreeChunk = NULL;
    CHUNK newChunk;
    CHUNK* nextChunkPtr = &firstFreeChunk;
    bool_t done = FALSE;

    cell* scanner =  CurrentHeap; /* current object */
    cell* endScanPoint = CurrentHeapEnd;
    long maximumFreeSize = 0;
    long thisFreeSize;
    do {
        /* Skip over groups of live objects */
        cell *lastLive;
        while (scanner < endScanPoint && ISKEPT(*scanner)) {
            *scanner &= ~MARKBIT;
            scanner += SIZE(*scanner) + HEADERSIZE;
        }
        lastLive = scanner;
        /* Skip over all the subsequent dead objects */
        while (scanner < endScanPoint && !ISKEPT(*scanner)) {
#if INCLUDEDEBUGCODE
            if (tracememoryallocation && (TYPE(*scanner) != GCT_FREE)) {
                Log->freeObject((long)scanner,
                    (INSTANCE_CLASS)((OBJECT)(scanner + HEADERSIZE))->ofClass,
                    (long)SIZE(*scanner) + HEADERSIZE);
            }
#endif /* INCLUDEDEBUGCODE */
            scanner += SIZE(*scanner) + HEADERSIZE;
        }
        if (scanner == endScanPoint) {
            if (scanner == lastLive) {
                /* The memory ended precisely with a live object. */
                break;
            } else {
                done = TRUE;
            }
        }
        thisFreeSize = (scanner - lastLive - 1);
        newChunk = (CHUNK)lastLive;
        newChunk->size = thisFreeSize << TYPEBITS;

        *nextChunkPtr = newChunk;
        nextChunkPtr = &newChunk->next;
        if (thisFreeSize > maximumFreeSize) {
            maximumFreeSize = thisFreeSize;
        }
    } while (!done);

    *nextChunkPtr = NULL;
    *maximumFreeSizeP = maximumFreeSize;
    return firstFreeChunk;
}

#if ENABLE_HEAP_COMPACTION

static cell*
compactTheHeap(breakTableStruct *currentTable, CHUNK firstFreeChunk)
{
    cell* copyTarget = CurrentHeap; /* end of last copied object */
    cell* scanner;                  /* current object */
    int count;                      /* keeps trace of break table */
    cell* currentHeapEnd = CurrentHeapEnd; /* cache for speed */
    int lastRoll = 0;               /* value of "count" during last roll */
    CHUNK freeChunk = firstFreeChunk;

    breakTableEntryStruct *table = NULL;

    for (scanner = CurrentHeap, count = -1;  ; count++) {
        /* Skip over groups of live objects */
        cell *live, *liveEnd;

        live = scanner;
        if (freeChunk != NULL) {
            liveEnd = (cell *)freeChunk;
            scanner = liveEnd + SIZE(*liveEnd) + HEADERSIZE;
            freeChunk = freeChunk->next;
        } else {
            liveEnd = scanner = currentHeapEnd;
        }
        if (count < 0) {
            /* This is a chunk of live objects at the beginning of memory.
             * There is no need to copy them.
             */
            copyTarget = liveEnd;
        } else {
            /* The size of the chunk of live objects */
            int liveSize = PTR_DELTA(liveEnd, live);
            if (count  == 0) {
                int i;
                /* This is the first chunk of live objects to move.  There is
                 * no break table yet.  We can just move the objects into
                 * place, and indicate that the break table goes at the end.
                 */
/*                for (i = 0; i < liveSize; i += CELL) {
 *                    CELL_AT_OFFSET(copyTarget, i) = CELL_AT_OFFSET(live, i);
 *                }
 */
                memmove(copyTarget, live, liveSize);
                table = (breakTableEntryStruct*)scanner - 1;
            } else {
                /* extraSize is the total amount of dead space at the end */
                int extraSize = PTR_DELTA(scanner, liveEnd);
                /* Slide the live objects to just after copyTarget.  Move
                 * the break table out of the way, if necessary.
                 * lastRoll is set to "count" if the break table had to
                 * be gotten "out of order".
                 */
                table = slideObject(copyTarget, live, liveSize, extraSize,
                                    table, count, &lastRoll);
            }
            /* Add a new entry to the break table */
            table[count].address = live;
            table[count].offset = PTR_DELTA(live, copyTarget);

            /* And update copy target. */
            copyTarget = PTR_OFFSET(copyTarget, liveSize);
        }

        if (scanner >= currentHeapEnd) {
            if (INCLUDEDEBUGCODE && scanner > currentHeapEnd) {
                fatalVMError(KVM_MSG_SWEEPING_SCAN_WENT_PAST_HEAP_TOP);
            }
            break;
        }
    }
    if (lastRoll > 0) {
        /* The elements between 0 and lastRoll - 1 may be unsorted */
        sortBreakTable(table, lastRoll);
    }
    currentTable->table = table;
    currentTable->length = count + 1;

    /* Return the location of the first free space in memory. */
    return copyTarget;
}

static breakTableEntryStruct*
slideObject(cell* target, cell *object, int objectSize, int extraSize,
            breakTableEntryStruct *table, int tableLength, int *lastRoll)
{
    /* The size of the break table, in bytes */
    int tableSize = tableLength * sizeof(table[0]);
    int fullTableSize = tableSize + sizeof(table[0]);
    int freeSize;
    int i;

 moreFreeSpaceBeforeTable:
    /* Memory is laid out as follows:
     *    CurrentHeap
     *                        Already moved objects
     *    target
     *                        Unused #0
     *    table
     *                        Break Table
     *    table + tableSize
     *                        Unused #1
     *    object
     *                        Object to be moved (objectSize >= CELL)
     *    object + objectSize
     *                        Unused #2
     *    object + objectSize + extraSize
     *
     * The goal is to move "object" so that it begins at target.
     *
     * The break table can be relocated, if necessary, so that the object can
     * be relocated.  We must also make sure that the break table has room for
     * at least one more entry.  We return the new location of the
     * break table.
     *
     * We are guaranteed that size(Unused #0) + size(Unused #1) >= 2 * CELL,
     * so that there is sufficient space for a new break table entry.  There
     * is usually a lot more space than that.
     *
     * The first time here, we are guaranteed that objectSize >= 2*CELL.
     *
     * We maintain the following invariants:
     * Though we don't maintain that invariant, we do maintain the invariants
     *     INV0: size(Unused #0) + size(Unused #1) >= 2 * CELL
     *     INV1  objectSize >= 2 * CELL
     */

    freeSize = PTR_DELTA(table, target); /* Size of unused #0 */
    if (objectSize <= freeSize) {
        /* We can just copy the object into the space where it belongs.
         * INV1 says that there is room for the break table to expand over
         * where the object had been.
         */
/*        for (i = 0; i < objectSize; i += CELL) {
 *            CELL_AT_OFFSET(target, i) = CELL_AT_OFFSET(object, i);
 *        }
 */
        memmove(target, object, objectSize);
        return table;
    }

    /* If the expanded break table fits into extraSize, then just move it
     * there, and then copy the object to where it belongs.
     *
     * This has the added benefit of moving the break table far to end of
     * memory, where it is hopefully out of the way for subsequent moves
     */
    if (extraSize >= fullTableSize) {
        cell *newTable = PTR_OFFSET(object,
                                    objectSize + extraSize - fullTableSize);
/*        for (i = 0; i < tableSize; i += CELL) {
 *            CELL_AT_OFFSET(newTable, i) = CELL_AT_OFFSET(table, i);
 *        }
 */
        memmove(newTable, table, tableSize);
/*        for (i = 0; i < objectSize; i += CELL) {
 *            CELL_AT_OFFSET(target, i) = CELL_AT_OFFSET(object, i);
 *        }
 */
        memmove(target, object, objectSize);
        return (breakTableEntryStruct *)newTable;
    }

    /* Copy as much of the object into Unused #0 as we can.
     * Adjust object and objectSize.
     * This increases the size of Unused #1 by the size of Unused #0, while
     * setting the size of Unused #0 to 0.
     */
/*    for (i = 0; i < freeSize; i += CELL) {
 *        CELL_AT_OFFSET(target, i) = CELL_AT_OFFSET(object, i);
 *    }
 */
    memmove(target, object, freeSize);
    object = PTR_OFFSET(object, freeSize);
    objectSize -= freeSize;

    /* Set freeSize to be the space between the table and the object */
    freeSize = PTR_DELTA(object, table) - tableSize;

    /* Memory is now laid out as follows:
     *    CurrentHeap
     *                        Already moved objects
     *    table
     *                        Break Table
     *    table + tableSize
     *                        Unused #1 (size freeSize >= 2 * CELL)
     *    object
     *                        Object to be moved (objectSize >= CELL)
     *    object + objectSize
     *                        Unused #2 (size extraSize)
     *    object + objectSize + extraSize
     *
     * The break table is right up against the already moved objects.
     *
     * We now have
     *     INV0:  size(Unused #1) = freeSize >= 2 * CELL
     *     INV1a:  objectSize >= CELL
     */

    /* If the total size of the break table is <= the size of the
     * object, then perhaps we can just swap the break table with the initial
     * elements of the object.
     */
    if (fullTableSize <= objectSize) {
        breakTableEntryStruct *oldTable = table;
        breakTableEntryStruct *newTable = (breakTableEntryStruct*)object;
        for (i = 0; i < tableSize; i += CELL) {
            cell temp = CELL_AT_OFFSET(table, i);
            CELL_AT_OFFSET(table, i) = CELL_AT_OFFSET(object, i);
            CELL_AT_OFFSET(object, i) = temp;
        }
        object = PTR_OFFSET(object, tableSize);
        objectSize -= tableSize;
        /* The new value of objectSize >= fullTableSize - tableSize = 2 * CELL,
         * so we are okay with INV1.  We also still have the same amount of
         * free space as before, though it has been chopped up into pieces,
         * again.  So INV0 is fine.
         */
        target = PTR_OFFSET(oldTable, i);
        table = newTable;
        goto moreFreeSpaceBeforeTable;
    }

    /* If the total size of the new break table is greater than the size of
     * the object, we can move break table so that its right end is the
     * end of the object.
     * We copy the break table into its new location as the same time that
     * we copy the object into the space evacuated by the break table.
     *
     * We run into a problem only if the beginning of the copied break table
     * overwrites its own end, i.e.
     *   tableSize + fullTableSize <=
     *            tableSize + freeSize + objectSize
     *
     */
    if (fullTableSize > objectSize && fullTableSize <= objectSize + freeSize) {
        cell *oldTable = (cell *)table;
        cell *newTable = PTR_OFFSET(object, objectSize - fullTableSize);

        for (i = 0; i < objectSize; i += CELL) {
            CELL_AT_OFFSET(newTable, i) = CELL_AT_OFFSET(oldTable, i);
            CELL_AT_OFFSET(oldTable, i) = CELL_AT_OFFSET(object, i);
        }
        /* Do not use memmove here! Index 'i' is initialized above */
        for ( ; i < tableSize; i += CELL) {
            CELL_AT_OFFSET(newTable, i) = CELL_AT_OFFSET(oldTable, i);
        }
        return (breakTableEntryStruct*)newTable;
    }

    /* We need to roll the break table. */
    {
        cell* endTable = PTR_OFFSET(table, tableSize);
        for (i = 0; i < objectSize; i += CELL) {
            cell temp = CELL_AT_OFFSET(table, i);
            CELL_AT_OFFSET(table, i) = CELL_AT_OFFSET(object, i);
            CELL_AT_OFFSET(endTable, i) = temp;
        }
        /* Memory is now laid out as follows:
         *    CurrentHeap
         *                        Already moved objects
         *    table
         *                        Moved object
         *    table + objectSize
         *                        table
         *    table + objectSize + tableSize
         *                        Unused
         *    table + objectSize + tableSize + freeSize + extraSize;
         */
        table = PTR_OFFSET(table, objectSize);

        if (objectSize & CELL) {
            /* We did an add number of rolls.  We need to roll one more. */
            if (freeSize + extraSize > 2 * CELL) {
                /* We can just roll one more time.  Remember that we are
                 * required that there be two free words at the end of
                 * the break table when we return */
                CELL_AT_OFFSET(table, tableSize) = CELL_AT_OFFSET(table, 0);
                table = PTR_OFFSET(table, CELL);
            } else {
                /* For the following code to be executed, we must have
                 * freeSize == 2 * CELL and extraSize == 0.  This means
                 * that all of our holes have been exactly 2 cells long,
                 * and that the objects we're moving are the end of the
                 * heap, with no unmarked objects after them.
                 */
                cell temp = CELL_AT_OFFSET(table, 0);
                memmove(table, PTR_OFFSET(table, CELL), tableSize - 4);
                CELL_AT_OFFSET(table, tableSize - 4) = temp;
            }
        }
        *lastRoll = tableLength;
        return table;
    }
}

static void
sortBreakTable(breakTableEntryStruct *table, int length)
{
    /* This is straight selection sort, Knuth, 5.2.3, Algorithm S.
     * Sorting happens extremely seldom, and even when it does, it's
     * generally only the first couple of entries that need to be sorted.
     * It's just not worth it do do a guaranteed fast algorithm.
     */
    breakTableEntryStruct *currentEnd;
    breakTableEntryStruct *ptr, *maxPtr;
    breakTableEntryStruct temp;
    for (currentEnd = table + length - 1; currentEnd > table; currentEnd--) {
        /* Find the largest element between table and currentEnd, inclusive */
        maxPtr = table;
        for (ptr = table + 1; ptr <= currentEnd; ptr++) {
            if (ptr->address > maxPtr->address) {
                maxPtr = ptr;
            }
        }
        /* Swap the entries at maxPtr and currentEnd */
        temp = *currentEnd;
        *currentEnd = *maxPtr;
        *maxPtr = temp;
    }
}

static void
updatePointer(void *address, breakTableStruct *currentTable)
{
    breakTableEntryStruct *table;
    cell *value = *(cell **)address;
    int low, high, middle;

    if (value == NULL || value < CurrentHeap || value >= CurrentHeapEnd) {
        return;
    }

    low = -1;
    high = currentTable->length - 1;
    table = currentTable->table;

    while (low < high) {
        middle = (low + high + 1) >> 1;
        if (value >= table[middle].address) {
            low = middle;
        } else {
            high = middle - 1;
        }
    }

    if (low != high) {
        fatalError(KVM_MSG_BREAK_TABLE_CORRUPTED);
    }

    if (low < 0) {
        /* We are smaller than the first item in the table */
    } else {
        int offset = table[low].offset;
        cell* result = PTR_OFFSET(value, -offset);
        *(cell **)address = result;
    }
}

#endif /* ENABLE_HEAP_COMPACTION */

/*=========================================================================
 * General-purpose memory system functions
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      getHeapSize
 * TYPE:          public function
 * OVERVIEW:      Return the total amount of memory in the dynamic heap
 *                in bytes.
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     total amount of memory (in bytes)
 *=======================================================================*/

long getHeapSize(void)
{
    return VMHeapSize;
}

/*=========================================================================
 * FUNCTION:      memoryFree
 * TYPE:          public function
 * OVERVIEW:      Return the total amount of free memory in the
 *                dynamic heap in bytes.
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     total amount of free memory (in bytes)
 *=======================================================================*/

long memoryFree(void)
{
    /* Calculate the amount of memory available in the free list */
    CHUNK thisChunk = FirstFreeChunk;
    long available = 0;
    for (; thisChunk != NULL; thisChunk = thisChunk->next) {
        available += (thisChunk->size >> TYPEBITS) + HEADERSIZE;
    }
    return available * CELL;
}

/*=========================================================================
 * Debugging and printing operations
 *=======================================================================*/

#if INCLUDEDEBUGCODE
void checkHeap(void) {}
#endif

#if INCLUDEDEBUGCODE && ENABLE_HEAP_COMPACTION

/*=========================================================================
 * FUNCTION:      printBreakTable
 * TYPE:          private garbage collection debugging function
 * OVERVIEW:      Given a heap record, prints the break table
 * INTERFACE:
 *   parameters:  <nothing>
 *   returns:     <nothing>
 * COMMENTS:      Prints out the break table in each segment of the heap.
 *                Useful for debugging potential break table problems,
 *                such as incorrect entries or incorrect chaining of tables.
 *=======================================================================*/

void printBreakTable(breakTableEntryStruct *table, int length)
{
    int i;
    for (i = 0; i < length; i++) {
        fprintf(stdout, "\tObject addr %lx, address offset %ld\n",
                (long)table[i].address,
                (long)table[i].offset);
    }
}

#endif /* INCLUDEDEBUGCODE && ENABLE_HEAP_COMPACTION */

#if INCLUDEDEBUGCODE

void printHeapContents(void)
{
    cell* scanner = CurrentHeap;
    cell* heapSpaceTop = CurrentHeapEnd;
    int   objectCounter  = 0;
    int   objectSize     = 0;
    int   garbageCounter = 0;
    int   garbageSize    = 0;
    int   garbageCounterX= 0;
    int   garbageSizeX   = 0;
    int   largestFree    = 0;
    Log->startHeapScan();
    for ( ; scanner < heapSpaceTop; scanner += SIZE(*scanner) + HEADERSIZE) {
        int  size = SIZE(*scanner);
        cell *object = scanner + HEADERSIZE;
        printObject(object);
        if (TYPE(*scanner) != GCT_FREE) {
            objectCounter++;
            objectSize += size + HEADERSIZE;
        } else {
            garbageCounter++;
            garbageSize += size + HEADERSIZE;
            if (size > largestFree) {
                largestFree = size;
            }
        }
    }
    Log->endHeapScan();
    Log->heapSummary(objectCounter, garbageCounter,
                     objectSize*CELL, garbageSize*CELL,
                     largestFree*CELL, (long)CurrentHeap, (long)CurrentHeapEnd);

    /* Check that free list matches with heap contents */
    for (scanner = (cell*)FirstFreeChunk; scanner != NULL;
         scanner = (cell*)(((CHUNK)scanner)->next)) {
        garbageCounterX++;
        garbageSizeX +=  SIZE(*scanner) + HEADERSIZE;
    }
    if (garbageCounter != garbageCounterX || garbageSize != garbageSizeX) {
        Log->heapWarning((long)garbageCounterX, (long)garbageSizeX * CELL);
    }
    Log->fprintf(stdout, "====================\n\n");
}

static void
checkValidHeapPointer(cell *number)
{
    int lowbits;
    cell* scanner;

    /* We don't need to deal with NULL pointers */
    if (number == NULL || isROMString(number) || isROMClass(number)) {
        return;
    }

    /* Valid heap addresses are always four-byte aligned */
    if ((cell)number & 0x00000003) {
        fatalError(KVM_MSG_HEAP_ADDRESS_NOT_FOUR_BYTE_ALIGNED);
    }

    if (number >= CurrentHeapEnd && number < AllHeapEnd) {
        /* Object in Permanent memory */
        return;
    }

    /* The type field must contain a valid type tag; additionally, */
    /* both static bit and mark bit must be unset. */
    lowbits = OBJECT_HEADER(number);
    if (ENABLE_HEAP_COMPACTION && (lowbits & STATICBIT)) {
        fatalError(KVM_MSG_UNEXPECTED_STATICBIT);
    }
    lowbits &= 0xFC;
    if ((lowbits < (GCT_FIRSTVALIDTAG << TYPE_SHIFT)) ||
          (lowbits > (GCT_LASTVALIDTAG << TYPE_SHIFT))) {
        fatalError(KVM_MSG_BAD_GC_TAG_VALUE);
    }

    /* Now have to scan from bottom of heap */
    scanner = CurrentHeap;
    while (scanner <= number-HEADERSIZE) {
        if (number == scanner+HEADERSIZE) {
            return;
        } else {
            cell header = *scanner;
            long size;
            size = SIZE(header);
            /* Jump to the next object/chunk header in the heap */
            scanner += size+HEADERSIZE;
        }
    }

    fatalError(KVM_MSG_INVALID_HEAP_POINTER);
}

#endif /* INCLUDEDEBUGCODE */

#if ONLY_FOR_TESTING_THE_ABOVE_CODE

void rollTheHeap()
{
    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(POINTERLIST, holder,
            (POINTERLIST)callocObject(SIZEOF_POINTERLIST(40),
            GCT_POINTERLIST));

        int i;
        cell *bad, *good;

        holder->length = 40;
        garbageCollect(0);

        for (i = 0; i < 20; i++) {
            bad  = callocObject(1, GCT_NOPOINTERS);
            good = callocObject((i / 3) + 1, GCT_NOPOINTERS);
            good[0] = (i + 1) * 100;
            holder->data[i].cellp = good;
        }

        for (i = 20; i < 40; i++) {
            bad  = callocObject(100, GCT_NOPOINTERS);
            good = callocObject(100, GCT_NOPOINTERS);
            good[0] = (i + 1) * 100;
            holder->data[i].cellp = good;
        }

        garbageCollect(0);

        for (i = 0; i < 40; i++) {
            cell *good = holder->data[i].cellp;
            fprintf(stdout, "%d: %ld\n", i, good[0]);
        }
    END_TEMPORARY_ROOTS
}

#endif /* ONLY_FOR_TESTING_THE_ABOVE_CODE */

