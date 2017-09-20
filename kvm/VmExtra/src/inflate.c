/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: JAR file reader / inflater.
 * FILE:      inflate.c
 * OVERVIEW:  Routines for inflating (decompressing) the contents 
 *            of a JAR file. The routines are optimized to reduce
 *            code size and run-time memory requirement so that 
 *            they can run happily on small devices.
 * AUTHOR:    Ioi Lam, Consumer & Embedded, Sun Microsystems, Inc.
 *            Refined by Tasneem Sayeed, Consumer & Embedded
 *            Frank Yellin
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#ifdef INFLATE_DEBUG_FILE
#include <sys/stat.h>
#endif

#include <global.h>

#ifndef POCKETPC
#include <assert.h>
#endif

#include <jar.h>

/* Assume that we're being compiled as part of the KVM, unless told
 * otherwise
 */
#ifndef COMPILING_FOR_KVM
#  define COMPILING_FOR_KVM 1
#endif

#include <inflate.h>
#include <inflateint.h>
#include <inflatetables.h>

static bool_t decodeDynamicHuffmanTables(inflaterState *state,
                                         HuffmanCodeTable **lcodesPtr,
                                         HuffmanCodeTable **dcodesPtr);

static HuffmanCodeTable *makeCodeTable(inflaterState *state,
                                       unsigned char *codelen,
                                       unsigned numElems,
                                       unsigned maxQuickBits);

static bool_t inflateHuffman(inflaterState *state, bool_t fixedHuffman);
static bool_t inflateStored(inflaterState *state);

/*=========================================================================
 * Decompression functions
 *=======================================================================*/

/*===========================================================================
 * FUNCTION:  inflateData
 * TYPE:      jar file decoding
 * INTERFACE:
 *   parameters: method, compressed data, compressed length,
 *               decompressed data, decompressed size
 *   returns:    TRUE if the data was encoded in a supported <method> and the
 *                  size of the decoded data is exactly the same as <decompLen>
 *               FALSE if an error occurs
 * NOTE:
 *    The caller of this method must insure that this function can safely
 *    up to INFLATER_EXTRA_BYTES beyond compData + compLen without causing
 *    any problems.
 *    The inflater algorithm occasionally reads one more byte than it needs
 *    to.  But it double checks that it doesn't actually care what's in that
 *    extra byte.
 *===========================================================================*/

/* Change some definitions so that this compiles niceless, even if it
 * compiled as part of something that requires real malloc() and free()
 */
#if !COMPILING_FOR_KVM
#  undef START_TEMPORARY_ROOTS
#  undef END_TEMPORARY_ROOTS
#  undef ASSERTING_NO_ALLOCATION 
#  undef END_ASSERTING_NO_ALLOCATION 
#  undef INDICATE_DYNAMICALLY_INSIDE_TEMPORARY_ROOTS
#  undef IS_TEMPORARY_ROOT
#  define START_TEMPORARY_ROOTS
#  define END_TEMPORARY_ROOTS
#  define ASSERTING_NO_ALLOCATION 
#  define END_ASSERTING_NO_ALLOCATION 
#  define INDICATE_DYNAMICALLY_INSIDE_TEMPORARY_ROOTS
#  define IS_TEMPORARY_ROOT(var, value) var = value
#  define mallocBytes(x) malloc(x)
#  define freeBytes(x)   if (x == NULL) {} else free(x)
#else
#  define freeBytes(x)
#endif

/* These three macros are provided because some ports may want to 
 * put the output bytes into something other than the provided outFile
 * buffer. 
 */

#ifndef  INFLATER_PUT_BYTE
#    define INFLATER_PUT_BYTE(offset, value)  outFile[offset] = value;
#endif
#ifndef INFLATER_GET_BYTE
#    define INFLATER_GET_BYTE(offset)         outFile[offset]
#endif
#ifndef INFLATER_FLUSH_OUTPUT
#    define INFLATER_FLUSH_OUTPUT()
#endif

bool_t
inflateData(void *compData, /* compressed data source */
        JarGetByteFunctionType getBytes,
        int compLen,        /* length of compressed data */
        UNSIGNED_CHAR_HANDLE decompData,
        int decompLen)      /* length of decompression buffer */
{
    inflaterState stateStruct;
    bool_t result;
    inflateBufferIndex = inflateBufferCount = 0;
/* Temporarily define state, so that LOAD_IN, LOAD_OUT, etc. macros work */
#define state (&stateStruct)
    stateStruct.outFileH = decompData;
    stateStruct.outOffset = 0;
    stateStruct.outLength = decompLen;

    stateStruct.inFile = compData;
    stateStruct.inData = 0;
    stateStruct.inDataSize = 0;
    stateStruct.inRemaining = compLen + INFLATER_EXTRA_BYTES;
    stateStruct.getBytes = getBytes;

#ifdef INFLATE_DEBUG_FILE
    {
        static int length = 0;
        if (length == 0) {
            struct stat stat_buffer;
            stat(INFLATE_DEBUG_FILE, &stat_buffer);
            length = stat_buffer.st_size;;
        }
        if (length == decompLen) {
            FILE *f = fopen(INFLATE_DEBUG_FILE, "rb");
            state->jarDebugBytes = malloc(length);
            fseek(f, 0, SEEK_SET);
            fread(state->jarDebugBytes, sizeof(char), length, f);
            fclose(f);
        } else {
            state->jarDebugBytes = NULL;
        }
    }
#endif

    for(;;) {
        int type;
        DECLARE_IN_VARIABLES

        LOAD_IN;
        NEEDBITS(3);
        type = NEXTBITS(3);
        DUMPBITS(3);
        STORE_IN;

        switch (type >> 1) {
            default:
            case BTYPE_INVALID:
                ziperr(KVM_MSG_JAR_INVALID_BTYPE);
                result = FALSE;
                break;

            case BTYPE_NO_COMPRESSION:
                result = inflateStored(state);
                break;

            case BTYPE_FIXED_HUFFMAN:
                result = inflateHuffman(state, TRUE);
                break;

            case BTYPE_DYNA_HUFFMAN:
                START_TEMPORARY_ROOTS
                    result = inflateHuffman(state, FALSE);
                END_TEMPORARY_ROOTS
                break;
        }
        if (!result) { 
            break;
        } else if (type & 1) { 
            INFLATER_FLUSH_OUTPUT();
            if (state->inRemaining + (state->inDataSize >> 3) != 
                             INFLATER_EXTRA_BYTES) {
                ziperr(KVM_MSG_JAR_INPUT_BIT_ERROR);
                result = FALSE;
            } else if (state->outOffset != state->outLength) {
                ziperr(KVM_MSG_JAR_OUTPUT_BIT_ERROR);
                result = FALSE;
            }
            break;
        }
    }
#ifdef INFLATE_DEBUG_FILE
    if (state->jarDebugBytes != NULL) {
        free(state->jarDebugBytes);
    }
#endif

    /* Remove temporary definition of state defined above */
#undef state

    return result;
}

static bool_t
inflateStored(inflaterState *state)
{
    DECLARE_IN_VARIABLES
    DECLARE_OUT_VARIABLES
    unsigned len, nlen;

    LOAD_IN; LOAD_OUT;

    DUMPBITS(inDataSize & 7);   /* move to byte boundary */
    NEEDBITS(32)
    len = NEXTBITS(16);
    DUMPBITS(16);
    nlen = NEXTBITS(16);
    DUMPBITS(16);

    ASSERT(inDataSize == 0);

    if (len + nlen != 0xFFFF) {
        ziperr(KVM_MSG_JAR_BAD_LENGTH_FIELD);
        return FALSE;
    } else if (inRemaining < len) {
        ziperr(KVM_MSG_JAR_INPUT_OVERFLOW);
        return FALSE;
    } else if (outOffset + len > outLength) {
        ziperr(KVM_MSG_JAR_OUTPUT_OVERFLOW);
        return FALSE;
    } else {
        int count;
        while (len > 0) {
          if (inflateBufferCount > 0) {
            /* we have data buffered, copy it first */
            memcpy(&outFile[outOffset], &inflateBuffer[inflateBufferIndex],
                   (count = (inflateBufferCount <= len ? inflateBufferCount : len)));
            len -= count;
            inflateBufferCount -= count;
            inflateBufferIndex += count;
            outOffset += count;
            inRemaining -= count;
          }
          if (len > 0) {
            /* need more, refill the buffer */
            outFile[outOffset++] = NEXTBYTE;
            len--;
            inRemaining--;
          }
       }
    }

    STORE_IN;
    STORE_OUT;
    return TRUE;
}

static bool_t
inflateHuffman(inflaterState *state, bool_t fixedHuffman)
{
    bool_t noerror = FALSE;
    DECLARE_IN_VARIABLES
    DECLARE_OUT_VARIABLES

    unsigned int quickDataSize = 0, quickDistanceSize = 0;
    unsigned int code, litxlen;
    HuffmanCodeTable *lcodes, *dcodes;

    if (!fixedHuffman) {

        INDICATE_DYNAMICALLY_INSIDE_TEMPORARY_ROOTS;
        IS_TEMPORARY_ROOT(lcodes, NULL);
        IS_TEMPORARY_ROOT(dcodes, NULL);
        if (!decodeDynamicHuffmanTables(state, &lcodes, &dcodes)) {
            noerror = TRUE;
            goto done;
        }

        quickDataSize = lcodes->h.quickBits;
        quickDistanceSize = dcodes->h.quickBits;
        UPDATE_IN_OUT_AFTER_POSSIBLE_GC;
    }

    LOAD_IN;
    LOAD_OUT;

    for (;;) {
        if (inRemaining < 0) {
            goto done_loop;
        }
        NEEDBITS(MAX_BITS + MAX_ZIP_EXTRA_LENGTH_BITS);

        if (fixedHuffman) {
            /*   literal (hex)
             * 0x100 - 0x117   7   0.0000.00   -  0.0101.11
             *     0 -    8f   8   0.0110.000  -  1.0111.111
             *   118 -   11f   8   1.1000.000  -  1.1000.111
             *    90 -    ff   9   1.1001.0000 -  1.1111.1111
             */

            /* Get 9 bits, and reverse them. */
            code = NEXTBITS(9);
            code = REVERSE_9BITS(code);

            if (code <  0x060) {
                /* A 7-bit code  */
                DUMPBITS(7);
                litxlen = 0x100 + (code >> 2);
            } else if (code < 0x190) {
                DUMPBITS(8);
                litxlen = (code >> 1) + ((code < 0x180) ? (0x000 - 0x030)
                          : (0x118 - 0x0c0));
            } else {
                DUMPBITS(9);
                litxlen = 0x90 + code - 0x190;
            }
        } else {
            GET_HUFFMAN_ENTRY(lcodes, quickDataSize, litxlen, done_loop);
        }

        if (litxlen <= 255) {
            if (outOffset < outLength) {
#ifdef INFLATE_DEBUG_FILE
                if (state->jarDebugBytes
                    && state->jarDebugBytes[outOffset] != litxlen) {
                    ziperr(KVM_MSG_JAR_DRAGON_SINGLE_BYTE);
                }
#endif
                INFLATER_PUT_BYTE(outOffset, litxlen);
                outOffset++;
            } else {
                goto done_loop;
            }
        } else if (litxlen == 256) {               /* end of block */
            noerror = TRUE;
            goto done_loop;
        } else if (litxlen > 285) {
            ziperr(KVM_MSG_JAR_INVALID_LITERAL_OR_LENGTH);
            goto done_loop;
        } else {
            unsigned int n = litxlen - LITXLEN_BASE;
            unsigned int length = ll_length_base[n];
            unsigned int moreBits = ll_extra_bits[n];
            unsigned int d0, distance;

            /* The NEEDBITS(..) above took care of this */
            length += NEXTBITS(moreBits);
            DUMPBITS(moreBits);

            NEEDBITS(MAX_BITS);
            if (fixedHuffman) {
                d0 = REVERSE_5BITS(NEXTBITS(5));
                DUMPBITS(5);
            } else {
                GET_HUFFMAN_ENTRY(dcodes, quickDistanceSize, d0, done_loop);
            }

            if (d0 > MAX_ZIP_DISTANCE_CODE) {
                ziperr(KVM_MSG_JAR_BAD_DISTANCE_CODE);
                goto done_loop;
            }

            NEEDBITS(MAX_ZIP_EXTRA_DISTANCE_BITS)
            distance = dist_base[d0];
            moreBits = dist_extra_bits[d0];
            distance += NEXTBITS(moreBits);
            DUMPBITS(moreBits);

            if (outOffset < distance) {
                ziperr(KVM_MSG_JAR_COPY_UNDERFLOW);
                goto done_loop;
            } else if (outOffset + length > outLength) {
                ziperr(KVM_MSG_JAR_OUTPUT_OVERFLOW);
                goto done_loop;
            } else {
                unsigned long prev = outOffset - distance;
                unsigned long end = outOffset + length;
                unsigned char value;
                while (outOffset != end) {
#ifdef INFLATE_DEBUG_FILE
                    if (state->jarDebugBytes
                      && state->jarDebugBytes[outOffset] != outFile[prev]) {
                        ziperr(KVM_MSG_JAR_DRAGON_COPY_FAILED);
                    }
#endif
                    value = INFLATER_GET_BYTE(prev); 
                    INFLATER_PUT_BYTE(outOffset, value);
                    outOffset++;
                    prev++;
                }
            }
        }
    }

 done_loop:
    STORE_IN;
    STORE_OUT;

 done:
    if (!COMPILING_FOR_KVM && !fixedHuffman) {
        freeBytes(lcodes);
        freeBytes(dcodes);
    }

    return noerror;
}

/*=========================================================================
 * FUNCTION:  decodeDynamicHuffmanTables
 * TYPE:      Huffman code Decoding
 * INTERFACE:
 *   parameters: inData Pointer, Mask Pointer, Max InLimit,
 *               literal/length codes Pointer, Decodes Pointer
 *   returns:    TRUE if successful in decoding or
 *               FALSE if an error occurs
 *=======================================================================*/

static bool_t
decodeDynamicHuffmanTables(inflaterState *state,
               HuffmanCodeTable **lcodesPtr,
               HuffmanCodeTable **dcodesPtr) {
    DECLARE_IN_VARIABLES

    HuffmanCodeTable *ccodes = NULL; 

    int hlit, hdist, hclen;
    int i;
    unsigned int quickBits;
    unsigned char codelen[286 + 32];
    unsigned char *codePtr, *endCodePtr;

    LOAD_IN;

    /* 5 Bits: hlit,  # of Literal/Length codes  (257 - 286) 
     * 5 Bits: hdist, # of Distance codes        (1 - 32) 
     * 4 Bits: hclen, # of Code Length codes     (4 - 19) 
     */
    NEEDBITS(14);
    hlit = 257 + NEXTBITS(5);
    DUMPBITS(5);
    hdist = 1 + NEXTBITS(5);
    DUMPBITS(5);
    hclen = 4 + NEXTBITS(4);
    DUMPBITS(4);

    /*
     *  hclen x 3 bits: code lengths for the code length
     *  alphabet given just above, in the order: 16, 17, 18,
     *  0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
     *
     *  These code lengths are interpreted as 3-bit integers
     *  (0-7); as above, a code length of 0 means the
     *  corresponding symbol (literal/length or distance code
     *  length) is not used.
     */
    memset(codelen, 0x0, 19);
    for (i=0; i<hclen; i++) {
        NEEDBITS(3);    
        if (inRemaining < 0) {
            goto error;
        }
        codelen[(int)ccode_idx[i]] = NEXTBITS(3);
        DUMPBITS(3);
    }

    ccodes = makeCodeTable(state, codelen, 19, MAX_QUICK_CXD);

    /* DANGER:  ccodes is a heap object.   It can become
     * unusable anytime we allocate from the heap.
     */

    if (ccodes == NULL) {
        goto error;
    }
    quickBits = ccodes->h.quickBits;

    /*
     * hlit code lengths for the literal/length alphabet,
     * encoded using the code length Huffman code.
     *
     * hdist code lengths for the distance alphabet,
     * encoded using the code length Huffman code.
     *
     * The code length repeat codes can cross from the literal/length
     * to the code length alphabets.  In other words, all code lengths form
     * a single sequence of hlit + hdist
     */

    ASSERTING_NO_ALLOCATION
        memset(codelen, 0x0, sizeof(codelen));
        for (   codePtr = codelen, endCodePtr = codePtr + hlit + hdist;
            codePtr < endCodePtr; ) {

            int val;

            if (inRemaining < 0) {
                goto error;
            }

            NEEDBITS(MAX_BITS + 7); /* 7 is max repeat bits below */
            GET_HUFFMAN_ENTRY(ccodes, quickBits, val, error);

            /*
             *  0 - 15: Represent code lengths of 0 - 15
             *      16: Copy the previous code length 3 - 6 times.
             *          3 + (2 bits of length)
             *      17: Repeat a code length of 0 for 3 - 10 times.
             *          3 + (3 bits of length)
             *      18: Repeat a code length of 0 for 11 - 138 times
             *          11 + (7 bits of length)
             */
            if (val <= 15) {
                *codePtr++ = val;
            } else if (val <= 18) {
                unsigned repeat  = (val == 18) ? 11 : 3;
                unsigned bits    = (val == 18) ? 7 : (val - 14);

                repeat += NEXTBITS(bits); /* The NEEDBITS is above */
                DUMPBITS(bits);

                if (codePtr + repeat > endCodePtr) {
                    ziperr(KVM_MSG_JAR_BAD_REPEAT_CODE);
                }

                if (val == 16) {
                    if (codePtr == codelen) {
                        ziperr(KVM_MSG_JAR_BAD_REPEAT_CODE);
                        goto error;
                    }
                    memset(codePtr, codePtr[-1], repeat);
                } else {
                    /* The values have already been set to zero, above, so
                     * we don't have to do anything */
                }
                codePtr += repeat;
            } else {
                ziperr(KVM_MSG_JAR_BAD_CODELENGTH_CODE);
                goto error;
            }
        }
    END_ASSERTING_NO_ALLOCATION

    /* ccodes, at this point, is unusable */

    *lcodesPtr = makeCodeTable(state, codelen, hlit, MAX_QUICK_LXL);
    if (*lcodesPtr == NULL) {
        goto error;
    }

    *dcodesPtr = makeCodeTable(state, codelen + hlit, hdist, MAX_QUICK_CXD);
    if (*dcodesPtr == NULL) {
        goto error;
    }

    STORE_IN;
    if (!COMPILING_FOR_KVM) {
        freeBytes(ccodes);
    }
    return TRUE;

error:
    if (!COMPILING_FOR_KVM) {
        freeBytes(ccodes);
        freeBytes(*dcodesPtr);
        freeBytes(*lcodesPtr);
        *lcodesPtr = *dcodesPtr = NULL;
    }
    return FALSE;
}

/*=========================================================================
 * FUNCTION:  makeCodeTable
 * TYPE:      Huffman code table creation
 * INTERFACE:
 *   parameters: code length, number of elements, type of the alphabet,
 *               maxQuickBits
 *   returns:    Huffman code table created if successful or
 *               NULL if an error occurs
 *=======================================================================*/

HuffmanCodeTable *
makeCodeTable(inflaterState *state,
              unsigned char *codelen, /* Code lengths */
              unsigned numElems,      /* Number of elements of the alphabet */
              unsigned maxQuickBits)  /* If the length of a code is longer than
                                       * <maxQuickBits> number of bits, the
                                       * code is stored in the sequential
                                       * lookup table instead of the quick 
                                       * lookup array. */
{
    unsigned int bitLengthCount[MAX_BITS + 1];
    unsigned int codes[MAX_BITS + 1];
    unsigned bits, minCodeLen = 0, maxCodeLen = 0;
    const unsigned char *endCodeLen = codelen + numElems;
    unsigned int code, quickMask;
    unsigned char *p;

    HuffmanCodeTable * table;
    int mainTableLength, longTableLength, numLongTables;
    int tableSize;
    int j;

    unsigned short *nextLongTable;

    /* Count the number of codes for each code length */
    memset(bitLengthCount, 0, sizeof(bitLengthCount));
    for (p = codelen; p < endCodeLen; p++) {
        bitLengthCount[*p]++;
    }

    if (bitLengthCount[0] == numElems) {
        ziperr(KVM_MSG_JAR_CODE_TABLE_EMPTY);
        return NULL;
    }

    /* Find the minimum and maximum.  It's faster to do it in a separate
     * loop that goes 1..MAX_BITS, than in the above loop that looks at
     * every code element */
    code = minCodeLen = maxCodeLen = 0;
    for (bits = 1; bits <= MAX_BITS; bits++) {
        codes[bits] = code;
        if (bitLengthCount[bits] != 0) {
            if (minCodeLen == 0) { 
                minCodeLen = bits;
            }
            maxCodeLen = bits;
            code += bitLengthCount[bits] << (MAX_BITS - bits);
        }
    }

#if INCLUDEDEBUGCODE
    if (code != (1 << MAX_BITS)) {
        code += (1 << (MAX_BITS - maxCodeLen));
        if (code != (1 << MAX_BITS)) {
            ziperr(KVM_MSG_JAR_UNEXPECTED_BIT_CODES);
        }
    }
#endif /* INCLUDEDEBUGCODE */

    /* Calculate the size of the code table and allocate it. */
    if (maxCodeLen <= maxQuickBits) {
        /* We don't need any subtables.  We may even be able to get
         * away with a table smaller than maxCodeLen
         */
        maxQuickBits = maxCodeLen;
        mainTableLength = (1 << maxCodeLen);
        numLongTables = longTableLength = 0;
    } else {
        mainTableLength = (1 << maxQuickBits);
        numLongTables = (1 << MAX_BITS) - codes[maxQuickBits + 1];
        numLongTables = numLongTables >> (MAX_BITS - maxQuickBits);
        longTableLength = 1 << (maxCodeLen - maxQuickBits);
    }

    ASSERT(mainTableLength == 1 << maxQuickBits);
    tableSize = sizeof(HuffmanCodeTableHeader)
          + (mainTableLength + numLongTables * longTableLength)
          * sizeof(table->entries[0]);
    table = (HuffmanCodeTable*)mallocBytes(tableSize);
#if !COMPILING_FOR_KVM
    if (table == NULL) {
        return NULL;
    }
#endif
    nextLongTable = &table->entries[mainTableLength];

    memset(table, 0, tableSize);

    table->h.maxCodeLen   = maxCodeLen;
    table->h.quickBits    = maxQuickBits;

    quickMask = (1 << maxQuickBits) - 1;

    for (p = codelen; p < endCodeLen; p++) {
        unsigned short huff;
        bits = *p;
        if (bits == 0) {
            continue;
        }

        /* Get the next code of the current length */
        code = codes[bits];
        codes[bits] += 1 << (MAX_BITS - bits);
        code = REVERSE_15BITS(code);
        huff = ((p - codelen) << 4) + bits;

        if (bits <= maxQuickBits) {
            unsigned stride = 1 << bits;
            for (j = code; j < mainTableLength; j += stride) {
                table->entries[j] = huff;
            }
        } else {
            unsigned short *thisLongTable;
            unsigned stride = 1 << (bits - maxQuickBits);
            unsigned int prefixCode = code & quickMask;
            unsigned int suffixCode = code >> maxQuickBits;

            if (table->entries[prefixCode] == 0) {
                /* This in the first long code with the indicated prefix.
                 * Create a pointer to the subtable */
                long delta = (char *)nextLongTable - (char *)table;
                table->entries[prefixCode] = 
                    (unsigned short)(HUFFINFO_LONG_MASK | delta);
                thisLongTable = nextLongTable;
               nextLongTable += longTableLength;
            } else {
                long delta = table->entries[prefixCode] & ~HUFFINFO_LONG_MASK;
                thisLongTable = (unsigned short *)((char *)table + delta);
            }

            for (j = suffixCode; j < longTableLength; j += stride) {
                thisLongTable[j] = huff;
            }
        }
    }

    ASSERT(nextLongTable == 
           &table->entries[mainTableLength + numLongTables * longTableLength]);

    return table;
}

#if INCLUDEDEBUGCODE

static void
ziperr(const char *message) {
    fprintf(stderr, "JAR error: %s\n", message);
}

#endif /* INCLUDEDEBUGCODE */

