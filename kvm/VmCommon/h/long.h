/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: 64-bit arithmetic
 * FILE:      long.h
 * OVERVIEW:  Definitions needed for supporting 64-bit arithmetic
 *            in a portable fashion.
 * AUTHOR:    Frank Yellin
 *=======================================================================*/

/*=========================================================================
 * Definitions and declarations
 *=======================================================================*/

#undef long64
#undef ulong64

#if !COMPILER_SUPPORTS_LONG

#if BIG_ENDIAN
typedef struct { unsigned long high; unsigned long low; } long64;
typedef struct { unsigned long high; unsigned long low; } ulong64;
#else
#undef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1
typedef struct { unsigned long low; unsigned long high; } long64;
typedef struct { unsigned long low; unsigned long high; } ulong64;
#endif /* BIG_ENDIAN */

long64  ll_mul(long64, long64);
long64  ll_div(long64, long64);
long64  ll_rem(long64, long64);
long64  ll_shl(long64, int);
long64  ll_shr(long64, int);
long64  ll_ushr(long64, int);

long64  float2ll(float);
long64  double2ll(double);
float   ll2float(long64);
double  ll2double(long64);

#else /* COMPILER_SUPPORTS_LONG */

#define ll_mul(a, b)    ((long64)(a) * (long64)(b))
#define ll_div(a, b)    ((long64)(a) / (long64)(b))
#define ll_rem(a,b)     ((long64)(a) % (long64)(b))
#define ll_shl(a, n)    ((long64)(a) << (n))
#define ll_shr(a, n)    ((long64)(a) >> (n))
#define ll_ushr(a, n)   ((ulong64)(a) >>(n))

#define float2ll(a)     ((long64)a)
#define double2ll(a)    ((long64)a)

#define ll2float(a)     ((float) (a))
#define ll2double(a)    ((double) (a))

#endif /* !COMPILER_SUPPORTS_LONG */

/* Here are the functions that we create the definitions for machines
 * that don't support longs.
 *
 * For all three-operand arguments, the result cannot be the same location
 * as either of the operands */

#if COMPILER_SUPPORTS_LONG

#define ll_setZero(a)        ((a) = 0)
#define ll_inc(a,b)          ((a) += (b))
#define ll_dec(a,b)          ((a) -= (b))
#define ll_negate(a)         ((a) = (-a))

/* The following weird definition fixes a bug in one particular compiler */
#define ll_uint_to_long(a,b) ((a) = (ulong64)(unsigned long)b)
#define ll_int_to_long(a,b)  ((a) = (signed long)b)
#define ll_long_to_uint(a,b) (b = (unsigned int)(a))

#define ll_compare_lt(a, b)  ((a) < (b))
#define ll_compare_eq(a, b)  ((a) == (b))
#define ll_compare_gt(a, b)  ((a) > (b))

#define ll_zero_lt(a)        (a < 0)
#define ll_zero_eq(a)        (a == 0)
#define ll_zero_gt(a)        (a > 0)

#else /* !COMPILER_SUPPORTS_LONG */

#define ll_setZero(a)        ((a).high = (a).low = 0)
#define ll_inc(a,b)          ((a).low += (b).low, \
                             (a).high += (b).high + ((a).low < (b).low));
#define ll_dec(a,b)          ((a).high -= ((b).high + ((a).low < (b).low)), \
                             (a).low  -= (b).low)
#define ll_negate(a)         ((a).low = -(a).low, \
                             (a).high = -((a).high + ((a).low != 0)))

#define ll_uint_to_long(a,b) ((a).high = 0, (a).low = b)
#define ll_int_to_long(a,b)  ((a).low = b; (a).high = ((signed long)b) >> 31)
#define ll_long_to_uint(a,b) (b = (unsigned int)((a).low))

#define ll_compare_lt(a, b)  (((a).high < (b).high) || \
                             (((a).high == (b).high) && ((a).low < (b).low)))
#define ll_compare_eq(a, b)  (((a).high == (b).high) && ((a).low == (b).low))
#define ll_compare_gt(a, b)  ll_compare_lt(b, a)

#define ll_zero_lt(a)        ((a).high < 0)
#define ll_zero_eq(a)        (((a).high | (a).low) == 0)
#define ll_zero_gt(a)        (!(ll_zero_lt(a) || ll_zero_eq(a)))

#endif /* COMPILER_SUPPORTS_LONG */

#define ll_compare_le(a,b)   (!ll_compare_gt(a,b))
#define ll_compare_ne(a,b)   (!ll_compare_eq(a,b))
#define ll_compare_ge(a,b)   (!ll_compare_lt(a,b))

#define ll_zero_le(a)        (!ll_zero_gt(a))
#define ll_zero_ne(a)        (!ll_zero_eq(a))
#define ll_zero_ge(a)        (!ll_zero_lt(a))

