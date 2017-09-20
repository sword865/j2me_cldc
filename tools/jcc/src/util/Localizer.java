/*
 *    Localizer.java    1.4    03/01/14 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package util;
import text.*;

import java.text.MessageFormat;

/*
 * The source of all localized messages for all the programs
 * and classes that used the JCCMessage bundle. Since there
 * are multiple programs involved, this facility is a separate class,
 * rather than part of any one of them. It is assumed that locale
 * is uniform over all classes of these programs, so does not need
 * to be determined per-class or per-object.
 */

public class Localizer {
    static      PI18n localizer = new PI18n("JCCMessage");

    public static String getString( String key ){
    return localizer.getString( key );
    }

    public static String getString (String key, Object[] values) { 
        return new MessageFormat(localizer.getString(key)).format(values);
    }

    public static String getString(String key, Object value) {
        return getString(key, new Object[]{value});
    }

    public static String getString(String key, Object value1, Object value2) {
        return getString(key, new Object[]{value1, value2});
    }

    public static String getString(String key, Object value1, 
                   Object value2, Object value3) {
        return getString(key, new Object[]{value1, value2, value3});
    }
}
