/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Constant pool
 * FILE:      pool.h
 * OVERVIEW:  Constant pool management definitions.
 * AUTHOR:    Antero Taivalsaari, Sun Labs, 1998
 *            Frank Yellin, Sheng Liang
 *=======================================================================*/

/*=========================================================================
 * COMMENTS:
 * This file defines a JVM Specification compliant implementation
 * of constant pool structures.  A constant pool is a generic, 
 * relocatable, symbol table -like structure that contains all the
 * symbolic information of a Java class and its references to the 
 * outside world. 
 * 
 * Constant pool is a rather inefficient class representation 
 * format from the implementation point of view. However, since 
 * Java bytecodes access data and methods solely via constant pool
 * indices, a runtime constant pool structure must be maintained 
 * for every class in the system.  In practice, constant pool
 * access is so inefficient that other, additional runtime 
 * structures must be introduced to speed up the system.
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

/*=========================================================================
 * Definitions and declarations
 *=======================================================================*/

/*=========================================================================
 * Field and method access flags (from JVM Specification)
 *=======================================================================*/

#define ACC_PUBLIC       0x0001
#define ACC_PRIVATE      0x0002
#define ACC_PROTECTED    0x0004
#define ACC_STATIC       0x0008
#define ACC_FINAL        0x0010
#define ACC_SYNCHRONIZED 0x0020
#define ACC_SUPER        0x0020
#define ACC_VOLATILE     0x0040
#define ACC_TRANSIENT    0x0080
#define ACC_NATIVE       0x0100
#define ACC_INTERFACE    0x0200
#define ACC_ABSTRACT     0x0400
#define ACC_STRICT       0x0800

/* The following flags are not part of the JVM specification.
 * We add them when the structure is created. 
 */
#define ACC_ARRAY_CLASS   0x1000  /* Class is an array class */
#define ACC_ROM_CLASS     0x2000  /* ROM class */
                                  /* A ROM class in which neither it nor any */
                                  /* of its superclasses has a <clinit> */
#define ACC_ROM_NON_INIT_CLASS 0x4000  

/* These are the same values as in the JDK.  Makes ROMizer simpler */
#define ACC_DOUBLE        0x4000  /* Field uses two words */
#define ACC_POINTER       0x8000  /* Field is a pointer   */

/*=========================================================================
 * Array types / type indices from JVM Specification (p. 320)
 *=======================================================================*/

#define T_BOOLEAN   4
#define T_CHAR      5
#define T_FLOAT     6
#define T_DOUBLE    7
#define T_BYTE      8
#define T_SHORT     9
#define T_INT       10
#define T_LONG      11

/*  Our own extensions to array types */
#define T_VOID      0
#define T_REFERENCE 1
#define T_CLASS     1  /* Another name for T_REFERENCE */

/* Define the range of primitive array types */
#define T_FIRSTPRIMITIVE_TYPE 4
#define T_LASTPRIMITIVETYPE 11

/*=========================================================================
 * Tag values of constant pool entries from JVM Specification
 *=======================================================================*/

#define CONSTANT_Utf8               1
#define CONSTANT_Integer            3
#define CONSTANT_Float              4
#define CONSTANT_Long               5
#define CONSTANT_Double             6
#define CONSTANT_Class              7
#define CONSTANT_String             8
#define CONSTANT_Fieldref           9
#define CONSTANT_Methodref          10
#define CONSTANT_InterfaceMethodref 11
#define CONSTANT_NameAndType        12

/* Cache bit */
#define CP_CACHEBIT  0x80
#define CP_CACHEMASK 0x7F

/* Cache bit operations */
#define CONSTANTPOOL_LENGTH(cp)      (cp->entries[0].length)
#define CONSTANTPOOL_TAGS(cp)        ((unsigned char *)&(cp)->entries[CONSTANTPOOL_LENGTH(cp)])
#define CONSTANTPOOL_TAG(cp, index)  ((CONSTANTPOOL_TAGS(cp))[index])

/*=========================================================================
 * Generic constant pool table entry structure
 *=======================================================================*/

/*=========================================================================
 * COMMENT:
 * Even though the size of constant pool entries depends on the
 * actual type of the entry (types are listed above), in the runtime
 * constant pool of each class all the entries are of the same size.
 *
 * The size of all the entries must be the same, since we want 
 * to be able to address constant pool entries via indexing 
 * rather than more time-consuming lookups.
 *
 * Keep in mind that LONG and DOUBLE constant pool entries consume
 * two entries in the constant pool (see page 98 in JVM Specification).
 *=======================================================================*/

/* CONSTANTPOOLENTRY */

/* Each of these represents one entry in the constant pool */
union constantPoolEntryStruct {
    struct { 
        unsigned short classIndex;
        unsigned short nameTypeIndex;
    }               method;  /* Also used by Fields */
    CLASS           clazz;
    INTERNED_STRING_INSTANCE String;
    cell           *cache;   /* Either clazz or String */
    cell            integer;
    long            length;
    NameTypeKey     nameTypeKey;
    NameKey         nameKey;
    UString         ustring;
};

/* CONSTANTPOOL */

/* The constant pool data structure is slightly complicated.
 * The first (= zeroeth) element in the constant pool contains 
 * the number of entries in the constant pool.
 * 
 * The actual constant pool entries are located in entries
 * entry[1] -- entry[length-1].
 *
 * Immediately after the last constant pool entry[length - 1] is 
 * an array of bytes that contains the tags that tell the type of
 * each constant pool entry.
 * 
 * The i'th byte in this byte array is the tag for the i'th 
 * constant pool entry in the table.
 * 
 * Each type tag may contain a special cache bit if 
 * the corresponding constant pool entry has already
 * been resolved earlier.  In that case, the constant
 * pool entry points directly to the appropriate data
 * structure, so that we don't have to do a constant 
 * pool resolution for that entry again.
 * 
 * Keep in mind that the length of the constant pool is
 * always +1 larger than the actual number of entries
 * (see Java VM Spec Section 4.1)
 * 
 * Here's a diagram that illustrates the structure:
 *
 *        +--------+
 *        |   5    |  entry[0]
 *        |        |  Contains the number of entries + 1 (4+1 here)
 *        +--------+
 *        |        |  entry[1]
 *        |        |  This is the first actual const pool entry
 *        +--------+
 *        |        |  entry[2]
 *        |        |  This is the second constant pool entry
 *        +--------+
 *        |        |  ...  more constant pool entries ...
 *        |        |
 *        +--------+
 *        |        |  entry[n-1] / n = 5 in this case
 *        |        |  Last actual constant pool entry
 *        +--------++
 *        |0| | | | | Byte array to store the type tag of each entry.
 *        +-+-+-+-+-+ If the entry contains a cached (previously resolved) entry,
 *                    then the tag is OR'red with CP_CACHEBIT. 
 *                    In this example, 5 bytes are needed.  Note that the
 *                    type tag of the zeroeth entry is always zero (dummy entry).
*/

struct constantPoolStruct { 
    union constantPoolEntryStruct entries[1];
};

#define SIZEOF_CONSTANTPOOL_ENTRY  UnionSizeInCells(constantPoolEntryStruct)

/*=========================================================================
 * Constant pool access methods (low level)
 *=======================================================================*/

void  verifyClassAccess(CLASS targetClass, INSTANCE_CLASS fromClass);

bool_t classHasAccessToClass(INSTANCE_CLASS currentClass, CLASS targetClass);
bool_t classHasAccessToMember(INSTANCE_CLASS currentClass, 
                              int access, INSTANCE_CLASS memberClass,
                              INSTANCE_CLASS cpClass);

/*=========================================================================
 * Constant pool access methods (high level)
 *=======================================================================*/

CLASS  resolveClassReference (CONSTANTPOOL, unsigned int cpIndex, 
                              INSTANCE_CLASS fromClass);
FIELD  resolveFieldReference (CONSTANTPOOL, unsigned int cpIndex, bool_t
                              isStatic, int opcode, INSTANCE_CLASS fromClass);
METHOD resolveMethodReference(CONSTANTPOOL, unsigned int cpIndex, bool_t
                              isStatic, INSTANCE_CLASS fromClass); 

