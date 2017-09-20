/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Error messages
 * FILE:      messages.h
 * OVERVIEW:  This file separates the error message strings
 *            and other user-level C strings from the rest 
 *            of the virtual machine.
 * AUTHOR:    Antero Taivalsaari
 *            Frank Yellin
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

/*=========================================================================
 * Definitions and declarations
 *=======================================================================*/

/*=========================================================================
 * Messages in VmCommon
 *=======================================================================*/

/* Messages in events.h */

#define KVM_MSG_INVALID_TIMESLICE \
        "Fatal: Timeslice < 0"

/* Messages in global.h */

#define KVM_MSG_TRY_BLOCK_ENTERED_WHEN_ALLOCATION_FORBIDDEN \
        "Try block entered when allocation forbidden"

/* Messages in StartJVM.c */

#define KVM_MSG_CLASS_NOT_FOUND_1STRPARAM \
        "Class '%s' not found"

#define KVM_MSG_MUST_PROVIDE_CLASS_NAME \
        "Must provide class name"

/* Messages in bytecodes.c */

#define KVM_MSG_EXPECTED_INITIALIZED_CLASS \
        "Expected an initialized class"

#define KVM_MSG_BAD_CLASS_CANNOT_INSTANTIATE \
        "Cannot instantiate bad class"

#define KVM_MSG_ILLEGAL_WIDE_BYTECODE_EXTENSION \
        "Illegal WIDE bytecode extension"

#define KVM_MSG_BREAKPOINT \
        "Breakpoint\n"

#define KVM_MSG_NO_SUCH_METHOD_2STRPARAMS \
        "No such method %s.%s"

/* Messages in class.c */

#define KVM_MSG_CLASS_EXTENDS_FINAL_CLASS_2STRPARAMS \
        "Class %s extends final class %s"

#define KVM_MSG_INTERFACE_DOES_NOT_EXTEND_JAVALANGOBJECT_1STRPARAM \
        "Interface %s does not extend java.lang.Object"

#define KVM_MSG_ERROR_VERIFYING_CLASS_1STRPARAM \
        "Error verifying class %s"

#define KVM_MSG_BAD_CLASS_STATE \
        "Bad class state"

#define KVM_MSG_STATIC_INITIALIZER_FAILED \
        "Static class initializer failed"

#define KVM_MSG_UNABLE_TO_INITIALIZE_SYSTEM_CLASSES \
        "Unable to initialize system classes"

#define KVM_MSG_BAD_CALL_TO_GETRAWCLASS \
        "Bad call to getRawClass"

#define KVM_MSG_BAD_CALL_TO_GETCLASS \
        "Bad call to getClass"

#define KVM_MSG_BAD_SIGNATURE \
        "Bad signature found"

#define KVM_MSG_STRINGBUFFER_OVERFLOW \
        "Buffer overflow in getStringContentsSafely()"

#define KVM_MSG_CREATING_PRIMITIVE_ARRAY \
        "Creating primitive array?"

/* Messages in collector.c and collectorDebug.c */

#define KVM_MSG_NOT_ENOUGH_MEMORY \
        "Not enough memory to initialize the system"

#define KVM_MSG_CALLED_ALLOCATOR_WHEN_FORBIDDEN \
        "Called memory allocator when forbidden"

#define KVM_MSG_UNABLE_TO_EXPAND_PERMANENT_MEMORY \
        "Unable to expand permanent memory"

#define KVM_MSG_BAD_DYNAMIC_HEAP_OBJECTS_FOUND \
        "Bad dynamic heap objects found"

#define KVM_MSG_BAD_STACK_INFORMATION \
        "Bad stack information found"

#define KVM_MSG_SWEEPING_SCAN_WENT_PAST_HEAP_TOP \
        "Sweeping scan went past heap top"

#define KVM_MSG_BREAK_TABLE_CORRUPTED \
        "GC break table corrupted"

#define KVM_MSG_HEAP_ADDRESS_NOT_FOUR_BYTE_ALIGNED \
        "Heap address is not four-byte aligned"

#define KVM_MSG_UNEXPECTED_STATICBIT \
        "Unexpected STATICBIT found"

#define KVM_MSG_BAD_GC_TAG_VALUE \
        "Bad GC tag value found"

#define KVM_MSG_INVALID_HEAP_POINTER \
        "Invalid heap pointer found"

/* Additional messages in collectorDebug.c */

#define KVM_MSG_POINTER_IN_OLD_SPACE \
        "Fatal: Pointer in old space"

#define KVM_MSG_HEAP_CORRUPTED \
        "Heap corrupted"

/* Messages in execute.c */

#define KVM_MSG_ABSTRACT_METHOD_INVOKED \
        "Abstract method invoked"

#define KVM_MSG_ILLEGAL_BYTECODE_1LONGPARAM \
        "Illegal bytecode %ld"

#define KVM_MSG_SLOWINTERPRETER_STOPPED \
        "Stopped in SlowInterpret()"

#define KVM_MSG_INTERPRETER_STOPPED \
        "Stopped in Interpret()"

#define KVM_MSG_BYTECODE_NOT_IMPLEMENTED_1LONGPARAM \
        "Bytecode %ld not implemented"

#define KVM_MSG_VMSAVE_NOT_CALLED \
        "VMSAVE not called"

/* Messages in fields.c */

#define KVM_MSG_BAD_METHOD_SIGNATURE \
        "Bad method signature"

#define KVM_MSG_BAD_CALL_TO_GETNAMEANDTYPEKEY \
        "Bad call to getNameAndTypeKey()"

/* Messages in frame.c */

#define KVM_MSG_ILLEGAL_EXCEPTION_NAME_1STRPARAM \
        "Illegal exception name %s"

/* Messages in garbage.c */

#define KVM_MSG_TEMPORARY_ROOT_OVERFLOW \
        "Temporary root overflow"

#define KVM_MSG_GLOBAL_ROOT_OVERFLOW \
        "Global root overflow"

#define KVM_MSG_OUT_OF_HEAP_MEMORY \
        "Out of heap memory!"

#define KVM_MSG_ERROR_TOO_MANY_CLEANUP_REGISTRATIONS \
        "Error, too many cleanup registrations"

#define KVM_MSG_CIRCULAR_GC_INVOCATION \
        "Circular GC invocation!"

/* Messages in hashtable.c */

#define KVM_MSG_BAD_CALL_TO_GETUSTRING \
        "Bad call to getUString()"

#define KVM_MSG_TOO_MANY_NAMETABLE_KEYS \
        "Too many entries in name table"

#define KVM_MSG_TOO_MANY_CLASS_KEYS \
        "Too many entries in class table"

#define KVM_MSG_CREATING_CLASS_IN_SYSTEM_PACKAGE \
        "Cannot create class in system package"

#define KVM_MSG_BAD_UTF_STRING \
        "Bad UTF string"

/* Messages in interpret.c */

#define KVM_MSG_STACK_POINTER_CORRUPTED \
        "Stack pointer corrupted"

#define KVM_MSG_FRAME_POINTER_CORRUPTED \
        "Frame pointer corrupted"

#define KVM_MSG_LOCALS_POINTER_CORRUPTED \
        "Locals pointer corrupted"

#define KVM_MSG_ACTIVE_THREAD_COUNT_CORRUPTED \
        "Active thread count corrupted"

#define KVM_MSG_NO_SUCH_FIELD_2STRPARAMS \
        "No such field %s.%s"

#define KVM_MSG_NO_SUCH_METHOD_2STRPARAMS \
        "No such method %s.%s"

/* Messages in loader.c */

#define KVM_MSG_BAD_UTF8_INDEX \
        "Bad Utf8 index"

#define KVM_MSG_BAD_UTF8_STRING \
        "Bad Utf8 string"

#define KVM_MSG_BAD_NAME \
        "Bad class, field or method name"

#define KVM_MSG_BAD_CLASS_ACCESS_FLAGS \
        "Bad class access flags"

#define KVM_MSG_BAD_FIELD_ACCESS_FLAGS \
        "Bad field access flags"

#define KVM_MSG_BAD_FIELD_SIGNATURE \
        "Bad field signature"

#define KVM_MSG_BAD_METHOD_ACCESS_FLAGS \
        "Bad method access flags"

#define KVM_MSG_BAD_CONSTANT_INDEX \
        "Bad constant index"

#define KVM_MSG_BAD_CONSTANT_TAG\
        "Bad constant tag"

#define KVM_MSG_BAD_MAGIC_VALUE \
        "Bad magic value"

#define KVM_MSG_BAD_VERSION_INFO \
        "Bad version information"

#define KVM_MSG_BAD_64BIT_CONSTANT \
        "Bad 64-bit constant"

#define KVM_MSG_INVALID_CONSTANT_POOL_ENTRY \
        "Invalid constant pool entry"

#define KVM_MSG_BAD_FIELD_OR_METHOD_REFERENCE \
        "Bad field or method reference"

#define KVM_MSG_BAD_NAME_OR_TYPE_REFERENCE \
        "Bad name/type reference"

#define KVM_MSG_BAD_SUPERCLASS \
        "Bad superclass"

#define KVM_MSG_BAD_CONSTANTVALUE_LENGTH \
        "Bad ConstantValue length"

#define KVM_MSG_DUPLICATE_CONSTANTVALUE_ATTRIBUTE \
        "Duplicate ConstantValue attribute"

#define KVM_MSG_BAD_CONSTANT_INDEX \
        "Bad constant index"

#define KVM_MSG_DUPLICATE_FIELD_FOUND \
        "Duplicate field found"

#define KVM_MSG_BAD_EXCEPTION_HANDLER_FOUND \
        "Bad exception handler found"

#define KVM_MSG_BAD_NEWOBJECT \
        "Bad NewObject"

#define KVM_MSG_BAD_STACKMAP \
        "Bad stack map"

#define KVM_MSG_DUPLICATE_STACKMAP_ATTRIBUTE \
        "Duplicate StackMap attribute"

#define KVM_MSG_METHOD_LONGER_THAN_32KB \
        "Maximum byte code length (32kB) exceeded"

#define KVM_MSG_TOO_MANY_LOCALS_AND_STACK \
        "Maximum size of locals and stack exceeded"

#define KVM_MSG_BAD_ATTRIBUTE_SIZE \
        "Bad attribute size"

#define KVM_MSG_DUPLICATE_CODE_ATTRIBUTE \
        "Duplicate Code attribute"

#define KVM_MSG_BAD_CODE_ATTRIBUTE_LENGTH \
        "Bad Code attribute length"

#define KVM_MSG_DUPLICATE_EXCEPTION_TABLE \
        "Duplicate exception table"

#define KVM_MSG_BAD_EXCEPTION_ATTRIBUTE \
        "Bad exception attribute"

#define KVM_MSG_MISSING_CODE_ATTRIBUTE \
        "Missing code attribute"

#define KVM_MSG_TOO_MANY_METHOD_ARGUMENTS \
        "More than 255 method arguments"

#define KVM_MSG_BAD_FRAME_SIZE \
        "Bad frame size"

#define KVM_MSG_DUPLICATE_METHOD_FOUND \
        "Duplicate method found"

#define KVM_MSG_CLASSFILE_SIZE_DOES_NOT_MATCH \
        "Class file size does not match"

#define KVM_MSG_CANNOT_LOAD_CLASS_1PARAM \
        "Unable to load class %s"

#define KVM_MSG_CLASS_CIRCULARITY_ERROR \
        "Class circularity error"

#define KVM_MSG_CLASS_EXTENDS_FINAL_CLASS \
        "Class extends a final class"

#define KVM_MSG_CLASS_EXTENDS_INTERFACE \
        "Class extends an interface"

#define KVM_MSG_CLASS_IMPLEMENTS_ARRAY_CLASS \
        "Class implements an array class"

#define KVM_MSG_CLASS_IMPLEMENTS_ITSELF \
        "Class implements itself"

#define KVM_MSG_INTERFACE_CIRCULARITY_ERROR \
        "Interface circularity error"

#define KVM_MSG_CLASS_IMPLEMENTS_NON_INTERFACE \
        "Class implements a non-interface"

#define KVM_MSG_EXPECTED_CLASS_STATUS_OF_CLASS_RAW \
        "Expected class status of CLASS_RAW"

#define KVM_MSG_EXPECTED_CLASS_STATUS_OF_CLASS_RAW_OR_CLASS_LINKED \
        "Expected class status of CLASS_RAW or CLASS_LINKED"

#define KVM_MSG_EXPECTED_CLASS_STATUS_GREATER_THAN_EQUAL_TO_CLASS_LINKED \
        "Expected class status >= CLASS_LINKED"

#define KVM_MSG_UNABLE_TO_COPY_TO_STATIC_MEMORY \
        "Unable to copy data to static memory"

#define KVM_MSG_FLOATING_POINT_NOT_SUPPORTED \
        "Floating point not supported"

/* Messages in native.c */

#define KVM_MSG_NATIVE_METHOD_NOT_FOUND_2STRPARAMS \
        "Native method '%s::%s' not found" 

#define KVM_MSG_NATIVE_METHOD_BAD_USE_OF_TEMPORARY_ROOTS \
        "Native method '%s::%s' has used temporary roots incorrectly" 

/* Messages in pool.c */

#define KVM_MSG_CANNOT_ACCESS_CLASS_FROM_CLASS_2STRPARAMS \
        "Cannot access class %s from class %s"

#define KVM_MSG_ILLEGAL_CONSTANT_CLASS_REFERENCE \
        "Illegal CONSTANT_Class reference"

#define KVM_MSG_INCOMPATIBLE_CLASS_CHANGE_2STRPARAMS \
        "Incompatible class change: %s.%s"

#define KVM_MSG_INCOMPATIBLE_CLASS_CHANGE_3STRPARAMS \
        "Incompatible class change: %s.%s%s"

#define KVM_MSG_CANNOT_MODIFY_FINAL_FIELD_3STRPARAMS \
        "Cannot modify final field %s.%s from class %s"

#define KVM_MSG_CANNOT_ACCESS_MEMBER_FROM_CLASS_3STRPARAMS \
        "Cannot access %s.%s from class %s"

/* Messages in thread.c */

#define KVM_MSG_BAD_PENDING_EXCEPTION \
        "Bad pendingException in SwitchThread()"

#define KVM_MSG_ATTEMPTING_TO_SWITCH_TO_INACTIVE_THREAD \
        "Attempting to switch to an inactive thread"

#define KVM_MSG_CLASS_DOES_NOT_HAVE_MAIN_FUNCTION \
        "Class does not contain 'main' function"

#define KVM_MSG_MAIN_FUNCTION_MUST_BE_PUBLIC \
        "'main' must be declared public"

#define KVM_MSG_ATTEMPTING_TO_RESUME_NONSUSPENDED_THREAD \
        "Attempting to resume a non-suspended thread"

#define KVM_MSG_ATTEMPTING_TO_RESUME_CURRENT_THREAD \
        "Attempting to resume current thread"

#define KVM_MSG_BAD_CALL_TO_ADDCONDVARWAIT \
        "Bad call to addCondvarWait()"

#define KVM_MSG_THREAD_NOT_ON_CONDVAR_QUEUE \
        "Thread not on condvar queue"

#define KVM_MSG_COLLECTOR_IS_RUNNING_ON_ENTRY_TO_ASYNCFUNCTIONPROLOG \
        "Collector is running on entry to asyncFunctionProlog"

#define KVM_MSG_COLLECTOR_RUNNING_IN_RUNDOWNASYNCHRONOUSFUNCTIONS \
        "Collector running in RundownAsynchronousFunctions"

#define KVM_MSG_COLLECTOR_NOT_RUNNING_ON_ENTRY_TO_RESTARTASYNCHRONOUSFUNCTIONS \
        "Collector not running on entry to RestartAsynchronousfunctions"

/* Messages in stackmap.c */

#define KVM_MSG_EXPECTED_RESOLVED_FIELD \
        "Expected a resolved field"

#define KVM_MSG_EXPECTED_RESOLVED_METHOD \
        "Expected a resolved method"

#define KVM_MSG_UNEXPECTED_BYTECODE \
        "Unexpected bytecode"

#define KVM_MSG_ARGUMENT_POPPING_FAILED \
        "Popping args reduced stack size to 0"

#define KVM_MSG_ILLEGAL_STACK_SIZE \
        "Illegal stack size"

#define KVM_MSG_STRANGE_VALUE_OF_THISIP \
        "Strange value of thisIP"

/*=========================================================================
 * Messages in VmExtra
 *=======================================================================*/

/* Messages in async.c */

#define KVM_MSG_PROBLEM_IN_ACQUIRE_ASYNC_IOCB \
        "Problem in AcquireAsyncIOCB"

#define KVM_MSG_PROBLEM_IN_RELEASE_ASYNC_IOCB \
        "Problem in ReleaseAsyncIOCB"

/* Messages in commProtocol.c */

#define KVM_MSG_COMM_WRITE_INCOMPLETE \
        "Comm port error: Write incomplete"

#define KVM_MSG_PROTOCOL_NAME_TOO_LONG \
        "Protocol name too long"

/* Messages in debugger.c */

#define KVM_MSG_DEBUGGER_COULD_NOT_FIND_METHOD \
        "Debugger could not find method"

#define KVM_MSG_DEBUGGER_COULD_NOT_FIND_FIELD \
        "Debugger could not find field"

#define KVM_MSG_UNKNOWN_DEBUGGER_COMMAND_SET \
        "Unknown JDWP debugger command set\n"

#define KVM_MSG_UNKNOWN_JDWP_COMMAND \
        "Unknown JDWP command\n"

#define KVM_MSG_DEBUG_ROOT_CANNOT_BE_FULL \
        "Debug root cannot be full"

#define KVM_MSG_DEBUG_HASHTABLE_INCONSISTENT \
        "Debug Hashtable inconsistent"

#define KVM_MSG_OUT_OF_DEBUGGER_ROOT_SLOTS \
        "Out of debugger root slots"

/* Messages in debuggerOutputStream.c */

#define KVM_MSG_INVALID_JDWP_TYPE_TAG \
        "Invalid JDWP type tag"

/* Messages in debuggerSocketIO.c */

#define KVM_MSG_DEBUGGER_COULD_NOT_INIT_WINSOCK \
        "Debugger could not init WinSock\n"

#define KVM_MSG_DEBUGGER_COULD_NOT_OPEN_LISTENSOCKET \
        "Debugger could not open listenSocket\n"

#define KVM_MSG_DEBUGGER_COULD_NOT_BIND_LISTENSOCKET \
        "Debugger could not bind listenSocket\n"

#define KVM_MSG_DEBUGGER_COULD_NOT_LISTEN_TO_SOCKET \
        "Debugger could not listen to socket\n"

#define KVM_MSG_DEBUGGER_FD_SELECT_FAILED \
        "Debugger FD select failed\n"

#define KVM_MSG_DEBUGGER_FD_ACCEPT_FAILED \
        "Debugger FD accept failed\n"

/* Messages in fakeStaticMemory.c */

#define KVM_MSG_STATIC_MEMORY_ERROR \
        "Bad static memory pointers"

#define KVM_MSG_OUT_OF_STATIC_MEMORY \
        "Out of static memory!"

/* Messages in inflate.c */

#define KVM_MSG_JAR_INVALID_BTYPE \
        "Invalid BTYPE"

#define KVM_MSG_JAR_INPUT_BIT_ERROR \
        "Invalid input bits"

#define KVM_MSG_JAR_OUTPUT_BIT_ERROR \
        "Invalid output bits"

#define KVM_MSG_JAR_BAD_LENGTH_FIELD \
        "Bad length field"

#define KVM_MSG_JAR_INPUT_OVERFLOW \
        "Input overflow"

#define KVM_MSG_JAR_OUTPUT_OVERFLOW \
        "Output overflow"

#define KVM_MSG_JAR_DRAGON_SINGLE_BYTE \
        "Dragon single byte"

#define KVM_MSG_JAR_INVALID_LITERAL_OR_LENGTH \
        "Invalid literal/length"

#define KVM_MSG_JAR_BAD_DISTANCE_CODE \
        "Bad distance code"

#define KVM_MSG_JAR_COPY_UNDERFLOW \
        "Copy underflow"

#define KVM_MSG_JAR_DRAGON_COPY_FAILED \
        "Dragon copy failed"

#define KVM_MSG_JAR_BAD_REPEAT_CODE \
        "Bad repeat code"

#define KVM_MSG_JAR_BAD_CODELENGTH_CODE \
        "Bad code-length code"

#define KVM_MSG_JAR_CODE_TABLE_EMPTY \
        "Code table empty"

#define KVM_MSG_JAR_UNEXPECTED_BIT_CODES \
        "Unexpected bit codes"

/* Messages in loaderFile.c */

#define KVM_MSG_RESOURCE_NOT_FOUND_1STRPARAM \
        "Resource %s not found"

/* Messages in main.c */

#define KVM_MSG_CANT_COMBINE_CLASSPATH_OPTION_WITH_JAM_OPTION \
        "Can't combine '-classpath' with '-jam' option\n"

#define KVM_MSG_CANT_COMBINE_DEBUGGER_OPTION_WITH_REPEAT_OPTION \
        "Can't combine '-debugger' with '-repeat' option\n"

#define KVM_MSG_EXPECTING_HTTP_OR_FILE_WITH_JAM_OPTION \
        "Expecting 'http:' or 'file:' URL with the '-jam' option\n"

#define KVM_MSG_USES_16K_MINIMUM_MEMORY \
        "KVM requires 16kB minimum heap"

#define KVM_MSG_USES_64M_MAXIMUM_MEMORY \
        "KVM allows 64MB maximum heap"

/* Messages in nativeSpotlet.c */

#define KVM_MSG_NOT_IMPLEMENTED \
        "Unimplemented feature"

/* Messages in network.c */

#define KVM_MSG_COULD_NOT_FIND_FIELD_1STRPARAM \
        "Could not find field %s"

#define KVM_MSG_CANNOT_RECEIVE_DATAGRAMS_LONGER_THAN_65535 \
        "Cannot receive datagrams > 65535"

#define KVM_MSG_BUFFER_OVERFLOW_IN_SERVERSOCKET_OPEN \
        "Buffer overflow in serversocket:open()"

/* Messages in networkPrim.c */

#define KVM_MSG_COULD_NOT_MAKE_SOCKET_NONBLOCKING \
        "Could not make socket non-blocking"

#define KVM_MSG_WSASTARTUP_FAILURE \
        "Windows sockets WSAtartup failure"

#define KVM_MSG_PALM_APP_MUST_BE_BUILT_WITH_NETWORKING \
        "Palm application must be built with '-networking' flag!"

/* Messages in verifier.c */

#define KVM_MSG_VERIFIER_STATUS_INFO_INITIALIZER                       \
{   /*  0 */    NULL,  /* SUCCESS */                                   \
    /*  1 */    "Stack Overflow",                                      \
    /*  2 */    "Stack Underflow",                                     \
    /*  3 */    "Unexpected Long or Double on Stack",                  \
    /*  4 */    "Bad type on stack",                                   \
    /*  5 */    "Too many locals",                                     \
    /*  6 */    "Bad type in local",                                   \
    /*  7 */    "Locals underflow",                                    \
    /*  8 */    "Inconsistent or missing stackmap at target",          \
    /*  9 */    "Backwards branch with unitialized object",            \
    /* 10 */    "Inconsistent stackmap at next instruction",           \
    /* 11 */    "Expect constant pool entry of type class",            \
    /* 12 */    "Expect subclass of java.lang.Throwable",              \
    /* 13 */    "Items in lookupswitch not sorted",                    \
    /* 14 */    "Bad constant pool for ldc",                           \
    /* 15 */    "baload requires byte[] or boolean[]",                 \
    /* 16 */    "aaload requires subtype of Object[]",                 \
    /* 17 */    "bastore requires byte[] or boolean[]",                \
    /* 18 */    "bad array or element type for aastore",               \
    /* 19 */    "VE_FIELD_BAD_TYPE",                                   \
    /* 20 */    "Bad constant pool type for invoker",                  \
    /* 21 */    "Insufficient args on stack for method call",          \
    /* 22 */    "Bad arguments on stack for method call",              \
    /* 23 */    "Bad invocation of initialization method",             \
    /* 24 */    "Bad stackmap reference to unitialized object",        \
    /* 25 */    "Initializer called on already initialized object",    \
    /* 26 */    "Illegal byte code (possibly floating point)",         \
    /* 27 */    "arraylength on non-array",                            \
    /* 28 */    "Bad dimension of constant pool for multianewarray",   \
    /* 29 */    "Value returned from void method",                     \
    /* 30 */    "Wrong value returned from method",                    \
    /* 31 */    "Value not returned from method",                      \
    /* 32 */    "Initializer not initializing this",                   \
    /* 33 */    "Illegal offset for stackmap",                         \
    /* 34 */    "Code can fall off the bottom",                        \
    /* 35 */    "Last byte of invokeinterface must be zero",           \
    /* 36 */    "Bad nargs field for invokeinterface",                 \
    /* 37 */    "Bad call to invokespecial",                           \
    /* 38 */    "Bad call to <init> method",                           \
    /* 39 */    "Constant pool entry must be a field reference",       \
    /* 40 */    "Override of final method",                            \
    /* 41 */    "Code ends in middle of byte code",                    \
    /* 42 */    "ITEM_NewObject stack-map type has illegal offset",    \
    /* 43 */    "Bad exception handler range",                         \
    /* 44 */    "Expected object or array on stack"                    \
}

/* Messages in verifierUtil.c */

#define KVM_MSG_V_BAD_POPSTACK_TYPE \
        "Bad popStack type"

#define KVM_MSG_VFY_UNEXPECTED_RETURN_TYPE \
        "Unexpected return type"

