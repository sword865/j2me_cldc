/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Floating point operations for CLDC 1.1
 * FILE:      fp_math.c
 * OVERVIEW:  This file provides definitions that are needed for
 *            supporting the Math library functions defined in
 *            CLDC Specification 1.1.
 * AUTHOR:    Kinsley Wong
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>
#include <fp_math.h>

/*=========================================================================
 * Native functions needed by CLDC 1.1 java.lang.Float/Double classes
 *=======================================================================*/

/*=========================================================================
 * FUNCTIONS:     intBitsToFloat(I)F (STATIC)
 *                floatToIntBits(F)I (STATIC)
 *                longBitsToDouble(L)D (STATIC)
 *                doubleToLongBits(D)L (STATIC)
 * CLASS:         java.lang.Float, java.lang.Double
 * TYPE:          static native functions
 * OVERVIEW:      Convert floating point representations to
 *                integer bit representation, and vice versa.
 * INTERFACE (operand stack manipulation):
 *   parameters:  value in
 *   returns:     value out
 * NOTE:          The definitions of these functions are intentionally
 *                empty.  These operations are really just dynamic
 *                type casts that allow us to manipulate floating
 *                point numbers as raw bits.
 *=======================================================================*/

void Java_java_lang_Float_intBitsToFloat(void) {
    /* Intentionally empty */
}

void Java_java_lang_Float_floatToIntBits(void) {
    /* Intentionally empty */
}

void Java_java_lang_Double_longBitsToDouble(void) {
    /* Intentionally empty */
}

void Java_java_lang_Double_doubleToLongBits(void) {
    /* Intentionally empty */
}

/*=========================================================================
 * Native functions needed by CLDC 1.1 java.lang.Math class
 *=======================================================================*/

/*=========================================================================
 * FUNCTIONS:     ceil(D)D (STATIC)
 *                floor(D)D) (STATIC)
 * CLASS:         java.lang.Math
 * TYPE:          static native functions
 * OVERVIEW:      Calculate ceiling and floor of double-length
 *                floating point numbers.
 * INTERFACE (operand stack manipulation):
 *   parameters:  value in
 *   returns:     value out
 *=======================================================================*/

void Java_java_lang_Math_ceil(void) {
    double value;

    popDouble(value);
    value = JFP_lib_ceil(value);
    pushDouble(value);
}

void Java_java_lang_Math_floor(void) {
    double value;

    popDouble(value);
    value = JFP_lib_floor(value);
    pushDouble(value);
}

/*=========================================================================
 * FUNCTIONS:     sqrt(D)D (STATIC)
 *                sin(D)D) (STATIC)
 *                cos(D)D) (STATIC)
 *                tan(D)D) (STATIC)
 * CLASS:         java.lang.Math
 * TYPE:          static native functions
 * OVERVIEW:      Calculate square root and trigonometric functions.
 * INTERFACE (operand stack manipulation):
 *   parameters:  value in
 *   returns:     value out
 *=======================================================================*/

void Java_java_lang_Math_sqrt(void) {
    double value;

    popDouble(value);
    value = JFP_lib_sqrt(value);
    pushDouble(value);
}

void Java_java_lang_Math_sin(void) {
    double value;

    popDouble(value);
    value = JFP_lib_sin(value);
    pushDouble(value);
}

void Java_java_lang_Math_cos(void) {
    double value;

    popDouble(value);
    value = JFP_lib_cos(value);
    pushDouble(value);
}

void Java_java_lang_Math_tan(void) {
    double value;

    popDouble(value);
    value = JFP_lib_tan(value);
    pushDouble(value);
}

