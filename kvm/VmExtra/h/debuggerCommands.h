/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */
/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Debugger
 * FILE:      debuggerCommands.h
 * OVERVIEW:  Table of commands that can be sent from the debugger
 *            to the KVM
 * AUTHOR:    Bill Pittore, Sun Labs
 *=======================================================================*/

typedef void (*CommandHandler)(PACKET_INPUT_STREAM_HANDLE, PACKET_OUTPUT_STREAM_HANDLE);

static void *VirtualMachine_Cmds[] = { 
     (void *)16
    ,(void *)nop        /* VirtualMachine_SendVersion */
    ,(void *)nop        /* VirtualMachine_ClassesBySignature */
    ,(void *)VirtualMachine_AllClasses
    ,(void *)VirtualMachine_AllThreads
    ,(void *)nop        /* VirtualMachine_TopLevelThreadGroups */
    ,(void *)nop        /* dispose */
    ,(void *)nop        /* VirtualMachine_IDSizes */
    ,(void *)VirtualMachine_Suspend
    ,(void *)VirtualMachine_Resume
    ,(void *)VirtualMachine_Exit
    ,(void *)nop        /* VirtualMachine_CreateString */
    ,(void *)nop        /* VirtualMachine_Capabilities */
    ,(void *)nop        /* VirtualMachine_ClassPaths */
    ,(void *)nop        /* VirtualMachine_DisposeObjects */
    ,(void *)nop        /* holdEvents */
    ,(void *)nop        /* releaseEvents */
};

static void *ReferenceType_Cmds[] = { 
     (void *)11
    ,(void *)nop        /* ReferenceType_Signature */
    ,(void *)nop        /* ReferenceType_ClassLoader */
    ,(void *)nop        /* ReferenceType_Modifiers */
    ,(void *)nop        /* ReferenceType_Fields */
    ,(void *)nop        /* ReferenceType_Methods */
    ,(void *)ReferenceType_GetValues
    ,(void *)nop        /* ReferenceType_SourceFile */
    ,(void *)nop        /* nestedTypes */
    ,(void *)nop        /* ReferenceType_Status */
    ,(void *)nop        /* ReferenceType_Interfaces */
    ,(void *)nop        /* classObject */
};

static void *ClassType_Cmds[] = { 
     (void *)0x4
    ,(void *)ClassType_SuperClass
    ,(void *)ClassType_SetValues
    ,(void *)nop        /* invokeStatic */
    ,(void *)nop        /* invokeStatic */
};

static void *ArrayType_Cmds[] = { 
     (void *)0x1
    ,(void *)nop /* newInstance */
};

static void *Field_Cmds[] = { (void *)0x0 };

static void *Method_Cmds[] = { 
      (void *)0x3
    ,(void *)nop /* Method_LineTable */
    ,(void *)nop /* variable Table */
    ,(void *)nop /* bytecodes */
};

static void *ObjectReference_Cmds[] = { 
     (void *)0x9
    ,(void *)ObjectReference_ReferenceType
    ,(void *)ObjectReference_GetValues
    ,(void *)ObjectReference_SetValues
    ,(void *)NULL /* no longer used */
    ,(void *)nop  /* monitorInfo */
    ,(void *)nop  /* invokeInstance */
    ,(void *)nop  /* disableCollection */
    ,(void *)nop  /* enableCollection */
    ,(void *)ObjectReference_IsCollected
    };

static void *StringReference_Cmds[] = { 
     (void *)0x1
    ,(void *)StringReference_Value 
};

static void *ThreadReference_Cmds[] = { 
     (void *)12
    ,(void *)ThreadReference_Name
    ,(void *)ThreadReference_Suspend
    ,(void *)ThreadReference_Resume
    ,(void *)ThreadReference_Status
    ,(void *)nop  /* ThreadReference_ThreadGroup */
    ,(void *)ThreadReference_Frames
    ,(void *)ThreadReference_FrameCount
    ,(void *)nop  /* ownedMonitors */
    ,(void *)nop  /* currentContendedMonitor */
    ,(void *)nop  /* stop */
    ,(void *)nop  /* interrupt */
    ,(void *)ThreadReference_SuspendCount
    };

static void *ThreadGroupReference_Cmds[] = { 
     (void *)3
    ,(void *)nop  /* ThreadGroupReference_Name */
    ,(void *)nop  /* ThreadGroupReference_Parent */
    ,(void *)nop  /* ThreadGroupReference_Children */
    };

static void *ClassLoaderReference_Cmds[] = { 
     (void *)0x1
    ,(void *)nop  /* visibleClasses */
};

static void *ArrayReference_Cmds[] = { 
     (void *)0x3
    ,(void *)ArrayReference_Length
    ,(void *)ArrayReference_GetValues
    ,(void *)ArrayReference_SetValues};

static void *EventRequest_Cmds[] = { 
     (void *)0x3
    ,(void *)EventRequest_Set
    ,(void *)EventRequest_Clear
    ,(void *)EventRequest_ClearAllBreakpoints};

static void *StackFrame_Cmds[] = { 
     (void *)0x3
    ,(void *)StackFrame_GetValues
    ,(void *)StackFrame_SetValues
    ,(void *)nop  /* StackFrame_ThisObject */
};

static void *ClassObjectReference_Cmds[] = { 
     (void *)1
    ,(void *)nop  /* reflectedType */
};

void *KVMCommands[] = { (void *)3
    ,(void *)KVMCommands_HandShake
    ,(void *)KVMCommands_GetSteppingInfo
    ,(void *)nop  /* command from KVM to debug agent - KVMCommands_Stepping_Event */
};

static void** funcArray[] = { 
    /* We use slot 0 for the KVMCommands.  Normally, 0 isn't allowed */
    (void *)KVMCommands,
    /* And the result of them. */
    (void *)VirtualMachine_Cmds,   /* 1 */
    (void *)ReferenceType_Cmds,    /* 2 */
    (void *)ClassType_Cmds,        /* 3 */
    (void *)ArrayType_Cmds,        /* 4 */
    NULL,                          /* 5 */
    (void *)Method_Cmds,           /* 6 */
    NULL,                          /* 7 */
    (void *)Field_Cmds,            /* 8 */
    (void *)ObjectReference_Cmds,  /* 9 */
    (void *)StringReference_Cmds,  /* 10 */
    (void *)ThreadReference_Cmds,  /* 11 */
    (void *)ThreadGroupReference_Cmds, /* 12 */
    (void *)ArrayReference_Cmds,   /* 13 */
    (void *)ClassLoaderReference_Cmds, /* 14 */
    (void *)EventRequest_Cmds,     /* 15 */
    (void *)StackFrame_Cmds,       /* 16 */
    (void *)ClassObjectReference_Cmds, /* 17 */
};

