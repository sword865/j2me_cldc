/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Constant pool
 * FILE:      pool.c
 * OVERVIEW:  Constant pool management operations (see pool.h).
 * AUTHOR:    Antero Taivalsaari, Sun Labs, 1998
 *            Frank Yellin, Sheng Liang (tightened the access 
 *            checks for JLS (Java Language Spec compliance)
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>

/*=========================================================================
 * Constant pool access methods (low level)
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      cachePoolEntry()
 * TYPE:          private instance-level operation
 * OVERVIEW:      Given an index to a pool entry, and a resolved value
 *                update that entry to have the given value.
 * INTERFACE:
 *   parameters:  constant pool pointer, constant pool index, resolved value 
 *   returns:     <nothing>
 *=======================================================================*/

static void 
cachePoolEntry(CONSTANTPOOL constantPool, unsigned int cpIndex, cell* value)
{
    CONSTANTPOOL_ENTRY thisEntry = &constantPool->entries[cpIndex];

    if (!USESTATIC || inCurrentHeap(constantPool)) { 
        /* For !USESTATIC, the constant pool is always in the heap.
         * For USESTATIC, the constant pool can be in the heap when we're 
         *    setting the initial value of a final static String.
         */
        CONSTANTPOOL_TAG(constantPool, cpIndex) |= CP_CACHEBIT;
        thisEntry->cache = value;
    } else { 
        unsigned char *thisTagAddress = &CONSTANTPOOL_TAG(constantPool, cpIndex);
        int entryOffset = (char *)thisEntry - (char *)constantPool;
        int tagOffset =   (char *)thisTagAddress - (char *)constantPool;
        
        unsigned char                 newTag = *thisTagAddress | CP_CACHEBIT;
        union constantPoolEntryStruct newEntry;
        newEntry.cache = value;

        modifyStaticMemory(constantPool, entryOffset, &newEntry, sizeof(newEntry));
        modifyStaticMemory(constantPool, tagOffset,   &newTag,   sizeof(newTag));
    }
}

/*=========================================================================
 * FUNCTION:      verifyClassAccess
 *                classHasAccessToClass
 *                classHasAccessToMember
 * TYPE:          public instance-level operation
 * OVERVIEW:      Helper functions for checking access rights
 *                in field/method resolution.
 * INTERFACE:
 *   parameters:
 *   returns:
 *   throws:      IllegalAccessError on failure
 *                (this class is not part of CLDC 1.0 or 1.1)
 *=======================================================================*/

void verifyClassAccess(CLASS targetClass, INSTANCE_CLASS currentClass) 
{
    if (!classHasAccessToClass(currentClass, targetClass)) { 
        START_TEMPORARY_ROOTS
            DECLARE_TEMPORARY_ROOT(char *, targetName, 
                                   getClassName((CLASS)targetClass));
            DECLARE_TEMPORARY_ROOT(char *, currentName, 
                                   getClassName((CLASS)currentClass));

            sprintf(str_buffer, 
                    KVM_MSG_CANNOT_ACCESS_CLASS_FROM_CLASS_2STRPARAMS,
                    targetName, currentName);
        END_TEMPORARY_ROOTS
        raiseExceptionWithMessage(IllegalAccessError, str_buffer);
    }
}

bool_t
classHasAccessToClass(INSTANCE_CLASS currentClass, CLASS targetClass) { 
    if (    currentClass == NULL 
        || ((CLASS)currentClass == targetClass)
        /* Note that array classes have the same package and access as
         * their base classes */
        || (targetClass->accessFlags & ACC_PUBLIC)
        || (targetClass->packageName == currentClass->clazz.packageName)
        ) { 
        return TRUE;
    } else { 
        return FALSE;
    }
}

bool_t
classHasAccessToMember(INSTANCE_CLASS currentClass, 
                       int access, INSTANCE_CLASS fieldClass,
                       INSTANCE_CLASS cpClass) { 
    if (   currentClass == NULL 
        || currentClass == fieldClass 
        || (ACC_PUBLIC & access)) {
        return TRUE;
    } else if (ACC_PRIVATE & access) { 
        return FALSE;
    } else if (currentClass->clazz.packageName == fieldClass->clazz.packageName) {
        return TRUE;
    } else if (ACC_PROTECTED & access) { 
        /* See if currentClass is a subclass of fieldClass */
        INSTANCE_CLASS cb;
        for (cb = currentClass->superClass; ; cb = cb->superClass) {
            if (cb == fieldClass) {
                break;
            } else if (cb == NULL) {
                return FALSE;
            }
        }
        /* In addition, cpClass must either be a subclass, superclass or the
         * same as currentClass
         */
        if (cpClass == currentClass) {
            return TRUE;
        }
        for (cb = currentClass->superClass; cb; cb = cb->superClass) {
            if (cb == cpClass) {
                return TRUE;
            }
        }
        for (cb = cpClass; cb; cb = cb->superClass) {
            if (cb == currentClass) {
                return TRUE;
            }
        }

    }
    return FALSE;
}

/*=========================================================================
 * Constant pool access methods (high level)
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      resolveClassReference()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Given an index to a CONSTANT_Class, get the class
 *                that the index refers to.
 * INTERFACE:
 *   parameters:  constant pool pointer, constant pool index, current class
 *   returns:     class pointer
 *=======================================================================*/

CLASS resolveClassReference(CONSTANTPOOL constantPool,
                            unsigned int cpIndex,
                            INSTANCE_CLASS currentClass) {
    CONSTANTPOOL_ENTRY thisEntry = &constantPool->entries[cpIndex];
    unsigned char thisTag = CONSTANTPOOL_TAG(constantPool, cpIndex);
    CLASS thisClass;

    if (thisTag & CP_CACHEBIT) { 
        /*  Check if this entry has already been resolved (cached) */
        /*  If so, simply return the earlier resolved class */
        return (CLASS)(thisEntry->cache);
    }

    if (VERIFYCONSTANTPOOLINTEGRITY && 
        (thisTag & CP_CACHEMASK) != CONSTANT_Class) { 
        raiseExceptionWithMessage(VirtualMachineError, 
            KVM_MSG_ILLEGAL_CONSTANT_CLASS_REFERENCE);
    }
    thisClass = thisEntry->clazz;
    if (IS_ARRAY_CLASS(thisClass)) { 
        loadArrayClass((ARRAY_CLASS)thisClass);
    } else if (((INSTANCE_CLASS)thisClass)->status == CLASS_RAW) { 
        loadClassfile((INSTANCE_CLASS)thisClass, TRUE);
    }
    verifyClassAccess(thisClass, currentClass);
    cachePoolEntry(constantPool, cpIndex, (cell*)thisClass);
    return thisClass;
}

/*=========================================================================
 * FUNCTION:      resolveFieldReference()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Given an index to a CONSTANT_Fieldref, get the Field
 *                that the index refers to.
 * INTERFACE:
 *   parameters:  constant pool pointer, constant pool index, isStatic
 *   returns:     field pointer
 *   throws:      IncompatibleClassChangeError, IllegalAccessError
 *                (these error classes are not supported by CLDC 1.0/1.1)
 *=======================================================================*/

FIELD resolveFieldReference(CONSTANTPOOL constantPool, unsigned int cpIndex, 
                            bool_t isStatic, int opcode,
                            INSTANCE_CLASS currentClass) {
    CONSTANTPOOL_ENTRY thisEntry = &constantPool->entries[cpIndex];
    unsigned char thisTag = CONSTANTPOOL_TAG(constantPool, cpIndex);
    FIELD thisField;
    CLASS thisClass;

    if (thisTag & CP_CACHEBIT) { 
        /*  Check if this entry has already been resolved (cached) */
        /*  If so, simply return the earlier resolved class */
        
        thisField = (FIELD)(thisEntry->cache);
    } else { 
        unsigned short classIndex = thisEntry->method.classIndex;
        unsigned short nameTypeIndex = thisEntry->method.nameTypeIndex;
        
        NameTypeKey nameTypeKey = 
            constantPool->entries[nameTypeIndex].nameTypeKey;

        thisClass = resolveClassReference(constantPool,
                                          classIndex,
                                          currentClass);

        thisField = NULL;
        if (  !IS_ARRAY_CLASS(thisClass) 
            && (((INSTANCE_CLASS)thisClass)->status != CLASS_ERROR)) { 
            thisField = lookupField((INSTANCE_CLASS)thisClass, nameTypeKey);
        }
    }

    /* Cache the value */
    if (thisField != NULL) {
        if (isStatic ? ((thisField->accessFlags & ACC_STATIC) == 0)
                     : ((thisField->accessFlags & ACC_STATIC) != 0)) {
            START_TEMPORARY_ROOTS
                DECLARE_TEMPORARY_ROOT(char *, fieldClassName, 
                    getClassName((CLASS)(thisField->ofClass)));
                sprintf(str_buffer, 
                        KVM_MSG_INCOMPATIBLE_CLASS_CHANGE_2STRPARAMS,
                        fieldClassName, 
                        fieldName(thisField));
            END_TEMPORARY_ROOTS
            raiseExceptionWithMessage(IncompatibleClassChangeError, str_buffer);
        }
        if ((thisField->accessFlags & ACC_FINAL) &&
            (opcode == PUTSTATIC || opcode == PUTFIELD) &&
            (thisField->ofClass != currentClass)) {
            START_TEMPORARY_ROOTS
                DECLARE_TEMPORARY_ROOT(char *, fieldClassName, 
                    getClassName((CLASS)(thisField->ofClass)));
                DECLARE_TEMPORARY_ROOT(char *, currentClassName, 
                    getClassName((CLASS)currentClass));
                    sprintf(str_buffer, 
                        KVM_MSG_CANNOT_MODIFY_FINAL_FIELD_3STRPARAMS,
                        fieldClassName, fieldName(thisField),
                        currentClassName);
            END_TEMPORARY_ROOTS
            raiseExceptionWithMessage(IllegalAccessError, str_buffer);
        }
        if (!(thisTag & CP_CACHEBIT)) { 
            /* Since access only depends on the class, and not on the
             * specific byte code used to access the field, we don't need
             * to perform this check if the constant pool entry
             * has already been resolved.
             */
            if (!classHasAccessToMember(currentClass, 
                                        thisField->accessFlags, 
                                        thisField->ofClass,
                                        (INSTANCE_CLASS)thisClass)) { 
                START_TEMPORARY_ROOTS
                    DECLARE_TEMPORARY_ROOT(char *, fieldClassName, 
                        getClassName((CLASS)(thisField->ofClass)));
                    DECLARE_TEMPORARY_ROOT(char *, currentClassName, 
                        getClassName((CLASS)currentClass));
                    sprintf(str_buffer, 
                            KVM_MSG_CANNOT_ACCESS_MEMBER_FROM_CLASS_3STRPARAMS,
                            fieldClassName, fieldName(thisField), 
                            currentClassName);
                END_TEMPORARY_ROOTS
                raiseExceptionWithMessage(IllegalAccessError, str_buffer);
            }
            cachePoolEntry(constantPool, cpIndex, (cell*)thisField);
        }
    }
    return thisField;
}

/*=========================================================================
 * FUNCTION:      resolveMethodReference()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Given an index to a CONSTANT_Methodref or 
 *                CONSTANT_InterfaceMethodref, get the Method
 *                that the index refers to.
 * INTERFACE:
 *   parameters:  constant pool pointer, constant pool index 
 *   returns:     method pointer
 *   throws:      IncompatibleClassChangeError, IllegalAccessError
 *                (these error classes are not supported by CLDC 1.0/1.1)
 *=======================================================================*/

METHOD resolveMethodReference(CONSTANTPOOL constantPool, unsigned int cpIndex, 
                              bool_t isStatic, INSTANCE_CLASS currentClass) {
    CONSTANTPOOL_ENTRY thisEntry = &constantPool->entries[cpIndex];
    unsigned char thisTag = CONSTANTPOOL_TAG(constantPool, cpIndex);
    if (thisTag & CP_CACHEBIT) { 
        /*  Check if this entry has already been resolved (cached) */
        /*  If so, simply return the earlier resolved class */
        return (METHOD)(thisEntry->cache);
    } else { 
        unsigned short classIndex = thisEntry->method.classIndex;
        unsigned short nameTypeIndex = thisEntry->method.nameTypeIndex;
        
        CLASS thisClass = resolveClassReference(constantPool,
                                                classIndex,
                                                currentClass);
        NameTypeKey nameTypeKey;
        METHOD thisMethod = NULL;

        if (  (((thisTag & CP_CACHEMASK) == CONSTANT_InterfaceMethodref) &&
               !(thisClass->accessFlags & ACC_INTERFACE))
            ||
              (((thisTag & CP_CACHEMASK) == CONSTANT_Methodref) &&
                (thisClass->accessFlags & ACC_INTERFACE))) {
            /* "Bad methodref" */
            raiseExceptionWithMessage(IncompatibleClassChangeError,
                KVM_MSG_BAD_FIELD_OR_METHOD_REFERENCE);
        }

        nameTypeKey = constantPool->entries[nameTypeIndex].nameTypeKey;
        if (IS_ARRAY_CLASS(thisClass) || 
            ((INSTANCE_CLASS)thisClass)->status != CLASS_ERROR) { 
            thisMethod = lookupMethod(thisClass, nameTypeKey, currentClass);
            if (nameTypeKey.nt.nameKey == initNameAndType.nt.nameKey) { 
                if (thisMethod != NULL 
                    && thisMethod->ofClass != (INSTANCE_CLASS)thisClass) { 
                    thisMethod = NULL;
                }
            }
        }
        if (thisMethod) {
            if (isStatic ? ((thisMethod->accessFlags & ACC_STATIC) == 0)
                         : ((thisMethod->accessFlags & ACC_STATIC) != 0)) {
                START_TEMPORARY_ROOTS
                    DECLARE_TEMPORARY_ROOT(char*, methodClassName, 
                        getClassName((CLASS)thisMethod->ofClass));
                    DECLARE_TEMPORARY_ROOT(char*, methodSignature, 
                        getMethodSignature(thisMethod));
                    sprintf(str_buffer, 
                            KVM_MSG_INCOMPATIBLE_CLASS_CHANGE_3STRPARAMS,
                            methodClassName, 
                            methodName(thisMethod), methodSignature);
                END_TEMPORARY_ROOTS
                raiseExceptionWithMessage(IncompatibleClassChangeError, str_buffer);
            }
            if (!classHasAccessToMember(currentClass, thisMethod->accessFlags, 
                                        thisMethod->ofClass,
                                        (INSTANCE_CLASS)thisClass)) { 
                START_TEMPORARY_ROOTS
                    DECLARE_TEMPORARY_ROOT(char*, className, 
                        getClassName((CLASS)currentClass));
                    DECLARE_TEMPORARY_ROOT(char*, methodClassName, 
                        getClassName((CLASS)thisMethod->ofClass));
                    sprintf(str_buffer, 
                        KVM_MSG_CANNOT_ACCESS_MEMBER_FROM_CLASS_3STRPARAMS,
                        methodClassName, methodName(thisMethod), 
                        className);
                END_TEMPORARY_ROOTS
                raiseExceptionWithMessage(IllegalAccessError, str_buffer);
            }
            cachePoolEntry(constantPool, cpIndex, (cell*)thisMethod);
        } else { 
            /* Some appropriate error message in debug mode */
        }
        return thisMethod;
    }
}

