/*
 *    EVMInitInfo.java    1.2    03/01/14    SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package runtime;

class EVMInitInfo {
    /*
     * This is just a tripl of:
     *     from-address
     *     to-address
     *     byte count
     * that get stuffed in an array-of-triples
     * for interpretation by the startup code.
     */
    String fromAddress;
    String toAddress;
    String byteCount;
    EVMInitInfo next;

    EVMInitInfo( String f, String t, String c, EVMInitInfo n ){
    fromAddress = f;
    toAddress = t;
    byteCount = c;
    next = n;
    }

    static EVMInitInfo initList = null;

    public static void
    addInfo( String f, String t, String c ){
    initList = new EVMInitInfo( f, t, c, initList );
    }

    public static void
    write( CCodeWriter out, String typename, String dataname, String nname ){
    int n = 0;
    out.println("const struct "+typename+" "+dataname+"[] = {");
    for ( EVMInitInfo p = initList; p != null; p = p.next ){
        out.println("    { "+p.fromAddress+", "+p.toAddress+", "+p.byteCount+" },");
        n++;
    }
    out.println("};");
    out.println("const int "+nname+" = "+n+";");
    }
}
