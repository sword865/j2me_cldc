/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

package kdp;

public class Log {

    static int verbose = 0;

    public static void SET_LOG(int level) {
        verbose = level;
    }

    public static void LOG(int level, String s) {
        if (verbose >= level)
            System.out.print(s);
    }

    public static void LOGE(int level, String s) {
        if (verbose == level)
            System.out.print(s);
    }

    public static void LOGN(int level, String s) {
        if (verbose >= level)
            System.out.println(s);
    }

    public static void LOGNE(int level, String s) {
        if (verbose == level)
            System.out.println(s);
    }
}
