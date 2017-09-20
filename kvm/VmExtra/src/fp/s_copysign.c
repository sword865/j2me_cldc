/* @(#)s_copysign.c 1.3 95/01/18 */
/* 
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 */


/*
 * copysign(double x, double y)
 * copysign(x,y) returns a value with the magnitude of x and
 * with the sign bit of y.
 */

#include <global.h>

double JFP_lib_copysign(double x, double y) {
    __HI(x) = (__HI(x)&0x7fffffff)|(__HI(y)&0x80000000);
    return x;
}
