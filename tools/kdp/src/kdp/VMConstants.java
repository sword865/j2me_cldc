/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package kdp;

public interface VMConstants {

    final int VIRTUALMACHINE_CMDSET = 1;
    final int SENDVERSION_CMD = 1;
    final int CLASSESBYSIG_CMD = 2;
    final int ALL_CLASSES_CMD = 3;
    final int ALL_THREADS_CMD = 4;
    final int TOPLEVELTHREADGROUP_CMD = 5;
    final int DISPOSE_CMD = 6;
    final int IDSIZES_CMD = 7;
    final int EXIT_CMD = 10;
    final int CAPABILITIES_CMD = 12;
    final int CLASSPATHS_CMD = 13;
    final int DISPOSE_OBJECTS_CMD = 14;

    final int REFERENCE_TYPE_CMDSET = 2;
    final int SIGNATURE_CMD = 1;
    final int CLASSLOADER_CMD = 2;
    final int MODIFIERS_CMD = 3;
    final int FIELDS_CMD = 4;
    final int METHODS_CMD = 5;
    final int SOURCEFILE_CMD = 7;
    final int INTERFACES_CMD = 10;

    final int METHOD_CMDSET = 6;
    final int METHOD_LINETABLE_CMD = 01;
    final int METHOD_VARIABLETABLE_CMD = 2;
    final int METHOD_BYTECODES_CMD = 3;

    final int THREADREFERENCE_CMDSET = 11;
    final int THREADGROUP_CMD = 5;

    final int THREADGROUPREFERENCE_CMDSET = 12;
    final int THREADGROUP_NAME_CMD = 1;
    final int THREADGROUP_PARENT_CMD = 2;
    final int THREADGROUP_CHILDREN_CMD = 3;

    final int STACKFRAME_CMDSET = 16;
    final int STACKFRAME_GETVALUES_CMD = 1;
    final int STACKFRAME_THISOBJECT_CMD = 3;

    final int CLASSOBJECTREFERENCE_CMDSET = 17;
    final int REFLECTEDTYPE_CMD = 1;

    final int EVENT_CMDSET = 64;
    final int COMPOSITE_CMD = 100;

    final int KVM_CMDSET = 128;
    final int KVM_HANDSHAKE_CMD = 1;
    final int KVM_GET_STEPPINGINFO_CMD = 2;
    final int KVM_STEPPING_EVENT_CMD = 3;

    final short NOTFOUND_ERROR = 41;
    final short INVALID_METHODID = 23;
    final short INVALID_OBJECT = 20;

    final byte JDWP_TypeTag_ARRAY = 3;
    final byte JDWP_EventKind_CLASS_PREPARE = 8;

    final int CLASS_PREPARED = 2;
    final int CLASS_INITIALIZED = 4;

    final byte TYPE_TAG_CLASS = 1;
    final byte TYPE_TAG_ARRAY = 3;

    final int ONLY_THREADGROUP_ID = 0xffffffe0;
    final String KVM_THREADGROUP_NAME = "KVM_System";
}
