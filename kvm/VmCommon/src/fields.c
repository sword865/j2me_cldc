/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Internal runtime structures
 * FILE:      fields.c
 * OVERVIEW:  Internal runtime structures for storing different kinds 
 *            of fields (constants, variables, methods, interfaces).
 *            Tables of these fields are created every time a new class
 *            is loaded into the virtual machine.
 * AUTHOR:    Antero Taivalsaari, Sun Labs, 1998
 *            Edited by Doug Simon 11/1998
 *            Sheng Liang, Frank Yellin
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>

/*=========================================================================
 * Static variables and definitions (specific to this file)
 *=======================================================================*/

static void change_Key_to_MethodSignatureInternal(unsigned char**, char**);
static void change_MethodSignature_to_KeyInternal(CONST_CHAR_HANDLE, 
                                                  int *offsetP, 
                                                  unsigned char **to);

/*=========================================================================
 * Operations on field and method tables
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      lookupField()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Find a field table entry with the given field name.
 * INTERFACE:
 *   parameters:  class pointer, field name pointer
 *   returns:     pointer to the field or NIL
 *=======================================================================*/

FIELD lookupField(INSTANCE_CLASS thisClass, NameTypeKey key) { 
    do { 
        FIELDTABLE fieldTable = thisClass->fieldTable;
        /*  Get the length of the field table */
        FOR_EACH_FIELD(thisField, fieldTable) 
            if (thisField->nameTypeKey.i == key.i) { 
                return thisField;
            }
        END_FOR_EACH_FIELD
            thisClass = thisClass->superClass;
    } while (thisClass != NULL);
    return NULL;
}

/*=========================================================================
 * FUNCTION:      lookupMethod()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Find a method table entry with the given name and type
 *                using a simple linear lookup strategy.
 * INTERFACE:
 *   parameters:  class pointer, method name and signature pointers
 *   returns:     pointer to the method or NIL
 *
 * NOTES:         This is a naive implementation with linear search.
 *                In most cases this does not matter, however, since
 *                inline caching (turning ENABLEFASTBYTECODES on) 
 *                allows us to avoid the method lookup overhead.
 *=======================================================================*/

METHOD lookupMethod(CLASS thisClass, NameTypeKey key, 
                    INSTANCE_CLASS currentClass)
{
    INSTANCE_CLASS thisInstanceClass = 
        IS_ARRAY_CLASS(thisClass) ? JavaLangObject : (INSTANCE_CLASS)thisClass;
    do {
        METHODTABLE methodTable = thisInstanceClass->methodTable;
        FOR_EACH_METHOD(thisMethod, methodTable) 
            if (thisMethod->nameTypeKey.i == key.i) { 
                if (   currentClass == NULL 
                    || currentClass == thisInstanceClass
                    || ((ACC_PUBLIC|ACC_PROTECTED) & thisMethod->accessFlags)
                    || ( ((thisMethod->accessFlags & ACC_PRIVATE) == 0)
                    && thisInstanceClass->clazz.packageName == 
                           currentClass->clazz.packageName)
                ) { 
                    return thisMethod;
                }
            }
        END_FOR_EACH_METHOD
            /*  If the class has a superclass, look its methods as well */
            thisInstanceClass = thisInstanceClass->superClass;
    } while (thisInstanceClass != NULL);

    /* Before we give up on an interface class, check any interfaces that
     * it implements */
    if ((thisClass->accessFlags & ACC_INTERFACE) 
        && (((INSTANCE_CLASS)thisClass)->ifaceTable != NULL)) {
        CONSTANTPOOL constantPool = ((INSTANCE_CLASS)thisClass)->constPool;
        unsigned short *ifaceTable = ((INSTANCE_CLASS)thisClass)->ifaceTable;
        int tableLength = ifaceTable[0];
        int i;
        for (i = 1; i <= tableLength; i++) {
            int cpIndex = ifaceTable[i];
            CLASS intf = constantPool->entries[cpIndex].clazz;
            METHOD temp = lookupMethod(intf, key, currentClass);
            if (temp != NULL) { 
                return temp;
            }
        }
    }
    return NULL;
}

/* Return true if thisClass or some superclass of it is in the indicated
 * package and has a public declaration of the method
 */

static bool_t 
has_public_declaration(INSTANCE_CLASS thisClass, UString packageName, 
                       NameTypeKey key) 
{ 
    for( ; thisClass != NULL; thisClass = thisClass->superClass) { 
        if (thisClass->clazz.packageName == packageName) { 
            METHODTABLE methodTable = thisClass->methodTable;
            FOR_EACH_METHOD(thisMethod, methodTable) 
                if ((thisMethod->nameTypeKey.i == key.i)
                    && ((thisMethod->accessFlags & ACC_STATIC) == 0)
                    && ((thisMethod->accessFlags & (ACC_PUBLIC|ACC_PROTECTED))))
                    { return TRUE; }
                                                   
            END_FOR_EACH_METHOD
        }
    }
    return FALSE;
}

/*=========================================================================
 * FUNCTION:      lookupDynamicMethod()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Find a method table entry with the given name.
 * INTERFACE:
 *   parameters:  class pointer, and declared method name 
 *   returns:     pointer to the method or NIL
 *
 * NOTES:         This function is called by invokevirtual only. 
 *                It can make at most two passes through the superClass
 *                hierarchy. If a method match is found, but it is in
 *                a different package, it will call the helper function, 
 *                has_public_declaration. This helper function will also
 *                traverse through the class hierarchy to determine if 
 *                this class or some superclass of it is in the indicated 
 *                package, and has a public declaration of it.
 *                A boolean, guaranteed_not_public, is used to ensure that
 *                has_public_declaration is only called once.
 *=======================================================================*/
METHOD lookupDynamicMethod(CLASS thisClass, METHOD declaredMethod) 
{
    NameTypeKey key = declaredMethod->nameTypeKey;
    INSTANCE_CLASS currentClass = declaredMethod->ofClass;
    INSTANCE_CLASS thisInstanceClass = 
        IS_ARRAY_CLASS(thisClass) ? JavaLangObject : (INSTANCE_CLASS)thisClass;
    bool_t guaranteed_not_public = FALSE;
    bool_t accept_any = 
        (declaredMethod->accessFlags & (ACC_PUBLIC | ACC_PROTECTED)) != 0;

    if ((declaredMethod->accessFlags & ACC_PRIVATE) != 0) { 
        /* invokevirtual of a private method always return that method */
        return declaredMethod;
    }

    do {
        METHODTABLE methodTable = thisInstanceClass->methodTable;
        FOR_EACH_METHOD(thisMethod, methodTable) 
            if ((thisMethod->nameTypeKey.i == key.i) 
                  && ((thisMethod->accessFlags & ACC_STATIC) == 0)) { 
                /* static methods are in a completely different namespace
                 * and are therefore ignored
                 */ 
                if (currentClass == thisInstanceClass) { 
                    return thisMethod;
                }
                if ((thisMethod->accessFlags & ACC_PRIVATE) == 0) { 
                    if (accept_any || 
                        (thisInstanceClass->clazz.packageName == 
                              currentClass->clazz.packageName))  { 
                        return thisMethod;
                    }
                    /* We have a match, but it is in the wrong package */
                    if (!guaranteed_not_public) { 
                      INSTANCE_CLASS super = thisInstanceClass->superClass;
                      UString packageName = currentClass->clazz.packageName;
                      if (has_public_declaration(super, packageName, key)) {
                          return thisMethod;
                      } else { 
                          /* No need to ever go through this again */
                          guaranteed_not_public = TRUE;
                      }
                    }
                }
            }
        END_FOR_EACH_METHOD
        /*  If the class has a superclass, look its methods as well */
       thisInstanceClass = thisInstanceClass->superClass;
    } while (thisInstanceClass != NULL);
    return NULL;
}

/*=========================================================================
 * FUNCTION:      getSpecialMethod()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Find a specific special method (<clinit>, main)
 *                using a simple linear lookup strategy.
 * INTERFACE:
 *   parameters:  class pointer, method name and signature pointers
 *   returns:     pointer to the method or NIL
 *=======================================================================*/

METHOD getSpecialMethod(INSTANCE_CLASS thisClass, NameTypeKey key)
{
    METHODTABLE methodTable = thisClass->methodTable;
    FOR_EACH_METHOD(thisMethod, methodTable) 
        if (    (thisMethod->accessFlags & ACC_STATIC)
             && (thisMethod->nameTypeKey.i == key.i))
            return thisMethod;
    END_FOR_EACH_METHOD
    return NIL;
}

/*=========================================================================
 * FUNCTION:      getMethodTableSize()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Return the size of a method table
 * INTERFACE:
 *   parameters:  pointer to a method table
 *   returns:     the size of the given method table
 *=======================================================================*/

#if ENABLEPROFILING
int getMethodTableSize(METHODTABLE methodTable)
{
#if !USESTATIC
    int size = getObjectSize((cell *)methodTable);
    FOR_EACH_METHOD(entry, methodTable) 
        /*  Add the size of the code segment for a non-native method */
        if ((entry->accessFlags & ACC_NATIVE) == 0) { 
            if (entry->u.java.code != NULL) { 
                size += getObjectSize((cell*)entry->u.java.code);
            }
            /*  Add the size of an exception handler table */
            if (entry->u.java.handlers != NIL) {
                size += SIZEOF_HANDLERTABLE(entry->u.java.handlers->length);
            }
        }
    END_FOR_EACH_METHOD
    return size;
#else
    /* All of the pieces of code mentioned above are in Static memory, and
     * there's no easy way to get its size.  So we just ignore them.
     */
    UNUSEDPARAMETER(methodTable)
    return 0;
#endif /* !USESTATIC */
}
#endif /* ENABLEPROFILING */

/*=========================================================================
 * Debugging and printing operations
 *=======================================================================*/

#if INCLUDEDEBUGCODE

/*=========================================================================
 * FUNCTION:      printField()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Print the contents of the given runtime field structure
 *                for debugging purposes.
 * INTERFACE:
 *   parameters:  pointer to a field object
 *   returns:     <nothing>
 *=======================================================================*/

static void 
printField(FIELD thisField) {
    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(char *, className, 
                               getClassName((CLASS)(thisField->ofClass)));
        DECLARE_TEMPORARY_ROOT(char *, fieldSignature, 
                               getFieldSignature(thisField));
        /* Note that getFieldSignature may also allocate */
        fprintf(stdout,"Field %s%s%s'%s.%s%s'\n", 
                ((thisField->accessFlags & ACC_STATIC) ? "static " : ""),
                ((thisField->accessFlags & ACC_FINAL)  ? "final "  : ""),
                ((thisField->accessFlags & ACC_NATIVE) ? "native " : ""), 
                className, fieldName(thisField), fieldSignature);
        fprintf(stdout,"Access flags....: %lx\n", thisField->accessFlags);
    END_TEMPORARY_ROOTS
}

/*=========================================================================
 * FUNCTION:      printFieldTable()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Print the contents of the given field table
 *                for debugging purposes.
 * INTERFACE:
 *   parameters:  pointer to a field table
 *   returns:     <nothing>
 *=======================================================================*/

void printFieldTable(FIELDTABLE fieldTable) {
    FOR_EACH_FIELD(thisField, fieldTable) 
        printField(thisField);
    END_FOR_EACH_FIELD;
}

/*=========================================================================
 * FUNCTION:      printMethod()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Print the contents of the given runtime method structure
 *                for debugging purposes.
 * INTERFACE:
 *   parameters:  pointer to a method object
 *   returns:     <nothing>
 *=======================================================================*/

static void 
printMethod(METHOD thisMethod) {
   START_TEMPORARY_ROOTS
       char *className = getClassName((CLASS)(thisMethod->ofClass));
       fprintf(stdout,"Method %s%s%s'%s.%s%s'\n", 
               ((thisMethod->accessFlags & ACC_STATIC) ? "static " : ""),
               ((thisMethod->accessFlags & ACC_FINAL)  ? "final "  : ""),
               ((thisMethod->accessFlags & ACC_NATIVE) ? "native " : ""), 
               className, methodName(thisMethod), getMethodSignature(thisMethod));
       fprintf(stdout,"Access flags....: %lx\n", (long)thisMethod->accessFlags);
       fprintf(stdout,"Code pointer....: %lx\n", (long)thisMethod->u.java.code);
       fprintf(stdout,"Frame size......: %d\n", thisMethod->frameSize);
       fprintf(stdout,"Argument count..: %d\n", thisMethod->argCount);
       fprintf(stdout,"Maximum stack...: %d\n", thisMethod->u.java.maxStack);

       if (thisMethod->u.java.handlers) {
           printExceptionHandlerTable(thisMethod->u.java.handlers);
       }
       fprintf(stdout,"\n");
   END_TEMPORARY_ROOTS
}

/*=========================================================================
 * FUNCTION:      printMethodTable()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Print the contents of the given method table
 *                for debugging purposes.
 * INTERFACE:
 *   parameters:  pointer to a method table
 *   returns:     <nothing>
 *=======================================================================*/

void printMethodTable(METHODTABLE methodTable) {
    FOR_EACH_METHOD(thisMethod, methodTable) 
        printMethod(thisMethod); 
    END_FOR_EACH_METHOD
}

/*=========================================================================
 * FUNCTION:      printMethodName()
 * TYPE:          debugging function
 * OVERVIEW:      Prints the method name, class, and signature
 * INTERFACE:
 *   parameters:  pointer to a method object
 *   returns:     <nothing>
 *
 * NOTE:          This function is intended to be GC friendly.
 *                It performs no memory allocation.
 *=======================================================================*/

void
printMethodName(METHOD thisMethod, LOGFILEPTR file)
{
    char buffer[512];
    char *p = buffer;
    unsigned short nameKey = thisMethod->nameTypeKey.nt.nameKey;
    unsigned short typeKey = thisMethod->nameTypeKey.nt.typeKey;
    p = getClassName_inBuffer((CLASS)(thisMethod->ofClass), p);
    *p++ = ' ';
    strcpy(p, change_Key_to_Name(nameKey, NULL));
    p += strlen(p);
    change_Key_to_MethodSignature_inBuffer(typeKey, p);

    if (thisMethod->accessFlags & ACC_STATIC) { 
        fprintf(stdout,"static ");
    }
    if (thisMethod->accessFlags & ACC_NATIVE) { 
        fprintf(stdout,"native ");
    }
    if (file == NULL) { 
        /* So it's easy to use this function from a debugger */
        file = stdout;
    }
    fprintf(file, "%s\n", buffer);
}

/*=========================================================================
 * FUNCTION:      printFieldName()
 * TYPE:          debugging function
 * OVERVIEW:      Prints the field name, class, and signature
 * INTERFACE:
 *   parameters:  pointer to a field
 *   returns:     <nothing>
 *
 * NOTE:          This function is intended to be GC friendly.
 *                It performs no memory allocation.
 *=======================================================================*/

void 
printFieldName(FIELD thisField,  LOGFILEPTR file)
{
    char buffer[512];
    char *p = buffer;
    unsigned short nameKey = thisField->nameTypeKey.nt.nameKey;
    unsigned short typeKey = thisField->nameTypeKey.nt.typeKey;
    p = getClassName_inBuffer((CLASS)(thisField->ofClass), p);
    *p++ = ' ';
    strcpy(p, change_Key_to_Name(nameKey, NULL));
    p += strlen(p);
    change_Key_to_FieldSignature_inBuffer(typeKey, p);
    if (file == NULL) { 
        /* So it's easy to use this function from a debugger */
        file = stdout;
    }
    fprintf(file, "%s\n", buffer);
}

#endif /* INCLUDEDEBUGCODE */

/*=========================================================================
 * Internal operations on signatures
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      change_Key_to_FieldSignature()
 *                change_Key_to_FieldSignature_inBuffer()
 *
 * TYPE:          public instance-level operation on runtime classes
 * OVERVIEW:      Converts a FieldType key to its actual signature
 *
 * INTERFACE:
 *   parameters:  key:           An existing FieldType key
 *                resultBuffer:  Where to put the result
 *
 *   returns:     getClassName() returns a pointer to the result
 *                getClassName_inBuffer() returns a pointer to the NULL
 *                     at the end of the result.  The result is in the
 *                     passed buffer.
 * 
 * DANGER:  change_Key_to_FieldSignature_inBuffer() doesn't return what you
 *          expect.
 *
 *=======================================================================*/

char *
change_Key_to_FieldSignature(FieldTypeKey key) { 
    char *endBuffer = change_Key_to_FieldSignature_inBuffer(key, str_buffer);
    int length = endBuffer - str_buffer;
    char *result = mallocBytes(length + 1);
    memcpy(result, str_buffer, length);
    result[length] = 0;
    return result;
}

char*                   /* returns end of buffer */
change_Key_to_FieldSignature_inBuffer(FieldTypeKey key, char *resultBuffer) { 
    char *p          = resultBuffer;
    int depth        = key >> FIELD_KEY_ARRAY_SHIFT;
    FieldTypeKey baseClassKey = key & 0x1FFF;
    if (depth == MAX_FIELD_KEY_ARRAY_DEPTH) { 
        /* just use whatever the real classname is */
        return getClassName_inBuffer(change_Key_to_CLASS(key), resultBuffer);
    }
    if (depth > 0) { 
        memset(p, '[', depth);
        p += depth;
    }
    if (baseClassKey <= 255) { 
        *p++ = (char)baseClassKey;
    } else { 
        *p++ = 'L';
        p = getClassName_inBuffer(change_Key_to_CLASS(baseClassKey), p);
        *p++ = ';';
    } 
    *p = '\0';
    return p;
}
        
/*=========================================================================
 * FUNCTION:      change_Key_to_MethodSignature()
 *                change_Key_to_MethodSignature_inBuffer()
 *
 * TYPE:          public instance-level operation on runtime classes
 * OVERVIEW:      Converts a MethodType key to its actual signature
 *
 * INTERFACE:
 *   parameters:  key:         An existing MethodType key
 *                resultBuffer:  Where to put the result
 *
 *   returns:     getClassName() returns a pointer to the result
 *                getClassName_inBuffer() returns a pointer to the NULL
 *                     at the end of the result.  The result is in the
 *                     passed buffer.
 * 
 * DANGER:  change_Key_to_MethodSignature_inBuffer() doesn't return what you
 *          expect.
 *
 * DANGER:  change_Key_to_MethodSignature() allocates a byte array, and marks
 *          this array as a temporary root.  This function should only be
 *          called inside START_TEMPORARY_ROOTS ... END_TEMPORARY_ROOTS
 *
 * NOTE: The algorithm for converting a method signature to a key is
 * explained in the documentation below for change_MethodSignature_to_Key()
 *=======================================================================*/

char *
change_Key_to_MethodSignature(MethodTypeKey key) 
{ 
    char *endBuffer = change_Key_to_MethodSignature_inBuffer(key, str_buffer);
    int length = endBuffer - str_buffer;
    char *result = mallocBytes(length + 1);
    memcpy(result, str_buffer, length);
    result[length] = 0;
    return result;
}

char*                           /* returns end of string */
change_Key_to_MethodSignature_inBuffer(MethodTypeKey key, char *resultBuffer) {
    int  codedSignatureLength;
    unsigned char *codedSignature = 
        (unsigned char *)change_Key_to_Name(key, &codedSignatureLength);

    unsigned char *from = codedSignature; /* for parsing the codedSignature */
    char *to = resultBuffer;    /* for writing to the result */

    int  argCount = *from++;    /* the first byte is the arg count */

    *to++ = '(';                /* message signatures start with '(' */

    /* Parse the coded signature one argument at a time, and put the 
     * resulting signature into the result.  "from" and "to" are updated. 
     */
    while (--argCount >= 0) { 
        change_Key_to_MethodSignatureInternal(&from, &to);
    }

    /* Now output the closing ')' */
    *to++ = ')';

    /* And the return type */
    change_Key_to_MethodSignatureInternal(&from, &to);

    /* And the closing NULL */
    *to = '\0';

    /* Return the end of the string */
    return to;
}

/* A helper function used above.  
 *
 * It expects the start of the signature of an argument or return type at
 * *fromP.   It encodes that single argument. Both fromP, and toP are updated
 * to point just past the argument just read/written.
 */
static void
change_Key_to_MethodSignatureInternal(unsigned char **fromP, char **toP) { 
    unsigned char *from = *fromP;
    char *to = *toP;
    unsigned char tag = *from++;

    /* normal tags represent themselves */
    if ((tag >= 'A' && tag <= 'Z') && (tag != 'L')) { 
        *to++ = (char)tag;
    } else { 
        FieldTypeKey classKey;
        if (tag == 'L') { 
            tag = *from++;
        }
        classKey = (((int)tag) << 8) + *from++;
        to = change_Key_to_FieldSignature_inBuffer(classKey, to);
    }
    *fromP = from;
    *toP = to;
}

/*=========================================================================
 * FUNCTION:      change_FieldSignature_to_Key()
 *
 * TYPE:          public instance-level operation on runtime classes
 *
 * OVERVIEW:      Converts a field signature into a unique 16-bit value.
 *
 * INTERFACE:
 *   parameters:  type:        A pointer to the field signature
 *                length:      length of signature
 *
 *   returns:     The unique 16-bit key that represents this field type
 * 
 * The encoding is as follows:
 *    0 - 255:    Primitive types.  These use the same encoding as signature.
 *                For example:  int is 'I', boolean is 'Z', etc.
 *
 *  256 - 0x1FFF  Non-array classes.  Every class is assigned a unique 
 *                key value when it is created.  This value is used.  
 *
 * 0x2000-0xDFFF  Arrays of depth 1 - 6.  The high three bits represent the
 *                depth, the low 13-bits (0 - 0x1FFF) represent the type.
 *
 * 0xE000-0xFFFF  Arrays of depth 8 or more.  These are specially encoded
 *                so that the low 13-bits are different that the low 13-bits
 *                of any other class.  The high bits are set so that we'll
 *                recognize it as an array.
 *
 * Note that this scheme lets us encode an array class with out actually
 * creating it.
 * 
 * The algorithm that gives key codes to classes cooperates with this scheme.
 * When an array class is ever created, it is given the appropriate key code.
 *
 * NOTE! [Sheng 1/30/00] The verifier now uses the 13th bit (0x1000) to
 * indicate whether a class instance has been initialized (ITEM_NewObject).
 * As a result, only the low 12 bits (0 - 0xFFF) represent the base class
 * type.
 * 
 *=======================================================================*/

FieldTypeKey
change_FieldSignature_to_Key (CONST_CHAR_HANDLE typeH, int offset, int length) {
    CLASS clazz;
    const char *start = unhand(typeH); /* volatile */
    const char *type = start + offset; /* volatile */
    const char *end = type + length;   /* volatile */
    const char *p = type;              /* volatile */
    
    int depth;

    /* Get the depth, and skip over the initial '[' that indicate the */

    while (*p == '[') { 
        p++;
    }
    depth = p - type;

    /* Arrays of depth greater than 6 are just handled as if they were real
     * classes.  They will automatically be assigned */
    if (depth >= MAX_FIELD_KEY_ARRAY_DEPTH) { 
        clazz = getRawClassX(typeH, offset, length);
        /* I haven't decided yet whether these classes' keys will actually 
         * have their 3 high bits set to 7 or not.  The following will work
         * in either case */
        return (clazz->key) | (short)(MAX_FIELD_KEY_ARRAY_DEPTH
               << FIELD_KEY_ARRAY_SHIFT);
    } else if (*p != 'L') { 
        /* We have a primitive type, or an array thereof (depth <
           MAX_FIELD_KEY_ARRAY_DEPTH )
        */
        return (*(unsigned char *)p) + (depth << FIELD_KEY_ARRAY_SHIFT);
    } else { 
        /* We have an object, or an array thereof.  With signatures, these
         * always have an 'L' at the beginning, and a ';' at the end.  
         * We're not checking that this is a well-formed signature, but we 
         * could */
        p++;                    /* skip over 'L' for non-arrays */
        end--;                  /* ignore ; at the end */
        clazz = getRawClassX(typeH, p - start, end - p);
        return clazz->key + (depth << FIELD_KEY_ARRAY_SHIFT);
    }
} 

/*=========================================================================
 * FUNCTION:      change_MethodSignature_to_Key()
 *
 * TYPE:          public instance-level operation on runtime classes
 *
 * OVERVIEW:      Converts a method signature into a unique 16-bit value.
 *
 * INTERFACE:
 *   parameters:  type:        A pointer to the method signature
 *                length:      length of signature
 *
 *   returns:     The unique 16-bit key that represents this field type
 * 
 * The encoding is as follows:
 * 
 * We first encode the method signature into an "encoded" signature.  This
 * encoded signature is then treated as it were a name, and that name.  We
 * call change_Name_to_Key on it. 
 * 
 * The character array is created as follows
 *   1 byte indicates the number of arguments
 *   1 - 3 bytes indicate the first argument
 *       ..
 *   1 - 3 bytes indicate the nth argument
 *   1 - 3 bytes indicate the return type.
 * 
 *  % If the type is primitive, it is encoded using its signature, like 'I',
 *    'B', etc.  
 *  % Otherwise, we get its 16-bit class key.  
 *     .  If the high byte is in the range 'A'..'Z', we encode it as the 3
 *        bytes,:  'L', hi, lo
 *     .  Otherwise, we encode it as the 2 byte, hi, lo
 *
 * We're saving the whole range 'A'..'Z' since we may want to make 'O' be
 * java.lang.Object, or 'X' be java.lang.String. . . .
 * 
 * Note that the first argument indicates slots, not arguments.  doubles and
 * longs are two slots.
 *=======================================================================*/

FieldTypeKey 
change_MethodSignature_to_Key(CONST_CHAR_HANDLE nameH, int offset, int length)
{

    /* We're first going to encode the signature, as indicated above. */
    unsigned char *buffer = (unsigned char *)str_buffer;/* temporary storage */

    /* unhand(nameH)[fromOffset] points into signature */
    int fromOffset = offset + 1;

    unsigned char *to = buffer + 1; /* pointer into encoding */
    int argCount = 0;
    int result;

    /* Call a helper function to do the actual work.  This help function
     * converts one field into its encoding.  fromOffset and to are updated.  
     * The function returns the number of slots of its argument */
    while (unhand(nameH)[fromOffset] != ')') { 
        /* The following may GC */
        change_MethodSignature_to_KeyInternal(nameH, &fromOffset, &to);
        argCount++;
    }

    /* Skip over the ')' */
    fromOffset++;

    /* And interpret the return type, too */
    change_MethodSignature_to_KeyInternal(nameH, &fromOffset, &to);
    
    /* Set the slot type. */
    buffer[0] = argCount;
    if (fromOffset != offset + length) { 
        /* We should have exactly reached the end of the string */
        raiseExceptionWithMessage(ClassFormatError, KVM_MSG_BAD_METHOD_SIGNATURE);
    }

    /* And get the name key for the encoded signature */
    result = change_Name_to_Key((CONST_CHAR_HANDLE)&buffer, 0, to - buffer);
    return result;
}

/* Helper function used by the above function.  It parses the single field
 * signature starting at *fromP, and puts the result at *toP.  Both are
 * updated to just past where they were.  The size of the argument is returned.
 */

static void
change_MethodSignature_to_KeyInternal(CONST_CHAR_HANDLE nameH, 
                                      int *fromOffsetP, unsigned char **toP)
{
    const char *start = unhand(nameH);
    const int fromOffset = *fromOffsetP;
    const char  *const from = start + fromOffset;
    unsigned char *to = *toP;
    const char *endFrom;
    
    switch (*from) { 
        default:
            endFrom = from + 1;
            break;
        case 'L':
            endFrom = strchr(from + 1, ';') + 1;
            break;
        case '[':
            endFrom = from + 1;
            while (*endFrom == '[') endFrom++;
            if (*endFrom == 'L') { 
                endFrom = strchr(endFrom + 1, ';') + 1;             
            } else { 
                endFrom++;
            }
            break;
    }
    if (endFrom == from + 1) { 
        *to++ = (unsigned char)*from;
    } else { 
        FieldTypeKey key = change_FieldSignature_to_Key(nameH, from - start, 
                                                        endFrom - from);
        unsigned char hiChar = ((unsigned)key) >> 8;
        unsigned char loChar = key & 0xFF;
        if (hiChar >= 'A' && hiChar <= 'Z') { 
            *to++ = 'L';
        }
        *to++ = hiChar;
        *to++ = loChar;
    }
    *to = '\0';                 /* always helps debugging */
    *toP = to;
    /* Although from and endfrom are no longer valid, their difference is
     * still the amount by which fromOffset has increased
     */
    *fromOffsetP = fromOffset + (endFrom - from);
}

/*=========================================================================
 * FUNCTION:      getNameAndTypeKey
 *
 * OVERVIEW:      converts a name and signature (either field or method) to 
 *                a 32bit value. 
 *
 * INTERFACE:
 *   parameters:  name:      Name of method
 *                type:      Field or method signature.
 *
 *   returns:     The unique 32-bit value that represents the name and type.
 * 
 * We use a structure with two 16-bit values.  One 16-bit value encodes the
 * name using standard name encoding.  The other holds the key of the
 * signature.
 * 
 * Note that we cannot do the reverse operation.  Given a type key, we can't
 * tell if its a method key or a field key!
 *=======================================================================*/

NameTypeKey 
getNameAndTypeKey(const char *name, const char *type) {
    int typeLength = strlen(type);
    NameTypeKey result;
    if (INCLUDEDEBUGCODE && (inCurrentHeap(name) || inCurrentHeap(type))) { 
        fatalError(KVM_MSG_BAD_CALL_TO_GETNAMEANDTYPEKEY);
    }
    result.nt.nameKey = getUString(name)->key;
    result.nt.typeKey = ((type)[0] == '(')
              ? change_MethodSignature_to_Key(&type, 0, typeLength)
              : change_FieldSignature_to_Key(&type, 0, typeLength);
    return result;
}

