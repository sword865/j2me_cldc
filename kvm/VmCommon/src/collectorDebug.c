/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Memory management
 * FILE:      collectorDebug.c
 * OVERVIEW:  Debug version of the garbage collector.  This
 *            implementation can be used in place of the standard
 *            garbage collector to find spurious pointers elsewhere
 *            in the codebase.  This implementation is particularly
 *            useful if you add your own native functions to the KVM
 *            and you want to make sure that your functions access
 *            memory correctly. Be sure to turn the EXCESSIVE_GARBAGE-
 *            COLLECTION mode on to find bugs fast.
 *
 *            The code here also includes an implementation of the
 *            classic Cheney two-space copying collector.  This
 *            collector is very fast and simple, but it requires two
 *            semi-spaces (two memory areas of the same size) to operate.
 * AUTHOR:    Frank Yellin
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

/* The number of separate memory spaces */
#define SPACE_COUNT 10

#define PERMANENT_MEMORY_SIZE 1024 * 1024
#define MEMORY_SIZE 256 * 1024

#define OBJECT_HEADER(object) ((cell *)(object))[-HEADERSIZE]

#define inTargetSpace(object) \
  (((cell *)(object) >= TargetSpace) && (((cell *)(object) < TargetSpaceFreePtr)))

#define OVERWRITE_FREE_MEMORY (ENABLEPROFILING || INCLUDEDEBUGCODE)
#define OVERWRITE_FREE_MEMORY_BYTE 0xF1
#define OVERWRITE_FREE_MEMORY_WORD 0xF1F1F1F1

#define PTR_OFFSET(ptr, offset) ((void *)((char *)ptr + offset))
#define PTR_DELTA(ptr1, ptr2) ((char *)(ptr1) - (char *)ptr2)

/*=========================================================================
 * Variables
 *=======================================================================*/

cell* AllHeapStart;       /* Start of first heap */
cell* CurrentHeap;        /* Bottom of current heap */
cell* CurrentHeapEnd;     /* End of current heap */
cell* CurrentHeapFreePtr; /* Next allocation in current space */

cell* PermanentSpace;     /* Beginning of permanent space */
cell* PermanentSpaceFreePtr;
cell* AllHeapEnd;

WEAKPOINTERLIST WeakPointers;   /* List of weak pointers during GC */
WEAKREFERENCE   WeakReferences; /* List of weak refs during GC */

static cell* TargetSpace;
static cell* TargetSpaceFreePtr;

long nHeapSize;           /* Size of each heap */

#if INCLUDEDEBUGCODE
static void checkValidHeapPointer(cell *number);
int NoAllocation = 0;
#else
#define checkValidHeapPointer(x)
#define NoAllocation 0
#endif

#if ASYNCHRONOUS_NATIVE_FUNCTIONS
#include <async.h>
extern ASYNCIOCB IocbRoots[];
#endif

/*=========================================================================
 * Static functions (private to this file)
 *=======================================================================*/

static void updatePointer(void *ptr);
static void updateMonitor(OBJECT object);
static void copyRootObjects(void);
static void copyNonRootObjects(void);
static void copyWeakPointerLists(void);
static void copyWeakReferences(void);
static void updateThreadAndStack(THREAD thisThread);
static void expandPermanentMemory(cell *newPermanentSpace);

/*=========================================================================
 * Heap initialization operations
 *=======================================================================*/

#if CHENEY_TWO_SPACE
void* TheHeap;
#endif;

void InitializeHeap(void)
{
#if CHENEY_TWO_SPACE
    long VMHeapSize = RequestedHeapSize * 2;

    AllHeapStart = allocateHeap(&VMHeapSize, &TheHeap);
    if (TheHeap == NIL) {
        fatalVMError(KVM_MSG_NOT_ENOUGH_MEMORY);
    }

    /* Each space is 45% of the total, rounded to a multiple of 32 */
    nHeapSize = (((VMHeapSize * 9) / 20) >> 5) << 5;

    PermanentSpace = PTR_OFFSET(AllHeapStart, 2 * nHeapSize);
    AllHeapEnd     = PTR_OFFSET(AllHeapStart, VMHeapSize);

#else
    nHeapSize = MEMORY_SIZE;
    AllHeapStart   = allocateVirtualMemory_md(MEMORY_SIZE * SPACE_COUNT
                                            + PERMANENT_MEMORY_SIZE);
    PermanentSpace = PTR_OFFSET(AllHeapStart, MEMORY_SIZE * SPACE_COUNT);
    AllHeapEnd     = PTR_OFFSET(PermanentSpace, PERMANENT_MEMORY_SIZE);

#endif /* CHENEY_TWO_SPACE */

    /* We start in the first semi space */
    CurrentHeap        = AllHeapStart;
    CurrentHeapFreePtr = AllHeapStart;
    CurrentHeapEnd     = PTR_OFFSET(CurrentHeap, nHeapSize);

    /* Permanent space grows from the end toward the middle; */
    PermanentSpaceFreePtr = AllHeapEnd;

    memset((char *)PermanentSpace, 0, PTR_DELTA(AllHeapEnd, PermanentSpace));

#if !CHENEY_TWO_SPACE
    /* Protect all of the heaps except for the current one */
    protectVirtualMemory_md(CurrentHeapEnd, MEMORY_SIZE * (SPACE_COUNT - 1),
                            PVM_NoAccess);
#endif    

#if INCLUDEDEBUGCODE
    if (tracememoryallocation) {
        Log->allocateHeap(MEMORY_SIZE, (long)CurrentHeap, (long)CurrentHeapEnd);
    }
    NoAllocation = 0;
#endif

}

void FinalizeHeap(void)
{
#if CHENEY_TWO_SPACE
    freeHeap(TheHeap);
#else
    freeVirtualMemory_md(AllHeapStart,
                         MEMORY_SIZE * SPACE_COUNT + PERMANENT_MEMORY_SIZE);
#endif
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
 *                may contain invalid heap references.
 *=======================================================================*/

cell* mallocHeapObject(long size, GCT_ObjectType type)
/* Remember: size is given in CELLs rather than bytes */
{
    cell* thisChunk;
    long realSize;

    realSize = size + HEADERSIZE;

#if INCLUDEDEBUGCODE
    if (NoAllocation > 0) {
        fatalError(KVM_MSG_CALLED_ALLOCATOR_WHEN_FORBIDDEN);
    }
#endif /* INCLUDEDEBUGCODE */

    if (EXCESSIVE_GARBAGE_COLLECTION && CurrentHeapFreePtr > CurrentHeap) {
        garbageCollect(0);
    }

    if (CurrentHeapEnd - CurrentHeapFreePtr < realSize) {
        garbageCollect(0);
        if (CurrentHeapEnd - CurrentHeapFreePtr < realSize) {
            return NULL;
        }
    }
    thisChunk = CurrentHeapFreePtr;
    CurrentHeapFreePtr += realSize;

    /* Augment the object header with gc type information */
    *thisChunk = (type << TYPE_SHIFT) + (size << TYPEBITS);

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
    cell* result = PermanentSpaceFreePtr - size;
    PermanentSpaceFreePtr = result;

    if (result < PermanentSpace) {
        /* We have to expand permanent memory */
        cell* temp = PermanentSpace;
        do {
            temp = PTR_OFFSET(temp, -0x800);
        } while (temp > result);
        expandPermanentMemory(temp);
    }
    return result;
}

/*=========================================================================
 * Garbage collection operations
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
garbageCollectForReal(int moreMemory)
{
    TargetSpace = (CurrentHeapEnd == PermanentSpace)
                                ? AllHeapStart : CurrentHeapEnd;
    TargetSpaceFreePtr = TargetSpace;

#if !CHENEY_TWO_SPACE
    protectVirtualMemory_md(TargetSpace, MEMORY_SIZE, PVM_ReadWrite);
#endif

    /* The actual high-level GC algorithm is here */
    WeakPointers = NULL;
    WeakReferences = NULL;
    copyRootObjects();

    /* Mark all the objects pointed at by the root objects, or pointed at
     * by objects that are pointed at by root objects. . . .
     */
#if INCLUDEDEBUGCODE
    if (tracegarbagecollectionverbose) {
        Log->fprintf(stdout, "Scanning heap for additional objects\n");
    }
#endif

    copyNonRootObjects();
    copyWeakPointerLists();
    copyWeakReferences();

#if !CHENEY_TWO_SPACE
    protectVirtualMemory_md(CurrentHeap, MEMORY_SIZE, PVM_NoAccess);
#endif

    CurrentHeap        = TargetSpace;
    CurrentHeapFreePtr = TargetSpaceFreePtr;
    CurrentHeapEnd     = PTR_OFFSET(CurrentHeap, nHeapSize);
}

static void
expandPermanentMemory(cell *newPermanentSpace) {
#if CHENEY_TWO_SPACE
    int newHeapBytes = PTR_DELTA(newPermanentSpace, AllHeapStart);
    int newHeapSize  = newHeapBytes / 2;
    cell *newHeapSpace1 = PTR_OFFSET(AllHeapStart, newHeapSize);
    int count;

    for (count = 0; ; count++) {
        if (CurrentHeap == AllHeapStart) {
            /* We're currently allocating into HeapSpace 0 */
            if (CurrentHeapFreePtr <= newHeapSpace1) {
                /* We can just adjust the pointers */
                CurrentHeapEnd = newHeapSpace1;
                break;
            } else if (count == 0) {
                /* Try compacting the heap to see if that helps */
                garbageCollect(-1);
            } else {
                /* We've already garbage collected, so we can't compact the
                 * heap any more */
                raiseExceptionWithMessage(OutOfMemoryError,
                    KVM_MSG_UNABLE_TO_EXPAND_PERMANENT_MEMORY);
            }
        } else {
            /* We're currently allocating into HeapSpace 1 */
            if (CurrentHeapFreePtr < newPermanentSpace) {
                /* Let's put a fake object to fill up the space between
                 * the old pointer and the new, just in case we decide to
                 * scan memory */
                int delta = (CurrentHeap - newHeapSpace1);
                *(newHeapSpace1) =
                    (GCT_NOPOINTERS << 2) + ((delta - 1) << TYPEBITS);
                CurrentHeap = newHeapSpace1;
                CurrentHeapEnd = newPermanentSpace;
                break;
            } else {
                garbageCollect(0);
            }
        }
    }
    /* Zero then new memory */
    memset((char *)newPermanentSpace, 0,
           PTR_DELTA(PermanentSpace, newPermanentSpace));
    nHeapSize = newHeapSize;
    PermanentSpace = newPermanentSpace;
#else
    raiseExceptionWithMessage(OutOfMemoryError,
        KVM_MSG_UNABLE_TO_EXPAND_PERMANENT_MEMORY);
#endif /* CHENEY_TWO_SPACE */
}

static void
copyRootObjects(void)
{
    cellOrPointer *ptr, *endptr;
    HASHTABLE stringTable;

    ptr = &GlobalRoots[0];
    endptr = ptr + GlobalRootsLength;
    for ( ; ptr < endptr; ptr++) {
        updatePointer(ptr->cellpp);
    }

    ptr = &TemporaryRoots[0];
    endptr = ptr + TemporaryRootsLength;
    for ( ; ptr < endptr; ptr++) {
        cellOrPointer location = *ptr;
        if (location.cell == -1) {
            /* Actual Location is ptr[1], base is ptr[2] */
            long offset = *(ptr[1].charpp) - ptr[2].charp;
            updatePointer(&ptr[2]);
            *(ptr[1].charpp) = ptr[2].charp + offset;
            ptr += 2;
        } else {
            updatePointer(location.cellpp);
        }
    }

#if ASYNCHRONOUS_NATIVE_FUNCTIONS
{
    int i;
    for (i = 0 ; i < ASYNC_IOCB_COUNT ; i++) {
        ASYNCIOCB *aiocb = &IocbRoots[i];
        updatePointer(&aiocb->thread);
        updatePointer(&aiocb->instance);
        updatePointer(&aiocb->array);
    }
}
#endif

#if ROMIZING
    {
        /* In RELOCATABLE_ROM builds, we have a pointer to the static data.
         * In !RELOCATABLE_ROM builds, we have the actual array.
         */
#if RELOCATABLE_ROM
        cell **staticPtrData = (cell **)KVM_staticDataPtr;
#else
        cell **staticPtrData = (cell **)KVM_staticData;
#endif
        int refCount = ((int *)staticPtrData)[0];
        cell **staticPtr = staticPtrData + 1;
        cell **lastStaticPtr = staticPtr + refCount;
        while (staticPtr < lastStaticPtr) {
            updatePointer(staticPtr);
            staticPtr++;
        }
    }
#endif /* ROMIZING */

    if (ROMIZING || ClassTable != NULL) {
        FOR_ALL_CLASSES(clazz)
            updateMonitor((OBJECT)clazz);
            if (!IS_ARRAY_CLASS(clazz)) {
                INSTANCE_CLASS iclazz = (INSTANCE_CLASS)clazz;
                POINTERLIST statics = iclazz->staticFields;
                THREAD initThread = iclazz->initThread;

                if (initThread != NULL) {
                    updatePointer(&initThread);
                    setClassInitialThread(iclazz, initThread);
                }

                if (clazz->accessFlags & ACC_ROM_CLASS) {
                    continue;
                }

                if (USESTATIC) {
                    updatePointer(&iclazz->constPool);
                    updatePointer(&iclazz->ifaceTable);
                    updatePointer(&iclazz->fieldTable);
                    updatePointer(&iclazz->methodTable);
                }

                if (statics != NULL) {
                    int count = statics->length;
                    while (--count >= 0) {
                        updatePointer(&statics->data[count].cellp);
                    }
                }

                if (iclazz->status == CLASS_VERIFIED) {
                    continue;
                }

                FOR_EACH_METHOD(thisMethod, iclazz->methodTable)
                    /* Mark the bytecode object alive for non-native methods */
                    if (!(thisMethod->accessFlags & ACC_NATIVE)) {
                        if (!USESTATIC || inTargetSpace(thisMethod)) {
                            updatePointer(&thisMethod->u.java.stackMaps.verifierMap);
                        } else {
                            POINTERLIST oldValue = thisMethod->u.java.stackMaps.verifierMap;
                            POINTERLIST newValue = oldValue;
                            updatePointer(&newValue);
                            if (oldValue != newValue) {
                                CONSTANTPOOL cp = iclazz->constPool;
                                int offset = PTR_DELTA(
                                       &thisMethod->u.java.stackMaps.pointerMap,
                                       cp);
                                modifyStaticMemory(cp, offset, &newValue, sizeof(POINTERLIST));
                            }
                        }
                    }
                END_FOR_EACH_METHOD
            }
        END_FOR_ALL_CLASSES
    }

    stringTable = InternStringTable;
    if (ROMIZING || stringTable != NULL) {
        int count = stringTable->bucketCount;
        while (--count >= 0) {
            INTERNED_STRING_INSTANCE instance =
                (INTERNED_STRING_INSTANCE)stringTable->bucket[count];
            for ( ; instance != NULL; instance = instance->next) {
                updateMonitor((OBJECT)instance);
            }
        }
    }
}

static void
copyNonRootObjects()
{
    cell* scanner;
    for (   scanner = TargetSpace;
            scanner < TargetSpaceFreePtr;
            scanner += SIZE(*scanner) + HEADERSIZE) {
        cell *header = scanner;
        cell *object = scanner + 1;
        GCT_ObjectType gctype = TYPE(*header);
        switch (gctype) {
            int length;
            cell **ptr;

        case GCT_INSTANCE: {
            /* The object is a Java object instance.  Mark pointer fields */
            INSTANCE instance = (INSTANCE)object;
            INSTANCE_CLASS clazz = instance->ofClass;
            updateMonitor((OBJECT)instance);
            while (clazz) {
                FOR_EACH_FIELD(thisField, clazz->fieldTable)
                    /* Is this a non-static pointer field? */
                    if ((thisField->accessFlags & (ACC_POINTER | ACC_STATIC))
                               == ACC_POINTER) {
                        updatePointer(&instance->data[thisField->u.offset].cellp);
                    }
                END_FOR_EACH_FIELD
                clazz = clazz->superClass;
            }
            break;
        }

        case GCT_ARRAY: {
            /* The object is a Java array with primitive values. */
            /* Only the possible monitor will have to be marked alive. */
            updateMonitor((OBJECT)object);
        }
        break;

        case GCT_POINTERLIST: {
            POINTERLIST list = (POINTERLIST)object;
            length = list->length;
            ptr = &list->data[0].cellp;
            goto markArray;
        }

        case GCT_WEAKPOINTERLIST:
            /* Push this object onto the linked list of weak pointers */
            ((WEAKPOINTERLIST)object)->gcReserved = WeakPointers;
            WeakPointers = (WEAKPOINTERLIST)object;
            break;

        case GCT_OBJECTARRAY: {
            /* The object is a Java array with object references. */
            /* The contents of the array and the possible monitor  */
            /* will have to be scanned. */
            ARRAY array = (ARRAY)object;
            updateMonitor((OBJECT)array);

            length = array->length;
            ptr = &array->data[0].cellp;
            /* FALL THROUGH */
        }

        markArray:
            /* Keep objects in the array alive. */
            while (--length >= 0) {
                updatePointer(ptr);
                ptr++;
            }
            break;

        /* Added for java.lang.ref.WeakReference support in CLDC 1.1 */
        case GCT_WEAKREFERENCE: {
            /* Keep the possible monitor object alive */
            updateMonitor((OBJECT)object);

            /* Push this object onto the linked list of weak refs */
            ((WEAKREFERENCE)object)->gcReserved = WeakReferences;
            WeakReferences = (WEAKREFERENCE)object;

            /* NOTE: We don't copy the 'referent' field of the  */
            /* weak reference object here, because we want to */
            /* keep the referent alive only if there are */
            /* strong references to it. */
            break;
        }

        case GCT_MONITOR: {
            MONITOR monitor = (MONITOR)object;
            updatePointer(&monitor->owner);
            updatePointer(&monitor->monitor_waitq);
            updatePointer(&monitor->condvar_waitq);
#if INCLUDEDEBUGCODE
            updatePointer(&monitor->object);
#endif
            break;
        }

        case GCT_THREAD: {
            THREAD thread  = (THREAD)object;
            updatePointer(&thread->nextAliveThread);
            updatePointer(&thread->nextThread);
            updatePointer(&thread->javaThread);
            updatePointer(&thread->monitor);
            updatePointer(&thread->nextAlarmThread);
            updatePointer(&thread->stack);
            if (thread->fpStore != NULL) {
                updateThreadAndStack(thread);
            }
            break;
        }

        case GCT_METHODTABLE:
            FOR_EACH_METHOD(thisMethod, ((METHODTABLE)object))
                if ((thisMethod->accessFlags & ACC_NATIVE) == 0) {
                    updatePointer(&thisMethod->u.java.code);
                    updatePointer(&thisMethod->u.java.handlers);
                }
            END_FOR_EACH_METHOD
            break;

        case GCT_NOPOINTERS:
            break;

        case GCT_EXECSTACK:
            /* This is ugly, but it gets handled by GCT_THREAD; */
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
copyWeakPointerLists()
{
    WEAKPOINTERLIST list;

    /* save the current native method pointer */
    cell* currentNativeLp;
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
                if (ISMARKED((object)[-HEADERSIZE])) {
                    ptr->cellp = (cell *)(((object)[-HEADERSIZE]) & ~MARKBIT);
                } else {
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
 * FUNCTION:      copyWeakReferences
 * OVERVIEW:      Handle the weak reference objects (CLDC 1.1).
 *                The 'referent' field of a weak reference object
 *                needs to be cleared if the referent object is
 *                not referred to by at least one strong
 *                reference.
 * INTERFACE:
 *   parameters:  <none>
 * NOTE:          The name of this routine is a bit misleading, since
 *                we aren't really "copying" anything.  Rather, we are
 *                clearing references to those objects that are referred
 *                to by weak references only.
 *=======================================================================*/

static void
copyWeakReferences()
{
    WEAKREFERENCE thisRef;

    /* Walk through the list of weak reference objects that was */
    /* created earlier during the copyNonRootObjects phase.  This list */
    /* contains all the weak reference objects that exist in the heap */
    for (thisRef = WeakReferences; thisRef != NULL; thisRef = thisRef->gcReserved) {

        /* If the referent object is not marked, clear the weak reference */
        cell* referent = (cell*)thisRef->referent;
        if (referent != NULL && ISMARKED((referent)[-HEADERSIZE])) {
            /* Weak reference should be kept alive: update pointer */
            updatePointer(&thisRef->referent);
            
        } else {
            /* No strong references found: zap weak pointer to NULL */
            thisRef->referent = NULL;
        }
    }
}

static void updateThreadAndStack(THREAD thread) {
    /* Note that when this is called, some (or all) of the thread stacks
     * may have already been copied into new space.  However none of the
     * stacks, themselves, have been scanned.
     */
    char   map[(MAXIMUM_STACK_AND_LOCALS + 7) >> 3];
    long   delta;
    FRAME  thisFP;
    cell*  thisSP;
    BYTE*  thisIP;
    cell*  thisNativeLP;
    STACK  oldStack, newStack, stack;
    unsigned int i;

    oldStack = stack = thread->fpStore->stack;
    updatePointer(&stack);
    newStack = stack;
    delta = PTR_DELTA(newStack, oldStack);

    thisIP       = thread->ipStore;
    thisSP       = PTR_OFFSET(thread->spStore,  delta); /* new space */
    thisFP       = PTR_OFFSET(thread->fpStore,  delta); /* new space */
    thisNativeLP = PTR_OFFSET(thread->nativeLp, delta); /* new space */

    thread->spStore  = thisSP;
    thread->fpStore  = thisFP;
    thread->nativeLp = thisNativeLP;

#if ENABLE_JAVA_DEBUGGER
    if (thread->stepInfo.fp != 0)
        thread->stepInfo.fp = PTR_OFFSET(thread->stepInfo.fp, delta);
#endif

    newStack->next = NULL;

    for (;;) {
        /* thisSP and thisFP are pointers to the current frame
         *       in new space.
         * oldStack and newStack contain the old and new addresses of the
         *       stack containing the current frame.
         */
        METHOD method = thisFP->thisMethod;
        cell* localVars = FRAMELOCALS(thisFP);
        cell *operandStack = (cell*)thisFP + SIZEOF_FRAME;
        unsigned int localsCount = method->frameSize;
        long realStackSize = thisSP - (cell*)thisFP - SIZEOF_FRAME + 1;
        unsigned int totalSize = realStackSize + localsCount;
        FRAME previousFp;

        /* Mark the possible synchronization object alive */
        updatePointer(&thisFP->syncObject);

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
                cell **argP;
                if (i < localsCount) {
                    argP = (cell **)&localVars[i];
                } else {
                    argP = (cell **)&operandStack[i - localsCount];
                }
                if (INCLUDEDEBUGCODE && method != RunCustomCodeMethod) {
                    checkValidHeapPointer(*argP);
                }
                updatePointer(argP);
            }
        }
        if (INCLUDEDEBUGCODE && thisFP->stack != oldStack) {
            fatalError(KVM_MSG_BAD_STACK_INFORMATION);
        }
        thisFP->stack = newStack;

        previousFp = thisFP->previousFp; /* address in old space */
        if (previousFp == NULL) {
            break;
        } else {
            STACK prevOldStack = previousFp->stack; /* old space */
            STACK prevNewStack;
            if (prevOldStack == oldStack) {
                thisFP->previousSp = PTR_OFFSET(thisFP->previousSp, delta);
                thisFP->previousFp = PTR_OFFSET(thisFP->previousFp, delta);
            } else {
                long offset1 = PTR_DELTA(thisFP->previousFp, prevOldStack);
                long offset2 = PTR_DELTA(thisFP->previousSp, prevOldStack);
                stack = prevOldStack;
                updatePointer(&stack); /* use temp, since non-register var */
                prevNewStack = stack;
                thisFP->previousFp = PTR_OFFSET(prevNewStack, offset1);
                thisFP->previousSp = PTR_OFFSET(prevNewStack, offset2);
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

static void
updatePointer(void *ptr)
{
    cell *value = *(cell **)ptr;
    if (value == NULL) {
        return;
    } else if (!inCurrentHeap(value)) {
        if (INCLUDEDEBUGCODE) {
            if (value >= AllHeapStart && value < PermanentSpace) {
                fatalError(KVM_MSG_POINTER_IN_OLD_SPACE);
            }
        }
        return;
    } else {
        cell valueHeader = OBJECT_HEADER(value);
        cell *result;
        if (valueHeader & MARKBIT) {
            result = (cell *)(valueHeader & ~MARKBIT);
        } else {
            long size = SIZE(valueHeader) + 1;
            result = TargetSpaceFreePtr + 1;
            /* checkValidHeapPointer(value); */
            memcpy(TargetSpaceFreePtr, value - 1, size  << 2);
            OBJECT_HEADER(value) = (long)result | MARKBIT;
            TargetSpaceFreePtr += size;
        }
        *(cell **)ptr = result;
        return;
    }
}

static void
updateMonitor(OBJECT object)
{
    if (OBJECT_HAS_MONITOR(object)) {
        /* The mhc slot contains either a MONITOR or a THREAD,
         * incremented by a small constant, so we will be pointing into the
         * middle of the first word.
         */
        int tag = OBJECT_MHC_TAG(object);
        char *address = (char *)object->mhc.address - tag; /* thread/monitor */
        updatePointer((cell **)&address);
        SET_OBJECT_MHC_ADDRESS(object, (char *)address + tag);
    }
}

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
    return PTR_DELTA(CurrentHeapEnd, CurrentHeap);
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
    return PTR_DELTA(CurrentHeapEnd, CurrentHeapFreePtr);
}

/*=========================================================================
 * FUNCTION:      memoryUsage
 * TYPE:          public function
 * OVERVIEW:      Return the amount of memory currently consumed by
 *                all the heap objects.
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     amount of consumed memory (in bytes).
 *=======================================================================*/

long memoryUsage(void)
{
    return PTR_DELTA(CurrentHeapFreePtr, CurrentHeap);
}

/*=========================================================================
 * FUNCTION:      largestFree
 * TYPE:          public function
 * OVERVIEW:      Return the largest block of free memory in the
 *                dynamic heap in bytes.
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     size of largest free block (in bytes)
 *=======================================================================*/

long largestFree(void)
{
    return PTR_DELTA(CurrentHeapEnd, CurrentHeapFreePtr);
}

/*=========================================================================
 * Debugging and printing operations
 *=======================================================================*/

#if INCLUDEDEBUGCODE

void printHeapContents(void)
{
    cell* scanner = CurrentHeap;
    cell* heapSpaceFreePtr = CurrentHeapFreePtr;
    int   freeBytes = PTR_DELTA(CurrentHeapEnd, CurrentHeapFreePtr);
    int   objectCounter  = 0;
    int   objectSize     = 0;
    int   size;
    Log->startHeapScan();
    for (; scanner < heapSpaceFreePtr; scanner += size + HEADERSIZE) {
        cell header = *scanner;
        cell *object;
        if (header & MARKBIT) {
            object = (cell *)(header & ~MARKBIT);
            header = OBJECT_HEADER(object);
            Log->fprintf(stdout, "%lx => %lx\n\t",
                         (long)(scanner + 1), (long)object);
            printObject(object);
        } else {
            object = scanner + 1;
            printObject(object);
        }
        size = SIZE(header);
        objectCounter++;
        objectSize += size + HEADERSIZE;
    }
    Log->endHeapScan();
    Log->heapSummary(objectCounter, 1,
                     objectSize*CELL, freeBytes, freeBytes,
                     (long)CurrentHeap, (long)CurrentHeapEnd);
    Log->fprintf(stdout, "====================\n\n");
}

void checkHeap(void)
{
    /* Scan the entire heap, looking for badly formed headers */
    cell* scanner = CurrentHeap;
    cell* heapSpaceEnd = CurrentHeapFreePtr;
    int result = TRUE;

    while (scanner < heapSpaceEnd) {
        long size = SIZE(*scanner);
        GCT_ObjectType type = TYPE(*scanner);

        /* In this GC, we don't have FREE chunks or marked objects. */
        if (   ISFREECHUNK(*scanner) ||  ISMARKED(*scanner)
            || type < GCT_FIRSTVALIDTAG || type > GCT_LASTVALIDTAG) {
            result = FALSE;
            break;
        }

        /* Jump to the next object/chunk header in the heap */
        scanner += size + HEADERSIZE;
    }

    /* The scanner should now be pointing to exactly the heap top */
    if (!result || scanner != heapSpaceEnd) {
        fatalVMError(KVM_MSG_HEAP_CORRUPTED);
    }
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
    if (number >= PermanentSpaceFreePtr && number < AllHeapEnd) {
        return;
    }
    /* The type field must contain a valid type tag; additionally, */
    /* both static bit and mark bit must be unset. */
    lowbits = OBJECT_HEADER(number);
    if (lowbits & STATICBIT) {
        fatalError(KVM_MSG_UNEXPECTED_STATICBIT);
    }
    if (lowbits & MARKBIT) {
        cell *movedObject = (cell *)(lowbits & ~MARKBIT);
        lowbits = OBJECT_HEADER(movedObject);
    }
    lowbits &= 0xFF;
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
            if (header & MARKBIT) {
                cell *movedObject = (cell *)(header & ~MARKBIT);
                header = OBJECT_HEADER(movedObject);
            }
            /* Get the size of the object (excluding the header) */
            size = SIZE(header);

            /* Jump to the next object/chunk header in the heap */
            scanner += size+HEADERSIZE;
        }
    }
    fatalError(KVM_MSG_INVALID_HEAP_POINTER);
}

#endif /* INCLUDEDEBUGCODE */

