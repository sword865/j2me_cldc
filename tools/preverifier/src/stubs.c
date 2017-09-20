/*
 * @(#)stubs.c	1.6 03/01/14
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/*=========================================================================
 * SYSTEM:    Verifier
 * SUBSYSTEM: Stubs.
 * FILE:      stubs.c
 * OVERVIEW:  Miscellaneous functions used during class loading, etc.
 *
 * AUTHOR:    Sheng Liang, Sun Microsystems, Inc.
 *            Edited by Tasneem Sayeed, Sun Microsystems
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <oobj.h>
#include <sys_api.h>

void
SignalError(struct execenv * ee, char *ename, char *DetailMessage)
{
    printCurrentClassName();
    jio_fprintf(stderr, "%s", ename);
    if (DetailMessage) {
        jio_fprintf(stderr, ": %s\n", DetailMessage);
    } else {
        jio_fprintf(stderr, "\n");
    }
    exit(1);
}

bool_t
VerifyClassAccess(ClassClass *current_class, ClassClass *new_class, 
          bool_t classloader_only) 
{
    return TRUE;
}

/* This is called by FindClassFromClass when a class name is being requested
 * by another class that was loaded via a classloader.  For javah, we don't
 * really care.
 */

ClassClass *
ClassLoaderFindClass(struct execenv *ee, 
             struct Hjava_lang_ClassLoader *loader, 
             char *name, bool_t resolve)
{ 
    return NULL;
}

/* Get the execution environment
 */
ExecEnv *
EE() {
    static struct execenv lee;
    return &lee;
}

bool_t RunStaticInitializers(ClassClass *cb)
{
    return TRUE;
}

void InitializeInvoker(ClassClass *cb)
{
}

int verifyclasses = VERIFY_ALL;
long nbinclasses, sizebinclasses;
ClassClass **binclasses;
bool_t verbose;
struct StrIDhash *nameTypeHash;
struct StrIDhash *stringHash;

void *allocClassClass()
{
    ClassClass *cb = sysCalloc(1, sizeof(ClassClass));
    Classjava_lang_Class *ucb = sysCalloc(1, sizeof(Classjava_lang_Class));
    
    cb->obj = ucb;
    ucb->HandleToSelf = cb;
    return cb;
}

void DumpThreads() {}
