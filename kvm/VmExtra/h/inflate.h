/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: JAR file reader / inflater.
 * FILE:      inflate.h
 * OVERVIEW:  Public header file for the JAR inflater module. 
 * AUTHOR:    Tasneem Sayeed, Frank Yellin
 *=======================================================================*/

#ifndef _INFLATE_H_
#define _INFLATE_H_

/*=========================================================================
 * Forward declarations for JAR file reader
 *=======================================================================*/

typedef int    (*JarGetByteFunctionType)(unsigned char *, int, void *);

bool_t inflateData(void *data, JarGetByteFunctionType, int compLen, 
               UNSIGNED_CHAR_HANDLE outFileH, int decompLen);

/* Any caller to inflate must ensure that it is safe to read at least
 * this many bytes beyond compData + compLen
 */
#define INFLATER_EXTRA_BYTES 4

#endif /* _INFLATE_H_ */

