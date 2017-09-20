/* 
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 */

/*=========================================================================
 * KVM
 *=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Java-level debugger
 * FILE:      debugger.c
 * OVERVIEW:  This file defines the Java-level debugger interface for
 *            KVM based on JPDA (Java Platform Debug Architecture).
 * AUTHOR:    Jonathan Coles, Sun Labs (summer 2000)
 *            Bill Pittore, Sun Labs
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>

#if ENABLE_JAVA_DEBUGGER /* Extends until the end of the file */

#include <thread.h>

#ifdef PILOT
# include <sys_types.h>
# include <netinet_in.h>
#endif

#ifdef WINDOWS
#define WIN32_LEAN_AND_MEAN
#define NOMSG

#ifdef POCKETPC
# include <winsock.h>
#else
# ifdef GCC
#  include <winsock.h>
# else
#  include <winsock2.h>
# endif /* GCC */
#endif /* POCKETPC */
#endif /* WINDOWS */

/*========================================================================
 * Definitions and declarations
 *========================================================================*/

#if INCLUDEDEBUGCODE

#ifndef PILOT
#include <stdarg.h>
#endif

static void printDebuggerInfo(const char *major, const char *minor, 
                              const char *format,...);
static void printEventInfo(const char *format,...);
static void printValueFromTag(void *address, unsigned char tag);

#define TRACE_DEBUGGER(argList) \
        if (tracedebugger) { printDebuggerInfo argList ; }  else 
#define DEBUGGER_EVENT(argList) \
        if (tracedebugger) { printEventInfo argList ; }  else 
#else

#define TRACE_DEBUGGER(argList)
#define DEBUGGER_EVENT(argList)

#endif /* INCLUDEDEBUGCODE */

static void clearEvent(VMEventPtr ep);

/*========================================================================
 * Variables
 *========================================================================*/

static bool_t resumeOK = FALSE;

HASHTABLE debuggerHashTable;

ID_HASH_ENTRY bucketFreeList = NULL;

WEAKPOINTERLIST debugRoot = NULL;
unsigned long *bucketMap = NULL;
unsigned long bucketsAllocated;

unsigned long uniqueDebuggerID = 1;

VMEventPtr eventHead = NIL;
EVENTMODIFIER modHead = NIL;

struct CE_Modifiers *CE_ModHead = NIL;

struct Modifiers *ModHead = NIL;

static long requestID = 1;

bool_t debuggerActive;

bool_t waitOnSuspend;
bool_t allThreadsSuspended = FALSE;
bool_t suspend;       /* Suspend all threads on VM startup. 
                       * Set to TRUE by default in main.c.
                       */

int _method_index_base = 0;

short debuggerPort = DEFAULT_DEBUGGER_PORT;

long debuggerNotifyList=Dbg_EventKind_NONE;

#define GET_VMTHREAD(threadID) (getObjectPtr(threadID) == NULL ? NULL : ((JAVATHREAD)getObjectPtr(threadID))->VMthread)

/*========================================================================
 * Debugging operations
 *========================================================================*/

#if INCLUDEDEBUGCODE

/*========================================================================
 * OP Code Lengths used by isLegalOffset() below. 
 *========================================================================*/

static const unsigned char opcodeLengths[] = {
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0 */
     1, 1, 1, 1, 1, 1, 2, 3, 2, 3, /* 10 */
     3, 2, 2, 2, 2, 2, 1, 1, 1, 1, /* 20 */
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 30 */
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 40 */
     1, 1, 1, 1, 2, 2, 2, 2, 2, 1, /* 50 */
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 60 */
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 70 */
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 80 */
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 90 */
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 100 */
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 110 */
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 120 */
     1, 1, 3, 1, 1, 1, 1, 1, 1, 1, /* 130 */
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 140 */
     1, 1, 1, 3, 3, 3, 3, 3, 3, 3, /* 150 */
     3, 3, 3, 3, 3, 3, 3, 3, 3, 2, /* 160 */
     0, 0, 1, 1, 1, 1, 1, 1, 3, 3, /* 170 */
     3, 3, 3, 3, 3, 5, 0, 3, 2, 3, /* 180 */
     1, 1, 3, 3, 1, 1, 0, 4, 3, 3, /* 190 */
     5, 5, 1,                      /* 200 - 202 */
              3, 3, 3, 3, 3, 3, 3, /* 202 - 209.  Artificial */
     3, 3, 3, 0, 3, 3, 3, 5, 3, 3, /* 210.        Artificial */
     4, 3, 3, 1                    /* 220 - 223.  Aritificial */
 };

/*========================================================================
 * Function:        isLegalOffset()
 * Overview:        Given a method and an offset, it determines if the 
 *                  offset is a legal offset into the bytecode of the method. 
 * Interface:
 *   parameters:    pointer to thisMethod 
 *                  long offset 
 *   returns:       TRUE if it is a legal offset, FALSE otherwise
 *
 *=======================================================================*/

static bool_t isLegalOffset(METHOD thisMethod, long offset)
 {
     unsigned char *code = thisMethod->u.java.code;
     unsigned int codeLength = thisMethod->u.java.codeLength;
     unsigned char *expectedCode = &code[offset];
     unsigned char *currentCode = code;

     if (offset >= (long)codeLength) {
         return FALSE;
     }

     while (currentCode < expectedCode) {
         unsigned char token = currentCode[0];

         switch(token) {

       case TABLESWITCH: {
             long low, high;
             currentCode = (unsigned char *)((long)(currentCode + 4) & ~3);
             low = getAlignedCell(currentCode + 4);
             high = getAlignedCell(currentCode + 8);
             currentCode += 12 + (high - low + 1) * 4;
             break;
         }

       case LOOKUPSWITCH: {
             long pairs;
             currentCode = (unsigned char *)((long)(currentCode + 4) & ~3);
             pairs = getAlignedCell(currentCode + 4);
             currentCode += 8 + pairs * 8;
             break;
         }

       case WIDE:
             currentCode += ((currentCode[1] == IINC) ? 6 : 4);
             break;

         default:
             currentCode += opcodeLengths[token];
             break;
       }
     }
     return (currentCode == expectedCode);
    }

#endif /* INCLUDEDEBUGCODE */

static void 
nop(PACKET_INPUT_STREAM_HANDLE inH, PACKET_OUTPUT_STREAM_HANDLE outH)
{
    struct CmdPacket *cmd = &unhand(inH)->packet->type.cmd;
    TRACE_DEBUGGER(("Unknown Command", "", "%d/%d", cmd->cmdSet, cmd->cmd));
    outStream_setError(outH, JDWP_Error_NOT_IMPLEMENTED);
}

static void * allocAlignedBytes(long size) {
    void *p;

#if COMPILER_SUPPORTS_LONG
    /*
     * mallocBytes allocates a cell sized block which means that
     * the address returned is always 4 byte aligned.  We need
     * 8 byte aligned so we add 4 more bytes to size then
     * re-adjust the address of newMod
     */
        p = callocPermanentObject(ByteSizeToCellSize((size + 4)));
    /* roundup to next 8 byte boundary */
    p = (void *)(((unsigned long)p + 7) & ~7);
#else
        p = callocPermanentObject(ByteSizeToCellSize(size));
#endif
    return (p);

}

static void setResumeOK(bool_t val) {
    resumeOK = val;
}

/*========================================================================
 * Function:        isValidThread()
 * Overview:        determines whether a thread actually exists
 *                  
 * Interface:
 *  parameters:     thread
 *  returns:        bool_t
 *
 * Notes:
 *=======================================================================*/

static bool_t 
isValidThread(THREAD thread)
{
    THREAD tptr;

    for (tptr = AllThreads; tptr != NULL; tptr = tptr->nextAliveThread) { 
        if (tptr == thread) { 
            return TRUE;
        }
    }
    return FALSE;
}

/*========================================================================
 * Function:        isValidJavaThread()
 * Overview:        determines whether this Java thread actually exists
 *                  
 * Interface:
 *  parameters:     object to test
 *  returns:        bool_t
 *
 * Notes:
 *=======================================================================*/

static bool_t 
isValidJavaThread(OBJECT obj)
{
    THREAD tptr;

    for (tptr = AllThreads; tptr != NULL; tptr = tptr->nextAliveThread) { 
        if (tptr->javaThread == (JAVATHREAD)obj) { 
            return TRUE;
        }
    }
    return FALSE;
}

static bool_t isValidObject(OBJECT object)
{
    if (object == NULL || object->ofClass == NULL) { 
        return FALSE;
    } else if (!inCurrentHeap(object)) { 
        CLASS clazz = object->ofClass;
        /* These are the only user visible types that don't live in the
         * heap.  Anything else is almost certainly a bug. 
         */
        return   clazz == (CLASS)JavaLangClass || clazz == (CLASS)JavaLangString
              || clazz == (CLASS)PrimitiveArrayClasses[T_CHAR];
    } else { 
        /* Occasionally, Java instances point to heap values that shouldn't
         * be externally visible.  (E.g. Thread's point to an "implementation".)
         * Let's make sure the type of the object really is something the
         * user ought to know about.
         */
        GCT_ObjectType type = getObjectType((cell*)object);
        return (type == GCT_INSTANCE || type == GCT_ARRAY 
                 || type == GCT_OBJECTARRAY);
    }
}

static unsigned long 
debuggerHash(OBJECT p)
{
    return (unsigned long)objectHashCode(p);
}

static void freeBucket(ID_HASH_ENTRY bucket) {
    ID_HASH_ENTRY tmpBucket, *freeBucketPtr, *lastBucketPtr;
    unsigned long index;

    index = (bucket->key >> DEBUGGER_INDEX_SHIFT) & DEBUGGER_INDEX_MASK;
    freeBucketPtr = (ID_HASH_ENTRY *)&debuggerHashTable->bucket[index];
    lastBucketPtr = freeBucketPtr;
    for (tmpBucket = *freeBucketPtr;
        tmpBucket != NULL; tmpBucket = tmpBucket->next) {
        if (bucket == tmpBucket) {
            *lastBucketPtr = tmpBucket->next;
            break;
         }
         lastBucketPtr = &tmpBucket->next;
    }
    bucket->next = bucketFreeList;
    bucketFreeList = bucket;
    bucket->key = 0;
}

/*========================================================================
 * Function:        addToDebugRoot()
 * Overview:        add this object to the debug root
 * Interface:
 *   parameters:    pointer to object
 *   returns:       index into array
 *
 * Notes:
 *                 The debugRoot contains object references and these
 *                 references will be updated by the GC since debugRoot is a
 *                 weak Pointerlist type.  We also keep a parallel array
 *                 called the bucketMap which contains a pointer to the
 *                 hash bucket that will contain this entry.  We do this
 *                 so we can easily re-use buckets since they are allocated
 *                 from permanent memory.  If we re-use a debugRoot entry
 *                 (because it was GC'd and NULL'd out) we set the bucket->key
 *                 value to 0 so that the getObjectID code will notice that
 *                 this bucket is free and re-use it.
 *                 Note that classes/objects that are in permanent memory
 *                 have an ID that is their actual address.  We distinguish
 *                 between these and transient objects by bit 0 being set in
 *                 the ID.
 *=======================================================================*/

static unsigned int addToDebugRoot(void *object, ID_HASH_ENTRY bucket)
{
    unsigned long i;
    unsigned long length = debugRoot->length;
    ID_HASH_ENTRY tmpBucket;
    unsigned long *tmpPtr;

#if INCLUDEDEBUGCODE    
    /** 
     * Note:  debugRoot->data[length - 1] is always empty.
     */
    if (debugRoot->data[length - 1].cellp != NULL) { 
        fatalError(KVM_MSG_DEBUG_ROOT_CANNOT_BE_FULL);
    }
#endif
    for (i = 0; ; i++) { 
        if (debugRoot->data[i].cellp == NULL) {
            debugRoot->data[i].cellp = (cell *)object;
            if (bucketMap[i] != 0) {
                /* This debugRoot entry was freed by the GC, so lets free 
                 * up the bucket that corresponds to it.
                 */
                tmpBucket = (ID_HASH_ENTRY)bucketMap[i];
#if INCLUDEDEBUGCODE
                if (i != tmpBucket->rootIndex)
                    fatalError(KVM_MSG_DEBUG_HASHTABLE_INCONSISTENT);
#endif
                freeBucket(tmpBucket);
            }
            bucketMap[i] = (unsigned long)bucket;
            if (i < length - 1) { 
                return i;
            } else if (i >= DEBUGGER_INDEX_MASK) { 
                fatalError(KVM_MSG_OUT_OF_DEBUGGER_ROOT_SLOTS);
            } else { 
                break;
            }
        }
    }

    /* We need to expand debugRoot and bucketMap */
    {
        /* We need to expand the table */
        const int expansion = 20;
        int newLength = length + expansion;
        WEAKPOINTERLIST tmpList = 
            (WEAKPOINTERLIST)callocObject(SIZEOF_WEAKPOINTERLIST(newLength),
                                          GCT_WEAKPOINTERLIST);
        tmpList->length = newLength;
        memcpy(tmpList->data, debugRoot->data, length << log2CELL);
        debugRoot = tmpList;

        tmpPtr = (unsigned long *)mallocBytes(newLength * CELL);
        memcpy(tmpPtr, bucketMap, length << log2CELL);
        memset(&tmpPtr[length], 0, expansion << log2CELL);
        bucketMap = tmpPtr;
        return i;
    }
}

static unsigned long 
getObjectIDInternal(OBJECT object)
{
    HASHTABLE table;
    unsigned long index;
    ID_HASH_ENTRY bucket, *bucketPtr;

    table = debuggerHashTable;
    index = debuggerHash(object) % table->bucketCount;
    bucketPtr = (ID_HASH_ENTRY *)&table->bucket[index];
    for (bucket = *bucketPtr; bucket != NULL; bucket = bucket->next) {
        if (debugRoot->data[bucket->rootIndex].cellp == (cell*)object)
            return (bucket->key);
    }
    return (0);
}

/*========================================================================
 * Function:        getObjectID()
 * Overview:        given an object, find/create it's ID
 * Interface:
 *   parameters:    pointer to object
 *   returns:       index into array
 *
 * Notes:
 *=======================================================================*/

unsigned long getObjectID(OBJECT object)
{
    HASHTABLE table;
    unsigned long key;
    unsigned long index;
    ID_HASH_ENTRY bucket, *bucketPtr;

    if (!vmDebugReady || object == NULL) { 
        return 0;
    }
    if (!inCurrentHeap(object)) { 
        return (unsigned long)object;
    }
    key = getObjectIDInternal(object);
    if (key == 0) {
        table = debuggerHashTable;
        index = debuggerHash(object) % table->bucketCount;
        bucketPtr = (ID_HASH_ENTRY *)&table->bucket[index];
        bucket = NULL;
        /* need to create a new one, first try to find an unused bucket */
        if (bucketFreeList != NULL) {
            bucket = bucketFreeList;
            bucketFreeList = bucket->next;
        }
        if (bucket == NULL) {
            START_TEMPORARY_ROOTS
                DECLARE_TEMPORARY_ROOT(void *, objectX, object);
                bucket = 
                    (ID_HASH_ENTRY)callocPermanentObject(SIZEOF_ID_HASH_ENTRY);
                bucketsAllocated++;
                object = objectX; /* in case GC occurred */
            END_TEMPORARY_ROOTS
        }
        bucket->rootIndex = addToDebugRoot(object, bucket);
        bucket->next = *bucketPtr;
        *bucketPtr = bucket;
        bucket->key = NEXT_UNIQUE_DEBUGGER_ID() 
            + ((index & DEBUGGER_INDEX_MASK) << DEBUGGER_INDEX_SHIFT) + 1;
        key = bucket->key;
    }
    return key;
}

/*========================================================================
 * Function:        getObjectPtr()
 * Overview:        given an ID, find the object it maps to
 * Interface:
 *   parameters:    ID number
 *   returns:       void pointer
 *
 * Notes:
 *=======================================================================*/

OBJECT 
getObjectPtr(unsigned long key)
{
    unsigned long index;
    ID_HASH_ENTRY bucket, *bucketPtr;

    if (!vmDebugReady) { 
        return NULL;
    } else if ((key & 1) == 0) { 
        return ((void *)key);
    } 
    index = (key >> DEBUGGER_INDEX_SHIFT) & DEBUGGER_INDEX_MASK;
    bucketPtr = (ID_HASH_ENTRY *)&debuggerHashTable->bucket[index];
    for (bucket = *bucketPtr; bucket != NULL; bucket = bucket->next) {
        if (key == bucket->key)
            return (OBJECT)(debugRoot->data[bucket->rootIndex].cellp);
    }
    return NULL;
}

/*========================================================================
 * Function:        getModifier()
 * Overview:        return a specific type of modifier contained within
 *                  an event
 * Interface:
 *  parameters:     event pointer, modifier type
 *  returns:        pointer to an event modifier struct
 *
 * Notes:
 *=======================================================================*/

static EVENTMODIFIER getModifier(VMEventPtr ep, BYTE type)
{
    EVENTMODIFIER mod;
    for (mod = ep->mods; mod != NIL; mod = mod->next) {
        if (mod->kind == type) {
            return mod;
        }
    }
    
    return NIL;
}

/*========================================================================
 * Function:        findBreakEventAndOpcode()
 * Overview:        Search through our list of breakpoints to find this one 
 *              that we hit
 * Interface:
 *  parameters:     pointer to opcode, mod, class, method , offset
 *  returns:        pointer to breakpoint structure
 *
 * Notes:
 *=======================================================================*/

static VMEventPtr findBreakEventAndOpcode(ByteCode *opcode,
        EVENTMODIFIER *modpp,
        unsigned long classID, 
        unsigned long thisMethod, 
        unsigned long offset)
{
    VMEventPtr ep = eventHead;
    EVENTMODIFIER mod;
    if (ep == NIL) { 
        return (NIL);       /* no breakpoints??? */
    }
    do {
        if (ep->kind == JDWP_EventKind_BREAKPOINT) {
            if ((mod = getModifier(ep, JDWP_EventRequest_Set_Out_modifiers_Modifier_LocationOnly)) != NIL) {
                if (mod->u.loc.classID == classID && 
                    mod->u.loc.methodIndex == thisMethod && 
                    mod->u.loc.offset == offset     
                    /*&& mod->u.loc.tag    == tag */) {
                    if (opcode != NIL)
                        *opcode = mod->u.loc.opcode;
                    if (modpp != NIL)
                        *modpp = mod;
                    return (ep); 
                }
            }
        }
    } while ((ep = ep->next));
    return NIL;
}

static bool_t 
checkClassMatch(CEModPtr cep, EVENTMODIFIER em)
{
    bool_t result = TRUE;
    START_TEMPORARY_ROOTS
        CLASS clazz = GET_DEBUGGERID_CLASS(cep->loc.classID);
        DECLARE_TEMPORARY_ROOT(char *, fullName, getClassName(clazz));
        char *modName =  em->u.classMatch.className;
        unsigned int modNameLength = strlen(modName);
        char *p;

        /* make sure that fullName is a 'standard' classname with '.'s */
        p = fullName;
        while (*p != '\0') {
            if (*p == '/' || *p == '\\')
                *p = '.';
            p++;
        }
        
        if (modName[0] == '*' || modName[modNameLength - 1] == '*') {
            /* We have a prefix/suffix match.  The rest of match has to be
             * the same as the tail/head of fullName */
            int compLen = modNameLength - 1;
            int fullNameLength = strlen(fullName);
            int offset = fullNameLength - compLen;
            char *start;
            if (offset < 0) { 
                result = FALSE;
            } else { 
                if (modName[0] == '*') {
                    /* suffix match */
                    modName++;
                    start = fullName + offset;
                } else {
                    start = fullName;
                }
                result = (strncmp(modName, start, compLen) == 0);
            }
        } else { 
            /* Just compare the two. */
            result = (strcmp(modName, fullName) == 0);
        }
    END_TEMPORARY_ROOTS
    return result;
}

/*========================================================================
 * Function:        satisfiesModifiers()
 * Overview:        determines whether an event which has occured
 *                  satisfies its modifiers
 * Interface:
 *  parameters:     event pointer, current event pointer, mod pointer
 *
 * Notes:
 *=======================================================================*/

static bool_t satisfiesModifiers(VMEventPtr ep, CEModPtr cep, EVENTMODIFIER em) 
{
    
    if (em == NIL)
        return TRUE;
    switch (em->kind) {
    case JDWP_EventRequest_Set_Out_modifiers_Modifier_Conditional:
        /* reserved for future use */
        break;
    case JDWP_EventRequest_Set_Out_modifiers_Modifier_ThreadOnly:
        if (cep->threadID != em->u.threadID)
            return FALSE;
        break;
    case JDWP_EventRequest_Set_Out_modifiers_Modifier_ClassOnly:
        return (em->u.classID == cep->loc.classID);
    case JDWP_EventRequest_Set_Out_modifiers_Modifier_ClassMatch:
        return checkClassMatch(cep, em);

    case JDWP_EventRequest_Set_Out_modifiers_Modifier_ClassExclude:
        break;
    case JDWP_EventRequest_Set_Out_modifiers_Modifier_LocationOnly:
        if ((cep->loc.classID != em->u.loc.classID) ||
             (cep->loc.methodIndex != em->u.loc.methodIndex) ||
             (cep->loc.offset  != em->u.loc.offset))
             return FALSE;
        break;
    case JDWP_EventRequest_Set_Out_modifiers_Modifier_ExceptionOnly: {
        bool_t ret = TRUE;
        if (em->u.exp.classID != 0) {
            INSTANCE_CLASS clazz = (INSTANCE_CLASS)GET_DEBUGGERID_CLASS(cep->exp.classID);
            while (clazz != NULL) {
                if (GET_CLASS_DEBUGGERID(&clazz->clazz) == em->u.exp.classID) {
                    ret = TRUE;
                    break;
                }
                clazz = clazz->superClass;
            }
        }

        ret = ret && ((em->u.exp.sig_caught && cep->exp.sig_caught) ||
            (em->u.exp.sig_uncaught && cep->exp.sig_uncaught));

        if (!ret)
            return FALSE;

        break;
        }
    case JDWP_EventRequest_Set_Out_modifiers_Modifier_FieldOnly:
        break;
    case JDWP_EventRequest_Set_Out_modifiers_Modifier_Step:
        if ((cep->step.threadID != em->u.step.threadID) ||
            (cep->step.size != em->u.step.size) ||
            (cep->step.depth != em->u.step.depth)) {
    /*
    || (cep->step.target.cl != em->u.step.target.cl)
    || (cep->step.target.method != em->u.step.target.method)
    || (cep->step.target.offset != em->u.step.target.offset))
    */
             return FALSE;
        }
        break;
    }
    return TRUE;
}

/*========================================================================
 * Function:        checkEventCount()
 * Overview:        deals with events with a count modifier
 * Interface:
 * parameters:      event kind
 * returns:     nothing
 *
 * Notes:
 *=======================================================================*/

static void checkEventCount(VMEventPtr ep)
{
    if (ep->count_active) {
        if (ep->count == 0) {
            /* if it's 0 then the count was satisfied */
            ep->active = FALSE;
            ep->count_active = FALSE;
            clearEvent(ep);
        }
    }
}

/*========================================================================
 * Function:        findSatisfyingEvent()
 * Overview:        finds a specific type of event that also satisfies
 *                  its modifiers
 * Interface:
 * parameters:      event kind, pointer to current event structure
 *            pointer to return list of events, count of events
 * returns:     true if event to process, false otherwise
 *
 * Notes:
 *=======================================================================*/

static bool_t findSatisfyingEvent(BYTE kind, CEModPtr cep, VMEventPtr *epp,
    int *eventCount, BYTE *suspendPolicy)
{
    VMEventPtr ep = eventHead;
    EVENTMODIFIER em;

    *epp = NULL;
    *eventCount = 0;
    *suspendPolicy = 0;
    if (ep == NIL)
        return (FALSE);     /* no events */
    do {
        if (!ep->active)
            continue;

        if (ep->kind == kind) {
            /*
             * The count modifier is a global modifier that filters all
             * other modifiers, hence it's in the VMEvent structure
             */
            if (ep->count_active) {
                if (ep->count-- > 1) {
                    continue;
                }
            }
        /* check modifiers */
        em = ep->mods;
        do  {
            if (em == NULL || satisfiesModifiers(ep, cep, em)) {
                /* add to list of events to send */
            ep->sendNext = NULL;
                if (*epp == NULL)  {
                    *epp = ep;
            } else {
                ep->sendNext = *epp;
                *epp = ep;
            }
            (*eventCount)++;
            if (ep->suspendPolicy > *suspendPolicy)
                *suspendPolicy = ep->suspendPolicy;
            break;
            }
            
        } while (em && (em = em->next) != NULL);
        }
    } while ((ep = ep->next));
    if (*epp != NULL)
        return TRUE;
    else
        return FALSE;
}

/*========================================================================
 * Function:        setNotification()
 * Overview:        tells the debugger to only listen for only 
 *                  one type of event
 *                  
 * Interface:
 *  parameters:     debugger event type
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

static void setNotification(long n)
{
    debuggerNotifyList = n;
}

/*========================================================================
 * Function:        addNotification()
 * Overview:        tells the debugger to listen to the event as well as
 *                  anything else that was set previously
 *                  
 * Interface:
 *  parameters:     debugger event type
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

static void addNotification(long n)
{
    debuggerNotifyList |= n;
}

/*========================================================================
 * Function:        FreeCEModifier()
 * Overview:        free a modifier structure for an event
 *                  
 * Interface:
 *  parameters:     
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

void FreeCEModifier(CEModPtr cep) {

    cep->inUse = FALSE;
    cep->nextEvent = NULL;
}

/*========================================================================
 * Function:        GetCEModifier()
 * Overview:        get a free modifier structure for an event
 *                  
 * Interface:
 *  parameters:     
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

CEModPtr GetCEModifier() {

    CEModPtr cep = CE_ModHead;
    CEModPtr lastCep;

    while (cep) {
        if (!cep->inUse) {
            lastCep = cep->next;
            memset(cep, 0, sizeof(*cep));
            cep->next = lastCep;
            cep->inUse = TRUE;
            return (cep);
        }
        lastCep = cep;
        cep = cep->next;
    }
    cep = (CEModPtr)allocAlignedBytes(sizeof(*cep));
    if (CE_ModHead == NIL) {
        CE_ModHead = cep;
    } else {
        lastCep->next = cep;
    }
    cep->inUse = TRUE;
    return(cep);
}

/*========================================================================
 * Function:        getModPtr()
 * Overview:        get a free modifier structure for an event
 *                  
 * Interface:
 *  parameters:     
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

static EVENTMODIFIER getModPtr() {

    EVENTMODIFIER newMod = modHead;
    EVENTMODIFIER lastMod = NULL;

    while (newMod) {
        if (!newMod->inUse) {
            lastMod = newMod->freeNext;
            memset(newMod, 0, sizeof(*newMod));
            newMod->freeNext = lastMod;
            newMod->inUse = TRUE;
            return (newMod);
        }
        lastMod = newMod;
        newMod = newMod->freeNext;
    }
    newMod = (EVENTMODIFIER)allocAlignedBytes(sizeof(*newMod));
    if (modHead == NIL) {
            modHead = newMod;
    } else {
        lastMod->freeNext = newMod;
    }
    newMod->inUse = TRUE;
    return(newMod);
}

/*========================================================================
 * Function:        createVmEvent()
 * Overview:        creates an event structure but doesn't put it in
 *                  the list of events
 *                  
 * Interface:
 *  parameters:     
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

static VMEventPtr createVmEvent()
{
    VMEventPtr  ep = eventHead;
    VMEventPtr lastEp = NULL;

    /* first try to find an event to be reaped */
    while (ep != NULL) {
        if (!ep->inUse) {
            lastEp = ep->next;
            memset(ep, 0, sizeof(*ep));
            ep->next = lastEp;
            ep->active = TRUE;
            ep->mods = NIL;
            ep->numModifiers = 0;
            ep->inUse = TRUE;
            return (ep);
        }
        lastEp = ep;
        ep = ep->next;
    }
    ep = (VMEventPtr)callocPermanentObject(SIZEOF_VMEVENT);
    ep->active = TRUE;
    ep->mods = NIL;
    ep->numModifiers = 0;
    ep->inUse = TRUE;
    if (eventHead == NIL) {
            eventHead = ep;
    } else {
        lastEp->next = ep;
    }
    return ep;
}

/*========================================================================
 * Function:        installBreakpoint()
 * Overview:        set a breakpoint in the given method
 * Interface:
 *   parameters:    pointer to event modifier
 *   returns:       TRUE if installed, FALSE otherwise
 *
 *=======================================================================*/

static bool_t installBreakpoint(EVENTMODIFIER *mods)
{
    unsigned char bk = BREAKPOINT;
    unsigned long classID = (*mods)->u.loc.classID;
    INSTANCE_CLASS clazz = (INSTANCE_CLASS)GET_DEBUGGERID_CLASS(classID);
    METHOD thisMethod = &clazz->methodTable->methods[(*mods)->u.loc.methodIndex - _method_index_base];
    unsigned int offset = (*mods)->u.loc.offset;
    ByteCode opcode;
    char *start;

#if INCLUDEDEBUGCODE
    /* Determine if this is a legal offset 
     * into the bytecode for this method 
     */
    if (!isLegalOffset(thisMethod, offset))
        return FALSE;
#endif

    if (thisMethod->u.java.code[offset] == BREAKPOINT) {
        if (findBreakEventAndOpcode(&opcode, NIL, classID,
             (*mods)->u.loc.methodIndex, offset) == NIL) {
             return FALSE;
        }
    } else { 
        opcode = thisMethod->u.java.code[offset];
    }
    start = (char *)((INSTANCE_CLASS)GET_DEBUGGERID_CLASS(classID))->constPool;
    setBreakpoint(thisMethod, offset, start, bk);
    (*mods)->u.loc.opcode = opcode;

    return TRUE;
}

/*========================================================================
 * Function:        processBreakCommands()
 * Overview:        called  after we send an event to debugger
 * Interface:
 *   parameters:    none
 *   returns:       nothing
 *
 * Notes:
 *=======================================================================*/

void processBreakCommands()
{
    resumeOK = FALSE;

    /* wait for some command to come from the debugger */
    for (;;) {
        ProcessDebugCmds(-1);   /* wait forever */
        if (resumeOK) {
            resumeOK = FALSE;
            return;
        }
    }
}

/*========================================================================
 * Function:        suspendAllThreads()
 * Overview:        Suspends all threads
 *                  
 * Interface:
 *  parameters:     void
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

static void 
suspendAllThreads() 
{ 
    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(THREAD, tptr, AllThreads);
        for (; tptr != NIL; tptr = tptr->nextAliveThread) {
            suspendSpecificThread(tptr);
        }
        allThreadsSuspended = TRUE;
        if (waitOnSuspend) { 
            processBreakCommands();
        }
    END_TEMPORARY_ROOTS
}

/*========================================================================
 * Function:        handleSuspendPolicy()
 * Overview:        determines which suspend policy this event was to
 *                  use and acts appropriately
 *                  
 * Interface:
 *  parameters:     policy, thread pointer, bool_t (unused)
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

static void handleSuspendPolicy(BYTE policy, THREAD thread, bool_t forceWait)
{
    switch(policy)
    {
        case JDWP_SuspendPolicy_NONE:
            /* do nothing */
            break;
        case JDWP_SuspendPolicy_EVENT_THREAD:
            suspendSpecificThread(thread);
            break;
        case JDWP_SuspendPolicy_ALL:
            waitOnSuspend = forceWait;
            suspendAllThreads();
            break;
    }
    
}

/*========================================================================
 * Function:        getMethodIndex()
 * Overview:        given a class and method address, find the index
 *          of that method in the methodtable
 *
 * Interface:
 *  parameters: Class, method
 *  returns:    int
 *
 * Notes:
 *=======================================================================*/

static long getMethodIndex(METHOD thisMethod) {
    return thisMethod - thisMethod->ofClass->methodTable->methods + _method_index_base;
}

/*========================================================================
 * Function:        replaceEvnetOpcode()
 * Overview:        replace the opcode in the breakpoint event with the new
 *                      fast bytecode
 * Interface:
 *   parameters:    new fast bytecode
 *   returns:       nothing
 *
 *=======================================================================*/

void replaceEventOpcode(ByteCode bcode)
{

    METHOD thisMethod = fp_global->thisMethod;
    INSTANCE_CLASS clazz = thisMethod->ofClass;
    unsigned long classID = GET_CLASS_DEBUGGERID(clazz);
    VMEventPtr ep;
    unsigned long offset;
    ByteCode opcode;
    EVENTMODIFIER modp;

    offset = ip_global - thisMethod->u.java.code;   /* where are we? */

    /* find this breakpoint */
    ep = findBreakEventAndOpcode(&opcode, &modp,
         GET_CLASS_DEBUGGERID(&clazz->clazz),
         getMethodIndex(thisMethod), offset);
    if (ep == NIL) {
        *ip_global = bcode;
    } else {
        modp->u.loc.opcode = bcode;
    }
    return;
}

static void sendEvent(CEModPtr cep) {

    switch (cep->eventKind) {
    case JDWP_EventKind_CLASS_PREPARE:
        setEvent_ClassPrepare(cep);
        break;

    case JDWP_EventKind_CLASS_LOAD:
        setEvent_ClassLoad(cep);
        break;

    case JDWP_EventKind_THREAD_START:
        setEvent_ThreadStart(cep);
        break;
    }
}

void insertDebugEvent(CEModPtr cep) {

    CEModPtr lastcep;

    if (CurrentThread != NULL) {
        if (CurrentThread->debugEvent == NULL)
            CurrentThread->debugEvent = cep;
        else {
            lastcep = CurrentThread->debugEvent;
            while (lastcep->nextEvent != NULL) {
                lastcep = lastcep->nextEvent;
            }
            lastcep->nextEvent = cep;
        }
        CurrentThread->needEvent = TRUE;
        signalTimeToReschedule();
    } else {
        sendEvent(cep);
        FreeCEModifier(cep);
    }
}

void checkDebugEvent(THREAD tptr) {

    CEModPtr cep;

    START_TEMPORARY_ROOTS
    IS_TEMPORARY_ROOT(tptr, tptr);
    tptr->needEvent = FALSE;
    while ((cep = tptr->debugEvent) != NULL) {
        sendEvent(cep);
        tptr->debugEvent = cep->nextEvent;
        FreeCEModifier(cep);
    }
    END_TEMPORARY_ROOTS
    return;
}

/*========================================================================
 * Function:        setEvent_NeedSteppingInfo()
 * Overview:        tell the _proxy_ that _we_ need info about class lines
 *                  
 * Interface:
 *  parameters:         pointer to thread that's stepping
 *              pointer to CEMod structure
 *  returns:        void
 *
 * Notes:   Since this is called re-entrantly via CreateEvent, we need
 *      to use our own output packet since on return to CreateEvent
 *      and thence to processCommands, we don't want to have blown the
 *      global outputStream  used for most commands
 *=======================================================================*/

static void setEvent_NeedSteppingInfo(unsigned long threadID, CEModPtr cep)
{
    FRAME frame;
    unsigned long offset;
    unsigned long methodIndex;
    METHOD thisMethod;

    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(THREAD, thread, GET_VMTHREAD(threadID));
        DECLARE_TEMPORARY_ROOT(PACKET_INPUT_STREAM, out,
                               outStream_newCommand(FLAGS_None, KVM_CMDSET,
                                                   KVM_STEPPING_EVENT_COMMAND));
        DEBUGGER_EVENT(("Need Stepping Info"));
        if (thread == CurrentThread) {
            frame =  fp_global;
            offset = ip_global - frame->thisMethod->u.java.code;
        } else {
            frame = thread->fpStore;
            offset = thread->ipStore - frame->thisMethod->u.java.code;
        }       
        /* Note that frame is volatile, and can be bashed by a GC */

        thisMethod = frame->thisMethod;
        methodIndex = getMethodIndex(thisMethod);
        outStream_writeLong(&out, (long)cep);
        outStream_writeClass(&out, (CLASS)(thisMethod->ofClass));
        outStream_writeLong(&out, methodIndex);
        outStream_writeLong64(&out, offset);
        outStream_sendCommand(&out);
    END_TEMPORARY_ROOTS
}

/*========================================================================
 * Function:        setEvent_VMInit()
 * Overview:        called when the vm starts
 *                  
 * Interface:
 *  parameters:     none
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

void setEvent_VMInit()
{
    VMEventPtr reqID;
    reqID = NIL;

    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(PACKET_OUTPUT_STREAM, out,
                        outStream_newCommand(FLAGS_None,
                                             JDWP_COMMAND_SET(Event), 
                                             JDWP_COMMAND(Event, Composite)));
        DEBUGGER_EVENT(("VM Init"));
        if (suspend) {
            /* Suspend all threads by default */
            DEBUGGER_EVENT(("VM Suspend All"));
            outStream_writeByte(&out, JDWP_SuspendPolicy_ALL);
        } else {
            /* Suspend no threads */
            DEBUGGER_EVENT(("VM Suspend None"));
            outStream_writeByte(&out, JDWP_SuspendPolicy_NONE);
        }
        outStream_writeLong(&out, 1);
        outStream_writeByte(&out, JDWP_EventKind_VM_START);
        outStream_writeLong(&out, (long)reqID);
        /* hopefully this will be called when CurrentThread==initial_thread */
        outStream_writeThread(&out, CurrentThread);
        outStream_sendCommand(&out);
    
        if (suspend) {
            /* Use suspend policy which suspends all threads by default */ 
            handleSuspendPolicy(JDWP_SuspendPolicy_ALL, CurrentThread, TRUE);
        } else {
            /* Use suspend policy which suspends no threads */
            DEBUGGER_EVENT(("VM SuspendPolicy not specified"));
            /*
            handleSuspendPolicy(JDWP_SuspendPolicy_NONE, CurrentThread, TRUE);
            */
        }
    END_TEMPORARY_ROOTS
}

/*========================================================================
 * Function:        setEvent_VMDeath()
 * Overview:        called when the vm terminates
 *                  
 * Interface:
 *  parameters:     none
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

void setEvent_VMDeath()
{
    checkNOTIFY_WANTED(Dbg_EventKind_VM_DEATH);

    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(PACKET_OUTPUT_STREAM, out, 
                        outStream_newCommand(FLAGS_None,
                                             JDWP_COMMAND_SET(Event), 
                                             JDWP_COMMAND(Event, Composite)));
        DEBUGGER_EVENT(("VM Death"));
        outStream_writeByte(&out, JDWP_SuspendPolicy_ALL);
        outStream_writeLong(&out, 1);
        outStream_writeByte(&out, JDWP_EventKind_VM_DEATH);
        outStream_writeLong(&out, 0); /* spec says always zero */
        outStream_sendCommand(&out);
    END_TEMPORARY_ROOTS
}

/*========================================================================
 * Function:        setEvent_MidletDeath()
 * Overview:        called when a midlet is dying
 *                  
 * Interface:
 *  parameters:     midlet class
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

void setEvent_MidletDeath(CEModPtr cep)
{
    CLASS clazz = GET_DEBUGGERID_CLASS(cep->loc.classID);
    VMEventPtr ep, epFirst;
    int eventCount = 0;
    BYTE suspendPolicy = JDWP_SuspendPolicy_NONE;

    checkNOTIFY_WANTED(Dbg_EventKind_MIDLET_DEATH);

    if (!IS_ARRAY_CLASS(clazz) &&
        ((INSTANCE_CLASS)clazz)->status < CLASS_LOADED) { 
            return;
    }
    /* In case of an allocation in outStream_writeBytes */
    if (findSatisfyingEvent(JDWP_EventKind_MIDLET_DEATH, cep, &epFirst,
                            &eventCount, &suspendPolicy)) {
        START_TEMPORARY_ROOTS
            DECLARE_TEMPORARY_ROOT(PACKET_OUTPUT_STREAM, out, 
                        outStream_newCommand(FLAGS_None,
                                             JDWP_COMMAND_SET(Event), 
                                             JDWP_COMMAND(Event, Composite)));
            DEBUGGER_EVENT(("Midlet Death %s", getClassName(clazz)));
            ep = epFirst;
            outStream_writeByte(&out, suspendPolicy);
            outStream_writeLong(&out, eventCount);
            while (ep != NULL) {
                outStream_writeByte(&out, JDWP_EventKind_MIDLET_DEATH);        
                outStream_writeLong(&out, (long)ep);
                outStream_writeClassName(&out, clazz);

                checkEventCount(ep);
                ep = ep->sendNext;
            }
            outStream_sendCommand(&out);
            handleSuspendPolicy(suspendPolicy, GET_VMTHREAD(cep->threadID), FALSE);
        END_TEMPORARY_ROOTS
    }
}

/*========================================================================
 * Function:        setEvent_SingleStep
 * Overview:        
 *                  
 * Interface:
 *  parameters:     none
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

void setEvent_SingleStep(CEModPtr cep, THREAD threadArg)
{
    VMEventPtr ep, epFirst;
    int eventCount = 0;
    BYTE suspendPolicy = JDWP_SuspendPolicy_NONE;
    checkNOTIFY_WANTED(Dbg_EventKind_SINGLE_STEP);

    /* In case of an allocation in outStream_writeBytes */
    if (findSatisfyingEvent(JDWP_EventKind_SINGLE_STEP, cep, &epFirst,
                            &eventCount, &suspendPolicy)) {
        ep = epFirst;
        START_TEMPORARY_ROOTS
            DECLARE_TEMPORARY_ROOT(PACKET_OUTPUT_STREAM, out, 
                          outStream_newCommand(FLAGS_None,
                                               JDWP_COMMAND_SET(Event), 
                                               JDWP_COMMAND(Event, Composite)));
            DECLARE_TEMPORARY_ROOT(THREAD, thread, threadArg);
            DEBUGGER_EVENT(("Single Step"));
            outStream_writeByte(&out, suspendPolicy);
            outStream_writeLong(&out, eventCount);
            while (ep != NULL) {
                outStream_writeByte(&out, JDWP_EventKind_SINGLE_STEP);
                outStream_writeObjectID(&out, (long)ep);
                outStream_writeThread(&out, thread);
                outStream_writeLocation(&out, JDWP_TypeTag_CLASS,
                                        cep->loc.classID, 
                                        cep->loc.methodIndex,
                                        cep->loc.offset);
                checkEventCount(ep);
                ep = ep->sendNext;
            }
            outStream_sendCommand(&out);
            handleSuspendPolicy(suspendPolicy, thread, FALSE);
        END_TEMPORARY_ROOTS
    }
}

/*========================================================================
 * Function:        setEvent_Breakpoint()
 * Overview:        sends a break event to debugger
 * Interface:
 *   parameters:    pointer to breakpoint structure, type of event
 *   returns:       nothing
 *
 * Notes:       writes out a composite event packet per JDWP spec
 *=======================================================================*/

void setEvent_Breakpoint(VMEventPtr ep, CEModPtr cep)
{
    EVENTMODIFIER em;

    /* BYTE suspendPolicy = JDWP_SuspendPolicy_ALL; */

    if (!NOTIFY_WANTED(Dbg_EventKind_SINGLE_STEP) &&
         !NOTIFY_WANTED(Dbg_EventKind_BREAKPOINT))
         return;

    em = ep->mods;
    if (!satisfiesModifiers(ep, cep, em)) {
        return; 
    }

    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(PACKET_OUTPUT_STREAM, out, 
                      outStream_newCommand(FLAGS_None,
                                           JDWP_COMMAND_SET(Event), 
                                           JDWP_COMMAND(Event, Composite)));
        DEBUGGER_EVENT(("Breakpoint at method: 0x%lx, classID: 0x%lx, offset: 0x%lx", cep->loc.methodIndex, cep->loc.classID, (int)cep->loc.offset));
        outStream_writeByte(&out, ep->suspendPolicy);
        
        /* count is 1 for now */
        outStream_writeLong(&out, 1);
        outStream_writeByte(&out, JDWP_EventKind_BREAKPOINT);
        outStream_writeLong(&out, (long)ep);
        outStream_writeThread(&out, CurrentThread);
        
        /* write the Location */
        outStream_writeLocation(&out, JDWP_TypeTag_CLASS,
                                cep->loc.classID, cep->loc.methodIndex,
                                cep->loc.offset);

        outStream_sendCommand(&out);
        handleSuspendPolicy(ep->suspendPolicy, CurrentThread, FALSE);
    END_TEMPORARY_ROOTS
}

/*========================================================================
 * Function:        setEvent_MethodEntry()
 * Overview:        called when the code enters a method
 *                  
 * Interface:
 *  parameters:     none
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

void setEvent_MethodEntry()
{
    checkNOTIFY_WANTED(Dbg_EventKind_METHOD_ENTRY);
}

/*========================================================================
 * Function:        setEvent_MethodExit()
 * Overview:        called when the code exits a method
 *                  
 * Interface:
 *  parameters:     
 *  returns:        
 *
 * Notes:
 *=======================================================================*/

void setEvent_MethodExit()
{
    checkNOTIFY_WANTED(Dbg_EventKind_METHOD_EXIT);
}

/*========================================================================
 * Function:        setEvent_ThreadStart()
 * Overview:        called when a thread is created
 *                  
 * Interface:
 *  parameters:     none
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

void setEvent_ThreadStart(CEModPtr cep)
{
    VMEventPtr ep, epFirst;
    int eventCount = 0;
    BYTE suspendPolicy = JDWP_SuspendPolicy_NONE;

    checkNOTIFY_WANTED(Dbg_EventKind_THREAD_START);

    if (findSatisfyingEvent(JDWP_EventKind_THREAD_START, cep, &epFirst,
                            &eventCount, &suspendPolicy)) {
        ep = epFirst;
        START_TEMPORARY_ROOTS
            DECLARE_TEMPORARY_ROOT(PACKET_OUTPUT_STREAM, out, 
                       outStream_newCommand(FLAGS_None,
                                            JDWP_COMMAND_SET(Event), 
                                            JDWP_COMMAND(Event, Composite)));
            DEBUGGER_EVENT(("Thread Start ID=%lx", cep->threadID));
            outStream_writeByte(&out, suspendPolicy);
            outStream_writeLong(&out, eventCount);
            while(ep != NULL) {
                outStream_writeByte(&out, JDWP_EventKind_THREAD_START);
                outStream_writeLong(&out, (long)ep);
                outStream_writeObjectID(&out, cep->threadID);
                checkEventCount(ep);
                ep = ep->sendNext;
            }
            outStream_sendCommand(&out);
            handleSuspendPolicy(suspendPolicy, GET_VMTHREAD(cep->threadID), FALSE);
        END_TEMPORARY_ROOTS
    }
}

/*========================================================================
 * Function:        setEvent_ThreadDeath()
 * Overview:        called when a thread terminates
 *                  
 * Interface:
 *  parameters:     
 *  returns:        
 *
 * Notes:
 *=======================================================================*/

void setEvent_ThreadDeath(CEModPtr cep)
{
    VMEventPtr ep, epFirst;
    int eventCount = 0;
    BYTE suspendPolicy = JDWP_SuspendPolicy_NONE;

    checkNOTIFY_WANTED(Dbg_EventKind_THREAD_END);

    if (findSatisfyingEvent(JDWP_EventKind_THREAD_END, cep, &epFirst,
                            &eventCount, &suspendPolicy)) {    
        ep = epFirst;
        START_TEMPORARY_ROOTS
            DECLARE_TEMPORARY_ROOT(PACKET_OUTPUT_STREAM, out, 
                        outStream_newCommand(FLAGS_None,
                                             JDWP_COMMAND_SET(Event), 
                                             JDWP_COMMAND(Event, Composite)));
            DEBUGGER_EVENT(("Thread Death ID=%lx", cep->threadID));
            outStream_writeByte(&out, suspendPolicy);
            outStream_writeLong(&out, eventCount);
            while (ep != NULL) {
                outStream_writeByte(&out, JDWP_EventKind_THREAD_END);
                outStream_writeLong(&out, (long)ep);
                outStream_writeObjectID(&out, cep->threadID);
                checkEventCount(ep);
                ep = ep->sendNext;
            } 
            outStream_sendCommand(&out);
            handleSuspendPolicy(suspendPolicy, GET_VMTHREAD(cep->threadID), FALSE);
        END_TEMPORARY_ROOTS
    }
}

/*========================================================================
 * Function:        getJDWPClassStatus()
 * Overview:        translates an internal class status 
 *                  into the JDWP equivalent.
 *                  
 * Interface:
 *  parameters:     class pointer
 *  returns:        JDWP class status
 *
 * Notes:
 *=======================================================================*/

int getJDWPClassStatus(INSTANCE_CLASS clazz)
{
    int ret = 0;

    if (IS_ARRAY_CLASS(clazz)) { 
        return JDWP_ClassStatus_INITIALIZED;
    }
    switch (clazz->status) {
        case CLASS_ERROR:
            return JDWP_ClassStatus_ERROR;
        /* case CLASS_RAW: */
        case CLASS_READY:
            ret = JDWP_ClassStatus_INITIALIZED;
        /* fall through */
        case CLASS_LOADING:
        case CLASS_LOADED:
        case CLASS_LINKED:
            ret |= JDWP_ClassStatus_PREPARED;
        /* fall through */
        case CLASS_VERIFIED:
            ret |= JDWP_ClassStatus_VERIFIED;
            break;
        }
    return ret;
}

/*========================================================================
 * Function:        getJDWPThreadStatus()
 * Overview:        translates an internal thread status
 *                  into the JDWP equivalent
 *                  
 * Interface:
 *  parameters:     thread pointer
 *  returns:        JDWP thread status
 *
 * Notes:
 *=======================================================================*/

static int getJDWPThreadStatus(THREAD thread)
{
    int state = thread->state;
    if (state == THREAD_SUSPENDED) {
        return JDWP_ThreadStatus_RUNNING;
    } else {
        state &= ~THREAD_SUSPENDED;
        state &= ~THREAD_DBG_SUSPENDED;
        switch (state) {
            case THREAD_ACTIVE:
                return JDWP_ThreadStatus_RUNNING;
            case THREAD_MONITOR_WAIT:
                return JDWP_ThreadStatus_MONITOR;
            case THREAD_CONVAR_WAIT:
                return JDWP_ThreadStatus_WAIT;
            case THREAD_DEAD:
                return JDWP_ThreadStatus_ZOMBIE;
            default:
                return JDWP_ThreadStatus_UNKNOWN;
        }
    }
}

/*========================================================================
 * Function:        getJDWPThreadSuspendStatus()
 * Overview:        returns if this thread has been suspended 
 *                  through the debugger
 *                  
 * Interface:
 *  parameters:     thread pointer
 *  returns:        suspend status
 *
 * Notes:
 *=======================================================================*/

static int getJDWPThreadSuspendStatus(THREAD thread) 
{
    int ret = 0;
    int state = thread->state;
    
    if (state == THREAD_SUSPENDED) { 
        ret |= JDWP_SuspendStatus_SUSPEND_STATUS_SUSPENDED;
    }

    if (state & THREAD_DBG_SUSPENDED) { 
        ret |= JDWP_SuspendStatus_SUSPEND_STATUS_SUSPENDED;
    }
        
    if ((state & THREAD_MONITOR_WAIT) || (state & THREAD_CONVAR_WAIT)) { 
        ret |= JDWP_SuspendStatus_SUSPEND_STATUS_SUSPENDED;
    }

    return ret;
}

/*========================================================================
 * Function:        getJDWPTagType()
 * Overview:        determines what kind of class a class is
 *                  (array, interface, class)
 *                  
 * Interface:
 *  parameters:     class pointer
 *  returns:        tag type
 *
 * Notes:
 *=======================================================================*/

static BYTE getJDWPTagType(CLASS clazz) 
{
    if (IS_ARRAY_CLASS(clazz)) { 
        return JDWP_TypeTag_ARRAY;
    } else if (clazz->accessFlags & ACC_INTERFACE) {
        return JDWP_TypeTag_INTERFACE;
    } else { /* is a class */
        return JDWP_TypeTag_CLASS;
    }
}

void sendAllClassPrepares() 
{
    CEModPtr cep;
#if INCLUDEDEBUGCODE
    int old_tracedebugger = tracedebugger;
    tracedebugger = 0;
    if (old_tracedebugger) { 
        DEBUGGER_EVENT(("Class Prepare ... All Classes"));
    } 
#endif
    if (NOTIFY_WANTED(Dbg_EventKind_CLASS_PREPARE)) {
        cep = GetCEModifier();
        FOR_ALL_CLASSES(clazz)
            cep->loc.classID = GET_CLASS_DEBUGGERID(clazz);
            cep->threadID = CurrentThread == NULL 
                          ? 0
                          : getObjectID((OBJECT)CurrentThread->javaThread);
            setEvent_ClassPrepare(cep);
        END_FOR_ALL_CLASSES
        FreeCEModifier(cep);
    }
#if INCLUDEDEBUGCODE
    tracedebugger = old_tracedebugger;
#endif

}

/*========================================================================
 * Function:        setEvent_ClassPrepare()
 * Overview:        called when a class is loading
 *                  
 * Interface:
 *  parameters:     thread pointer, class pointer
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

void setEvent_ClassPrepare(CEModPtr cep)
{
    CLASS clazz = GET_DEBUGGERID_CLASS(cep->loc.classID);
    VMEventPtr ep, epFirst;
    int eventCount = 0;
    BYTE suspendPolicy = JDWP_SuspendPolicy_NONE;

    checkNOTIFY_WANTED(Dbg_EventKind_CLASS_PREPARE);

    if (!IS_ARRAY_CLASS(clazz) &&
        ((INSTANCE_CLASS)clazz)->status < CLASS_LOADED) { 
            return;
    }
    /* In case of an allocation in outStream_writeBytes */
    if (findSatisfyingEvent(JDWP_EventKind_CLASS_PREPARE, cep, &epFirst,
                            &eventCount, &suspendPolicy)) {
        START_TEMPORARY_ROOTS
            DECLARE_TEMPORARY_ROOT(PACKET_OUTPUT_STREAM, out, 
                        outStream_newCommand(FLAGS_None,
                                             JDWP_COMMAND_SET(Event), 
                                             JDWP_COMMAND(Event, Composite)));
            DEBUGGER_EVENT(("Class Prepare %s", getClassName(clazz)));
            ep = epFirst;
            outStream_writeByte(&out, suspendPolicy);
            outStream_writeLong(&out, eventCount);
            while (ep != NULL) {
                outStream_writeByte(&out, JDWP_EventKind_CLASS_PREPARE);        
                outStream_writeLong(&out, (long)ep);
                outStream_writeObjectID(&out, cep->threadID);

                outStream_writeByte(&out, getJDWPTagType(clazz));
                outStream_writeClass(&out, clazz);
                
                outStream_writeClassName(&out, clazz);

                outStream_writeLong(&out,
                                    getJDWPClassStatus((INSTANCE_CLASS)clazz));

                checkEventCount(ep);
                ep = ep->sendNext;
            }
            outStream_sendCommand(&out);
            handleSuspendPolicy(suspendPolicy, GET_VMTHREAD(cep->threadID), FALSE);
        END_TEMPORARY_ROOTS
    }
}

/*========================================================================
 * Function:        setEvent_ClassLoad()
 * Overview:        called when a class is loaded
 *                  
 * Interface:
 *  parameters:     thread pointer, class pointer
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

void setEvent_ClassLoad(CEModPtr cep)
{
    checkNOTIFY_WANTED(Dbg_EventKind_CLASS_LOAD);
    setEvent_ClassPrepare(cep);
}

/*========================================================================
 * Function:        setEvent_ClassUnload()
 * Overview:        called when a class is unloaded
 *                  
 * Interface:
 *  parameters:     thread pointer, class pointer
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

void setEvent_ClassUnload(CEModPtr cep)
{
    CLASS clazz = GET_DEBUGGERID_CLASS(cep->loc.classID);
    VMEventPtr ep, epFirst;
    int eventCount = 0;
    BYTE suspendPolicy = JDWP_SuspendPolicy_NONE;

    checkNOTIFY_WANTED(Dbg_EventKind_CLASS_UNLOAD);

    /* In case of an allocation in outStream_writeBytes */
    if (findSatisfyingEvent(JDWP_EventKind_CLASS_UNLOAD, cep, &epFirst,
                            &eventCount, &suspendPolicy)) {
        START_TEMPORARY_ROOTS
            DECLARE_TEMPORARY_ROOT(PACKET_OUTPUT_STREAM, out,
                          outStream_newCommand(FLAGS_None,
                                               JDWP_COMMAND_SET(Event), 
                                               JDWP_COMMAND(Event, Composite)));
            DEBUGGER_EVENT(("Class Unload %s", getClassName(clazz)));
            ep = epFirst;
            outStream_writeByte(&out, suspendPolicy);
            outStream_writeLong(&out, eventCount);
            while (ep != NULL) {
                outStream_writeByte(&out, JDWP_EventKind_CLASS_UNLOAD);
                outStream_writeLong(&out, (long)ep);
                outStream_writeClassName(&out, clazz);
                checkEventCount(ep);
                ep = ep->sendNext;
            }
            outStream_sendCommand(&out);
            handleSuspendPolicy(suspendPolicy, GET_VMTHREAD(cep->threadID), FALSE);
        END_TEMPORARY_ROOTS
    }
}

/*========================================================================
 * Function:        setEvent_FieldAccess()
 * Overview:        called when a field is accessed (read)
 *                  
 * Interface:
 *  parameters:     none
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

void setEvent_FieldAccess()
{
    checkNOTIFY_WANTED(Dbg_EventKind_FIELD_ACCESS);
}

/*========================================================================
 * Function:        setEvent_FieldModification()
 * Overview:        called when a field is modified
 *                  
 * Interface:
 *  parameters:     
 *  returns:        
 *
 * Notes:
 *=======================================================================*/

void setEvent_FieldModification()
{
    checkNOTIFY_WANTED(Dbg_EventKind_FIELD_MODIFICATION);
}

/*========================================================================
 * Function:        setEvent_FramePop()
 * Overview:        
 *                  
 * Interface:
 *  parameters:     
 *  returns:        
 *
 * Notes:
 *=======================================================================*/

void setEvent_FramePop()
{
    checkNOTIFY_WANTED(Dbg_EventKind_FRAME_POP);
}

/*========================================================================
 * Function:        setEvent_Exception()
 * Overview:        
 *                  
 * Interface:
 *  parameters:     
 *  returns:        
 *
 * Notes:
 *=======================================================================*/

void setEvent_Exception(THROWABLE_INSTANCE_HANDLE exceptionH, FRAME throwFrame, BYTE *throwIP,
                        METHOD catchMethod, unsigned long catchOffs, 
                        CEModPtr cep)
{
    VMEventPtr ep, epFirst;
    unsigned long throwOffset;
    THREAD thread = GET_VMTHREAD(cep->threadID);
    int eventCount = 0;
    BYTE suspendPolicy = JDWP_SuspendPolicy_NONE;
    METHOD thisMethod = throwFrame->thisMethod;
    
    checkNOTIFY_WANTED(Dbg_EventKind_EXCEPTION);
    
    if (thread->javaThread == NIL) {
        /* this isn't a java exception */
        return;
    }
    if (findSatisfyingEvent(JDWP_EventKind_EXCEPTION, cep, &epFirst,
                            &eventCount, &suspendPolicy)) {
        /* throwFrame is bashed if GC happens */
        throwOffset = throwIP - thisMethod->u.java.code;

        START_TEMPORARY_ROOTS
            DECLARE_TEMPORARY_ROOT(PACKET_OUTPUT_STREAM, out, 
                     outStream_newCommand(FLAGS_None,
                                          JDWP_COMMAND_SET(Event), 
                                          JDWP_COMMAND(Event, Composite)));
            /* Frame's are in the middle of a stack, and it is hard to make
             * them into a root */
            DEBUGGER_EVENT(("Exception"));
            ep = epFirst;
            outStream_writeByte(&out, suspendPolicy);
            outStream_writeLong(&out, eventCount);
            while (ep != NULL) {
                outStream_writeByte(&out, JDWP_EventKind_EXCEPTION);        
                outStream_writeLong(&out, (long)ep);
            
                /* thread with exception */
                outStream_writeObjectID(&out, cep->threadID);
            
                /* location of exception throw */
                outStream_writeLocation(&out, JDWP_TypeTag_CLASS, 
                     (GET_CLASS_DEBUGGERID(&thisMethod->ofClass->clazz)),
                     getMethodIndex(thisMethod), throwOffset);
    
                /* thrown exception */
                outStream_writeByte(&out, 'L');
                outStream_writeObject(&out, (OBJECT)(unhand(exceptionH)));
            
                /* location of catch, or 0 if not caught */
                if (catchMethod == NIL) {
                    outStream_writeLocation(&out, 0, 0, 0, 0);
                } else {
                    outStream_writeLocation(&out, JDWP_TypeTag_CLASS,
                         (GET_CLASS_DEBUGGERID(&catchMethod->ofClass->clazz)),
                         getMethodIndex(catchMethod), catchOffs);
                }
            
                checkEventCount(ep);
                ep = ep->sendNext;
            }
            outStream_sendCommand(&out);
            handleSuspendPolicy(suspendPolicy, GET_VMTHREAD(cep->threadID), TRUE);
        END_TEMPORARY_ROOTS
    }
}

/*========================================================================
 * Function:        setEvent_ExceptionCatch()
 * Overview:        
 *                  
 * Interface:
 *  parameters:     
 *  returns:        
 *
 * Notes:
 *=======================================================================*/

void setEvent_ExceptionCatch()
{
    checkNOTIFY_WANTED(Dbg_EventKind_EXCEPTION_CATCH);
}

/*========================================================================
 * Function:        getVerifierBreakpointOpcode()
 * Overview:        called by verifier loop to retrieve the underlying
 *          opcode that was breakpointed.
 * Interface:
 *   parameters:    none
 *   returns:       byte opcode
 *
 * Notes:
 *=======================================================================*/

ByteCode getVerifierBreakpointOpcode(METHOD thisMethod, unsigned short insp)
{
    INSTANCE_CLASS cl;
    ByteCode opcode;

    cl = thisMethod->ofClass;   /* point to class of method */
    if (findBreakEventAndOpcode(&opcode, NIL, GET_CLASS_DEBUGGERID(&cl->clazz),
            getMethodIndex(thisMethod), insp) == NIL) {
        raiseException(VirtualMachineError);    /* XXX */
        return 0;
    }
    /*opcode = mod->u.loc.opcode;*/
    return opcode;
}

/*========================================================================
 * Function:        handleSingleStep()
 * Overview:        determines when a stepping event has occured
 *                  stepping can be in, over, or out.
 *                  
 * Interface:
 *  parameters:     
 *  returns:        
 *
 * Notes:
 *    How single stepping works
    Assume you are sitting at a breakpoint in some code and you
    decide to single step.  The debugger issues an event request for
    a single step event.  KVM gets this request in createEvent() and
    sends a vendor specific command to the debug agent to obtain 
    information as to where the next line may be. The proxy returns
    with three pieces of info (see KVMCommands_GetSteppingInfo()).
     1) the offset of the next line that might be executed.  Note
        that the debug agent doesn't really know if the next line will
        be executed as there could be a goto type of instruction in the
        bytecode.
     2) The offset of code that has the same line number as the above
        offset (or -1 if no duplicate).  You would only get a duplicate
        if there is a for loop or while loop (as far as I know).
     3) If there is a duplicate offset, then this offset is the offset of
        the line after the above duplicate offset.  Here's an example.

    Line #
     1            public void run() {
     2                for (int j = 0; j < 2; j++) {
     3                    int k = j;
     4                }
     5           }

    Code for Method void run()
    Offset
     0     iconst_0
     1     istore_1
     2     goto 10
     5     iload_1
     6     istore_2
     7     iinc 1 1
    10     iload_1
    11     iconst_2
    12     if_icmplt 5
    15     return

    Line numbers for method void run()

       line #: code offset

       line 2: 0
       line 3: 5
       line 2: 7
       line 5: 15

    You set a breakpoint at line 2 and run the program and hit the
    breakpoint.  Now you single step.  The debugger requests a single
    step event.  KVM calls back to the debug agent requesting stepping
    info.  The proxy responds with this info:
        target offset  5
            duplicate offset  7
            offset of line after duplicate  15
    As you can see from the above line number table, line 2 corresponds
    to two offsets.  Offset 0 is the start of the for loop and offset 7
    is the increment and test part of the loop.
      All this info is stored in the stepinfo structure which is part of
    the thread structure and we return back to the interpreter loop.  At 
    the top of the loop we call back to debugger.c to the 
    handleSingleStep() function (below).  We obtain the current frame
    pointer and the current offset into the method code. We then
    start testing to see if we need to send a single step event to
    the debugger.  If the frame is the same as the one when we started this
    single step process and we are not doing a step OUT type of single
    step, we check to see if the current offset has reached the 
    target offset.  If so then send the event.  If not then we check to
    see if the current offset is less than the starting offset.  If so
    then we must have jumped back somehow so we send an event.  Next
    we test if the current offset is > the target offset.  If so we
    check to see if the dupCurrentLineOffs or postDupLineOffs is -1. If
    so then there is no duplicate line info and this isn't a for or 
    while loop so we just send the event.  If there is dup line info
    then we check to see if the current offset is >= the dupCurrentLineOffs.
    If it is then we may be at the end of the loop or we may have gone
    beyond the end of the loop to the next line.  We check to see if
    the current offset >= postDupLineOffs and if so then we've gone out
    of the loop so send an event.  Otherwise we haven't left the loop and
    we just continue interpreting and repeat the whole process.
    Eventually we reach the offset 5 (which was our target offset) and send
    the single step event.
    If we single step again we won't have any duplicate line info 
    (since line 3 has only one entry in the table) so we simply send
    an event when we reach offset 7.  Now we are stopped at the end of the
    loop although the debugger is correctly placing the cursor at line
    2 since offset 7 corresponds to line 2.  When we single step now, KVM
    will get the stepping info from the debug agent.  Since this is
    a duplicate line (line 2 is at offset 0 and offset 7) we will get
    duplicate offset information back.  If you look at the code in
    kdp/classparser/attributes/LineNumberTableAttribute.java for the
    method getNextExecutableLineCodeIndex() you notice that if there is
    a duplicate entry we check to see if the duplicate offset (in this
    case offset 0 since we are at offset 7) is before or after the 
    current offset.  We always return the lesser of the two, in this 
    case we return offset 5 as the target since it's the next line after
    the line that corresponds to offset 0. So the next time we step we
    will step back to line 3 which is what we expect.  However, if the
    loop test terminates the loop then we will not go to line 3 offset 5
    but rather to offset 15 line 5.  This will be caught by the test that
    looks for the current offset >= postDupLineOffs and we will send the
    event with the offset of 15.

    If the frame has changed then we do testing to determine if we've 
    stepped into a new function or popped back up to the caller.  Depending
    on the type of step (step OVER or step INTO or step OUT) we 
    determine whether or not we need to send an event.

 *=======================================================================*/

int handleSingleStep(THREAD thread, THREAD *tp)
{
    unsigned long offset;
    FRAME frame, tframe;
    unsigned long classID;
    unsigned long methodIndex;
    bool_t sendEvent = FALSE;
    
    /* if the proxy has returned -1 it means that we have reached
     * the end of a function and we must look up stack for the
     * calling function
     *
     */
    frame = fp_global;
    offset = ip_global - frame->thisMethod->u.java.code;
    if (thread->stepInfo.target.offset == -1) {
        /*
         * Wait for the frame to change and that will mean we popped up to
         * the calling frame. (Or we called down to another frame, maybe...
         * should look into that possibility)
         */
        if (frame != thread->stepInfo.fp) { 
            sendEvent = TRUE;
            /* the frame has changed but we don't know whether we
             * have gone into a function or gone up one.
             * if we look up the call stack and find the frame 
             * that we originally set the request then we stepped
             * into a function, otherwise we stepped out 
             */
            if (thread->stepInfo.depth == JDWP_StepDepth_OVER ||
                 thread->stepInfo.depth == JDWP_StepDepth_OUT) {
                for (tframe = frame->previousFp; tframe != NIL;
                    tframe = tframe->previousFp) {
                    if (tframe == thread->stepInfo.fp) {
                         sendEvent = FALSE;
                         break;
                    }
                }
            }
        }
    } else {
        struct location_mod *target = &thread->stepInfo.target;

        switch (thread->stepInfo.size) {

        case JDWP_StepSize_LINE:
            /* stepping by line can have two effects: we stay in the same 
             * frame or the frame changes
             */
            if (thread->stepInfo.fp == frame) {
                /* if the frame is the same and we are not running to the
                 * end of a function we need to see if we have reached
                 * the location we are looking for
                 */
                if (thread->stepInfo.depth != JDWP_StepDepth_OUT) {
                    if (offset == target->offset) {
                        /* we reached the exact offset we were looking for */
                        sendEvent = TRUE;
                        break;
                    } else if (offset < thread->stepInfo.startingOffset) {
                        /* we ended up before the offset were the stepping
                         * request was initiated. we jumped back
                         */
                        sendEvent = TRUE;
                        break;
                    } else if (offset > target->offset &&
                        offset > thread->stepInfo.startingOffset) {
                        if (target->dupCurrentLineOffs == -1 ||
                            target->postDupLineOffs == -1) {
                            /* We stepped beyond the target and there's no
                             * duplicate offset information
                             * (like there would be in a for or while loop).
                             * Most likely a break in a switch case.
                             * However, we checked to see if our starting offset
                             * is equal to our current, if so don't issue
                             * an event otherwise we'll be in an infinite loop.
                             */
                            sendEvent = TRUE;
                            break;
                        } else if (offset >= target->dupCurrentLineOffs) {
                            /*
                             * We are after the location we were looking for;
                             * we jumped forward.  However, we may be at the end
                             * of a for/while loop where we do the loop
                             * test.  We check to see if we are in the line after
                             * the for/do end line.  If not, we just continue.
                             */
                            if (offset >= target->postDupLineOffs) {
                                /* we were beyond the end of the loop */
                                sendEvent = TRUE;
                                break;
                            }
                        }
                    }
                }
            } else {
                /* the frame has changed but we don't know whether we
                 * have gone into a function or gone up one.
                 * if we look up the call stack and find the frame 
                 * that originally set the request then we stepped
                 * into a function, otherwise we stepped out 
                 */
                sendEvent = TRUE;
                if (thread->stepInfo.depth == JDWP_StepDepth_OVER ||
                     thread->stepInfo.depth == JDWP_StepDepth_OUT) {
                    for (tframe = frame->previousFp; tframe != NIL;
                        tframe = tframe->previousFp) {
                        if (tframe == thread->stepInfo.fp) {
                             sendEvent = FALSE;
                             break;
                        }
                    }
                }
            }

            break;

        /* stepping by bytecode */
        case JDWP_StepSize_MIN:
            
            if (thread->stepInfo.depth == JDWP_StepDepth_OVER) {
                if (frame == thread->stepInfo.fp)
                    sendEvent = TRUE;
                else {
                    sendEvent = TRUE;
                    /* If we are deeper in the call stack do not break... */
                    for (tframe = frame->previousFp; 
                         tframe != NIL;
                         tframe = tframe->previousFp) {
                        if (tframe == thread->stepInfo.fp) {
                            sendEvent = FALSE;
                            break;
                        }
                    }
                }
            } else if (thread->stepInfo.depth == JDWP_StepDepth_OUT) {
                if (frame != thread->stepInfo.fp) {
                    sendEvent = TRUE;
                    /* If we are deeper in the call stack do not break... */
                    for (tframe = frame->previousFp; 
                         tframe != NIL;
                         tframe = tframe->previousFp) {
                        if (tframe == thread->stepInfo.fp) {
                            sendEvent = FALSE;
                            break;
                        }
                    }
                }
            } else
                sendEvent = TRUE;
            break;
        }
    }

    if (sendEvent) {
        CEModPtr cep;
        START_TEMPORARY_ROOTS
            IS_TEMPORARY_ROOT(thread, thread);
            classID = GET_CLASS_DEBUGGERID(&frame->thisMethod->ofClass->clazz);
            methodIndex = getMethodIndex(frame->thisMethod);
            cep = GetCEModifier();
            cep->step.threadID = getObjectID((OBJECT)thread->javaThread);
            cep->loc.classID = classID;
            cep->loc.methodIndex = methodIndex;
            cep->loc.offset = offset;
            cep->step.depth = thread->stepInfo.depth;
            cep->step.size = thread->stepInfo.size;

            if (thread->needEvent)
                checkDebugEvent(thread);
            /* if the above event is sent, CurrentThread is null so
             * setEvent_SingleStep needs a thread argument
             */
            setEvent_SingleStep(cep, thread);
            FreeCEModifier(cep);
            *tp = thread;
        END_TEMPORARY_ROOTS
        if (CurrentThread == NIL) {
            signalTimeToReschedule();
            /* make sure that we don't continue executing
             * code in the CurrentThread if we have 
             * suspended it
             */
        return 1;
        }
    } else
        *tp = thread;
    return 0;
}

void handleBreakpoint(THREAD threadArg)
{
    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(THREAD, thread, threadArg);

        METHOD thisMethod = fp_global->thisMethod;
        INSTANCE_CLASS clazz = thisMethod->ofClass;
        VMEventPtr ep;
        unsigned long offset;
        ByteCode opcode;
        CEModPtr cep;

        offset = ip_global - thisMethod->u.java.code;   /* where are we? */
    
        /* find this breakpoint */
        ep = findBreakEventAndOpcode(&opcode, NIL,
             GET_CLASS_DEBUGGERID(&clazz->clazz),
             getMethodIndex(thisMethod), offset);
    
        if (ep == NIL) {
            raiseException(VirtualMachineError); /* XXX */
            goto breakout;
        }
        
        thread->nextOpcode = opcode;
        thread->isAtBreakpoint = TRUE;

        cep = GetCEModifier();
        cep->loc.classID = GET_CLASS_DEBUGGERID(&clazz->clazz);
        cep->threadID = getObjectID((OBJECT)thread->javaThread);
        cep->loc.methodIndex = getMethodIndex(thisMethod);
        cep->loc.offset = offset;

        setEvent_Breakpoint(ep, cep);
        FreeCEModifier(cep);
        /*
         * Make sure that we don't continue executing
         * code in the CurrentThread if we have suspended it
         */
        signalTimeToReschedule();
breakout:
    END_TEMPORARY_ROOTS
}

/*========================================================================
 * Function:        VirtualMachine_AllClasses()
 * Overview:        send all loaded class id's to the debugger
 * Interface:
 *   parameters:    pointer to debug header
 *   returns:       none
 *                  
 *
 * Notes:
 *=======================================================================*/

static void
VirtualMachine_AllClasses(PACKET_INPUT_STREAM_HANDLE inH, 
                           PACKET_OUTPUT_STREAM_HANDLE outH)
{
    long count = 0;

    TRACE_DEBUGGER(("VM", "All Classes", NULL));
    FOR_ALL_CLASSES(clazz)
        if (IS_ARRAY_CLASS(clazz) ||
               ((INSTANCE_CLASS)clazz)->status >= CLASS_LOADED) {
            count++;
        }
    END_FOR_ALL_CLASSES

    outStream_writeLong(outH, count);
#if INCLUDEDEBUGCODE            
    if (tracedebugger) { 
        fprintf(stdout, "    %ld classes\n", count);
    }
#endif
    
    FOR_ALL_CLASSES(clazz)
        if (IS_ARRAY_CLASS(clazz) ||
              ((INSTANCE_CLASS)clazz)->status >= CLASS_LOADED) { 
            outStream_writeByte(outH, getJDWPTagType(clazz));
            outStream_writeClass(outH, clazz);
            outStream_writeClassName(outH, clazz);
            if (IS_ARRAY_CLASS(clazz)) { 
                outStream_writeLong(outH, (JDWP_ClassStatus_INITIALIZED |
                    JDWP_ClassStatus_VERIFIED | JDWP_ClassStatus_PREPARED));
            } else { 
                outStream_writeLong(outH, 
                                 getJDWPClassStatus((INSTANCE_CLASS)clazz));
            }
        }
    END_FOR_ALL_CLASSES
}

/*========================================================================
 * Function:        VirtualMachine_AllThreads()
 * Overview:        send all running thread id's to the debugger
 * Interface:
 *   parameters:    pointer to debug header
 *   returns:       none
 *                  
 *
 * Notes:
 *=======================================================================*/

static void 
VirtualMachine_AllThreads(PACKET_INPUT_STREAM_HANDLE inH,
                          PACKET_OUTPUT_STREAM_HANDLE outH)
{
    long count=0;

    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(THREAD, tptr, AllThreads);

        for (tptr=AllThreads; tptr; tptr = tptr->nextAliveThread) {
            if (!(tptr->state & THREAD_DEAD) &&
                !(tptr->state & THREAD_JUST_BORN)) {
                count++;
            }
        }
        TRACE_DEBUGGER(("VM", "All Threads", NULL));
        outStream_writeLong(outH, count);
        tptr = AllThreads;
        for (; tptr; tptr = tptr->nextAliveThread) {
            if (!(tptr->state & THREAD_DEAD) &&
                !(tptr->state & THREAD_JUST_BORN)) {
                outStream_writeObject(outH, (OBJECT)tptr->javaThread);
            }
        }
    END_TEMPORARY_ROOTS
}

/*========================================================================
 * Function:        VirtualMachine_Suspend()
 * Overview:        called from interpreter loop to suspend VM
 * Interface:
 *   parameters:    none
 *   returns:       none
 *
 * Notes:
 *=======================================================================*/

static void 
VirtualMachine_Suspend(PACKET_INPUT_STREAM_HANDLE inH, 
                       PACKET_OUTPUT_STREAM_HANDLE outH)
{
    TRACE_DEBUGGER(("VM", "Suspend All", NULL));
    suspendAllThreads();
}

/*========================================================================
 * Function:        VirtualMachine_Resume()
 * Overview:        resume VM
 * Interface:
 *   parameters:    none
 *   returns:       none
 *
 * Notes:
 *=======================================================================*/

static void 
VirtualMachine_Resume(PACKET_INPUT_STREAM_HANDLE inH, 
                      PACKET_OUTPUT_STREAM_HANDLE outH)
{
    START_TEMPORARY_ROOTS
       DECLARE_TEMPORARY_ROOT(THREAD, tptr, AllThreads);
       TRACE_DEBUGGER(("VM", "Resume All", NULL));

       CurrentThread = NIL;
       for (; tptr != NIL; tptr = tptr->nextAliveThread) {
           resumeSpecificThread(tptr);
       }
       allThreadsSuspended = FALSE;
       setResumeOK(TRUE);
   END_TEMPORARY_ROOTS
}

/*========================================================================
 * Function:        VirtualMachine_Exit()
 * Overview:        causes the vm to terminate
 *                  
 * Interface:
 *  parameters:     input pointer, output pointer, isBreak bool
 *  returns:        
 *
 * Notes:
 *=======================================================================*/

static void 
VirtualMachine_Exit(PACKET_INPUT_STREAM_HANDLE inH, 
                    PACKET_OUTPUT_STREAM_HANDLE outH)
{
    long errorCode;
    errorCode = inStream_readLong(inH);
    
    TRACE_DEBUGGER(("VM", "Exit", "Code %d", errorCode));

    setNotification(Dbg_EventKind_NONE);
    
    /* Stop the VM */
    VM_EXIT(errorCode);
}

static void 
writeValueFromAddress(PACKET_OUTPUT_STREAM_HANDLE outH, 
                      void *address, char typeTag, bool_t writeTag) { 
    long value, value2;

    /* We must grab these values before we do the first write, since that
     * could cause a GC 
     */
    value = ((long *)address)[0];

    /* Stuff that's got to be done before we possibly GC */
    switch(typeTag) { 
        case JDWP_Tag_DOUBLE:
        case JDWP_Tag_LONG:
            value2 = ((long *)address)[1];
            break;
        case JDWP_Tag_OBJECT:
        case JDWP_Tag_ARRAY:
            if (value == 0) {
                typeTag = JDWP_Tag_OBJECT;
            } else if (!isValidObject((OBJECT)value)) { 
                value = 0;
                typeTag = JDWP_Tag_OBJECT;
            } else {
                OBJECT object = (OBJECT)value;
                CLASS clazz = object->ofClass;
                if (IS_ARRAY_CLASS(clazz)) { 
                    typeTag = JDWP_Tag_ARRAY;
                } else if (clazz == (CLASS)JavaLangString) { 
                    typeTag = JDWP_Tag_STRING;
                } else if (clazz == (CLASS)JavaLangThread) { 
                    typeTag = JDWP_Tag_THREAD;
                } else if (clazz == (CLASS)JavaLangClass) { 
                    typeTag = JDWP_Tag_CLASS_OBJECT;
                } else if (isValidJavaThread(object)) {
                    typeTag = JDWP_Tag_THREAD;
                }
                /* This may GC, but we don't care, since we'll no longer need
                 * the object */
                value = getObjectID(object);
            }
            break;
    }

    if (writeTag) { 
        outStream_writeByte(outH, typeTag);
    }

    switch(typeTag) { 
        case JDWP_Tag_DOUBLE: 
        case JDWP_Tag_LONG:
#if BIG_ENDIAN
            outStream_writeLong(outH, value);
            outStream_writeLong(outH, value2);
#else
            outStream_writeLong(outH, value2);
            outStream_writeLong(outH, value);
#endif
            break;
            
        case JDWP_Tag_BYTE:
            outStream_writeByte(outH, (unsigned char)(value & 0xFF));
            break;

        case JDWP_Tag_CHAR:
            outStream_writeChar(outH, (short)(value & 0xFFFF));
            break;

        case JDWP_Tag_INT:
        case JDWP_Tag_FLOAT:
            outStream_writeLong(outH, value);
            break;
 
        case JDWP_Tag_SHORT:
            outStream_writeShort(outH, (short)(value & 0xFFFF));
            break;

        case JDWP_Tag_BOOLEAN:
            outStream_writeBoolean(outH,  value & 0xFF );
            break;

        case JDWP_Tag_VOID:  /* happens with function return values */   
            /* write nothing */
            break;

        default:
            outStream_writeObjectID(outH, value);
            break;
    }
}

static void 
readValueToAddress(PACKET_INPUT_STREAM_HANDLE inH, void *address, char typeTag){
    switch(typeTag) { 
        case JDWP_Tag_BYTE:
        case JDWP_Tag_BOOLEAN:
            *(cell *)address = inStream_readByte(inH);
            break;

        case JDWP_Tag_CHAR:
        case JDWP_Tag_SHORT:
            *(cell *)address = inStream_readShort(inH);
            break;

        case JDWP_Tag_LONG:
        case JDWP_Tag_DOUBLE: { 
            long high = inStream_readLong(inH);
            long low  = inStream_readLong(inH);
            SET_LONG_FROM_HALVES(address, high, low);
            break;
        }
            
        case JDWP_Tag_INT:
        case JDWP_Tag_FLOAT:
            *((cell *)address) = inStream_readLong(inH);
            break;

        case JDWP_Tag_VOID: 
            /* do nothing */
            break;

        default:
            *(OBJECT *)address = inStream_readObject(inH);
            break;
    }
}

/*========================================================================
 * Function:        ReferenceType_GetValues()
 * Overview:        get the value of one or more static variables
 * Interface:
 *   parameters:    pointer to debug header
 *   returns:
 *
 * Notes:       Packet format:
 *              Input:
 *                  class ID    Long
 *                  number of slots     Long
 *    
 *              Output:
 *                  debug header -      11 bytes
 *                  count - Long
 *                  taggedvalues - byte + Long
 *=======================================================================*/
 
static void 
StaticField_GetterSetter(PACKET_INPUT_STREAM_HANDLE inH, 
                         PACKET_OUTPUT_STREAM_HANDLE outH,
                         bool_t isSetter)
{
    INSTANCE_CLASS clazz = (INSTANCE_CLASS)inStream_readClass(inH); 
    long num = inStream_readLong(inH);
    int i;

    TRACE_DEBUGGER(("Static", (isSetter ? "Set" : "Get"), 
                    "class=0x%lx, count=%ld", clazz, num));

    /* Prevent compiler warnings */
    clazz = clazz;

    if (!isSetter) { 
        outStream_writeLong(outH, num);
    }

    for (i = 0; i < num; i++) {
        INSTANCE_CLASS fieldClazz = (INSTANCE_CLASS)inStream_readClass(inH);
        long fieldID = inStream_readLong(inH);
        FIELD field;
        char typeTag;

        if (fieldClazz == NULL || fieldClazz->fieldTable == NULL) {
            outStream_setError(outH, JDWP_Error_INVALID_FIELDID);
            goto breakout;
        } 
        if (fieldID >= fieldClazz->fieldTable->length) {
            outStream_setError(outH, JDWP_Error_INVALID_FIELDID);
            goto breakout;
        } 
        field = &fieldClazz->fieldTable->fields[fieldID];
        if ((field->accessFlags & ACC_STATIC) == 0) { 
            outStream_setError(outH, JDWP_Error_INVALID_FIELDID);
            goto breakout;
        }
        
        typeTag = (field->nameTypeKey.nt.typeKey < 256 ) 
                ? field->nameTypeKey.nt.typeKey 
                : JDWP_Tag_OBJECT;

        if (isSetter) { 
            readValueToAddress(inH, field->u.staticAddress, typeTag);
        } else { 
            writeValueFromAddress(outH, field->u.staticAddress, typeTag, TRUE);
        }
#if INCLUDEDEBUGCODE
        if (tracedebugger) { 
            fprintf(stdout, "    %s.%s: ", 
                    getClassName((CLASS)(field->ofClass)), fieldName(field));
            printValueFromTag(field->u.staticAddress, typeTag);
        }
#endif
    }
breakout:
    ;
}

static void
ReferenceType_GetValues(PACKET_INPUT_STREAM_HANDLE inH, 
                        PACKET_OUTPUT_STREAM_HANDLE outH) 
{ 
    StaticField_GetterSetter(inH, outH, FALSE);
}

/*========================================================================
 * Function:        ClassType_SuperClass()
 * Overview:        returns the id of the super class
 *                  
 * Interface:
 *  parameters:     input pointer, output pointer, isBreak bool
 *  returns:        void
 *
 * Notes:           returns null if class is Object
 *=======================================================================*/

static void 
ClassType_SuperClass(PACKET_INPUT_STREAM_HANDLE inH, 
                     PACKET_OUTPUT_STREAM_HANDLE outH)
{
    INSTANCE_CLASS clazz = (INSTANCE_CLASS)inStream_readClass(inH);

    TRACE_DEBUGGER(("Class", "Superclass", "%s", getClassName((CLASS)clazz)));
    if (clazz == NULL || clazz->superClass == NULL ||
           ((clazz->clazz.accessFlags & ACC_INTERFACE) != 0)) { 
        outStream_writeLong(outH, 0);
    } else {  
        outStream_writeClass(outH, (CLASS)(clazz->superClass));
    }
}

/*========================================================================
 * Function:        ClassType_SetValues()
 * Overview:        set the value of one of an static variable
 * Interface:
 *   parameters:    pointer to debug header
 *   returns:
 *
 * Notes:       Packet format:
 *              Input:
 *                  object ID   Long
 *                  number of slots     Long
 *                  values  Long
 *
 *              Output:
 *                  debug header -      11 bytes
 *=======================================================================*/

static void ClassType_SetValues(PACKET_INPUT_STREAM_HANDLE inH, 
                                PACKET_OUTPUT_STREAM_HANDLE outH)
{
    /* I have no idea why the command to get statics is in one category, and
     * the command to set them is in another.
     * I'll combine them anyway.
     */
    StaticField_GetterSetter(inH, outH, TRUE);
}

/*========================================================================
 * Function:        ObjectRefernce_ReferenceType
 * Overview:        returns the type tag of the object and what
 *                  class it is represented by
 *                  
 * Interface:
 *  parameters:     input pointer, output pointer, isBreak bool
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

static void 
ObjectReference_ReferenceType(PACKET_INPUT_STREAM_HANDLE inH, 
                              PACKET_OUTPUT_STREAM_HANDLE outH)
{
    OBJECT object = inStream_readObject(inH);

    TRACE_DEBUGGER(("Object", "Type", "ObjectID=%ld, Object=0x%lx", 
                    getObjectID(object),object));
    
    /* Note, we don't need to GC protect object, since we get its class 
     * before we write it out */

    if (object == (OBJECT)ONLY_THREADGROUP_ID)    {
        outStream_writeByte(outH, JDWP_TypeTag_CLASS);
        outStream_writeClass(outH, (CLASS)JavaLangThread);
    } else if (isValidObject(object)) {
        CLASS clazz = object->ofClass;
        outStream_writeByte(outH, getJDWPTagType(clazz));
        outStream_writeClass(outH, clazz);
#if INCLUDEDEBUGCODE
        if (tracedebugger) { 
            fprintf(stdout, "    Type is %s\n", getClassName(clazz));
        }
#endif
    } else {
        outStream_setError(outH, JDWP_Error_INVALID_OBJECT);
    }
}

/*========================================================================
 * Function:        ObjectReference_GetValues
 * Overview:        get the value of one of an instance variable
 * Interface:
 *   parameters:    pointer to debug header
 *   returns:
 *
 * Notes:       Packet format:
 *              Input:
 *                  object ID   long
 *                  number of slots     long
 *                  
 *              Output:
 *                  debug header -      11 bytes
 *                  count - long
 *                  values - long
 *=======================================================================*/
 
static void 
ObjectField_GetterSetter(PACKET_INPUT_STREAM_HANDLE inH, 
                         PACKET_OUTPUT_STREAM_HANDLE outH, 
                         bool_t isSetter)
{
    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(INSTANCE, instance, 
                               (INSTANCE)inStream_readObject(inH));
        long num = inStream_readLong(inH);
        int i;
        if (instance == NULL) {
            outStream_writeLong(outH, 0);
            goto breakout;
        }

        TRACE_DEBUGGER(("Object", (isSetter ? "Set" : "Get"), 
                        "objectID=%ld, object=0x%lx, count=%ld",
                        getObjectID((OBJECT)instance), instance, num));

        if (!isSetter) { 
            outStream_writeLong(outH, num);
        }

        for (i = 0; i < num; i++) {
            INSTANCE_CLASS fieldClazz = (INSTANCE_CLASS)inStream_readClass(inH);
            long fieldID = inStream_readLong(inH);
            
            if (fieldClazz == NULL || fieldClazz->fieldTable == NULL) {
                outStream_setError(outH, JDWP_Error_INVALID_FIELDID);
                goto breakout;
            } else if (fieldID >= fieldClazz->fieldTable->length) {
                outStream_setError(outH, JDWP_Error_INVALID_FIELDID);
                goto breakout;
            } else { 
                FIELD field = &fieldClazz->fieldTable->fields[fieldID];
                /* Note, address is volatile for non-statics */
                void *address = (field->accessFlags & ACC_STATIC) 
                              ? field->u.staticAddress 
                              : &instance->data[field->u.offset];
                char typeTag = (field->nameTypeKey.nt.typeKey < 256) 
                             ? field->nameTypeKey.nt.typeKey 
                             : JDWP_Tag_OBJECT;
                if (isSetter) { 
                    readValueToAddress(inH, address, typeTag);
                } else { 
                    writeValueFromAddress(outH, address, typeTag, TRUE);
                }
#if INCLUDEDEBUGCODE
                if (tracedebugger) { 
                    fprintf(stdout, "    %s.%s: ", 
                            getClassName((CLASS)(field->ofClass)), 
                            fieldName(field));
                    if ((field->accessFlags & ACC_STATIC) == 0) {
                        /* Need to refresh, since volatile */
                        address = &instance->data[field->u.offset];
                    }
                    printValueFromTag(address, typeTag);
                }
#endif
            }
        }
breakout:
    END_TEMPORARY_ROOTS
}

static void 
ObjectReference_GetValues(PACKET_INPUT_STREAM_HANDLE inH, 
                          PACKET_OUTPUT_STREAM_HANDLE outH)
{
    ObjectField_GetterSetter(inH, outH, FALSE);
}

static void 
ObjectReference_SetValues(PACKET_INPUT_STREAM_HANDLE inH, 
                          PACKET_OUTPUT_STREAM_HANDLE outH)
{
    ObjectField_GetterSetter(inH, outH, TRUE);
}

static void
ObjectReference_IsCollected(PACKET_INPUT_STREAM_HANDLE inH,
                            PACKET_OUTPUT_STREAM_HANDLE outH)
{
    OBJECT object = inStream_readObject(inH);
    if (object != NULL) {
        cell object_header = ((cell *)(object))[-HEADERSIZE];
        if (!ISFREECHUNK(object_header)) {
            outStream_writeBoolean(outH, FALSE); /* object still here */
            return;
        }
    }
    outStream_writeBoolean(outH, TRUE);  /* object is no longer here */
}

static void 
StringReference_Value(PACKET_INPUT_STREAM_HANDLE inH, 
                      PACKET_OUTPUT_STREAM_HANDLE outH)
{
    START_TEMPORARY_ROOTS 
        DECLARE_TEMPORARY_ROOT(STRING_INSTANCE, string, 
                               (STRING_INSTANCE)inStream_readObject(inH));
        int buflen = unicode2utfstrlen((unsigned short *)&((SHORTARRAY)string->array)->sdata[string->offset],
             string->length) + 1;
        DECLARE_TEMPORARY_ROOT(char *, buf, mallocBytes(buflen));

        unicode2utf((unsigned short *)&((SHORTARRAY)string->array)->sdata[string->offset], string->length,  buf,
            buflen);
        outStream_writeString(outH, (CONST_CHAR_HANDLE)&buf);

        TRACE_DEBUGGER(("String", "Value", 
                        "stringID=%ld, string=0x%lx =\"%s\"", 
                        getObjectID((OBJECT)string), string, buf));
    END_TEMPORARY_ROOTS
}

/*========================================================================
 * Function:        ThreadReference_Name()
 * Overview:        send the specified thread's name
 * Interface:
 *   parameters:    pointer to debug header
 *   returns:       none
 *                  
 *
 * Notes:
 *=======================================================================*/

static void 
ThreadReference_Name(PACKET_INPUT_STREAM_HANDLE inH, 
                     PACKET_OUTPUT_STREAM_HANDLE outH)
{
    START_TEMPORARY_ROOTS
        long threadID = inStream_readLong(inH);
        DECLARE_TEMPORARY_ROOT(THREAD, thread, GET_VMTHREAD(threadID));

        TRACE_DEBUGGER(("Thread", "Name", "threadID=%ld", threadID));

        if (isValidThread(thread)) {
            if (thread == MainThread) { 
                char *result = "KVM_main";
                outStream_writeString(outH, (CONST_CHAR_HANDLE)&result);
            } else { 
                DECLARE_TEMPORARY_ROOT(char *, name, 
                                       mallocBytes(10 + sizeof(long)*2 + 1));
                sprintf(name, "KVM_Thread%lx", (long)thread);
                outStream_writeString(outH, (CONST_CHAR_HANDLE)&name);
            } 
        } else { 
            char *nullString = "";
            outStream_writeString(outH, (CONST_CHAR_HANDLE)&nullString);
        }
    END_TEMPORARY_ROOTS
}

/*========================================================================
 * Function:        ThreadReference_Suspend()
 * Overview:        suspends a thread
 *                  
 * Interface:
 *  parameters:     input pointer, output pointer, isBreak bool
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

static void 
ThreadReference_Suspend(PACKET_INPUT_STREAM_HANDLE inH, 
                        PACKET_OUTPUT_STREAM_HANDLE outH)
{
    START_TEMPORARY_ROOTS
        long threadID = inStream_readLong(inH);
        DECLARE_TEMPORARY_ROOT(THREAD, thread, GET_VMTHREAD(threadID));

        TRACE_DEBUGGER(("Thread", "Suspend", "ThreadID=%lx", threadID));

        if (isValidThread(thread)) {
            suspendSpecificThread(thread);
        } else {
            /* outStream_setError(out, JDWP_Error_INVALID_THREAD); */
        }
    END_TEMPORARY_ROOTS
}

/*========================================================================
 * Function:        ThreadReference_Resume
 * Overview:        resumes a thread
 *                  
 * Interface:
 *  parameters:     input pointer, output pointer, isBreak bool
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

static void 
ThreadReference_Resume(PACKET_INPUT_STREAM_HANDLE inH, 
                       PACKET_OUTPUT_STREAM_HANDLE outH)
{
    START_TEMPORARY_ROOTS
        long threadID = inStream_readLong(inH);
        DECLARE_TEMPORARY_ROOT(THREAD, thread, GET_VMTHREAD(threadID));

        TRACE_DEBUGGER(("Thread", "Resume", "ThreadID=%lx", threadID));

        if (isValidThread(thread)) {
            resumeSpecificThread(thread);
        } else {
            outStream_setError(outH, JDWP_Error_INVALID_THREAD);
        }
        allThreadsSuspended = FALSE;
        setResumeOK(TRUE);
    END_TEMPORARY_ROOTS
}

/*========================================================================
 * Function:        ThreadReference_Status()
 * Overview:        returns the JDWP thread status
 *                  
 * Interface:
 *  parameters:     input pointer, output pointer, isBreak bool
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

static void 
ThreadReference_Status(PACKET_INPUT_STREAM_HANDLE inH, 
                       PACKET_OUTPUT_STREAM_HANDLE outH)
{
    START_TEMPORARY_ROOTS
        long threadID = inStream_readLong(inH);
        DECLARE_TEMPORARY_ROOT(THREAD, thread, GET_VMTHREAD(threadID));

        TRACE_DEBUGGER(("Thread", "Status", "ThreadID=%lx", threadID));

        if (isValidThread(thread)) {
            outStream_writeLong(outH, getJDWPThreadStatus(thread));
            outStream_writeLong(outH, getJDWPThreadSuspendStatus(thread));
        } else {
            /* outStream_setError(out, JDWP_Error_INVALID_THREAD); */
            outStream_writeLong(outH, JDWP_ThreadStatus_ZOMBIE);
            outStream_writeLong(outH, !JDWP_SuspendStatus_SUSPEND_STATUS_SUSPENDED);
        }
    END_TEMPORARY_ROOTS
}

/*========================================================================
 * Function:        ThreadReference_SuspendCount()
 * Overview:        returns the number of times this thread 
 *                  has been suspended
 *                  
 * Interface:
 *  parameters:     input pointer, output pointer, isBreak bool
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

static void 
ThreadReference_SuspendCount(PACKET_INPUT_STREAM_HANDLE inH, 
                             PACKET_OUTPUT_STREAM_HANDLE outH)
{
    START_TEMPORARY_ROOTS
        long threadID = inStream_readLong(inH);
        DECLARE_TEMPORARY_ROOT(THREAD, thread, GET_VMTHREAD(threadID));

        TRACE_DEBUGGER(("Thread", "Suspend Count", "threadID=%ld", threadID));

        if (isValidThread(thread)) {
            outStream_writeLong(outH, thread->debugSuspendCount);
        } else {
            outStream_setError(outH, JDWP_Error_INVALID_THREAD);
        }
    END_TEMPORARY_ROOTS
}

/*========================================================================
 * Function:        ThreadReference_Frames()
 * Overview:        dump all frames or a particular frame
 * Interface:
 *   parameters:    debug header pointer
 *   returns:       nothing
 *
 * Notes:       Packet format:
 *              Input:
 *                  thread ID   long
 *                  startIndex  long
 *                  len         long
 *              Output:
 *                  debug header    11 bytes
 *                  frame count-    long
 *                  For each frame:
 *                      current fp long
 *                      location    17 bytes
 *=======================================================================*/

static void 
ThreadReference_Frames(PACKET_INPUT_STREAM_HANDLE inH, 
                       PACKET_OUTPUT_STREAM_HANDLE outH)
{
    START_TEMPORARY_ROOTS
        long threadID = inStream_readLong(inH);
        long startIndex = inStream_readLong(inH);
        long len = inStream_readLong(inH);

        DECLARE_TEMPORARY_ROOT(THREAD, thread, GET_VMTHREAD(threadID));
        DECLARE_TEMPORARY_ROOT(STACK, stack, NULL);

        FRAME currentFP;
        BYTE *currentIP;
        long index, frameCount;

        TRACE_DEBUGGER(("Thread", "Frames", 
                        "threadID=%ld, start=%ld, length=%ld", 
                        threadID, startIndex, len));
        
        if (!isValidThread(thread)) {
            outStream_setError(outH, JDWP_Error_INVALID_THREAD);
            goto breakout;
        }

        if (CurrentThread) { 
            storeExecutionEnvironment(CurrentThread);
        }

        currentFP = thread->fpStore;
        frameCount = 0;
        /* first get the count of frames on the stack */
        while (currentFP != NIL) {
            frameCount++;
            currentFP = currentFP->previousFp;
        }
        if (startIndex >= frameCount) {
            outStream_setError(outH, JDWP_Error_INVALID_INDEX);
            goto breakout;
        }

        /* Set frameCount to be the index of the last frame to retrieve */
        if ((len != -1) && (len < (frameCount - startIndex))) { 
            frameCount = startIndex + len;
        } else { 
            /* do nothing */
        }
            
        outStream_writeLong(outH, frameCount - startIndex);

        currentFP = thread->fpStore;
        currentIP = thread->ipStore;
        for (index = 0; index < frameCount; index++) { 
            if (index >= startIndex) { 
                METHOD method = currentFP->thisMethod;
                INSTANCE_CLASS clazz = method->ofClass;
                unsigned long offset = currentIP - method->u.java.code;
                unsigned long fpOffset;
                /* This is needed to preserve currentFP */
                stack = currentFP->stack;   /* This is protected, above */
                fpOffset = (char *)currentFP - (char *)stack;
                outStream_writeLong(outH, index);
                outStream_writeLocation(outH, JDWP_TypeTag_CLASS,
                                        (GET_CLASS_DEBUGGERID(&clazz->clazz)),
                                        getMethodIndex(method), offset);
#if INCLUDEDEBUGCODE
                if (tracedebugger) { 
                    fprintf(stdout, "    %ld: %s.%s\n", 
                            index, getClassName((CLASS)clazz), 
                            methodName(method));
                }
#endif
                currentFP = (FRAME)((char *)stack + fpOffset);
            }
            currentIP = currentFP->previousIp;
            currentFP = currentFP->previousFp;
        }
breakout:
    END_TEMPORARY_ROOTS
}

/*========================================================================
 * Function:        ThreadReference_FrameCount()
 * Overview:        returns the frame depth of a thread
 *                  
 * Interface:
 *  parameters:     input pointer, output pointer, isBreak bool
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

static void 
ThreadReference_FrameCount(PACKET_INPUT_STREAM_HANDLE inH, 
                           PACKET_OUTPUT_STREAM_HANDLE outH)
{
    START_TEMPORARY_ROOTS
        long threadID = inStream_readLong(inH);
        DECLARE_TEMPORARY_ROOT(THREAD, thread, GET_VMTHREAD(threadID));

        TRACE_DEBUGGER(("Thread", "Frame Count", "threadID=%lx", threadID));
        
        if (isValidThread(thread)) {
            if (thread->state & THREAD_DBG_SUSPENDED) {
                FRAME currentFP = thread->fpStore;
                long frameCount = 0;
                while (currentFP != NIL) {
                    frameCount++;
                    currentFP = currentFP->previousFp;
                }
#if INCLUDEDEBUGCODE
                if (tracedebugger) { 
                    fprintf(stdout, "frame count is %ld\n", frameCount);
                }
#endif
                outStream_writeLong(outH, frameCount);
            } else {
#if INCLUDEDEBUGCODE
                if (tracedebugger) { 
                    fprintf(stdout, "    Thread not suspended\n");
                }
#endif
                outStream_writeLong(outH, JDWP_Error_THREAD_NOT_SUSPENDED);
            }
        } else {
            outStream_setError(outH, JDWP_Error_INVALID_THREAD);
        }
    END_TEMPORARY_ROOTS
}

static void 
ArrayReference_Length(PACKET_INPUT_STREAM_HANDLE inH, 
                      PACKET_OUTPUT_STREAM_HANDLE outH)
{
    /****
    ARRAY array = (ARRAY)inStream_readObject(inH);
    ****/
    long objectID = inStream_readLong(inH);
    ARRAY array = (ARRAY)getObjectPtr(objectID);

    TRACE_DEBUGGER(("Array", "Length", "ArrayID=%ld, Array=0x%lx",
                    getObjectID((OBJECT)array), array));
    if (array != NULL && IS_ARRAY_CLASS(array->ofClass)) {
#if INCLUDEDEBUGCODE
        if (tracedebugger) { 
            fprintf(stdout, "    Length = %ld\n", array->length);
        }
#endif
        outStream_writeLong(outH, array->length);
    } else {    
        outStream_writeLong(outH, 0);
    }
}

static void
Array_GetterSetter(PACKET_INPUT_STREAM_HANDLE inH, 
                   PACKET_OUTPUT_STREAM_HANDLE outH, 
                   bool_t isSetter)
{ 
    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(ARRAY, array, (ARRAY)inStream_readObject(inH));
        long firstIndex = inStream_readLong(inH);
        long length = inStream_readLong(inH);
        long lastIndex = 
            (!isSetter && (length == -1)) ? array->length : firstIndex + length;

        TRACE_DEBUGGER(("Array", (isSetter ? "Set" : "Get"), 
                        "arrayID=%ld, array=0x%x, first=%ld, length=%ld", 
                        getObjectID((OBJECT)array), array, firstIndex, length));

        if (array != NULL) { 
            BYTE elementTypeTag = (array->ofClass->gcType == GCT_ARRAY) 
                ? (BYTE)typeCodeToSignature((char)(array->ofClass->u.primType))
                : (BYTE)JDWP_Tag_OBJECT;
            int i;

            if (!isSetter) { 
                if ((elementTypeTag == JDWP_Tag_OBJECT) &&
                      IS_ARRAY_CLASS(array->ofClass->u.elemClass)) { 
                    outStream_writeByte(outH, JDWP_Tag_ARRAY);
                } else { 
                    outStream_writeByte(outH, elementTypeTag);
                }
                outStream_writeLong(outH, lastIndex - firstIndex);
            }

            for (i=firstIndex; i < lastIndex; i++) {
                if (isSetter) { 
                    switch (elementTypeTag) { 
                    case JDWP_Tag_LONG:
                    case JDWP_Tag_DOUBLE:
                        readValueToAddress(inH, &array->data[2 * i].cell, elementTypeTag);
                        break;

                    case JDWP_Tag_BYTE:
                    case JDWP_Tag_BOOLEAN: 
                        ((BYTEARRAY)array)->bdata[i] = inStream_readByte(inH);
                         break;

                    case JDWP_Tag_CHAR:
                    case JDWP_Tag_SHORT: 
                        ((SHORTARRAY)array)->sdata[i] = inStream_readShort(inH);
                        break;

                    default:
                        readValueToAddress(inH, &array->data[i].cell, elementTypeTag);
                        break;
                    }
                } else { 
                    switch(elementTypeTag) {
                    case JDWP_Tag_LONG:
                    case JDWP_Tag_DOUBLE:
                        writeValueFromAddress(outH, &array->data[2 * i].cell, 
                                              elementTypeTag, FALSE);
                        break;

                    case JDWP_Tag_BYTE:
                    case JDWP_Tag_BOOLEAN: 
                        outStream_writeByte(outH, ((BYTEARRAY)array)->bdata[i]);
                        break;

                    case JDWP_Tag_CHAR:
                    case JDWP_Tag_SHORT: 
                        outStream_writeShort(outH, 
                                             ((SHORTARRAY)array)->sdata[i]);
                        break;

                    case JDWP_Tag_INT:
                    case JDWP_Tag_FLOAT:
                        writeValueFromAddress(outH, &array->data[i].cell, 
                                              elementTypeTag, FALSE);
                        break;

                    default: 
                        /* This is the only case in which we write out the
                         * tag */
                        writeValueFromAddress(outH, &array->data[i].cell, 
                                              elementTypeTag, TRUE);
                        break;
                    }
                }
#if INCLUDEDEBUGCODE
                if (tracedebugger) { 
                    fprintf(stdout, "    array[%d] = ", i);
                    switch(elementTypeTag) { 

                    case JDWP_Tag_LONG:
                    case JDWP_Tag_DOUBLE:
                        printValueFromTag(&array->data[2 * i].cell, elementTypeTag);
                        break;

                    case JDWP_Tag_BYTE:
                    case JDWP_Tag_BOOLEAN: 
                        fprintf(stdout, "%d\n", ((BYTEARRAY)array)->bdata[i]);
                        break;

                    case JDWP_Tag_CHAR:
                    case JDWP_Tag_SHORT: 
                        fprintf(stdout, "%d\n", ((SHORTARRAY)array)->sdata[i]);
                        break;

                    default:
                        printValueFromTag(&array->data[i].cell, elementTypeTag);
                        break;
                    }
                }
#endif
            }
        }
    END_TEMPORARY_ROOTS
}

static void
ArrayReference_GetValues(PACKET_INPUT_STREAM_HANDLE inH, 
                         PACKET_OUTPUT_STREAM_HANDLE outH)
{ 
    Array_GetterSetter(inH, outH, FALSE);
}

static void
ArrayReference_SetValues(PACKET_INPUT_STREAM_HANDLE inH, 
                         PACKET_OUTPUT_STREAM_HANDLE outH)
{ 
    Array_GetterSetter(inH, outH, TRUE);
}

/*========================================================================
 * Function:        createEvent
 * Overview:        creates a new event
 *                  
 * Interface:
 *  parameters:     
 *  returns:        void
 *
 * Notes:           since there is only one thread group all threads
 *                  are its children
 *=======================================================================*/

static void createEvent(PACKET_INPUT_STREAM_HANDLE inH, 
                        PACKET_OUTPUT_STREAM_HANDLE outH, BYTE kind)
{
    VMEventPtr ep;
    int i;
    BYTE modKind;
    EVENTMODIFIER curModSlot;
    EVENTMODIFIER newMod;
    THREAD thread;
    char *tmpName;
    unsigned long dummyLong;

    
    START_TEMPORARY_ROOTS
    ep = createVmEvent();
    
    ep->kind = kind;
    ep->suspendPolicy = inStream_readByte(inH);
    ep->numModifiers = inStream_readLong(inH);
    ep->count = 0;
    ep->count_active = FALSE;

    for (i=0; i < ep->numModifiers; i++) {

        modKind = inStream_readByte(inH);

        newMod = NIL;        

        switch(modKind) {
        case JDWP_EventRequest_Set_Out_modifiers_Modifier_Count:
            ep->count = inStream_readLong(inH);
        if (ep->count > 0)
            ep->count_active = TRUE;
            break;
        case JDWP_EventRequest_Set_Out_modifiers_Modifier_Conditional:
            inStream_readLong(inH); 
            break;
        case JDWP_EventRequest_Set_Out_modifiers_Modifier_ThreadOnly:
            inStream_readLong(inH); 
            break;
        case JDWP_EventRequest_Set_Out_modifiers_Modifier_ClassOnly:
            newMod = (EVENTMODIFIER)getModPtr();
            newMod->kind = modKind;
            newMod->u.classID = inStream_readLong(inH);
            break;
        case JDWP_EventRequest_Set_Out_modifiers_Modifier_ClassMatch:
            newMod = (EVENTMODIFIER)getModPtr();
            newMod->kind = modKind;
            IS_TEMPORARY_ROOT(tmpName, inStream_readString(inH));
            newMod->u.classMatch.className =
                (char *)callocPermanentObject(ByteSizeToCellSize((strlen(tmpName) + 1)));
        strcpy(newMod->u.classMatch.className, tmpName);
            break;
        case JDWP_EventRequest_Set_Out_modifiers_Modifier_ClassExclude:
            inStream_readString(inH); 
            break;
        case JDWP_EventRequest_Set_Out_modifiers_Modifier_LocationOnly:

            switch(kind) {
            case JDWP_EventKind_BREAKPOINT:
            case JDWP_EventKind_FIELD_ACCESS:
            case JDWP_EventKind_FIELD_MODIFICATION:
            case JDWP_EventKind_SINGLE_STEP:
            case JDWP_EventKind_EXCEPTION:
                newMod = (EVENTMODIFIER)getModPtr();
                newMod->kind = modKind;
                newMod->u.loc.tag = inStream_readByte(inH);
                newMod->u.loc.classID = inStream_readLong(inH);
                if (newMod->u.loc.classID == 0) {
                    outStream_setError(outH, JDWP_Error_ILLEGAL_ARGUMENT);
                    ep->inUse = FALSE;
                    goto noEvent;        /* skunked! */
                }
                newMod->u.loc.methodIndex =
                    (unsigned long)inStream_readLong(inH);
                inStream_readLong64(inH, &newMod->u.loc.offset, &dummyLong);
                
                /* Return an error back to the debugger if 
                 * the breakpoint could not be installed.
                 */
                if (installBreakpoint(&newMod) == FALSE) {
                    outStream_setError(outH, JDWP_Error_ILLEGAL_ARGUMENT);
                    ep->inUse = FALSE;
                    goto noEvent;
                }
                break;
            }

            break;
        case JDWP_EventRequest_Set_Out_modifiers_Modifier_ExceptionOnly:
            switch(kind) {
            case JDWP_EventKind_EXCEPTION:
                newMod = (EVENTMODIFIER)getModPtr();
                newMod->kind = modKind;
                newMod->u.exp.classID = inStream_readLong(inH);
                newMod->u.exp.sig_caught = inStream_readBoolean(inH);
                newMod->u.exp.sig_uncaught = inStream_readBoolean(inH);
                break;
            }
            break;
        case JDWP_EventRequest_Set_Out_modifiers_Modifier_FieldOnly:
            inStream_readObjectID(inH);
            inStream_readLong(inH);
            break;
        case JDWP_EventRequest_Set_Out_modifiers_Modifier_Step:
            switch (kind) {
            case JDWP_EventKind_SINGLE_STEP:

                newMod = (EVENTMODIFIER)getModPtr();
                newMod->kind = modKind;
                newMod->u.step.threadID = inStream_readLong(inH);
                thread = GET_VMTHREAD(newMod->u.step.threadID);
                if (thread == NULL || (thread->state & THREAD_DEAD)) {
                    /*
                     * If you try to single step after suspending because
                     * of an uncaught exception event we'll get sent the
                     * thread id of the thread that had the exception.
                     * That thread is dead though.
                     */
                    outStream_setError(outH, JDWP_Error_INVALID_THREAD);
                    ep->inUse = FALSE;
                    goto noEvent;
                }

                newMod->u.step.size = inStream_readLong(inH);
                newMod->u.step.depth = inStream_readLong(inH);
                
                /* query the _proxy_ for next line location */                        
        {
        CEModPtr cep = GetCEModifier();

                setEvent_NeedSteppingInfo(newMod->u.step.threadID, cep);
        /*
         * At this point, the proxy has made sure no more commands
         * from the debugger get sent until we get our line number
         * info from the proxy
         */
        processBreakCommands();
        /*
         * Once we get back here, then we know we've gotten the
         * command from the proxy with the line number info put into
         * the cep structure.
         */
 
                /* setup the relevant info */
        thread = GET_VMTHREAD(newMod->u.step.threadID);
                thread->isStepping = TRUE;
/*
                newMod->u.step.thread->stepInfo.target.cl =
            cep->step.target.cl;
                newMod->u.step.thread->stepInfo.target.method =
            cep->step.target.method;
*/
                thread->stepInfo.target.offset = cep->step.target.offset;
                thread->stepInfo.size = newMod->u.step.size;
                thread->stepInfo.depth = newMod->u.step.depth;
                if (thread == CurrentThread) {
                    thread->stepInfo.fp = fp_global;
                    thread->stepInfo.startingOffset =
                        ip_global - fp_global->thisMethod->u.java.code;
                } else {
                    thread->stepInfo.fp = thread->fpStore;
                    thread->stepInfo.startingOffset =
                        thread->ipStore - thread->fpStore->thisMethod->u.java.code;
                }
                thread->stepInfo.target.dupCurrentLineOffs =
                    cep->step.target.dupCurrentLineOffs;
                thread->stepInfo.target.postDupLineOffs =
                    cep->step.target.postDupLineOffs;
                FreeCEModifier(cep);
        }

                break;
            }
            break;
        }

        if (newMod != NIL) {
            /* insert the mod */
            newMod->next = NIL;
            if (ep->mods == NIL) {
                ep->mods = newMod;
                curModSlot = ep->mods;
            }
            else {
                curModSlot->next = newMod;
                curModSlot = curModSlot->next;
            }
        }
    }
    TRACE_DEBUGGER(("Create Event: ", "", "ep = 0x%lx, kind = 0x%lx, suspend = 0x%lx, num mods = 0x%lx", (long)ep, (long)ep->kind, (long)ep->suspendPolicy, ep->numModifiers));
#if INCLUDEDEBUGCODE
    if (tracedebugger) {
        for (i = 0; i < ep->numModifiers; i++) {
            newMod = ep->mods;
            switch (newMod->kind) {
            case JDWP_EventRequest_Set_Out_modifiers_Modifier_LocationOnly:
                fprintf(stdout, "DEBUGGER: Create Event: location modifier: classID = 0x%lx, methodIndex = 0x%lx, offset = 0x%lx\n",
                    newMod->u.loc.classID, newMod->u.loc.methodIndex,
                    newMod->u.loc.offset);
                break;
            case JDWP_EventRequest_Set_Out_modifiers_Modifier_ClassMatch:
                fprintf(stdout, "DEBUGGER: Create Event: location class match: className = %s\n", newMod->u.classMatch.className);
                break;
            default:
                break;
            }
       }
    }
#endif
    outStream_writeObjectID(outH, (long)ep);
noEvent:
    END_TEMPORARY_ROOTS
}

/*========================================================================
 * Function:        EventRequest_Set()
 * Overview:        sets up an event to happen such as a breakpoint
 * Interface:
 *   parameters:    pointer to debug header
 *   returns:       nothing
 *
 * Notes:
 *=======================================================================*/

static void 
EventRequest_Set(PACKET_INPUT_STREAM_HANDLE inH, 
                 PACKET_OUTPUT_STREAM_HANDLE outH)
{
    unsigned char eventType;
    long dbg_eventType;

    TRACE_DEBUGGER(("Event", "Set", NULL));

    eventType = inStream_readByte(inH);

    switch (eventType) {
    case JDWP_EventKind_BREAKPOINT:
        dbg_eventType = Dbg_EventKind_BREAKPOINT;
        break;
    case JDWP_EventKind_SINGLE_STEP:
        dbg_eventType = Dbg_EventKind_SINGLE_STEP;
        break;
    case JDWP_EventKind_THREAD_START:
        dbg_eventType = Dbg_EventKind_THREAD_START;
        break;
    case JDWP_EventKind_THREAD_END:
        dbg_eventType = Dbg_EventKind_THREAD_END;
        break;
    case JDWP_EventKind_CLASS_PREPARE:
        dbg_eventType = Dbg_EventKind_CLASS_PREPARE;
        break;
    case JDWP_EventKind_CLASS_LOAD:
        dbg_eventType = Dbg_EventKind_CLASS_LOAD;
        break;
    case JDWP_EventKind_CLASS_UNLOAD:
        dbg_eventType = Dbg_EventKind_CLASS_UNLOAD;
        break;
    case JDWP_EventKind_VM_INIT:
        dbg_eventType = Dbg_EventKind_VM_INIT;
        break;
    case JDWP_EventKind_VM_DEATH:
        dbg_eventType = Dbg_EventKind_VM_DEATH;
        break;
    case JDWP_EventKind_METHOD_EXIT:
        dbg_eventType = Dbg_EventKind_METHOD_EXIT;
        break;
    case JDWP_EventKind_EXCEPTION_CATCH:
        dbg_eventType = Dbg_EventKind_EXCEPTION_CATCH;
        break;
    case JDWP_EventKind_USER_DEFINED:
        dbg_eventType = Dbg_EventKind_USER_DEFINED;
        break;
    case JDWP_EventKind_METHOD_ENTRY:
        dbg_eventType = Dbg_EventKind_METHOD_ENTRY;
        break;
    case JDWP_EventKind_FIELD_MODIFICATION:
        dbg_eventType = Dbg_EventKind_FIELD_MODIFICATION;
        break;
    case JDWP_EventKind_FRAME_POP:
        dbg_eventType = Dbg_EventKind_FRAME_POP;
        break;
    case JDWP_EventKind_FIELD_ACCESS:
        dbg_eventType = Dbg_EventKind_FIELD_ACCESS;
        break;
    case JDWP_EventKind_EXCEPTION:
        dbg_eventType = Dbg_EventKind_EXCEPTION;
        break;
    case JDWP_EventKind_MIDLET_DEATH:
        dbg_eventType = Dbg_EventKind_MIDLET_DEATH;
        break;
    default:
        outStream_setError(outH, JDWP_Error_INVALID_EVENT_TYPE);
        return;
    }

    createEvent(inH, outH, eventType);
    
    addNotification(dbg_eventType);

}

/*========================================================================
 * Function:        clearEvent()
 * Overview:        clear one or all events
 * Interface:
 *   parameters:    pointer to debug header,  flag
 *   returns:       nothing
 *
 * Notes:       Packet format:
 *              Input:
 *                  event type  byte
 *                  event ID    int
 *              Output:
 *                  debug header -  11 bytes
 *=======================================================================*/

static void clearEvent(VMEventPtr ep)
{
    EVENTMODIFIER mod = NIL;
    INSTANCE_CLASS clazz;
    METHOD thisMethod;
    char *start;
    THREAD thread;

    START_TEMPORARY_ROOTS
    switch (ep->kind) {
    case JDWP_EventKind_BREAKPOINT:
        mod = getModifier(ep,
                     JDWP_EventRequest_Set_Out_modifiers_Modifier_LocationOnly);
        if (mod != NIL) {
            clazz = (INSTANCE_CLASS)GET_DEBUGGERID_CLASS(mod->u.loc.classID);
            thisMethod = &clazz->methodTable->methods[mod->u.loc.methodIndex - _method_index_base];
            start = (char *)clazz->constPool;
            setBreakpoint(thisMethod, mod->u.loc.offset, start,
                          mod->u.loc.opcode);
        }
        break;
    case JDWP_EventKind_SINGLE_STEP:
        mod = getModifier(ep,
                          JDWP_EventRequest_Set_Out_modifiers_Modifier_Step);
        if (mod != NIL) {
            IS_TEMPORARY_ROOT(thread, GET_VMTHREAD(mod->u.step.threadID));
            if (thread != NULL)
                thread->isStepping = FALSE;
        }
        break;
    }
    ep->active = FALSE;
    ep->inUse = FALSE;
    END_TEMPORARY_ROOTS
}

void clearAllBreakpoints()
{
    VMEventPtr ep;
    
    ep = eventHead;
    if (ep == NIL) return;
    
    do {
        if (ep->kind == JDWP_EventKind_BREAKPOINT)
            clearEvent(ep);
    } while ((ep = ep->next));
}

/*========================================================================
 * Function:        EventRequest_Clear()
 * Overview:        cancel a specific event request
 *                  
 * Interface:
 *  parameters:     input pointer, output pointer, isBreak bool
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

static void 
EventRequest_Clear(PACKET_INPUT_STREAM_HANDLE inH, 
                   PACKET_OUTPUT_STREAM_HANDLE outH)
{
    BYTE eventType = inStream_readByte(inH);
    VMEventPtr epid = (VMEventPtr)inStream_readObjectID(inH);
    VMEventPtr ep;
    
    TRACE_DEBUGGER(("Event", "Clear", NULL));

    eventType = 0;      /* satisfy GCC */
    ep = eventHead;
    if (ep == NIL) return;
    
    do {
        if (ep == epid)
            clearEvent(ep);
    } while ((ep = ep->next));

}

/*========================================================================
 * Function:        EventRequest_ClearAllBreakpoints()
 * Overview:        clears all breakpoint notifications
 *                  
 * Interface:
 *  parameters:     input pointer, output pointer, isBreak bool
 *  returns:        void
 *
 * Notes:
 *=======================================================================*/

static void 
EventRequest_ClearAllBreakpoints(PACKET_INPUT_STREAM_HANDLE inH, 
                                 PACKET_OUTPUT_STREAM_HANDLE outH)
{
    TRACE_DEBUGGER(("Event", "Clear All", NULL));
    clearAllBreakpoints();
}

/*========================================================================
 * Function:        StackFrame_SetValues
 * Overview:        set the value of one of the locals on the stack
 * Interface:
 *   parameters:    pointer to debug header
 *   returns:
 *
 * Notes:       Packet format:
 *              Input:
 *                  thread ID   Long
 *                  frame ID    Long
 *                  number of slots Long
 *                  
 *              Output:
 *                  debug header -      11 bytes
 *=======================================================================*/

static void
StackFrame_GetterSetter(PACKET_INPUT_STREAM_HANDLE inH, 
                        PACKET_OUTPUT_STREAM_HANDLE outH, 
                        bool_t isSetter)
{
    long threadID = inStream_readLong(inH);
    unsigned long frameID = inStream_readLong(inH);
    unsigned long numValues = inStream_readLong(inH);
    FRAME currentFP;
    unsigned long i;

    TRACE_DEBUGGER(("Stack Frame", (isSetter ? "Set" : "Get"), 
                    "threadID=%ld, frame=%ld, count=%ld", 
                    threadID, frameID, numValues));

    if (CurrentThread) { 
        storeExecutionEnvironment(CurrentThread);
    }

    currentFP = GET_VMTHREAD(threadID)->fpStore; /* volatile for GC */
    for (i = 0; i < frameID; i++) { 
        currentFP = currentFP->previousFp;
    }
    
    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_FRAME_ROOT(targetFP, currentFP);

        if (!isSetter) { 
            outStream_writeLong(outH, numValues);
        }
        
        for (i=0; i < numValues; i++) {
            cell *localP = FRAMELOCALS(targetFP); /* this is volatile */
            int slot = inStream_readLong(inH);
            char tag  = inStream_readByte(inH);
            if (isSetter) { 
                readValueToAddress(inH, localP + slot, tag);
            } else { 
                writeValueFromAddress(outH, localP + slot, tag, TRUE);
            }
#if INCLUDEDEBUGCODE            
            if (tracedebugger) { 
                fprintf(stdout, "    Local #%ld = ", (long)slot);
                localP = FRAMELOCALS(targetFP); /* it's volatile */
                printValueFromTag(localP + slot, tag);
            }
#endif
        }
    END_TEMPORARY_ROOTS
}

static void StackFrame_GetValues(PACKET_INPUT_STREAM_HANDLE inH, 
                                 PACKET_OUTPUT_STREAM_HANDLE outH)
{
    StackFrame_GetterSetter(inH, outH, FALSE);
}

static void StackFrame_SetValues(PACKET_INPUT_STREAM_HANDLE inH, 
                                 PACKET_OUTPUT_STREAM_HANDLE outH)
{
    StackFrame_GetterSetter(inH, outH, TRUE);
}

static void 
KVMCommands_HandShake(PACKET_INPUT_STREAM_HANDLE inH, 
                      PACKET_OUTPUT_STREAM_HANDLE outH) {

    char *versionString;
    int major;
    int minor;
    char *kvmString = "KVM Reference Debugger Interface";

    TRACE_DEBUGGER(("KVM Commands", "Handshake", NULL));

    START_TEMPORARY_ROOTS
    IS_TEMPORARY_ROOT(versionString, inStream_readString(inH));
    major = inStream_readByte(inH);
    minor = inStream_readByte(inH);
    outStream_writeString(outH, (CONST_CHAR_HANDLE)&kvmString);
    if (minor > 1) {
        /* This version of the agent indexes methods from 1 */
        _method_index_base = 1;
        outStream_writeLong(outH, 0x17FFF);  /* NOTE: bit 15-16 == 0x10 */
                                             /* method indexes start at 1 */
    } else {
        outStream_writeLong(outH, 0x7FFF);  /* set appropriate bits */
    }
    setResumeOK(TRUE);
    END_TEMPORARY_ROOTS
}

static void 
KVMCommands_GetSteppingInfo(PACKET_INPUT_STREAM_HANDLE inH, 
                            PACKET_OUTPUT_STREAM_HANDLE outH)
{
    unsigned long dummyLong;
    CEModPtr cep = (CEModPtr)inStream_readLong(inH);

    TRACE_DEBUGGER(("KVM Commands", "Get Stepping Info", NULL));
/*
    cep->step.target.cl = 0x00;
    cep->step.target.method = 0x00;
*/
    inStream_readLong64(inH, &cep->step.target.offset, &dummyLong);
    inStream_readLong64(inH, &cep->step.target.dupCurrentLineOffs, &dummyLong);
    inStream_readLong64(inH, &cep->step.target.postDupLineOffs, &dummyLong);
    setResumeOK(TRUE);
}

/*========================================================================
 * Function:        InitDebugger
 * Type:            private function
 * Overview:        Open listener socket to listen for debugger commands
 *
 * Interface:
 *   parameters:    <none>
 *   returns:       true - no error
 *                  false -  Error
 * Note:
 *=======================================================================*/

bool_t InitDebugger()
{

    if (!dbgInitialized()) {
        InitDebuggerIO();
        if (!GetDebugger(6000))
            return FALSE;
    }
    requestID = 1;

    debugRoot = (WEAKPOINTERLIST)callocObject(SIZEOF_WEAKPOINTERLIST(OBJECT_ROOT_SIZE),
                                              GCT_WEAKPOINTERLIST);
    debugRoot->length = (OBJECT_ROOT_SIZE);
    makeGlobalRoot((cell **)&debugRoot);
    bucketMap = (unsigned long *)mallocBytes(OBJECT_ROOT_SIZE * CELL);
    memset(bucketMap, 0, (OBJECT_ROOT_SIZE * CELL));
    makeGlobalRoot((cell **)&bucketMap);
    createHashTable(&debuggerHashTable, DEBUGGER_HASH_SIZE);

    vmDebugReady = TRUE;
    /*
     * Get the handshake from the proxy
     */
    processBreakCommands();
    return TRUE;
}

/*========================================================================
 * Function:        GetDebugger()
 * Overview:        see if a debugger is connecting to our listener socket
 * Interface:
 *   parameters:    timeout
 *   returns:       true - OK, false - bad
 *
 * Notes:
 *=======================================================================*/

bool_t GetDebugger(int timeout)
{
    bool_t retval;

    retval = GetDebuggerChannel(timeout);
    return retval;
}

/*========================================================================
 * Function:        CloseDebuggerIO()
 * Overview:        close the sockets
 * Interface:
 *   parameters:    none
 *   returns:       none
 *
 * Notes:
 *=======================================================================*/

void CloseDebugger()
{
    CloseDebuggerIO();
}

/*========================================================================
 * Function:        processDebugCmds()
 * Overview:        parse commands from debugger
 * Interface:
 *   parameters:    select timeout in milliseconds, -1 = forever
 *   returns:       nothing
 *
 * Notes:
 *=======================================================================*/

#include <debuggerCommands.h>

void ProcessDebugCmds(int timeout)
{
    struct CmdPacket *cmd;
    void **f2Array;
    CommandHandler func;
    Packet *p;

    if (!vmDebugReady) {
        return;
    }
    if (!dbgCharAvail(allThreadsSuspended ? -1 : timeout)) { 
        return;
    }
    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(PACKET_INPUT_STREAM, in, 
                 (PacketInputStream *)callocObject(SIZEOF_PACKETSTREAM,
                                                   GCT_POINTERLIST));
        DECLARE_TEMPORARY_ROOT(PACKET_INPUT_STREAM, out, 
                 (PacketInputStream *)callocObject(SIZEOF_PACKETSTREAM,
                                                   GCT_POINTERLIST));
        in->numPointers = 2;
        in->packet = (Packet *)callocObject(SIZEOF_PACKET, GCT_POINTERLIST);
        in->segment = in->packet->type.cmd.data =
            (PacketData *)callocObject(SIZEOF_PACKETDATA, GCT_POINTERLIST);
        in->packet->type.cmd.numPointers = 1;

        in->packet->type.cmd.data->data =
                   (unsigned char *)mallocBytes(MAX_INPUT_PACKET_SIZE);
        in->segment->numPointers = 1;

        if (dbgReceivePacket(&in) == SYS_ERR) { 
          /* We most likely lost the connection to the debug agent */
          VirtualMachine_Resume(&in, &out);
          vmDebugReady = FALSE;
          setNotification(0);
          clearAllBreakpoints();
          goto getOut;
        } 

        p = in->packet;
        if (p->type.cmd.flags & FLAGS_Reply) {
            /* It's a reply, just drop it */
            goto getOut;
        }
        cmd = &p->type.cmd;

        if (cmd->cmdSet == KVM_CMDSET) {
            f2Array = (void **)funcArray[0];
        } else {
            if (cmd->cmdSet == 0 || cmd->cmdSet > JDWP_HIGHEST_COMMAND_SET) {
                fprintf(stderr, KVM_MSG_UNKNOWN_DEBUGGER_COMMAND_SET);
                VM_EXIT(0);
            }
            f2Array = (void **)funcArray[cmd->cmdSet];
        }
        if (f2Array == NULL || cmd->cmd > (int)f2Array[0]) {
            fprintf(stderr, KVM_MSG_UNKNOWN_JDWP_COMMAND);
            VM_EXIT(0);
        }
        func = (CommandHandler)(f2Array[cmd->cmd]);

        outStream_initReply(&out, inStream_id(&in));
        inStream_init(&in);

        waitOnSuspend = FALSE;
        
        func(&in, &out);

        if (inStream_error(&in)) {
            outStream_setError(&out, inStream_error(&in));
        }
        outStream_sendReply(&out);
getOut:
    END_TEMPORARY_ROOTS
}

#if INCLUDEDEBUGCODE

static void 
printDebuggerInfo(const char *major, const char *minor, const char *format,...)
{
#if TERSE_MESSAGES
    fprintf(stdout, "DEBUGGER: %s %s\n", major, minor);
#else 
    fprintf(stdout, "DEBUGGER: %s %s: ", major, minor);
    if (format != NULL) { 
        va_list  args; 
        va_start(args, format);
            vfprintf(stdout, format, args);
        va_end(args);
    }
    fprintf(stdout, "\n");
#endif
}

static void 
printEventInfo(const char *format,...)
{
#if TERSE_MESSAGES
    fprintf(stdout, "EVENT: %s", format);
#else 
    va_list  args; 
    fprintf(stdout, "EVENT: ");
    va_start(args, format);
        vfprintf(stdout, format, args);
    va_end(args);
    fprintf(stdout, "\n");
#endif
}

static void
printValueFromTag(void *address, unsigned char typeTag) { 
    switch(typeTag) { 
        case JDWP_Tag_LONG:
        case JDWP_Tag_DOUBLE:
            fprintf(stdout, "(%c) %ld %ld\n", typeTag, 
                    ((long *)address)[0], ((long *)address)[1]);
            break;
            
        case JDWP_Tag_BYTE:
        case JDWP_Tag_CHAR:
        case JDWP_Tag_INT:
        case JDWP_Tag_FLOAT:
        case JDWP_Tag_SHORT:
        case JDWP_Tag_BOOLEAN:
            fprintf(stdout, "(%c) %ld\n", typeTag, ((long *)address)[0]);
            break;

        case JDWP_Tag_VOID:  /* happens with function return values */   
            fprintf(stdout, "\n");
            break;

        default: { 
            OBJECT value = ((OBJECT *)address)[0];
            fprintf(stdout, "(%c) ID=%ld, value=0x%lx\n", 
                    typeTag, getObjectID(value), (long)value);
            break;
        }
    }
}

#endif /* INCLUDEDEBUGCODE */

#endif /* ENABLE_JAVA_DEBUGGER */

