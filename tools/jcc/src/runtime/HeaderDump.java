/*
 *    HeaderDump.java    1.10    99/06/22 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package runtime;
import vm.Const;
import jcc.Util;
import util.*;
import components.*;
import vm.*;
import java.io.PrintStream;
import java.util.Enumeration;
import java.util.Vector;
import java.util.Hashtable;

abstract
public class HeaderDump{
    char        CDelim;
    final static char    DIR_DELIM = '/';
    final static char    INNER_DELIM = '$';

    public HeaderDump( char cdelim ){
    CDelim = cdelim;
    }

    public String filename( String classname ){
    return Util.convertToClassName( classname );
    }

    // this data is really per-header.
    // it is guarded by synchronous method access
    PrintStream    o;
    String      className;

    static String
    strsub( String src, char substitute ){
    return src.replace( DIR_DELIM, substitute).replace(INNER_DELIM, substitute );
    }

    protected boolean
    generateConsts( String classname, FieldInfo fields[] ){
    if ( fields == null ) return false;
    boolean didWork = false;
    for ( int i = 0; i < fields.length; i++ ){
        FieldInfo f = fields[i];
        if ( f.value == null ) continue; // not a constant.
        String constName = classname+CDelim+f.name.string;
        o.println("#undef "+constName );
        ConstantObject v = f.value;
        DoubleValueConstant dvc;
        switch ( v.tag ){
        case Const.CONSTANT_FLOAT:
        float fval = Float.intBitsToFloat( ((SingleValueConstant)v).value );
        o.println("#define "+constName+" "+fval+"f");
        break;
        case Const.CONSTANT_DOUBLE:
        dvc = (DoubleValueConstant)v;
        double dval = Double.longBitsToDouble((((long)(dvc.highVal))<<32) | ((long)dvc.lowVal&0xffffffffL));
        o.println("#define "+constName+" "+dval+"D");
        break;
        
        case Const.CONSTANT_LONG:
        dvc = (DoubleValueConstant)v;
        long lval = (((long)(dvc.highVal))<<32) | ((long)dvc.lowVal&0xffffffffL);
        o.println("#define "+constName+" "+lval+"LL");
        break;
        case Const.CONSTANT_INTEGER:
        int ival = ((SingleValueConstant)v).value;
        o.println("#define "+constName+" "+ival+"L");
        break;
        default:
        }
        didWork = true;
    }
    return didWork;
    }

    // returns true if anything worthwhile got written,
    // else false.
    abstract public boolean
    dumpHeader( ClassInfo c, PrintStream outfile );

    abstract public boolean
    dumpExternals( ClassInfo c, PrintStream outfile );
}
