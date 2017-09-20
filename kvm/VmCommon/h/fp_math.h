/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Floating point operations for CLDC 1.1
 * FILE:      fp_math.h
 * OVERVIEW:  This file provides definitions that are needed for
 *            supporting the Math library functions defined in
 *            CLDC Specification 1.1.
 * AUTHOR:    Kinsley Wong
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

/*=========================================================================
 * Definitions and declarations
 *=======================================================================*/

/* Macros for NaN (Not-A-Number) and Infinity for floats and doubles */
#define F_POS_INFINITY    0x7F800000
#define F_NEG_INFINITY    0xFF800000
#define F_L_POS_NAN       0x7F800001
#define F_H_POS_NAN       0x7FFFFFFF
#define F_L_NEG_NAN       0xFF800001
#define F_H_NEG_NAN       0xFFFFFFFF

#define D_POS_INFINITY    0x7FF0000000000000
#define D_NEG_INFINITY    0xFFF0000000000000
#define D_L_POS_NAN       0x7FF0000000000001
#define D_H_POS_NAN       0x7FFFFFFFFFFFFFFF
#define D_L_NEG_NAN       0xFFF0000000000001
#define D_H_NEG_NAN       0xFFFFFFFFFFFFFFFF

/* MAX and MIN values for INT and LONG */
#define MAX_INT           0x7FFFFFFF
#define MIN_INT           0x80000000
#define MAX_LONG          0x7FFFFFFFFFFFFFFF
#define MIN_LONG          0x8000000000000000

/*=========================================================================
 * Function prototype for initializing floating point arithmetic
 *=======================================================================*/
void InitializeFloatingPoint();

/*=========================================================================
 * Function prototypes for the internal library routines that we use
 *=======================================================================*/

double __kernel_sin(double x, double y, int iy);
double __kernel_cos(double x, double y);
double __kernel_tan(double x, double y, int iy);
int    __kernel_rem_pio2(double *x, double *y, int e0, int nx, int prec, const int *ipio2);

double __ieee754_sqrt(double x);
int    __ieee754_rem_pio2(double x, double *y);

double JFP_lib_copysign(double x, double y);
double JFP_lib_fabs(double x);
double JFP_lib_scalbn(double x, int n);

double JFP_lib_sin(double x);
double JFP_lib_cos(double x);
double JFP_lib_tan(double x);
double JFP_lib_floor(double x);
double JFP_lib_ceil(double x);
double JFP_lib_sqrt(double x);

/*=========================================================================
 * Function prototypes for the some floating point operations
 * that are needed for fixing strictfp problems on x86 CPUs.
 *=======================================================================*/

void JFP_lib_dmul_x86(double lvalue, double rvalue);
void JFP_lib_ddiv_x86(double lvalue, double rvalue);
void JFP_lib_fcmpl_x86(float lvalue, float rvalue);
void JFP_lib_fcmpg_x86(float lvalue, float rvalue);
void JFP_lib_dcmpl_x86(double lvalue, double rvalue);
void JFP_lib_dcmpg_x86(double lvalue, double rvalue);
void JFP_lib_frem_x86(float lvalue, float rvalue);
void JFP_lib_drem_x86(double lvalue, double rvalue);

/*=========================================================================
 * Native function prototypes for CLDC 1.1 functions
 *=======================================================================*/

void Java_java_lang_Float_intBitsToFloat(void);
void Java_java_lang_Float_floatToIntBits(void);

void Java_java_lang_Double_longBitsToDouble(void);
void Java_java_lang_Double_doubleToLongBits(void);

void Java_java_lang_Math_ceil(void);
void Java_java_lang_Math_floor(void);

void Java_java_lang_Math_sqrt(void);

void Java_java_lang_Math_sin(void);
void Java_java_lang_Math_cos(void);
void Java_java_lang_Math_tan(void);

