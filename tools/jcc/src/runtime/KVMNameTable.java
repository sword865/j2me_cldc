/* 
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 */

package runtime;

import java.util.BitSet;
import java.util.Collections;
import java.util.Comparator;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;
import java.io.*;


class KVMNameTable extends KVMHashtable { 

    static final String NameStringPrefix = "UString_Item";
    static final int TABLE_SIZE = 256;



    KVMNameTable() { 
        super(TABLE_SIZE, String.class); 
    }

    long hash(Object x) { 
        String str = (String)x;
        return stringHash(str);
    }


    KVMClassTable classTable;

    void writeTable(CCodeWriter out, String tableName) { 
    Vector all = new Vector();
    BitSet seenSizes   = new BitSet();
    for (int i = 0; i < TABLE_SIZE; i++) { 
        for (  String entry = (String)getFirst(i); 
           entry != null; 
           entry = (String)getNext(entry)) { 
        String keyFilled = 
            Integer.toHexString(getNameKey(entry)+0x10000).substring(1);
        all.add(new String[]{entry, keyFilled});
        seenSizes.set(entry.length());
        }
    }
    for (int i = 0; i < seenSizes.size(); i++) { 
        if (seenSizes.get(i)) { 
                out.println("DECLARE_USTRING_STRUCT(" + i + ");");
        }
    }
    Collections.sort(all, new Comparator() { 
          public int compare(Object o1, Object o2) { 
          String[] pair1 = (String[])o1;
          String[] pair2 = (String[])o2;
          int key1 = getNameKey(pair1[0]);
          int key2 = getNameKey(pair2[0]);
          return key1 - key2;
          }
        });
    out.println();
    out.println();
    out.println("static CONST struct AllUTFStrings_Struct {");
    for (Enumeration e = all.elements(); e.hasMoreElements(); ) { 
        String[] pair = (String[])e.nextElement(); 
        String entry = pair[0];
        String key = pair[1];
        out.println("\tstruct UTF_Hash_Entry_" + entry.length() + 
            " U" + key + ";");
    }
    out.println("} AllUTFStrings = {");
    for (Enumeration e = all.elements(); e.hasMoreElements(); ) { 
        String[] pair = (String[])e.nextElement(); 
        String entry = pair[0];
        String key = pair[1];
        String nextEntryName = getUString((String)getNext(entry));
            out.print("\tUSTRING(" + key + ", " + nextEntryName + ", " + 
              entry.length() + ", ");
            out.printSafeString(entry);
            out.print("),");
        try { 
        if (entry.charAt(0) < 20) { 
            String s = classTable.decodeMethodSignature(entry);
            out.print(" /* ");
            out.print(s);
            out.print(" */");
        } 
        } catch (RuntimeException ex) { }
        out.println();
    }
    out.println("};\n");
    super.writeTable(out, tableName);
    }



    Object tableChain(CCodeWriter out, int bucket, Object[] list) {
        return list.length == 0 ? null : list[0];
    }

    void tableEntry(CCodeWriter out, int bucket, Object token ) { 
        out.print(getUString((String)token));
    }

    public void addNewEntryCallback(int bucket, Object neww) {
        int value;
        String next = (String)getNext((String)neww);
        if (next == null) { 
            value = bucket + getSize();
        } else { 
            value = getKey(next) + getSize();
        }
        setKey(neww, value);
    }

    public String getUString(int key) { 
        String thisKey = Integer.toHexString(key + 0x10000).substring(1);
    return "&AllUTFStrings.U" + thisKey;
    }

    public String getUString(String string) { 
    if (string == null) return "NULL";
        return getUString(getKey(string));
    }

    public int getNameKey(String string) { 
        return getKey(string);
    }


    public void declareUString(CCodeWriter out, String name) { }

}


