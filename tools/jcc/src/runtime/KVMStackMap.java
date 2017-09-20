/*
 *        KVMStackMap.java        1.4        01/02/02        SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package runtime;

import components.*;
import jcc.Util;
import vm.EVMMethodInfo;
import util.*;
import java.io.ByteArrayOutputStream;
import vm.Const;

/*
 * The CoreImageWriter for the Embedded VM
 */

public class KVMStackMap implements Const { 
    KVMWriter    writer;
    KVMNameTable nameTable;

    public KVMStackMap(KVMWriter writer, KVMNameTable nameTable) { 
    this.writer = writer;
    this.nameTable = nameTable;
    }

    private long
    frameToLong(MethodInfo mi, StackMapFrame frame) { 
    boolean[] locals = frame.getLocalsBitmap();
    boolean[] stack = frame.getStackBitmap();
    int maxLocals = mi.locals;

    long stackMap = 0;    // this gives us 64 bits

    for (int i = 0; i < locals.length; i++) { 
        if (locals[i]) { 
        stackMap |= 1L << i;
        }
    }
    for (int i = 0; i < stack.length; i++) { 
        if (stack[i]) { 
        stackMap |= 1L << (i + maxLocals);
        }
    }
    return stackMap;
    }

    private String frameToString(MethodInfo mi, StackMapFrame frame) {
    long stackMap = frameToLong(mi, frame);
    StringBuffer sb = new StringBuffer();
    sb.append((char)frame.getStackSize());
    while (stackMap != 0) { 
        sb.append((char)(stackMap & 0xFF));
        stackMap = stackMap >>> 8;
    }
    String result = sb.toString();
    return result;
    }

    public void initialPass(EVMMethodInfo meth) { 
    MethodInfo mi = meth.method;
    StackMapFrame frames[] = mi.stackMapTable;
    if (frames != null) { 
            // removeExtraStackMaps(meth);
            if (!useShortStackMaps(meth)) { 
                for (int i = 0; i < frames.length; i++) { 
                    nameTable.getKey(frameToString(mi, frames[i]));    
                }
            }
        }
    }

    void showStatistics() { 
        if (removeKeep + removeBut + removeSame > 0) { 
            System.out.println("Total: " + 
                               (removeKeep +  removeBut + removeSame));
            System.out.println("Keep:  " + (removeKeep));
            System.out.println("Toss:  " + (removeSame));
            System.out.println("But:   " + (removeBut));
            System.out.println("Long stack maps: " + longStackMaps);
        }
    }

    void printDeclaration(CCodeWriter out, EVMMethodInfo meth, String prefix) 
    {
    String methodNativeName = meth.getNativeName();
    MethodInfo mi = meth.method;
    out.println(prefix + "struct { /* " 
            + mi.parent.className + ": " + writer.prettyName(mi) + "*/");
    out.println(prefix + "\tunsigned short length;");
    if (useShortStackMaps(meth)) { 
        out.println(prefix + "\tstruct { unsigned short offset; "
            + "unsigned char map[2]; } frame["
            + mi.stackMapTable.length + "];");
    } else { 
        out.println(prefix + "\tstruct { unsigned short offset; "
            + "unsigned short info; } frame["
            + mi.stackMapTable.length + "];");
    }
    out.println(prefix + "} " + methodNativeName + ";");
    }

    void printDefinition(CCodeWriter out, EVMMethodInfo meth, String prefix)
    {
    MethodInfo mi = meth.method;
    StackMapFrame frames[] = mi.stackMapTable;
    String pprefix = prefix + "\t";
    out.println(prefix + "{ /* " + mi.parent.className + ": " 
            + writer.prettyName(mi) + "*/");
    boolean useShort = useShortStackMaps(meth);
    
    out.println(pprefix + frames.length + (useShort ? " | STACK_MAP_SHORT_ENTRY_FLAG" : "") + ",");
    out.println(pprefix + "{");
    for (int i = 0; i < frames.length; i++) { 
        StackMapFrame frame = frames[i];
        int offset = frame.getOffset();
        out.print(pprefix + "\t\t");
        printComment(out, meth, frame);
        out.println();
        if (useShort) { 
        int map = (int)frameToLong(mi, frame);
        out.print(pprefix + "\t{" + offset + " + (" + 
              frame.getStackSize() + " << 12), { ");
        out.printHexInt(map & 0xFF);
        out.print(", ");
        out.printHexInt((map >> 8) & 0xFF);
        out.print(" }");
        } else { 
        String string = frameToString(mi, frame);
        int key = nameTable.getKey(string);
        out.print(pprefix + "\t{" + offset + ", " );
        out.printHexInt(key);
        }
        out.print("}");
        if (i < frames.length - 1) { 
        out.println(",");
        } else { 
        out.println();
        }
    }
    out.println(pprefix + "}");
    out.println(prefix + "},");
    }

    java.util.Hashtable useShortStackMapsCache = new java.util.Hashtable();
    static int longStackMaps;

    public boolean useShortStackMaps(EVMMethodInfo meth) { 
        Boolean value = (Boolean)useShortStackMapsCache.get(meth);
        if (value != null) { 
            return value.booleanValue();
        } else { 
            boolean result = useShortStackMapsInternal(meth.method);
            if (result) { 
                useShortStackMapsCache.put(meth, Boolean.TRUE);
            } else { 
                useShortStackMapsCache.put(meth, Boolean.FALSE);
                longStackMaps++;
            }
            return result;
        }
    }

    boolean useShortStackMapsInternal(MethodInfo mi) { 
        if (mi.code.length > 1023) { 
            return false;
        } else if (mi.stack <= 15 && mi.locals + mi.stack <= 16) { 
            /* This is almost always they case */
            return true;
        } else { 
            StackMapFrame frames[] = mi.stackMapTable;
            for (int i = 0; i < frames.length; i++) { 
                if (frames[i].getStackSize() > 15) { 
                    return false;
                }
                if (frameToLong(mi, frames[i]) > 65535) { 
                    return false;
                }
            }

            return true;
    }
    }

    String frameToComment(MethodInfo mi, StackMapFrame frame) { 
    StringBuffer sb = new StringBuffer();
    boolean[] locals = frame.getLocalsBitmap();
    boolean[] stack = frame.getStackBitmap();
    int maxLocals = mi.locals;
    int maxStack = mi.stack;

    sb.append("Locals: ");
    for (int i = 0; i < locals.length; i++) { 
        sb.append(locals[i] ? 'X' : '-');
    }
    for (int i = locals.length; i < maxLocals; i++) { 
        sb.append('.');
    }
    sb.append("; Stack: ");
    for (int i = 0; i < stack.length; i++) { 
        sb.append(stack[i] ? 'X' : '-');
    }
    return sb.toString();
    }

    private void 
    printComment(CCodeWriter out, EVMMethodInfo meth, StackMapFrame frame) 
    { 
    MethodInfo mi = meth.method;
    boolean[] locals = frame.getLocalsBitmap();
    boolean[] stack = frame.getStackBitmap();
    int maxLocals = mi.locals;
    int maxStack = mi.stack;
    out.print("/* Locals: ");
    if (maxLocals == 0) { 
        out.print("<None>");
    } else { 
        for (int i = 0; i < locals.length; i++) { 
        out.write(locals[i] ? 'X' : '-');
        }
        for (int i = locals.length; i < maxLocals; i++) { 
        out.write('.');
        }
    }
    out.print("; Stack: ");
    if (stack.length == 0) { 
        out.print("<None>");
    } else { 
        for (int i = 0; i < stack.length; i++) { 
        out.write(stack[i] ? 'X' : '-');
        }
    } 
    out.print(" */");

    }
    
    /* This is some experimental code that we're not yet using.
     * It allows us to delete some unneeded stack maps from the code.
     */

    private boolean removeVerbose = false;
    private int removeKeep, removeBut, removeSame;

    private void
    removeExtraStackMaps(EVMMethodInfo meth) { 
    MethodInfo mi = meth.method;
        ClassInfo ci = mi.parent;
        ConstantObject cpool[] = ci.constants;
    StackMapFrame frames[] = mi.stackMapTable;

        boolean[] locals = new boolean[mi.locals];
        boolean[] stack = new boolean[mi.stack];

        boolean[] discardList = new boolean[frames.length];
        int discardCount = 0;

        byte[] code = mi.code;

        int stackSize = 0;
        int thisIP = 0;
        boolean needStackmap = false;
        int currentFrameIndex = 0;

        if (removeVerbose) { 
            System.out.println("Method: " + 
                               ci.className + "." + writer.prettyName(mi));
        }

        initializeLocals(mi, locals);

        for (thisIP = 0; thisIP < code.length; ) { 
            
            StackMapFrame currentFrame = null;

            boolean[] newStack = null;
            boolean[] newLocals = null;
            
            if (currentFrameIndex < frames.length) { 
                if (frames[currentFrameIndex].getOffset() < thisIP) { 
                    throw new RuntimeException("Missed stack frame??");
                } else if (frames[currentFrameIndex].getOffset() == thisIP) {
                    currentFrame = frames[currentFrameIndex];
                    newStack = currentFrame.getStackBitmap();
                    boolean[] temp = currentFrame.getLocalsBitmap();
                    if (temp.length == locals.length) { 
                        newLocals = temp;
                    } else { 
                        newLocals = new boolean[mi.locals];
                        System.arraycopy(temp, 0, newLocals, 0, temp.length);
                    } 
                }
            }
            
            if (removeVerbose) { 
                if (currentFrame != null || thisIP == 0) { 
                    System.out.print("    Old: ");
                    for (int i = 0; i < locals.length; i++) { 
                        System.out.print(locals[i] ? 'X' : '-');
                    }
                    System.out.print(' ');
                    for (int i = 0; i < stackSize; i++) { 
                        System.out.print(stack[i] ? 'X' : '-');
                    }
                    System.out.println();
                }
                if (currentFrame != null) { 
                    System.out.print("    New: ");
                    for (int i = 0; i < newLocals.length; i++) { 
                        System.out.print(newLocals[i] ? 'X' : '-');
                    }
                    System.out.print(' ');
                    for (int i = 0; i < newStack.length; i++) { 
                        System.out.print(newStack[i] ? 'X' : '-');
                    }
                }
            }
            
            if (needStackmap) { 
                if (currentFrame == null) { 
                    throw new RuntimeException("Expected stack map at " 
                                               + thisIP);
                }
            } 
            
            if (currentFrame != null) { 
                if (!needStackmap && (newStack.length != stackSize)) { 
                    throw new RuntimeException("Inconsistent stack size at " + 
                                               thisIP);
                }
                boolean same = sameStackMaps(locals, stack, stackSize, 
                                             newLocals, newStack);
                if (!same) { 
                    removeKeep++;
                } else if (needStackmap) { 
                    removeBut++;
                } else { 
                    removeSame++;
                    discardCount++;
                    discardList[currentFrameIndex] = true;
                }
                if (removeVerbose) { 
                    System.out.print(same ? "=" : "#");
                    System.out.println(needStackmap ? "+" : "-");
                }
            }

            if (currentFrame != null) { 
                locals = newLocals;
                stackSize = newStack.length;
                System.arraycopy(newStack, 0, stack, 0, stackSize);
                currentFrameIndex++;
            }

            needStackmap = false;

            if (removeVerbose) { 
                System.out.print(thisIP + ": " + 
                                 mi.disassemble(thisIP, thisIP + 1));
                /* Note, that the println is at the end */
            }

            int token = code[thisIP++] & 0xFF;
            int index;

            switch(token) {

                /* Store a single non pointer from the stack to a register */
            case opc_istore: case opc_fstore:
                index = code[thisIP++] & 0xFF;
                locals[index] = false;
                stackSize--;
                break;

                /* Store a single pointer from the stack to a register */
            case opc_astore:
                index = code[thisIP++] & 0xFF;
                locals[index] = true;
                stackSize--;
                break;
                
                /* Store a double or long from the stack to a register */
            case opc_lstore: case opc_dstore:
                index = code[thisIP++] & 0xFF;
                locals[index] = false;
                locals[index+1] = false;
                stackSize -= 2;
                break;

            case opc_istore_0: case opc_istore_1: 
            case opc_istore_2: case opc_istore_3:
                index = token - opc_istore_0;
                locals[index] = false;
                stackSize--;
                break;

            case opc_lstore_0: case opc_lstore_1: 
            case opc_lstore_2: case opc_lstore_3:
                index = token - opc_lstore_0;
                locals[index] = false;
                locals[index+1] = false;
                stackSize -= 2;
                break;

            case opc_fstore_0: case opc_fstore_1: 
            case opc_fstore_2: case opc_fstore_3:
                index = (token - opc_fstore_0);
                locals[index] = false;
                stackSize--;
                break;

            case opc_dstore_0: case opc_dstore_1: 
            case opc_dstore_2: case opc_dstore_3:
                index = (token - opc_dstore_0);
                locals[index] = false;
                locals[index+1] = false;
                stackSize -= 2;
                break;

            case opc_astore_0: case opc_astore_1: 
            case opc_astore_2: case opc_astore_3:
                index = (token - opc_astore_0);
                locals[index] = true;
                stackSize--;
                break;

                /* These leave any pointers on the stack as pointers, and
                 * any nonpointers on the stack as nonpointers */
            case opc_iinc: case opc_checkcast: 
                thisIP += 2;
            case opc_nop:
            case opc_ineg: case opc_lneg: case opc_fneg: case opc_dneg:
            case opc_i2f:  case opc_l2d:  case opc_f2i:  case opc_d2l:
            case opc_i2b:  case opc_i2c:  case opc_i2s:
                break;

                /* These push a non-pointer onto the stack */
            case opc_sipush:
                thisIP++;
            case opc_iload:  case opc_fload:  case opc_bipush:
                thisIP++;
            case opc_aconst_null:
            case opc_iconst_m1: case opc_iconst_0: case opc_iconst_1:
            case opc_iconst_2:  case opc_iconst_3: case opc_iconst_4:
            case opc_iconst_5:
            case opc_fconst_0:  case opc_fconst_1: case opc_fconst_2:
            case opc_iload_0:   case opc_iload_1:  
            case opc_iload_2:   case opc_iload_3:
            case opc_fload_0:   case opc_fload_1:  
            case opc_fload_2:   case opc_fload_3:
            case opc_i2l: case opc_i2d: case opc_f2l: case opc_f2d:
                stack[stackSize++] = false;
                break;

                /* These push two non-pointers onto the stack */
            case opc_ldc2_w:
                thisIP++;
            case opc_lload:    case opc_dload:
                thisIP++;
            case opc_lconst_0: case opc_lconst_1: 
            case opc_dconst_0: case opc_dconst_1:
            case opc_lload_0:  case opc_lload_1:  
            case opc_lload_2:  case opc_lload_3:
            case opc_dload_0:  case opc_dload_1: 
            case opc_dload_2:  case opc_dload_3:
                stack[stackSize++] = false;
                stack[stackSize++] = false;
                break;

                /* These push a pointer onto the stack */
            case opc_new: 
                thisIP++;
            case opc_aload:
                thisIP++;
            case opc_aload_0:  case opc_aload_1:  
            case opc_aload_2: case opc_aload_3:
                stack[stackSize++] = true;
                break;

                /* These pop an item off the stack */
            case opc_ifeq: case opc_ifne: case opc_iflt: case opc_ifge:
            case opc_ifgt: case opc_ifle: case opc_ifnull: case opc_ifnonnull:
                thisIP += 2;
            case opc_pop:   case opc_iadd: case opc_fadd: case opc_isub: 
            case opc_fsub:  case opc_imul: case opc_fmul: case opc_idiv: 
            case opc_fdiv:  case opc_irem: case opc_frem: case opc_ishl: 
            case opc_lshl:  case opc_ishr: case opc_lshr: case opc_iushr: 
            case opc_lushr: case opc_iand: case opc_ior:  case opc_ixor:
            case opc_l2i:   case opc_l2f:  case opc_d2i:  case opc_d2f:
            case opc_fcmpl: case opc_fcmpg:
            case opc_monitorenter: case opc_monitorexit:
            case opc_aaload:        /* Ptr Int => Ptr */
                stackSize--;
                break;

                /* These pop an item off the stack, and then push a pointer */
            case opc_anewarray:
                thisIP++;
            case opc_newarray:
                thisIP++;
                stack[stackSize - 1] = true;
                break;

                /* These pop an item off the stack, and then push an int */
            case opc_instanceof:
                thisIP += 2;
            case opc_arraylength:
                stack[stackSize - 1] = false;
                break;

                /* These pop two items off the stack */
            case opc_if_icmpeq: case opc_if_icmpne: case opc_if_icmplt: 
            case opc_if_icmpge: case opc_if_icmpgt: case opc_if_icmple: 
            case opc_if_acmpeq: case opc_if_acmpne: 
                thisIP += 2;
            case opc_pop2:
            case opc_ladd: case opc_dadd: case opc_lsub: 
            case opc_dsub: case opc_lmul: case opc_dmul:
            case opc_ldiv: case opc_ddiv: case opc_lrem: case opc_drem:
            case opc_land: case opc_lor: case opc_lxor:
                stackSize -= 2;
                break;

                /* These pop two items off, and then push non-pointer */
            case opc_iaload: case opc_faload: case opc_baload: 
            case opc_caload: case opc_saload:
                stackSize -= 2;
                stack[stackSize++] = false;
                break;

                /* These pop two items off, and then push two non pointers */
            case opc_daload: case opc_laload:
                stack[stackSize - 1] = false;
                stack[stackSize - 2] = false;
                break;

                /* These pop three items off the stack. */
            case opc_iastore: case opc_fastore: case opc_aastore: 
            case opc_bastore: case opc_castore: case opc_sastore:
            case opc_lcmp:    case opc_dcmpl:   case opc_dcmpg:
                stackSize -= 3;
                break;

                /* These pop four items off the stack. */
            case opc_lastore: case opc_dastore:
                stackSize -= 4;
                break;

                /* These either load a pointer or an integer */
            case opc_ldc:
            case opc_ldc_w:
                if (token == opc_ldc) { 
                    index = code[thisIP++] & 0xFF;
                } else { 
                    index = mi.getUnsignedShort(thisIP);
                    thisIP += 2;
                }
                stack[stackSize++] = 
                    (cpool[index] instanceof StringConstant);
                break;
                
                /* These involve doing bit twiddling */
            case opc_dup:
                stackSize++;
                stack[stackSize - 1] = stack[stackSize - 2];
                break;

            case opc_dup_x1:
                stackSize++;
                stack[stackSize - 1] = stack[stackSize - 2];
                stack[stackSize - 2] = stack[stackSize - 3];
                stack[stackSize - 3] = stack[stackSize - 1];
                break;

            case opc_dup_x2:
                stackSize++;
                stack[stackSize - 1] = stack[stackSize - 2];
                stack[stackSize - 2] = stack[stackSize - 3];
                stack[stackSize - 3] = stack[stackSize - 4];
                stack[stackSize - 4] = stack[stackSize - 1];
                break;

            case opc_dup2:
                stackSize += 2;
                stack[stackSize - 1] = stack[stackSize - 3];
                stack[stackSize - 2] = stack[stackSize - 4];
                break;

            case opc_dup2_x1:
                stackSize += 2;
                stack[stackSize - 1] = stack[stackSize - 3];
                stack[stackSize - 2] = stack[stackSize - 4];
                stack[stackSize - 3] = stack[stackSize - 5];
                stack[stackSize - 4] = stack[stackSize - 1];
                stack[stackSize - 5] = stack[stackSize - 2];
                break;

            case opc_dup2_x2:
                stackSize += 2;
                stack[stackSize - 1] = stack[stackSize - 3];
                stack[stackSize - 2] = stack[stackSize - 4];
                stack[stackSize - 3] = stack[stackSize - 5];
                stack[stackSize - 4] = stack[stackSize - 6];
                stack[stackSize - 5] = stack[stackSize - 1];
                stack[stackSize - 6] = stack[stackSize - 2];
                break;

            case opc_swap: { 
                boolean temp = stack[stackSize - 1];
                stack[stackSize - 1] = stack[stackSize - 2];
                stack[stackSize - 2] = temp;
                break;
            }

            case opc_getfield:
                /* Remove the pointer to the object */
                stackSize -= 1;
            case opc_getstatic: {
                index = mi.getUnsignedShort(thisIP);
                thisIP += 2;
                FieldInfo field = ((FieldConstant)cpool[index]).find();
                if (removeVerbose) { 
                    System.out.print(" " + writer.prettyName(field));
                }
                String type = field.type.string;
                switch(field.type.string.charAt(0)) { 
                    case SIGC_CLASS: case SIGC_ARRAY:
                        stack[stackSize++] = true;
                        break;
                    case SIGC_LONG: case SIGC_DOUBLE:
                        stack[stackSize++] = false;
                    default:
                        stack[stackSize++] = false;
                        break;
                }
                break;
            }

                /* Set a field value from the stack */
            case opc_putfield:
                /* Remove the pointer to the object */
                stackSize -= 1;
            case opc_putstatic: { 
                index = mi.getUnsignedShort(thisIP);
                thisIP += 2;
                FieldInfo field = ((FieldConstant)cpool[index]).find();
                if (removeVerbose) { 
                    System.out.print(" " + writer.prettyName(field));
                }
                String type = field.type.string;
                switch(field.type.string.charAt(0)) { 
                    case SIGC_LONG: case SIGC_DOUBLE: 
                        stackSize -= 2;
                        break;
                    default:
                        stackSize -= 1;
                        break;
                }
                break;
            }

            case opc_multianewarray:
                stackSize -= (code[thisIP + 2] & 0xFF);
                thisIP += 3;
                stack[stackSize++] = true;
                break;

            case opc_wide:
                token = code[thisIP++] & 0xFF;
                index = mi.getUnsignedShort(thisIP); thisIP += 2;
                switch(token) {
                    case opc_lload: case opc_dload:
                        stack[stackSize++] = false;
                    case opc_iload: case opc_fload: case opc_aload:
                        stack[stackSize++] = (token == opc_aload);
                        break;

                    case opc_lstore: case opc_dstore:
                        locals[index + 1] = false;
                        stackSize--;
                    case opc_istore: case opc_fstore: case opc_astore:
                        locals[index] = (token == opc_astore);
                        stackSize--;
                        break;

                    case opc_iinc:
                        thisIP += 2;
                        break;

                    case opc_ret:
                    default: 
                        throw new RuntimeException("Unexpected byte code");
                }
                break;
                

            case opc_invokevirtual:
            case opc_invokespecial:
            case opc_invokestatic:
            case opc_invokeinterface: {
                index = mi.getUnsignedShort(thisIP);
                MethodInfo method = ((MethodConstant)cpool[index]).find();
                if (removeVerbose) { 
                    System.out.print(" " + writer.prettyName(method));
                }
                String type = method.type.string;
                thisIP += (token == opc_invokeinterface) ? 4 : 2;
                stackSize -= method.argsSize;
                int paren = type.indexOf(')');
                switch (type.charAt(paren + 1)) { 
                    case SIGC_CLASS: case SIGC_ARRAY:
                        stack[stackSize++] = true;
                        break;
                    case SIGC_LONG: case SIGC_DOUBLE:
                        stack[stackSize++] = false; 
                    default:
                        stack[stackSize++] = false;
                    case SIGC_VOID:
                        break;
                }
                break;
            }
            
            case opc_lookupswitch:
            case opc_tableswitch: 
                thisIP = (thisIP + 3) & ~3; // round up to multiple of 4
                if (token == opc_tableswitch) { 
                    int low = mi.getInt(thisIP + 4);
                    int high = mi.getInt(thisIP + 8);
                    thisIP += (high - low + 1) * 4 + 12;
                } else { 
                    int pairs = mi.getInt(thisIP + 4);
                    thisIP += pairs * 8 + 8;
                }
                stackSize--;    // pop int off the stack
                needStackmap = true;
                break;

            case opc_goto:
                thisIP += 2;
                needStackmap = true;
                break;

        case opc_goto_w:
                thisIP += 4;
                needStackmap = true;
                break;

            case opc_lreturn: case opc_dreturn:
                stackSize--;
            case opc_athrow: 
            case opc_ireturn: case opc_freturn: case opc_areturn:
                stackSize--;
            case opc_return:
                needStackmap = true;
                break;

                /* The KVM doesn't allow these.  But if it did, they should
                 * be treated the same as goto, since that's almost always
                 * what's there for the next instruction.
                 */
        case opc_jsr: 
        case opc_jsr_w:
            case opc_ret:
            default:
                throw new RuntimeException("Unexpected byte code at " + thisIP);

            }
            if (removeVerbose) { 
                System.out.println();
            }
        } // end of for loop;
        if (discardCount > 0) { 
            StackMapFrame newFrames[] = 
                new StackMapFrame[frames.length - discardCount];
            int i, j;
            for (i = j = 0; i < frames.length; i++) { 
                if (!discardList[i]) { 
                    newFrames[j++] = frames[i];
                }
            }
            if (j != newFrames.length) { 
                throw new RuntimeException("I cannot code");
            }
            mi.stackMapTable = newFrames;
        }
    }

    private void
    initializeLocals(MethodInfo mi, boolean locals[]) { 
        int argsSize = 0;
        String sig = mi.type.string;

        if (!mi.isStaticMember()) { 
            locals[argsSize++] = true;
        }

    for (int pos = 1; sig.charAt(pos) !=SIGC_ENDMETHOD; pos++) {
        switch (sig.charAt(pos)) {
          case SIGC_BOOLEAN: case SIGC_BYTE:   case SIGC_CHAR:    
              case SIGC_SHORT:  case SIGC_INT:     case SIGC_FLOAT:

          argsSize++;
          break;

          case SIGC_LONG: case SIGC_DOUBLE:
          argsSize += 2;
          break;
                  
          case SIGC_CLASS:  
                  locals[argsSize++] = true;
                  while (sig.charAt(pos) != SIGC_ENDCLASS) {
                      pos++;
                  }
                  break;

              case SIGC_ARRAY:
                  locals[argsSize++] = true;
                  while (sig.charAt(pos) == SIGC_ARRAY) {
                      pos++;
                  }

                  if (sig.charAt(pos) == SIGC_CLASS) {
                      while (sig.charAt(pos) != SIGC_ENDCLASS) {
                          pos++;
                      }
                  }
                  break;

             default:
                 System.err.println("Error: unparseable signature: " + sig);
                 System.exit(3);
        }
    }
    }

    private boolean 
    sameStackMaps(boolean[] locals, boolean[] stack, int stackSize, 
                  boolean[] newLocals, boolean [] newStack)
    { 
        if (stackSize != newStack.length) { 
            return false;
        } 
        if (!java.util.Arrays.equals(locals, newLocals)) { 
            return false;
        }
        for (int i = 0; i < stackSize; i++) { 
            if (stack[i] != newStack[i]) { 
                return false;
            }
        }
        return true;
    }

}
