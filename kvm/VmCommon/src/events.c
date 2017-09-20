/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Event handling support
 * FILE:      events.c
 * OVERVIEW:  This file defines the operations for binding the
 *            interpreter to the event handling mechanisms of
 *            the host operating system.
 * AUTHOR:    Nik Shaylor 4/20/00
 *            Bill Pittore (debugging support)
 *            Frank Yellin
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>
#if ENABLE_JAVA_DEBUGGER
#include <debugger.h>
#endif

#ifndef PILOT
#include <stdarg.h>
#endif

/*=========================================================================
 * Definitions and declarations
 *=======================================================================*/

#define MAXPARMLENGTH 20

/*=========================================================================
 * Local variables
 *=======================================================================*/

static THREAD waitingThread;

static cell   eventBuffer[MAXPARMLENGTH];
static int    eventInP;
       int    eventCount;

/*=========================================================================
 * Event handling functions
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      InitializeEvents
 * TYPE:          global function
 * OVERVIEW:      Initialize the event system.
 *                We have to explicitly initialize the event system
 *                so that the KVM can shutdown and restart cleanly.
 * INTERFACE:
 *   parameters:  none
 *   returns:     none
 *=======================================================================*/

void InitializeEvents()
{
    waitingThread = 0;
    makeGlobalRoot((cell**)&waitingThread);

    eventInP = 0;
    eventCount = 0;

#if INCLUDEDEBUGCODE
    if (traceevents) {
        fprintf(stdout, "Event system initialized\n");
    }
#endif /* INCLUDEDEBUGCODE */
}

/*=========================================================================
 * FUNCTION:      StoreKVMEvent
 * TYPE:          global function
 * OVERVIEW:      Callback to indicate an event has occurred

 * INTERFACE:
 *   parameters:  type:      event type
 *                argCount:  number of parameters taken by event
 *                .....:     argCount parameters
 *   returns:     none
 *=======================================================================*/

void
StoreKVMEvent(cell type, int argCount, ...)
{
    int inP;
    int i;
    va_list args;
    if (eventCount == 0) {
        /* Reset the in pointer to 0 if we can */
        eventInP = 0;
    } else if (eventInP > (MAXPARMLENGTH - 1 - argCount)) {
        /* Ignore the event, for now, since we don't have any space to
         * store it.
         */
        return;
    }

#if INCLUDEDEBUGCODE
    if (traceevents) {
        fprintf(stdout, "Event %ld received\n", (long)type);
    }
#endif

    inP = eventInP;             /* cache this locally */
    eventBuffer[inP++] = type;

    va_start(args, argCount);
        for (i = 0; i < argCount; i++) {
            eventBuffer[inP++] = va_arg(args, cell);
        }
    va_end(args);

    /* Update the count and the pointer to where to put the next argument */
    eventCount += (inP - eventInP);
    eventInP = inP;
}

/*=========================================================================
 * FUNCTION:      getKVMEvent
 * TYPE:          Local function
 * OVERVIEW:      Find next event or parameter on the input queue.
 * INTERFACE:
 *   parameters:  forever:   if TRUE, wait forever, if FALSE wait
 *                           until waitUntil
 *                waitFor:   wait at most until this time delta (if !forever).
 *                result:    location to put the next event
 *   returns:     boolean    indicating whether an event or parameter
 *                           is available
 *=======================================================================*/

static bool_t
getKVMEvent(bool_t forever, ulong64 waitFor, cell* result) {
    if (eventCount == 0) {
        ulong64 currentTime = CurrentTime_md();
        if (ll_zero_eq(waitFor)) {
            /* special case, just send zero as argument */
            GetAndStoreNextKVMEvent(forever, waitFor);
        } else {
            /* Try and get the machine's system to give us an event */
        /* NOTE: we send a future time, callee must ensure that 
             * any delta calculated is positive.
             */
            GetAndStoreNextKVMEvent(forever, waitFor + currentTime);
        }
    }
    if (eventCount > 0) {
        /* We have an event */
        *result = eventBuffer[eventInP - eventCount];
        eventCount--;
        return TRUE;
    } else {
        /* No event available */
        return FALSE;
    }
}

 /*=========================================================================
  * FUNCTION:      InterpreterHandleEvent
  * TYPE:          Global C function
  * OVERVIEW:      Wait for a system event
  * INTERFACE:
  *   parameters:  Delta time to wait if no events are available immediately;
  *                zero value indicates that the host platform can go
  *                to sleep and/or conserve battery until a new native
  *                event occurs
  *   returns:     nothing
  *=======================================================================*/

void InterpreterHandleEvent(ulong64 wakeupDelta) {
    bool_t forever = FALSE;     /* The most common value */

    if (ll_zero_ne(wakeupDelta)) {
        /* wakeupDelta already has the right value.  But change it to
         * be at most 20ms from now if the Java debugger is being used.
         * We also do this if asynchronous native functions are in use.
         */
        if (vmDebugReady || ASYNCHRONOUS_NATIVE_FUNCTIONS) {
            ulong64 max = 20;
            if (ll_compare_ge(wakeupDelta, max)) {
              wakeupDelta = max;
            }
        }
    } else if (!areActiveThreads()) {
        /*
         * At this point, wakeupDelta == 0 AND there are NO active threads
         * waiting to run.  This state could be caused by the debugger
         * suspending threads OR there is an active async call that's somewhere
         * in the OS doing it's thing.  We should therefore determine if there
         * is a debugger active OR ASYNC functions in which case sleep for
         * at most 20 ms. so that we can give the OS time to work and then
         * return back to KVM and check status again
         */
      if (vmDebugReady || ASYNCHRONOUS_NATIVE_FUNCTIONS) {
          ll_int_to_long(wakeupDelta, 20);
      } else {
          /* debugger not active AND no ASYNC_NATIVE_FUNCTIONS, wait forever */
          forever = TRUE;
          /* set wakeupDelta to the max given the current time.
           * actually 1000 seconds less than the max to avoid any
           * possibility of rollover 
           */
          ll_int_to_long(wakeupDelta, -1);
          ll_dec(wakeupDelta, CurrentTime_md());
          ll_dec(wakeupDelta, 1000000);
        }
    }

    if (waitingThread == NULL) {
        /* Nobody waiting for an event */
        /* If active threads then just return */
        if (areActiveThreads()) {
            return;
        }
      if (vmDebugReady) {
#if ENABLE_JAVA_DEBUGGER
        unsigned long delta;
        ll_long_to_uint(wakeupDelta, delta);

        ProcessDebugCmds(delta);
#endif
      } else {
        SLEEP_FOR(wakeupDelta);
      }
    } else {
        cell result;
        if (getKVMEvent(forever, wakeupDelta, &result)) {
            (void)popStackForThread(waitingThread);
            pushStackForThread(waitingThread, result);
            resumeThread(waitingThread);
            waitingThread = NULL;
        }
    }
}

 /*=========================================================================
  * FUNCTION:      readOneEvent()
  * OVERVIEW:      Read one event from the event queue.
  *                Call these functions to access the event
  *                queue from Java level.
  * INTERFACE (operand stack manipulation):
  *   parameters:  this
  *   returns:     the integer value
  *=======================================================================*/

static void readOneEvent() {
    ulong64 wakeupDelta;
    ll_setZero(wakeupDelta);

    if (!getKVMEvent(FALSE, wakeupDelta, &topStack)) {
        waitingThread = CurrentThread;
        topStack = 0;
        suspendThread();
    }
 }

void JVM_EventsReadInt(void) {
    readOneEvent();
}

void JVM_EventsReadUTF(void) {
    readOneEvent();
}

