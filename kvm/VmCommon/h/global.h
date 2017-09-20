/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Global definitions
 * FILE:      global.h
 * OVERVIEW:  Global system-wide definitions.
 * AUTHOR:    Antero Taivalsaari, Sun Labs, 1998
 *            Many others since then...
 *=======================================================================*/

/*=========================================================================
 * COMMENT:
 * This file contains declarations that do not belong to any
 * particular structure or subsystem of the VM. There are many
 * other additional global variable declarations in other files.
 *=======================================================================*/

/*=========================================================================
 * System include files
 *=======================================================================*/

/* The performs per-machine initialization */
#include <machine_md.h>

/*=========================================================================
 * Global compile-time constants and typedefs
 *=======================================================================*/

#undef  TRUE
#undef  FALSE

#define NIL   0

typedef enum {
    FALSE = 0,
    TRUE = 1
} bool_t;

/*=========================================================================
 * Global data type declarations
 *=======================================================================*/

/*=========================================================================
 * NOTE: These type declarations are not quite as portable as they
 * could be.  It might be useful to declare specific type names for
 * all Java-specific types (e.g., jint instead of normal int, jlong
 * instead of long64, and so forth).
 *=======================================================================*/

#define CELL  4        /* Size of a java word (= 4 bytes) */
#define log2CELL 2     /* Shift amount equivalent to dividing by CELL */
#define SHORTSIZE 2

typedef unsigned char     BYTE;
typedef unsigned long     cell;    /* Must be 32 bits long! */

/*=========================================================================
 * System-wide structure declarations
 *=======================================================================*/

typedef struct classStruct*         CLASS;
typedef struct instanceClassStruct* INSTANCE_CLASS;
typedef struct arrayClassStruct*    ARRAY_CLASS;

typedef struct objectStruct*        OBJECT;
typedef struct instanceStruct*      INSTANCE;
typedef struct arrayStruct*         ARRAY;
typedef struct stringInstanceStruct* STRING_INSTANCE;
typedef struct throwableInstanceStruct* THROWABLE_INSTANCE;
typedef struct internedStringInstanceStruct* INTERNED_STRING_INSTANCE;
typedef struct classInstanceStruct* CLASS_INSTANCE;

typedef struct byteArrayStruct*     BYTEARRAY;
typedef struct shortArrayStruct*    SHORTARRAY;
typedef struct pointerListStruct*   POINTERLIST;
typedef struct weakPointerListStruct* WEAKPOINTERLIST;
typedef struct weakReferenceStruct* WEAKREFERENCE;

typedef struct fieldStruct*         FIELD;
typedef struct fieldTableStruct*    FIELDTABLE;
typedef struct methodStruct*        METHOD;
typedef struct methodTableStruct*   METHODTABLE;
typedef struct stackMapStruct*      STACKMAP;
typedef struct icacheStruct*        ICACHE;
typedef struct chunkStruct*         CHUNK;

typedef struct staticChunkStruct*   STATICCHUNK;
typedef struct threadQueue*         THREAD;
typedef struct javaThreadStruct*    JAVATHREAD;
typedef struct monitorStruct*       MONITOR;

typedef struct stackStruct*         STACK;

typedef struct frameStruct*            FRAME;
typedef struct exceptionHandlerStruct* HANDLER;
typedef struct exceptionHandlerTableStruct* HANDLERTABLE;
typedef struct filePointerStruct*      FILEPOINTER;
typedef union constantPoolEntryStruct* CONSTANTPOOL_ENTRY;
typedef struct constantPoolStruct*     CONSTANTPOOL;
typedef char* BYTES;

typedef FILEPOINTER        *FILEPOINTER_HANDLE;
typedef OBJECT             *OBJECT_HANDLE;
typedef INSTANCE           *INSTANCE_HANDLE;
typedef ARRAY              *ARRAY_HANDLE;
typedef BYTEARRAY          *BYTEARRAY_HANDLE;
typedef POINTERLIST        *POINTERLIST_HANDLE;
typedef WEAKPOINTERLIST    *WEAKPOINTERLIST_HANDLE;
typedef JAVATHREAD         *JAVATHREAD_HANDLE;
typedef BYTES              *BYTES_HANDLE;
typedef METHOD             *METHOD_HANDLE;
typedef FRAME              *FRAME_HANDLE;
typedef const char*        *CONST_CHAR_HANDLE;
typedef unsigned char*     *UNSIGNED_CHAR_HANDLE;
typedef STRING_INSTANCE    *STRING_INSTANCE_HANDLE;
typedef THROWABLE_INSTANCE *THROWABLE_INSTANCE_HANDLE;
typedef THREAD             *THREAD_HANDLE;

#define BitSizeToByteSize(n)    (((n) + 7) >> 3)
#define ByteSizeToCellSize(n)   (((n) + (CELL - 1)) >> log2CELL)
#define StructSizeInCells(structName) ((sizeof(struct structName) + 3) >> 2)
#define UnionSizeInCells(structName) ((sizeof(union structName) + 3) >> 2)

/* Field and Method key types */
typedef unsigned short NameKey;
typedef unsigned short MethodTypeKey;
typedef unsigned short FieldTypeKey;

typedef union {
    struct {
        unsigned short nameKey;
        unsigned short typeKey; /* either MethodTypeKey or FieldTypeKey */
    } nt;
    unsigned long i;
} NameTypeKey;

/* Machines such as the Palm that use something other than FILE* for stdin
 * and stdout should redefine this in their machine_md.h
 */
#ifndef LOGFILEPTR
#define LOGFILEPTR FILE*
#endif

/*=========================================================================
 * Locally defined include files
 *=======================================================================*/

#include <messages.h>

#include <main.h>
#include <long.h>
#include <fp_math.h>
#include <hashtable.h>

#include <class.h>
#include <garbage.h>
#include <interpret.h>
#include <events.h>

#if ENABLE_JAVA_DEBUGGER
#include <debuggerStreams.h>
#include <debugger.h>
#endif /* ENABLE_JAVA_DEBUGGER */

#include <thread.h>
#include <pool.h>
#include <fields.h>
#include <frame.h>
#include <loader.h>
#include <native.h>
#include <cache.h>

#include <runtime.h>
#include <profiling.h>
#include <verifier.h>
#include <log.h>
#include <property.h>

/*=========================================================================
 * Miscellaneous global variables
 *=======================================================================*/

/* Shared string buffer that is used internally by the VM */
extern char str_buffer[];

/* Requested heap size when starting the VM from command line */
extern long RequestedHeapSize;

/* determine whether a debugger is connected to the VM */
extern bool_t vmDebugReady;

/*=========================================================================
 * Global execution modes
 *=======================================================================*/

/* Flags for toggling certain global modes on and off */
extern bool_t JamEnabled;
extern bool_t JamRepeat;

/*=========================================================================
 * Most frequently called functions are "inlined" here
 *=======================================================================*/

/*=========================================================================
 * Quick operations for stack manipulation (for manipulating stack
 * frames and operands).
 *=======================================================================*/

#define topStack                        (*getSP())
#define secondStack                     (*(getSP()-1))
#define thirdStack                      (*(getSP()-2))
#define fourthStack                     (*(getSP()-3))

#define topStackAsType(_type_)          (*(_type_ *)(getSP()))
#define secondStackAsType(_type_)       (*(_type_ *)(getSP() - 1))
#define thirdStackAsType(_type_)        (*(_type_ *)(getSP() - 2))
#define fourthStackAsType(_type_)       (*(_type_ *)(getSP() - 3))

#define oneMore                         getSP()++
#define oneLess                         getSP()--
#define moreStack(n)                    getSP() += (n)
#define lessStack(n)                    getSP() -= (n)

#define popStack()                      (*getSP()--)
#define popStackAsType(_type_)          (*(_type_ *)(getSP()--))

#define pushStack(data)                 *++getSP() = (data)
#define pushStackAsType(_type_, data)   *(_type_ *)(++getSP()) = (data)

/*=========================================================================
 * The equivalent macros to the above for use when the stack being
 * manipulated is not for the currently executing thread. In all cases
 * the additional parameter is the THREAD pointer.
 *=======================================================================*/

#define topStackForThread(t)         (*((t)->spStore))
#define secondStackForThread(t)      (*((t)->spStore - 1))
#define thirdStackForThread(t)       (*((t)->spStore - 2))
#define fourthStackForThread(t)      (*((t)->spStore - 3))

#define popStackForThread(t)         (*((t)->spStore--))
#define pushStackForThread(t, data)  (*(++(t)->spStore) = (data))
#define popStackAsTypeForThread(t, _type_) \
                                     (*(_type_*)((t)->spStore--))
#define pushStackAsTypeForThread(t, _type_, data)    \
                                     (*(_type_ *)(++(t)->spStore) = (data))
#define moreStackForThread(t, n)     ((t)->spStore += (n))
#define lessStackForThread(t, n)     ((t)->spStore -= (n))
#define oneMoreForThread(t)          ((t)->spStore++)
#define oneLessForThread(t)          ((t)->spStore--)

#define spForThread(t)               ((t)->spStore)

/*=========================================================================
 * Operations for handling memory fetches and stores
 *=======================================================================*/

/*=========================================================================
 * These macros define Java-specific memory read/write operations
 * for reading high-endian numbers.
 *=======================================================================*/

/* Get a 32-bit value from the given memory location */
#define getCell(addr) \
                  ((((long)(((unsigned char *)(addr))[0])) << 24) |  \
                   (((long)(((unsigned char *)(addr))[1])) << 16) |  \
                   (((long)(((unsigned char *)(addr))[2])) << 8)  |  \
                   (((long)(((unsigned char *)(addr))[3])) << 0))

#if BIG_ENDIAN
#define getAlignedCell(addr) (*(long *)(addr))
#else
#define getAlignedCell(addr) getCell(addr)
#endif

/* Get an unsigned 16-bit value from the given memory location */
#define getUShort(addr) \
                  ((((unsigned short)(((unsigned char *)(addr))[0])) << 8)  | \
                   (((unsigned short)(((unsigned char *)(addr))[1])) << 0))

/* Get a 16-bit value from the given memory location */
#define getShort(addr) ((short)(getUShort(addr)))

/* Store a 16-bit value in the given memory location */
#define putShort(addr, value) \
              ((unsigned char *)(addr))[0] = (unsigned char)((value) >> 8); \
              ((unsigned char *)(addr))[1] = (unsigned char)((value) & 0xFF)

/*=========================================================================
 * Data types and macros for data alignment
 *=======================================================================*/

typedef union Java8Str {
    long x[2];
#if IMPLEMENTS_FLOAT
    double d;
#endif
    long64 l;
    ulong64 ul;
    void *p;
} Java8;

#if NEED_LONG_ALIGNMENT

#define GET_LONG(_addr) ( ((tdub).x[0] = ((long*)(_addr))[0]), \
                          ((tdub).x[1] = ((long*)(_addr))[1]), \
                          (tdub).l )

#define GET_ULONG(_addr) ( ((tdub).x[0] = ((long*)(_addr))[0]), \
                           ((tdub).x[1] = ((long*)(_addr))[1]), \
                           (tdub).ul )

#define SET_LONG( _addr, _v) ( (tdub).l = (_v),                   \
                               ((long*)(_addr))[0] = (tdub).x[0], \
                               ((long*)(_addr))[1] = (tdub).x[1] )

#define SET_ULONG( _addr, _v) ( (tdub).ul = (_v),                  \
                                ((long*)(_addr))[0] = (tdub).x[0], \
                                ((long*)(_addr))[1] = (tdub).x[1] )

#define COPY_LONG(_new, _old) ( ((long*)(_new))[0] = ((long*)(_old))[0], \
                                ((long*)(_new))[1] = ((long*)(_old))[1] )

#else

#define GET_LONG(_addr) (*(long64*)(_addr))
#define GET_ULONG(_addr) (*(ulong64*)(_addr))
#define SET_LONG(_addr, _v) (*(long64*)(_addr) = (_v))
#define SET_ULONG(_addr, _v) (*(ulong64*)(_addr) = (_v))
#define COPY_LONG(_new, _old) (*(long64*)(_new) = *(long64*)(_old))

#endif /* NEED_LONG_ALIGNMENT */

/* If double's must be aligned on doubleword boundaries then define this */
#if NEED_DOUBLE_ALIGNMENT

#define GET_DOUBLE(_addr) ( ((tdub).x[0] = ((long*)(_addr))[0]), \
                            ((tdub).x[1] = ((long*)(_addr))[1]), \
                            (tdub).d )
#define SET_DOUBLE( _addr, _v) ( (tdub).d = (_v),                   \
                                 ((long*)(_addr))[0] = (tdub).x[0], \
                                 ((long*)(_addr))[1] = (tdub).x[1] )
#define COPY_DOUBLE(_new, _old) ( ((long*)(_new))[0] = ((long*)(_old))[0], \
                                  ((long*)(_new))[1] = ((long*)(_old))[1] )

#else

#define GET_DOUBLE(_addr) (*(double*)(_addr))
#define SET_DOUBLE(_addr, _v) (*(double*)(_addr) = (_v))
#define COPY_DOUBLE(_new, _old) (*(double*)(_new) = *(double*)(_old))

#endif /* NEED_DOUBLE_ALIGNMENT */

/* Macros for handling endianness of the hardware architecture properly */
#if LITTLE_ENDIAN
#   define SET_LONG_FROM_HALVES(_addr, _hi, _lo) \
        (((long *)_addr)[0] = (_lo), ((long *)_addr)[1] = (_hi))
#elif BIG_ENDIAN
#   define SET_LONG_FROM_HALVES(_addr, _hi, _lo) \
        (((long *)_addr)[0] = (_hi), ((long *)_addr)[1] = (_lo))
#elif COMPILER_SUPPORTS_LONG
#  if NEED_LONG_ALIGNMENT
#     define SET_LONG_FROM_HALVES(_addr, _hi, _lo) \
             do { long64 _tmp = ((((long64)_hi)<<32) | ((unsigned long)_lo)); \
                      COPY_LONG(_addr, &_tmp); } while(0)
#  else
#     define SET_LONG_FROM_HALVES(_addr, _hi, _lo) \
          (*(long64 *)(_addr) = ((((long64)_hi)<<32) | ((unsigned long)_lo)))
#  endif
#else
#   error "You must define LITTLE_ENDIAN or BIG_ENDIAN"
#endif

/* Macros for handling endianness in floating point and trigonometric calculations */
#if LITTLE_ENDIAN
#define __HI(x) *(1+(int*)&x)
#define __LO(x) *(int*)&x
#define __HIp(x) *(1+(int*)x)
#define __LOp(x) *(int*)x
#else
#define __HI(x) *(int*)&x
#define __LO(x) *(1+(int*)&x)
#define __HIp(x) *(int*)x
#define __LOp(x) *(1+(int*)x)
#endif

/* Macros for popping and pushing 64-bit numbers from/to operand stack */
#define popDouble(_lval) (oneLess, COPY_DOUBLE(&_lval, getSP()), oneLess)
#define popLong(_lval) (oneLess, COPY_LONG(&_lval, getSP()), oneLess)

#define pushDouble(_lval) (oneMore, COPY_DOUBLE(getSP(), &_lval), oneMore)
#define pushLong(_lval) (oneMore, COPY_LONG(getSP(), &_lval), oneMore)

/*=========================================================================
 * The equivalent macros to the above for use when the stack being
 * manipulated is not for the currently executing thread. In all cases
 * the additional parameter is the THREAD pointer.
 *=======================================================================*/

#define popDoubleForThread( t, _lval) (oneLessForThread(t), COPY_DOUBLE(&_lval, spForThread(t)), oneLessForThread(t))
#define popLongForThread(   t, _lval) (oneLessForThread(t), COPY_LONG(  &_lval, spForThread(t)), oneLessForThread(t))
#define pushDoubleForThread(t, _lval) (oneMoreForThread(t), COPY_DOUBLE(spForThread(t), &_lval), oneMoreForThread(t))
#define pushLongForThread(  t, _lval) (oneMoreForThread(t), COPY_LONG(  spForThread(t), &_lval), oneMoreForThread(t))

/*=========================================================================
 * Error handling definitions
 *=======================================================================*/

/*=========================================================================
 * Redesigned C exception handling mechanism for KVM 1.1
 *
 * 15/11/01: Doug Simon
 *     This mechanism has been extended so that Throwable objects
 *     can now be thrown. This is to support (near) complete
 *     JLS and JVMS error handling semantics.
 *
 *     There is still only one catch statement and as such, polymorphic
 *     exception handling has to be implemented manually.
 *=======================================================================*/

/*
 * This struct models the nesting of try/catch scopes.
 */
struct throwableScopeStruct {
    /*
     * State relevant to the platform dependent mechanism used to
     * implement the flow of control (e.g. setjmp/longjmp).
     */
    jmp_buf*   env;
    /*
     * A THROWABLE_INSTANCE object.
     */
    THROWABLE_INSTANCE throwable;
    /*
     * The number of temporary roots at the point of TRY.
     */
    int     tmpRootsCount;
    /*
     * The enclosing try/catch (if any).
     */
    struct throwableScopeStruct* outer;
};

typedef struct throwableScopeStruct* THROWABLE_SCOPE;

/*
 * IMPORTANT: Jumping into or out of a TRY clause (for example via
 * return, break, continue, goto, longjmp) is forbidden--the compiler
 * will not complain, but bad things will happen at run-time.  Jumping
 * into or out of a CATCH clause is okay, and so is jumping around
 * inside a TRY clause.  In many cases where one is tempted to return
 * from a TRY clause, it will suffice to use THROW, and then return
 * from the CATCH clause.  Another option is to set a flag variable and
 * use goto to jump to the end of the TRY clause, then check the flag
 * after the TRY/CATCH statement.
 */
#if INCLUDEDEBUGCODE
#define TRACE_EXCEPTION(name) \
if (traceexceptions) \
{ \
    int level = 0; \
    struct throwableScopeStruct* s = ThrowableScope; \
    while (s->outer != NULL) { \
        s = s->outer; \
        level++; \
    } \
    s = ThrowableScope; \
    fprintf(stdout, \
        "%s(%d):\tenv: 0x%lx, throwable: 0x%lx, tmpRootsCount: %d\n", \
        name, \
        level,(long)s->env,(long)s->throwable,s->tmpRootsCount); \
}

/*
 * There must not be any allocation guard active when entering a TRY block
 * as an exception throw will always cause at least one allocation (for the
 * exception object) thus potentially invalidating any pointers being
 * guarded by the allocation guard. Note however that this does not preclude
 * using an allocation guard inside a TRY block.
 */
#define ASSERT_NO_ALLOCATION_GUARD \
    if (NoAllocation > 0) { \
        fatalError(KVM_MSG_TRY_BLOCK_ENTERED_WHEN_ALLOCATION_FORBIDDEN); \
    }
#else
#define TRACE_EXCEPTION(name)
#define ASSERT_NO_ALLOCATION_GUARD
#endif

#define TRY                                                    \
    {                                                          \
        struct throwableScopeStruct __scope__;                 \
        int __state__;                                         \
        jmp_buf __env__;                                       \
        __scope__.outer = ThrowableScope;                      \
        ThrowableScope = &__scope__;                           \
        ThrowableScope->env = &__env__;                        \
        ThrowableScope->tmpRootsCount = TemporaryRootsLength;  \
        ASSERT_NO_ALLOCATION_GUARD                             \
        TRACE_EXCEPTION("TRY")                                 \
        if ((__state__ = setjmp(__env__)) == 0) {

/*
 * Any non-null THROWABLE_INSTANCE passed into a CATCH clause
 * is protected as a temporary root.
 */
#define CATCH(__throwable__)                                   \
        }                                                      \
        TRACE_EXCEPTION("CATCH")                               \
        ThrowableScope = __scope__.outer;                      \
        TemporaryRootsLength = __scope__.tmpRootsCount;        \
        if (__state__ != 0) {                                  \
            START_TEMPORARY_ROOTS                              \
                 DECLARE_TEMPORARY_ROOT(THROWABLE_INSTANCE,    \
                     __throwable__,__scope__.throwable);

#define END_CATCH                                              \
            END_TEMPORARY_ROOTS                                \
        }                                                      \
    }

/*
 * This macro is required for jumping out of a CATCH block with a goto.
 * This is used in FastInterpret so that the interpreter loop is re-entered
 * with a fresh TRY statement.
 */
#define END_CATCH_AND_GOTO(label)                              \
            END_TEMPORARY_ROOTS                                \
            goto label;                                        \
        }                                                      \
    }

#define THROW(__throwable__)                                   \
    {                                                          \
        THROWABLE_INSTANCE __t__ = __throwable__;              \
        TRACE_EXCEPTION("THROW")                               \
        if (__t__ == NULL)                                     \
            fatalVMError("THROW called with NULL");            \
        ThrowableScope->throwable = __t__;                     \
        longjmp(*((jmp_buf*)ThrowableScope->env),1);           \
    }

/*
 * This is a non-nesting use of setjmp/longjmp used solely for the purpose
 * of exiting the VM in a clean way. By separating it out from the
 * exception mechanism above, we don't need to worry about whether or not
 * we are throwing an exception or exiting the VM in an CATCH block.
 */

#define VM_START                                               \
    {                                                          \
        jmp_buf __env__;                                       \
        VMScope = &(__env__);                                  \
        if (setjmp(__env__) == 0) {

#define VM_EXIT(__code__)                                      \
        VMExitCode = __code__;                                 \
        longjmp(*((jmp_buf*)VMScope),1)

#define VM_FINISH(__code__)                                    \
        } else {                                               \
            int __code__ = VMExitCode;

#define VM_END_FINISH                                          \
        }                                                      \
    }

extern void* VMScope;
extern int   VMExitCode;
extern THROWABLE_SCOPE ThrowableScope;

#ifndef FATAL_ERROR_EXIT_CODE
#define FATAL_ERROR_EXIT_CODE 127
#endif

#ifndef UNCAUGHT_EXCEPTION_EXIT_CODE
#define UNCAUGHT_EXCEPTION_EXIT_CODE 128
#endif

/*=========================================================================
 * Macros for controlling global execution tracing modes
 *=======================================================================*/

/* The following isn't really intended to be unreadable, but it simplifies
 * the maintenance of the various execution tracing flags.
 *
 * NOTE: Logically these operations belong to VmExtra directory,
 * since this code is not useful for those ports that do not support
 * command line operation.  However, since this code is intimately
 * tied with the tracing capabilities of the core KVM, we'll keep
 * these definitions in this file for the time being.
 *
 * The intent is that you can call
 *    FOR_EACH_TRACE_FLAG(Macro_Of_Two_Arguments)
 * where Macro_Of_Two_Arguments is a two-argument macro whose first argument
 * is the name of a variable, and the second argument is the string the user
 * enters to turn on that flag.
 */
#define FOR_EACH_TRACE_FLAG(Macro) \
     FOR_EACH_ORDINARY_TRACE_FLAG(Macro) \
     FOR_EACH_DEBUGGER_TRACE_FLAG(Macro) \
     FOR_EACH_JAM_TRACE_FLAG(Macro)

/* The ordinary trace flags are those included in any debugging build */
#if INCLUDEDEBUGCODE
#   define FOR_EACH_ORDINARY_TRACE_FLAG(Macro)  \
         Macro(tracememoryallocation,         "-traceallocation") \
         Macro(tracegarbagecollection,        "-tracegc") \
         Macro(tracegarbagecollectionverbose, "-tracegcverbose") \
         Macro(traceclassloading,             "-traceclassloading") \
         Macro(traceclassloadingverbose,      "-traceclassloadingverbose") \
         Macro(traceverifier,                 "-traceverifier") \
         Macro(tracestackmaps,                "-tracestackmaps") \
         Macro(tracebytecodes,                "-tracebytecodes") \
         Macro(tracemethodcalls,              "-tracemethods") \
         Macro(tracemethodcallsverbose,       "-tracemethodsverbose") \
         Macro(tracestackchunks,              "-tracestackchunks") \
         Macro(traceframes,                   "-traceframes") \
         Macro(traceexceptions,               "-traceexceptions") \
         Macro(traceevents,                   "-traceevents") \
         Macro(tracemonitors,                 "-tracemonitors") \
         Macro(tracethreading,                "-tracethreading") \
         Macro(tracenetworking,               "-tracenetworking")
#else
#    define FOR_EACH_ORDINARY_TRACE_FLAG(Macro)
#endif

/* The debugger tracing flags are those included only if we support KWDP */
#if INCLUDEDEBUGCODE && ENABLE_JAVA_DEBUGGER
#  define FOR_EACH_DEBUGGER_TRACE_FLAG(Macro)  \
               Macro(tracedebugger, "-tracedebugger")
#else
#  define FOR_EACH_DEBUGGER_TRACE_FLAG(Macro)
#endif

/* The debugger tracing flags are those included only if we support KWDP */
#if INCLUDEDEBUGCODE && USE_JAM
#  define FOR_EACH_JAM_TRACE_FLAG(Macro)  \
               Macro(tracejam, "-tracejam")
#else
#  define FOR_EACH_JAM_TRACE_FLAG(Macro)
#endif

/* Declare each of the trace flags to be external.  Note that we define
 * a two-variable macro, then use that macro as an argument to
 * FOR_EACH_TRACE_FLAG
 */

#define DECLARE_TRACE_VAR_EXTERNAL(varName, userName) \
    extern int varName;
FOR_EACH_TRACE_FLAG(DECLARE_TRACE_VAR_EXTERNAL)

