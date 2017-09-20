/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Internal runtime structures
 * FILE:      cache.h
 * OVERVIEW:  Inline caching support.
 * AUTHOR:    Antero Taivalsaari, Sun Labs
 *=======================================================================*/

/*=========================================================================
 * COMMENTS:
 * In order to reach reasonable execution performance without fancy
 * compilation techniques, we are using a simple Deutsch-Schiffman
 * Smalltalk-style inline caching for optimizing message sending
 * and other bytecodes.
 *
 * If the optimization mode is enabled (ENABLEFASTBYTECODES), many
 * bytecodes are patched at runtime with inline cache entries so
 * that we can avoid costly constant pool and method table
 * lookup in most situations. Since Java message sending
 * bytecodes do not have enough space for truly inlined
 * parameters, a special inline cache area is allocated
 * separately and individual inline cache entries are
 * referenced by indices from the code.
 *
 * Code before optimization:
 *             ...  +----+--------+ ...
 *                  | BC | PARAM  |     BC = "slow" bytecode (8 bits)
 *             ...  +----+--------+ ... PARAM = parameter for BC (16 bits)
 *
 * Code after optimization:
 *             ...  +----+--------+ ...
 *                  | BF | ICIDX  |     BF    = "fast" bytecode (8 bits)
 *             ...  +----+--------+ ... ICIDX = inline cache index (16 bits)
 *
 * The optimization process is fully reversible and repeatable,
 * i.e., we can de-optimize code at any point if necessary.
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

/*=========================================================================
 * Global variables and definitions
 *=======================================================================*/

/* The master inline cache in the system */
/* In principle, each thread could have its own inline cache area, */
/* but this would not improve performance substantially. */
extern ICACHE InlineCache;

/* Index of the next inline cache entry to be used */
extern int InlineCachePointer;

/*=========================================================================
 * Inline cache structures
 *=======================================================================*/

/*=========================================================================
 * COMMENTS:
 * ICACHE is a structure for storing individual inline cache entries.
 * The whole inline cache is simply a contiguous array of ICACHE entries.
 *
 * Each inline cache entry contains a forward pointer to the actual method
 * to be executed, a backpointer to the location of code referring to
 * to inline cache entry, and the original contents of code (before
 * inline patching) so that inline cached can be removed if necessary.
 *
 * There is a fixed number of inline cache entries in the system.
 * Once the inline cache area is full, we start reusing the oldest
 * entries starting from the beginning of the icache area.
 * The code of the methods referring to the reused icache entries
 * is replaced with the original (pre-inline cache) code. In other
 * words, the whole inline caching process is completely reversable
 * and repeatable.
 *
 * Note: in order to avoid garbage collection problems, we
 * do not store any dynamic heap pointers in inline caches!
 * This ensures that we can simply ignore the whole inline cache
 * area during garbage collection.
 *=======================================================================*/

/* ICACHE (allocated in inline cache area) */
struct icacheStruct {
    cell* contents;  /* Cache contents (used differently in different entries) */
    BYTE* codeLoc;   /* Backpointer to the code location using this icache */
    short origParam; /* Original bytecode parameter in (codeLoc+1) */
    BYTE  origInst;  /* Original bytecode instruction in codeLoc */
};

#define SIZEOF_ICACHE            StructSizeInCells(icacheStruct)

#if ENABLEFASTBYTECODES

/*=========================================================================
 * Constructor/destructors for (re)initializing inline caching
 *=======================================================================*/

void InitializeInlineCaching();
void FinalizeInlineCaching();

/*=========================================================================
 * Constructors/destructors for individual icache entries
 *=======================================================================*/

int createInlineCacheEntry(cell* contents, BYTE* originalCode);

/*=========================================================================
 * Operations on individual icache entries
 *=======================================================================*/

ICACHE getInlineCache(int index);

/*=========================================================================
 * Macro for high performance!
 *=======================================================================*/

#if ENABLE_JAVA_DEBUGGER

#define REPLACE_BYTECODE(ip, bytecode)          \
        if (*ip == BREAKPOINT) {                \
            VMSAVE                              \
            replaceEventOpcode(bytecode);       \
            VMRESTORE                           \
        } else {                                \
            *ip = bytecode;                     \
        }

#else /* !ENABLE_JAVA_DEBUGGER */

#define REPLACE_BYTECODE(ip, bytecode) *ip = bytecode;

#endif /* ENABLE_JAVA_DEBUGGER */

#define CREATE_CACHE_ENTRY(cellp, ip)           \
        iCacheIndex = createInlineCacheEntry((cell*)cellp, ip);

#define GETINLINECACHE(index) (&InlineCache[index])

#else /* !ENABLEFASTBYTECODES */

#define InitializeInlineCaching()
#define FinalizeInlineCaching()

#define createInlineCacheEntry(contents, originalCode) 0
#define getInlineCache(index) NULL

#endif /* ENABLEFASTBYTECODES */

