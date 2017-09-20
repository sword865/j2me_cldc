/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 */

/*=========================================================================
 * KVM 
 *=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Internal hashtables
 * FILE:      hashtable.h
 * OVERVIEW:  This file defines the structures for maintaining 
 *            hashtables inside the KVM.  Hashtable data structures
 *            are used internally by the KVM for maintaining:
 *            1) classes (ClassTable)
 *            2) strings (InternStringTable)
 *            3) UTF8 strings (UTFStringTable)
 * AUTHOR:    Frank Yellin
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <stddef.h>

/*=========================================================================
 * Definitions and declarations
 *=======================================================================*/

/* The sizes of the various hash tables */

#define UTF_TABLE_SIZE 256
#define CLASS_TABLE_SIZE 32
#define INTERN_TABLE_SIZE 32

/* The declaration of a hashtable.  We make the buckets fairly
 * generic.
 */
typedef struct HashTable {
    long bucketCount;     /* Number of buckets */
    long count;           /* Number of total items in the table */
    cell *bucket[1];     /* Array of entries */
} *HASHTABLE;

/* The declaration of one bucket that holds unique instances of UTF strings
 */
typedef struct UTF_Hash_Entry { 
    struct UTF_Hash_Entry *next; /* The next bucket */
        unsigned short length;
        unsigned short key;
        char string[1];          /* The characters of the string */
} *UTF_HASH_ENTRY, *UString;

/*=========================================================================
 * Size macros
 *=======================================================================*/

/* A hashtable with n buckets */
#define SIZEOF_HASHTABLE(n)     (StructSizeInCells(HashTable) + (n - 1))

/* A UTF_HASH_ENTRY that can hold a string of length n.  Note that
 * this >>does<< include the NULL character, since we've already saved
 * space for one character */
 
#define SIZEOF_UTF_HASH_ENTRY(n) \
    ByteSizeToCellSize(offsetof(struct UTF_Hash_Entry, string) + n + 1)

/*=========================================================================
 * Global variables
 *=======================================================================*/

/* Hashtable containing all the Java strings in the system */
extern HASHTABLE InternStringTable;

/* Hashtable containing all the utf C strings in the system */
extern HASHTABLE UTFStringTable;

/* Hashtable containing all the classes in the system */
extern HASHTABLE ClassTable;

/*=========================================================================
 * Hashtable creation and deletion
 *=======================================================================*/

/* Create a new hash table */
void createHashTable(HASHTABLE *tablePtr, int bucketCount);

/* Called at startup to initialize the hashtables */
void InitializeHashtables(void);
void FinalizeHashtables(void);

#if ROMIZING
void finalizeROMHashTable(HASHTABLE, int offset);
#endif

/*=========================================================================
 * Functions
 *=======================================================================*/

/* Convert a const char* to a UString, or a UString to a const char */
UString getUString(const char *string);
UString getUStringX(CONST_CHAR_HANDLE nameH, int offset, int length);

#define UStringInfo(str) str->string

/* Get a unique String for a given char array */
INTERNED_STRING_INSTANCE internString(const char *string, int length);

/* Convert utf8 to unicode */
short utf2unicode(const char **utf);

char* unicode2utf(unsigned short *, int, char *, int);
int unicode2utfstrlen(unsigned short *, int);

/* Determine the actual number of characters in a utf8 string */
unsigned int utfStringLength(const char *utfstring, int length);

/* Conversion between keys and names */
NameKey change_Name_to_Key(CONST_CHAR_HANDLE, int offset, int length);
char *change_Key_to_Name(NameKey, int *length);

/* Conversion between classes and names */
CLASS change_Name_to_CLASS(UString package, UString base);
CLASS change_Key_to_CLASS(FieldTypeKey);

/*=========================================================================
 * Printing and debugging operations
 *=======================================================================*/

#if INCLUDEDEBUGCODE
void printUTFStringTable();
#else
#  define printUTFStringTable()
#endif

