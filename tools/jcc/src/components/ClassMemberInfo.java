/*
 *    ClassMemberInfo.java    1.9    99/05/05 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package components;
import jcc.Util;
import vm.Const;
import jcc.Str2ID;

public abstract
class ClassMemberInfo extends ClassComponent {
    public int         access;
    public int         nameIndex;
    public int         typeIndex;
    public UnicodeConstant name;
    public UnicodeConstant type;
    public ClassInfo     parent;
    private int          ID;
    private boolean      computedID = false;

    public int         index;        // used by in-core output writers

    public int          flags;        // used by member loader
    public final static int INCLUDE    = 1; // a flag value.

    public ClassMemberInfo( int n, int t, int a, ClassInfo p ){
    nameIndex = n;
    typeIndex = t;
    access    = a;
    parent    = p;
    flags      = INCLUDE; // by default, we want everything.
    }

    public boolean isStaticMember( ){
    return ( (access & Const.ACC_STATIC) != 0 );
    }

    public boolean isPrivateMember( ){
    return ( (access & Const.ACC_PRIVATE) != 0 );
    }

    public void
    resolve( ConstantObject table[] ){
    if ( resolved ) return;
    name     = (UnicodeConstant)table[nameIndex];
    type     = (UnicodeConstant)table[typeIndex];
    resolved = true;
    }

    public int
    getID(){
    if ( ! computedID ){
        ID       = Str2ID.sigHash.getID( name, type );
        computedID = true;
    }
    return ID;
    }

    public void
    countConstantReferences( ){
    if ( name != null ) name.incReference();
    if ( type != null ) type.incReference();
    }

    public void
    externalize( ConstantPool p ){
    name = (UnicodeConstant)p.add( name );
    type = (UnicodeConstant)p.add( type );
    }

    public String toString(){
    if ( resolved ){
        return Util.accessToString(access)+" "+name.string+" : "+type.string;
    } else {
        return Util.accessToString(access)+" [ "+nameIndex+" : "+typeIndex+" ]";
    }
    }
    public String qualifiedName(){
    if ( resolved ){
        return name.string+":"+type.string;
    }else{
        return Util.accessToString(access)+" "+parent.className+" [ "+nameIndex+" : "+typeIndex+" ]";
    }
    }
}

