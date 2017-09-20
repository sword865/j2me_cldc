/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Native function interface
 * FILE:      native.h
 * OVERVIEW:  This file contains the definitions of native functions
 *            needed by the Java Virtual Machine. The implementation
 *            is _not_ based on JNI (Java Native Interface),
 *            because JNI seems too expensive for small devices.
 * AUTHOR:    Antero Taivalsaari, Sun Labs, 1998
 *            Edited by Doug Simon 11/1998
 *            Frank Yellin
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

/*=========================================================================
 * Definitions and declarations
 *=======================================================================*/

typedef void NativeFunctionType(void);
typedef NativeFunctionType *NativeFunctionPtr;

typedef struct {
    const char *const name;
    const char *const signature;
    const NativeFunctionPtr implementation;
} NativeImplementationType;

typedef struct {
    const char *const packageName;
    const char *const baseName;
    const NativeImplementationType *const implementation;
} ClassNativeImplementationType;

#define NATIVE_END_OF_LIST { NULL, NULL }

/*=========================================================================
 * Variables
 *=======================================================================*/

extern const ClassNativeImplementationType nativeImplementations[];

/* This variable is set to point to the current native */
/* method during native function calls.  The variable  */
/* is used for adjusting exception handler ranges in   */
/* frame.c, as well as helping access local variables  */
/* in KNI calls.  It is important that this variable   */
/* is kept NULL when the VM is not executing a native  */
/* method */
extern METHOD CurrentNativeMethod;

/*=========================================================================
 * Operations on native functions
 *=======================================================================*/

NativeFunctionPtr getNativeFunction(INSTANCE_CLASS clazz,
                                    const char* methodName,
                                    const char* methodSignature);

void  invokeNativeFunction(METHOD thisMethod);
void  nativeInitialization(int *argc, char **argv);

#if INCLUDEDEBUGCODE
void printNativeFunctions(void);
#endif

/*=========================================================================
 * Native function prototypes
 *=======================================================================*/

/*
 * Function prototypes of those native functions
 * that are commonly linked into KVM.
 */
void Java_java_lang_Object_getClass(void);
void Java_java_lang_Object_hashCode(void);
void Java_java_lang_System_identityHashCode(void);
void Java_java_lang_Object_notify(void);
void Java_java_lang_Object_notifyAll(void);
void Java_java_lang_Object_wait(void);
void Java_java_lang_Class_isInterface(void);
void Java_java_lang_Class_isPrimitive(void);
void Java_java_lang_Class_forName(void);
void Java_java_lang_Class_newInstance(void);
void Java_java_lang_Class_getName(void);
void Java_java_lang_Class_isInstance(void);
void Java_java_lang_Class_isArray(void);
void Java_java_lang_Class_isAssignableFrom(void);
void Java_java_lang_Class_getPrimitiveClass(void);
void Java_java_lang_Class_exists(void);
void Java_java_lang_Class_getResourceAsStream0(void);
void Java_java_lang_Thread_activeCount(void);
void Java_java_lang_Thread_currentThread(void);
void Java_java_lang_Thread_yield(void);
void Java_java_lang_Thread_sleep(void);
void Java_java_lang_Thread_start(void);
void Java_java_lang_Thread_isAlive(void);
void Java_java_lang_Thread_setPriority0(void);
void Java_java_lang_Thread_setPriority(void);
void Java_java_lang_Thread_interrupt0(void);
void Java_java_lang_Thread_interrupt(void);

void Java_java_lang_Throwable_fillInStackTrace(void);
void Java_java_lang_Throwable_printStackTrace0(void);
void Java_java_lang_Runtime_exitInternal(void);
void Java_java_lang_Runtime_freeMemory(void);
void Java_java_lang_Runtime_totalMemory(void);
void Java_java_lang_Runtime_gc(void);
void Java_java_lang_System_arraycopy (void);
void Java_java_lang_System_currentTimeMillis(void);
void Java_java_lang_System_getProperty0(void);
void Java_java_lang_System_getProperty(void);
void Java_java_lang_String_charAt(void);
void Java_java_lang_String_equals(void);
void Java_java_lang_String_indexOf__I(void);
void Java_java_lang_String_indexOf__II(void);
void Java_java_lang_String_intern(void);
void Java_java_lang_StringBuffer_append__I(void);
void Java_java_lang_StringBuffer_append__Ljava_lang_String_2(void);
void Java_java_lang_StringBuffer_toString(void);
void Java_java_lang_Math_randomInt(void);
void Java_java_lang_ref_WeakReference_initializeWeakReference(void);
void Java_java_util_Calendar_init(void);
void Java_com_sun_cldc_io_Waiter_waitForIO(void);
void Java_com_sun_cldc_io_j2me_socket_Protocol_initializeInternal(void);
void Java_com_sun_cldc_io_ConsoleOutputStream_write(void);
