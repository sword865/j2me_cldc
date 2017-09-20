/*
 *    KVMStringTable.java    1.10    00/09/11
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package runtime;
import java.util.Enumeration;
import vm.Const;
import components.*;
import vm.*;

public class KVMStringTable extends vm.StringTable {
    static final public String charArrayName =  "stringCharArrayInternal";
    static final public String stringArrayName = "stringArrayInternal";
    
    StringHashTable stringHashTable = new StringHashTable();

    final int writeStrings(KVMWriter writer, String name){
    writeCharacterArray(writer);
    writeStringInfo(writer);
    stringHashTable.writeTable(writer.out, name);
    return internedStringCount();
    }

    private boolean writeCharacterArray(KVMWriter writer) {
    final CCodeWriter out = writer.out;
    int n = arrangeStringData();
    if ( n == 0 ) { 
        return false; // nothing to do here.
    }
    final char v[] = new char[n];
    data.getChars( 0, n, v, 0 );

    // First, typedef the header we're about to create
    out.print("static CONST CHARARRAY_X(" + n + ") " 
          + charArrayName + " = {\n");
    
    out.println("\tCHARARRAY_HEADER(" + v.length + "),");
    out.println("\t{");
    writer.writeArray(v.length, 10, "\t\t", 
              new KVMWriter.ArrayPrinter() { 
                              public void print(int index) { 
                  char c = v[index];
                  if (c == '\\') {
                      out.print("'\\\\'");
                  } else if (c == '\'') {
                      out.print("'\\''");
                  } else if (c >= ' ' && c <= 0x7E) { 
                      out.print("'" + c + "' ");
                  } else { 
                      out.printHexInt(c);
                  }
                  }
                      });
    out.println("\t}");
    out.println("};\n");
    return true;
    }

    private void writeStringInfo(KVMWriter writer) {
    CCodeWriter out = writer.out;
    int count = internedStringCount();

    StringConstant stringTable[] = new StringConstant[count];
    
    for (Enumeration e = allStrings(); e.hasMoreElements(); ) { 
        StringConstant s = (StringConstant)e.nextElement();
        stringTable[ s.unicodeIndex ] = s;
    }
        
    out.println("static CONST struct internedStringInstanceStruct " + 
            stringArrayName + "[" + count + "] = {");
    for (int i = 0; i < count; i++) { 
        StringConstant s = stringTable[i];
        String string = s.str.string;
        StringConstant next = 
           (StringConstant)stringHashTable.getNext(s);

        commentary(string, out);
        out.print("\tKVM_INIT_JAVA_STRING(" + s.unicodeOffset + ", "
              + string.length() + ", ");
        if (next == null) { 
        out.print(0);
        } else { 
        writeString(out, next);
        } 
        out.println("), ");
    }
    out.println("};\n\n");
    }

    public void writeString(CCodeWriter out, StringConstant s) { 
    if (s.unicodeIndex < 0) { 
        System.out.println("String not in stringtable: \"" + s.str.string + "\"");
    }
    out.write('&');
    out.print(stringArrayName);
    out.write('[');
    out.print(s.unicodeIndex);
    out.write(']');        
    }

    /* Overwrite intern in vm.StringTable */
    public void intern( StringConstant s ) { 
    super.intern(s);
    stringHashTable.addEntry(s);
    }

    private final static int maxCom = 20;

    private void commentary( String s, CCodeWriter out ) {
    out.print("\t\t/* ");
    if (s.length() > maxCom) { 
        s = s.substring(0, maxCom - 3) + "...";
    } 
    out.printSafeString(s);
    out.print(" */\n");
    }

    class StringHashTable extends KVMHashtable { 
    
    StringHashTable() { 
        super(32, StringConstant.class); 
    }
    
    long hash(Object x) { 
        StringConstant sc = (StringConstant) x;
        String str = sc.str.string;
        return stringHash(str);
    }

    Object tableChain(CCodeWriter out, int bucket, Object[] list) {
        return (list.length == 0) ? null : list[0];
    }
    
    void tableEntry(CCodeWriter out, int bucket, Object token ) { 
        StringConstant s = (StringConstant)token;
        if (s == null) { 
        out.print("NULL");
        } else { 
        writeString(out, s);
        }
    }
    }
}
