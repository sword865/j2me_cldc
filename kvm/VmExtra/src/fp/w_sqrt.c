/* @(#)w_sqrt.c 1.3 95/01/18 */
/* 
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 */

/*
 * wrapper sqrt(x)
 */

#include <global.h>

double JFP_lib_sqrt(double x) {
    return __ieee754_sqrt(x);
}


