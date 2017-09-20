/* 
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 */

package runtime;
import vm.*;
import components.*;


class KVMClassName { 
    private String baseName; 
    private String packageName; 
    private int depth;
    private char baseType;              // base type is  a primitive

    KVMClassName(String p, String b) { this(p, b, 0); }

    KVMClassName(String p, String b, int d) { 
        this.baseName = b; 
        this.packageName = p; 
        this.depth = d;
    }
    
    KVMClassName(char p, int d) {
        this.baseType = p;
        this.depth = d;
    }

    KVMClassName(String name) { 
        int length = name.length();
        for (depth = 0; name.charAt(depth) == '['; depth++);
        if (depth == 0) { 
            String packageName;
            int lastSlash = name.lastIndexOf('/');
            if (lastSlash == -1) { 
                this.baseName = name;
                this.packageName = null;
            } else { 
                this.packageName = name.substring(0, lastSlash);
                this.baseName = name.substring(lastSlash + 1);
            }
        } else { 
            if (depth + 1 == length) { 
                this.baseType = name.charAt(depth);
            } else { 
                String base = name.substring(depth + 1, length - 1);
                KVMClassName temp = new KVMClassName(base);
                this.baseName = temp.baseName;
                this.packageName = temp.packageName;
            }
        }
    }

    public boolean equals(Object x) { 
        if (!(x instanceof KVMClassName)) { 
            return false;
        } 
        KVMClassName y = (KVMClassName)x;
        return  (this.depth == y.depth && this.baseType == y.baseType
                 && sameString(this.baseName, y.baseName) 
                 && sameString(this.packageName, y.packageName));
    } 

    private static boolean sameString(String x, String y) { 
        if (x == null) { 
            return y == null;
        } else { 
            return y != null && x.equals(y);
        }
    }
    
    public String getBaseName() { return baseName; }
    public String getPackageName() { return packageName; }
    public int getDepth() { return depth; }
    public char getBaseType() { return baseType; }
    public boolean isPrimitiveBaseType() { return baseType != 0; }
    public boolean isArrayClass() { return depth > 0; }
    
    public KVMClassName deltaDepth(int delta) {
        if (isPrimitiveBaseType()) {
           return new KVMClassName(baseType, depth + delta);
        } else {
           return new KVMClassName(packageName, baseName, depth + delta);
        }
    }
        
    public String getFullBaseName() {
        if (depth >= 1) {
            StringBuffer b = new StringBuffer();
            for (int i = 0; i < depth; i++) {
                b.append('[');
            }
            if (isPrimitiveBaseType()) {
                b.append(baseType);
            } else {
                b.append('L').append(baseName).append(';');
            }
            return b.toString();
        } else {
            if (isPrimitiveBaseType()) {
                throw new Error("Shouldn't happen!");
            }
            return baseName;
        }
    }
         
    public int hashCode() { 
        int hash = (int)(KVMHashtable.stringHash(getFullBaseName()) + 37);
        if (packageName != null) { 
            hash += KVMHashtable.stringHash(packageName) * 3;
        } 
        return hash;
    }

    public String toString() { 
        StringBuffer b = new StringBuffer();
        if (depth >= 1) { 
            for (int i = 0; i < depth; i++) { 
                b.append('[');
            }
            if (baseType != 0) { 
                b.append(baseType);
                return b.toString();
            }
            b.append('L');
        } 
        if (packageName != null) { 
            b.append(packageName).append('/').append(baseName);
        } else { 
            b.append(baseName);
        }
        if (depth >= 1) { 
            b.append(';');
        }
        return b.toString();
    }

    public EVMClass getEVMClass() { 
        ClassInfo ci = ClassInfo.lookupClass(toString());
        if (ci != null) { 
            return ((EVMClass)ci.vmClass);
        } else { 
            return null;
        }
    }


}


