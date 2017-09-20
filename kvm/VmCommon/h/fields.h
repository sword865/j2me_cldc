/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Internal runtime structures
 * FILE:      fields.h
 * OVERVIEW:  Internal runtime structures for storing different kinds 
 *            of fields (constants, variables, methods, interfaces).
 *            Tables of these fields are created every time a new class
 *            is loaded into the virtual machine.
 * AUTHOR:    Antero Taivalsaari, Sun Labs
 *            Edited by Doug Simon 11/1998
 *            Sheng Liang, Frank Yellin
 *=======================================================================*/

/*=========================================================================
 * COMMENTS:
 * This file defines the VM-specific internal runtime structures
 * needed for representing fields, methods and interfaces.
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

/*=========================================================================
 * COMMENTS:
 * FIELD is the basic structure that is used for representing
 * information about instance variables, static variables, constants
 * and static methods. It is used as a component in constant pool
 * and instance variable tables stored in CLASS structures.
 *
 * Method tables use a slightly larger structure (METHOD)
 * which is a logical subtype of FIELD. 
 *
 * Note: to speed up stack frame generation at runtime, we calculate
 * the number of parameters (argCount) and local variables (localCount)
 * for each method in advance upon class loading.
 *=======================================================================*/

/*=========================================================================
 * IMPORTANT: If you change the order of any of the data elements
 * in these data structures, make sure that the order matches
 * with the corresponding data structures in 'rom.h'.
 *=======================================================================*/

/*  FIELD */
struct fieldStruct {
    NameTypeKey nameTypeKey;
    long        accessFlags; /* Access indicators (public/private etc.) */
    INSTANCE_CLASS ofClass;  /* Backpointer to the class owning the field */
    union { 
        long  offset;        /* for dynamic objects */
        void *staticAddress; /* pointer to static.  */
    } u;
};

struct fieldTableStruct { 
    long length;
    struct fieldStruct fields[1];
};

/*  METHOD */
struct methodStruct {
    NameTypeKey   nameTypeKey;
    union { 
        struct {
            BYTE* code;
            HANDLERTABLE handlers;
            union {
                STACKMAP pointerMap;
                POINTERLIST verifierMap;
            } stackMaps;
            unsigned short codeLength;
            unsigned short maxStack; 
            /* frameSize should be here, rather than in the generic part, */
            /* but this gives us better packing of the bytes */
        } java;
        struct { 
            void (*code)(void);
            void *info;
        } native;
    } u;
    long  accessFlags;        /* Access indicators (public/private etc.) */
    INSTANCE_CLASS ofClass;   /* Backpointer to the class owning the field */
    unsigned short frameSize; /* Method frame size (arguments+local vars) */
    unsigned short argCount;  /* Method argument (parameter) count */
};

struct methodTableStruct { 
    long length;
    struct methodStruct methods[1];
};

/*=========================================================================
 * COMMENTS:
 * STACKMAPs are used internally by the KVM to store
 * information that the runtime part of the classfile
 * verifier can utilize to speed up verification.
 * This information is also utilized by the exact
 * garbage collector.
 *=======================================================================*/

#define STACK_MAP_SHORT_ENTRY_FLAG 0x8000
#define STACK_MAP_ENTRY_COUNT_MASK 0x7FFF
#define STACK_MAP_SHORT_ENTRY_OFFSET_MASK 0xFFF
#define STACK_MAP_SHORT_ENTRY_MAX_OFFSET 0xFFF
#define STACK_MAP_SHORT_ENTRY_MAX_STACK_SIZE 0xF

struct stackMapStruct {

    /* The high bit of nEntries is a flag indicating whether the structs are
     * short entries or not.  
     * If this is a short entry
     *       The low 12 bits of offset are the actual offset
     *       The high 4 bits of offset are the operand stack size
     *       stackMapKey is a bitmap giving the live locals and operand
     *          stacks.  ((char *)stackMapKey)[0] contains the low 8 bits and
     *          ((char *)stackMapKey)[1] contains the high 8 bits.
     * If this isn't a short entry, then offset is the actual 16-bit offset.
     * Stackmap key is an entry in the name table.  The first byte of the
     * entry is the stack size.  The remaining bytes are a variable length 
     * bitmap.
     */
    unsigned short nEntries;
    struct stackMapEntryStruct {
        unsigned short offset;
        unsigned short stackMapKey;
    } entries[1];
};

/*=========================================================================
 * COMMENTS:
 * The Java-level debugging interfaces need access to line number 
 * information.  The following structures are used for storing 
 * that information.
 *=======================================================================*/

#if ENABLE_JAVA_DEBUGGER

struct lineNumberStruct {
    unsigned short start_pc;
    unsigned short line_number;
};

struct lineNumberTableStruct {
    unsigned short length;
    struct lineNumberStruct lineNumber[1];
};

#endif /* ENABLE_JAVA_DEBUGGER */

/*=========================================================================
 * Sizes of the data structures above
 *=======================================================================*/

#define SIZEOF_FIELD             StructSizeInCells(fieldStruct)
#define SIZEOF_METHOD            StructSizeInCells(methodStruct)

#define SIZEOF_METHODTABLE(n)  \
        (StructSizeInCells(methodTableStruct) + (n - 1) * SIZEOF_METHOD)

#define SIZEOF_FIELDTABLE(n)   \
        (StructSizeInCells(fieldTableStruct) + (n - 1) * SIZEOF_FIELD)

/*=========================================================================
 * Operations on field and method tables
 *=======================================================================*/

FIELD  lookupField(INSTANCE_CLASS thisClass, NameTypeKey key);
METHOD lookupMethod(CLASS thisClass, NameTypeKey key, 
                    INSTANCE_CLASS currentClass);
METHOD lookupDynamicMethod(CLASS objectClass, METHOD declaredMethod);
METHOD getSpecialMethod(INSTANCE_CLASS thisClass, NameTypeKey key);

#if ENABLEPROFILING
int    getMethodTableSize(METHODTABLE methodTable);
#else 
# define getMethodTableSize(table) 0
#endif

/*=========================================================================
 * Debugging and printing operations
 *=======================================================================*/

#if INCLUDEDEBUGCODE
void   printFieldTable(FIELDTABLE fieldTable);
void   printMethodTable(METHODTABLE methodTable);
void   printFieldName(FIELD fieldTable,  LOGFILEPTR file);
void   printMethodName(METHOD methodTable, LOGFILEPTR file);
#else 
# define printFieldTable(x)
# define printMethodTable(y)
# define printFieldName(x, y)
# define printMethodName(x, y)
#endif

/*=========================================================================
 * Iterators
 *=======================================================================*/

#define FOR_EACH_METHOD(__var__, methodTable) {                            \
    if (methodTable != NULL) {                                             \
         METHOD __first_method__ = methodTable->methods;                   \
         METHOD __end_method__ = __first_method__ + methodTable->length;   \
         METHOD __var__;                                                   \
         for (__var__ = __first_method__; __var__ < __end_method__; __var__++) {

#define END_FOR_EACH_METHOD } } }

#define FOR_EACH_FIELD(__var__, fieldTable) {                            \
     if (fieldTable != NULL) {                                           \
         FIELD __first_field__ = fieldTable->fields;                     \
         FIELD __end_field__ = __first_field__ + fieldTable->length;     \
         FIELD __var__;                                                  \
         for (__var__ = __first_field__; __var__ < __end_field__; __var__++) { 

#define END_FOR_EACH_FIELD } } }

/*=========================================================================
 * Internal operations on signatures
 *=======================================================================*/

#define methodName(method) \
     (change_Key_to_Name((method)->nameTypeKey.nt.nameKey, NULL))

#define fieldName(field) \
     (change_Key_to_Name((field)->nameTypeKey.nt.nameKey, NULL))

#define getMethodSignature(method) \
      change_Key_to_MethodSignature((method)->nameTypeKey.nt.typeKey)

#define getFieldSignature(field) \
      change_Key_to_FieldSignature((field)->nameTypeKey.nt.typeKey)

char *change_Key_to_FieldSignature(FieldTypeKey);
char *change_Key_to_FieldSignature_inBuffer(FieldTypeKey, char *result);

char *change_Key_to_MethodSignature(MethodTypeKey);
char *change_Key_to_MethodSignature_inBuffer(MethodTypeKey, char *result);

FieldTypeKey change_FieldSignature_to_Key(CONST_CHAR_HANDLE, int offset,
                                          int length);
MethodTypeKey change_MethodSignature_to_Key(CONST_CHAR_HANDLE, int offset,
                                            int length);

NameTypeKey getNameAndTypeKey(const char* name, const char* type);

#define FIELD_KEY_ARRAY_SHIFT 13
#define FIELD_KEY_MASK 0x1FFF
#define MAX_FIELD_KEY_ARRAY_DEPTH 7

