/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=======================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: K Native Interface (KNI)
 * FILE:      kni.h
 * OVERVIEW:  KNI definitions and declarations
 * AUTHORS:   Efren A. Serra, Antero Taivalsaari
 *=======================================================================*/

/*=======================================================================
 * KNI is an implementation-level native function interface 
 * for CLDC-category VMs.  KNI is intended to be significantly
 * more lightweight than JNI, so we have made some compromises:
 *
 * - Compile-time interface with static linking.
 * - Source-level (no binary level) compatibility.
 * - No argument marshalling. Arguments are read explicitly,
 *   and return values are set explicitly.
 * - No invocation API (cannot call Java from native code).
 * - No class definition support.
 * - Limited object allocation support (strings only).
 * - Limited array region access for arrays of a primitive type.
 * - No monitorenter/monitorexit support.
 * - Limited error handling and exception support. 
 *   KNI functions do not throw exceptions, but return error
 *   values/null instead, or go fatal for severe errors.
 * - Exceptions can be thrown explicitly, but no direct 
 *   exception manipulation is supported.
 *
 * An important goal for the KNI implementation for the
 * KVM is backwards compatibility.  All the native functions 
 * written for the KVM using the earlier pre-KNI style should 
 * still work.
 *=======================================================================*/

#ifndef _KNI_H_
#define _KNI_H_

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>

/*=========================================================================
 * Definitions and declarations
 *=======================================================================*/

/*
 * KNI primitive data types
 *
 * Note: jlong is defined in machine_md.h, since its definition 
 * varies from one machine/compiler to another.
 */
typedef unsigned char  jboolean;
typedef signed char    jbyte;
typedef unsigned short jchar;
typedef short          jshort;
typedef long           jint;
typedef float          jfloat;
typedef double         jdouble;
typedef long           jsize;

/*
 * KNI reference data types
 * 
 * Note: jfieldID and jobject are intended to be opaque
 * types.  The programmer should not make any assumptions
 * about the actual type of these types, since the actual
 * type may vary from one KNI implementation to another.
 * 
 * In the KVM implementation of KNI, we use the corresponding
 * KVM-specific types to make debugging easier.
 */
typedef FIELD   jfieldID;
typedef cell**  jobject;

typedef jobject jclass;
typedef jobject jthrowable;
typedef jobject jstring;
typedef jobject jarray;
typedef jarray  jbooleanArray;
typedef jarray  jbyteArray;
typedef jarray  jcharArray;
typedef jarray  jshortArray;
typedef jarray  jintArray;
typedef jarray  jlongArray;
typedef jarray  jfloatArray;
typedef jarray  jdoubleArray;
typedef jarray  jobjectArray;

/*
 * KNI return types
 *
 * The KNI implementation uses these type declarations
 * to define the actual machine-specific return type of
 * a KNI function.
 *
 * Note: KNI return types are intended to be opaque types.
 * On the KVM, all these return types are defined as 'void',
 * but their definition may vary from one VM to another.
 */
typedef void KNI_RETURNTYPE_VOID;
typedef void KNI_RETURNTYPE_BOOLEAN;
typedef void KNI_RETURNTYPE_BYTE;
typedef void KNI_RETURNTYPE_CHAR;
typedef void KNI_RETURNTYPE_SHORT;
typedef void KNI_RETURNTYPE_INT;
typedef void KNI_RETURNTYPE_LONG;
typedef void KNI_RETURNTYPE_FLOAT;
typedef void KNI_RETURNTYPE_DOUBLE;
typedef void KNI_RETURNTYPE_OBJECT;

/*
 * jboolean constants
 */
#define KNI_TRUE  1
#define KNI_FALSE 0

/*
 * Return values for KNI functions.
 * Values correspond to JNI.
 */
#define KNI_OK           0                 /* Success */
#define KNI_ERR          (-1)              /* Unknown error */
#define KNI_ENOMEM       (-4)              /* Not enough memory */
#define KNI_EINVAL       (-6)              /* Invalid arguments */

#define KNIEXPORT extern

/*
 * Version information
 */
#define KNI_VERSION 0x00010000 /* KNI version 1.0 */

/*=========================================================================
 * KNI functions (refer to KNI Specification for details)
 *=======================================================================*/

/* Version information */
KNIEXPORT jint     KNI_GetVersion();

/* Class and interface operations */
KNIEXPORT void     KNI_FindClass(const char* name, jclass classHandle);
KNIEXPORT void     KNI_GetSuperClass(jclass classHandle, jclass superclassHandle);
KNIEXPORT jboolean KNI_IsAssignableFrom(jclass classHandle1, jclass classHandle2);

/* Exceptions and errors */
KNIEXPORT jint     KNI_ThrowNew(const char* name, const char* message);
KNIEXPORT void     KNI_FatalError(const char* message);

/* Object operations */
KNIEXPORT void     KNI_GetObjectClass(jobject objectHandle, jclass classHandle);
KNIEXPORT jboolean KNI_IsInstanceOf(jobject objectHandle, jclass classHandle);

/* Instance field access */
KNIEXPORT jfieldID KNI_GetFieldID(jclass classHandle, const char* name, const char* signature);

KNIEXPORT jboolean KNI_GetBooleanField(jobject objectHandle, jfieldID fieldID);
KNIEXPORT jbyte    KNI_GetByteField(jobject objectHandle, jfieldID fieldID);
KNIEXPORT jchar    KNI_GetCharField(jobject objectHandle, jfieldID fieldID);
KNIEXPORT jshort   KNI_GetShortField(jobject objectHandle, jfieldID fieldID);
KNIEXPORT jint     KNI_GetIntField(jobject objectHandle, jfieldID fieldID);
KNIEXPORT jfloat   KNI_GetFloatField(jobject objectHandle, jfieldID fieldID);
KNIEXPORT jlong    KNI_GetLongField(jobject objectHandle, jfieldID fieldID);
KNIEXPORT jdouble  KNI_GetDoubleField(jobject objectHandle, jfieldID fieldID);
KNIEXPORT void     KNI_GetObjectField(jobject objectHandle, jfieldID fieldID, jobject toHandle);

KNIEXPORT void     KNI_SetBooleanField(jobject objectHandle, jfieldID fieldID, jboolean value);
KNIEXPORT void     KNI_SetByteField(jobject objectHandle, jfieldID fieldID, jbyte value);
KNIEXPORT void     KNI_SetCharField(jobject objectHandle, jfieldID fieldID, jchar value);
KNIEXPORT void     KNI_SetShortField(jobject objectHandle, jfieldID fieldID, jshort value);
KNIEXPORT void     KNI_SetIntField(jobject objectHandle, jfieldID fieldID, jint value);
KNIEXPORT void     KNI_SetFloatField(jobject objectHandle, jfieldID fieldID, jfloat value);
KNIEXPORT void     KNI_SetLongField(jobject objectHandle, jfieldID fieldID, jlong value);
KNIEXPORT void     KNI_SetDoubleField(jobject objectHandle, jfieldID fieldID, jdouble value);
KNIEXPORT void     KNI_SetObjectField(jobject objectHandle, jfieldID fieldID, jobject fromHandle);

/* Static field access */
KNIEXPORT jfieldID KNI_GetStaticFieldID(jclass classHandle, const char* name, const char* signature);

KNIEXPORT jboolean KNI_GetStaticBooleanField(jclass classHandle, jfieldID fieldID);
KNIEXPORT jbyte    KNI_GetStaticByteField(jclass classHandle, jfieldID fieldID);
KNIEXPORT jchar    KNI_GetStaticCharField(jclass classHandle, jfieldID fieldID);
KNIEXPORT jshort   KNI_GetStaticShortField(jclass classHandle, jfieldID fieldID);
KNIEXPORT jint     KNI_GetStaticIntField(jclass classHandle, jfieldID fieldID);
KNIEXPORT jlong    KNI_GetStaticLongField(jclass classHandle, jfieldID fieldID);
KNIEXPORT jfloat   KNI_GetStaticFloatField(jclass classHandle, jfieldID fieldID);
KNIEXPORT jdouble  KNI_GetStaticDoubleField(jclass classHandle, jfieldID fieldID);
KNIEXPORT void     KNI_GetStaticObjectField(jclass classHandle, jfieldID fieldID, jobject toHandle);

KNIEXPORT void     KNI_SetStaticBooleanField(jclass classHandle, jfieldID fieldID, jboolean value);
KNIEXPORT void     KNI_SetStaticByteField(jclass classHandle, jfieldID fieldID, jbyte value);
KNIEXPORT void     KNI_SetStaticCharField(jclass classHandle, jfieldID fieldID, jchar value);
KNIEXPORT void     KNI_SetStaticShortField(jclass classHandle, jfieldID fieldID, jshort value);
KNIEXPORT void     KNI_SetStaticIntField(jclass classHandle, jfieldID fieldID, jint value);
KNIEXPORT void     KNI_SetStaticLongField(jclass classHandle, jfieldID fieldID, jlong value);
KNIEXPORT void     KNI_SetStaticFloatField(jclass classHandle, jfieldID fieldID, jfloat value);
KNIEXPORT void     KNI_SetStaticDoubleField(jclass classHandle, jfieldID fieldID, jdouble value);
KNIEXPORT void     KNI_SetStaticObjectField(jclass classHandle, jfieldID fieldID, jobject fromHandle);

/* String operations */
KNIEXPORT jsize    KNI_GetStringLength(jstring stringHandle);
KNIEXPORT void     KNI_GetStringRegion(jstring stringHandle, jsize offset, jsize n, jchar* jcharbuf);
KNIEXPORT void     KNI_NewString(const jchar* uchars, jsize length, jstring stringHandle);
KNIEXPORT void     KNI_NewStringUTF(const char* utf8chars, jstring stringHandle);

/* Array operations */
KNIEXPORT jsize    KNI_GetArrayLength(jarray arrayHandle);

KNIEXPORT jboolean KNI_GetBooleanArrayElement(jbooleanArray arrayHandle, jint index);
KNIEXPORT jbyte    KNI_GetByteArrayElement(jbyteArray arrayHandle, jint index);
KNIEXPORT jchar    KNI_GetCharArrayElement(jcharArray arrayHandle, jint index);
KNIEXPORT jshort   KNI_GetShortArrayElement(jshortArray arrayHandle, jint index);
KNIEXPORT jint     KNI_GetIntArrayElement(jintArray arrayHandle, jint index);
KNIEXPORT jfloat   KNI_GetFloatArrayElement(jfloatArray arrayHandle, jint index);
KNIEXPORT jlong    KNI_GetLongArrayElement(jlongArray arrayHandle, jint index);
KNIEXPORT jdouble  KNI_GetDoubleArrayElement(jdoubleArray arrayHandle, jint index);
KNIEXPORT void     KNI_GetObjectArrayElement(jobjectArray arrayHandle, jint index, jobject toHandle);

KNIEXPORT void     KNI_SetBooleanArrayElement(jbooleanArray arrayHandle, jint index, jboolean value);
KNIEXPORT void     KNI_SetByteArrayElement(jbyteArray arrayHandle, jint index, jbyte value);
KNIEXPORT void     KNI_SetCharArrayElement(jcharArray arrayHandle, jint index, jchar value);
KNIEXPORT void     KNI_SetShortArrayElement(jshortArray arrayHandle, jint index, jshort value);
KNIEXPORT void     KNI_SetIntArrayElement(jintArray arrayHandle, jint index, jint value);
KNIEXPORT void     KNI_SetFloatArrayElement(jfloatArray arrayHandle, jint index, jfloat value);
KNIEXPORT void     KNI_SetLongArrayElement(jlongArray arrayHandle, jint index, jlong value);
KNIEXPORT void     KNI_SetDoubleArrayElement(jdoubleArray arrayHandle, jint index, jdouble value);
KNIEXPORT void     KNI_SetObjectArrayElement(jobjectArray arrayHandle, jint index, jobject fromHandle);

KNIEXPORT void     KNI_GetRawArrayRegion(jarray arrayHandle, jsize offset, jsize n, jbyte* dstBuffer);
KNIEXPORT void     KNI_SetRawArrayRegion(jarray arrayHandle, jsize offset, jsize n, const jbyte* srcBuffer);

/* Parameter access */
KNIEXPORT jboolean KNI_GetParameterAsBoolean(jint index);
KNIEXPORT jbyte    KNI_GetParameterAsByte(jint index);
KNIEXPORT jchar    KNI_GetParameterAsChar(jint index);
KNIEXPORT jshort   KNI_GetParameterAsShort(jint index);
KNIEXPORT jint     KNI_GetParameterAsInt(jint index);
KNIEXPORT jfloat   KNI_GetParameterAsFloat(jint index);
KNIEXPORT jlong    KNI_GetParameterAsLong(jint index);
KNIEXPORT jdouble  KNI_GetParameterAsDouble(jint index);

KNIEXPORT void     KNI_GetParameterAsObject(jint index, jobject toHandle);
KNIEXPORT void     KNI_GetThisPointer(jobject toHandle);
KNIEXPORT void     KNI_GetClassPointer(jclass toHandle);

/* Return operations (implemented as macros on the KVM) */
#define KNI_ReturnVoid() \
{ \
    if (CurrentNativeMethod != NULL) { \
        kvm_resetOperandStack(); \
    } \
    return; \
}

#define KNI_ReturnBoolean(value) \
{ \
    kvm_resetOperandStack(); \
    pushStack((value)); \
    return; \
}

#define KNI_ReturnByte(value) \
{ \
    kvm_resetOperandStack(); \
    pushStack((value)); \
    return; \
}

#define KNI_ReturnChar(value) \
{ \
    kvm_resetOperandStack(); \
    pushStack((value)); \
    return; \
}

#define KNI_ReturnShort(value) \
{ \
    kvm_resetOperandStack(); \
    pushStack((value)); \
    return; \
}

#define KNI_ReturnInt(value) \
{ \
    kvm_resetOperandStack(); \
    pushStack((value)); \
    return; \
}

#define KNI_ReturnLong(value) \
{ \
    kvm_resetOperandStack(); \
    pushLong((value)); \
    return; \
}

#define KNI_ReturnFloat(value) \
{ \
    kvm_resetOperandStack(); \
    pushStackAsType(float, (value)); \
    return; \
}

#define KNI_ReturnDouble(value) \
{ \
    kvm_resetOperandStack(); \
    pushDouble(value); \
    return; \
}

/* Handle operations (implemented as macros on the KVM) */
#define KNI_StartHandles(n) \
    { int _tmp_roots_ = TemporaryRootsLength

#define KNI_DeclareHandle(handle) \
    DECLARE_TEMPORARY_ROOT(jobject, handle ## _0, NULL); \
    jobject handle = (jobject)&(handle ## _0)

#define KNI_IsNullHandle(handle) \
    ((*(handle)) ? KNI_FALSE : KNI_TRUE)

#define KNI_IsSameObject(handle1, handle2) \
    ((*(handle1) == *(handle2)) ? KNI_TRUE : KNI_FALSE)

#define KNI_ReleaseHandle(handle) \
    *(handle) = 0;

#define KNI_EndHandles() \
    END_TEMPORARY_ROOTS

#define KNI_EndHandlesAndReturnObject(handle) \
    kvm_resetOperandStack(); \
    pushStack(*(cell*)(handle)); \
    END_TEMPORARY_ROOTS \
    return;

/* Private operations */
void kvm_resetOperandStack();

/*=========================================================================
 * COMMENT:
 * Since version 1.0.3, KVM garbage collector includes a special
 * callback mechanism that allows each KVM port to register
 * implementation-specific cleanup routines that get called
 * automatically upon system exit.  This mechanism can be used, 
 * e.g., for adding network or UI-widget cleanup routines to 
 * the system.  These cleanup routines will be called automatically
 * when the corresponding Java object gets garbage collected.
 *
 * In KVM 1.0.4 and 1.1, we have made this mechanism capable
 * of being used from KNI functions.  A separate cleanup
 * registration function, KNI_registerCleanup, has been 
 * defined for this purpose.  Unlike the regular register-
 * Cleanup routine defined in file 'garbage.h', this function
 * accepts parameters as 'jobject' handles instead of 
 * INSTANCE_HANDLEs. 
 *
 * NOTE: These routines are not part of the KNI API!!
 *       They have been provided as a convenience for
 *       people who need to port existing class libraries
 *       onto KNI.
 *=======================================================================*/

/* The function type of a cleanup callback function */
typedef void (*KNICleanupCallback)(jobject);

/*
 * Register a cleanup function for the instance.
 * When the gc finds the object unreachable it will see if there is
 * an associated cleanup function and if so will call it with the instance.
 */
extern void KNI_registerCleanup(jobject, KNICleanupCallback);

#endif /* _KNI_H_ */

