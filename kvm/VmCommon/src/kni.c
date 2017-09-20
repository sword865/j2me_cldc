/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=======================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: K Native Interface (KNI)
 * FILE:      kni.c
 * OVERVIEW:  KNI implementation for the KVM.
 * AUTHORS:   Efren A. Serra, Antero Taivalsaari
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <kni.h>

/*=========================================================================
 * Definitions and declarations
 *=======================================================================*/

/* Get the contents of a handle */
/* (not intended for end users) */
#define KNI_UNHAND(handle) (*(handle))

/* Set the contents of a handle */
/* (not intended for end users) */
#define KNI_SETHAND(handle, value) (*(cell*)(handle)) = (cell)(value)

/*=========================================================================
 * Private functions
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:       kvm_resetOperandStack()
 * OVERVIEW:       Private function for resetting the operand 
 *                 stack of the KVM to the state it is intended to
 *                 be upon exiting a native function or before
 *                 pushing the return value of a native function
 *                 to be operand stack.
 * INTERFACE:
 *   parameter(s): <none>
 *   returns:      <nothing>
 *=======================================================================*/

void kvm_resetOperandStack() {
    int decrement = (CurrentNativeMethod->accessFlags & ACC_STATIC) ? 0 : 1;
    setSP(CurrentThread->nativeLp - decrement);
}

/*=========================================================================
 * FUNCTION:       KNI_registerCleanup()
 * OVERVIEW:       Private function for calling KVM-specific
 *                 native cleanup registration functionality.
 *                 For more comments on this feature, see file
 *                 "kni.h" or "garbage.h".
 * INTERFACE:
 *   parameter(s): objectHandle: the object for whom a native cleanup
 *                 callback is registered
 *                 cb: the callback routine to be called when the
 *                 object is garbage collected.
 *   returns:      <nothing>
 *=======================================================================*/

void KNI_registerCleanup(jobject objectHandle, KNICleanupCallback cb) {
    /* We can safely cast the parameters,     */
    /* since they are functionally equivalent */
    /*
    registerCleanup((INSTANCE_HANDLE)objectHandle, (CleanupCallback)cb);
    */
}

/*=========================================================================
 * Public KNI functions
 *=======================================================================*/

/*=========================================================================
 * Version information
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:       KNI_GetVersion()
 * OVERVIEW:       Returns the KNI version information.
 * INTERFACE:
 *   parameter(s): <none>
 *   returns:      The current KNI version
 *=======================================================================*/

jint KNI_GetVersion() {
    return KNI_VERSION;
}

/*=========================================================================
 * Class and interface operations
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:       KNI_FindClass()
 * OVERVIEW:       Returns a handle to a class or interface type.
 *                 If the class cannot be found, the function sets 
 *                 the classHandle to NULL.             
 * INTERFACE:
 *   parameter(s): name: a class descriptor as a UTF8 string
 *                 (example: "java/lang/String")
 *                 classHandle: a handle to which the class pointer
 *                 will be stored.
 *   returns:      nothing directly, but returns the class pointer
 *                 in handle 'classHandle'.  If the class cannot be
 *                 found, classHandle will be set to NULL.
 *=======================================================================*/

void KNI_FindClass(const char* name, jclass classHandle) {
    CLASS clazz = getRawClass(name);
    
    if (IS_ARRAY_CLASS(clazz) ||
        ((INSTANCE_CLASS)clazz)->status >= CLASS_READY) {
        KNI_SETHAND(classHandle, clazz);
    } else {
        KNI_SETHAND(classHandle, NULL);
    }
}

/*=========================================================================
 * FUNCTION:       KNI_GetSuperClass()
 * OVERVIEW:       Returns a handle to the superclass of the given class 
 *                 or interface.
 * INTERFACE:
 *   parameter(s): classHandle: a handle pointing to a class
 *                 superclassHandle: another handle to which the 
 *                 superclass pointer will be stored
 *   returns:      nothing directly, but returns the superclass pointer
 *                 in handle 'superclassHandle'.  If no superclass can
 *                 be found, superclassHandle will be set to NULL.
 *=======================================================================*/

void KNI_GetSuperClass(jclass classHandle, jclass superclassHandle) {
    INSTANCE_CLASS clazz = (INSTANCE_CLASS)KNI_UNHAND(classHandle);
    if (INCLUDEDEBUGCODE && clazz == 0) return;
    KNI_SETHAND(superclassHandle, clazz->superClass);
}

/*=========================================================================
 * FUNCTION:       KNI_IsAssignableFrom()
 * OVERVIEW:       Determines whether an instance of one class or interface
 *                 can be assigned to an instance of another class or
 *                 interface
 * INTERFACE:
 *   parameter(s): classHandle1: a handle to a class or interface
 *                 classHandle2: a handle to a class or interface
 *   returns:      true or false
 *=======================================================================*/

jboolean KNI_IsAssignableFrom(jclass classHandle1, jclass classHandle2) {
    CLASS class1 = (CLASS)KNI_UNHAND(classHandle1);
    CLASS class2 = (CLASS)KNI_UNHAND(classHandle2);
    return isAssignableTo(class1, class2);
}

/*=========================================================================
 * Exceptions and errors
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:       KNI_ThrowNew()
 * OVERVIEW:       Raises an exception in the current thread.
 * INTERFACE:
 *   parameter(s): name: class descriptor in UTF8 format
 *                 (example: "java/lang/Exception")
 *                 message: the exception message
 *   returns:      KNI_OK upon success; otherwise KNI_ERR
 * NOTE:           KNI_ThrowNew does not run the constructor
 *                 on the new exception objects.
 *=======================================================================*/

jint KNI_ThrowNew(const char* name, const char* message) {
    CLASS clazz;

    /* If the exception is thrown from a native */
    /* finalizer (CurrentNativeMethod is NULL), */
    /* then the exception must not be allowed.  */
    if (CurrentNativeMethod == NULL)
        return KNI_ERR;
 
    /* Redundant check: Make sure the exception class exists */
    clazz = getRawClass(name);
    if (IS_ARRAY_CLASS(clazz) ||
        ((INSTANCE_CLASS)clazz)->status >= CLASS_READY) {
        CurrentThread->pendingException = (char*)name;
        CurrentThread->exceptionMessage = (char*)message;
        return KNI_OK;
    } else {
        return KNI_ERR;
    }
}

/*=========================================================================
 * FUNCTION:       KNI_FatalError()
 * OVERVIEW:       Print an error message and stop the VM immediately.
 * INTERFACE:
 *   parameter(s): message: an error message
 *   returns:      <nothing> (the function does not return)
 *=======================================================================*/

void KNI_FatalError(const char* message) {
    fatalError(message);
}

/*=========================================================================
 * Object operations
 *=======================================================================*/

/*=======================================================================
 * FUNCTION:       KNI_GetObjectClass()
 * OVERVIEW:       Returns the class of a given instance.
 * INTERFACE:
 *   parameter(s): objectHandle: a handle to an object
 *                 classHandle: another handle to which the
 *                 class pointer will be assigned
 *   returns:      Nothing directly, but the handle 'classHandle'
 *                 will contain the class pointer that was found.
 *=======================================================================*/

void KNI_GetObjectClass(jobject objectHandle, jclass classHandle) {
    INSTANCE object = (INSTANCE)KNI_UNHAND(objectHandle);
    KNI_SETHAND(classHandle, object->ofClass);
}

/*=========================================================================
 * FUNCTION:       KNI_IsInstanceOf()
 * OVERVIEW:       Determines whether an object is an instance of 
 *                 the given class or interface.
 * INTERFACE:
 *   parameter(s): An pointer to an instance object and a pointer 
 *                 to a class object
 *   returns:      true or false
 *=======================================================================*/

jboolean KNI_IsInstanceOf(jobject objectHandle, jclass classHandle) {
    INSTANCE object = (INSTANCE)KNI_UNHAND(objectHandle);
    CLASS    class1 = (CLASS)object->ofClass;
    CLASS    class2 = (CLASS)KNI_UNHAND(classHandle);
    return isAssignableToFast(class1, class2);
}

/*=========================================================================
 * Instance field access
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:       KNI_GetFieldID()
 * OVERVIEW:       Get the fieldID of the field with the given
 *                 name and signature.  This operation is used before
 *                 accessing the instance variables of an object.
 * INTERFACE:
 *   parameter(s): - classHandle: handle to a class
 *                 - name: UTF8 string containing the name of the field.
 *                 - signature: UTF8 string containing the signature
 *                   of the field (using the standard Java field
 *                   signature format, e.g., "I", "Ljava/lang/Object;")
 *   returns:      - A jfieldID
 *=======================================================================*/

jfieldID
KNI_GetFieldID(jclass classHandle, const char* name, const char* signature) {
    FIELD field;
    INSTANCE_CLASS clazz = (INSTANCE_CLASS)KNI_UNHAND(classHandle);
    if (clazz == 0) return NULL;
    field = lookupField(clazz, getNameAndTypeKey(name, signature));
    return (jfieldID)(field == NULL || field->accessFlags & ACC_STATIC)
                     ? NULL : field;
}

/*=======================================================================
 * FUNCTION:       KNI_Get<Type>Field()
 * OVERVIEW:       Return the value of a primitive instance field.
 *                 No type checking is performed.
 * INTERFACE:
 *   parameter(s): objectHandle: handle to an object
 *                 fid: a fieldID
 *   returns:      The value of the field
 *=======================================================================*/

jboolean KNI_GetBooleanField(jobject objectHandle, jfieldID fid) {
    INSTANCE object = (INSTANCE)KNI_UNHAND(objectHandle);
    if (INCLUDEDEBUGCODE && (object == 0 || fid == 0)) return 0;
    return (jboolean)(object->data[fid->u.offset].cell);
}

jbyte KNI_GetByteField(jobject objectHandle, jfieldID fid) {
    INSTANCE object = (INSTANCE)KNI_UNHAND(objectHandle);
    if (INCLUDEDEBUGCODE && (object == 0 || fid == 0)) return 0;
    return (jbyte)(object->data[fid->u.offset].cell);
}

jchar KNI_GetCharField(jobject objectHandle, jfieldID fid) {
    INSTANCE object = (INSTANCE)KNI_UNHAND(objectHandle);
    if (INCLUDEDEBUGCODE && (object == 0 || fid == 0)) return 0;
    return (jchar)(object->data[fid->u.offset].cell);
}

jshort KNI_GetShortField(jobject objectHandle, jfieldID fid) {
    INSTANCE object = (INSTANCE)KNI_UNHAND(objectHandle);
    if (INCLUDEDEBUGCODE && (object == 0 || fid == 0)) return 0;
    return (jshort)(object->data[fid->u.offset].cell);
}

jint KNI_GetIntField(jobject objectHandle, jfieldID fid) {
    INSTANCE object = (INSTANCE)KNI_UNHAND(objectHandle);
    if (object == 0 || fid == 0) return 0;
    return (jint)(object->data[fid->u.offset].cell);
}

jlong KNI_GetLongField(jobject objectHandle, jfieldID fid) {
    INSTANCE object = (INSTANCE)KNI_UNHAND(objectHandle);
    union {
        jlong j; /* Platform dependent */
        cell v[2];
    } tmp;
    int offset = fid->u.offset;

    tmp.v[0] = object->data[offset+0].cell;
    tmp.v[1] = object->data[offset+1].cell;
    return tmp.j;
}

#if IMPLEMENTS_FLOAT

jfloat KNI_GetFloatField(jobject objectHandle, jfieldID fid) {
    INSTANCE object = (INSTANCE)KNI_UNHAND(objectHandle);
    if (INCLUDEDEBUGCODE && (object == 0 || fid == 0)) return 0;
    return *(jfloat*)&(object->data[fid->u.offset].cell);
}

jdouble KNI_GetDoubleField(jobject objectHandle, jfieldID fid) {
    INSTANCE object = (INSTANCE)KNI_UNHAND(objectHandle);
    union {
        jdouble d; /* Platform dependent */
        cell v[2];
    } tmp;
    int offset = fid->u.offset;

    tmp.v[0] = object->data[offset+0].cell;
    tmp.v[1] = object->data[offset+1].cell;
    return tmp.d;
}

#endif /* IMPLEMENTS_FLOAT */

/*=======================================================================
 * FUNCTION:       KNI_GetObjectField()
 * OVERVIEW:       Return the value of an instance field 
 *                 containing an object. No type checking is
 *                 performed.
 * INTERFACE:
 *   parameter(s): objectHandle: handle to an object whose field 
 *                 is to be accessed
 *                 fid: a fieldID
 *                 toHandle: handle to which the value is to 
 *                 be assigned
 *   returns:      nothing directly, but 'toHandle' will contain
 *                 the value of the field
 *=======================================================================*/

void KNI_GetObjectField(jobject objectHandle, jfieldID fid, jobject toHandle) {
    INSTANCE object = (INSTANCE)KNI_UNHAND(objectHandle);
    if (INCLUDEDEBUGCODE && (object == 0 || fid == 0)) return;
    KNI_SETHAND(toHandle, object->data[fid->u.offset].cell);
}

/*=======================================================================
 * FUNCTION:       KNI_Set<Type>Field()
 * OVERVIEW:       Set the value of a primitive instance field.
 *                 No type checking is performed.
 * INTERFACE:
 *   parameter(s): objectHandle: handle to an object whose field
 *                 is to be changed.
 *                 fid: a fieldID
 *                 value: the value to be assigned
 *   returns:      <nothing>
 *=======================================================================*/

void KNI_SetBooleanField(jobject objectHandle, jfieldID fid, jboolean value) {
    INSTANCE object = (INSTANCE)KNI_UNHAND(objectHandle);
    if (INCLUDEDEBUGCODE && (object == 0 || fid == 0)) return;
    object->data[fid->u.offset].cell = value;
}

void KNI_SetByteField(jobject objectHandle, jfieldID fid, jbyte value) {
    INSTANCE object = (INSTANCE)KNI_UNHAND(objectHandle);
    if (INCLUDEDEBUGCODE && (object == 0 || fid == 0)) return;
    object->data[fid->u.offset].cell = value;
}

void KNI_SetCharField(jobject objectHandle, jfieldID fid, jchar value) {
    INSTANCE object = (INSTANCE)KNI_UNHAND(objectHandle);
    if (INCLUDEDEBUGCODE && (object == 0 || fid == 0)) return;
    object->data[fid->u.offset].cell = value;
}

void KNI_SetShortField(jobject objectHandle, jfieldID fid, jshort value) {
    INSTANCE object = (INSTANCE)KNI_UNHAND(objectHandle);
    if (INCLUDEDEBUGCODE && (object == 0 || fid == 0)) return;
    object->data[fid->u.offset].cell = value;
}

void KNI_SetIntField(jobject objectHandle, jfieldID fid, jint value) {
    INSTANCE object = (INSTANCE)KNI_UNHAND(objectHandle);
    if (object == 0 || fid == 0) return;
    object->data[fid->u.offset].cell = value;
}

void KNI_SetLongField(jobject objectHandle, jfieldID fid, jlong value) {
    INSTANCE object = (INSTANCE)KNI_UNHAND(objectHandle);
    union {
        jlong j; /* Platform dependent */
        cell v[2];
    } tmp;
    int offset = fid->u.offset;

    tmp.j = value;
    object->data[offset+0].cell = tmp.v[0];
    object->data[offset+1].cell = tmp.v[1];
}

#if IMPLEMENTS_FLOAT

void KNI_SetFloatField(jobject objectHandle, jfieldID fid, jfloat value) {
    INSTANCE object = (INSTANCE)KNI_UNHAND(objectHandle);
    if (INCLUDEDEBUGCODE && (object == 0 || fid == 0)) return;
    *(jfloat*)&object->data[fid->u.offset].cell = value;
}

void KNI_SetDoubleField(jobject objectHandle, jfieldID fid, jdouble value) {
    INSTANCE object = (INSTANCE)KNI_UNHAND(objectHandle);
    union {
        jdouble d; /* Platform dependent */
        cell v[2];
    } tmp;
    int offset = fid->u.offset;

    tmp.d = value;
    object->data[offset+0].cell = tmp.v[0];
    object->data[offset+1].cell = tmp.v[1];
}

#endif /* IMPLEMENTS_FLOAT */

/*=======================================================================
 * FUNCTION:       KNI_SetObjectField()
 * OVERVIEW:       Set the value of an instance field containing
 *                 an object.  No type checking is performed.
 * INTERFACE:
 *   parameter(s): objectHandle: a handle to an object whose field
 *                 is to be changed.
 *                 fid: a fieldID
 *                 fromHandle: a handle to an object that is to be
 *                 to be assigned to 'toHandle'.
 *   returns:      <nothing>
 *=======================================================================*/

void KNI_SetObjectField(jobject objectHandle, jfieldID fid, jobject fromHandle) {
    INSTANCE object = (INSTANCE)KNI_UNHAND(objectHandle);
    if (INCLUDEDEBUGCODE && (object == 0 || fid == 0)) return;
    object->data[fid->u.offset].cell = (cell)KNI_UNHAND(fromHandle);
}

/*=========================================================================
 * Static field access
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:       KNI_GetStaticFieldID()
 * OVERVIEW:       Get the fieldID of the static field with the given
 *                 name and signature.  This operation is used before
 *                 accessing the static variables of an object.
 * INTERFACE:
 *   parameter(s): - classHandle: handle to a class
 *                 - name: UTF8 string containing the name of the field.
 *                 - signature: UTF8 string containing the signature
 *                   of the field (using the standard Java field
 *                   signature format, e.g., "I", "Ljava/lang/Object;")
 *   returns:      - A jfieldID
 *=======================================================================*/

jfieldID
KNI_GetStaticFieldID(jclass classHandle, const char* name, const char* signature) {
    FIELD field;
    INSTANCE_CLASS clazz = (INSTANCE_CLASS)KNI_UNHAND(classHandle);
    if (clazz == 0) return NULL;
    field = lookupField(clazz, getNameAndTypeKey(name, signature));
    return (jfieldID)(field == NULL || field->accessFlags & ACC_STATIC)
                     ? field : NULL;
}

/*=======================================================================
 * FUNCTION:       KNI_GetStatic<Type>Field()
 * OVERVIEW:       Return the value of a primitive static field.
 *                 No type checking is performed.
 * INTERFACE:
 *   parameter(s): classHandle: handle to an class
 *                 fid: a fieldID
 *   returns:      The value of the field
 * NOTE:           The KVM implementation of KNI does not need the
 *                 'classHandle' parameter.
 *=======================================================================*/

jboolean
KNI_GetStaticBooleanField(jclass classHandle, jfieldID fid) {
    if (INCLUDEDEBUGCODE && fid == 0) return 0;
    return *((jint*)fid->u.staticAddress);
}

jbyte
KNI_GetStaticByteField(jclass classHandle, jfieldID fid) {
    if (INCLUDEDEBUGCODE && fid == 0) return 0;
    return *((jint*)fid->u.staticAddress);
}

jchar
KNI_GetStaticCharField(jclass classHandle, jfieldID fid) {
    if (INCLUDEDEBUGCODE && fid == 0) return 0;
    return *((jint*)fid->u.staticAddress);
}

jshort
KNI_GetStaticShortField(jclass classHandle, jfieldID fid) {
    if (INCLUDEDEBUGCODE && fid == 0) return 0;
    return *((jint*)fid->u.staticAddress);
}

jint
KNI_GetStaticIntField(jclass classHandle, jfieldID fid) {
    if (INCLUDEDEBUGCODE && fid == 0) return 0;
    return *((jint*)fid->u.staticAddress);
}

jlong
KNI_GetStaticLongField(jclass classHandle, jfieldID fid) {
    union {
        jlong j; /* Platform dependent */
        cell v[2];
    } tmp;

    tmp.v[0] = *((cell*)fid->u.staticAddress);
    tmp.v[1] = *((cell*)fid->u.staticAddress + 1);
    return tmp.j;
}

#if IMPLEMENTS_FLOAT

jfloat
KNI_GetStaticFloatField(jclass classHandle, jfieldID fid) {
    return *((jfloat*)fid->u.staticAddress);
}

jdouble
KNI_GetStaticDoubleField(jclass classHandle, jfieldID fid) {
    union {
        jdouble d; /* Platform dependent */
        cell v[2];
    } tmp;

    tmp.v[0] = *((cell*)fid->u.staticAddress);
    tmp.v[1] = *((cell*)fid->u.staticAddress + 1);
    return tmp.d;
}

#endif /* IMPLEMENTS_FLOAT */

/*=======================================================================
 * FUNCTION:       KNI_GetStaticObjectField()
 * OVERVIEW:       Return the value of a static field 
 *                 containing an object. No type checking is
 *                 performed.
 * INTERFACE:
 *   parameter(s): classHandle: handle to a class whose field 
 *                 is to be accessed
 *                 fid: a fieldID
 *                 toHandle: handle to which the value is to 
 *                 be assigned
 *   returns:      nothing directly, but 'toHandle' will contain
 *                 the value of the field
 * NOTE:           The KVM implementation of KNI does not need the
 *                 'classHandle' parameter.
 *=======================================================================*/

void KNI_GetStaticObjectField(jclass classHandle, jfieldID fid, jobject toHandle) {
    if (INCLUDEDEBUGCODE && fid == 0) return;
    KNI_SETHAND(toHandle, *((cell*)fid->u.staticAddress));
}

/*=======================================================================
 * FUNCTION:       KNI_SetStatic<Type>Field()
 * OVERVIEW:       Set the value of a primitive static field.
 *                 No type checking is performed.
 * INTERFACE:
 *   parameter(s): classHandle: handle to an class
 *                 fid: a fieldID
 *                 value: the value to be assigned
 *   returns:      <nothing>
 * NOTE:           The KVM implementation of KNI does not need the
 *                 'classHandle' parameter.
 *=======================================================================*/

void
KNI_SetStaticBooleanField(jclass classHandle, jfieldID fid, jboolean value) {
    if (INCLUDEDEBUGCODE && fid == 0) return;
    *((jint*)fid->u.staticAddress) = value;
}

void
KNI_SetStaticByteField(jclass classHandle, jfieldID fid, jbyte value) {
    if (INCLUDEDEBUGCODE && fid == 0) return;
    *((jint*)fid->u.staticAddress) = value;
}

void
KNI_SetStaticCharField(jclass classHandle, jfieldID fid, jchar value) {
    if (INCLUDEDEBUGCODE && fid == 0) return;
    *((jint*)fid->u.staticAddress) = value;
}

void
KNI_SetStaticShortField(jclass classHandle, jfieldID fid, jshort value) {
    if (INCLUDEDEBUGCODE && fid == 0) return;
    *((jint*)fid->u.staticAddress) = value;
}

void
KNI_SetStaticIntField(jclass classHandle, jfieldID fid, jint value) {
    if (INCLUDEDEBUGCODE && fid == 0) return;
    *((jint*)fid->u.staticAddress) = value;
}

void
KNI_SetStaticLongField(jclass classHandle, jfieldID fid, jlong value) {
    union {
        jlong j; /* Platform dependent */
        cell v[2];
    } tmp;

    tmp.j = value;
    *((cell*)fid->u.staticAddress    ) = tmp.v[0];
    *((cell*)fid->u.staticAddress + 1) = tmp.v[1];
}

#if IMPLEMENTS_FLOAT

void
KNI_SetStaticFloatField(jclass classHandle, jfieldID fid, jfloat value) {
    if (INCLUDEDEBUGCODE && fid == 0) return;
    *((jfloat*)fid->u.staticAddress) = value;
}

void
KNI_SetStaticDoubleField(jclass classHandle, jfieldID fid, jdouble value) {
    union {
        jdouble d; /* Platform dependent */
        cell v[2];
    } tmp;

    tmp.d = value;
    *((cell*)fid->u.staticAddress    ) = tmp.v[0];
    *((cell*)fid->u.staticAddress + 1) = tmp.v[1];
}

#endif /* IMPLEMENTS_FLOAT */

/*=======================================================================
 * FUNCTION:       KNI_SetStaticObjectField()
 * OVERVIEW:       Set the value of a static field containing
 *                 an object.  No type checking is performed.
 * INTERFACE:
 *   parameter(s): classHandle: a handle to a class whose field
 *                 is to be changed.
 *                 fid: a fieldID
 *                 fromHandle: a handle to an object that is to be
 *                 to be assigned to 'classHandle'.
 *   returns:      <nothing>
 * NOTE:           The KVM implementation of KNI does not need the
 *                 'classHandle' parameter.
 *=======================================================================*/

void KNI_SetStaticObjectField(jclass classHandle, jfieldID fid, jobject fromHandle) {
    if (INCLUDEDEBUGCODE && fid == 0) return;
    *(cell*)fid->u.staticAddress = (cell)KNI_UNHAND(fromHandle);
}

/*=========================================================================
 * String operations
 *=======================================================================*/

/*=======================================================================
 * FUNCTION:       KNI_GetStringLength()
 * OVERVIEW:       Returns the number of Unicode characters in a
 *                 java.lang.String object.  No type checking is
 *                 performed.
 * INTERFACE:
 *   parameter(s): stringHandle: a handle to a java.lang.String object.
 *   returns:      Length of the string as jsize, or -1 if the 
 *                 string handle was empty.
 *=======================================================================*/

jsize KNI_GetStringLength(jstring stringHandle) {
    STRING_INSTANCE string = (STRING_INSTANCE)KNI_UNHAND(stringHandle);
    if (string == 0) return -1;
    return string->length;
}

/*=======================================================================
 * FUNCTION:       KNI_GetStringRegion()
 * OVERVIEW:       Copies a region of the contents of a java.lang.String 
 *                 object into a native buffer.  Characters are returned 
 *                 as Unicode, i.e., each character is 16 bits wide.
 *                 It is the responsibility of the native function
 *                 programmer to allocate the native buffer and to 
 *                 make sure that it is long enough (no buffer
 *                 overflow checking is performed.)
 * INTERFACE:
 *   parameter(s): stringHandle: a handle to a java.lang.String object
 *                 offset: offset from which the copy region begins
 *                 n: the number of characters to copy
 *                 jcharbuf: pointer to a native buffer
 *   returns:      nothing directly, but copies the string region
 *                 to the user-supplied buffer.
 *=======================================================================*/

void KNI_GetStringRegion(jstring stringHandle, jsize offset, jsize n, jchar* jcharbuf) {
    STRING_INSTANCE string = (STRING_INSTANCE)KNI_UNHAND(stringHandle);
    SHORTARRAY stringContents;
    long stroffset;
    long strlength;
    long i;

    if (INCLUDEDEBUGCODE && string == 0) return;

    stringContents = string->array;
    stroffset      = string->offset;
    strlength      = string->length;
    /*
     * Invariants:
     *   1) offset >= 0,
     *   2) n > 0, and
     *   3) offset + n <= strlength
     *
     * Boundary conditions:
     *   1) offset == 0 and n == strlength, and
     *   2) offset == strlength and n == 0
     *
     *   Note: 2) should not return any data, by definition (e.g.,
     *   a region of zero length is just that)
     */
    if (offset >= 0 && n > 0 && (offset + n) <= strlength) {
        for (i = 0; i < n; i++) {
            jcharbuf[i] = ((unsigned short*)stringContents->sdata)[i + stroffset + offset];
        }
    }
}

/*=======================================================================
 * FUNCTION:       KNI_NewString()
 * OVERVIEW:       Creates a java.lang.String object.
 * INTERFACE:
 *   parameter(s): uchars: a Unicode string (each character is 16 bits
 *                 wide, no terminating zero assumed)
 *                 length: the number of characters in the Unicode string
 *                 stringHandle: a handle to which the new string
 *                 object will be assigned.
 *   returns:      nothing directly, but 'stringHandle' will contain
 *                 the new string or NULL if no string could be created.
 * NOTE:           This operation may cause a garbage collection!
 *=======================================================================*/

void KNI_NewString(const jchar* uchars, jsize length, jstring stringHandle) {
    STRING_INSTANCE newString;
    int i;
    int arraySize = SIZEOF_ARRAY((length * 2 + (CELL - 1)) >> log2CELL);

    START_TEMPORARY_ROOTS
        /*  Allocate a character array */
        DECLARE_TEMPORARY_ROOT(SHORTARRAY, newArray, 
            (SHORTARRAY)callocObject(arraySize, GCT_ARRAY));
        newArray->ofClass = PrimitiveArrayClasses[T_CHAR];
        newArray->length  = length;

        /*  Initialize the character array with string contents */
        for (i = 0; i < length; i++) { 
            newArray->sdata[i] = uchars[i];
        }

        /* Allocate a new string object (no temporary roots used, */
        /* so we must not garbage collect after this call) */
        newString = (STRING_INSTANCE)instantiate(JavaLangString);
        newString->offset = 0;
        newString->length = length;
        newString->array  = newArray;
    END_TEMPORARY_ROOTS

    KNI_SETHAND(stringHandle, newString);
}

/*=======================================================================
 * FUNCTION:       KNI_NewStringUTF()
 * OVERVIEW:       Creates a java.lang.String object.
 * INTERFACE:
 *   parameter(s): utf8chars: the string contents as a UTF8 string
 *                 stringHandle: a handle to which the new string
 *                 object will be assigned.
 *   returns:      nothing directly, but 'stringHandle' will contain
 *                 the new string or NULL if no string could be created.
 * NOTE:           This operation may cause a garbage collection!
 *=======================================================================*/

void KNI_NewStringUTF(const char* utf8chars, jstring stringHandle) {
    KNI_SETHAND(stringHandle, instantiateString(utf8chars, strlen(utf8chars)));
}

/*=========================================================================
 * Array operations
 *=======================================================================*/

/*=======================================================================
 * FUNCTION:       KNI_GetArrayLength()
 * OVERVIEW:       Returns the number of elements in an array
 * INTERFACE:
 *   parameter(s): arrayHandle: a handle to an array object.
 *   returns:      Length of the array as jsize, or -1 if the 
 *                 array handle was empty.
 *=======================================================================*/

jsize KNI_GetArrayLength(jstring arrayHandle) {
    ARRAY array = (ARRAY)KNI_UNHAND(arrayHandle);
    if (array == 0) return -1;
    return array->length;
}

/*=========================================================================
 * FUNCTION:       KNI_Get<Type>ArrayElement()
 * OVERVIEW:       Return an element of a primitive array.
 *                 No type checking is performed.
 * INTERFACE:
 *   parameter(s): arrayHandle: a handle to a primitive array
 *                 index: the index of the array element to read
 *   returns:      The value of the requested element in the array.
 *=======================================================================*/

jboolean KNI_GetBooleanArrayElement(jbooleanArray arrayHandle, jint index) {
    BYTEARRAY array = (BYTEARRAY)KNI_UNHAND(arrayHandle);
    if (INCLUDEDEBUGCODE && array == 0) return 0;
    return (jboolean)array->bdata[index];
}

jbyte KNI_GetByteArrayElement(jbyteArray arrayHandle, jint index) {
    BYTEARRAY array = (BYTEARRAY)KNI_UNHAND(arrayHandle);
    if (INCLUDEDEBUGCODE && array == 0) return 0;
    return (jbyte)array->bdata[index];
}

jchar KNI_GetCharArrayElement(jcharArray arrayHandle, jint index) {
    SHORTARRAY array = (SHORTARRAY)KNI_UNHAND(arrayHandle);
    if (INCLUDEDEBUGCODE && array == 0) return 0;
    return (jchar)array->sdata[index];
}

jshort KNI_GetShortArrayElement(jshortArray arrayHandle, jint index) {
    SHORTARRAY array = (SHORTARRAY)KNI_UNHAND(arrayHandle);
    if (INCLUDEDEBUGCODE && array == 0) return 0;
    return (jshort)array->sdata[index];
}

jint KNI_GetIntArrayElement(jintArray arrayHandle, jint index) {
    ARRAY array = (ARRAY)KNI_UNHAND(arrayHandle);
    if (INCLUDEDEBUGCODE && array == 0) return 0;
    return (jint)array->data[index].cell;
}

jlong KNI_GetLongArrayElement(jlongArray arrayHandle, jint index) {
    ARRAY array = (ARRAY)KNI_UNHAND(arrayHandle);
    union {
        jlong j; /* Platform dependent */
        cell v[2];
    } tmp;
    int offset = index*2;

    tmp.v[0] = array->data[offset  ].cell;
    tmp.v[1] = array->data[offset+1].cell;
    return tmp.j;
}

#if IMPLEMENTS_FLOAT

jfloat KNI_GetFloatArrayElement(jfloatArray arrayHandle, jint index) {
    ARRAY array = (ARRAY)KNI_UNHAND(arrayHandle);
    if (INCLUDEDEBUGCODE && array == 0) return 0;
    return *((jfloat*)&array->data[index].cell);
}

jdouble KNI_GetDoubleArrayElement(jdoubleArray arrayHandle, jint index) {
    ARRAY array = (ARRAY)KNI_UNHAND(arrayHandle);
    union {
        jdouble d; /* Platform dependent */
        cell v[2];
    } tmp;
    int offset = index*2;

    tmp.v[0] = array->data[offset  ].cell;
    tmp.v[1] = array->data[offset+1].cell;
    return tmp.d;
}

#endif /* IMPLEMENTS_FLOAT */

/*=========================================================================
 * FUNCTION:       KNI_GetObjectArrayElement()
 * OVERVIEW:       Return an element of an object array.
 *                 No type checking is performed.
 * INTERFACE:
 *   parameter(s): arrayHandle: a handle to a primitive array
 *                 index: the index of the array element to read
 *                 toHandle: a handle to an object to which 
 *                 the return value will be assigned.
 *   returns:      Nothing directly, but assigns the array
 *                 element to 'objectHandle'.
 *=======================================================================*/

void
KNI_GetObjectArrayElement(jobjectArray arrayHandle, jint index, jobject toHandle) {
    ARRAY array = (ARRAY)KNI_UNHAND(arrayHandle);
    if (INCLUDEDEBUGCODE && array == 0) return;
    KNI_SETHAND(toHandle, array->data[index].cell);
}

/*=========================================================================
 * FUNCTION:       KNI_Set<Type>ArrayElement()
 * OVERVIEW:       Set the element of a primitive array.
 *                 No type checking is performed.
 * INTERFACE:
 *   parameter(s): objectArray: a handle to a primitive array
 *                 index: the index of the array element
 *                 value: the value that will be stored
 *   returns:      <nothing>
 *=======================================================================*/

void
KNI_SetBooleanArrayElement(jbooleanArray arrayHandle, jint index, jboolean value) {
    BYTEARRAY array = (BYTEARRAY)KNI_UNHAND(arrayHandle);
    if (INCLUDEDEBUGCODE && array == 0) return;
    array->bdata[index] = value;
}

void KNI_SetByteArrayElement(jbyteArray arrayHandle, jint index, jbyte value) {
    BYTEARRAY array = (BYTEARRAY)KNI_UNHAND(arrayHandle);
    if (INCLUDEDEBUGCODE && array == 0) return;
    array->bdata[index] = value;
}

void KNI_SetCharArrayElement(jcharArray arrayHandle, jint index, jchar value) {
    SHORTARRAY array = (SHORTARRAY)KNI_UNHAND(arrayHandle);
    if (INCLUDEDEBUGCODE && array == 0) return;
    array->sdata[index] = value;
}

void KNI_SetShortArrayElement(jshortArray arrayHandle, jint index, jshort value) {
    SHORTARRAY array = (SHORTARRAY)KNI_UNHAND(arrayHandle);
    if (INCLUDEDEBUGCODE && array == 0) return;
    array->sdata[index] = value;
}

void KNI_SetIntArrayElement(jintArray arrayHandle, jint index, jint value) {
    ARRAY array = (ARRAY)KNI_UNHAND(arrayHandle);
    if (INCLUDEDEBUGCODE && array == 0) return;
    array->data[index].cell = value;
}

void KNI_SetLongArrayElement(jlongArray arrayHandle, jint index, jlong value) {
    ARRAY array = (ARRAY)KNI_UNHAND(arrayHandle);
    union {
        jlong j; /* Platform dependent */
        cell v[2];
    } tmp;
    int offset = index*2;

    tmp.j = value;
    array->data[offset  ].cell = tmp.v[0];
    array->data[offset+1].cell = tmp.v[1];
}

#if IMPLEMENTS_FLOAT

void KNI_SetFloatArrayElement(jfloatArray arrayHandle, jint index, jfloat value) {
    ARRAY array = (ARRAY)KNI_UNHAND(arrayHandle);
    if (INCLUDEDEBUGCODE && array == 0) return;
    *(jfloat*)&array->data[index].cell = value;
}

void KNI_SetDoubleArrayElement(jdoubleArray arrayHandle, jint index, jdouble value) {
    ARRAY array = (ARRAY)KNI_UNHAND(arrayHandle);
    union {
        jdouble d; /* Platform dependent */
        cell v[2];
    } tmp;
    int offset = index*2;

    tmp.d = value;
    array->data[offset  ].cell = tmp.v[0];
    array->data[offset+1].cell = tmp.v[1];
}

#endif /* IMPLEMENTS_FLOAT */

/*=======================================================================
 * FUNCTION:       KNI_SetObjectArrayElement()
 * OVERVIEW:       Set the element of an object array.
 *                 No type checking is performed.
 * INTERFACE:
 *   parameter(s): arrayHandle: a handle to a primitive array
 *                 index: the index of the array element
 *                 fromHandle: a handle to an object that will
 *                 be stored to the array
 *   returns:      <nothing>
 *=======================================================================*/

void
KNI_SetObjectArrayElement(jobjectArray arrayHandle, jint index, jobject fromHandle) {
    ARRAY array = (ARRAY)KNI_UNHAND(arrayHandle);
    if (INCLUDEDEBUGCODE && array == 0) return;
    array->data[index].cell = (cell)KNI_UNHAND(fromHandle);
}

/*=======================================================================
 * FUNCTION:       KNI_GetRawArrayRegion()
 * OVERVIEW:       Copies a region of the contents of an array
 *                 into a native buffer as raw bytes.  No type
 *                 checking is performed (the actual type of the
 *                 array is ignored).  It is the responsibility
 *                 of the native function programmer to allocate
 *                 the native buffer and to make sure that it is
 *                 long enough (no buffer overflow checking is
 *                 performed).
 * INTERFACE:
 *   parameter(s): arrayHandle: a handle to an array object
 *                 offset: offset from which the copy region begins
 *                 n: the number of bytes to copy
 *                 dstBuffer: pointer to a native buffer to which
 *                 data will be copied
 *   returns:      nothing directly, but copies the array region
 *                 to the user-supplied buffer.
 *=======================================================================*/

void
KNI_GetRawArrayRegion(jarray arrayHandle, jsize offset, jsize n, jbyte* dstBuffer) {
    BYTEARRAY barray = (BYTEARRAY)KNI_UNHAND(arrayHandle);
    if (INCLUDEDEBUGCODE && (barray == 0 || dstBuffer == 0)) return;
    (void)memcpy(dstBuffer, &barray->bdata[offset], n);
}

/*=======================================================================
 * FUNCTION:       KNI_SetRawArrayRegion()
 * OVERVIEW:       Set the contents of a region in a Java array object.
 *                 The data is copied as bytes from a native buffer
 *                 provided by the native function programmer.  No
 *                 type or range checking is performed.
 * INTERFACE:
 *   parameter(s): arrayHandle: a handle to an array object
 *                 offset: offset inside the array to which
 *                 the data will be copied
 *                 n: the number of bytes to copy
 *                 srcBuffer: pointer to a native buffer from which
 *   returns:      <nothing>
 *=======================================================================*/

void
KNI_SetRawArrayRegion(jarray arrayHandle, jsize offset, jsize n, const jbyte* srcBuffer) {
    BYTEARRAY barray = (BYTEARRAY)KNI_UNHAND(arrayHandle);
    if (INCLUDEDEBUGCODE && (barray == 0 || srcBuffer == 0)) return;
    (void)memcpy(&barray->bdata[offset], srcBuffer, n);
}

/*=========================================================================
 * Operand stack (parameter) access
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:       KNI_GetParameterAs<Type>()
 * OVERVIEW:       Returns the primitive value at the given index on
 *                 the operand stack. Indexing of parameters starts
 *                 from 1. Note that long and double parameters take
 *                 up two elements on the stack.  No range or type 
 *                 checking is performed.
 * INTERFACE:
 *   parameter(s): index: The index of a parameter
 *   returns:      value representing a primitive type
 * 
 * IMPORTANT       long and double elements take up two elements
 *                 on the operand stack. For example, when calling
 *                 the following native function
 *
 *                     native void foo(int a, long b, int c);
 *
 *                 the index of parameter 'a' would be 1, 
 *                 the index of parameter 'b' would be 2, and
 *                 the index of parameter 'c' would be 4.
 *=======================================================================*/

jboolean KNI_GetParameterAsBoolean(jint index) {
    return (jboolean)CurrentThread->nativeLp[index];
}

jbyte KNI_GetParameterAsByte(jint index) {
    return (jbyte)CurrentThread->nativeLp[index];
}

jchar KNI_GetParameterAsChar(jint index) {
    return (jchar)CurrentThread->nativeLp[index];
}

jshort KNI_GetParameterAsShort(jint index) {
    return (jshort)CurrentThread->nativeLp[index];
}

jint KNI_GetParameterAsInt(jint index) {
    return (jint)CurrentThread->nativeLp[index];
}

jlong KNI_GetParameterAsLong(jint index) {
    jlong value;
    COPY_LONG(&value, &CurrentThread->nativeLp[index]);
    return value;
}

#if IMPLEMENTS_FLOAT

jfloat KNI_GetParameterAsFloat(jint index) {
    return *(jfloat*)&CurrentThread->nativeLp[index];
}

jdouble KNI_GetParameterAsDouble(jint index) {
    jdouble value;
    COPY_DOUBLE(&value, &CurrentThread->nativeLp[index]);
    return value;
}

#endif /* IMPLEMENTS_FLOAT */

/*=========================================================================
 * FUNCTION:       KNI_GetParameterAsObject()
 * OVERVIEW:       Returns the jobject at the given index on the 
 *                 operand stack. Indexing of parameters starts
 *                 from 1. No range or type checking is performed.
 * INTERFACE:
 *   parameter(s): - index: The index of a parameter
 *                 - toHandle: handle to which the return value
 *                   will be assigned
 *   returns:      - nothing directly, but toHandle will contain
 *                   the return value.
 *=======================================================================*/

void KNI_GetParameterAsObject(jint index, jobject toHandle) {
    KNI_SETHAND(toHandle, CurrentThread->nativeLp[index]);
}

/*=========================================================================
 * FUNCTION:       KNI_GetThisPointer()
 * OVERVIEW:       Returns the 'this' pointer of an instance method.
 *                 The value of this function is undefined for static
 *                 methods.
 * INTERFACE:
 *   parameter(s): toHandle: handle to which the return value
 *                 will be assigned
 *   return:       nothing directly, but toHandle will contain
 *                 the return value.
 *=======================================================================*/

void KNI_GetThisPointer(jobject toHandle) {
    KNI_SETHAND(toHandle, CurrentThread->nativeLp[0]);
}

/*=========================================================================
 * FUNCTION:       KNI_GetClassPointer()
 * OVERVIEW:       Returns the class pointer (as a handle) of the 
 *                 class of the currently executing native method.
 * INTERFACE:
 *   parameter(s): toHandle: handle to which the return value
 *                 will be assigned
 *   return:       nothing directly, but toHandle will contain
 *                 the return value.
 *=======================================================================*/

void KNI_GetClassPointer(jobject toHandle) {
    KNI_SETHAND(toHandle, (cell)(getFP()->thisMethod->ofClass));
}
