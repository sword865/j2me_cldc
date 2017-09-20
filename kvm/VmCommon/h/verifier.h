/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Class file verifier (runtime part)
 * FILE:      verifier.h
 * OVERVIEW:  KVM has a new, two-phase class file verifier.  In order
 *            to run in KVM, class files must first be processed with
 *            a special "pre-verifier" tool. This phase is typically
 *            done on the development workstation.  During execution,
 *            the runtime verifier (defined in this file) of the KVM
 *            performs the actual class file verification based on 
 *            both runtime information and pre-verification information.
 * AUTHOR:    Sheng Liang, Frank Yellin, restructured by Nik Shaylor
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

/*=========================================================================
 * Functions
 *=======================================================================*/

void InitializeVerifier(void);
int  verifyClass(INSTANCE_CLASS c);
void Vfy_verifyMethodOrAbort(const METHOD vMethod);

