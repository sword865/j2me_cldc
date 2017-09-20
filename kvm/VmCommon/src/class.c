/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Internal runtime structures
 * FILE:      class.c
 * OVERVIEW:  Internal runtime class structures (see Class.h).
 * AUTHOR:    Antero Taivalsaari, Sun Labs, 1998
 *            Edited by Doug Simon 11/1998 (added the string pool)
 *            Added access checks by Sheng Liang for VM-spec compliance
 *            Frank Yellin (more checks for JLS compliance)
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>
#include <stddef.h>

/*=========================================================================
 * Global variables and definitions
 *=======================================================================*/

/* Pointers to the most important Java classes needed by the VM */
/* If Romizing, these are defined and initialized in ROMjava.c */

#if ROMIZING
#define EXTERN_IF_ROMIZING extern
#else
#define EXTERN_IF_ROMIZING
#endif

EXTERN_IF_ROMIZING
INSTANCE_CLASS JavaLangObject;    /* Pointer to class 'java.lang.Object' */

EXTERN_IF_ROMIZING
INSTANCE_CLASS JavaLangClass;     /* Pointer to class 'java.lang.Class' */

EXTERN_IF_ROMIZING
INSTANCE_CLASS JavaLangSystem;    /* Pointer to class 'java.lang.System' */

EXTERN_IF_ROMIZING
INSTANCE_CLASS JavaLangString;    /* Pointer to class 'java.lang.String' */

EXTERN_IF_ROMIZING
INSTANCE_CLASS JavaLangThread;    /* Pointer to class 'java.lang.Thread' */

EXTERN_IF_ROMIZING
INSTANCE_CLASS JavaLangThrowable; /* Pointer to class 'java.lang.Throwable' */

EXTERN_IF_ROMIZING
INSTANCE_CLASS JavaLangError;     /* Pointer to class 'java.lang.Error' */

EXTERN_IF_ROMIZING METHOD RunCustomCodeMethod;

EXTERN_IF_ROMIZING NameTypeKey initNameAndType;   /* void <init>() */
EXTERN_IF_ROMIZING NameTypeKey clinitNameAndType; /* void <clinit>() */
EXTERN_IF_ROMIZING NameTypeKey runNameAndType;    /* void run() */
EXTERN_IF_ROMIZING NameTypeKey mainNameAndType;   /* void main(String[]) */

EXTERN_IF_ROMIZING ARRAY_CLASS PrimitiveArrayClasses[T_LASTPRIMITIVETYPE + 1];

INSTANCE_CLASS JavaLangOutOfMemoryError;
THROWABLE_INSTANCE OutOfMemoryObject;
THROWABLE_INSTANCE StackOverflowObject;

/*=========================================================================
 * Static methods (only used in this file)
 *=======================================================================*/

static void runClinit(FRAME_HANDLE);
static void runClinitException(FRAME_HANDLE);

/*=========================================================================
 * Constructors
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      initializeClass()
 * TYPE:          constructor
 * OVERVIEW:      After loading a class, it must be initialized
 *                by executing the possible internal static
 *                constructor '<clinit>'. This will initialize the
 *                necessary static structures.
 *
 *                This function sets up the necessary Class.runClinit
 *                frame and returns to the interpreter.
 * INTERFACE:
 *   parameters:  class pointer
 *   returns:     <nothing>
 *=======================================================================*/

void initializeClass(INSTANCE_CLASS thisClass)
{
    if (thisClass->status == CLASS_ERROR) {
        raiseException(NoClassDefFoundError);
    } else if (thisClass->status < CLASS_READY) {
        if (thisClass->status < CLASS_VERIFIED) {
            verifyClass(thisClass);
        }
        /*
         * VerifyError will have been thrown or status will be
         * CLASS_VERIFIED. We can skip execution of <clinit> altogether if
         * it does not exists AND the superclass is already initialised.
         */
        if ((thisClass->superClass == NULL ||
            thisClass->superClass->status == CLASS_READY) &&
            getSpecialMethod(thisClass,clinitNameAndType) == NULL) {
            setClassStatus(thisClass,CLASS_READY);
        }
        else {
            TRY {
                pushFrame(RunCustomCodeMethod);
                pushStackAsType(CustomCodeCallbackFunction, &runClinit);
                pushStackAsType(INSTANCE_CLASS, thisClass);
                pushStackAsType(long, 1);
            } CATCH (e) {
                /* Stack overflow */
                setClassStatus(thisClass, CLASS_ERROR);
                THROW(e);
            } END_CATCH
        }
    }
}

/*=========================================================================
 * FUNCTION:      runClinit()
 * TYPE:          private initialization of a class
 * OVERVIEW:      Initialize a class. The Class.runCustomCode frame has
 *                already been pushed.
 *
 *        This function follows the exact steps as documented in
 *        the Java virtual machine spec (2ed) 2.17.5, except that
 *        Error is used instead of ExceptionInInitializerError
 *        because the latter is not defined in CLDC.
 *=======================================================================*/

static void
runClinit(FRAME_HANDLE exceptionFrameH)
{
    INSTANCE_CLASS thisClass;
    int state;
    bool_t haveMonitor = FALSE;
    if (exceptionFrameH) {
        runClinitException(exceptionFrameH);
        return;
    }

    state = topStackAsType(cell);
    thisClass = secondStackAsType(INSTANCE_CLASS);

    /* The 11 steps as documented in page 53 of the virtual machine spec. */
    switch (state) {
    case 1:
        /* A short cut that'll probably happen 99% of the time.  This class
         * has no monitor, and no one is in the middle of initializing this
         * class.  Since our scheduler is non preemptive, we can just
         * mark the class, without worrying about the monitor or other threads.
         */
        if (!OBJECT_HAS_MONITOR(&thisClass->clazz)
                && thisClass->initThread == NULL) {
            goto markClass;
        }

        /* Step 1:  Grab the class monitor so we have exclusive access. */
        if (monitorEnter((OBJECT)thisClass) != MonitorStatusOwn) {
            /* We've been forced to wait.  When we're awoken, we'll have
             * the lock */
            topStack = 2;
            return;
        } else {
            /* FALL THROUGH.  We have the lock */
        }

    case 2:
        haveMonitor = TRUE;

        if (thisClass->initThread && thisClass->initThread != CurrentThread) {
            /* Step 2:
             * Someone else is initializing this class.  Just wait until
             * a notification.  Of course, we'll have to recheck, since the
             * class could also be notified for other reasons.
             */
            long64 timeout;
            ll_setZero(timeout);
            monitorWait((OBJECT)thisClass, timeout);
            topStack = 2;
            return;
        }

    markClass:
        if (thisClass->initThread == CurrentThread ||
                   thisClass->status == CLASS_READY) { /* step 4 */
            /* Step 3, Step 4:
             * This thread is already initializing the class, or the class
             * has somehow already become initialized.  We're done.
             */
            if (haveMonitor) {
                char *junk;
                monitorExit((OBJECT)thisClass, &junk);
            }
            popFrame();
            return;
        }
        if (thisClass->status == CLASS_ERROR) {
            /* Step 5:
             * What can we do?
             */
            if (haveMonitor) {
                char *junk;
                monitorExit((OBJECT)thisClass, &junk);
            }
            popFrame();
            return;
        }

        /* Step 6:
         * Mark that we're about to initialize this class */
        setClassInitialThread(thisClass, CurrentThread);
        if (haveMonitor) {
            char *junk;
            monitorExit((OBJECT)thisClass, &junk);
            haveMonitor = FALSE;
        }
        /* FALL THROUGH */

    case 3:
        /* Step 7:
         * Initialize the superclass, if necessary */
        if ((thisClass->clazz.accessFlags & ACC_INTERFACE) == 0) {
            INSTANCE_CLASS superClass = thisClass->superClass;
            if (superClass && superClass->status != CLASS_READY) {
                topStack = 4;
                initializeClass(superClass);
                return;
            }
        }
        /* FALL THROUGH */

    case 4: {
        /* Step 8:
         * Run the <clinit> method, if the class has one
         */
        METHOD thisMethod = getSpecialMethod(thisClass, clinitNameAndType);
        if (thisMethod) {

#if INCLUDEDEBUGCODE
            if (traceclassloading || traceclassloadingverbose) {
                START_TEMPORARY_ROOTS
                    fprintf(stdout, "Initializing class: '%s'\n",
                            getClassName((CLASS)thisClass));
                END_TEMPORARY_ROOTS
            }
#endif /* INCLUDEDEBUGCODE */

            topStack = 5;
            pushFrame(thisMethod);
            return;
        } else {
            /* No <clinit> method. */
            /* FALL THROUGH */
        }
    }
    case 5:
        /* Step 9:
         * Grab the monitor so we can change the flags, and wake up any
         * other thread waiting on us.
         *
         * SHORTCUT: 99% of the time, there is no contention for the class.
         * Since our scheduler is non-preemptive, if there is no contention
         * for this class, we just go ahead and unmark the class, without
         * bothering with the monitor.
         */

        if (!OBJECT_HAS_MONITOR(&thisClass->clazz)) {
            goto unmarkClass;
        }

        if (monitorEnter((OBJECT)thisClass) != MonitorStatusOwn) {
            /* When we wake up, we'll have the monitor */
            topStack = 6;
            return;
        } else {
            /* FALL THROUGH */
        }

    case 6:
        haveMonitor = TRUE;
        /* Step 9, cont.
         * Mark the class as initialized. Wake up anyone waiting for the
         * class to be initialized.  Return the monitor.
         */
    unmarkClass:
        setClassInitialThread(thisClass, NULL);
        setClassStatus(thisClass, CLASS_READY);
#if ENABLE_JAVA_DEBUGGER
        if (vmDebugReady) {
            CEModPtr cep = GetCEModifier();
            cep->loc.classID = GET_CLASS_DEBUGGERID(&thisClass->clazz);
            cep->threadID = getObjectID((OBJECT)CurrentThread->javaThread);
            cep->eventKind = JDWP_EventKind_CLASS_PREPARE;
            insertDebugEvent(cep);
        }
#endif /* ENABLE_JAVA_DEBUGGER */
        if (haveMonitor) {
            char *junk;
            monitorNotify((OBJECT)thisClass, TRUE); /* wakeup everyone */
            monitorExit((OBJECT)thisClass, &junk);
        }
        popFrame();
        return;
        /* Step 10, 11:
         * These handle error conditions that cannot currently be
         * implemented in the KVM.
         */

    default:
        fatalVMError(KVM_MSG_STATIC_INITIALIZER_FAILED);
    }
}

static void
runClinitException(FRAME_HANDLE frameH)
{
    START_TEMPORARY_ROOTS
        void **bottomStack = (void **)(unhand(frameH) + 1); /* transient */
        INSTANCE_CLASS thisClass = bottomStack[1];
        int state = (int)bottomStack[2];
        DECLARE_TEMPORARY_ROOT(THROWABLE_INSTANCE, exception, bottomStack[0]);

        /*
         * Must be:
         *   a. a class initialization during bootstrap (state == 1), or
         *   a. executing either clinit or superclass(es) clinit
         *      (state == 4 || state = 5)
         */
        if (state != 1 && state != 4 && state != 5)
            fatalVMError(KVM_MSG_STATIC_INITIALIZER_FAILED);

        setClassStatus(thisClass, CLASS_ERROR);
        setClassInitialThread(thisClass, NULL);

        /* Handle exception during clinit */
        if (!isAssignableTo((CLASS)(exception->ofClass),(CLASS)JavaLangError)) {

            /* Replace exception with Error, then continue the throwing */
            DECLARE_TEMPORARY_ROOT(THROWABLE_INSTANCE, error,
                (THROWABLE_INSTANCE)instantiate(JavaLangError));
            DECLARE_TEMPORARY_ROOT(STRING_INSTANCE, messageString,
                exception->message);
            char *p = str_buffer;

            /* Create the new message:
             *   Static initializer: <className of throwable> <message>
             */
            strcpy(p, "Static initializer: ");
            p += strlen(p);
            getClassName_inBuffer(&exception->ofClass->clazz, p);
            p += strlen(p);
            if (messageString != NULL) {
                strcpy(p, ": ");
                p += strlen(p);
                getStringContentsSafely(messageString, p,
                    STRINGBUFFERSIZE - (p - str_buffer));
                p += strlen(p);
            }
            error->message = instantiateString(str_buffer, p - str_buffer);
            /* Replace the exception with our new Error, continue throwing */
            *(THROWABLE_INSTANCE *)(unhand(frameH) + 1) = error;

            /* ALTERNATIVE, JLS/JVMS COMPLIANT IMPLEMENTATION FOR THE CODE ABOVE:  */
            /* The code below works correctly if class ExceptionInInitializerError */
            /* is available.  For CLDC 1.1, we will keep on using the earlier code */
            /* located above, since ExceptionInInitializerError is not supported.  */

            /* Replace exception with Error, then continue the throwing */
            /*
            DECLARE_TEMPORARY_ROOT(THROWABLE_INSTANCE, error, NULL);

            TRY {
                raiseException(ExceptionInInitializerError);
            } CATCH(e) {
                if ((CLASS)e->ofClass == getClass(ExceptionInInitializerError)){
                    *((THROWABLE_INSTANCE*)&(e->backtrace) + 1) = exception;
                }
                error = e;
            } END_CATCH
            *(THROWABLE_INSTANCE *)(unhand(frameH) + 1) = error;
            */
        }

        /* If any other thread is waiting for this, we should try to wake them
         * up if we can.
         */
        if (OBJECT_HAS_REAL_MONITOR(&thisClass->clazz)) {
            MONITOR monitor = OBJECT_MHC_MONITOR(&thisClass->clazz);
            if (monitor->owner == NULL || monitor->owner == CurrentThread) {
                char *junk;
                monitorEnter((OBJECT)thisClass);
                monitorNotify((OBJECT)thisClass, TRUE); /* wakeup everyone */
                monitorExit((OBJECT)thisClass, &junk);
            }
        }

    END_TEMPORARY_ROOTS
}

/*=========================================================================
 * System-level constructors
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      InitializeJavaSystemClasses()
 * TYPE:          private constructor
 * OVERVIEW:      Load the standard Java system classes needed by the VM.
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

void InitializeJavaSystemClasses()
{
#if !ROMIZING
        int i;
        JavaLangObject = (INSTANCE_CLASS)getRawClass("java/lang/Object");
        JavaLangClass  = (INSTANCE_CLASS)getRawClass("java/lang/Class");
        JavaLangString = (INSTANCE_CLASS)getRawClass("java/lang/String");

        memset(PrimitiveArrayClasses, 0, sizeof(PrimitiveArrayClasses));
        for (i = T_FIRSTPRIMITIVE_TYPE; i <= T_LASTPRIMITIVETYPE; i++) {
            if (!IMPLEMENTS_FLOAT && (i == T_FLOAT || i == T_DOUBLE)) {
                /* If we're not implementing floats, then don't try to
                 * create arrays of floats or arrays of doubles */
            } else {
                PrimitiveArrayClasses[i] =
                    getArrayClass(1, NULL, typeCodeToSignature((char)i));
            }
        }

        TRY {

            /* Now we can go back and create these for real. . . .*/
            loadClassfile(JavaLangObject, TRUE);
            loadClassfile(JavaLangClass, TRUE);
            loadClassfile(JavaLangString, TRUE);

            /* Load or initialize some other system classes */
            JavaLangSystem = (INSTANCE_CLASS)getClass("java/lang/System");
            JavaLangThread = (INSTANCE_CLASS)getClass("java/lang/Thread");
            JavaLangThrowable = (INSTANCE_CLASS)getClass("java/lang/Throwable");
            JavaLangError = (INSTANCE_CLASS)getClass("java/lang/Error");

        } CATCH (e) {
            fprintf(stdout,"%s", getClassName((CLASS)e->ofClass));
            if (e->message != NULL) {
                fprintf(stdout, ": %s\n", getStringContents(e->message));
            } else {
                fprintf(stdout, "\n");
            }
            fatalVMError(KVM_MSG_UNABLE_TO_INITIALIZE_SYSTEM_CLASSES);
        } END_CATCH

#if INCLUDEDEBUGCODE
            if (   (JavaLangObject->status == CLASS_ERROR)
                || (JavaLangClass->status == CLASS_ERROR)
                || (JavaLangString->status == CLASS_ERROR)
                || (JavaLangThread == NULL)
                || (JavaLangThrowable == NULL)
                || (JavaLangError == NULL)) {
                fatalVMError(KVM_MSG_UNABLE_TO_INITIALIZE_SYSTEM_CLASSES);
            }
#endif /* INCLUDEDEBUGCODE */

#else /* ROMIZING */
        InitializeROMImage();
#endif /* !ROMIZING */

    if (!ROMIZING || RELOCATABLE_ROM) {
        /* Get handy pointers to special methods */
        initNameAndType   = getNameAndTypeKey("<init>",   "()V");
        clinitNameAndType = getNameAndTypeKey("<clinit>", "()V");
        runNameAndType    = getNameAndTypeKey("run",     "()V");
        mainNameAndType   = getNameAndTypeKey("main", "([Ljava/lang/String;)V");

        RunCustomCodeMethod = getSpecialMethod(JavaLangClass,
                                   getNameAndTypeKey("runCustomCode", "()V"));

        /* Patch the bytecode, was a "return" */
        if (RELOCATABLE_ROM) {
            /* Do nothing. Already patched */
        } else if (!USESTATIC || (inAnyHeap(RunCustomCodeMethod->u.java.code))){
            RunCustomCodeMethod->u.java.code[0] = CUSTOMCODE;
            RunCustomCodeMethod->u.java.maxStack =
                RunCustomCodeMethod_MAX_STACK_SIZE;
        } else {
            BYTE newcode = CUSTOMCODE;
            short newMaxStack = RunCustomCodeMethod_MAX_STACK_SIZE;
            char *start = (char*)((INSTANCE_CLASS)JavaLangClass)->constPool;
            int offset = (char*)(RunCustomCodeMethod->u.java.code) - start;
            modifyStaticMemory(start, offset, &newcode, sizeof(newcode));

            offset = (char *)(&RunCustomCodeMethod->u.java.maxStack) - start;
            modifyStaticMemory(start, offset,
                               &newMaxStack, sizeof(newMaxStack));
        }
    }
    JavaLangOutOfMemoryError =
        (INSTANCE_CLASS)getClass(OutOfMemoryError);
    OutOfMemoryObject =
        (THROWABLE_INSTANCE)instantiate(JavaLangOutOfMemoryError);
    makeGlobalRoot((cell **)&OutOfMemoryObject);
    StackOverflowObject = OutOfMemoryObject;
    /* If we had full JLS/JVMS error classes:
    StackOverflowObject =
        (THROWABLE_INSTANCE)instantiate(
            (INSTANCE_CLASS)getClass(StackOverflowError));
    */
    makeGlobalRoot((cell **)&StackOverflowObject);
}

/*=========================================================================
 * FUNCTION:      FinalizeJavaSystemClasses()
 * TYPE:          private constructor
 * OVERVIEW:      Perform any cleanup necessary so that the class files can
 *                be run again.
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *=======================================================================*/

void FinalizeJavaSystemClasses()
{
    if (ROMIZING) {
        FinalizeROMImage();
        FOR_ALL_CLASSES(clazz)
            /* Remove any class monitors */
            if (OBJECT_HAS_MONITOR(clazz)) {
                clearObjectMonitor((OBJECT)clazz);
            }
            if (!IS_ARRAY_CLASS(clazz)) {
                /* Reset the state of the class to be its initial state.
                 * The ACC_ROM_NON_INIT_CLASS says that neither this class nor
                 * any of its superclasses has a <clinit> method
                 */
                INSTANCE_CLASS iclazz = ((INSTANCE_CLASS)clazz);
                setClassInitialThread(iclazz, NULL);
                setClassStatus(iclazz,
                     (iclazz->clazz.accessFlags & ACC_ROM_NON_INIT_CLASS) ?
                              CLASS_READY : CLASS_VERIFIED);
            }
        END_FOR_ALL_CLASSES
    }
}

/*=========================================================================
 * Operations on classes
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      getRawClass()
 * TYPE:          instance-level operation on runtime classes
 * OVERVIEW:      Find a class with the given name, or create an empty
 *                stub for the class, if necessary.
 * INTERFACE:
 *=======================================================================*/

CLASS
getRawClass(const char *name) {
    if (INCLUDEDEBUGCODE && inCurrentHeap(name)) {
        fatalError(KVM_MSG_BAD_CALL_TO_GETRAWCLASS);
    }
    return getRawClassX(&name, 0, strlen(name));
}

CLASS
getRawClassX(CONST_CHAR_HANDLE nameH, int offset, int length)
{
    CLASS result;
    const char *start = unhand(nameH);
    const char *className = start + offset;
    const char *firstNonBracket, *p;
    int depth;

    /* Find the firstNonBracket character, and the last slash in the string */
    for (p = className; *p == '['; p++) {};
    firstNonBracket = p;
    depth = p - className;

    if (depth == 0) {
        UString packageName, baseName;
        for (p = className + length ; ;) {
            --p;
            if (*p == '/') {
                int packageNameLength = p - className;
                int baseNameLength = (length - 1) - packageNameLength;
                packageName = getUStringX(nameH, offset, packageNameLength);
                /* p and start may no longer be valid pointers, but there
                 * difference is still a valid offset */
                baseName = getUStringX(nameH, (p + 1) - start, baseNameLength);
                break;
            } else if (p == className) {
                packageName = NULL;
                baseName = getUStringX(nameH, offset, length);
                break;
            }
        }
        result = change_Name_to_CLASS(packageName, baseName);
        return result;
    } else if (depth + 1 == length) {
        /* An array of some primitive type */
        return  (CLASS)getArrayClass(depth, NULL, *firstNonBracket);
    } else {
        INSTANCE_CLASS baseClass;
        const char *baseClassStart = firstNonBracket + 1; /* skip the 'L' */
        const char *baseClassEnd = className + length - 1;  /* skip final ';' */
        baseClass = (INSTANCE_CLASS)
            getRawClassX(nameH, baseClassStart - start,
                baseClassEnd  - baseClassStart);
        /* The call to getArrayClass but we don't have any pointers any more. */
        return (CLASS)getArrayClass(depth, baseClass, '\0');
    }
}

/*=========================================================================
 * FUNCTION:      getClass()
 * TYPE:          public instance-level operation on runtime classes
 * OVERVIEW:      Find a class with the given name, loading the class
 *                if necessary.
 * INTERFACE:
 *   parameters:  class name as a string
 *   returns:     class pointer or NIL if not found
 *
 * NOTE:  This operation may load new classes into the system.
 *        Since class loading may necessitate the execution
 *        of class initialization methods (<clinit> methods),
 *        hence requiring the invocation of the bytecode interpreter,
 *        we must be careful not to mess the native (C/C++) call
 *        stack.
 *=======================================================================*/

CLASS
getClass(const char *name)
{
    if (INCLUDEDEBUGCODE && inCurrentHeap(name)) {
        fatalError(KVM_MSG_BAD_CALL_TO_GETCLASS);
    }
    return getClassX(&name, 0, strlen(name));
}

CLASS
getClassX(CONST_CHAR_HANDLE nameH, int offset, int length)
{
    CLASS clazz;
    clazz = getRawClassX(nameH, offset, length);
    if (!IS_ARRAY_CLASS(clazz)) {
        if (((INSTANCE_CLASS)clazz)->status == CLASS_RAW) {
            loadClassfile((INSTANCE_CLASS)clazz, TRUE);
        } else if (((INSTANCE_CLASS)clazz)->status == CLASS_ERROR) {
            START_TEMPORARY_ROOTS
                DECLARE_TEMPORARY_ROOT(char*, className, getClassName(clazz));
                raiseExceptionWithMessage(NoClassDefFoundError, className);
            END_TEMPORARY_ROOTS
        }
    }
    return clazz;
}

/*=========================================================================
 * FUNCTION:      revertToRawClass()
 * TYPE:          public instance-level operation on runtime classes
 * OVERVIEW:      Return a class to the state it is in after being returned
 *                getRawClassX the first time.
 * INTERFACE:
 *   parameters:  class pointer
 *   returns:     class pointer
 *=======================================================================*/

INSTANCE_CLASS revertToRawClass(INSTANCE_CLASS clazz) {
    memset(&(clazz->superClass),0,
            sizeof(struct instanceClassStruct) - sizeof(struct classStruct));
    return clazz;
}

/*=========================================================================
 * FUNCTION:      getArrayClass()
 * TYPE:          private helper function
 * OVERVIEW:      Create an array class.
 * INTERFACE:
 *   arguments:   depth:     depth of the new array
 *                baseClass: if the base type is a class this is nonNull and
 *                           contains the base type
 *                signCode:  if the base type is a primitive type, then
 *                           baseClass must be NULL, and this contains the
 *                           letter representing the base type.
 *   parameters:  result: class pointer.
 *=======================================================================*/

ARRAY_CLASS
getArrayClass(int depth, INSTANCE_CLASS baseClass, char signCode)
{
    UString packageName, baseName;
    bool_t isPrimitiveBase = (baseClass == NULL);
    ARRAY_CLASS clazz, result;
#if ENABLE_JAVA_DEBUGGER
    bool_t needEvent = FALSE;
#endif

    if (isPrimitiveBase) {
        const char *temp = str_buffer;
        packageName = NULL;
        memset(str_buffer, '[', depth);
        str_buffer[depth] = signCode;
        baseName = getUStringX(&temp, 0, depth + 1);
    } else {
        memset(str_buffer, '[', depth);
        sprintf(str_buffer + depth, "L%s;", UStringInfo(baseClass->clazz.baseName));
        baseName = getUString(str_buffer);
        packageName = baseClass->clazz.packageName;
    }

    result = (ARRAY_CLASS)change_Name_to_CLASS(packageName, baseName);
#if ENABLE_JAVA_DEBUGGER
    if (result->clazz.ofClass == NULL)
        needEvent = TRUE;
#endif

    for (clazz = result; clazz->clazz.ofClass == NULL; ) {
        /* LOOP ASSERT:  depth is the array depth of "clazz" */

        /* This clazz is newly created.  We need to fill in info.  We
         * may need to iterate, since this class's element type may also need
         * to be be created */
        clazz->clazz.ofClass = JavaLangClass;
        if (depth == 1 && isPrimitiveBase) {
#if ROMIZING
            fatalError(KVM_MSG_CREATING_PRIMITIVE_ARRAY);
#else
            char typeCode;
            switch(signCode) {
                case 'C' : typeCode = T_CHAR    ; break;
                case 'B' : typeCode = T_BYTE    ; break;
                case 'Z' : typeCode = T_BOOLEAN ; break;
#if IMPLEMENTS_FLOAT
                case 'F' : typeCode = T_FLOAT   ; break;
                case 'D' : typeCode = T_DOUBLE  ; break;
#endif /* IMPLEMENTS_FLOAT */
                case 'S' : typeCode = T_SHORT   ; break;
                case 'I' : typeCode = T_INT     ; break;
                case 'J' : typeCode = T_LONG    ; break;
                case 'V' : typeCode = T_VOID    ; break;
                case 'L' : typeCode = T_CLASS   ; break;
                default : fatalVMError(KVM_MSG_BAD_SIGNATURE); typeCode = 0;
            }
            clazz->gcType = GCT_ARRAY;
            clazz->itemSize = arrayItemSize(typeCode);
            clazz->u.primType = typeCode;
            clazz->clazz.accessFlags =
                ACC_FINAL | ACC_ABSTRACT | ACC_PUBLIC | ACC_ARRAY_CLASS;
            clazz->clazz.key = signCode + (short)(1 << FIELD_KEY_ARRAY_SHIFT);
            /* All finished */
#endif /* ROMIZING */
            break;
        } else {
            clazz->gcType = GCT_OBJECTARRAY;
            clazz->itemSize = arrayItemSize(T_REFERENCE);
            if (isPrimitiveBase) {
                clazz->clazz.accessFlags =
                    ACC_FINAL | ACC_ABSTRACT | ACC_PUBLIC | ACC_ARRAY_CLASS;
            } else if (baseClass->status >= CLASS_LOADED) {
                clazz->clazz.accessFlags =
                    ACC_FINAL | ACC_ABSTRACT | ACC_ARRAY_CLASS |
                    (baseClass->clazz.accessFlags & ACC_PUBLIC);
            } else {
                clazz->clazz.accessFlags =
                    ACC_FINAL | ACC_ABSTRACT | ACC_ARRAY_CLASS;
                clazz->flags = ARRAY_FLAG_BASE_NOT_LOADED;
            }
            if (depth >= MAX_FIELD_KEY_ARRAY_DEPTH) {
                clazz->clazz.key |=
                    (MAX_FIELD_KEY_ARRAY_DEPTH << FIELD_KEY_ARRAY_SHIFT);
            } else if (isPrimitiveBase) {
                clazz->clazz.key = (depth << FIELD_KEY_ARRAY_SHIFT) + signCode;
            } else {
                clazz->clazz.key =
                    (depth << FIELD_KEY_ARRAY_SHIFT) + baseClass->clazz.key;
            }

            if (depth == 1) {
                /* We must be nonPrimitive.  primitive was handled above */
                clazz->u.elemClass = (CLASS)baseClass;

                /* All finished */
                break;
            } else {
                /* Class of depth > 1. */
                UString thisBaseName = clazz->clazz.baseName;
                const char *baseName = UStringInfo(thisBaseName);
                UString subBaseName =
                    getUStringX(&baseName, 1, thisBaseName->length - 1);
                CLASS elemClass = change_Name_to_CLASS(packageName,
                                                       subBaseName);
                clazz->u.elemClass = elemClass;

                /* Now "recurse" on elemClass, initialized it, */
                /* also, if necessary */
                clazz = (ARRAY_CLASS)elemClass;
                depth--;
                continue;
            }
        }
    }
#if ENABLE_JAVA_DEBUGGER
    if (vmDebugReady && needEvent) {
        CEModPtr cep = GetCEModifier();
        cep->loc.classID = GET_CLASS_DEBUGGERID(&result->clazz);
        cep->threadID = getObjectID((OBJECT)CurrentThread->javaThread);
        cep->eventKind = JDWP_EventKind_CLASS_PREPARE;
        insertDebugEvent(cep);
    }
#endif /* ENABLE_JAVA_DEBUGGER */
    return result;
}

/*=========================================================================
 * FUNCTION:      getObjectArrayClass
 * TYPE:          public instance-level operation on runtime classes
 * OVERVIEW:      Creates an array class whose elements are the specified type
 *                getObjectArrayClass expects an array or instance Class
 * INTERFACE:
 *   parameters:  see above
 *   returns:     class pointer
 *=======================================================================*/

ARRAY_CLASS
getObjectArrayClass(CLASS clazz) {
    CLASS subClass = clazz;
    int depth = 1;
    for (;;) {
        if (!IS_ARRAY_CLASS(subClass)) {
            /* subClass is a base type */
            return getArrayClass(depth, (INSTANCE_CLASS)subClass, '\0');
        } else if (((ARRAY_CLASS)subClass)->gcType == GCT_ARRAY) {
            /* subClass is an array of primitives of depth 1. */
            int typeCode  = ((ARRAY_CLASS)subClass)->u.primType;
            char signCode = typeCodeToSignature((char)typeCode);
            return getArrayClass(depth + 1, NULL, signCode);
        } else {
            /* iterate down one level */
            subClass = ((ARRAY_CLASS)subClass)->u.elemClass;
            depth++;
        }
    }
}

/*=========================================================================
 * FUNCTION:      getClassName, getClassName_inBuffer
 * TYPE:          public instance-level operation on runtime classes
 * OVERVIEW:      Gets the name of a class
 *
 * INTERFACE:
 *   parameters:  clazz:       Any class
 *                str_buffer:  Where to put the result
 *
 *   returns:     getClassName() returns a pointer to the result
 *                getClassName_inBuffer() returns a pointer to the NULL
 *                at the end of the result.  The result is in the
 *                passed buffer.
 *
 * DANGER:  getClassName_inBuffer() does not return what you expect.
 *=======================================================================*/

char*
getClassName(CLASS clazz) {
    UString UPackageName = clazz->packageName;
    UString UBaseName    = clazz->baseName;
    int     baseLength = UBaseName->length;
    int     packageLength = UPackageName == 0 ? 0 : UPackageName->length;
    char *result = mallocBytes(baseLength + packageLength + 5);
    getClassName_inBuffer(clazz, result);
    return result;
}

char*                   /* returns pointer to '\0' at end of resultBuffer */
getClassName_inBuffer(CLASS clazz, char *resultBuffer) {
    UString UPackageName = clazz->packageName;
    UString UBaseName    = clazz->baseName;
    char *baseName = UStringInfo(UBaseName);
    char *from = baseName;      /* pointer into baseName */
    char *to = resultBuffer;    /* used for creating the output */
    int  fromLength = UBaseName->length;
    bool_t isArrayOfObject;

    /* The result should have exactly as many "["s as the baseName has at
     * its beginning.  Then skip over that part in both the input and the
     * output */
    for (from = baseName; *from == '['; from++, fromLength--) {
        *to++ = '[';
    }
    /* We're an array of objects if we've actually written something already
     * to "from", and more than one character remains
     */
    isArrayOfObject = (from != baseName && fromLength != 1);

    if (isArrayOfObject) {
        *to++ = 'L';
    }
    /* Now print out the package name, followed by the rest of the baseName */
    if (UPackageName != NULL) {
        int packageLength = UPackageName->length;
        memcpy(to, UStringInfo(UPackageName), packageLength);
        to += packageLength;
        *to++ = '/';
    }
    if (isArrayOfObject) {
        memcpy(to, from + 1, fromLength - 2); /* skip L and ; */
        to += (fromLength - 2);
        *to++ = ';';
    } else {
        memcpy(to, from, fromLength);
        to += fromLength;
    }
    *to = '\0';
    return to;
}

/*=========================================================================
 * FUNCTION:      printClassName()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Print the name of a class
 *                for debugging purposes.
 * INTERFACE:
 *   parameters:  class pointer
 *   returns:     <nothing>
 *=======================================================================*/

#if INCLUDEDEBUGCODE
void printClassName(CLASS thisClass)
{
    /* This shouldn't allocate memory. */
    char buffer[256];
    getClassName_inBuffer(thisClass, buffer);
    fprintf(stdout, "Class '%s'\n", buffer);
}

#endif /* INCLUDEDEBUGCODE */

/*=========================================================================
 * Type conversion helper functions
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      typeCodeToSignature, signatureToTypeCode
 * TYPE:          private function
 * OVERVIEW:      Converts signature characters to the corresponding
 *                type code small integer, and back.
 *=======================================================================*/

char
typeCodeToSignature(char typeCode) {
    switch(typeCode) {
        case T_CHAR :    return 'C';
        case T_BYTE :    return 'B';
        case T_BOOLEAN : return 'Z';
        case T_FLOAT :   return 'F';
        case T_DOUBLE :  return 'D';
        case T_SHORT :   return 'S';
        case T_INT :     return 'I';
        case T_LONG :    return 'J';
        case T_VOID:     return 'V';
        case T_CLASS:    return 'L';
        default :    fatalVMError(KVM_MSG_BAD_SIGNATURE); return 0;
    }
}

/*=========================================================================
 * Operations on instances
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      instantiate()
 * TYPE:          constructor
 * OVERVIEW:      Create an instance of the given class.
 * INTERFACE:
 *   parameters:  class pointer
 *   returns:     pointer to object instance
 *
 * NOTE:          This operation only allocates the space for the
 *                instance and initializes its instance variables to zero.
 *                Actual constructor for initializing the variables
 *                must be invoked separately.
 *=======================================================================*/

INSTANCE instantiate(INSTANCE_CLASS thisClass)
{
    long size = SIZEOF_INSTANCE(thisClass->instSize);
    INSTANCE newInstance = (INSTANCE)mallocHeapObject(size, GCT_INSTANCE);

    if (newInstance != NULL) {
        memset(newInstance, 0, size << log2CELL);
        /* Initialize the class pointer (zeroeth field in the instance) */
        newInstance->ofClass = thisClass;
    } else {
        THROW(OutOfMemoryObject);
    }
    return newInstance;
}

/*=========================================================================
 * FUNCTION:      objectHashCode()
 * TYPE:          public function
 * OVERVIEW:      Return the identity of the object.
 * INTERFACE:
 *   parameters:  object pointer
 *   returns:     object identity as long
 *
 * NOTE:          This operation needs to take into account that
 *                objects can move. Therefore, we can't simply use
 *                the address of the object as identity.
 *=======================================================================*/

long
objectHashCode(OBJECT object)
{
    unsigned static long lastHash = 0xCAFEBABE;

    /* The following may GC, but only if the result it returns is non-NULL */
    long* hashAddress = monitorHashCodeAddress(object);

    long result = (hashAddress == NULL) ? object->mhc.hashCode : *hashAddress;
    if (result == 0) {
        do {
            lastHash = lastHash * 0xDEECE66DL + 0xB;
            result = lastHash & ~03;
        } while (result == 0);
        if (hashAddress == NULL) {
            SET_OBJECT_HASHCODE(object, result);
        } else {
            *hashAddress = result;
        }
    }
    return result >> 2;
}

/*=========================================================================
 * FUNCTION:      implementsInterface()
 * TYPE:          public instance-level operation on runtime objects
 * OVERVIEW:      Check if the given class implements the given
 *                interface.
 * INTERFACE:
 *   parameters:  thisClass: class
 *                interface: the interface to implement
 *   returns:     boolean
 *=======================================================================*/

bool_t
implementsInterface(INSTANCE_CLASS thisClass, INSTANCE_CLASS thisInterface)
{
    unsigned short *ifaceTable;
    if (thisClass == thisInterface) {
        return TRUE;
    }
    if (!IS_ARRAY_CLASS(thisClass)
        && (((INSTANCE_CLASS)thisClass)->status == CLASS_RAW)){
        loadClassfile((INSTANCE_CLASS)thisClass, TRUE);
    }
    for (;;) {
        ifaceTable = thisClass->ifaceTable;
        if (ifaceTable != NULL) {
            int tableLength = ifaceTable[0];
            int i;
            for (i = 1; i <= tableLength; i++) {
                INSTANCE_CLASS ifaceClass =
                    (INSTANCE_CLASS)resolveClassReference(thisClass->constPool,
                                                          ifaceTable[i],
                                                          thisClass);
                if (implementsInterface(ifaceClass, thisInterface)) {
                    return TRUE;
                }
            }
        }
        if (thisClass == JavaLangObject) {
            return FALSE;
        }
        thisClass = thisClass->superClass;
    }
}

/*=========================================================================
 * FUNCTION:      isAssignableTo()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Check if the class 'fromClass' can be assigned
 *                to a variable of type 'toClass'.
 * INTERFACE:
 *   parameters:  fromClass, toClass: class pointers
 *   returns:     true if is assignable, false otherwise
 *
 * NOTE:          This code now implements all the hairy checks
 *                required by Java Language Specification.
 *=======================================================================*/

bool_t
isAssignableTo(CLASS fromClass, CLASS toClass) {
    for (;;) {
        /* You might be wondering how fromClass could ever be an interface,
         * since objects of never of type "fromClass".  This can only happen
         * on the second or subsequent iteration through the loop, when
         * fromClass was originally of type someInterface[]
         */
        if ((fromClass == toClass) || (toClass == (CLASS)JavaLangObject)) {
            return TRUE;
        }
        if (IS_ARRAY_CLASS(toClass)) {
            loadArrayClass((ARRAY_CLASS)toClass);
        } else {
            if (((INSTANCE_CLASS)toClass)->status == CLASS_RAW) {
                loadClassfile((INSTANCE_CLASS)toClass, TRUE);
            }
        }
        if (IS_ARRAY_CLASS(toClass)) {
            /* Only a compatible array can be assigned to an array */
            if (!IS_ARRAY_CLASS(fromClass)) {
                /* a non-array class can't be a subclass of an array class */
                return FALSE;
            } else {
                /* assigning an array to an array */
                int fromType = ((ARRAY_CLASS)fromClass)->gcType;
                int toType = ((ARRAY_CLASS)toClass)->gcType;
                /* fromType, toType are either GCT_OBJECTARRAY or GCT_ARRAY */
                if (toType != fromType) {
                    /* An array of primitives, and an array of non-primitives */
                    return FALSE;
                } else if (toType == GCT_ARRAY) {
                    /* Two arrays of primitive types */
                    return ((ARRAY_CLASS)fromClass)->u.primType ==
                           ((ARRAY_CLASS)toClass)->u.primType;
                } else {
                    /* Two object array types.  Look at there element
                     * types and recurse. */
                    fromClass = ((ARRAY_CLASS)fromClass)->u.elemClass;
                    toClass = ((ARRAY_CLASS)toClass)->u.elemClass;
                    continue;
                }
            }
        } else if ((toClass->accessFlags & ACC_INTERFACE) != 0) {
            /* Assigning to an interface.  fromClass must be a non-array that
             * implements the interface.
             * NB: If we ever have Cloneable or Serializable, we must fix
             * this, since arrays do implement those. */
            return (!(IS_ARRAY_CLASS(fromClass)) &&
                    implementsInterface((INSTANCE_CLASS)fromClass,
                                        (INSTANCE_CLASS)toClass));
        } else {
            /* Assigning to a non-array, non-interface, honest-to-god class,
             * that's not java.lang.Object */
            if (IS_ARRAY_CLASS(fromClass) ||
                    ((fromClass->accessFlags & ACC_INTERFACE) != 0)) {
                /* arrays and interfaces can only be assigned to
                 * java.lang.Object.  We already know that's it's not */
                return FALSE;
            } else {
                /* fromClass is also a non-array, non-interface class.  We
                 * need to check to see if fromClass is a subclass of toClass.
                 * We've know fromClass != toClass from the shortcut test
                 * way above
                 */
                INSTANCE_CLASS fromIClass = (INSTANCE_CLASS)fromClass;
                INSTANCE_CLASS toIClass = (INSTANCE_CLASS)toClass;
                while (fromIClass != JavaLangObject) {
                    if (!IS_ARRAY_CLASS(fromIClass)
                        && (((INSTANCE_CLASS)fromIClass)->status == CLASS_RAW)){
                        loadClassfile((INSTANCE_CLASS)fromIClass, TRUE);
                    }
                    fromIClass = fromIClass->superClass;
                    if (fromIClass == toIClass) {
                        return TRUE;
                    }
                }
                return FALSE;
            }
        }
    }
}

/*=========================================================================
 * FUNCTION:      isAssignableToFast()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Does a fast, non-GCing check to see if fromClass is
 *                assignable to toClass.  TRUE means yes.  FALSE means
 *                don't know.
 * INTERFACE:
 *   parameters:  fromClass, toClass: class pointers
 *   returns:     true if is assignable, false if uncertain
 *
 * THIS FUNCTION IS GUARANTEED NOT TO GARBAGE COLLECT
 *=======================================================================*/

bool_t
isAssignableToFast(CLASS fromClass, CLASS toClass) {
    if ((fromClass == toClass) || (toClass == (CLASS)JavaLangObject)) {
        return TRUE;
    } else if (IS_ARRAY_CLASS(fromClass) || IS_ARRAY_CLASS(toClass)) {
        /* We don't want to waste our time on these.  Just say "don't know" */
        return FALSE;
    } else {
        INSTANCE_CLASS fromIClass = (INSTANCE_CLASS)fromClass;
        INSTANCE_CLASS toIClass = (INSTANCE_CLASS)toClass;
        while (fromIClass != JavaLangObject) {
            if (fromIClass->status == CLASS_RAW) {
                /* Can't get more information without GC'ing. */
                return FALSE;
            }
            fromIClass = fromIClass->superClass;
            if (fromIClass == toIClass) {
                return TRUE;
            }
        }
        return FALSE;
    }
}

/*=========================================================================
 * FUNCTION:      printInstance()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Print the contents of an object instance
 *                for debugging purposes.
 * INTERFACE:
 *   parameters:  instance pointer
 *   returns:     <nothing>
 *=======================================================================*/

#if INCLUDEDEBUGCODE
void printInstance(INSTANCE thisInstance)
{
    INSTANCE_CLASS thisClass = thisInstance->ofClass;

    int   instSize  = thisClass->instSize;
    int   i;

    fprintf(stdout, "Instance %lx contents:\n", (long)thisInstance);
    printClassName((CLASS)thisClass);
    for (i = 0; i < instSize; i++) {
        long contents = thisInstance->data[i].cell;
        fprintf(stdout, "Field %d: %lx (%ld)\n", i, contents, contents);
    }
}
#endif /* INCLUDEDEBUGCODE */

/*=========================================================================
 * Operations on array instances
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      instantiateArray()
 * TYPE:          constructor
 * OVERVIEW:      Create an array of given type and size.
 * INTERFACE:
 *   parameters:  array class, array size
 *   returns:     pointer to array object instance
 * NOTE:          This operation only allocates the space for the
 *                array and initializes its slots to zero.
 *                Actual constructor for initializing the array
 *                must be invoked separately.
 *=======================================================================*/

ARRAY instantiateArray(ARRAY_CLASS arrayClass, long length)
{
    if (length < 0) {
        raiseException(NegativeArraySizeException);
        return NULL;
    } else if (length > 0x1000000) {
        /* Don't even try; 'dataSize' below might overflow */
        THROW(OutOfMemoryObject);
    } else {
        /* Does this array contain pointers or not? */
        GCT_ObjectType gctype = arrayClass->gcType;
        /* How big is each item */
        long slotSize = arrayClass->itemSize;
        /* What's the total data size */
        long dataSize = (length * slotSize + (CELL - 1)) >> log2CELL;
        /* What's the total size */
        long arraySize = SIZEOF_ARRAY(dataSize);
        ARRAY newArray;

        newArray = (ARRAY)mallocHeapObject(arraySize, gctype);
        if (newArray == NULL) {
            THROW(OutOfMemoryObject);
        } else {
            memset(newArray, 0, arraySize << log2CELL);
            newArray->ofClass   = arrayClass;
            newArray->length    = length;
        }
        return newArray;
    }
}

/*=========================================================================
 * FUNCTION:      instantiateMultiArray()
 * TYPE:          constructor
 * OVERVIEW:      Create an n-dimensional array (recursively).
 * INTERFACE:
 *   parameters:  array type index, integer array containing the
 *                different dimensions, number of dimensions
 *   returns:     pointer to array object instance
 *=======================================================================*/

ARRAY
instantiateMultiArray(ARRAY_CLASS arrayClass, long *lengthsArg, int dimensions)
{
    long currDepth;
    ARRAY result;
    for (currDepth = 0; currDepth < dimensions; currDepth++) {
        long dim = lengthsArg[currDepth];
        if (dim < 0) {
            raiseException(NegativeArraySizeException);
            return NIL;
        }
    }

    START_TEMPORARY_ROOTS
        STACK stack = getFP()->stack; /* volatile */
        /* lengthsArg is pointing into the stack */
        DECLARE_TEMPORARY_ROOT_FROM_BASE(long*, lengths, lengthsArg, stack);
        DECLARE_TEMPORARY_ROOT(ARRAY, rootArray,
            instantiateArray(getArrayClass(1, JavaLangObject, 0), 1));
        DECLARE_TEMPORARY_ROOT(ARRAY, prevArraySet, NULL);
        DECLARE_TEMPORARY_ROOT(ARRAY, currArraySet, rootArray);
        DECLARE_TEMPORARY_ROOT(ARRAY, prevArray, NULL);
        DECLARE_TEMPORARY_ROOT(ARRAY, currArray, NULL);
        long prevArrayWidth, currArrayWidth;
        CLASS currClass;

        if (rootArray == NULL) {
            /* Out of memory exception has already been thrown */
            return NULL;
        }

        currArrayWidth = 1;

        /* We have rootArray acting as the previous level to level 0.
         * It has only one slot, and the allocator has already set it to 0,
         * so it will appear as a linked list with one node alone.
         *
         * Now go through all the lengths and do the work
         */
        for (currDepth = 0, currClass = (CLASS)arrayClass;
                 ;
             currDepth++,   currClass = ((ARRAY_CLASS)currClass)->u.elemClass) {
            bool_t lastIteration;
            prevArraySet   = currArraySet;
            prevArrayWidth = currArrayWidth;

            currArraySet   = NULL;
            currArrayWidth = lengths[currDepth];
            lastIteration = (currDepth == dimensions - 1 ||
                             currArrayWidth == 0);

            /* Go through each of the previous level's nodes, and allocate
             * and link the current nodes, if needed.  Fill them in the slots
             * of the previous level's nodes.
             */
            do {
                long index;

                /* Get an item from the chain, and advance the chain. */
                prevArray = prevArraySet;
                prevArraySet = (ARRAY)prevArraySet->data[0].cellp;

                /* Fill in each index of prevArray */
                for (index = 0; index < prevArrayWidth; index++) {
                    /* Allocate the node, and fill in the parent's slot. */
                    currArray =
                        instantiateArray((ARRAY_CLASS)currClass,
                                         currArrayWidth);
                    if (currArray == NULL) {
                        /* OutOfMemoryError has already been thrown */
                        rootArray->data[0].cellp = NULL;
                        goto done;
                    }

                    prevArray->data[index].cellp = (cell *)currArray;
                    if (!lastIteration) {
                        /* Link it in, for the next time through the loop. */
                        currArray->data[0].cellp = (cell *)currArraySet;
                        currArraySet = currArray;
                    }
                }
            } while (prevArraySet != NULL);

            if (lastIteration) {
                break;
            }
        }
 done:
        result = (ARRAY)rootArray->data[0].cellp;
    END_TEMPORARY_ROOTS
    return result;
}

/*=========================================================================
 * FUNCTION:      arrayItemSize()
 * TYPE:          private helper operation for array manipulation
 * OVERVIEW:      Get the item size of the array on the basis of its type.
 * INTERFACE:
 *   parameters:  array type index
 *   returns:     array item size
 *=======================================================================*/

long arrayItemSize(int arrayType)
{
    long itemSize;

    switch (arrayType) {
    case T_BOOLEAN: case T_BYTE:
        itemSize = 1;
        break;
    case T_CHAR: case T_SHORT:
        itemSize = SHORTSIZE;
        break;
    case T_INT: case T_FLOAT:
        itemSize = CELL;
        break;
    case T_DOUBLE: case T_LONG:
        itemSize = CELL*2;
        break;
    default:
        /* Array of reference type (T_REFERENCE or class pointer) */
        itemSize = CELL;
        break;
    }
    return itemSize;
}

/*=========================================================================
 * FUNCTION:      printArray()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Print the contents of an array object instance
 *                for debugging purposes.
 * INTERFACE:
 *   parameters:  array object pointer
 *   returns:     <nothing>
 *=======================================================================*/

#if INCLUDEDEBUGCODE
void printArray(ARRAY thisArray)
{
    ARRAY_CLASS thisClass = thisArray->ofClass;
    if (!IS_ARRAY_CLASS(thisClass)) {
        fprintf(stdout, "Object %lx is not an array\n", (long)thisArray);
        return;
    }

    fprintf(stdout, "Array %lx contents:\n", (long)thisArray);
    printClassName((CLASS)thisClass);
    fprintf(stdout, "Length: %ld\n", thisArray->length);

    if (thisClass->gcType == GCT_ARRAY) {
        int length, i;
        if (thisClass->u.primType == T_CHAR) {
            SHORTARRAY shortArray = (SHORTARRAY)thisArray;
            fprintf(stdout, "Contents....: '\n");
            length = (thisArray->length < 40) ? thisArray->length : 40;
            for (i = 0; i < length; i++) {
                putchar((char)shortArray->sdata[i]);
            }
            fprintf(stdout, "'\n");
        }
    }
}
#endif /* INCLUDEDEBUGCODE */

/*=========================================================================
 * Operations on string instances
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      createCharArray()
 * TYPE:          constructor (internal use only)
 * OVERVIEW:      Create a character array. Handle Utf8 input properly.
 *=======================================================================*/

SHORTARRAY createCharArray(const char *utf8stringArg,
                                  int utf8length,
                                  int *unicodelengthP,
                                  bool_t isPermanent)
{
    int unicodelength = 0;
    int i;
    SHORTARRAY newArray;
    const char *p, *end;
    int size, objSize;

    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(const char *, utf8string, utf8stringArg);
        for (p = utf8string, end = p + utf8length;  p < end;  ) {
            utf2unicode(&p);
            unicodelength++;
        }
        size = (unicodelength * sizeof(short) + CELL - 1) >> log2CELL;
        objSize = SIZEOF_ARRAY(size);

        /* Allocate room for the character array */
        newArray = isPermanent
                       ? (SHORTARRAY)callocPermanentObject(objSize)
                       : (SHORTARRAY)callocObject(objSize, GCT_ARRAY);
        newArray->ofClass = PrimitiveArrayClasses[T_CHAR];
        newArray->length  = unicodelength;

        /* Initialize the array with string contents */
        for (p = utf8string, i = 0; i < unicodelength; i++) {
            newArray->sdata[i] = utf2unicode(&p);
        }
        *unicodelengthP = unicodelength;
    END_TEMPORARY_ROOTS
    return newArray;
}

/*=========================================================================
 * FUNCTION:      instantiateString(), instantiateInternedString()
 * TYPE:          constructor
 * OVERVIEW:      Create an initialized Java-level string object.
 * INTERFACE:
 *   parameters:  UTF8 String containing the value of the new string object
 *                number of bytes in the UTF8 string
 *   returns:     pointer to array object instance
 * NOTE:          String arrays are not intended to be manipulated
 *                directly; they are manipulated from within instances
 *                of 'java.lang.String'.
 *=======================================================================*/

STRING_INSTANCE
instantiateString(const char *stringArg, int utflength)
{
    int unicodelength;
    STRING_INSTANCE result;
    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(const char *, string, stringArg);
        DECLARE_TEMPORARY_ROOT(SHORTARRAY, chars,
            createCharArray(string, utflength, &unicodelength, FALSE));
        result = (STRING_INSTANCE)instantiate(JavaLangString);
        /* We can't do any garbage collection, since result isn't protected */
        result->offset = 0;
        result->length = unicodelength;
        result->array = chars;
    END_TEMPORARY_ROOTS
    return result;
}

INTERNED_STRING_INSTANCE
instantiateInternedString(const char *stringArg, int utflength)
{
    int unicodelength;
    INTERNED_STRING_INSTANCE result;
    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(const char *, string, stringArg);
        SHORTARRAY chars =
            createCharArray(string, utflength, &unicodelength, TRUE);
        result = (INTERNED_STRING_INSTANCE)
                 callocPermanentObject(SIZEOF_INTERNED_STRING_INSTANCE);
        /* We can't do any garbage collection, since result isn't protected */
        result->ofClass = JavaLangString;
        result->offset = 0;
        result->length = unicodelength;
        result->array = chars;
    END_TEMPORARY_ROOTS
    return result;
}

/*=========================================================================
 * FUNCTION:      getStringContents()
 * TYPE:          internal operation on string objects
 * OVERVIEW:      Get the contents of a string object in C/C++
 *                string format.
 * INTERFACE:
 *   parameters:  String object pointer
 *   returns:     char* to the string in C/C++ format
 *
 * NOTE:          This operation uses an internal string buffer
 *                that is shared and has a fixed length.  Thus,
 *                the contents of the returned string may (will)
 *                change over time.
 *=======================================================================*/

char* getStringContents(STRING_INSTANCE string)
{
   return getStringContentsSafely(string, str_buffer, STRINGBUFFERSIZE);
}

/*=========================================================================
 * FUNCTION:      getStringContentsSafely()
 * TYPE:          internal operation on string objects
 * OVERVIEW:      Get the contents of a string object in C/C++
 *                string format.
 * INTERFACE:
 *   parameters:  String object pointer
 *   returns:     char* to the string in C/C++ format
 *=======================================================================*/

char* getStringContentsSafely(STRING_INSTANCE string, char *buf, int lth)
{
    /* Get the internal unicode array containing the string */
    SHORTARRAY thisArray = string->array;
    int        offset    = string->offset;
    int        length    = string->length;
    int        i;

    if ((length+1) > lth) {
        fatalError(KVM_MSG_STRINGBUFFER_OVERFLOW);
    }

    /* Copy contents of the unicode array to the C/C++ string */
    for (i = 0; i < length; i++) {
        buf[i] = (char)thisArray->sdata[offset + i];
    }

    /* Terminate the C/C++ string with zero */
    buf[length] = 0;
    return buf;
}

#if INCLUDEDEBUGCODE
void
printString(INSTANCE string)
{
    SHORTARRAY data = (SHORTARRAY)string->data[0].cellp;
    int offset = string->data[1].cell;
    int length = string->data[2].cell;
    int i;
    putchar('"');
    for (i = 0; i < length; i++) {
        putchar((char)data->sdata[offset + i]);
    }
    putchar('"');
    putchar('\n');
 }

#endif /* INCLUDEDEBUGCODE */

