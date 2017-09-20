/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: JAR file reader / inflater.
 * FILE:      jar.c
 * OVERVIEW:  Structures and operations for reading a jar file.
 *            The JAR file can either be a file, or it can be mapped
 *            into memory.
 * AUTHOR:    Tasneem Sayeed, Consumer & Embedded division
 *            Frank Yellin
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>
#include <jar.h>
#include <inflate.h>

/* Assume that we're being compiled as part of the KVM, unless told
 * otherwise
 */
#ifndef COMPILING_FOR_KVM
#  define COMPILING_FOR_KVM 1
#endif

/* Change some definitions so that this compiles niceless, even if it
 * compiled as part of something that requires real malloc() and free()
 */
#if !COMPILING_FOR_KVM
#  undef START_TEMPORARY_ROOTS
#  undef END_TEMPORARY_ROOTS
#  undef DECLARE_TEMPORARY_ROOT
#  undef DECLARE_TEMPORARY_ROOT_FROM_BASE
#  define START_TEMPORARY_ROOTS {
#  define END_TEMPORARY_ROOTS }
#  define mallocBytes(x) malloc(x)
#  define DECLARE_TEMPORARY_ROOT(type, name, value) type name = value
#  define DECLARE_TEMPORARY_ROOT_FROM_BASE(type, name, value, base) \
         type name = value
#  define freeBytes(x)  free(x)
#else
#  define freeBytes(x)
#endif

#if JAR_FILES_USE_STDIO
#include <stdio.h>
#endif

/*=========================================================================
 * Forward declarations of static functions
 *=======================================================================*/

static void *loadJARFileEntryInternal(JAR_INFO entry, 
                                      const unsigned char *centralInfo, 
                                      long *lengthP, int extraBytes);

static unsigned long jarCRC32(unsigned char *data, unsigned long length);

static int jar_getBytes(char*, int, void* p);

/*=========================================================================
 * JAR Data Stream structure
 *=======================================================================*/

#define LOCSIG (('P' << 0) + ('K' << 8) + (3 << 16) + (4 << 24))
#define CENSIG (('P' << 0) + ('K' << 8) + (1 << 16) + (2 << 24))
#define ENDSIG (('P' << 0) + ('K' << 8) + (5 << 16) + (6 << 24))

/*
 * Supported compression types
 */
#define STORED      0
#define DEFLATED    8

/*
 * Header sizes including signatures
 */
#define LOCHDRSIZ 30
#define CENHDRSIZ 46
#define ENDHDRSIZ 22

/*
 * Header field access macros
 */
#define CH(b, n) ((long)(((unsigned char *)(b))[n]))
#define SH(b, n) ((long)(CH(b, n) | (CH(b, n+1) << 8)))
#define LG(b, n) ((long)(SH(b, n) | (SH(b, n+2) << 16)))

#define GETSIG(b) LG(b, 0)      /* signature */

/*
 * Macros for getting local file header (LOC) fields
 */
#define LOCVER(b) SH(b, 4)      /* version needed to extract */
#define LOCFLG(b) SH(b, 6)      /* encrypt flags */
#define LOCHOW(b) SH(b, 8)      /* compression method */
#define LOCTIM(b) LG(b, 10)     /* modification time */
#define LOCCRC(b) LG(b, 14)     /* uncompressed file crc-32 value */
#define LOCSIZ(b) LG(b, 18)     /* compressed size */
#define LOCLEN(b) LG(b, 22)     /* uncompressed size */
#define LOCNAM(b) SH(b, 26)     /* filename size */
#define LOCEXT(b) SH(b, 28)     /* extra field size */

/*
 * Macros for getting central directory header (CEN) fields
 */
#define CENVEM(b) SH(b, 4)      /* version made by */
#define CENVER(b) SH(b, 6)      /* version needed to extract */
#define CENFLG(b) SH(b, 8)      /* general purpose bit flags */
#define CENHOW(b) SH(b, 10)     /* compression method */
#define CENTIM(b) LG(b, 12)     /* file modification time (DOS format) */
#define CENCRC(b) LG(b, 16)     /* crc of uncompressed data */
#define CENSIZ(b) LG(b, 20)     /* compressed size */
#define CENLEN(b) LG(b, 24)     /* uncompressed size */
#define CENNAM(b) SH(b, 28)     /* length of filename */
#define CENEXT(b) SH(b, 30)     /* length of extra field */
#define CENCOM(b) SH(b, 32)     /* file comment length */
#define CENDSK(b) SH(b, 34)     /* disk number start */
#define CENATT(b) SH(b, 36)     /* internal file attributes */
#define CENATX(b) LG(b, 38)     /* external file attributes */
#define CENOFF(b) LG(b, 42)     /* offset of local header */

/*
 * Macros for getting end of central directory header (END) fields
 */
#define ENDSUB(b) SH(b, 8)      /* number of entries on this disk */
#define ENDTOT(b) SH(b, 10)     /* total number of entries */
#define ENDSIZ(b) LG(b, 12)     /* central directory size */
#define ENDOFF(b) LG(b, 16)     /* central directory offset */
#define ENDCOM(b) SH(b, 20)     /* size of zip file comment */

/*=========================================================================
 * JAR file reading operations
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      openJARFile
 * OVERVIEW:      Opens a Jar file so that it can be read using
 *                loadJARFileEntry or by loadJARFileEntries
 *
 * This function has two slightly different calling sequences, depending on
 * whether JAR files are read using STDIO (JAR_FILES_USE_STDIO) or whether
 * they are already in memory:
 *   parameters:  
 *      nameOrAddress: 
 *           JAR_FILES_USE_STDIO:  name of the jar file
 *          !JAR_FILES_USE_STDIO:  address of the jar file
 *      length:
 *           JAR_FILES_USE_STDIO:  ignored
 *          !JAR_FILES_USE_STDIO:  length of the jar file
 *      entry:
 *          Address of data structure that is filled in with information about
 *          the JAR file, if the "open" is successful.
 *
 *   returns:
 *      TRUE if the "open" succeeds, false otherwise.
 *=======================================================================*/

bool_t
openJARFile(void *nameOrAddress, int length, JAR_INFO entry)
{
    long currentOffset, minOffset;
    unsigned const char *bp;

#if JAR_FILES_USE_STDIO
    unsigned char *buffer = (unsigned char *)str_buffer;
    unsigned const int bufferSize = STRINGBUFFERSIZE;

    FILE *file = fopen((char *)nameOrAddress, "rb");
    if (file == NULL) {
        goto failureReturn;
    }
    /* Get the length of the file */
    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);
#else
    const unsigned char *jarFile = nameOrAddress;
    const unsigned char *buffer;
#endif /* JAR_FILES_USE_STDIO */

    /* Calculate the smallest possible offset for the end header.  It
     * can be at most 0xFFFF + ENDHDRSIZ bytes from the end of the file, but
     * the file must also have a local header and a central header
     */
    minOffset = length - (0xFFFF + ENDHDRSIZ);
    if (minOffset < LOCHDRSIZ + CENHDRSIZ) {
        minOffset = LOCHDRSIZ + CENHDRSIZ;
    }

    /* In order to simplify the code, both JAR_FILES_USE_STDIO and
     * !JAR_FILES_USE_STDIO assume that "buffer" contains the contents
     * of part of the file.  currentOffset contains the offset of buffer[0].
     *
     * For JAR_FILES_USE_STDIO, buffer is a temporary buffer into which we
     * read the contents of the file.  For !JAR_FILES_USE_STDIO, buffer
     * is a pointer into the actual jar file.
     */

#if JAR_FILES_USE_STDIO
    /* Read in the last ENDHDRSIZ bytes into the buffer.  99% of the time,
     * the file won't have a comment, and this is the only read we'll need */
    if (   (fseek(file, -ENDHDRSIZ, SEEK_END) < 0)
        || (fread(buffer, sizeof(char), ENDHDRSIZ, file) != ENDHDRSIZ)) {
        goto failureReturn;
    }
    /* Set currentOffset to be the offset of buffer[0] */
    currentOffset = length - ENDHDRSIZ; 
    /* Set bp to be the location at which to start looking */
    bp = buffer;
#else
    /* Create a pseudo buffer, that actually points at the jarFile */
    buffer = jarFile + minOffset;
    /* Set currentOffset to be the offset of buffer[0] */
    currentOffset = minOffset; 
    /* Set bp to be the location at which to start looking */
    bp = jarFile + length - ENDHDRSIZ;  /* Where to start looking */
#endif /* JAR_FILES_USE_STDIO */

    for (;;) {
        /* "buffer" contains a block of data from the file, starting at
         * currentOffset "position" in the file.
         * We investigate whether   currentOffset + (bp - buffer)  is the start
         * of the end header in the zip file.  
         *
         * We use a simplified version of Knuth-Morris-Pratt search algorithm.
         * The header we're looking for is 'P' 'K' 5  6
         */
        switch(bp[0]) {
            case '\006':   /* The header must start at least 3 bytes back */
                bp -= 3; break;
            case '\005':   /* The header must start at least 2 bytes back  */
                bp -= 2; break;
            case 'K':      /* The header must start at least 1 byte back  */
                bp -= 1; break;
            case 'P':      /* Either this is the header, or the header must
                            * start at least 4  back */
                if (bp[1] == 'K' && bp[2] == 5 && bp[3] == 6) {
                    /* We have what may be a header.  Let's make sure the
                     * implied length of the jar file matches the actual
                     * length.
                     */
                    int endpos = currentOffset + (bp - buffer);
                    if (endpos + ENDHDRSIZ + ENDCOM(bp) == length) {
                        unsigned long cenOffset = endpos - ENDSIZ(bp);
                        unsigned long locOffset = cenOffset - ENDOFF(bp);
#if JAR_FILES_USE_STDIO
                        if (   fseek(file, locOffset, SEEK_SET) >= 0
                            && getc(file) == 'P' && getc(file) == 'K'
                            && getc(file) == 3   && getc(file) == 4) {
                            entry->u.jar.file = file;
                            entry->u.jar.cenOffset = cenOffset;
                            entry->u.jar.locOffset = locOffset;
                            return TRUE;
                        }
#else
                        const unsigned char *cenPtr = jarFile + cenOffset;
                        const unsigned char *locPtr = jarFile + locOffset;
                        if (GETSIG(locPtr) == LOCSIG) { 
                            entry->u.mjar.base   = jarFile;
                            entry->u.mjar.cenPtr = cenPtr;
                            entry->u.mjar.locPtr = locPtr;
                            return TRUE;
                        }
#endif /* JAR_FILES_USE_STDIO */

                        goto failureReturn;
                    }
                }
                /* FALL THROUGH */
            default:    
                /* The header must start at least four characters back, since
                 * the current character isn't in the header */
                bp -= 4;
        }
        if (bp < buffer) {
#if JAR_FILES_USE_STDIO
            /* We've moved outside our window into the file.  We must
             * move the window backwards */
            int count = currentOffset - minOffset; /* Bytes left in file */
            if (count <= 0) {
                /* Nothing left to read.  Time to give up */
                goto failureReturn;
            } else {
                /* up to ((bp - buffer) + ENDHDRSIZ) bytes in the buffer might
                 * still be part of the end header, so the most bytes we can
                 * actually read are
                 *      bufferSize - ((bp - buffer) + ENDHDRSIZE).
                 */
                int available = (bufferSize - ENDHDRSIZ) + (buffer - bp);
                if (count > available) {
                    count = available;
                }
            }
            /* Back up, while keeping our virtual currentOffset the same */
            currentOffset -= count;
            bp += count;
            memmove(buffer + count, buffer, bufferSize - count);
            if (   (fseek(file, currentOffset, SEEK_SET) < 0)
                   || (fread(buffer, sizeof(char), count, file) 
                                 != (unsigned)count)) {
                goto failureReturn;
            }
#else
            /* There's nothing left to do. */
            goto failureReturn;
#endif /* JAR_FILE_USE_STDIO */
        }
    } /* end of for loop */

failureReturn:    
#if JAR_FILES_USE_STDIO
    if (file != NULL) { 
        fclose(file);
    }
#endif /* JAR_FILES_USE_STDIO */

    return FALSE;
}

/*=========================================================================
 * FUNCTION:      closeJARFile
 * OVERVIEW:      Close a jar file that had previous been opened by 
 *                openJARFile
 *   parameters:  
 *      JAR_INFO:  structure previously filled in by openJARFile
 *
 *   returns:
 *      nothing
 *=======================================================================*/

void
closeJARFile(JAR_INFO entry)
{
#if JAR_FILES_USE_STDIO
    fclose((FILE *)entry->u.jar.file);
#endif
}

/*=========================================================================
 * FUNCTION:      loadJARFileEntry()
 * OVERVIEW:      Reads an entry in a jar file
 *
 * INTERFACE:
 *   parameters:  JAR_INFO: structure returned by openJARFile
 *                filename: name of entry to read
 *                lengthP:  on return, contains length of entry in jar file
 *                          (does >>NOT<< include extraBytes)
 *                extraBytes:  value has this many extra bytes padded at the
 *                          beginning.
 *
 * NOTE: The result is malloc'ed on the heap.  It is up to the caller to protect
 *        this result from garbage collection
 *=======================================================================*/

void *
loadJARFileEntry(JAR_INFO entry, const char *filename,
                 long *lengthP, int extraBytes)
{
    unsigned int filenameLength = strlen(filename);
    unsigned int nameLength;

#if JAR_FILES_USE_STDIO
    unsigned char *p = (unsigned char *)str_buffer; /* temporary storage */
    int offset = entry->u.jar.cenOffset; /* offset of first header */
    FILE *file = entry->u.jar.file;
#else 
    unsigned const char *p = entry->u.mjar.cenPtr; /* pointer to first header */
#endif

    while(TRUE) { 
#if JAR_FILES_USE_STDIO        
        /* Offset contains the offset of the next central header. Read the
         * header into the temporary buffer */
        if (/* Go to the header */
               (fseek(file, offset, SEEK_SET) < 0) 
            /* Read the bytes */
            || (fread(p, sizeof(char), CENHDRSIZ, file) != CENHDRSIZ)) { 
            return NULL;
        }
#endif
        /* p contains the current central header */
        if (GETSIG(p) != CENSIG) { 
            /* We've reached the end of the headers */
            return NULL;
        } 
        nameLength = CENNAM(p);
        if (nameLength == filenameLength) { 
#if JAR_FILES_USE_STDIO
            if (fread(p + CENHDRSIZ, sizeof(char), nameLength, file)
                      != nameLength) {
                return NULL;
            }
#endif
            if (memcmp(p + CENHDRSIZ, filename, nameLength) == 0) { 
                break;
            } 
        }

#if JAR_FILES_USE_STDIO
        /* Set offset to the next central header */
        offset += CENHDRSIZ + nameLength + CENEXT(p) + CENCOM(p);
#else 
        /* Have p point to the next central header */
        p += CENHDRSIZ + nameLength + CENEXT(p) + CENCOM(p);
#endif
    }
    return loadJARFileEntryInternal(entry, p, lengthP, extraBytes);
}

/*=========================================================================
 * FUNCTION:      loadJARFileEntries()
 * OVERVIEW:      Reads multiple jar entries from a jar file
 *
 * INTERFACE:
 *   parameters:  JAR_INFO: structure returned by openJARFile
 *                testFunction: callback to determine whether to read
 *                      the specified file
 *                runFunction:  callback called after each jar file is read.
 *                info:  Info passed to testFunction and runFunction.
 *
 * NOTE: The jar files are malloc'ed on the heap.  It is up to the caller
 * to ensure that they are handled correctly.
 *
 * The testFunction is called as:
 *     bool_t testFunction(const char *name, int nameLength, 
 *                         int *extraBytes, void *info)
 * It should return TRUE or FALSE, indicating whether to read the specified
 * jar file or not.  
 *      name:        name of entry (not NULL terminated)
 *      nameLength:  length of name
 *      extraBytes:  *extraBytes contains 0, but the test function can
 *                   change this to indicate that padding is needed at the
 *                   beginning of the entry.
 *      info:        Arg passed to loadJARFileEntries
 * 
 * The runFunction is called for any entry for which the testFunction
 * returned TRUE
 *     void runFunction(const char *name, int nameLength, 
 *                      void *value, long length, void*info);
 * The arguments are as follows:
 *     name:          name of entry (not NULL terminated)
 *     nameLength:    length of name
 *     value:         Decoded jar entry, or NULL if a problem.
 *     length:        Length of jar file entry (not included extra bytes)
 *     info:          Arg passed to loadJARFileEntries
 *=======================================================================*/

void
loadJARFileEntries(JAR_INFO entry, 
                   JARFileTestFunction testFunction,
                   JARFileRunFunction runFunction, 
                   void *info)
{
    const char *name;
    unsigned int nameLength;

#if JAR_FILES_USE_STDIO
    unsigned char *p = (unsigned char *)str_buffer; /* temporary storage */
    FILE *file = entry->u.jar.file;
    unsigned long offset = entry->u.jar.cenOffset;
#else
    /* Start at the beginning of the central header */
    unsigned const char *p = entry->u.mjar.cenPtr; 
#endif
    for (;;) { 

#if JAR_FILES_USE_STDIO
        if (/* Go to the next central header */
               (fseek(file, offset, SEEK_SET) < 0) 
            /* Read the bytes */
            || (fread(p, sizeof(char), CENHDRSIZ, file) != CENHDRSIZ) 
           ) { 
              break;
        }
        /* Set offset to the next central header */
#endif
        if (GETSIG(p) != CENSIG) { 
            /* We've reached the end of the headers */
            break;
        }

        name = (const char *)p + CENHDRSIZ;
        nameLength = CENNAM(p);
        
#if JAR_FILES_USE_STDIO
        if (fread(p+CENHDRSIZ, sizeof(char), nameLength, file) != nameLength) {
            break;
        }
        /* We need to update the offset now, rather than later, since
         * the temporary buffer might get overwritten.
         */
        offset += CENHDRSIZ + nameLength + CENEXT(p) + CENCOM(p);
#endif
        if (name[nameLength - 1] != '/') { 
            START_TEMPORARY_ROOTS
                DECLARE_TEMPORARY_ROOT(JAR_INFO, entryX, entry);
                int extraBytes = 0;
                if (testFunction(name, nameLength, &extraBytes, info)) { 
                    long length;
                    unsigned char *value = 
                        loadJARFileEntryInternal(entryX, p, &length, extraBytes);
                    runFunction(name, nameLength, value, length, info);
                }
                entry = entryX;
            END_TEMPORARY_ROOTS
        }
#if !JAR_FILES_USE_STDIO
        p += CENHDRSIZ + nameLength + CENEXT(p) + CENCOM(p);
#endif  
    }
}

/*=========================================================================
 * FUNCTION:      loadJARFileEntryInternal()
 * OVERVIEW:      Internal function for reading a jar file
 *
 * parameters:  
 *     JAR_INFO: structure returned by openJARFile.
 *     centralInfo:  pointer to info in central directory for this entry
 *     lengthP:  On return, contains length of the entry (not including
 *             padding caused by extraBytes)
 *     extraBytes:  Pad the entry with this many extra bytes at the front.
 * returns:
 *     result of decompressing the Jar entry.
 *
 * NOTE:  If JAR_FILES_USE_STDIO, then centralInfo points at str_buffer,
 *        which contains the central header.  If !JAR_FILES_USE_STDIO, then
 *        centralInfo points at the actual bytes.
 */    

static void *
loadJARFileEntryInternal(JAR_INFO entry, const unsigned char *centralInfo, 
                         long *lengthP, int extraBytes) 
{
    unsigned long decompLen = CENLEN(centralInfo); /* the decompressed length */
    unsigned long compLen   = CENSIZ(centralInfo); /* the compressed length */
    unsigned long method    = CENHOW(centralInfo); /* how it is stored */
    unsigned long expectedCRC = CENCRC(centralInfo); /* expected CRC */
    unsigned long actualCRC;
    unsigned char *result = NULL;

#if JAR_FILES_USE_STDIO
    FILE *file = entry->u.jar.file;
    unsigned long locOffset = entry->u.jar.locOffset;
    unsigned char *p = (unsigned char *)str_buffer;
#else 
    unsigned const char *locPtr = entry->u.mjar.locPtr; 
    const unsigned char *p;
#endif

    /* Make sure file is not encrypted */
    if ((CENFLG(centralInfo) & 1) == 1) {
        goto errorReturn;
    }

    /* This may cause a GC, so we have to extract out of "entry" all the
     * info we need, before calling this.
     */
    result = (unsigned char *)mallocBytes(extraBytes + decompLen);
#if !COMPILING_FOR_KVM
    if (result == NULL) {
        goto errorReturn;
    }
#endif

#if JAR_FILES_USE_STDIO
    if (/* Go to the beginning of the LOC header */
        (fseek(file, locOffset + CENOFF(centralInfo), SEEK_SET) < 0) 
        /* Read it */
        || (fread(p, sizeof(char), LOCHDRSIZ, file) != LOCHDRSIZ) 
        /* Skip over name and extension, if any */
        || (fseek(file, LOCNAM(p) + LOCEXT(p), SEEK_CUR) < 0)) {
        goto errorReturn;
    }
#else 
    /* Go to the beginning of the LOC header */
    p = locPtr + CENOFF(centralInfo);
    /* Skip over the actual bits of the header */
    p += LOCHDRSIZ + LOCNAM(p) + LOCEXT(p);
#endif

    switch (method) { 
        case STORED:
            /* The actual bits are right there in the file */
            if (compLen != decompLen) {
                goto errorReturn;
            }
#if JAR_FILES_USE_STDIO         
            fread(result + extraBytes, sizeof(char), decompLen, file);
#else
            memcpy(result + extraBytes, p, decompLen);
#endif
            break;

        case DEFLATED: {
            bool_t inflateOK;
            START_TEMPORARY_ROOTS
                DECLARE_TEMPORARY_ROOT_FROM_BASE(unsigned char*, 
                                                 decompData, 
                                                 result + extraBytes, result);
#if JAR_FILES_USE_STDIO
                void *arg = file;
#else
                void *arg = &p;
#endif
                inflateOK = inflateData(arg, 
                               (JarGetByteFunctionType)jar_getBytes, compLen, 
                                    &decompData, decompLen);
                /* The inflater can allocate memory, so we need to regenerate
                 * value from decompData. */
                result = decompData - extraBytes;
            END_TEMPORARY_ROOTS
            if (!inflateOK) { 
                goto errorReturn;
            } 
            break;
        }
                
        default:
            /* Unknown method */
            goto errorReturn;
            break;
    }

    if (result != NULL) { 
        actualCRC = jarCRC32(result + extraBytes, decompLen);
        if (actualCRC != expectedCRC) { 
            goto errorReturn;
        }
    }
    *lengthP = decompLen;
    return (void *)result;

errorReturn:    
    freeBytes(result);
    *lengthP = 0;
    return NULL;
}

/*=========================================================================
 * FUNCTION:      jarCRC32
 * OVERVIEW:      Returns the CRC of an array of bytes, using the same
 *                algorithm as used by the JAR reader.
 * INTERFACE:
 *   parameters:  data:     pointer to the array of bytes
 *                length:   length of data, in bytes
 *   returns:     CRC
 *=======================================================================*/

static unsigned long
jarCRC32(unsigned char *data, unsigned long length) {
    unsigned long crc = 0xFFFFFFFF;
    unsigned int j;
    for ( ; length > 0; length--, data++) {
        crc ^= *data;
        for (j = 8; j > 0; --j) {
            crc = (crc & 1) ? ((crc >> 1) ^ 0xedb88320) : (crc >> 1);
        }
    }
    return ~crc;
}

/*
 * Callback passed to inflate, to read the next byte of data.
 */
static int
jar_getBytes(char *buff, int length, void* p) {
#if JAR_FILES_USE_STDIO
    return fread(buff, sizeof(char), length, (FILE *)p); 
#else
    return *(*(unsigned char **)p)++;
#endif
}

