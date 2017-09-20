/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Memory management
 * FILE:      garbage.h
 * OVERVIEW:  Memory manager/garbage collector for the KVM.
 * AUTHOR:    Antero Taivalsaari, Sun Labs, 1998
 *            Edited by Doug Simon 11/1998
 *            Frank Yellin (exact collection, compaction)
 *            Compaction based on code written by Matt Seidl
 *            Weak reference support for CLDC 1.1, A. Taivalsaari 3/2002
 *=======================================================================*/

/*=========================================================================
 * NOTE (KVM 1.0.3):
 * The garbage collector documentation below has been revised
 * in KVM 1.0.3.  Apart from some bug fixes, the only difference
 * between KVM 1.0.2 and 1.0.3 garbage collectors is the addition
 * of the new native cleanup mechanism defined at the bottom of
 * this file. Also, the new monitor/synchronization code in 
 * KVM 1.0.3 has caused some minor changes in the garbage collector.
 *=======================================================================*/

/*=========================================================================
 * COMMENTS ON GARBAGE COLLECTOR IN KVM:
 * The original KVM (Spotless) garbage collector was based on Cheney's
 * elegant copying collector presented in Communications of the ACM
 * in September 1970.  Although that collector has a lot of benefits
 * (in particular, it uses an iterative rather than recursive algorithm
 * for collecting objects), it has the general problem of copying
 * collectors in that it requires double the memory space than the
 * program is actually using.  That requirement was problematic
 * when porting the KVM to run on devices with very limited
 * memory such as the PalmPilot.
 *
 * In order to make the KVM more suitable to small
 * devices, a new garbage collector was implemented for KVM 1.0.
 * This collector was based on a straightforward, handle-free,
 * non-moving, mark-and-sweep algorithm. Since the KVM 1.0
 * collector did not move objects, it was somewhat simpler
 * than the original Spotless collector. However, since the 
 * KVM 1.0 collector maintained free lists of memory rather
 * than copying and/or compacting memory, object allocation
 * was not as fast as in the original Spotless collector.
 * Also, memory fragmentation could cause the the VM run
 * out of memory more easily than with a copying or compacting
 * collector.
 *
 * In KVM 1.0.2, a new compacting garbage collector,
 * fully precise ("exact") collector was added. 
 * Most of the documentation below has now been updated
 * to reflect the features of the 1.0.2/1.0.3 collector.
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

/*=========================================================================
 * Header word definitions and structures
 *=======================================================================*/

/*=========================================================================
 * OBJECT STRUCTURES:
 * In our VM, each heap-allocated structure in memory is preceded with
 * an object header that provides detailed information on the type and
 * size of the object. The length of the header is one word (32 bits).
 *
 * The basic heap object structure is as follows:
 *
 *                   +---------------+
 *                   ! gc header     ! (object length, type and other info)
 *                   +---------------+
 * Object pointer -> ! class ptr     !    ^
 *                   +---------------+    !
 *                   | mhc field     !    !  (mhc = monitorOrHashCode)
 *                   +---------------+
 *                   .               .    !
 *                   .               .  data
 *                   .               .    !
 *                   !               !    !
 *                   +---------------+    !
 *                   !               !    v
 *                   +---------------+
 *
 * IMPORTANT:
 * Note that there are no object handles in our VM. Thus, unlike in
 * many other JVM implementations, all the memory references are
 * direct rather than indirect.
 *
 * It is important to notice that references are only allowed to
 * memory locations that are immediately preceded by an object header;
 * this means that you may not create a pointer that refers, e.g., to
 * data fields of an object directly. This restriction simplifies the
 * garbage collection process substantially (but could be removed
 * relatively easily if necessary).
 *=========================================================================
 * OBJECT HEADERS:
 * An object header is a 32-bit structure that contains important
 * administrative information. In particular, the header stores
 * the length of the object, the type of the object and two
 * bits for administrative purposes.
 *                                                             +-- Static bit
 *                                                             | +-- Mark bit
 * <------------------ length -------------------> <-- type--> v v
 *|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|
 *| | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |0|0|
 *|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|-+-+-+-+-+-+-+-|
 *
 * Length holds the length of the object in cells, excluding the
 * object header. For instance, length = 1 means that the data
 * area of the object is 4 bytes.  The minimum physical size of
 * an object is 8 bytes (4 byte header + 4 byte data).
 *
 * Type field holds the garbage collection type of the object,
 * instructing the garbage collector to scan the data fields of
 * the object correctly upon garbage collection. Garbage collection
 * types (GCT_*) are defined below.
 *
 * Static bit is unused these days except on the Palm.  In the 
 * original KVM (Spotless) collector this bit was used for
 * denoting that an object was allocated in the static heap,
 * i.e., the object was immovable.
 *
 * Mark bit is used for marking live (non-garbage) objects during
 * garbage collection. During normal program execution this bit
 * should always be 0 in every heap object.
 *=======================================================================*/

#define MARKBIT         0x00000001
#define STATICBIT       0x00000002
#define TYPEMASK        0x000000FC

/*  HEADER */
/*  struct headerStruct { */
/*  int size      :24; */
/*  int type      :6; */
/*  int markBit   :1; */
/*  int staticBit :1; */
/*  }; */

/*  The amount of bits that will have to be shifted away
 *  in order to read the object length correctly
 */

#define inAnyHeap(ptr) \
     (((cell *)ptr >= AllHeapStart) && ((cell *)ptr < AllHeapEnd))

#define inCurrentHeap(ptr) \
      (((cell *)ptr >= CurrentHeap) && ((cell *)ptr < CurrentHeapEnd))

#define TYPEBITS        8

/* The number of bits that we have to shift to get the type */
#define TYPE_SHIFT      2

/*=========================================================================
 * Operations on header words
 *=======================================================================*/

#define ISMARKED(n)     ((n) & MARKBIT)
#define ISSTATIC(n)     ((n) & STATICBIT)
#define SIZE(n)         (((cell)(n)) >> TYPEBITS)
#define TYPE(n)         (GCT_ObjectType)(((n) & TYPEMASK) >> TYPE_SHIFT)
#define ISFREECHUNK(n)  (((n) & (TYPEMASK | MARKBIT | STATICBIT)) == 0)

/*  Header is 1 word long */
#define HEADERSIZE      1

/*=========================================================================
 * FREE LIST STRUCTURES:
 * The garbage collector maintains a free list of available memory.
 * Every available memory chunk is preceded by a free list header with
 * the following information:
 *
 *     +--------------+
 *     !     size     ! (the amount of memory in this chunk)
 *     +--------------+
 *     !     next     ! (pointer to the next chunk in the free list)
 *     +--------------+
 *
 * The size information is stored in cells (32 bit words). The size
 * information is stored in the same format as in regular object headers,
 * i.e., only the highest 24 bits are used (see above). The 6 low bits
 * must all be zero for a word to be recognized as a chunk header. The size
 * excludes the size field (HEADERSIZE) itself, i.e., if the physical
 * size of the chunk is 3 words, the size field actually contains 2.
 *
 * As obvious, the next field is NIL (0) if this is the last
 * free chunk in memory.
 *
 * Chunks are always at least 2 words (64 bits) long, because this
 * is the minimum size needed for allocating new objects. Free memory
 * areas smaller than this are automatically merged with other objects
 * (thus adding "dead" space to objects). In practice this happens rarely,
 * though.
 *=======================================================================*/

/*  CHUNK */
struct chunkStruct {
    long  size;     /*  The size of the chunk in words (including the header) */
    CHUNK next;     /*  Pointer to the next chunk (NIL if none) */
};

/*=========================================================================
 * As a Palm specific option, the static portion of a Palm device's memory
 * can be used to store relatively static structures such as the constant
 * pool and code of a class. By storing these structures in static memory
 * a substantial amount of room is freed up for use by the garbage
 * collected heap. Given that we expect anything put into static memory to
 * live for the lifetime of a VM execution, we can implement the management
 * of any static memory chunks simply as a list of chunks that are all
 * freed upon the end of execution. As such the basic structure of a
 * statically allocated chunk will be as follows:
 *
 *                   +---------------+
 *                   ! prev chunk ptr! (bit 0: bump bit)
 *                   +---------------+
 * Object pointer -> !               !    ^
 *                   +---------------+    !
 *                   .               .    !
 *                   .               .  data
 *                   .               .    !
 *                   !               !    !
 *                   +---------------+    !
 *                   !               !    v
 *                   +---------------+
 *
 * These chunks are allocated via the mallocStaticBytes operation and are
 * all collected at once by the FinalizeStaticMemory operation.
 *=======================================================================*/

/* Since the PalmOS memory returns two-byte aligned addresses, */
/* we must occasionally bump the statically allocated object */
/* addresses upwards in the memory (by two bytes). This is  */
/* indicated by raising a special "bump bit" stored in the */
/* least significant bit in the previous chunk pointer field  */
/* (shown above). */

#define STATICBUMPBIT  0x00000001
#define STATICBUMPMASK 0xFFFFFFFE

/*=========================================================================
 * Garbage collection types of objects (GCT_* types)
 *=========================================================================
 * These types are used for instructing the garbage collector to
 * scan the data fields of objects correctly upon garbage collection.
 * Since two low-end bits of the first header field are used for
 * flags, we don't use these in type declarations.
 *=======================================================================*/

typedef enum {
    GCT_FREE = 0,
    /*  Objects which have no pointers inside them */
    /*  (can be ignored safely during garbage collection) */
    GCT_NOPOINTERS,

    /*  Java-level objects which may have mutable pointers inside them */
    GCT_INSTANCE,
    GCT_ARRAY,
    GCT_OBJECTARRAY,

    /* Only if we have static roots */
    GCT_METHODTABLE,

    /*  Internal VM objects which may have mutable pointers inside them */
    GCT_POINTERLIST,
    GCT_EXECSTACK,
    GCT_THREAD,
    GCT_MONITOR,

    /* A weak pointer list gets marked/copied after all other objects */
    GCT_WEAKPOINTERLIST,

    /* Added for java.lang.ref.WeakReference support in CLDC 1.1 */
    GCT_WEAKREFERENCE
} GCT_ObjectType;

#define GCT_FIRSTVALIDTAG GCT_NOPOINTERS
#define GCT_LASTVALIDTAG  GCT_WEAKREFERENCE

#define GCT_TYPENAMES  { \
        "FREE",          \
        "NOPOINTERS",    \
        "INSTANCE",      \
        "ARRAY",         \
        "OBJECTARRAY",   \
        "METHODTABLE",   \
        "POINTERLIST",   \
        "EXECSTACK",     \
        "THREAD",        \
        "MONITOR",       \
        "WEAKPOINTERLIST", \
        "WEAKREFERENCE" \
    }

/*=========================================================================
 * Dynamic heap variables
 *=======================================================================*/

extern cell* AllHeapStart;   /* Lower limits of any heap space */
extern cell* AllHeapEnd;

extern cell* CurrentHeap;    /* Current limits of heap space */
extern cell* CurrentHeapEnd; /* Current heap top */

/*=========================================================================
 * Garbage collection operations
 *=======================================================================*/

/*    Memory system initialization */
void  InitializeGlobals(void);
void  InitializeMemoryManagement(void);
void  FinalizeMemoryManagement(void);
void  InitializeHeap(void);
void  FinalizeHeap(void);

/*    Garbage collection operations */
void  garbageCollect(int moreMemory);
void  garbageCollectForReal(int moreMemory);

int   garbageCollecting(void);

/*=========================================================================
 * Memory allocation operations
 *=======================================================================*/

/*    Memory allocation operations */
cell* mallocHeapObject(long size, GCT_ObjectType type);

cell* mallocObject(long size, GCT_ObjectType type);
cell* callocObject(long size, GCT_ObjectType type);
char* mallocBytes(long size);
cell* callocPermanentObject(long size);

/*    Printing and debugging operations */
long  getHeapSize(void);
long  memoryFree(void);
long  memoryUsage(void);

/*  Operations on individual heap objects */
unsigned long  getObjectSize(cell* object);
GCT_ObjectType getObjectType(cell* object);

/*=========================================================================
 * Printing and debugging operations
 *=======================================================================*/

#if INCLUDEDEBUGCODE
void  printHeapContents(void);
#else
#define printHeapContents()
#endif

#if INCLUDEDEBUGCODE
void checkHeap(void);
void printObject(cell *object);
void rollTheHeap();
#else
#define checkHeap()
#define printObject(object)
#define rollTheHeap()
#endif

/*=========================================================================
 * Memory management operations on static memory
 *=======================================================================*/

/*=========================================================================
 * These operations are really needed only in the Palm 
 * implementation of the KVM.  Read the description of the
 * static memory system earlier in this file.
 *=======================================================================*/

#if USESTATIC
void FinalizeStaticMemory(void);
void modifyStaticMemory(void *staticMemory, int offset, void *newVal, int size);
void *mallocStaticBytes(int size);
#else
#define FinalizeStaticMemory()
#define modifyStaticMemory(staticMemory, offset, newVal, size) \
           (((void)offset), ((void)newVal))
#define mallocStaticBytes(size)
#endif /* USESTATIC */

/*=========================================================================
 * Temporary root operations
 *=======================================================================*/

/*=========================================================================
 * COMMENT:
 * Temporary roots are structures that can be used for 
 * "pinning down" temporary objects and other structures
 * that have been allocated from the Java heap, but that
 * haven't yet been stored permanently to any Java object
 * or other VM runtime structure.
 *
 * Temporary roots MUST be used in situations when you 
 * have a native C routine that allocates two objects
 * from the Java heap and does not store the first of
 * these objects to any other runtime structure before
 * allocating the second object. Otherwise, a garbage
 * collection might occur when the second object is
 * allocated, invalidating the first pointer!
 * 
 * TYPICAL PROBLEM SCENARIO:
 * 
 * void yourNativeRoutineWrittenInC() {
 *
 *    // Allocate 100 bytes from Java heap
 *    char* foo = mallocBytes(100);
 *
 *    // Allocate another 100 bytes from Java heap
 *    char* bar = mallocBytes(100);
 *      ^
 *      +--- THE SECOND ALLOCATION ABOVE MAY CAUSE A 
 *           GARBAGE COLLECTION TO OCCUR AND THEREFORE
 *           INVALIDATE 'foo' !!!
 *  }
 *
 *
 * To verify that all the native functions have been
 * written in a GC-safe way, you should try the mode
 * EXCESSIVE_GARBAGE_COLLECTION on, and see if you
 * encounter any problems. In that mode, the KVM will
 * run very slowly, as it will do a full garbage collection
 * upon every object allocation, but you are guaranteed
 * to find the possible problems with missing temporary
 * root operations.
 *=======================================================================*/

#define MAXIMUM_TEMPORARY_ROOTS 50
#define MAXIMUM_GLOBAL_ROOTS 20

extern int TemporaryRootsLength;
extern int GlobalRootsLength;

extern union cellOrPointer TemporaryRoots[];
extern union cellOrPointer GlobalRoots[];

/* Handling of temporary roots */
#if  INCLUDEDEBUGCODE
#define VERIFY_INSIDE_TEMPORARY_ROOTS (_tmp_roots_ = _tmp_roots_),
#define INDICATE_DYNAMICALLY_INSIDE_TEMPORARY_ROOTS int _tmp_roots_ = 0;
#define VERIFY_TEMPORARY_ROOT_SPACE(size) verifyTemporaryRootSpace(size),
void verifyTemporaryRootSpace(int size);
#else
#define VERIFY_INSIDE_TEMPORARY_ROOTS 
#define INDICATE_DYNAMICALLY_INSIDE_TEMPORARY_ROOTS
#define VERIFY_TEMPORARY_ROOT_SPACE(size)
#endif /* INCLUDEDEBUGCODE */

#define START_TEMPORARY_ROOTS   { int _tmp_roots_ = TemporaryRootsLength;
#define END_TEMPORARY_ROOTS      TemporaryRootsLength = _tmp_roots_;  }

#if INCLUDEDEBUGCODE
extern int NoAllocation;
#define ASSERTING_NO_ALLOCATION { NoAllocation++; {
#define END_ASSERTING_NO_ALLOCATION } NoAllocation--; }
#else
#define ASSERTING_NO_ALLOCATION {
#define END_ASSERTING_NO_ALLOCATION }
#endif /* INCLUDEDEBUGCODE */

void makeGlobalRoot(cell** object);

#define unhand(x) (*(x))

#define IS_TEMPORARY_ROOT(_var_, _value_)                                \
  _var_ = (VERIFY_INSIDE_TEMPORARY_ROOTS                                 \
        _var_ = _value_,                                                 \
        VERIFY_TEMPORARY_ROOT_SPACE(1)                                   \
        TemporaryRoots[TemporaryRootsLength++].cellp = (cell *)&_var_,   \
        _var_)

#define IS_TEMPORARY_ROOT_FROM_BASE(_var_, _value_, _base_)              \
     _var_ = (VERIFY_INSIDE_TEMPORARY_ROOTS                              \
        VERIFY_TEMPORARY_ROOT_SPACE(3)                                   \
        _var_ = _value_,                                                 \
       TemporaryRoots[TemporaryRootsLength].cell = -1,                   \
       TemporaryRoots[TemporaryRootsLength+1].cellpp = (cell **)&_var_,  \
       TemporaryRoots[TemporaryRootsLength+2].cellp = (cell *)_base_,    \
       TemporaryRootsLength = TemporaryRootsLength + 3,                  \
      _var_)

#define DECLARE_TEMPORARY_ROOT(_type_, _var_, _value_)                   \
     _type_ IS_TEMPORARY_ROOT(_var_, _value_)

#define DECLARE_TEMPORARY_ROOT_FROM_BASE(_type_, _var_, _value_, _base_) \
    _type_ IS_TEMPORARY_ROOT_FROM_BASE(_var_, _value_, _base_)

#define DECLARE_TEMPORARY_METHOD_ROOT(_var_, _table_, _index_)           \
     DECLARE_TEMPORARY_ROOT_FROM_BASE(METHOD, _var_,                     \
                                      &_table_->methods[_index_], _table_)

#define DECLARE_TEMPORARY_FRAME_ROOT(_var_, _value_) \
     DECLARE_TEMPORARY_ROOT_FROM_BASE(FRAME, _var_, _value_, _value_->stack)

/*=========================================================================
 * Functions that are called only if you are using the 
 * JavaCodeCompact tool (a.k.a. romizer)
 *=======================================================================*/

#if ROMIZING
     /* Initialize the ROM image.  Clean up the ROM image when you're done */
     extern void InitializeROMImage(void);
     extern void FinalizeROMImage(void);

     /* The pointer to the all the static variables that need to be relocated.
      * It's a pointer when using relocation rom, and an actual array when
      * using fixed rom */
#if RELOCATABLE_ROM
extern long *KVM_staticDataPtr;
#else
extern long KVM_staticData[];
#endif
#else /* !ROMIZING */
#define InitializeROMImage()
#define FinalizeROMImage()
#endif /* ROMIZING */

/* This function creates the ROM image out of its relocatable pieces */

#if ROMIZING && RELOCATABLE_ROM
void CreateROMImage(void);
void DestroyROMImage(void);
#else
#define CreateROMImage()
#define DestroyROMImage()
#endif

/* When dealing with relocatable, preloaded classes, we can read preloaded
 * memory directly, but we have to use special functions to write them.
 *
 * We must call the following functions when there is a chance that the
 * object/class/method might be part of a preloaded class.
 */

#if ROMIZING && RELOCATABLE_ROM
void setClassInitialThread(INSTANCE_CLASS clazz, THREAD thread);
void setClassStatus(INSTANCE_CLASS clazz, int status);
void setBreakpoint(METHOD, long, char *, unsigned char);
#else
#define setClassInitialThread(iclazz, thread) \
                            ((iclazz)->initThread = (thread))
#define setClassStatus(iclazz, stat)   ((iclazz)->status = (stat))
#define setBreakpoint(method, offset, start, val)                     \
    if (USESTATIC) {                                                  \
        long offset1 = (char*)(&method->u.java.code[offset]) - start; \
        modifyStaticMemory(start, offset1, &val, 1);                  \
    } else {                                                          \
        method->u.java.code[offset] = val;                            \
    }
#endif

#if INCLUDEDEBUGCODE || RELOCATABLE_ROM
#if ROMIZING
extern bool_t isROMString(void *), isROMClass(void *), isROMMethod(void *);
#else
#define isROMClass(X) FALSE
#define isROMString(X) FALSE
#define isROMMethod(X) FALSE
#endif /* ROMIZING */
#endif

/*=========================================================================
 * Stackmap-related operations
 *=======================================================================*/

/* The following are defined in stackmap.c, but are really part of the
 * garbage collection system.
 */
STACKMAP rewriteVerifierStackMapsAsPointerMaps(METHOD);
unsigned int getGCRegisterMask(METHOD, unsigned char *targetIP,  char *map);

/*=========================================================================
 * Native function cleanup registration
 *=======================================================================*/

/*=========================================================================
 * COMMENT:
 * Since version 1.0.3, KVM garbage collector includes a special
 * callback mechanism that allows each KVM port to register
 * implementation-specific cleanup routines that get called
 * automatically upon system exit (FinalizeMemoryManagement()).
 * This mechanism can be used, e.g., for adding network or UI-widget
 * cleanup routines to the system.  For instance, all MIDP
 * implementations typically need to take advantage of this
 * feature.
 *
 * New callback routines are registered by calling the function
 * 'registerCleanup' defined below.
 *
 * NOTE: In KVM 1.0.4 and 1.1, the types 'INSTANCE_HANDLE' and
 *       the KNI type 'jobject' are identical.  Therefore, it is
 *       acceptable to use a jobject in these cleanup routines
 *       whenever an INSTANCE_HANDLE is required.  In order to make
 *       the native finalization code more compliant with KNI, the
 *       callback routine now takes an INSTANCE_HANDLE parameter
 *       instead of INSTANCE (in KVM 1.0.3).
 *=======================================================================*/

/* The function type of a cleanup callback function */
typedef void (*CleanupCallback)(INSTANCE_HANDLE);

/*
 * Register a cleanup function for the instance.
 * When the gc finds the object unreachable it will see if there is
 * an associated cleanup function and if so will call it with the instance.
 */
extern void registerCleanup(INSTANCE_HANDLE, CleanupCallback);

/* The maximum number of callback functions that can be registered */
#ifndef CLEANUP_ROOT_SIZE
#define CLEANUP_ROOT_SIZE 16
#endif

/* The initial size of each cleanup array */
#ifndef CLEANUP_ARRAY_SIZE
#define CLEANUP_ARRAY_SIZE 3
#endif

#ifndef CLEANUP_ARRAY_GROW
#define CLEANUP_ARRAY_GROW 3
#endif

