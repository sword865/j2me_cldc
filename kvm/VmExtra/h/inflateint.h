/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: JAR file reader / inflater.
 * FILE:      inflateint.h
 * OVERVIEW:  Internal declarations for JAR inflater module.
 * AUTHOR:    Ioi Lam, Tasneem Sayeed, Frank Yellin
 *=======================================================================*/

#ifndef _INFLATE_INT_H_
#define _INFLATE_INT_H_

/*
 * The HuffmanCodeTable structure contains the dynamic huffman codes for
 * the Code Length Codes or the Distance Codes. The structure is
 * dynamically allocated. We just allocate enough space to contain all
 * possible codes.
 */
typedef struct HuffmanCodeTableHeader {
    unsigned short quickBits;   /* quick bit size */
    unsigned short maxCodeLen;  /* Max number of bits in any code */
} HuffmanCodeTableHeader;

/* If this bit is set in a huffman entry, it means that this is not
 * really an entry, but a pointer into the long codes table.
 * The remaining 15 bits is the offset (in bytes) from the table header
 * to first "long" entry representing this item.
 */
#define HUFFINFO_LONG_MASK 0x8000 /*  high bit set */

#define MAX_QUICK_CXD  6
#define MAX_QUICK_LXL  9

/* For debugging, the following can be set to the name of a file (in quotes).
 * If we are decompressing something that is the exact same length as that
 * file, this will check to make sure that the decompressed bytes look
 * exactly the same as the bytes in the specified file.
 * java/lang/String.class is a particularly useful file to try.
 *
 * That only works on machines that have stat(), and for which you
 * can include sys/stat.h
 *
 * #define INFLATE_DEBUG_FILE java/lang/String.class
 */
#if INCLUDEDEBUGCODE
    static void ziperr(const char * msg);
#if POCKETPC
#   define ASSERT(x) (void)0
#else
#   define ASSERT(x) assert((x))
#endif
#else
#   define ziperr(msg)
#   define ASSERT(x) (void)0
#endif

/*=========================================================================
 * JAR file reader defines and macros
 *=======================================================================*/

#define BTYPE_NO_COMPRESSION 0x00  
#define BTYPE_FIXED_HUFFMAN  0x01  /* Fixed Huffman Code */
#define BTYPE_DYNA_HUFFMAN   0x02  /* Dynamic Huffman code */
#define BTYPE_INVALID        0x03  /* Invalid code */

#define MAX_BITS 15   /* Maximum number of codes in Huffman Code Table */

#define LITXLEN_BASE 257

/* A normal sized huffman code table with a 9-bit quick bit */
typedef struct HuffmanCodeTable { 
    struct HuffmanCodeTableHeader h;
    /* There are 1 << quickBit entries.  512 is just an example. 
     * For each entry:
     *     If the high bit is 0:
     *        Next 11 bits give the character
     *        Low   4 bits give the actual number of bits used
     *     If the high bit is 1:
     *        Next 15 bits give the offset (in bytes) from the header to 
     *        the first long entry dealing with this long code.
     */
    unsigned short entries[512];
} HuffmanCodeTable;

/* A small sized huffman code table with a 9-bit quick bit.  We have
 * this so that we can initialize fixedHuffmanDistanceTable in jartables.h
 */
typedef struct shortHuffmanCodeTable { 
    struct HuffmanCodeTableHeader h;
    unsigned short entries[32];
} shortHuffmanCodeTable;

typedef struct inflaterState {
    /* The input stream */
    void *inFile;               /* The information */
    JarGetByteFunctionType getBytes;

    int inRemaining;            /* Number of bytes left that we can read */
    unsigned int inDataSize;    /* Number of good bits in inData */
    unsigned long inData;       /* Low inDataSize bits are from stream. */
                                /* High unused bits must be zero */
    /* The output stream */
    UNSIGNED_CHAR_HANDLE outFileH;
    unsigned long outOffset;
    unsigned long outLength;
#ifdef JAR_DEBUG_FILE  
    unsigned char *jarDebugBytes;
#endif
} inflaterState;

/*=========================================================================
 * Macros used internally
 *=======================================================================*/

/* Call this macro to make sure that we have at least "j" bits of
 * input available
 */
#define NEEDBITS(j) {                                         \
      while (inDataSize < (j)) {                              \
           inData |= ((unsigned long)NEXTBYTE) << inDataSize; \
           inRemaining--; inDataSize += 8;                    \
      }                                                       \
      ASSERT(inDataSize <= 32);                               \
}

/* Return (without consuming) the next "j" bits of the input */
#define NEXTBITS(j) \
       (ASSERT((j) <= inDataSize), inData & ((1 << (j)) - 1))

/* Consume (quietly) "j" bits of input, and make them no longer available
 * to the user
 */
#define DUMPBITS(j) {                                         \
       ASSERT((j) <= inDataSize);                             \
       inData >>= (j);                                        \
       inDataSize -= (j);                                     \
    }  

/* Read bits from the input stream and decode it using the specified
 * table.  The resulting decoded character is placed into "result".
 * If there is a problem, we goto "errorLabel"
 *
 * For speed, we assume that quickBits = table->h.quickBits and that
 * it has been cached into a variable.
 */
#define GET_HUFFMAN_ENTRY(table, quickBits, result, errorLabel) {  \
    unsigned int huff = table->entries[NEXTBITS(quickBits)];       \
    if (huff & HUFFINFO_LONG_MASK) {                               \
        long delta = (huff & ~HUFFINFO_LONG_MASK);                 \
        unsigned short *table2 = (unsigned short *)((char *)table + delta); \
        huff = table2[NEXTBITS(table->h.maxCodeLen) >> quickBits]; \
    }                                                              \
    if (huff == 0) { goto errorLabel; }                            \
    DUMPBITS(huff & 0xF);                                          \
    result = huff >> 4;                                            \
    }

#define INFLATEBUFFERSIZE 256
unsigned char inflateBuffer[INFLATEBUFFERSIZE];
int inflateBufferIndex;
int inflateBufferCount;

#define NEXTBYTE (inflateBufferCount-- > 0 ? (unsigned long)inflateBuffer[inflateBufferIndex++] : \
    ((inflateBufferCount = getBytes(inflateBuffer, INFLATEBUFFERSIZE, inFile)) > 0 ? \
    (inflateBufferIndex = 1, inflateBufferCount--, (unsigned long)inflateBuffer[0]) : 0xff))

#define DECLARE_IN_VARIABLES                         \
    register void* inFile = state->inFile;           \
    JarGetByteFunctionType getBytes = state->getBytes; \
    register unsigned long inData;                   \
    register unsigned int inDataSize;                \
    register long inRemaining;

/* Copy values from the inflaterState structure to local variables */
#define LOAD_IN                       \
    inData = state->inData;           \
    inDataSize = state->inDataSize;   \
    inRemaining = state->inRemaining; 

/* Copy values from local variables back to the inflaterState structure */
#define STORE_IN                      \
    state->inData = inData;           \
    state->inDataSize = inDataSize;   \
    state->inRemaining = inRemaining; 

#define DECLARE_OUT_VARIABLES                                  \
    register unsigned char *outFile = unhand(state->outFileH); \
    register unsigned long outLength = state->outLength;       \
    register unsigned long outOffset;

#define LOAD_OUT outOffset = state->outOffset;

#define STORE_OUT state->outOffset = outOffset;

#define UPDATE_IN_OUT_AFTER_POSSIBLE_GC  \
    outFile = unhand(state->outFileH); 

#endif /* INFLATE_INT_H_ */

