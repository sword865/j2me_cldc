/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Global definitions
 * FILE:      global.c
 * OVERVIEW:  Global system-wide definitions.
 * AUTHOR:    Antero Taivalsaari, Sun Labs, 1998
 *            Many others since then...
 *=======================================================================*/

/*=========================================================================
 * COMMENT: 
 * This file contains declarations that do not belong to any 
 * particular structure or subsystem of the VM. There are additional 
 * global variable declarations in other files.
 *=======================================================================*/

/*=========================================================================
 * Local include files
 *=======================================================================*/

#include <global.h>

/*=========================================================================
 * Miscellaneous global variables
 *=======================================================================*/

/* Shared string buffer that is used internally by the VM */
/* NOTE: STRINGBUFFERSIZE is defined in main.h */
char str_buffer[STRINGBUFFERSIZE];

/* Requested heap size when starting the VM from the command line */
long RequestedHeapSize;    

/* tell whether there is a debugger connected to the VM */
bool_t vmDebugReady = FALSE;

/*=========================================================================
 * Global execution modes
 *=======================================================================*/

/*  Flags for toggling certain global modes on and off */
bool_t JamEnabled;
bool_t JamRepeat;

/*========================================================================
 * Runtime flags for choosing different tracing/debugging options.
 * All these options make the system very verbose. Turn them all off
 * for normal VM operation.
 *=======================================================================*/
                      
#define DEFINE_TRACE_VAR_AND_ZERO(varName, userName) int varName = 0;
FOR_EACH_TRACE_FLAG(DEFINE_TRACE_VAR_AND_ZERO)

/*=========================================================================
 * Error handling definitions
 *=======================================================================*/

static struct throwableScopeStruct ThrowableScopeStruct = {
   /* env =           */ NULL,
   /* throwable =     */ NULL,
   /* tmpRootsCount = */ 0,
   /* outer =         */ NULL
};
THROWABLE_SCOPE ThrowableScope = &ThrowableScopeStruct;
void* VMScope = NULL;
int   VMExitCode = 0;

