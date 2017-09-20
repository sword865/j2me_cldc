/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

#define KILLTHREAD 0x1
#define CLASS_READY 0x5
#define ACC_PUBLIC 0x1
#define ACC_NATIVE 0x100
#define ACC_ABSTRACT 0x400
#define ACC_STATIC 0x8
#define ACC_SYNCHRONIZED 0x20

#define MonitorStatusError 0x3

#define FOR_EACH_BYTE_CODE(_macro_)    \
    FOR_EACH_NORMAL_BYTE_CODE(_macro_) \
    FOR_EACH_EXTRA_BYTE_CODE(_macro_)

#define FOR_EACH_NORMAL_BYTE_CODE(_macro_) \
        _macro_(NOP, 0x0) \
        _macro_(ACONST_NULL, 0x1) \
        _macro_(ICONST_M1, 0x2) \
        _macro_(ICONST_0, 0x3) \
        _macro_(ICONST_1, 0x4) \
        _macro_(ICONST_2, 0x5) \
        _macro_(ICONST_3, 0x6) \
        _macro_(ICONST_4, 0x7) \
        _macro_(ICONST_5, 0x8) \
        _macro_(LCONST_0, 0x9) \
        _macro_(LCONST_1, 0xa) \
        _macro_(FCONST_0, 0xb) \
        _macro_(FCONST_1, 0xc) \
        _macro_(FCONST_2, 0xd) \
        _macro_(DCONST_0, 0xe) \
        _macro_(DCONST_1, 0xf) \
        _macro_(BIPUSH, 0x10) \
        _macro_(SIPUSH, 0x11) \
        _macro_(LDC, 0x12) \
        _macro_(LDC_W, 0x13) \
        _macro_(LDC2_W, 0x14) \
        _macro_(ILOAD, 0x15) \
        _macro_(LLOAD, 0x16) \
        _macro_(FLOAD, 0x17) \
        _macro_(DLOAD, 0x18) \
        _macro_(ALOAD, 0x19) \
        _macro_(ILOAD_0, 0x1a) \
        _macro_(ILOAD_1, 0x1b) \
        _macro_(ILOAD_2, 0x1c) \
        _macro_(ILOAD_3, 0x1d) \
        _macro_(LLOAD_0, 0x1e) \
        _macro_(LLOAD_1, 0x1f) \
        _macro_(LLOAD_2, 0x20) \
        _macro_(LLOAD_3, 0x21) \
        _macro_(FLOAD_0, 0x22) \
        _macro_(FLOAD_1, 0x23) \
        _macro_(FLOAD_2, 0x24) \
        _macro_(FLOAD_3, 0x25) \
        _macro_(DLOAD_0, 0x26) \
        _macro_(DLOAD_1, 0x27) \
        _macro_(DLOAD_2, 0x28) \
        _macro_(DLOAD_3, 0x29) \
        _macro_(ALOAD_0, 0x2a) \
        _macro_(ALOAD_1, 0x2b) \
        _macro_(ALOAD_2, 0x2c) \
        _macro_(ALOAD_3, 0x2d) \
        _macro_(IALOAD, 0x2e) \
        _macro_(LALOAD, 0x2f) \
        _macro_(FALOAD, 0x30) \
        _macro_(DALOAD, 0x31) \
        _macro_(AALOAD, 0x32) \
        _macro_(BALOAD, 0x33) \
        _macro_(CALOAD, 0x34) \
        _macro_(SALOAD, 0x35) \
        _macro_(ISTORE, 0x36) \
        _macro_(LSTORE, 0x37) \
        _macro_(FSTORE, 0x38) \
        _macro_(DSTORE, 0x39) \
        _macro_(ASTORE, 0x3a) \
        _macro_(ISTORE_0, 0x3b) \
        _macro_(ISTORE_1, 0x3c) \
        _macro_(ISTORE_2, 0x3d) \
        _macro_(ISTORE_3, 0x3e) \
        _macro_(LSTORE_0, 0x3f) \
        _macro_(LSTORE_1, 0x40) \
        _macro_(LSTORE_2, 0x41) \
        _macro_(LSTORE_3, 0x42) \
        _macro_(FSTORE_0, 0x43) \
        _macro_(FSTORE_1, 0x44) \
        _macro_(FSTORE_2, 0x45) \
        _macro_(FSTORE_3, 0x46) \
        _macro_(DSTORE_0, 0x47) \
        _macro_(DSTORE_1, 0x48) \
        _macro_(DSTORE_2, 0x49) \
        _macro_(DSTORE_3, 0x4a) \
        _macro_(ASTORE_0, 0x4b) \
        _macro_(ASTORE_1, 0x4c) \
        _macro_(ASTORE_2, 0x4d) \
        _macro_(ASTORE_3, 0x4e) \
        _macro_(IASTORE, 0x4f) \
        _macro_(LASTORE, 0x50) \
        _macro_(FASTORE, 0x51) \
        _macro_(DASTORE, 0x52) \
        _macro_(AASTORE, 0x53) \
        _macro_(BASTORE, 0x54) \
        _macro_(CASTORE, 0x55) \
        _macro_(SASTORE, 0x56) \
        _macro_(POP, 0x57) \
        _macro_(POP2, 0x58) \
        _macro_(DUP, 0x59) \
        _macro_(DUP_X1, 0x5a) \
        _macro_(DUP_X2, 0x5b) \
        _macro_(DUP2, 0x5c) \
        _macro_(DUP2_X1, 0x5d) \
        _macro_(DUP2_X2, 0x5e) \
        _macro_(SWAP, 0x5f) \
        _macro_(IADD, 0x60) \
        _macro_(LADD, 0x61) \
        _macro_(FADD, 0x62) \
        _macro_(DADD, 0x63) \
        _macro_(ISUB, 0x64) \
        _macro_(LSUB, 0x65) \
        _macro_(FSUB, 0x66) \
        _macro_(DSUB, 0x67) \
        _macro_(IMUL, 0x68) \
        _macro_(LMUL, 0x69) \
        _macro_(FMUL, 0x6a) \
        _macro_(DMUL, 0x6b) \
        _macro_(IDIV, 0x6c) \
        _macro_(LDIV, 0x6d) \
        _macro_(FDIV, 0x6e) \
        _macro_(DDIV, 0x6f) \
        _macro_(IREM, 0x70) \
        _macro_(LREM, 0x71) \
        _macro_(FREM, 0x72) \
        _macro_(DREM, 0x73) \
        _macro_(INEG, 0x74) \
        _macro_(LNEG, 0x75) \
        _macro_(FNEG, 0x76) \
        _macro_(DNEG, 0x77) \
        _macro_(ISHL, 0x78) \
        _macro_(LSHL, 0x79) \
        _macro_(ISHR, 0x7a) \
        _macro_(LSHR, 0x7b) \
        _macro_(IUSHR, 0x7c) \
        _macro_(LUSHR, 0x7d) \
        _macro_(IAND, 0x7e) \
        _macro_(LAND, 0x7f) \
        _macro_(IOR, 0x80) \
        _macro_(LOR, 0x81) \
        _macro_(IXOR, 0x82) \
        _macro_(LXOR, 0x83) \
        _macro_(IINC, 0x84) \
        _macro_(I2L, 0x85) \
        _macro_(I2F, 0x86) \
        _macro_(I2D, 0x87) \
        _macro_(L2I, 0x88) \
        _macro_(L2F, 0x89) \
        _macro_(L2D, 0x8a) \
        _macro_(F2I, 0x8b) \
        _macro_(F2L, 0x8c) \
        _macro_(F2D, 0x8d) \
        _macro_(D2I, 0x8e) \
        _macro_(D2L, 0x8f) \
        _macro_(D2F, 0x90) \
        _macro_(I2B, 0x91) \
        _macro_(I2C, 0x92) \
        _macro_(I2S, 0x93) \
        _macro_(LCMP, 0x94) \
        _macro_(FCMPL, 0x95) \
        _macro_(FCMPG, 0x96) \
        _macro_(DCMPL, 0x97) \
        _macro_(DCMPG, 0x98) \
        _macro_(IFEQ, 0x99) \
        _macro_(IFNE, 0x9a) \
        _macro_(IFLT, 0x9b) \
        _macro_(IFGE, 0x9c) \
        _macro_(IFGT, 0x9d) \
        _macro_(IFLE, 0x9e) \
        _macro_(IF_ICMPEQ, 0x9f) \
        _macro_(IF_ICMPNE, 0xa0) \
        _macro_(IF_ICMPLT, 0xa1) \
        _macro_(IF_ICMPGE, 0xa2) \
        _macro_(IF_ICMPGT, 0xa3) \
        _macro_(IF_ICMPLE, 0xa4) \
        _macro_(IF_ACMPEQ, 0xa5) \
        _macro_(IF_ACMPNE, 0xa6) \
        _macro_(GOTO, 0xa7) \
        _macro_(JSR, 0xa8) \
        _macro_(RET, 0xa9) \
        _macro_(TABLESWITCH, 0xaa) \
        _macro_(LOOKUPSWITCH, 0xab) \
        _macro_(IRETURN, 0xac) \
        _macro_(LRETURN, 0xad) \
        _macro_(FRETURN, 0xae) \
        _macro_(DRETURN, 0xaf) \
        _macro_(ARETURN, 0xb0) \
        _macro_(RETURN, 0xb1) \
        _macro_(GETSTATIC, 0xb2) \
        _macro_(PUTSTATIC, 0xb3) \
        _macro_(GETFIELD, 0xb4) \
        _macro_(PUTFIELD, 0xb5) \
        _macro_(INVOKEVIRTUAL, 0xb6) \
        _macro_(INVOKESPECIAL, 0xb7) \
        _macro_(INVOKESTATIC, 0xb8) \
        _macro_(INVOKEINTERFACE, 0xb9) \
        _macro_(UNUSED_BA, 0xba) \
        _macro_(NEW, 0xbb) \
        _macro_(NEWARRAY, 0xbc) \
        _macro_(ANEWARRAY, 0xbd) \
        _macro_(ARRAYLENGTH, 0xbe) \
        _macro_(ATHROW, 0xbf) \
        _macro_(CHECKCAST, 0xc0) \
        _macro_(INSTANCEOF, 0xc1) \
        _macro_(MONITORENTER, 0xc2) \
        _macro_(MONITOREXIT, 0xc3) \
        _macro_(WIDE, 0xc4) \
        _macro_(MULTIANEWARRAY, 0xc5) \
        _macro_(IFNULL, 0xc6) \
        _macro_(IFNONNULL, 0xc7) \
        _macro_(GOTO_W, 0xc8) \
        _macro_(JSR_W, 0xc9) \
        _macro_(BREAKPOINT, 0xca) \
        _macro_(GETFIELD_FAST, 0xcb) \
        _macro_(GETFIELDP_FAST, 0xcc) \
        _macro_(GETFIELD2_FAST, 0xcd) \
        _macro_(PUTFIELD_FAST, 0xce) \
        _macro_(PUTFIELD2_FAST, 0xcf) \
        _macro_(GETSTATIC_FAST, 0xd0) \
        _macro_(GETSTATICP_FAST, 0xd1) \
        _macro_(GETSTATIC2_FAST, 0xd2) \
        _macro_(PUTSTATIC_FAST, 0xd3) \
        _macro_(PUTSTATIC2_FAST, 0xd4) \
        _macro_(UNUSED_D5, 0xd5) \
        _macro_(INVOKEVIRTUAL_FAST, 0xd6) \
        _macro_(INVOKESPECIAL_FAST, 0xd7) \
        _macro_(INVOKESTATIC_FAST, 0xd8) \
        _macro_(INVOKEINTERFACE_FAST, 0xd9) \
        _macro_(NEW_FAST, 0xda) \
        _macro_(ANEWARRAY_FAST, 0xdb) \
        _macro_(MULTIANEWARRAY_FAST, 0xdc) \
        _macro_(CHECKCAST_FAST, 0xdd) \
        _macro_(INSTANCEOF_FAST, 0xde) \
        _macro_(CUSTOMCODE, 0xdf) \

#define FOR_EACH_EXTRA_BYTE_CODE(_macro_) \
        _macro_(UNUSED_E0, 0xe0) \
        _macro_(UNUSED_E1, 0xe1) \
        _macro_(UNUSED_E2, 0xe2) \
        _macro_(UNUSED_E3, 0xe3) \
        _macro_(UNUSED_E4, 0xe4) \
        _macro_(UNUSED_E5, 0xe5) \
        _macro_(UNUSED_E6, 0xe6) \
        _macro_(UNUSED_E7, 0xe7) \
        _macro_(UNUSED_E8, 0xe8) \
        _macro_(UNUSED_E9, 0xe9) \
        _macro_(UNUSED_EA, 0xea) \
        _macro_(UNUSED_EB, 0xeb) \
        _macro_(UNUSED_EC, 0xec) \
        _macro_(UNUSED_ED, 0xed) \
        _macro_(UNUSED_EE, 0xee) \
        _macro_(UNUSED_EF, 0xef) \
        _macro_(UNUSED_F0, 0xf0) \
        _macro_(UNUSED_F1, 0xf1) \
        _macro_(UNUSED_F2, 0xf2) \
        _macro_(UNUSED_F3, 0xf3) \
        _macro_(UNUSED_F4, 0xf4) \
        _macro_(UNUSED_F5, 0xf5) \
        _macro_(UNUSED_F6, 0xf6) \
        _macro_(UNUSED_F7, 0xf7) \
        _macro_(UNUSED_F8, 0xf8) \
        _macro_(UNUSED_F9, 0xf9) \
        _macro_(UNUSED_FA, 0xfa) \
        _macro_(UNUSED_FB, 0xfb) \
        _macro_(UNUSED_FC, 0xfc) \
        _macro_(UNUSED_FD, 0xfd) \
        _macro_(UNUSED_FE, 0xfe) \
        _macro_(UNUSED_FF, 0xff) \

#define SIZEOF_ARRAY 16
#define GET_ARRAY_ofClass(a, b) ldr a, [b, $0]
#define SET_ARRAY_ofClass(a, b) str a, [b, $0]
#define GET_ARRAY_ofClass_IF(cc, a, b) ldr##cc a, [b, $0]
#define SET_ARRAY_ofClass_IF(cc, a, b) str##cc a, [b, $0]
#define ARRAY_ofClass_OFFSET 0

#define GET_ARRAY_length(a, b) ldr a, [b, $8]
#define SET_ARRAY_length(a, b) str a, [b, $8]
#define GET_ARRAY_length_IF(cc, a, b) ldr##cc a, [b, $8]
#define SET_ARRAY_length_IF(cc, a, b) str##cc a, [b, $8]
#define ARRAY_length_OFFSET 8

#define GET_ARRAY_data(a, b) ldr a, [b, $12]
#define SET_ARRAY_data(a, b) str a, [b, $12]
#define GET_ARRAY_data_IF(cc, a, b) ldr##cc a, [b, $12]
#define SET_ARRAY_data_IF(cc, a, b) str##cc a, [b, $12]
#define ARRAY_data_OFFSET 12

#define SIZEOF_INSTANCE 12
#define GET_INSTANCE_ofClass(a, b) ldr a, [b, $0]
#define SET_INSTANCE_ofClass(a, b) str a, [b, $0]
#define GET_INSTANCE_ofClass_IF(cc, a, b) ldr##cc a, [b, $0]
#define SET_INSTANCE_ofClass_IF(cc, a, b) str##cc a, [b, $0]
#define INSTANCE_ofClass_OFFSET 0

#define GET_INSTANCE_data(a, b) ldr a, [b, $8]
#define SET_INSTANCE_data(a, b) str a, [b, $8]
#define GET_INSTANCE_data_IF(cc, a, b) ldr##cc a, [b, $8]
#define SET_INSTANCE_data_IF(cc, a, b) str##cc a, [b, $8]
#define INSTANCE_data_OFFSET 8

#define SIZEOF_FRAME 24
#define GET_FRAME_thisMethod(a, b) ldr a, [b, $12]
#define SET_FRAME_thisMethod(a, b) str a, [b, $12]
#define GET_FRAME_thisMethod_IF(cc, a, b) ldr##cc a, [b, $12]
#define SET_FRAME_thisMethod_IF(cc, a, b) str##cc a, [b, $12]
#define FRAME_thisMethod_OFFSET 12

#define GET_FRAME_previousFp(a, b) ldr a, [b, $0]
#define SET_FRAME_previousFp(a, b) str a, [b, $0]
#define GET_FRAME_previousFp_IF(cc, a, b) ldr##cc a, [b, $0]
#define SET_FRAME_previousFp_IF(cc, a, b) str##cc a, [b, $0]
#define FRAME_previousFp_OFFSET 0

#define GET_FRAME_previousSp(a, b) ldr a, [b, $8]
#define SET_FRAME_previousSp(a, b) str a, [b, $8]
#define GET_FRAME_previousSp_IF(cc, a, b) ldr##cc a, [b, $8]
#define SET_FRAME_previousSp_IF(cc, a, b) str##cc a, [b, $8]
#define FRAME_previousSp_OFFSET 8

#define GET_FRAME_previousIp(a, b) ldr a, [b, $4]
#define SET_FRAME_previousIp(a, b) str a, [b, $4]
#define GET_FRAME_previousIp_IF(cc, a, b) ldr##cc a, [b, $4]
#define SET_FRAME_previousIp_IF(cc, a, b) str##cc a, [b, $4]
#define FRAME_previousIp_OFFSET 4

#define GET_FRAME_syncObject(a, b) ldr a, [b, $16]
#define SET_FRAME_syncObject(a, b) str a, [b, $16]
#define GET_FRAME_syncObject_IF(cc, a, b) ldr##cc a, [b, $16]
#define SET_FRAME_syncObject_IF(cc, a, b) str##cc a, [b, $16]
#define FRAME_syncObject_OFFSET 16

#define SIZEOF_METHOD 32
#define GET_METHOD_nameTypeKey(a, b) ldr a, [b, $0]
#define SET_METHOD_nameTypeKey(a, b) str a, [b, $0]
#define GET_METHOD_nameTypeKey_IF(cc, a, b) ldr##cc a, [b, $0]
#define SET_METHOD_nameTypeKey_IF(cc, a, b) str##cc a, [b, $0]
#define METHOD_nameTypeKey_OFFSET 0

#define GET_METHOD_accessFlags(a, b) ldr a, [b, $20]
#define SET_METHOD_accessFlags(a, b) str a, [b, $20]
#define GET_METHOD_accessFlags_IF(cc, a, b) ldr##cc a, [b, $20]
#define SET_METHOD_accessFlags_IF(cc, a, b) str##cc a, [b, $20]
#define METHOD_accessFlags_OFFSET 20

#define GET_METHOD_ofClass(a, b) ldr a, [b, $24]
#define SET_METHOD_ofClass(a, b) str a, [b, $24]
#define GET_METHOD_ofClass_IF(cc, a, b) ldr##cc a, [b, $24]
#define SET_METHOD_ofClass_IF(cc, a, b) str##cc a, [b, $24]
#define METHOD_ofClass_OFFSET 24

#define GET_METHOD_frameSize(a, b) ldrh a, [b, $28]
#define SET_METHOD_frameSize(a, b) strh a, [b, $28]
#define GET_METHOD_frameSize_IF(cc, a, b) ldr##cc##h a, [b, $28]
#define SET_METHOD_frameSize_IF(cc, a, b) str##cc##h a, [b, $28]
#define METHOD_frameSize_OFFSET 28

#define GET_METHOD_argCount(a, b) ldrh a, [b, $30]
#define SET_METHOD_argCount(a, b) strh a, [b, $30]
#define GET_METHOD_argCount_IF(cc, a, b) ldr##cc##h a, [b, $30]
#define SET_METHOD_argCount_IF(cc, a, b) str##cc##h a, [b, $30]
#define METHOD_argCount_OFFSET 30

#define SIZEOF_FIELD 16
#define GET_FIELD_nameTypeKey(a, b) ldr a, [b, $0]
#define SET_FIELD_nameTypeKey(a, b) str a, [b, $0]
#define GET_FIELD_nameTypeKey_IF(cc, a, b) ldr##cc a, [b, $0]
#define SET_FIELD_nameTypeKey_IF(cc, a, b) str##cc a, [b, $0]
#define FIELD_nameTypeKey_OFFSET 0

#define GET_FIELD_accessFlags(a, b) ldr a, [b, $4]
#define SET_FIELD_accessFlags(a, b) str a, [b, $4]
#define GET_FIELD_accessFlags_IF(cc, a, b) ldr##cc a, [b, $4]
#define SET_FIELD_accessFlags_IF(cc, a, b) str##cc a, [b, $4]
#define FIELD_accessFlags_OFFSET 4

#define GET_FIELD_ofClass(a, b) ldr a, [b, $8]
#define SET_FIELD_ofClass(a, b) str a, [b, $8]
#define GET_FIELD_ofClass_IF(cc, a, b) ldr##cc a, [b, $8]
#define SET_FIELD_ofClass_IF(cc, a, b) str##cc a, [b, $8]
#define FIELD_ofClass_OFFSET 8

#define GET_FIELD_offset(a, b) ldr a, [b, $12]
#define SET_FIELD_offset(a, b) str a, [b, $12]
#define GET_FIELD_offset_IF(cc, a, b) ldr##cc a, [b, $12]
#define SET_FIELD_offset_IF(cc, a, b) str##cc a, [b, $12]
#define FIELD_offset_OFFSET 12

#define GET_FIELD_staticAddress(a, b) ldr a, [b, $12]
#define SET_FIELD_staticAddress(a, b) str a, [b, $12]
#define GET_FIELD_staticAddress_IF(cc, a, b) ldr##cc a, [b, $12]
#define SET_FIELD_staticAddress_IF(cc, a, b) str##cc a, [b, $12]
#define FIELD_staticAddress_OFFSET 12

#define SIZEOF_INSTANCE_CLASS 56
#define GET_INSTANCE_CLASS_constPool(a, b) ldr a, [b, $28]
#define SET_INSTANCE_CLASS_constPool(a, b) str a, [b, $28]
#define GET_INSTANCE_CLASS_constPool_IF(cc, a, b) ldr##cc a, [b, $28]
#define SET_INSTANCE_CLASS_constPool_IF(cc, a, b) str##cc a, [b, $28]
#define INSTANCE_CLASS_constPool_OFFSET 28

#define GET_INSTANCE_CLASS_status(a, b) ldrsh a, [b, $50]
#define SET_INSTANCE_CLASS_status(a, b) strh a, [b, $50]
#define GET_INSTANCE_CLASS_status_IF(cc, a, b) ldr##cc##sh a, [b, $50]
#define SET_INSTANCE_CLASS_status_IF(cc, a, b) str##cc##h a, [b, $50]
#define INSTANCE_CLASS_status_OFFSET 50

#define GET_INSTANCE_CLASS_initThread(a, b) ldr a, [b, $52]
#define SET_INSTANCE_CLASS_initThread(a, b) str a, [b, $52]
#define GET_INSTANCE_CLASS_initThread_IF(cc, a, b) ldr##cc a, [b, $52]
#define SET_INSTANCE_CLASS_initThread_IF(cc, a, b) str##cc a, [b, $52]
#define INSTANCE_CLASS_initThread_OFFSET 52

#define SIZEOF_ARRAY_CLASS 40
#define GET_ARRAY_CLASS_elemClass(a, b) ldr a, [b, $24]
#define SET_ARRAY_CLASS_elemClass(a, b) str a, [b, $24]
#define GET_ARRAY_CLASS_elemClass_IF(cc, a, b) ldr##cc a, [b, $24]
#define SET_ARRAY_CLASS_elemClass_IF(cc, a, b) str##cc a, [b, $24]
#define ARRAY_CLASS_elemClass_OFFSET 24

#define SIZEOF_ICACHE 16
#define GET_ICACHE_contents(a, b) ldr a, [b, $0]
#define SET_ICACHE_contents(a, b) str a, [b, $0]
#define GET_ICACHE_contents_IF(cc, a, b) ldr##cc a, [b, $0]
#define SET_ICACHE_contents_IF(cc, a, b) str##cc a, [b, $0]
#define GET_ICACHE_contents_nth(a, b, n) \
    ldr a, [b, n, lsl $4]
#define ICACHE_contents_OFFSET 0

#define GET_ICACHE_codeLoc(a, b) ldr a, [b, $4]
#define SET_ICACHE_codeLoc(a, b) str a, [b, $4]
#define GET_ICACHE_codeLoc_IF(cc, a, b) ldr##cc a, [b, $4]
#define SET_ICACHE_codeLoc_IF(cc, a, b) str##cc a, [b, $4]
#define GET_ICACHE_codeLoc_nth(a, b, n) \
    add a, b, n, lsl $4; \
    ldr a, [a, $4]
#define ICACHE_codeLoc_OFFSET 4

#define GET_ICACHE_inUse(a, b) ldr a, [b, $12]
#define SET_ICACHE_inUse(a, b) str a, [b, $12]
#define GET_ICACHE_inUse_IF(cc, a, b) ldr##cc a, [b, $12]
#define SET_ICACHE_inUse_IF(cc, a, b) str##cc a, [b, $12]
#define GET_ICACHE_inUse_nth(a, b, n) \
    add a, b, n, lsl $4; \
    ldr a, [a, $12]
#define ICACHE_inUse_OFFSET 12

#define SIZEOF_GLOBALSTATE 20
#define GET_GLOBALSTATE_ip(a, b) ldr a, [b, $0]
#define SET_GLOBALSTATE_ip(a, b) str a, [b, $0]
#define GET_GLOBALSTATE_ip_IF(cc, a, b) ldr##cc a, [b, $0]
#define SET_GLOBALSTATE_ip_IF(cc, a, b) str##cc a, [b, $0]
#define GLOBALSTATE_ip_OFFSET 0

#define GET_GLOBALSTATE_fp(a, b) ldr a, [b, $16]
#define SET_GLOBALSTATE_fp(a, b) str a, [b, $16]
#define GET_GLOBALSTATE_fp_IF(cc, a, b) ldr##cc a, [b, $16]
#define SET_GLOBALSTATE_fp_IF(cc, a, b) str##cc a, [b, $16]
#define GLOBALSTATE_fp_OFFSET 16

#define GET_GLOBALSTATE_cp(a, b) ldr a, [b, $12]
#define SET_GLOBALSTATE_cp(a, b) str a, [b, $12]
#define GET_GLOBALSTATE_cp_IF(cc, a, b) ldr##cc a, [b, $12]
#define SET_GLOBALSTATE_cp_IF(cc, a, b) str##cc a, [b, $12]
#define GLOBALSTATE_cp_OFFSET 12

#define GET_GLOBALSTATE_lp(a, b) ldr a, [b, $8]
#define SET_GLOBALSTATE_lp(a, b) str a, [b, $8]
#define GET_GLOBALSTATE_lp_IF(cc, a, b) ldr##cc a, [b, $8]
#define SET_GLOBALSTATE_lp_IF(cc, a, b) str##cc a, [b, $8]
#define GLOBALSTATE_lp_OFFSET 8

#define GET_GLOBALSTATE_sp(a, b) ldr a, [b, $4]
#define SET_GLOBALSTATE_sp(a, b) str a, [b, $4]
#define GET_GLOBALSTATE_sp_IF(cc, a, b) ldr##cc a, [b, $4]
#define SET_GLOBALSTATE_sp_IF(cc, a, b) str##cc a, [b, $4]
#define GLOBALSTATE_sp_OFFSET 4

