/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Class loader
 * FILE:      loader.h
 * OVERVIEW:  Internal definitions needed for loading
 *            Java class files (class loader).
 * AUTHOR:    Antero Taivalsaari, Sun Labs, 1998
 *            Sheng Liang, Frank Yellin, many others...
 *=======================================================================*/

/*=========================================================================
 * COMMENTS:
 * This file defines a JVM Specification compliant classfile reader.
 * It is capable of reading any standard Java classfile, and generating
 * the necessary corresponding VM runtime structures.
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

/*=========================================================================
 * Definitions and declarations
 *=======================================================================*/

#ifndef PATH_SEPARATOR
#   define PATH_SEPARATOR ':'
#endif

#define JAVA_MIN_SUPPORTED_VERSION           45
#define JAVA_MAX_SUPPORTED_VERSION           48

/*=========================================================================
 * Classfile loading data structures
 *=======================================================================*/

/* The classpath that the user has set. */
/* Set in main() or elsewhere           */
extern char* UserClassPath;

/* This structure is used for referring to open "files" when   */
/* loading Java classfiles from the storage system of the host */
/* operating system. It replaces the standard FILE* structure  */
/* used in C program. We need our own structure, since many of */
/* our target devices don't have a regular file system.        */

/* FILEPOINTER */
struct filePointerStruct;

#define SIZEOF_FILEPOINTER       StructSizeInCells(filePointerStruct)

/* This pointerlist maintains a mapping between an integer file      */
/* descriptor and filepointers for resource files.  Added to support */
/* MIDP changes to ResourceInputStream.java for commonality between  */
/* KVM and Project Monty */
#define FILE_OBJECT_SIZE 4
extern POINTERLIST filePointerRoot;

/* This flag indicates whether class loading has been  */
/* initiated from Class.forName() or from elsewhere in */
/* the virtual machine. */
extern bool_t loadedReflectively;

/*=========================================================================
 * Class file verification operations (performed during class loading)
 *=======================================================================*/

enum validName_type {LegalMethod, LegalField, LegalClass};
bool_t isValidName(const char* name, enum validName_type);

/*=========================================================================
 * Class loading operations
 *=======================================================================*/

void loadClassfile(INSTANCE_CLASS CurrentClass, bool_t fatalErrorIfFail);
void loadArrayClass(ARRAY_CLASS);

/*=========================================================================
 * Generic class file reading operations
 *=======================================================================*/

/*
 * NOTE: The functions below define an abstract interface for
 * reading data from class files.  The actual implementations
 * of these functions are platform-dependent, and therefore are
 * not provided in VmCommon. An implementation of these functions
 * must be provided for each platform. A sample implementation
 * can be found in VmExtra/src/loaderFile.c.
 */
int            loadByteNoEOFCheck(FILEPOINTER_HANDLE);
int            loadBytesNoEOFCheck(FILEPOINTER_HANDLE, char *buffer, int pos, int length);
unsigned char  loadByte(FILEPOINTER_HANDLE);
unsigned short loadShort(FILEPOINTER_HANDLE);
unsigned long  loadCell(FILEPOINTER_HANDLE);
void           loadBytes(FILEPOINTER_HANDLE, char *buffer, int length);
void           skipBytes(FILEPOINTER_HANDLE, unsigned long i);
int            getBytesAvailable(FILEPOINTER_HANDLE);

void           InitializeClassLoading(void);
void           FinalizeClassLoading();

FILEPOINTER    openClassfile(INSTANCE_CLASS clazz);
FILEPOINTER    openResourcefile(BYTES resourceName);
void           closeClassfile(FILEPOINTER_HANDLE);

int            setFilePointer(FILEPOINTER_HANDLE);
FILEPOINTER    getFilePointer(int);
void           clearFilePointer(int);

/*=========================================================================
 * Helper functions
 *=======================================================================*/

char*          replaceLetters(char* string, char c1, char c2);

