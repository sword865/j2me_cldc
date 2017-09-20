/*
 *    StackMapFrame.java    1.4    01/03/30
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package components;
import java.io.DataOutput;
import java.io.DataInput;
import java.io.IOException;
import util.DataFormatException;
import vm.Const;

public class StackMapFrame { 
    // A single frame in the stack map table

    public static final int ITEM_Bogus = 0;
    public static final int ITEM_Integer = 1;
    public static final int ITEM_Float = 2;
    public static final int ITEM_Double = 3;
    public static final int ITEM_Long = 4;
    public static final int ITEM_Null = 5;
    public static final int ITEM_InitObject = 6;
    public static final int ITEM_Object = 7;
    public static final int ITEM_NewObject = 8;

    int offset;
    ConstantObject locals[];
    ConstantObject stack[];

    // Read in a frame from the specified data input stream.
    StackMapFrame(DataInput in, ConstantObject globals[]) 
                 throws IOException { 
       this.offset = in.readShort();            
       this.locals = readOneTable(in, globals);
       this.stack = readOneTable(in, globals);
    }

    private ConstantObject[] 
    readOneTable(DataInput in, ConstantObject globals[]) 
          throws IOException { 
    int length = in.readShort();
    ConstantObject result[] = new ConstantObject[length];
    for (int i = 0; i < length; i++) { 
        int tag = in.readByte();
        switch(tag) { 
        case ITEM_Bogus:
        case ITEM_Integer: case ITEM_Float:
        case ITEM_Double:  case ITEM_Long:
        case ITEM_Null:    case ITEM_InitObject:
        // Small non-negative values represent the corresponding
        // type.
        result[i] = new SingleValueConstant(tag);
        break;

        case ITEM_Object:
        result[i] = globals[in.readShort()];
        break;

        case ITEM_NewObject:
        // This is a hack, but it's not really worth creating a new
        // type to represent this.  Negative numbers represent new
        // objects.
        result[i] = new SingleValueConstant(~in.readShort());
        break;

        default:
        System.out.println("Unknown tag");
        }
    }
    return result;
    }

    public int writeData(DataOutput out) throws IOException {
    out.writeShort(offset);
    // Return the number of bytes that we've written
    return 2 + writeData(out, locals) + writeData(out, stack);
    }

    public int writeData(DataOutput out, ConstantObject[] table) 
       throws IOException 
    { 
    int count = 0;
    out.writeShort(table.length);
    count += 2;
    for (int i = 0; i < table.length; i++) { 
        if (table[i] instanceof SingleValueConstant) { 
        int value = ((SingleValueConstant)table[i]).value;
        if (value < 0) { 
            out.writeByte(ITEM_NewObject);
            out.writeShort(~value);
            count += 3;
        } else { 
            out.writeShort(value);        
            count += 2;
        }
        } else if (table[i] instanceof ClassConstant) { 
        out.writeByte(ITEM_Object);
        out.writeShort(((ClassConstant)table[i]).name.index);
        count += 3;
        }
    }
    return count;
    }

    public void
    externalize( ConstantPool p ){
    // do nothing
    }

    public void countConstantReferences(boolean isRelocatable) { 
    if (isRelocatable) { 
        // Is this the right thing??
        for (int i = 0; i < locals.length; i++) { 
        locals[i].incReference();
        }
        for (int i = 0; i < stack.length; i++) { 
        stack[i].incReference();
        }
    }
    }
    
    // Return a bitmap indicating which values on the stack are pointers
    public boolean[] getStackBitmap() { 
    return getBitmap(stack);
    }

    public int getStackSize() { 
        ConstantObject table[] = stack;
        int extra = 0;
    for (int i = 0; i < table.length; i++) { 
        if (table[i] instanceof SingleValueConstant) { 
        int value = ((SingleValueConstant)table[i]).value;
        if (value == ITEM_Long || value == ITEM_Double) { 
            extra++;
        }
        }
    } 
    return table.length + extra;
    }

    // Return a bitmap indicating which values on the locals are pointers
    public boolean[] getLocalsBitmap() {
    return getBitmap(locals);
    }

    private boolean[] getBitmap(ConstantObject table[]) { 
    int extra = 0;
    // Count the actual number of values, since doubles and longs take up 
    // two slots
    for (int i = 0; i < table.length; i++) { 
        if (table[i] instanceof SingleValueConstant) { 
        int value = ((SingleValueConstant)table[i]).value;
        if (value == ITEM_Long || value == ITEM_Double) { 
            extra++;
        }
        }
    } 
    boolean result[] = new boolean[table.length + extra];
    extra = 0;
    // Fill in the table
    for (int i = 0; i < table.length; i++) { 
        if (table[i] instanceof SingleValueConstant) { 
        int value = ((SingleValueConstant)table[i]).value;
        if (value == ITEM_Long || value == ITEM_Double) { 
            extra++;
        } else if (value < 0 || value == ITEM_InitObject) { 
            result[i + extra] = true;
        }
        } else if (table[i] instanceof ClassConstant) { 
        result[i + extra] = true;
        } else { 
        throw new RuntimeException("Unknown table type");
        }
    }
    return result;
    }

    public int getOffset() { 
    return offset;
    }
}
