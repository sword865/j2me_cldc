/*
 *      PalmMain.c     1.17     03/01/14     SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

#define ArraySize(x) (sizeof(x)/sizeof(x[0]))

/* A fixup function is used to update each element of a hash table */
typedef void* (*fixupFunction)(void *object);

static void performFixup();

/* Update all the elements in a hashtable.  fixupFunction is called on
 * each element.  fixupfunction is expected to return its "next" in
 * the bucket */
static void updateHashTable(HASHTABLE, fixupFunction);

/* Called to update a single interned string.  Returns "next" */
static void* updateInternString(void *object);

/* Called to update a single UTF string.  Returns "next" */
static void* updateUTFString(void *object);

/* Called to update a single class and everything that hangs off it.
 */
static void* updateClass(void *object);

/* Called by updateClass to update a single field table */
static void updateFieldTable(FIELDTABLE fieldTable);
/* Called by updateClass to update a single method table */
static void updateMethodTable(METHODTABLE fieldTable);

static unsigned long swap4(unsigned long);
static unsigned short swap2(unsigned short);

#define UPDATE(x)                                                          \
    switch(sizeof(x)) {                                                    \
    case 1:                                                                \
       break;                                                              \
    case 2:                                                                \
       *(unsigned short *)(&x) = swap2(*(unsigned short *)(&x));  break; \
    case 4:                                                                \
       *(unsigned long *)(&x) = swap4(*(unsigned long *)(&x));  break;   \
    default:                                                               \
        fprintf(stderr, "Unknown size %d\n", sizeof(x));                   \
    } 

#define SECTION_ENTRY(name) \
    { (char *)(&name ## SectionHeader) + 4, { (&name ## SectionTrailer) } }

#define STRUCTURE_ENTRY(name) { &name, { ((char *)&name) + sizeof(name) } } 

#define NUMBER_OF_RESOURCES \
    (12 + NUMBER_OF_METHODTABLE_RESOURCES + NUMBER_OF_CODE_RESOURCES)

struct ROMTableOfContentsType ROMTableOfContents = { 
    NUMBER_OF_METHODTABLE_RESOURCES,
    NUMBER_OF_CODE_RESOURCES,
    { 
    (HASHTABLE)&UTFStringTableData,
    (HASHTABLE)&InternStringTableData,
    (HASHTABLE)&ClassTableData
    }, 
    { 
    &AllClassblocks.java_lang_Object,
    &AllClassblocks.java_lang_Class,
    &AllClassblocks.java_lang_String,
    &AllClassblocks.java_lang_Thread,
    &AllClassblocks.java_lang_System,
    &AllClassblocks.java_lang_Throwable,
    &AllClassblocks.java_lang_Error,
    },
    &PrimitiveArrayClasses[0], 
    0,
    { 
    STRUCTURE_ENTRY(ROMTableOfContents),
    SECTION_ENTRY(UTF),
    SECTION_ENTRY(String),
    SECTION_ENTRY(ClassDefinition),
    STRUCTURE_ENTRY(AllHandlers),
    STRUCTURE_ENTRY(AllFields),
    STRUCTURE_ENTRY(AllConstantPools),
    STRUCTURE_ENTRY(AllInterfaces),
    STRUCTURE_ENTRY(AllStackMaps),
    /* New types should go here.  Be sure to change ROMResourceType and 
     * NUMBER_OF_RESOURCES above */
    STRUCTURE_ENTRY(KVM_masterStaticData),
    STRUCTURE_ENTRY(KVM_staticData), 
    ALL_METHODTABLE_RESOURCES,
    ALL_CODE_RESOURCES,
    { NULL, { NULL } }
    }
};
       
static bool_t checkThatCompilerHasntReorderedData(void);
static bool_t checkLocation(void *location, ROMResourceType, const char *what);

int main(int argc, char **argv) { 
    /* Make a copy of the table of contents */
    struct ROMTableOfContentsType copy;
    long endianTest = 0x01020304; /* used to determine endian-ness */
    FILE *directory;
    int i;
    char *name = (argc > 1) ? argv[1] : "PalmROM";
    char buffer[100];
    
    if (sizeof(void *) != 4) { 
    fprintf(stderr, "You must run this on a machine with 4-byte pointers");
    return 1;
    }

    if (!checkThatCompilerHasntReorderedData()) { 
    fprintf(stderr, "Abandon hope, all ye who enter here.\n");
    fprintf(stderr, "Your compiler has reordered the data.\n");
    fprintf(stderr, "There is no hope of this working!\n");
    return 1;
    }

    /* For each section, convert its "start, end" into "start, size" */
    if (NUMBER_OF_RESOURCES > ArraySize(ROMTableOfContents.resources)) { 
    fprintf(stderr, "Error: Too many resources.  Increase the constant\n");
    fprintf(stderr, "in the definition of ROMTableOfContentsType\n");
    return 1;
    } else { 
    ROMTableOfContents.resources[TOCResource].u.endAddress = 
        &ROMTableOfContents.resources[NUMBER_OF_RESOURCES];
    }
    for (i = 0; i < NUMBER_OF_RESOURCES; i++) { 
    void *start = ROMTableOfContents.resources[i].startAddress;
    void *end = ROMTableOfContents.resources[i].u.endAddress;
    long size = (char *)end - (char *)start;
    /* We are going to write 2 scratch bytes at the end, so we need
     * to increment the size by 2 
     */
    ROMTableOfContents.resources[i].u.size = (size == 0 ? 0 : size + 2);
    }
    copy = ROMTableOfContents;
    
    if (*(char *)&endianTest == 0x04) {
    /* Fixup  all the bytes to be little endian instead of big endian */
    performFixup();
    } 
    
    sprintf(buffer, "%s.r", name);
    directory = fopen(buffer, "w");
    fprintf(directory, "/* This is a generated file.  Do not modify.\n");
    fprintf(directory, " * Generated on " ROM_GENERATION_DATE "\n");
    fprintf(directory, "*/\n\n\n");

    printf("Generating resources.\n");

    for (i = 0; i < NUMBER_OF_RESOURCES; i++) { 
    /* Start, end, size */
    void *start = copy.resources[i].startAddress;
    long size = copy.resources[i].u.size;
    if (size > 0 && start != KVM_staticData) { 
        int resource = 1000 + i;
        /* Open an appropriated named file */
        FILE *file;
        fprintf(directory, "read 'jROM' (%d) \"bin\\\\PalmROM%d.bin\";\n", 
            resource, resource);
        sprintf(buffer, "%s%04d.bin", name, resource);
        file = fopen(buffer, "wb");
            if (file == NULL) {
                fprintf(stderr, "Unable to open file %s\n", buffer);
                return 1;
            }
            if (fwrite(start, sizeof(char), size-2, file) != size-2) {
                fprintf(stderr, "Cannot write %d bytes to %s\n", size, buffer);
                return 1;
            }
        /* Add two null bytes at the end */
        putc(0,file); putc(0,file);
            if (fclose(file) != 0) {
                fprintf(stderr, "Unable to close file %s\n", buffer);
                return 1;
            }
        printf("%s: 0x%06lx-0x%06lx: %6ld bytes\n", 
           buffer, (long)start, (long)start + size, size);
    }
    }
    if (fclose(directory) != 0) {
    fprintf(stderr, "Unable to close directory\n", buffer);
    return 1;
    }
    return 0;
}

static void
performFixup() { 
    int i;
    
    /* Update all the entries in the table of contents */
    for (i = 0; i < NUMBER_OF_RESOURCES; i++) { 
    UPDATE(ROMTableOfContents.resources[i].startAddress);
    UPDATE(ROMTableOfContents.resources[i].u.size);
    }
    
    UPDATE(ROMTableOfContents.methodTableResourceCount);
    UPDATE(ROMTableOfContents.codeResourceCount);
    for (i = 0; i < ArraySize(ROMTableOfContents.hashtables); i++) {
    UPDATE(ROMTableOfContents.hashtables[i]);
    }
    for (i = 0; i < ArraySize(ROMTableOfContents.classes); i++) {
    UPDATE(ROMTableOfContents.classes[i]);
    }
    UPDATE(ROMTableOfContents.arrayClasses);
    UPDATE(ROMTableOfContents.flags);    

    /* Update the master static data.  We deal with swapping the two
     * words of doubles/longs later */
    UPDATE(KVM_masterStaticData.count);
    for (i = 0; i < ArraySize(KVM_masterStaticData.roots); i++) { 
    UPDATE(KVM_masterStaticData.roots[i]);
    }
    for (i = 0; i < ArraySize(KVM_masterStaticData.nonRoots); i++) { 
    UPDATE(KVM_masterStaticData.nonRoots[i]);
    }

    /* Update the character array pointed at by all the strings */
    { 
    /* For GC purposes, stringCharArray has an extra word at its start. */
    SHORTARRAY array  = (SHORTARRAY)&(stringCharArrayInternal.ofClass);
    int length = array->length;
    UPDATE(array->ofClass);
    UPDATE(array->mhc.hashCode);
    UPDATE(array->length);
    for (i = 0; i < length; i++) { 
        UPDATE(array->sdata[i]);
    }
    }

    /* Update the array of primitive classes */
    for (i = 0; i < ArraySize(PrimitiveArrayClasses); i++) { 
        UPDATE(PrimitiveArrayClasses[i]);
    }

    /* Update all the UTF Strings */
    updateHashTable((HASHTABLE)&UTFStringTableData, updateUTFString);

    /* Update all the interned strings */
    updateHashTable((HASHTABLE)&InternStringTableData, updateInternString);

    /* Update all the class tables */
    updateHashTable((HASHTABLE)&ClassTableData, updateClass);
}

/* Update a hashtable.  Call the functor on each of its elements */
static void
updateHashTable(HASHTABLE table, fixupFunction functor)
{
    int i;
    long bucketCount = table->bucketCount;

    /* Update these two fields */
    UPDATE(table->bucketCount);
    UPDATE(table->count);

    for (i = 0; i < bucketCount; i++) { 
    void *ptr = table->bucket[i];
    UPDATE(table->bucket[i]);
    while (ptr != NULL) { 
        ptr = functor(ptr);
    }
    }
}

/* Update a single UTF string.  Return string->next */
void *
updateUTFString(void *object) 
{ 
    UString entry = object;
    UString next = entry->next;    /* grab before modifying */

    UPDATE(entry->next);
    UPDATE(entry->length);
    UPDATE(entry->key);
    return next;
}

/* Update the hash entry of an interned string, and the corresponding String */
void* 
    updateInternString(void *object) 
{
    INTERNED_STRING_INSTANCE string = object;
    INTERNED_STRING_INSTANCE next = string->next; /* grab before modifying */

    UPDATE(string->ofClass);
    UPDATE(string->mhc.hashCode);
    UPDATE(string->array);
    UPDATE(string->offset);
    UPDATE(string->length);
    UPDATE(string->next);

    return next;
}   

/*  Update a class and all the stuff hanging off it */
void*
updateClass(void *object)
{
    CLASS clazz = object;
    CLASS next = clazz->next;
    bool_t isArray = IS_ARRAY_CLASS(clazz);
    int length, i;

    UPDATE(clazz->ofClass);
    UPDATE(clazz->mhc.hashCode);
    UPDATE(clazz->packageName);
    UPDATE(clazz->baseName);
    UPDATE(clazz->next);
    UPDATE(clazz->accessFlags);
    UPDATE(clazz->key);

    if (!isArray) { 
    INSTANCE_CLASS iclazz = (INSTANCE_CLASS)clazz;
    CONSTANTPOOL constPool = iclazz->constPool;
    FIELDTABLE fieldTable = iclazz->fieldTable;
    METHODTABLE methodTable = iclazz->methodTable;
    unsigned short* ifaceTable = iclazz->ifaceTable;
    POINTERLIST staticFields = iclazz->staticFields;

    UPDATE(iclazz->superClass);
    UPDATE(iclazz->constPool);
    UPDATE(iclazz->fieldTable);
    UPDATE(iclazz->methodTable);
    UPDATE(iclazz->ifaceTable);
    UPDATE(iclazz->staticFields);
    UPDATE(iclazz->instSize);
    UPDATE(iclazz->status);
    UPDATE(iclazz->initThread);

    /* Update the constant pool.  All entries in the constant pool,
     * except for CONSTANT_NameAndType, are 4-byte entries.  The
     * ROMized image shouldn't have any of those. */
        if (constPool != NULL) { 
        /* unsigned char *tags = CONSTANTPOOL_TAGS(constPool); */
        length = CONSTANTPOOL_LENGTH(constPool);
        for (i = 0; i < length; i++) { 
        UPDATE(constPool->entries[i].integer);
        }
        }

    /* Update the fields */
        if (fieldTable != NULL) {
        updateFieldTable(fieldTable);
    } 

    /* Update the methods */
        if (methodTable != NULL) { 
        updateMethodTable(methodTable);
    }

    /* Update the interface table */
    if (ifaceTable != NULL) { 
        length = ifaceTable[0];
        for (i = 0; i <= length; i++) { 
        UPDATE(ifaceTable[i]);
        }
    }

    /* In the ROMized image, static Fields should always be NULL */
    if (staticFields != NULL) { 
        fprintf(stderr, "Non null static fields\n");
    }
    } else { 
    /* An array class is much simpler */
    ARRAY_CLASS aclazz = (ARRAY_CLASS)clazz;
    int gcType = aclazz->gcType;
    if (gcType == GCT_ARRAY) { 
        UPDATE(aclazz->u.primType);
    } else { 
        UPDATE(aclazz->u.elemClass);
    }
    UPDATE(aclazz->itemSize);
    UPDATE(aclazz->gcType);
    }
    return next;
}    

/*  Update a field table */
static void 
updateFieldTable(FIELDTABLE fieldTable)
{
    FIELD field = fieldTable->fields;
    FIELD lastField = field + fieldTable->length;

    UPDATE(fieldTable->length);

    for ( ; field < lastField; field++) { 
    long accessFlags = field->accessFlags;

    /* Note that name and type are two 2-byte entries, not a single
     * 4-byte entry */
    UPDATE(field->nameTypeKey.nt.nameKey);
    UPDATE(field->nameTypeKey.nt.typeKey);
    UPDATE(field->accessFlags);    
    UPDATE(field->ofClass);    

    if ((accessFlags & ACC_STATIC) != 0) { 
        static long tester[2] = { ROM_STATIC_LONG(/* high */1, /*low */0) };
        if ((accessFlags & ACC_DOUBLE) != 0 && tester[0] == 0) { 
        /* We need to swap this guys two words in the master
         * static data, since it was stored as little ending, but
         * the PALM is big endian */
        void *staticAddr = field->u.staticAddress;
        long delta = (char*)staticAddr - (char*)KVM_staticData;
        void *masterAddr = (char *)&KVM_masterStaticData + delta;
        long *longAddr = (long *)masterAddr;
        long low = longAddr[0];
        long high = longAddr[1];
        longAddr[1] = low;
        longAddr[0] = high;
        }
        UPDATE(field->u.staticAddress);
    } else { 
        UPDATE(field->u.offset);
    }
    }
}

/* Update all the pointers in a method table */
static void updateMethodTable(METHODTABLE methodTable) 
{
    METHOD method = methodTable->methods; 
    METHOD lastMethod = method + methodTable->length;
    UPDATE(methodTable->length);
    for ( ; method < lastMethod; method++) { 
    long accessFlags = method->accessFlags;

    UPDATE(method->nameTypeKey.nt.nameKey);
    UPDATE(method->nameTypeKey.nt.typeKey);
    UPDATE(method->accessFlags);    
    UPDATE(method->ofClass);    
    UPDATE(method->frameSize); 
    UPDATE(method->argCount); 

    if (accessFlags & ACC_NATIVE) { 
        UPDATE(method->u.native.code);
        UPDATE(method->u.native.info);
    } else { 
        HANDLERTABLE handlerTable = method->u.java.handlers;
        STACKMAP stackMaps = method->u.java.stackMaps.pointerMap;
        UPDATE(method->u.java.code);
        UPDATE(method->u.java.handlers);
        UPDATE(method->u.java.stackMaps);
        UPDATE(method->u.java.codeLength);
        UPDATE(method->u.java.maxStack); 
        if (stackMaps != NULL) {
        unsigned short nEntries = stackMaps->nEntries;
        struct stackMapEntryStruct *stackMapEntry = stackMaps->entries;
        struct stackMapEntryStruct *lastEntry = stackMapEntry +
            (nEntries & STACK_MAP_ENTRY_COUNT_MASK);
        UPDATE(stackMaps->nEntries);
        for(; stackMapEntry < lastEntry; stackMapEntry++) {
            UPDATE(stackMapEntry->offset);
            /*
             * if this is a short entry, then we've already
             * put the bytes in the correct order.  If a 
             * long entry then we need to swap the bytes
             */
            if (!(nEntries & STACK_MAP_SHORT_ENTRY_FLAG)) {
                UPDATE(stackMapEntry->stackMapKey);
            }
        }
        }
        if (handlerTable != NULL) { 
        /* Update all the exception handlers */
        HANDLER handler = handlerTable->handlers; 
        HANDLER lastHandler = handler + handlerTable->length;
        UPDATE(handlerTable->length);
        for (; handler < lastHandler; handler++) { 
            UPDATE(handler->startPC);
            UPDATE(handler->endPC);
            UPDATE(handler->handlerPC);
            UPDATE(handler->exception);
        }
        }
    }
    }
}

/* Swap the two bytes a 16-bit quantity */
static unsigned short 
swap2(unsigned short value) {
    value = (value << 8) | (value >> 8);
    return value;
}

/* Swap the four bytes of a 32-bit quantity. */
static unsigned long 
swap4(unsigned long value) { 
    value = ((value & 0xFF00FF00) >> 8) | ((value & 0x00FF00FF) << 8);
    value = (value >> 16) | (value << 16);
    return value;
}

static bool_t
checkThatCompilerHasntReorderedData(void) 
{
    /* Note that we use & rather than && so that we'll see >>all<< the
     * error messages, rather than just the first one.
     */
    return checkLocation(&stringCharArrayInternal, StringResource, 
             "String character array")
    &  checkLocation(&stringArrayInternal, StringResource, "String array")
    &  checkLocation(&InternStringTableData, StringResource, 
             "String Table" )
    
    &  checkLocation(&UTFStringTableData, UTFResource, "UTF table") 

    /* Pick a class at random */
    &  checkLocation(&AllClassblocks.java_util_Date, ClassDefsResource, 
             "Class Information")
    &  checkLocation(&AllClassblocks.java_lang_Object, ClassDefsResource, 
             "Class Information")
    &  checkLocation(&ClassTableData, ClassDefsResource, "Class Table");
}

static bool_t 
checkLocation(void *location, ROMResourceType id, const char *what)
{
    void *start = ROMTableOfContents.resources[id].startAddress;
    void *end  = ROMTableOfContents.resources[id].u.endAddress;

    if (location < start || location >= end) { 
    fprintf(stderr, "%s at 0x%x is not in range 0x%x-0x%x\n", 
        what, location, start, end);
    return FALSE;
    }
    return TRUE;
}
