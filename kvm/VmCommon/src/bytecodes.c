/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Bytecode interpreter
 * FILE:      bytecodes.c
 * OVERVIEW:  This file defines the implementation of each of the
 *            Java bytecodes.
 *            The code has been separated from interpret.c so that we
 *            can more easily use different kinds of interpretation
 *            techniques and customize the VM to use sub/supersets
 *            of bytecodes.
 * AUTHOR:    Rewritten for better performance and customizability
 *            by Nik Shaylor 9/5/2000
 *=======================================================================*/

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(NOP)             /* Do nothing */
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ACONST_NULL)     /* Push null reference onto the operand stack */
        pushStack(0);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ICONST_M1)     /* Push integer constant -1 onto the operand stack */
        pushStack(-1);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ICONST_0)       /* Push integer constant 0 onto the operand stack */
        pushStack(0);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ICONST_1)       /* Push integer constant 1 onto the operand stack */
        pushStack(1);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ICONST_2)       /* Push integer constant 2 onto the operand stack */
        pushStack(2);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ICONST_3)       /* Push integer constant 3 onto the operand stack */
        pushStack(3);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ICONST_4)       /* Push integer constant 4 onto the operand stack */
        pushStack(4);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ICONST_5)       /* Push integer constant 5 onto the operand stack */
        pushStack(5);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LCONST_0)  /* Push long integer constant 0 onto the operand stack */
        oneMore;
        ((long *)sp)[0] = 0;
        ((long *)sp)[1] = 0;
        oneMore;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LCONST_1)  /* Push long integer constant 1 onto the operand stack */
        oneMore;
#if BIG_ENDIAN
        ((long *)sp)[0] = 0;
        ((long *)sp)[1] = 1;
#elif LITTLE_ENDIAN || !COMPILER_SUPPORTS_LONG
        ((long *)sp)[0] = 1;
        ((long *)sp)[1] = 0;
#else
        SET_LONG(sp, (ulong64)(1));
#endif
        oneMore;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(FCONST_0)         /* Push float constant 0 onto the operand stack */
        oneMore;
        *(float*)sp = 0.0f;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(FCONST_1)         /* Push float constant 1 onto the operand stack */
        oneMore;
        *(float*)sp = 1.0f;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(FCONST_2)         /* Push float constant 2 onto the operand stack */
        oneMore;
        *(float*)sp = 2.0f;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(DCONST_0)  /* Push double float constant 0 onto the operand stack */
        oneMore;
        SET_DOUBLE(sp, 0.0);
        oneMore;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(DCONST_1)  /* Push double float constant 1 onto the operand stack */
        oneMore;
        SET_DOUBLE(sp, 1.0);
        oneMore;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(BIPUSH)              /* Push byte constant onto the operand stack */
        int i = ((signed char *)ip)[1];
        pushStack(i);
DONE(2)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(SIPUSH)             /* Push short constant onto the operand stack */
        short s = getShort(ip + 1);
        pushStack((int)s); /* Extends sign for negative numbers */
DONE(3)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LDC)       /* Push item from constant pool onto the operand stack */
        unsigned int cpIndex = ip[1];
        CONSTANTPOOL_ENTRY thisEntry = &cp->entries[cpIndex];
        pushStack(thisEntry->integer);
DONE(2)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LDC_W)               /* Push item from constant pool (wide index) */
        unsigned int cpIndex = getUShort(ip + 1);
        CONSTANTPOOL_ENTRY thisEntry = &cp->entries[cpIndex];
        pushStack(thisEntry->integer);
DONE(3)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LDC2_W)    /* Push double or long from constant pool (wide index) */
        unsigned int cpIndex = getUShort(ip + 1);
        CONSTANTPOOL_ENTRY thisEntry = &cp->entries[cpIndex];
        unsigned long hiBytes = (unsigned long)(thisEntry[0].integer);
        unsigned long loBytes = (unsigned long)(thisEntry[1].integer);
        oneMore;
        SET_LONG_FROM_HALVES(sp, hiBytes, loBytes);
        oneMore;
DONE(3)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ILOAD)            /* Load integer from local variable */
        unsigned int index = ip[1];
        pushStack(lp[index]);
DONE(2)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LLOAD)            /* Load long integer from local variable */
        unsigned int index = ip[1];
        pushStack(lp[index]);
        pushStack(lp[index+1]);
DONE(2)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(FLOAD)            /* Load float from local variable */
        unsigned int index = ip[1];
        pushStack(lp[index]);
DONE(2)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(DLOAD)            /* Load double float from local variable */
        unsigned int index = ip[1];
        pushStack(lp[index]);
        pushStack(lp[index+1]);
DONE(2)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ALOAD)            /* Load address from local variable */
        unsigned int index = ip[1];
        pushStack(lp[index]);
DONE(2)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ILOAD_0)      /* Load integer from first (zeroeth) local variable */
        pushStack(lp[0]);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ILOAD_1)          /* Load integer from second local variable */
        pushStack(lp[1]);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ILOAD_2)          /* Load integer from third local variable */
        pushStack(lp[2]);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ILOAD_3)          /* Load integer from fourth local variable */
        pushStack(lp[3]);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LLOAD_0)      /* Load integer from first (zeroeth) local variable */
        pushStack(lp[0]);
        pushStack(lp[1]);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LLOAD_1)           /* Load integer from second local variable */
        pushStack(lp[1]);
        pushStack(lp[2]);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LLOAD_2)          /* Load integer from third local variable */
        pushStack(lp[2]);
        pushStack(lp[3]);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LLOAD_3)          /* Load integer from fourth local variable */
        pushStack(lp[3]);
        pushStack(lp[4]);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(FLOAD_0)      /* Load integer from first (zeroeth) local variable */
        pushStack(lp[0]);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(FLOAD_1)          /* Load integer from second local variable */
        pushStack(lp[1]);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(FLOAD_2)          /* Load integer from third local variable */
        pushStack(lp[2]);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(FLOAD_3)          /* Load integer from fourth local variable */
        pushStack(lp[3]);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(DLOAD_0)      /* Load integer from first (zeroeth) local variable */
        pushStack(lp[0]);
        pushStack(lp[1]);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(DLOAD_1)          /* Load integer from second local variable */
        pushStack(lp[1]);
        pushStack(lp[2]);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(DLOAD_2)          /* Load integer from third local variable */
        pushStack(lp[2]);
        pushStack(lp[3]);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(DLOAD_3)          /* Load integer from fourth local variable */
        pushStack(lp[3]);
        pushStack(lp[4]);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ALOAD_0)      /* Load integer from first (zeroeth) local variable */
        pushStack(lp[0]);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ALOAD_1)          /* Load integer from second local variable */
        pushStack(lp[1]);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ALOAD_2)          /* Load integer from third local variable */
        pushStack(lp[2]);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ALOAD_3)          /* Load integer from fourth local variable */
        pushStack(lp[3]);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IALOAD)           /* Load integer from array */
        long      index = popStack();
        ARRAY thisArray = topStackAsType(ARRAY);
        CHECKARRAY(thisArray, index);
            topStack = thisArray->data[index].cell;
        ENDCHECKARRAY
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LALOAD)           /* Load long from array */
        long      index = topStack;
        ARRAY thisArray = secondStackAsType(ARRAY);
        CHECKARRAY(thisArray, index);
            secondStack = thisArray->data[index * 2].cell;
            topStack    = thisArray->data[index * 2 + 1].cell;
        ENDCHECKARRAY
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(FALOAD)           /* Load float from array */
        long      index = popStack();
        ARRAY thisArray = topStackAsType(ARRAY);
        CHECKARRAY(thisArray, index);
            topStack = thisArray->data[index].cell;
        ENDCHECKARRAY
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(DALOAD)           /* Load double from array */
        long      index = topStack;
        ARRAY thisArray = secondStackAsType(ARRAY);
        CHECKARRAY(thisArray, index);
            secondStack = thisArray->data[index * 2].cell;
            topStack    = thisArray->data[index * 2 + 1].cell;
        ENDCHECKARRAY
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(AALOAD)           /* Load address from array */
        long      index = popStack();
        ARRAY thisArray = topStackAsType(ARRAY);
        CHECKARRAY(thisArray, index);
            topStack = thisArray->data[index].cell;
        ENDCHECKARRAY
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(BALOAD)           /* Load byte from array */
        long          index = popStack();
        BYTEARRAY thisArray = topStackAsType(BYTEARRAY);
        CHECKARRAY(thisArray, index);
            topStack = (long)thisArray->bdata[index];
        ENDCHECKARRAY
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(CALOAD)           /* Load UNICODE character (16 bits) from array */
        long           index = popStack();
        SHORTARRAY thisArray = topStackAsType(SHORTARRAY);
        CHECKARRAY(thisArray, index);
            topStack = (thisArray->sdata[index]&0xFFFF);
        ENDCHECKARRAY
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(SALOAD)           /* Load short from array */
        long           index = popStack();
        SHORTARRAY thisArray = topStackAsType(SHORTARRAY);
        CHECKARRAY(thisArray, index);
            topStack = thisArray->sdata[index];
        ENDCHECKARRAY
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ISTORE)           /* Store integer into local variable */
        unsigned int index = ip[1];
        lp[index] = popStack();
DONE(2)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LSTORE)           /* Store long into local variable */
        unsigned int index = ip[1];
        lp[index+1] = popStack();
        lp[index]   = popStack();
DONE(2)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(FSTORE)           /* Store integer into local variable */
        unsigned int index = ip[1];
        lp[index] = popStack();
DONE(2)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(DSTORE)           /* Store double into local variable */
        unsigned int index = ip[1];
        lp[index+1] = popStack();
        lp[index]   = popStack();
DONE(2)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ASTORE)           /* Store reference into local variable */
        unsigned int index = ip[1];
        lp[index] = popStack();
DONE(2)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ISTORE_0)    /* Store integer into first (zeroeth) local variable */
        lp[0] = popStack();
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ISTORE_1)         /* Store integer into second local variable */
        lp[1] = popStack();
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ISTORE_2)         /* Store integer into third local variable */
        lp[2] = popStack();
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ISTORE_3)         /* Store integer into fourth local variable */
        lp[3] = popStack();
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LSTORE_0)       /* Store long into first (zeroeth) local variable */
        lp[1] = popStack();
        lp[0] = popStack();
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LSTORE_1)         /* Store long into second local variable */
        lp[2] = popStack();
        lp[1] = popStack();
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LSTORE_2)         /* Store long into third local variable */
        lp[3] = popStack();
        lp[2] = popStack();
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LSTORE_3)         /* Store long into fourth local variable */
        lp[4] = popStack();
        lp[3] = popStack();
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(FSTORE_0)      /* Store float into first (zeroeth) local variable */
        lp[0] = popStack();
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(FSTORE_1)         /* Store float into second local variable */
        lp[1] = popStack();
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(FSTORE_2)         /* Store float into third local variable */
        lp[2] = popStack();
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(FSTORE_3)         /* Store float into fourth local variable */
        lp[3] = popStack();
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(DSTORE_0)     /* Store double into first (zeroeth) local variable */
        lp[1] = popStack();
        lp[0] = popStack();
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(DSTORE_1)         /* Store double into second local variable */
        lp[2] = popStack();
        lp[1] = popStack();
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(DSTORE_2)         /* Store double into third local variable */
        lp[3] = popStack();
        lp[2] = popStack();
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(DSTORE_3)         /* Store double into fourth local variable */
        lp[4] = popStack();
        lp[3] = popStack();
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ASTORE_0)    /* Store address into first (zeroeth) local variable */
        lp[0] = popStack();
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ASTORE_1)         /* Store address into second local variable */
        lp[1] = popStack();
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ASTORE_2)         /* Store address into third local variable */
        lp[2] = popStack();
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ASTORE_3)         /* Store address into fourth local variable */
        lp[3] = popStack();
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IASTORE)          /* Store into integer array */
        long      value = popStack();
        long      index = popStack();
        ARRAY thisArray = popStackAsType(ARRAY);
        CHECKARRAY(thisArray, index);
            thisArray->data[index].cell = value;
        ENDCHECKARRAY
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LASTORE)          /* Store into long array */
        cell  hiValue   = popStack();
        cell  loValue   = popStack();
        long  index     = popStack();
        ARRAY thisArray = popStackAsType(ARRAY);
        CHECKARRAY(thisArray, index);
            thisArray->data[index * 2].cell     = loValue;
            thisArray->data[index * 2 + 1].cell = hiValue;
        ENDCHECKARRAY
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(FASTORE)          /* Store into float array */
        long      value = popStack();
        long      index = popStack();
        ARRAY thisArray = popStackAsType(ARRAY);
        CHECKARRAY(thisArray, index);
            thisArray->data[index].cell = value;
        ENDCHECKARRAY
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(DASTORE)          /* Store into double array */
        cell  hiValue   = popStack();
        cell  loValue   = popStack();
        long  index     = popStack();
        ARRAY thisArray = popStackAsType(ARRAY);
        CHECKARRAY(thisArray, index);
            thisArray->data[index * 2].cell = loValue;
            thisArray->data[index * 2 + 1].cell = hiValue;
        ENDCHECKARRAY
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(AASTORE)          /* Store into object array */
        /* We cannot pop these off the stack, since a GC might happen
         * before we perform the assignment */
        OBJECT    value = topStackAsType(OBJECT);
        long      index = secondStack;
        ARRAY     thisArray = thirdStackAsType(ARRAY);
        int res;
        CLASS targetClass;
        CHECKARRAY(thisArray, index);
            targetClass = thisArray->ofClass->u.elemClass;
            if (value == NULL ||
                   isAssignableToFast(value->ofClass, targetClass)) {
                res = TRUE;
            } else {
                VMSAVE
                    /* This may cause a GC */
                    res = isAssignableTo(value->ofClass, targetClass);
                VMRESTORE
                /* GC may have occurred, need to update volatile args */
                value = topStackAsType(OBJECT);
                thisArray = thirdStackAsType(ARRAY);
            }
            lessStack(3);
            if (res) {
                thisArray->data[index].cellp = (cell*)value;
            } else {
                goto handleArrayStoreException;
            }
        ENDCHECKARRAY
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(BASTORE)          /* Store into byte array */
        long value = popStack();
        long index = popStack();
        BYTEARRAY thisArray = popStackAsType(BYTEARRAY);
        CHECKARRAY(thisArray, index);
            thisArray->bdata[index] = (char)value;
        ENDCHECKARRAY
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(CASTORE)          /* Store into UNICODE character (16-bit) array */
        long value = popStack();
        long index = popStack();
        SHORTARRAY thisArray = popStackAsType(SHORTARRAY);
        CHECKARRAY(thisArray, index);
            thisArray->sdata[index] = (short)value;
        ENDCHECKARRAY
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(SASTORE)          /* Store into short array */
        long value = popStack();
        long index = popStack();
        SHORTARRAY thisArray = popStackAsType(SHORTARRAY);
        CHECKARRAY(thisArray, index);
            thisArray->sdata[index] = (short)value;
        ENDCHECKARRAY
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(POP)              /* Pop top operand stack word */
        oneLess;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(POP2)             /* Pop two top operand stack words */
        lessStack(2);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(DUP)              /* Duplicate top operand stack word */
        long temp = topStack;
        pushStack(temp);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(DUP_X1)     /* Duplicate top operand stack word and put two down  */
        long a = topStack;
        long b = secondStack;
        secondStack = a;
        topStack    = b;
        pushStack(a);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(DUP_X2)    /* Duplicate top operand stack word and put three down */
        long a = topStack;
        long b = secondStack;
        long c = thirdStack;
        thirdStack  = a;
        secondStack = c;
        topStack    = b;
        pushStack(a);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(DUP2)             /* Duplicate top two operand stack words */
        long a = topStack;
        long b = secondStack;
        pushStack(b);
        pushStack(a);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(DUP2_X1)  /* Duplicate top two operand stack words and put three down */
        long a = topStack;
        long b = secondStack;
        long c = thirdStack;
        thirdStack  = b;
        secondStack = a;
        topStack    = c;
        pushStack(b);
        pushStack(a);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(DUP2_X2)  /* Duplicate top two operand stack words and put four down */
        long a = topStack;
        long b = secondStack;
        long c = thirdStack;
        long d = fourthStack;
        fourthStack = b;
        thirdStack  = a;
        secondStack = d;
        topStack    = c;
        pushStack(b);
        pushStack(a);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(SWAP)             /* Swap top two operand stack words */
        long a = topStack;
        long b = secondStack;
        topStack    = b;
        secondStack = a;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IADD)                             /* Add integer */
        long temp = popStack();
        *(long*)sp += temp;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LADD)                             /* Add long */
        long64 lvalue = GET_LONG(sp - 3);
        long64 rvalue = GET_LONG(sp - 1);
        ll_inc(lvalue, rvalue);
        SET_LONG(sp - 3, lvalue);
        lessStack(2);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(FADD)                             /* Add float */
        float temp = *(float*)sp--;
        *(float*)sp += temp;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(DADD)                             /* Add double */
        double rvalue = GET_DOUBLE(sp-1);
        double lvalue = GET_DOUBLE(sp-3);
        SET_DOUBLE(sp-3, lvalue + rvalue);
        lessStack(2);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ISUB)                             /* Sub integer */
        long temp = popStack();
        *(long*)sp -= temp;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LSUB)                             /* Sub long */
        long64 lvalue = GET_LONG(sp - 3);
        long64 rvalue = GET_LONG(sp - 1);
        ll_dec(lvalue, rvalue);
        SET_LONG(sp - 3, lvalue);
        lessStack(2);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(FSUB)                             /* Sub float */
        float temp = *(float*)sp--;
        *(float*)sp -= temp;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(DSUB)                             /* Sub double */
        double rvalue = GET_DOUBLE(sp-1);
        double lvalue = GET_DOUBLE(sp-3);
        SET_DOUBLE(sp-3, lvalue - rvalue);
        lessStack(2);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IMUL)                             /* Mul integer */
        long temp = popStack();
        *(long*)sp *= temp;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LMUL)                             /* Mul long */
        long64 rvalue = GET_LONG(sp - 1);
        long64 lvalue = GET_LONG(sp - 3);
        SET_LONG(sp - 3, ll_mul(lvalue, rvalue));
        lessStack(2);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(FMUL)                             /* Mul float */
        float temp = *(float*)sp--;
        *(float*)sp *= temp;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(DMUL)                             /* Mul double */
        double rvalue = GET_DOUBLE(sp-1);
        double lvalue = GET_DOUBLE(sp-3);

#if PROCESSOR_ARCHITECTURE_X86
        /* On x86, we have to do some additional magic */
        /* to get correct strictfp results.            */
        JFP_lib_dmul_x86(lvalue, rvalue);
#else
        SET_DOUBLE(sp - 3, lvalue * rvalue);
#endif
        lessStack(2);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IDIV)                             /* Div integer */
        long rValue = popStack();
        long lValue = topStack;
        if (rValue == 0) {
            goto handleArithmeticException;
        } else if (lValue == 0x80000000 && rValue == -1) {
            topStack = lValue;
        } else {
            topStack = lValue / rValue;
        }
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LDIV)                             /* Div long */
        long64 rvalue = GET_LONG(sp - 1);
        long64 lvalue = GET_LONG(sp - 3);
        if (ll_zero_eq(rvalue)) {
            goto handleArithmeticException;
        } else {
            SET_LONG(sp - 3, ll_div(lvalue, rvalue));
            lessStack(2);
        }
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(FDIV)                             /* Div float */
        float temp = *(float*)sp--;
        *(float*)sp /= temp;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(DDIV)                             /* Div double */
        double rvalue = GET_DOUBLE(sp-1);
        double lvalue = GET_DOUBLE(sp-3);

#if PROCESSOR_ARCHITECTURE_X86
        /* On x86, we have to do some additional magic */
        /* to get correct strictfp results.            */
        JFP_lib_ddiv_x86(lvalue, rvalue);
#else
        SET_DOUBLE(sp - 3,  lvalue / rvalue);
#endif
        lessStack(2);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IREM)                             /* Rem integer */
        long rValue = popStack();
        long lValue = topStack;

        if (rValue == 0) {
            goto handleArithmeticException;
        } else if (lValue == 0x80000000 && rValue == -1) {
            topStack = lValue % 1;
        } else {
            topStack = lValue % rValue;
        }
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LREM)                             /* Rem long */
        long64 rvalue = GET_LONG(sp - 1);
        long64 lvalue = GET_LONG(sp - 3);
        if (ll_zero_eq(rvalue)) {
            goto handleArithmeticException;
        } else {
            SET_LONG(sp - 3, ll_rem(lvalue, rvalue));
            lessStack(2);
        }
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(FREM)                             /* Rem float */
        float result;
        float rvalue = *(float*)sp--;
        float lvalue = *(float*)sp;
#if PROCESSOR_ARCHITECTURE_X86
        /* On x86, we have to do some additional magic */
        /* to get correct strictfp results.            */
        JFP_lib_frem_x86(lvalue, rvalue);
#else
        result = (float)DOUBLE_REMAINDER(lvalue, rvalue);
        *(float *)sp = result;
#endif
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(DREM)                             /* Rem double */
        double result;
        double rvalue = GET_DOUBLE(sp-1);
        double lvalue = GET_DOUBLE(sp-3);
#if PROCESSOR_ARCHITECTURE_X86
        /* On x86, we have to do some additional magic */
        /* to get correct strictfp results.            */
        JFP_lib_drem_x86(lvalue, rvalue);
#else
        result = DOUBLE_REMAINDER(lvalue, rvalue);
        SET_DOUBLE(sp-3,  result);
#endif
        lessStack(2);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(INEG)                             /* Negate integer */
        *(long*)sp = 0 - *(long*)sp;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LNEG)                             /* Negate long */
        long64 lvalue = GET_LONG(sp - 1);
        ll_negate(lvalue);
        SET_LONG(sp - 1, lvalue);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(FNEG)                             /* Negate float */
        *(float*)sp = *(float*)sp * -1.0f;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(DNEG)                             /* Negate double */
        double value = - GET_DOUBLE(sp-1);
        SET_DOUBLE(sp-1,  value);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ISHL)                     /* Arithmetic shift left integer */
        /* Note: shift range is limited to 31 (0x1F) */
        long s = popStack() & 0x0000001F;
        *(long*)sp <<= s;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LSHL)                     /* Arithmetic shift left long */
        /* Note: shift range is limited to 63 (0x3F) */
        long s = popStack() & 0x0000003F;
        long64 value = GET_LONG(sp - 1);
        SET_LONG(sp - 1, ll_shl(value, s));
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ISHR)                     /* Arithmetic shift right integer */
        /* Note: shift range is limited to 31 (0x1F) */
        long s = popStack() & 0x0000001F;

        /* Extends sign for negative numbers */
        *(long*)sp >>= s;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LSHR)                      /* Arithmetic shift right long */
        /* Note: shift range is limited to 63 (0x3F) */
        long s = popStack() & 0x0000003F;
        long64 value = GET_LONG(sp - 1);
        SET_LONG(sp - 1, ll_shr(value, s));
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IUSHR)                    /* Logical shift right integer */
        long s = popStack() & 0x0000001F;
        *(unsigned long*)sp >>= s;       /* No sign extension */
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LUSHR)                            /* Logical shift right long */
        long s = popStack() & 0x0000003F;
        long64 value = GET_LONG(sp - 1);
        SET_LONG(sp - 1, ll_ushr(value, s));
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IAND)                             /* Boolean integer AND */
        long temp = popStack();
        topStack &= temp;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LAND)                             /* Boolean long AND */
        thirdStack  &= topStack;
        fourthStack &= secondStack;
        lessStack(2);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IOR)                              /* Boolean integer OR */
        long temp = popStack();
        topStack |= temp;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LOR)                              /* Boolean long OR */
        thirdStack  |= topStack;
        fourthStack |= secondStack;
        lessStack(2);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IXOR)                             /* Boolean integer XOR */
        long temp = popStack();
        topStack ^= temp;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LXOR)                             /* Boolean long XOR */
        thirdStack  ^= topStack;
        fourthStack ^= secondStack;
        lessStack(2);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IINC)                             /* Increment local variable */
        unsigned int index = ip[1];
        long value = ((signed char *)ip)[2];
        lp[index] = ((long)(lp[index]) + value);
DONE(3)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(I2L)                              /* Convert integer to long */
        long value = *(long *)sp;
#if BIG_ENDIAN
        ((long *)sp)[1] = value;
        ((long *)sp)[0] = value >> 31;
#elif LITTLE_ENDIAN || !COMPILER_SUPPORTS_LONG
        ((long *)sp)[1] = value >> 31;
#else
        SET_LONG(sp, value);
#endif
        oneMore;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(I2F)                              /* Convert integer to float */
        *(float*)sp = (float)*(long*)sp;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(I2D)                              /* Convert integer to double */
        SET_DOUBLE(sp, (double)*(long*)sp);
        oneMore;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(L2I)                              /* Convert long to integer */
        oneLess;
#if BIG_ENDIAN
        sp[0] = sp[1];
#elif LITTLE_ENDIAN || !COMPILER_SUPPORTS_LONG
        /* do nothing */
#else
        sp[0] = (long)(GET_LONG(sp));
#endif
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(L2F)                              /* Convert long to float */
        oneLess;
        *(float*)sp = ll2float(GET_LONG(sp));
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(L2D)                              /* Convert long to double */
        double value = ll2double(GET_LONG(sp - 1));
        SET_DOUBLE(sp - 1, value);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(F2I)                              /* Convert float to integer */

/*
 * 0x4F000000 = ((float)0x80000000) magic number for Float any
 * number >= this number will be a special case
 */
        long value = *(long*)sp;
        if ((value & 0x7FFFFFFF) >= 0x4F000000) {
            if ((value >= F_L_POS_NAN && value <= F_H_POS_NAN) ||
                (value >= F_L_NEG_NAN && value <= F_H_NEG_NAN)) {
                *(long*)sp = 0;   /* NaN */
            } else if (value > 0) {
                *(long*)sp = MAX_INT;  /* +Infinity */
            } else if (value < 0) {
                *(long*)sp = MIN_INT;  /* -Infinity */
            }
        } else {
            *(long*)sp = (long)*(float*)sp;
        }
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(F2L)                              /* Convert float to long */

/*
 * 0x5F000000 = (0x4F000000 + 2E28) magic number for Float
 * any number >= this number will be a special case
 */
        long value = *(long*)sp;
        if ((value & 0x7FFFFFFF) >= 0x5F000000) {
            if ((value >= F_L_POS_NAN && value <= F_H_POS_NAN) ||
                (value >= F_L_NEG_NAN && value <= F_H_NEG_NAN)) {
                SET_LONG(sp, 0);   /* NaN */
            } else if (value > 0) {
                SET_LONG(sp, MAX_LONG);  /* +Infinity */
            } else if (value < 0) {
                SET_LONG(sp, MIN_LONG);  /* -Infinity */
            }
        } else {
            SET_LONG(sp, float2ll(*(float*)sp));
        }
        oneMore;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(F2D)                              /* Convert float to double */
        SET_DOUBLE(sp, (double)*(float*)sp);
        oneMore;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(D2I)                              /* Convert double to integer */

/*
 * 0x41E0000000000000L magic number for Float any number >= this number
 * will be a special case
 */
        long64 value;
        oneLess;
        value = GET_LONG(sp);

        if ((value & 0x7FFFFFFFFFFFFFFFL) >= 0x41E0000000000000L) {
            if ((value >= D_L_POS_NAN && value <= D_H_POS_NAN) ||
                (value >= D_L_NEG_NAN && value <= D_H_NEG_NAN)) {
                *(long*)sp = 0;   /* NaN */
            } else if (value > 0) {
                *(long*)sp = MAX_INT;  /* +Infinity */
            } else if (value < 0) {
                *(long*)sp = MIN_INT;  /* -Infinity */
            }
        } else {
            *(long*)sp = (long)GET_DOUBLE(sp);
        }
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(D2L)                              /* Convert double to long */

/*
 * 0x43E0000000000000L = (0x41E0000000000000L + 2e57) magic number
 * for Float any number >= this number will be a special case
 */
        long64 value = GET_LONG(sp - 1);
        if ((value & 0x7FFFFFFFFFFFFFFFL) >= 0x43E0000000000000L) {
            if ((value >= D_L_POS_NAN && value <= D_H_POS_NAN) ||
                (value >= D_L_NEG_NAN && value <= D_H_NEG_NAN)) {
                SET_LONG(sp - 1, 0);   /* NaN */
            } else if (value > 0) {
                SET_LONG(sp - 1, MAX_LONG);  /* +Infinity */
            } else if (value < 0) {
                SET_LONG(sp - 1, MIN_LONG);  /* -Infinity */
            }
        } else {
            long64 value = double2ll(GET_DOUBLE(sp - 1));
            SET_LONG(sp - 1, value);
        }
DONE(1)
#endif

/* --------------------------------------------------------------------- */
#if FLOATBYTECODES
SELECT(D2F)                              /* Convert double to float */
        oneLess;
        *(float*)sp = (float)GET_DOUBLE(sp);
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(I2B)             /* Convert integer to byte (with sign extension) */
        *(long*)sp = (signed char)*(long*)sp;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(I2C)              /* Convert integer to UNICODE character */
        *(long*)sp = (unsigned short)*(long*)sp;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(I2S)            /* Convert integer to short (with sign extension) */
        *(long*)sp = (short)*(long*)sp;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LCMP)                             /* Compare long */
        long result;
#if COMPILER_SUPPORTS_LONG
        long64 rvalue = GET_LONG(sp-1);
        long64 lvalue = GET_LONG(sp-3);
        result = lvalue < rvalue ? -1 : lvalue == rvalue ? 0 : 1;
#else
        long64 *rvalue = (long64*)(sp-1);
        long64 *lvalue = (long64*)(sp-3);

        signed long highLeft =  (signed long)(lvalue->high);
        signed long highRight = (signed long)(rvalue->high);
        if (highLeft == highRight) {
            unsigned long lowLeft =  (unsigned long)(lvalue->low);
            unsigned long lowRight = (unsigned long)(rvalue->low);
            result = (lowLeft < lowRight ? -1 : lowLeft > lowRight ? 1 : 0);
        } else {
            result = highLeft < highRight ? -1 : 1;
        }
#endif
        lessStack(3);
        *(long *)sp = result;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(FCMPL)                             /* Compare float */
        float rvalue = *(float*)sp;
        float lvalue = *(float*)(sp-1);
        oneLess;
#if PROCESSOR_ARCHITECTURE_X86
        /* On x86, we have to do some additional magic */
        /* to get correct strictfp results.            */
        JFP_lib_fcmpl_x86(lvalue, rvalue);
#else
        *(long*)sp = (lvalue >  rvalue) ?  1 :
                     (lvalue == rvalue) ?  0 : -1;
#endif
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(FCMPG)                             /* Compare float */
        float rvalue = *(float*)sp;
        float lvalue = *(float*)(sp-1);
        oneLess;
#if PROCESSOR_ARCHITECTURE_X86
        /* On x86, we have to do some additional magic */
        /* to get correct strictfp results.            */
        JFP_lib_fcmpg_x86(lvalue, rvalue);
#else
        *(long*)sp = (lvalue >  rvalue) ?  1 :
                     (lvalue == rvalue) ?  0 :
                     (lvalue <  rvalue) ? -1 : 1;
#endif
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(DCMPL)                             /* Compare double */
        double rvalue = GET_DOUBLE(sp-1);
        double lvalue = GET_DOUBLE(sp-3);
        lessStack(3);
#if PROCESSOR_ARCHITECTURE_X86
        /* On x86, we have to do some additional magic */
        /* to get correct strictfp results.            */
        JFP_lib_dcmpl_x86(lvalue, rvalue);
#else
        *(long*)sp = (lvalue >  rvalue) ?  1 :
                     (lvalue == rvalue) ?  0 : -1;
#endif
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if FLOATBYTECODES
SELECT(DCMPG)                             /* Compare double */
        double rvalue = GET_DOUBLE(sp-1);
        double lvalue = GET_DOUBLE(sp-3);
        lessStack(3);
#if PROCESSOR_ARCHITECTURE_X86
        /* On x86, we have to do some additional magic */
        /* to get correct strictfp results.            */
        JFP_lib_dcmpg_x86(lvalue, rvalue);
#else
        *(long*)sp = (lvalue >  rvalue) ?  1 :
                     (lvalue == rvalue) ?  0 :
                     (lvalue <  rvalue) ? -1 : 1;
#endif
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IFEQ)                             /* Branch if equal to zero */
        BRANCHIF( (popStack() == 0) )
DONEX
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IFNE)                            /* Branch if different than zero */
        BRANCHIF( (popStack() != 0) )
DONEX
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IFLT)                             /* Branch if less than zero */
        BRANCHIF( ((long)popStack() < 0) )
DONEX
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IFGE)                       /* Branch if greater or equal to zero */
        BRANCHIF( ((long)popStack() >= 0) )
DONEX
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IFGT)                             /* Branch if greater than zero */
        BRANCHIF( ((long)popStack() > 0) )
DONEX
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IFLE)                          /* Branch if less or equal to zero */
        BRANCHIF( ((long)popStack() <= 0) )
DONEX
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IF_ICMPEQ)                        /* Branch if equal */
        long b = popStack();
        long a = popStack();
        BRANCHIF( (a == b) )
DONEX
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IF_ICMPNE)                        /* Branch if not equal */
        long b = popStack();
        long a = popStack();
        BRANCHIF( (a != b) )
DONEX
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IF_ICMPLT)                        /* Branch if less than */
        long b = popStack();
        long a = popStack();
        BRANCHIF( (a < b) )
DONEX
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IF_ICMPGE)                        /* Branch if greater or equal */
        long b = popStack();
        long a = popStack();
        BRANCHIF( (a >= b) )
DONEX
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IF_ICMPGT)                        /* Branch if greater */
        long b = popStack();
        long a = popStack();
        BRANCHIF( (a > b) )
DONEX
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IF_ICMPLE)                        /* Branch if less or equal */
        long b = popStack();
        long a = popStack();
        BRANCHIF( (a <= b) )
DONEX
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IF_ACMPEQ)                      /* Branch if references are equal */
        long b = popStack();
        long a = popStack();
        BRANCHIF( (a == b) )
DONEX
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IF_ACMPNE)                  /* Branch if references are not equal */
        long b = popStack();
        long a = popStack();
        BRANCHIF( (a != b) )
DONEX
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(GOTO)                       /* Branch if references are not equal */
        ip += getShort(ip + 1);
DONE_R
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
NOTIMPLEMENTED(JSR)              /* Jump and stack return */
/***** JSR AND RET ARE NOT NEEDED IN KVM
SELECT(JSR)
        pushStackAsType(BYTE*, (ip + 3));
        ip += getShort(ip + 1);
DONE(0)
*****/
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
NOTIMPLEMENTED(RET)             /* Return from subroutine */
/***** JSR AND RET ARE NOT NEEDED IN KVM
SELECT(RET)
        unsigned int index = ip[1];
        ip = *(BYTE**)&lp[index];
DONE(0)
*****/
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(TABLESWITCH)             /* Access jump table by index and jump */
        long index = popStack();

        /* Align instruction pointer to the next 32-bit word boundary */
        unsigned char *base = (unsigned char *) (((long)ip + 4) & ~3);

        long lowValue = getAlignedCell(base + 4);
        long highValue = getAlignedCell(base + 8);
        if ((index < lowValue) || (index > highValue)) {
            ip += getAlignedCell(base); /* default value */
        } else {
            ip += getAlignedCell(base + CELL*(index-lowValue+3));
        }
DONE_R
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(LOOKUPSWITCH)          /* Access jump table by key match and jump */
        struct pair { cell value; cell offset; };
        long key = popStack();
        cell *base = (cell *)(((long)ip + 4) & ~3);
        long numberOfPairs = getAlignedCell(base + 1);
        long delta = numberOfPairs - 1;

        struct pair *firstPair = (struct pair *)(base + 2);
        for (;;) {
            /* ASSERT:  The item we are looking for, if it is in the table,
             * is in between firstPair and firstPair + delta, inclusive.
             */
            if (delta < 0) {
                /* Item not found, use default at base */
                break;
            } else {
                long halfDelta = (delta >> 1);
                struct pair *middlePair = firstPair + halfDelta;
                /* Don't use cell.  It's unsigned */
                long middleValue = getAlignedCell(&middlePair->value);
                if (middleValue < key) {
                    /* Item is second half of the table */
                    firstPair = middlePair + 1;
                    delta = delta - halfDelta - 1;
                } else if (middleValue > key) {
                    /* Item is at the beginning of the table. */
                    delta = halfDelta - 1;
                } else {
                    /* We have it.  Set base to point to the location of the
                     * offset.
                     */
                    base = &middlePair->offset;
                    break;
                }
            }
        }
        /* base points at the offset by which we increment the ip */
        ip += getAlignedCell(base);
DONE_R
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT6(IRETURN, LRETURN, FRETURN, DRETURN, ARETURN, RETURN)
        /* Return from method */
        BYTE*  previousIp    = fp->previousIp;
        OBJECT synchronized  = fp->syncObject;

        TRACE_METHOD_EXIT(fp->thisMethod);

        if (synchronized != NULL) {
            char*  exitError;
            if (monitorExit(synchronized, &exitError) == MonitorStatusError) {
                exception = exitError;
                goto handleException;
            }
        }

        /* Special case where we are killing a thread */
        if (previousIp == KILLTHREAD) {
            VMSAVE
            stopThread();
            VMRESTORE
            if (areAliveThreads()) {
                goto reschedulePoint;
            } else {
                return;                   /* Must be the end of the program */
            }
        }

        /* Regular case where we pop the stack frame and return data */
        if ((TOKEN & 1) == 0) {
            /* The even ones are all the return of a single value */
            cell data = topStack;
            POP_FRAME
            pushStack(data);
        } else if (TOKEN == RETURN) {
            POP_FRAME
        } else {
            /* We don't care whether it's little or big endian. . . */
            long t2 = sp[0];
            long t1 = sp[-1];
            POP_FRAME
            pushStack(t1);
            pushStack(t2);

        }
        goto reschedulePoint;
DONEX
#endif

/* --------------------------------------------------------------------- */

#if INFREQUENTSTANDARDBYTECODES
SELECT(GETSTATIC)                        /* Get static field from class */
        /* Get the CONSTANT_Fieldref index */
        unsigned int cpIndex;
        FIELD field;
        int currentToken = TOKEN;

        /* Get the constant pool index */
        cpIndex = getUShort(ip + 1);

        /* Resolve constant pool reference */
        VMSAVE
        field = resolveFieldReference(cp_global, cpIndex, TRUE, currentToken,
                                      fp_global->thisMethod->ofClass);
        VMRESTORE

       /*
        * If the class in question has not been initialized, do it now.
        * The system used in KVM is to push a special frame onto the
        * callers stack which is used to execute the CUSTOMCODE bytecode.
        * This is done repeatedly using an internal finite state machine
        * until all the stages are complete. When the class is initialized
        * the frame will be popped off and the calling frame will then
        * execute this bytecode again because the ip was not incremented.
        */
        if (field && field->ofClass->status == CLASS_ERROR) {
            VMSAVE
                raiseExceptionWithMessage(NoClassDefFoundError,
                    KVM_MSG_EXPECTED_INITIALIZED_CLASS);
            VMRESTORE
        }

        if (field && !CLASS_INITIALIZED(field->ofClass)) {
            VMSAVE
            /* Push class initialization frame */
            initializeClass(field->ofClass);
            VMRESTORE
            goto reschedulePoint;
        }

        if (field) {
            void *location = field->u.staticAddress;

#if ENABLEFASTBYTECODES
                REPLACE_BYTECODE(ip, ((field->accessFlags & ACC_DOUBLE)
                         ? GETSTATIC2_FAST
                         : (field->accessFlags & ACC_POINTER)
                         ? GETSTATICP_FAST : GETSTATIC_FAST))
#endif

            /* Load either one or two cells depending on field type */
            if (field->accessFlags & ACC_DOUBLE) {
                oneMore;
                COPY_LONG(sp, location);
                oneMore;
            } else {
                pushStack(*(cell *)location);
            }
        } else {
            VMSAVE
            fatalSlotError(cp, cpIndex);
            VMRESTORE
        }
DONE(3)
#endif

/* --------------------------------------------------------------------- */

#if INFREQUENTSTANDARDBYTECODES
SELECT(PUTSTATIC)                           /* Set static field in class */
        /* Get the CONSTANT_Fieldref index */
        unsigned int cpIndex;
        FIELD field;
        int currentToken = TOKEN;

        /* Get the constant pool index */
        cpIndex = getUShort(ip + 1);

        /* Resolve constant pool reference */
        VMSAVE
        field = resolveFieldReference(cp_global, cpIndex, TRUE, currentToken,
                                      fp_global->thisMethod->ofClass);
        VMRESTORE

       /*
        * If the class in question has not been initialized, do it now.
        * The system used in KVM is to push a special frame onto the
        * callers stack which is used to execute the CUSTOMCODE bytecode.
        * This is done repeatedly using an internal finite state machine
        * until all the stages are complete. When the class is initialized
        * the frame will be popped off and the calling frame will then
        * execute this bytecode again because the ip was not incremented.
        */
        if (field && field->ofClass->status == CLASS_ERROR) {
            VMSAVE
                raiseExceptionWithMessage(NoClassDefFoundError,
                    KVM_MSG_EXPECTED_INITIALIZED_CLASS);
            VMRESTORE
        }

        if (field && !CLASS_INITIALIZED(field->ofClass)) {
            VMSAVE
            /* Push class initialization frame */
            initializeClass(field->ofClass);
            VMRESTORE
            goto reschedulePoint;
        }

        if (field) {
            void *location = field->u.staticAddress;
#if ENABLEFASTBYTECODES
            REPLACE_BYTECODE(ip, ((field->accessFlags & ACC_DOUBLE)
                          ? PUTSTATIC2_FAST : PUTSTATIC_FAST))
#endif

            /* Store either one or two cells depending on field type */
            if (field->accessFlags & ACC_DOUBLE) {
                oneLess;
                COPY_LONG(location, sp);
                oneLess;
            }
            else *(cell *)location = popStack();
        } else {
            VMSAVE
            fatalSlotError(cp, cpIndex);
            VMRESTORE
        }
DONE(3)
#endif

/* --------------------------------------------------------------------- */

#if INFREQUENTSTANDARDBYTECODES
SELECT(GETFIELD)            /* Get field (instance variable) from object */
        /* Get the CONSTANT_Fieldref index */
        unsigned int cpIndex;
        FIELD field;
        int currentToken = TOKEN;

        /* Get the constant pool index */
        cpIndex = getUShort(ip + 1);

        /* Resolve constant pool reference */
        VMSAVE
        field = resolveFieldReference(cp_global, cpIndex, FALSE, currentToken,
                                      fp_global->thisMethod->ofClass);
        VMRESTORE
        if (field) {
            /* Get slot index */
            int offset = field->u.offset;
#if ENABLEFASTBYTECODES
            REPLACE_BYTECODE(ip, ((field->accessFlags & ACC_DOUBLE)
                          ? GETFIELD2_FAST
                          :  (field->accessFlags & ACC_POINTER)
                                   ? GETFIELDP_FAST : GETFIELD_FAST))
            putShort(ip + 1, offset);
            goto reschedulePoint;
#else
            INSTANCE instance = popStackAsType(INSTANCE);
            CHECK_NOT_NULL(instance);

            /* Load either one or two cells depending on field type */
            if (field->accessFlags & ACC_DOUBLE) {
                pushStack(instance->data[offset].cell);
                pushStack(instance->data[offset + 1].cell);
            } else {
                pushStack(instance->data[offset].cell);
            }
#endif
        } else {
            VMSAVE
            fatalSlotError(cp, cpIndex);
            VMRESTORE
        }
DONE(3)
#endif

/* --------------------------------------------------------------------- */

#if INFREQUENTSTANDARDBYTECODES
SELECT(PUTFIELD)                                 /* Set field in object */
        /* Get the CONSTANT_Fieldref index */
        unsigned int cpIndex;
        FIELD field;
        int currentToken = TOKEN;

        /* Get the constant pool index */
        cpIndex = getUShort(ip + 1);

        /* Resolve constant pool reference */
        VMSAVE
        field = resolveFieldReference(cp_global, cpIndex, FALSE, currentToken,
                                      fp_global->thisMethod->ofClass);
        VMRESTORE
        if (field) {

            /* Get slot index */
            int offset = field->u.offset;

#if ENABLEFASTBYTECODES
            /* If feasible, replace the current bytecode sequence
             * with a faster one
             */
            REPLACE_BYTECODE(ip, ((field->accessFlags & ACC_DOUBLE)
                      ? PUTFIELD2_FAST : PUTFIELD_FAST))
            putShort(ip + 1, offset);
            goto reschedulePoint;
#else
            /* Store either one or two cells depending on field type */
            if (field->accessFlags & ACC_DOUBLE) {
                cell  first       = popStack();
                cell  second      = popStack();
                INSTANCE instance = popStackAsType(INSTANCE);
                CHECK_NOT_NULL(instance);
                instance->data[offset].cell = second;
                instance->data[offset + 1].cell = first;
            } else {
                cell data         = popStack();
                INSTANCE instance = popStackAsType(INSTANCE);
                CHECK_NOT_NULL(instance)
                instance->data[offset].cell = data;
            }
#endif
        } else {
            VMSAVE
            fatalSlotError(cp, cpIndex);
            VMRESTORE
        }
DONE(3)
#endif

/* --------------------------------------------------------------------- */

#if INFREQUENTSTANDARDBYTECODES
SELECT(INVOKEVIRTUAL)
        /* Invoke instance method; dispatch based on dynamic class */

        /* Get the CONSTANT_Methodref index */
        unsigned int cpIndex;
        METHOD cpMethod;

        /* Get the constant pool index */
        cpIndex = getUShort(ip + 1);

        /* Resolve constant pool reference */
        VMSAVE
        cpMethod = resolveMethodReference(cp_global, cpIndex, FALSE,
                                          fp_global->thisMethod->ofClass);
        VMRESTORE
        if (cpMethod) {
            CLASS dynamicClass;

            /* Calculate the number of parameters  from signature */
            int argCount = cpMethod->argCount;

            /* Get this */
            thisObject = *(OBJECT*)(sp-argCount+1);
            CHECK_NOT_NULL(thisObject);

            /* Get the dynamic class of the object */
            dynamicClass = thisObject->ofClass;

            /* Find the actual method */
            VMSAVE
            thisMethod = lookupDynamicMethod(dynamicClass, cpMethod);
            VMRESTORE

            /*
             * In order for INVOKEVIRTUAL to be called, an instance
             * of the object must exist. Thus, the class initialization
             * is not needed since the class in question will already be 
             * initialized.
             */
            if (thisMethod) {

#if ENABLEFASTBYTECODES
                if (   (cpMethod->accessFlags & (ACC_PRIVATE | ACC_FINAL))
                    || (cpMethod->ofClass->clazz.accessFlags & ACC_FINAL)
                   ) {
                    REPLACE_BYTECODE(ip, INVOKESPECIAL_FAST)
                } else {
                    int iCacheIndex;
                    /* Replace the current bytecode sequence */
                    CREATE_CACHE_ENTRY((cell*)thisMethod, ip)
                    REPLACE_BYTECODE(ip, INVOKEVIRTUAL_FAST)
                    putShort(ip + 1, iCacheIndex);
                }
#endif /* ENABLEFASTBYTECODES */

                TRACE_METHOD_ENTRY(thisMethod, "virtual");

                CALL_VIRTUAL_METHOD
            } else {
                VMSAVE
                fatalSlotError(cp, cpIndex);
                VMRESTORE
            }
        } else {
            VMSAVE
            fatalSlotError(cp, cpIndex);
            VMRESTORE
        }
DONE(0)
#endif

/* --------------------------------------------------------------------- */

#if INFREQUENTSTANDARDBYTECODES
SELECT(INVOKESPECIAL)
        /* Invoke instance method; special handling for superclass, */
        /* private and instance initialization method invocations */

        /* Get the CONSTANT_Methodref index */
        unsigned int cpIndex;

        /* Get the constant pool index */
        cpIndex = getUShort(ip + 1);

        /* Resolve constant pool reference */
        VMSAVE
        thisMethod = resolveMethodReference(cp_global, cpIndex, FALSE,
                                            fp_global->thisMethod->ofClass);
        VMRESTORE

        /*
         * In order for INVOKESPECIAL to be called, an instance
         * of the object must exist. Thus, the class initialization
         * is not needed since the class in question will already be 
         * initialized.
         */
        if (thisMethod) {
            int argCount = thisMethod->argCount;

#if ENABLEFASTBYTECODES
            /* Replace the current bytecode sequence with inline
             * caches a la Deutch-Schiffman.
             */
            REPLACE_BYTECODE(ip, INVOKESPECIAL_FAST)
#endif
            thisObject = *(OBJECT*)(sp-argCount+1);
            CHECK_NOT_NULL(thisObject);

            TRACE_METHOD_ENTRY(thisMethod, "special");

            CALL_SPECIAL_METHOD
        } else {
            VMSAVE
            fatalSlotError(cp, cpIndex);
            VMRESTORE
        }
DONE(0)
#endif

/* --------------------------------------------------------------------- */

#if INFREQUENTSTANDARDBYTECODES
SELECT(INVOKESTATIC)                   /* Invoke a class (static) method */
        /* Get the CONSTANT_Methodref index */
        unsigned int cpIndex;

        /* Get the constant pool index */
        cpIndex = getUShort(ip + 1);

        /* Resolve constant pool reference */
        VMSAVE
        thisMethod = resolveMethodReference(cp_global, cpIndex, TRUE,
                                            fp_global->thisMethod->ofClass);
        VMRESTORE

       /*
        * If the class in question has not been initialized, do it now.
        * The system used in KVM is to push a special frame onto the
        * callers stack which is used to execute the CUSTOMCODE bytecode.
        * This is done repeatedly using an internal finite state machine
        * until all the stages are complete. When the class is initialized
        * the frame will be popped off and the calling frame will then
        * execute this bytecode again because the ip was not incremented.
        */
        if (thisMethod && thisMethod->ofClass->status == CLASS_ERROR) {
            VMSAVE
                raiseExceptionWithMessage(NoClassDefFoundError,
                    KVM_MSG_EXPECTED_INITIALIZED_CLASS);
            VMRESTORE
        }

        if (thisMethod && !CLASS_INITIALIZED(thisMethod->ofClass)) {
            VMSAVE
            /* Push class initialization frame */
            initializeClass(thisMethod->ofClass);
            VMRESTORE
            goto reschedulePoint;
        }

        if (thisMethod) {

#if ENABLEFASTBYTECODES
            REPLACE_BYTECODE(ip, INVOKESTATIC_FAST)
#endif
            TRACE_METHOD_ENTRY(thisMethod, "static");

            /* Indicate the object that is locked on for a monitor */
            thisObject = (OBJECT)thisMethod->ofClass;
            CALL_STATIC_METHOD
        } else {
            VMSAVE
            fatalSlotError(cp, cpIndex);
            VMRESTORE
        }
DONE(0)
#endif

/* --------------------------------------------------------------------- */

#if INFREQUENTSTANDARDBYTECODES
SELECT(INVOKEINTERFACE)         /* Invoke interface method */
        unsigned int cpIndex;
        unsigned int argCount;

        /* Get the constant pool index */
        cpIndex = getUShort(ip + 1);

        /* Get the argument count (specific to INVOKEINTERFACE bytecode) */
        argCount = ip[3];

        /* Resolve constant pool reference */
        VMSAVE
        thisMethod = resolveMethodReference(cp_global, cpIndex, FALSE,
                                            fp_global->thisMethod->ofClass);
        VMRESTORE
        if (thisMethod) {
            INSTANCE_CLASS dynamicClass;

            /* Get "this" */
            thisObject = *(OBJECT*)(sp-argCount+1);
            CHECK_NOT_NULL(thisObject);

            /* Get method table entry based on dynamic class */
            /* with given method name and signature */
            dynamicClass = ((INSTANCE)thisObject)->ofClass;
            VMSAVE
            thisMethod = lookupMethod((CLASS)dynamicClass,
                                      thisMethod->nameTypeKey,
                                      fp_global->thisMethod->ofClass);
            VMRESTORE
            if (thisMethod != NULL &&
                (thisMethod->accessFlags & (ACC_PUBLIC | ACC_STATIC)) == ACC_PUBLIC) {

#if ENABLEFASTBYTECODES
                /* Replace the current bytecode sequence */
                int iCacheIndex;
                CREATE_CACHE_ENTRY((cell*)thisMethod, ip)
                REPLACE_BYTECODE(ip, INVOKEINTERFACE_FAST)
                putShort(ip + 1, iCacheIndex);
#endif /* ENABLEFASTBYTECODES */
                TRACE_METHOD_ENTRY(thisMethod, "interface");
                CALL_INTERFACE_METHOD
            }
        }
        VMSAVE
        fatalSlotError(cp, cpIndex);
        VMRESTORE
DONE(0)

#endif

/* --------------------------------------------------------------------- */

NOTIMPLEMENTED(UNUSED_BA)

/* --------------------------------------------------------------------- */

#if INFREQUENTSTANDARDBYTECODES
SELECT(NEW)                             /* Create new object */
        /* Get the CONSTANT_Class index */
        unsigned int cpIndex = getUShort(ip + 1);

        /* Get the corresponding class pointer */
        INSTANCE_CLASS thisClass;
        INSTANCE newObject;

        VMSAVE
        thisClass = (INSTANCE_CLASS)resolveClassReference(cp_global, cpIndex,
                                                          fp_global->thisMethod->ofClass);
        VMRESTORE

       /*
        * If the class in question has not been initialized, do it now.
        * The system used in KVM is to push a special frame onto the
        * callers stack which is used to execute the CUSTOMCODE bytecode.
        * This is done repeatedly using an internal finite state machine
        * until all the stages are complete. When the class is initialized
        * the frame will be popped off and the calling frame will then
        * execute this bytecode again because the ip was not incremented.
        */
        if (thisClass->status == CLASS_ERROR) {
            VMSAVE
                raiseExceptionWithMessage(NoClassDefFoundError,
                    KVM_MSG_EXPECTED_INITIALIZED_CLASS);
            VMRESTORE
        }

        if (thisClass->clazz.accessFlags & (ACC_INTERFACE | ACC_ABSTRACT)) {
            VMSAVE
                raiseExceptionWithMessage(InstantiationError,
                    KVM_MSG_BAD_CLASS_CANNOT_INSTANTIATE);
            VMRESTORE

        /* CLDC 1.0:
        if (thisClass->status == CLASS_ERROR ||
            (thisClass->clazz.accessFlags & (ACC_INTERFACE | ACC_ABSTRACT)))
        {
            fatalError(KVM_MSG_BAD_CLASS_CANNOT_INSTANTIATE);
        */
        } else if (!CLASS_INITIALIZED(thisClass)) {
            VMSAVE
            /* Push class initialization frame */
            initializeClass(thisClass);
            VMRESTORE
            goto reschedulePoint;
        }

#if ENABLEFASTBYTECODES
        /*
         * Quicken bytecode only if there is no finalizer
         */
        if (thisClass->finalizer == NULL) {
            REPLACE_BYTECODE(ip, NEW_FAST)
        }
#endif

        VMSAVE
        newObject = instantiate(thisClass);
        VMRESTORE

        if (newObject != NULL) {
            if (thisClass->finalizer != NULL) {
                /* register finalizer for this object */
                registerCleanup((INSTANCE_HANDLE)&newObject,
                                (CleanupCallback)thisClass->finalizer);
            }
            pushStackAsType(INSTANCE, newObject);
            ip += 3;
        }
DONE_R
#endif

/* --------------------------------------------------------------------- */

#if INFREQUENTSTANDARDBYTECODES
SELECT(NEWARRAY)                              /* Create new array object */
        unsigned int arrayType = ip[1];
        long arrayLength = topStack;
        ARRAY_CLASS type;
        ARRAY result;
        VMSAVE
        type = PrimitiveArrayClasses[arrayType];
        result = instantiateArray(type, arrayLength);
        VMRESTORE
        if (result != NULL) {
            topStackAsType(ARRAY) = result;
            ip += 2;
        }
DONE_R
#endif

/* --------------------------------------------------------------------- */

#if INFREQUENTSTANDARDBYTECODES
SELECT(ANEWARRAY)                  /* Create new array of reference type */

        /* Get the CONSTANT_Class index */
        unsigned int cpIndex = getUShort(ip + 1);
        long  arrayLength = topStack;
        CLASS elemClass;
        ARRAY_CLASS thisClass;
        ARRAY result;

        /* Get the corresponding class pointer */
        VMSAVE
        elemClass =
           resolveClassReference(cp_global, cpIndex, fp_global->thisMethod->ofClass);
        thisClass = getObjectArrayClass(elemClass);
        VMRESTORE

#if ENABLEFASTBYTECODES
        {
            int iCacheIndex;
        /* Note that instantiateArray may change the ip on an error */
            CREATE_CACHE_ENTRY((cell*)thisClass, ip)
            REPLACE_BYTECODE(ip, ANEWARRAY_FAST)
            putShort(ip + 1, iCacheIndex);
        }
#endif /* ENABLEFASTBYTECODES */

        VMSAVE
        result = instantiateArray(thisClass, arrayLength);
        VMRESTORE
        if (result != NULL) {
            topStackAsType(ARRAY) = result;
            ip += 3;
        }
DONE_R
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ARRAYLENGTH)                     /* Get length of array */
        ARRAY thisArray = topStackAsType(ARRAY);
        CHECK_NOT_NULL(thisArray)
        topStack = thisArray->length;
DONE(1)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(ATHROW)                          /* Throw exception or error */
        THROWABLE_INSTANCE exception = popStackAsType(THROWABLE_INSTANCE);
        CHECK_NOT_NULL(exception)
        VMSAVE
            thisObjectGCSafe = (OBJECT)exception; /* Protect from GC */
#if PRINT_BACKTRACE
            if (exception->backtrace == NULL) {
                fillInStackTrace((THROWABLE_INSTANCE_HANDLE)&thisObjectGCSafe);
            }
#endif
            throwException((THROWABLE_INSTANCE_HANDLE)&thisObjectGCSafe);
            thisObjectGCSafe = NULL;
        VMRESTORE
DONE_R
#endif

/* --------------------------------------------------------------------- */

#if INFREQUENTSTANDARDBYTECODES
SELECT(CHECKCAST)               /* Check whether object is of given type */
        /* Get the CONSTANT_Class index */
        unsigned int cpIndex = getUShort(ip + 1);
        CLASS thisClass;
        OBJECT object;

        /* Get the corresponding class pointer */
        VMSAVE
        thisClass = resolveClassReference(cp_global, cpIndex,
                                          fp_global->thisMethod->ofClass);
        VMRESTORE

        /* Get the object */
        object = topStackAsType(OBJECT);

#if ENABLEFASTBYTECODES
        REPLACE_BYTECODE(ip, CHECKCAST_FAST)
#endif

        if (object != NULL) {
            int res;
            VMSAVE
                res = isAssignableTo(object->ofClass, thisClass);
            VMRESTORE
            if (!res) {
                goto handleClassCastException;
            }
        }
DONE(3)
#endif

/* --------------------------------------------------------------------- */

#if INFREQUENTSTANDARDBYTECODES
SELECT(INSTANCEOF)               /* Determine if object is of given type */
        /* Get the CONSTANT_Class index */
        unsigned int cpIndex = getUShort(ip + 1);
        CLASS thisClass;
        OBJECT object;

        /* Get the corresponding class pointer */
        VMSAVE
        thisClass = resolveClassReference(cp_global, cpIndex,
                                          fp_global->thisMethod->ofClass);
        VMRESTORE

        object = topStackAsType(OBJECT);

#if ENABLEFASTBYTECODES
        REPLACE_BYTECODE(ip, INSTANCEOF_FAST)
#endif

        if (object != NULL) {
            int res;
            VMSAVE
            res = isAssignableTo(object->ofClass, thisClass);
            VMRESTORE
            topStack = res;
        } else {
            topStack = FALSE;
        }

DONE(3)
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(MONITORENTER)                    /* Enter a monitor */
        /* Get the object pointer from stack */
        OBJECT object = popStackAsType(OBJECT);
        CHECK_NOT_NULL(object)
        ip++;                  /* Note monitorEnter can context switch */
        VMSAVE
        monitorEnter(object);
        VMRESTORE
DONE_R
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(MONITOREXIT)                     /* Exit a monitor */
        /* Get the object pointer from stack */
        OBJECT object = popStackAsType(OBJECT);
        char *exitFailure;
        CHECK_NOT_NULL(object)
        ip++; /* In case we context switch in monitorExit one day */
        if (monitorExit(object, &exitFailure) == MonitorStatusError) {
            exception = exitFailure;
            goto handleException;
        }
DONE_R
#endif

/* --------------------------------------------------------------------- */

#if INFREQUENTSTANDARDBYTECODES
SELECT(WIDE)    /* Extend local variable index by additional bytes */
        unsigned int index, wtoken, ipinc;

        wtoken = ip[1];
        index = getUShort(ip + 2);
        ipinc = 4;
        switch (wtoken) {
            case ILOAD: case FLOAD: case ALOAD:
                pushStack(lp[index]);
                break;
            case LLOAD: case DLOAD:
                pushStack(lp[index]);
                pushStack(lp[index+1]);
                break;
            case ISTORE: case FSTORE: case ASTORE:
                lp[index] = popStack();
                break;
            case LSTORE: case DSTORE:
                lp[index+1] = popStack();
                lp[index]   = popStack();
                break;
/********* JSR AND RET NOT NEEDED IN CLDC
            case RET:
                ip = *(BYTE**)&lp[index];
                ipinc = 0;
                break;
*********/
            case IINC: {
                int value = (int)getShort(ip + 4);
                lp[index] = ((long)(lp[index]) + value);
                ipinc = 6;
                break;
            }
            default:
                VMSAVE
                    raiseExceptionWithMessage(VerifyError,
                        KVM_MSG_ILLEGAL_WIDE_BYTECODE_EXTENSION);
                VMRESTORE
                break;
        }

        ip += ipinc;

DONE(0)
#endif

/* --------------------------------------------------------------------- */

#if INFREQUENTSTANDARDBYTECODES
SELECT(MULTIANEWARRAY)           /* Create new multidimensional array */
        unsigned int cpIndex;
        int dimensions;
        ARRAY_CLASS thisClass;
        ARRAY result;

        /* Get the CONSTANT_Class index */
        cpIndex = getUShort(ip + 1);

        /* Get the requested array dimensionality */
        dimensions = ip[3];

        /* Get the corresponding class pointer */
        VMSAVE
        thisClass = (ARRAY_CLASS)
            resolveClassReference(cp_global, cpIndex,
                                  fp_global->thisMethod->ofClass);
        VMRESTORE

#if ENABLEFASTBYTECODES
        REPLACE_BYTECODE(ip, MULTIANEWARRAY_FAST)
#endif

        /* Instantiate the array using a recursive algorithm */
        VMSAVE
        result = instantiateMultiArray(thisClass,
                                       (long*)(sp_global - dimensions + 1),
                                       dimensions);
        VMRESTORE
        if (result != NULL) {
            sp -= dimensions;
            pushStackAsType(ARRAY, result);
            ip += 4;
        }

DONE_R

#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IFNULL)                           /* Branch if reference is NULL */
        BRANCHIF( (popStack() == 0) )
DONEX
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(IFNONNULL)                    /* Branch if reference is not NULL */
        BRANCHIF( (popStack() != 0) )
DONEX
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(GOTO_W)                    /* Branch unconditionally (wide index) */
        ip += getCell(ip + 1);
DONE_R
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
NOTIMPLEMENTED(JSR_W)           /* Jump and stack return (wide index) */
/***** JSR AND RET NOT NEEDED IN CLDC
SELECT(JSR_W)
        pushStackAsType(BYTE*, (ip + 5));
        ip += getShort(ip + 1);
DONE(0)
*****/
#endif

/* --------------------------------------------------------------------- */

#if STANDARDBYTECODES
SELECT(BREAKPOINT)                      /* Breakpoint trap */
#if ENABLE_JAVA_DEBUGGER
        VMSAVE
        if (CurrentThread->needEvent) {
            /* After we send the event, CurrentThread could be null
             * so we must jump all the way back to the reschedule
             * point.  We'll eventually get back here
             */
            checkDebugEvent(CurrentThread);
            VMRESTORE
            goto reschedulePoint;
        }
        handleBreakpoint(CurrentThread);
        VMRESTORE
        goto reschedulePoint;
DONEX
#else
        fprintf(stdout, KVM_MSG_BREAKPOINT);
DONE(1)
#endif
#endif

/* --------------------------------------------------------------------- */

/*=========================================================================
 * End of Java bytecodes; bytecodes below are used for internal optimization
 *=======================================================================*/

/*=========================================================================
 * If FAST bytecodes are enabled, the system replaces official Java
 * bytecodes with faster bytecodes at runtime whenever applicable.
 *
 * Note: most of the FAST bytecodes are applicable only when accessing
 * methods and variables of the currently executing class, i.e., they
 * cannot be used for accessing fields and methods of other classes.
 *=======================================================================*/

/* --------------------------------------------------------------------- */

#if FASTBYTECODES
SELECT2(GETFIELD_FAST, GETFIELDP_FAST)
        /* Get single-word field (instance variable) from object */
        /* (fast version) */

        /* Get the field index (resolved earlier)    */
        /* relative to the data area of the instance */
        unsigned int index;
        INSTANCE instance;
        index = getShort(ip + 1);

        /* Load one word onto the operand stack */
        instance = popStackAsType(INSTANCE);
        CHECK_NOT_NULL(instance);
        pushStack(instance->data[index].cell);
DONE(3)
#endif

/* --------------------------------------------------------------------- */

#if FASTBYTECODES
SELECT(GETFIELD2_FAST)
        /* Get double-word field (instance variable) from object */
        /* (fast version) */

        /* Get the field index (resolved earlier) */
        /* relative to the data area of instance */
        unsigned int index;
        INSTANCE instance;

        /* Get the inline cache index */
        index = getShort(ip + 1);

        /* Load two words onto the operand stack */
        instance = popStackAsType(INSTANCE);
        CHECK_NOT_NULL(instance);
        pushStack(instance->data[index].cell);
        pushStack(instance->data[index + 1].cell);
DONE(3)
#endif

/* --------------------------------------------------------------------- */

#if FASTBYTECODES
SELECT(PUTFIELD_FAST)            /* Set single-word field (fast version) */
        /* Get the field index (resolved earlier) */
        /* relative to the data area of instance */
        unsigned int index;
        cell data;
        INSTANCE instance;
        index = getShort(ip + 1);

        /* Store one word from the operand stack */
        data     = popStack();
        instance = popStackAsType(INSTANCE);
        CHECK_NOT_NULL(instance);
        instance->data[index].cell = data;
DONE(3)
#endif

/* --------------------------------------------------------------------- */

#if FASTBYTECODES
SELECT(PUTFIELD2_FAST)           /* Set double-word field (fast version) */
        /* Get the field index (resolved earlier) */
        /* relative to the data area of instance */
        unsigned int index;
        cell first;
        cell second;
        INSTANCE instance;

        /* Get the inline cache index */
        index = getShort(ip + 1);

        /* Store two words from the operand stack */
        first    = popStack();
        second   = popStack();
        instance = popStackAsType(INSTANCE);
        CHECK_NOT_NULL(instance);
        instance->data[index].cell = second;
        instance->data[index + 1].cell = first;
DONE(3)
#endif

/* --------------------------------------------------------------------- */

#if FASTBYTECODES
SELECT2(GETSTATIC_FAST, GETSTATICP_FAST)
        /* Get single-word static field from class (fast version) */
        unsigned int cpIndex = getUShort(ip + 1);
        FIELD field = (FIELD)cp->entries[cpIndex].cache;

        if (!CLASS_INITIALIZED(field->ofClass)) {
            VMSAVE
            /* Push class initialization frame */
            initializeClass(field->ofClass);
            VMRESTORE
            goto reschedulePoint;
        }

        /* Push contents of the field onto the operand stack */
        pushStack(*(cell *)field->u.staticAddress);
DONE(3)
#endif

/* --------------------------------------------------------------------- */

#if FASTBYTECODES
SELECT(GETSTATIC2_FAST)
        /* Get double-word static field from class (fast version) */
        unsigned int cpIndex = getUShort(ip + 1);
        FIELD field = (FIELD)cp->entries[cpIndex].cache;

        if (!CLASS_INITIALIZED(field->ofClass)) {
            VMSAVE
            /* Push class initialization frame */
            initializeClass(field->ofClass);
            VMRESTORE
            goto reschedulePoint;
        }

        oneMore;
        COPY_LONG(sp, field->u.staticAddress);
        oneMore;
DONE(3)
#endif

/* --------------------------------------------------------------------- */

#if FASTBYTECODES
SELECT(PUTSTATIC_FAST)
        /* Set single-word static field in class (fast version) */
        unsigned int cpIndex = getUShort(ip + 1);
        FIELD field = (FIELD)cp->entries[cpIndex].cache;

        if (!CLASS_INITIALIZED(field->ofClass)) {
            VMSAVE
            /* Push class initialization frame */
            initializeClass(field->ofClass);
            VMRESTORE
            goto reschedulePoint;
        }

        *(cell *)field->u.staticAddress = popStack();
DONE(3)
#endif

/* --------------------------------------------------------------------- */

#if FASTBYTECODES
SELECT(PUTSTATIC2_FAST)
        /* Set double-word static field in class (fast version) */
        unsigned int cpIndex = getUShort(ip + 1);
        FIELD field = (FIELD)cp->entries[cpIndex].cache;

        if (!CLASS_INITIALIZED(field->ofClass)) {
            VMSAVE
            /* Push class initialization frame */
            initializeClass(field->ofClass);
            VMRESTORE
            goto reschedulePoint;
        }

        oneLess;
        COPY_LONG(field->u.staticAddress, sp);
        oneLess;
DONE(3)
#endif

/* --------------------------------------------------------------------- */

NOTIMPLEMENTED(UNUSED_D5)                   /* Not used */

/* --------------------------------------------------------------------- */

#if FASTBYTECODES
SELECT(INVOKEVIRTUAL_FAST)
        /* Invoke instance method; dispatch based on dynamic class */
        /* (fast version) */

        /* Get the inline cache index */
        unsigned int iCacheIndex;
        ICACHE   thisICache;
        INSTANCE_CLASS defaultClass;
        int      argCount;
        CLASS    dynamicClass;

        /* Get the inline cache index */
        iCacheIndex = getUShort(ip + 1);

        /* Get the inline cache entry */
        thisICache = GETINLINECACHE(iCacheIndex);

        /* Get the default method stored in cache */
        thisMethod = (METHOD)thisICache->contents;

        /* Get the class of the default method */
        defaultClass = thisMethod->ofClass;

        /* Get the object pointer ('this') from the operand stack */
        /* (located below the method arguments in the stack) */
        argCount = thisMethod->argCount;
        thisObject = *(OBJECT*)(sp-argCount+1);
        CHECK_NOT_NULL(thisObject);

        /* This may be different than the default class */
        dynamicClass = thisObject->ofClass;

        /* If the default class and dynamic class are the same, we can
         * just execute the method.  Otherwise a new lookup
         */
        if (dynamicClass != (CLASS)defaultClass) {
            /* Get method table entry based on dynamic class */
            VMSAVE
            thisMethod = lookupDynamicMethod(dynamicClass, thisMethod);
            VMRESTORE
            /* Update inline cache entry with the newly found method */
            thisICache->contents = (cell*)thisMethod;
            IncrInlineCacheMissCounter();
        }
        else IncrInlineCacheHitCounter();

        if (!thisMethod) {
            fatalIcacheMethodError(thisICache);
        } else {
            TRACE_METHOD_ENTRY(thisMethod, "fast virtual");
            CALL_VIRTUAL_METHOD
        }
DONEX
#endif

/* --------------------------------------------------------------------- */

#if FASTBYTECODES
SELECT(INVOKESPECIAL_FAST)
        /* Invoke instance method; special handling for superclass, */
        /* private and instance initialization method invocations */
        /* (fast version) */
        unsigned int cpIndex = getUShort(ip + 1);

        thisMethod = (METHOD)(cp->entries[cpIndex].cache);
        thisObject = *(OBJECT*)(sp-thisMethod->argCount+1);
        CHECK_NOT_NULL(thisObject)

        TRACE_METHOD_ENTRY(thisMethod, "fast special");
        CALL_SPECIAL_METHOD
DONEX
#endif

/* --------------------------------------------------------------------- */

#if FASTBYTECODES
SELECT(INVOKESTATIC_FAST)
        /* Invoke a class (static) method (fast version) */
        unsigned int cpIndex = getUShort(ip + 1);
        thisMethod = (METHOD)cp->entries[cpIndex].cache;

        if (!CLASS_INITIALIZED(thisMethod->ofClass)) {
            VMSAVE
            /* Push class initialization frame */
            initializeClass(thisMethod->ofClass);
            VMRESTORE
            goto reschedulePoint;
        }

        /* Get the class of the currently executing method */
        thisObject = (OBJECT)thisMethod->ofClass;

        TRACE_METHOD_ENTRY(thisMethod, "fast static");
        CALL_STATIC_METHOD
DONEX
#endif

/* --------------------------------------------------------------------- */

#if FASTBYTECODES
SELECT(INVOKEINTERFACE_FAST)
        /* Invoke interface method (fast version) */

        /* Get the inline cache index */
        unsigned int iCacheIndex;
        unsigned int   argCount;
        ICACHE   thisICache;
        INSTANCE_CLASS defaultClass;
        CLASS    dynamicClass;

        /* Get the inline cache index */
        iCacheIndex = getUShort(ip + 1);

        /* Get the argument count (specific to INVOKEINTERFACE bytecode) */
        argCount = ip[3];

        /* Get the inline cache entry */
        thisICache = GETINLINECACHE(iCacheIndex);

        /* Get the default method stored in cache */
        thisMethod = (METHOD)thisICache->contents;

        /* Get the class of the default method */
        defaultClass = thisMethod->ofClass;

        /* Get the object pointer ('this') from the operand stack */
        thisObject = *(OBJECT*)(sp-argCount+1);
        CHECK_NOT_NULL(thisObject);

        /* Get the runtime (dynamic) class of the object */
        dynamicClass = thisObject->ofClass;

        /* If the default class and dynamic class are the same, DONE(1) */
        if (dynamicClass != (CLASS)defaultClass) {
            /* Get method table entry based on dynamic class */
            VMSAVE
            thisMethod = lookupMethod(dynamicClass, thisMethod->nameTypeKey,
                                      fp_global->thisMethod->ofClass);
            VMRESTORE
            /* Update inline cache entry with the newly found method */
            thisICache->contents = (cell*)thisMethod;
            IncrInlineCacheMissCounter();
        } else {
            IncrInlineCacheHitCounter();
        }

        if (thisMethod == NULL ||
            ((thisMethod->accessFlags & (ACC_PUBLIC | ACC_STATIC)) != ACC_PUBLIC)) {
            fatalIcacheMethodError(thisICache);
        } else {
            TRACE_METHOD_ENTRY(thisMethod, "fast interface");
            CALL_INTERFACE_METHOD;
        }

DONEX
#endif

/* --------------------------------------------------------------------- */

#if FASTBYTECODES
SELECT(NEW_FAST)                     /* Create new object (fast version) */
        /* Get the inline cache index */
        unsigned int cpIndex = getUShort(ip + 1);
        INSTANCE_CLASS thisClass = (INSTANCE_CLASS)(cp->entries[cpIndex].clazz);
        INSTANCE newObject;

        if (!CLASS_INITIALIZED(thisClass)) {
            VMSAVE
            /* Push class initialization frame */
            initializeClass(thisClass);
            VMRESTORE
            goto reschedulePoint;
        }

        /* Instantiate */
        VMSAVE
        newObject = instantiate(thisClass);
        VMRESTORE
        if (newObject != NULL) {
            pushStackAsType(INSTANCE, newObject);
            ip += 3;
        }

DONE(0)
#endif

/* --------------------------------------------------------------------- */

#if FASTBYTECODES
SELECT(ANEWARRAY_FAST)
        /* Create new array of reference type (fast version) */

        ARRAY result;
        /* Get the inline cache index */
        unsigned int iCacheIndex = getUShort(ip + 1);

        /* Get the inline cache entry */
        ICACHE thisICache = GETINLINECACHE(iCacheIndex);
        /* Get the class pointer stored in cache */
        ARRAY_CLASS thisClass = (ARRAY_CLASS)thisICache->contents;
        long arrayLength = topStack;
        VMSAVE
        result = instantiateArray(thisClass, arrayLength);
        VMRESTORE
        if (result != NULL) {
            topStackAsType(ARRAY) = result;
            ip += 3;
        }
DONE(0)
#endif

/* --------------------------------------------------------------------- */

#if FASTBYTECODES
SELECT(MULTIANEWARRAY_FAST)
        /* Create new multidimensional array (fast version) */

        /* Get the inline cache index */
        unsigned int cpIndex;
        int dimensions;
        ARRAY_CLASS thisClass;
        ARRAY result;

        /* Get the inline cache index */
        cpIndex = getUShort(ip + 1);

        /* Get the requested array dimensionality */
        dimensions = ip[3];

        /* Get the class pointer stored in cache */
        thisClass = (ARRAY_CLASS)cp->entries[cpIndex].clazz;

        /* Instantiate the array using a recursive algorithm */
        VMSAVE
        result = instantiateMultiArray(thisClass,
                                       (long*)(sp_global - dimensions + 1),
                                       dimensions);
        VMRESTORE
        if (result != NULL) {
            sp -= dimensions;
            pushStackAsType(ARRAY, result);
            ip += 4;
        }
DONE(0)
#endif

/* --------------------------------------------------------------------- */

#if FASTBYTECODES
SELECT(CHECKCAST_FAST)
        /* Check whether object is of given type (fast version) */
        unsigned int cpIndex = getUShort(ip + 1);
        CLASS thisClass = cp->entries[cpIndex].clazz;
        OBJECT   object;

        /* If object reference is NULL, or if the object */
        /* is a proper subclass of the cpIndexed class,  */
        /* the operand stack remains unchanged; otherwise */
        /* throw exception */
        object = (OBJECT)topStack;

        if ((object != NULL)
               && !isAssignableToFast(object->ofClass, thisClass)) {
            int res;
            VMSAVE
                res = isAssignableTo(object->ofClass, thisClass);
            VMRESTORE
            if (!res) {
                goto handleClassCastException;
            }
        }

DONE(3)
#endif

/* --------------------------------------------------------------------- */

#if FASTBYTECODES
SELECT(INSTANCEOF_FAST)
        /* Determine if object is of given type (fast version) */
        unsigned int cpIndex = getUShort(ip + 1);
        CLASS thisClass = cp->entries[cpIndex].clazz;
        OBJECT object = (OBJECT)topStack;

        if (object == NULL) {
            topStack = FALSE;
        } else if (isAssignableToFast(object->ofClass, thisClass)) {
            topStack = TRUE;
        } else {
            int res;
            VMSAVE
                res = isAssignableTo(object->ofClass, thisClass);
            VMRESTORE
            topStack = res;
        }
DONE(3)
#endif

/*=========================================================================
 * End of FAST bytecodes
 *=======================================================================*/

#if INFREQUENTSTANDARDBYTECODES
SELECT(CUSTOMCODE)                             /* 0xDF */
        cell *stack = (cell*)(fp + 1);
        CustomCodeCallbackFunction func = *(CustomCodeCallbackFunction *)stack;
        /*
         * This frame may have been pushed for the sole purpose of
         * providing a separate frame for a native function or for
         * arguments passed to a Java method called from a native function
         * (e.g. initializeClass and Java_java_lang_Class_newInstance.
         * In this case, the function pointer stack slot will be NULL
         * and the only action taken when executing in this frame is to
         * pop and return to the calling frame.
         */
        if (func != NULL) {
            VMSAVE
            func(NULL);
            VMRESTORE
        }
        else {
            POP_FRAME
        }
DONE_R
#endif

/*=========================================================================
 * End of all bytecodes
 *=======================================================================*/

#if STANDARDBYTECODES

/*=========================================================================
 * Define slots to call infrequent routines
 *=======================================================================*/

#if !INFREQUENTSTANDARDBYTECODES
INFREQUENTROUTINE(GETSTATIC)
INFREQUENTROUTINE(PUTSTATIC)
INFREQUENTROUTINE(GETFIELD)
INFREQUENTROUTINE(PUTFIELD)
INFREQUENTROUTINE(INVOKEVIRTUAL)
INFREQUENTROUTINE(INVOKESPECIAL)
INFREQUENTROUTINE(INVOKESTATIC)
INFREQUENTROUTINE(INVOKEINTERFACE)
INFREQUENTROUTINE(NEW)
INFREQUENTROUTINE(NEWARRAY)
INFREQUENTROUTINE(ANEWARRAY)
INFREQUENTROUTINE(CHECKCAST)
INFREQUENTROUTINE(INSTANCEOF)
INFREQUENTROUTINE(WIDE)
INFREQUENTROUTINE(MULTIANEWARRAY)
INFREQUENTROUTINE(CUSTOMCODE)
#endif

#if IMPLEMENTS_FLOAT
INFREQUENTROUTINE(FCONST_0)
INFREQUENTROUTINE(FCONST_1)
INFREQUENTROUTINE(FCONST_2)
INFREQUENTROUTINE(DCONST_0)
INFREQUENTROUTINE(DCONST_1)
INFREQUENTROUTINE(FLOAD)
INFREQUENTROUTINE(DLOAD)
INFREQUENTROUTINE(FLOAD_0)
INFREQUENTROUTINE(FLOAD_1)
INFREQUENTROUTINE(FLOAD_2)
INFREQUENTROUTINE(FLOAD_3)
INFREQUENTROUTINE(DLOAD_0)
INFREQUENTROUTINE(DLOAD_1)
INFREQUENTROUTINE(DLOAD_2)
INFREQUENTROUTINE(DLOAD_3)
INFREQUENTROUTINE(FALOAD)
INFREQUENTROUTINE(DALOAD)
INFREQUENTROUTINE(FSTORE)
INFREQUENTROUTINE(DSTORE)
INFREQUENTROUTINE(FSTORE_0)
INFREQUENTROUTINE(FSTORE_1)
INFREQUENTROUTINE(FSTORE_2)
INFREQUENTROUTINE(FSTORE_3)
INFREQUENTROUTINE(DSTORE_0)
INFREQUENTROUTINE(DSTORE_1)
INFREQUENTROUTINE(DSTORE_2)
INFREQUENTROUTINE(DSTORE_3)
INFREQUENTROUTINE(FASTORE)
INFREQUENTROUTINE(DASTORE)
INFREQUENTROUTINE(FADD)
INFREQUENTROUTINE(DADD)
INFREQUENTROUTINE(FSUB)
INFREQUENTROUTINE(DSUB)
INFREQUENTROUTINE(FMUL)
INFREQUENTROUTINE(DMUL)
INFREQUENTROUTINE(FDIV)
INFREQUENTROUTINE(DDIV)
INFREQUENTROUTINE(FREM)
INFREQUENTROUTINE(DREM)
INFREQUENTROUTINE(FNEG)
INFREQUENTROUTINE(DNEG)
INFREQUENTROUTINE(I2F)
INFREQUENTROUTINE(I2D)
INFREQUENTROUTINE(L2F)
INFREQUENTROUTINE(L2D)
INFREQUENTROUTINE(F2I)
INFREQUENTROUTINE(F2L)
INFREQUENTROUTINE(F2D)
INFREQUENTROUTINE(D2I)
INFREQUENTROUTINE(D2L)
INFREQUENTROUTINE(D2F)
INFREQUENTROUTINE(FCMPL)
INFREQUENTROUTINE(FCMPG)
INFREQUENTROUTINE(DCMPL)
INFREQUENTROUTINE(DCMPG)

#else /* IMPLEMENTS_FLOAT */

/*=========================================================================
 * Define slots to pad out the switch table
 *=======================================================================*/

NOTIMPLEMENTED(FCONST_0)
NOTIMPLEMENTED(FCONST_1)
NOTIMPLEMENTED(FCONST_2)
NOTIMPLEMENTED(DCONST_0)
NOTIMPLEMENTED(DCONST_1)
NOTIMPLEMENTED(FLOAD)
NOTIMPLEMENTED(DLOAD)
NOTIMPLEMENTED(FLOAD_0)
NOTIMPLEMENTED(FLOAD_1)
NOTIMPLEMENTED(FLOAD_2)
NOTIMPLEMENTED(FLOAD_3)
NOTIMPLEMENTED(DLOAD_0)
NOTIMPLEMENTED(DLOAD_1)
NOTIMPLEMENTED(DLOAD_2)
NOTIMPLEMENTED(DLOAD_3)
NOTIMPLEMENTED(FALOAD)
NOTIMPLEMENTED(DALOAD)
NOTIMPLEMENTED(FSTORE)
NOTIMPLEMENTED(DSTORE)
NOTIMPLEMENTED(FSTORE_0)
NOTIMPLEMENTED(FSTORE_1)
NOTIMPLEMENTED(FSTORE_2)
NOTIMPLEMENTED(FSTORE_3)
NOTIMPLEMENTED(DSTORE_0)
NOTIMPLEMENTED(DSTORE_1)
NOTIMPLEMENTED(DSTORE_2)
NOTIMPLEMENTED(DSTORE_3)
NOTIMPLEMENTED(FASTORE)
NOTIMPLEMENTED(DASTORE)
NOTIMPLEMENTED(FADD)
NOTIMPLEMENTED(DADD)
NOTIMPLEMENTED(FSUB)
NOTIMPLEMENTED(DSUB)
NOTIMPLEMENTED(FMUL)
NOTIMPLEMENTED(DMUL)
NOTIMPLEMENTED(FDIV)
NOTIMPLEMENTED(DDIV)
NOTIMPLEMENTED(FREM)
NOTIMPLEMENTED(DREM)
NOTIMPLEMENTED(FNEG)
NOTIMPLEMENTED(DNEG)
NOTIMPLEMENTED(I2F)
NOTIMPLEMENTED(I2D)
NOTIMPLEMENTED(L2F)
NOTIMPLEMENTED(L2D)
NOTIMPLEMENTED(F2I)
NOTIMPLEMENTED(F2L)
NOTIMPLEMENTED(F2D)
NOTIMPLEMENTED(D2I)
NOTIMPLEMENTED(D2L)
NOTIMPLEMENTED(D2F)
NOTIMPLEMENTED(FCMPL)
NOTIMPLEMENTED(FCMPG)
NOTIMPLEMENTED(DCMPL)
NOTIMPLEMENTED(DCMPG)
#endif /* IMPLEMENTS_FLOAT */

#if !FASTBYTECODES
NOTIMPLEMENTED(GETFIELD_FAST)
NOTIMPLEMENTED(GETFIELDP_FAST)
NOTIMPLEMENTED(GETFIELD2_FAST)
NOTIMPLEMENTED(PUTFIELD_FAST)
NOTIMPLEMENTED(PUTFIELD2_FAST)
NOTIMPLEMENTED(GETSTATIC_FAST)
NOTIMPLEMENTED(GETSTATICP_FAST)
NOTIMPLEMENTED(GETSTATIC2_FAST)
NOTIMPLEMENTED(PUTSTATIC_FAST)
NOTIMPLEMENTED(PUTSTATIC2_FAST)
NOTIMPLEMENTED(INVOKEVIRTUAL_FAST)
NOTIMPLEMENTED(INVOKESPECIAL_FAST)
NOTIMPLEMENTED(INVOKESTATIC_FAST)
NOTIMPLEMENTED(INVOKEINTERFACE_FAST)
NOTIMPLEMENTED(NEW_FAST)
NOTIMPLEMENTED(ANEWARRAY_FAST)
NOTIMPLEMENTED(MULTIANEWARRAY_FAST)
NOTIMPLEMENTED(CHECKCAST_FAST)
NOTIMPLEMENTED(INSTANCEOF_FAST)
#endif /* !FASTBYTECODES */

NOTIMPLEMENTED(224)
NOTIMPLEMENTED(225)
NOTIMPLEMENTED(226)
NOTIMPLEMENTED(227)
NOTIMPLEMENTED(228)
NOTIMPLEMENTED(229)
NOTIMPLEMENTED(230)
NOTIMPLEMENTED(231)
NOTIMPLEMENTED(232)
NOTIMPLEMENTED(233)
NOTIMPLEMENTED(234)
NOTIMPLEMENTED(235)
NOTIMPLEMENTED(236)
NOTIMPLEMENTED(237)
NOTIMPLEMENTED(238)
NOTIMPLEMENTED(239)
NOTIMPLEMENTED(240)
NOTIMPLEMENTED(241)
NOTIMPLEMENTED(242)
NOTIMPLEMENTED(243)
NOTIMPLEMENTED(244)
NOTIMPLEMENTED(245)
NOTIMPLEMENTED(246)
NOTIMPLEMENTED(247)
NOTIMPLEMENTED(248)
NOTIMPLEMENTED(249)
NOTIMPLEMENTED(250)
NOTIMPLEMENTED(251)
NOTIMPLEMENTED(252)
NOTIMPLEMENTED(253)
NOTIMPLEMENTED(254)
NOTIMPLEMENTED(255)

#endif /* STANDARDBYTECODES */

