/* @(#)s_fabs.c 1.3 95/01/18 */
/* 
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 */

/*
 * fabs(x) returns the absolute value of x.
 */

#include <global.h>

double JFP_lib_fabs(double x) {
    __HI(x) &= 0x7fffffff;
    return x;
}
