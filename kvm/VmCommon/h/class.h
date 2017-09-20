/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Internal runtime structures
 * FILE:      class.h
 * OVERVIEW:  Internal runtime class structures.
 * AUTHOR:    Antero Taivalsaari, Sun Labs, 1998
 *            Edited by Doug Simon 11/1998 (added the string pool)
 *            Frank Yellin
 *=======================================================================*/

/*=========================================================================
 * COMMENTS:
 * This file defines the VM-specific internal runtime structures
 * needed for representing classes and their instances in the system.
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

/*=========================================================================
 * Global definitions
 *=======================================================================*/

/* Class status flags */
#define CLASS_RAW       0 /* this value must be 0 */
#define CLASS_LOADING   1
#define CLASS_LOADED    2
#define CLASS_LINKED    3
#define CLASS_VERIFIED  4
#define CLASS_READY     5
#define CLASS_ERROR    -1

/* A class is considered initialized if it's ready or being initialized
 * by the current thread.
 */
#define CLASS_INITIALIZED(c) \
    ((c)->status == CLASS_READY || (c)->initThread == CurrentThread)

#define IS_ARRAY_CLASS(c) (( ((CLASS)(c))->accessFlags & ACC_ARRAY_CLASS) != 0)

/* Use the 13th bit to indicate whether a class instance has been
 * initialized.
 */
#define ITEM_NewObject_Flag 0x1000
#define ITEM_NewObject_Mask 0x0FFF

#define ENCODE_NEWOBJECT(pc) ((((pc) & 0x7000)<<1) | 0x1000 | ((pc) & 0x0FFF)) 
#define DECODE_NEWOBJECT(no) ((((no) & 0xE000)>>1) | ((no) & 0x0FFF))

/* Abstract types used by the byte code verifier. */
enum {
    ITEM_Bogus,       /* Unused */
    ITEM_Integer,
    ITEM_Float,
    ITEM_Double,
    ITEM_Long,
    ITEM_Null,        /* Result of aconst_null */
    ITEM_InitObject,  /* "this" is in <init> method, before call to super() */

    ITEM_Object,      /* Extra info field gives name. */
    ITEM_NewObject,   /* Like object, but uninitialized. */

    /* The following codes are used by the verifier but don't actually occur in
     * class files.
     */
    ITEM_Long_2,      /* 2nd word of long in register */
    ITEM_Double_2,    /* 2nd word of double in register */

    ITEM_Category1,
    ITEM_Category2,
    ITEM_DoubleWord,
    ITEM_Reference
};

/*=========================================================================
 * Internal runtime class and instance data structures
 *=======================================================================*/

/*=========================================================================
 * Every object in KVM looks roughly like this:
 * 
 *               +--------------------------+
 *               | GC header word (32 bits) |
 *               +==========================+
 * object ptr -> | class pointer            |
 *               +--------------------------+
 *               | monitorOrHashCode field  |
 *               +--------------------------+
 *               | ... the rest of the      |
 *               .     data ...             .
 *               .                          .
 *
 * The data type declarations below define the various 
 * class and instance data structures that the KVM uses
 * internally at runtime.
 * 
 * The "real" (upper-caps) names for these data types 
 * are defined in file 'global.h'.
 *
 * IMPORTANT: If you change the order of any of the data elements
 * in these data structures, make sure that the order matches
 * with the corresponding data structures in 'rom.h'.
 *=======================================================================*/

/** 
 * The low two bits of the monitorOrHashCode (mhc) field of an object
 * say what it is.  See thread.h for more information.
 */
typedef union monitorOrHashCode { 
    void *address;          /* low 2 bits MHC_MONITOR,  
                                          MHC_SIMPLE_LOCK, 
                                          MHC_EXTENDED_LOCK */
    long  hashCode;         /* low 2 bits MHC_UNLOCKED */
} monitorOrHashCode;

/* Macro that defines the most common part of the objects */
#define COMMON_OBJECT_INFO(_type_) \
    _type_ ofClass; /* Pointer to the class of the instance */ \
    monitorOrHashCode mhc;

/* CLASS */
struct classStruct {
    COMMON_OBJECT_INFO(INSTANCE_CLASS)

    /* Note that arrays classes have the same packageName as their base
     * class.  the baseName for arrays is [...[L<className>; or
     * [...[<primitive type>, where [...[ indicates the appropriate
     * number of left brackets for the depth */
    UString packageName;            /* Everything before the final '/' */
    UString baseName;               /* Everything after the final '/' */
    CLASS   next;                   /* Next item in this hash table bucket */
    
    unsigned short accessFlags;     /* Access information */
    unsigned short key;             /* Class key */
};

typedef void (*NativeFuncPtr) (INSTANCE_HANDLE);

/* INSTANCE_CLASS */
struct instanceClassStruct {
    struct classStruct clazz;       /* common info */

    /* And specific to instance classes */
    INSTANCE_CLASS superClass;      /* Superclass, unless java.lang.Object */
    CONSTANTPOOL constPool;         /* Pointer to constant pool */
    FIELDTABLE  fieldTable;         /* Pointer to instance variable table */
    METHODTABLE methodTable;        /* Pointer to virtual method table */
    unsigned short* ifaceTable;     /* Pointer to interface table */
    POINTERLIST staticFields;       /* Holds static fields of the class */
    short   instSize;               /* The size of class instances */
    short status;                   /* Class readiness status */
    THREAD initThread;              /* Thread performing class initialization */
    NativeFuncPtr finalizer;        /* Pointer to finalizer */
};

/* ARRAY_CLASS */
struct arrayClassStruct {
    struct classStruct clazz;       /* Common info */

    /* And stuff specific to an array class */
    union {
        CLASS elemClass;            /* Element class of object arrays */
        long primType;              /* Element type for primitive arrays */
    } u;
    long itemSize;                  /* Size (bytes) of individual elements */
                                    /* wants to be GCT_ObjectType rather than */
                                    /* an int. But the Palm makes enumerations*/
                                    /* into "short" */
    long gcType;                    /* GCT_ARRAY or GCT_OBJECTARRAY */
    long flags;                     /*   */
};

#define ARRAY_FLAG_BASE_NOT_LOADED 1

/* OBJECT (Generic Java object) */
struct objectStruct {
    COMMON_OBJECT_INFO(CLASS)
};

/* INSTANCE */
struct instanceStruct {
    COMMON_OBJECT_INFO(INSTANCE_CLASS)
    union {
        cell *cellp;
        cell cell;
    } data[1];
};

/* These are never created directly. */
/* It's what a java.lang.String looks like */
/* STRING_INSTANCE */
struct stringInstanceStruct {
    COMMON_OBJECT_INFO(INSTANCE_CLASS)

    SHORTARRAY array;
    cell offset;
    cell length;
};

/* INTERNED_STRING_INSTANCE */
struct internedStringInstanceStruct { 
    COMMON_OBJECT_INFO(INSTANCE_CLASS)

    SHORTARRAY array;
    cell offset;
    cell length;
    struct internedStringInstanceStruct *next;
};

struct throwableInstanceStruct { 
    COMMON_OBJECT_INFO(INSTANCE_CLASS)

    STRING_INSTANCE message;
    ARRAY   backtrace;
};

typedef union cellOrPointer {
    cell cell;
    cell *cellp;
    cell **cellpp;              /* For global roots */

    /* Occasionally needed by GC */
    char *charp;
    char **charpp;
} cellOrPointer;

/* ARRAY */
struct arrayStruct {
    COMMON_OBJECT_INFO(ARRAY_CLASS)

    cell  length;               /* Number of elements */
    cellOrPointer data[1];  
};

/* POINTERLIST */
struct pointerListStruct {
    long  length;
    cellOrPointer data[1];  
};

/* WEAKPOINTERLIST */
struct weakPointerListStruct { 
    long length;
    struct weakPointerListStruct* gcReserved;
    void (*finalizer)(INSTANCE_HANDLE);
    cellOrPointer data[1];  
};

/* WEAKREFERENCE (Added for CLDC 1.1 support) */
struct weakReferenceStruct {
    COMMON_OBJECT_INFO(INSTANCE_CLASS)
    cell* referent;
    struct weakReferenceStruct* gcReserved;
};

/* BYTEARRAY */
struct byteArrayStruct {
    COMMON_OBJECT_INFO(ARRAY_CLASS)

    cell  length;               /* The size of the array (slot count) */
    signed char bdata[1];       /* First (zeroeth) data slot of the array */
};

/* SHORTARRAY */
struct shortArrayStruct {
    COMMON_OBJECT_INFO(ARRAY_CLASS)

    cell  length;               /* The size of the array (slot count) */
    short sdata[1];             /* First (zeroeth) data slot of the array */
};

/*=========================================================================
 * Sizes for the above structures
 *=======================================================================*/

#define SIZEOF_CLASS              StructSizeInCells(classStruct)
#define SIZEOF_INSTANCE(n)        (StructSizeInCells(instanceStruct)+((n)-1))
#define SIZEOF_ARRAY(n)           (StructSizeInCells(arrayStruct)+((n)-1))

#define SIZEOF_INSTANCE_CLASS     StructSizeInCells(instanceClassStruct)
#define SIZEOF_ARRAY_CLASS        StructSizeInCells(arrayClassStruct)

#define SIZEOF_POINTERLIST(n)     (StructSizeInCells(pointerListStruct)+((n)-1))
#define SIZEOF_WEAKPOINTERLIST(n) (StructSizeInCells(weakPointerListStruct)+((n)-1))

#define SIZEOF_STRING_INSTANCE           SIZEOF_INSTANCE(3)
#define SIZEOF_INTERNED_STRING_INSTANCE  SIZEOF_INSTANCE(4)

/*=========================================================================
 * Definitions that manipulate the monitorOrHashCode (mhc) field
 *=======================================================================*/

/** 
 * See thread.h for more information on monitorOrHashCode operations.
 */

#define OBJECT_HAS_MONITOR(obj)      (OBJECT_MHC_TAG(obj) != MHC_UNLOCKED)
#define OBJECT_HAS_REAL_MONITOR(obj) (OBJECT_MHC_TAG(obj) == MHC_MONITOR)

#define SET_OBJECT_HASHCODE(obj, hc) \
        SET_OBJECT_MHC_LONG(obj, (long)(hc) + MHC_UNLOCKED)

#define SET_OBJECT_MONITOR(obj, mon) \
        SET_OBJECT_MHC_ADDRESS(obj, (char *)(mon) + MHC_MONITOR)

#define SET_OBJECT_SIMPLE_LOCK(obj, thr) \
        SET_OBJECT_MHC_ADDRESS(obj, (char *)(thr) + MHC_SIMPLE_LOCK)

#define SET_OBJECT_EXTENDED_LOCK(obj, thr) \
        SET_OBJECT_MHC_ADDRESS(obj, (char *)(thr) + MHC_EXTENDED_LOCK)

#if !RELOCATABLE_ROM

#define SET_OBJECT_MHC_LONG(obj, value) (obj)->mhc.hashCode = (value)
#define SET_OBJECT_MHC_ADDRESS(obj, value) (obj)->mhc.address = (value)

#else
void setObjectMhcInternal(OBJECT, long);
#define SET_OBJECT_MHC_LONG(obj, value)    SET_OBJECT_MHC_FIELD(obj,(value))
#define SET_OBJECT_MHC_ADDRESS(obj, value) SET_OBJECT_MHC_FIELD(obj,(long)(value))

#define SET_OBJECT_MHC_FIELD(obj, value)              \
       if (!RELOCATABLE_ROM || inAnyHeap(obj)) {      \
           (obj)->mhc.hashCode = value;               \
       } else {                                       \
            setObjectMhcInternal((OBJECT)obj, value); \
       }                                              \

#endif /* !RELOCATABLE_ROM */

/*=========================================================================
 * Global variables
 *=======================================================================*/

/*=========================================================================
 * These global variables hold references to various Java-specific
 * runtime structures needed for executing certain low-level operations.
 *
 * NOTE: To minimize the footprint of the VM, the internal structures these
 *       variables represent are loaded lazily (as opposed to standard JDK
 *       behaviour where these core classes are loaded upon VM
 *       initialization). This means that care must be taken when using
 *       them to ensure that they are not null references.
 *=======================================================================*/

/* Pointers to the most important Java classes needed by the VM */
extern INSTANCE_CLASS JavaLangObject;    /* Pointer to java.lang.Object */
extern INSTANCE_CLASS JavaLangClass;     /* Pointer to java.lang.Class */
extern INSTANCE_CLASS JavaLangString;    /* Pointer to java.lang.String */
extern INSTANCE_CLASS JavaLangSystem;    /* Pointer to java.lang.System */
extern INSTANCE_CLASS JavaLangThread;    /* Pointer to java.lang.Thread */
extern INSTANCE_CLASS JavaLangThrowable; /* Pointer to java.lang.Throwable */
extern INSTANCE_CLASS JavaLangError;     /* Pointer to java.lang.Error */
extern INSTANCE_CLASS JavaLangOutOfMemoryError; /* java.lang.OutOfMemoryError */
extern ARRAY_CLASS    JavaLangCharArray; /* Array of characters */

extern NameTypeKey initNameAndType;
extern NameTypeKey clinitNameAndType;
extern NameTypeKey runNameAndType;
extern NameTypeKey mainNameAndType;

extern METHOD RunCustomCodeMethod;
extern THROWABLE_INSTANCE OutOfMemoryObject;
extern THROWABLE_INSTANCE StackOverflowObject;

#define RunCustomCodeMethod_MAX_STACK_SIZE 4

/*=========================================================================
 * Constructors
 *=======================================================================*/

void initializeClass(INSTANCE_CLASS);

/*=========================================================================
 * System-level constructors
 *=======================================================================*/

void  InitializeJavaSystemClasses(void);
void  FinalizeJavaSystemClasses(void);

/*=========================================================================
 * Operations on classes
 *=======================================================================*/

CLASS getRawClass(const char *name);
CLASS getRawClassX(CONST_CHAR_HANDLE nameH, int offset, int length);

CLASS getClass(const char *name);
CLASS getClassX(CONST_CHAR_HANDLE nameH, int offset, int length);

ARRAY_CLASS getArrayClass(int depth, INSTANCE_CLASS baseClass, char signCode);

extern ARRAY_CLASS PrimitiveArrayClasses[];
ARRAY_CLASS getObjectArrayClass(CLASS elementType);

/* Returns the result.  Note, the return value of the second one is
 * the closing '\0' */
char *getClassName(CLASS clazz);
char *getClassName_inBuffer(CLASS clazz, char *result);

/*
 * This is required if an error occurs during class loading. That is, if an
 * error occurs before a class reaches the class initialization step, it is
 * reverted to a raw class. This complies with the JVM specification.
 */
INSTANCE_CLASS revertToRawClass(INSTANCE_CLASS clazz);

/*=========================================================================
 * Type conversion helper functions
 *=======================================================================*/

char typeCodeToSignature(char typeCode);

/*=========================================================================
 * Operations on (regular, non-array) instances
 *=======================================================================*/

INSTANCE instantiate(INSTANCE_CLASS);
long     objectHashCode(OBJECT object);
bool_t   implementsInterface(INSTANCE_CLASS, INSTANCE_CLASS);
bool_t   isAssignableTo(CLASS, CLASS);

/* guaranteed not to GC.  Returns TRUE or "don't know" */
bool_t   isAssignableToFast(CLASS, CLASS);

/*=========================================================================
 * Operations on array instances
 *=======================================================================*/

ARRAY    instantiateArray(ARRAY_CLASS arrayClass, long length);
ARRAY    instantiateMultiArray(ARRAY_CLASS arrayClass, long*, int dimensions);
long     arrayItemSize(int arrayType);

/*=========================================================================
 * Operations on string instances
 *=======================================================================*/

STRING_INSTANCE instantiateString(const char* string, int length);
INTERNED_STRING_INSTANCE instantiateInternedString(const char* string, int length);
SHORTARRAY createCharArray(const char* utf8stringArg, int utf8length,
                           int* unicodelengthP, bool_t isPermanent);

char*    getStringContents(STRING_INSTANCE string);
char*    getStringContentsSafely(STRING_INSTANCE string, char *buf, int lth);

/*=========================================================================
 * Debugging operations
 *=======================================================================*/

#if INCLUDEDEBUGCODE
void printInstance(INSTANCE);
void printArray(ARRAY);
void printClassName(CLASS);
void printString(INSTANCE);
#else
#  define printInstance(instance)
#  define printArray(ARRAY)
#endif

/*=========================================================================
 * Iterator macros
 *=======================================================================*/

#define FOR_ALL_CLASSES(__clazz__)                                   \
    {                                                                \
     HASHTABLE __table__ = ClassTable;                               \
     int __count__ = __table__->bucketCount;                         \
     while (-- __count__ >= 0) {                                     \
         CLASS __bucket__ = (CLASS)__table__->bucket[__count__];     \
         for ( ; __bucket__ != NULL; __bucket__ = __bucket__->next) { \
              CLASS __clazz__ = __bucket__ ; 

#define END_FOR_ALL_CLASSES   \
                        }     \
                }             \
        }

