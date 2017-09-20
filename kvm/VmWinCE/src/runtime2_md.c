/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * KVM
 *=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Machine-specific implementations needed by virtual machine
 * FILE:      runtime_md2.c
 * AUTHOR:    Nik Shaylor
 *
 * NOTE:      These functions have been split out from runtime_md.c
 *            because of conflicts between windows.h and global.h
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <machine_md.h>
#include <main.h>

#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#define NOMSG
#include <windows.h>
#include <winsock.h>

#define MAXCALENDARFLDS 15

#define YEAR 1
#define MONTH 2
#define DAY_OF_MONTH 5
#define HOUR 10
#define MINUTE 12
#define SECOND 13
#define MILLISECOND 14

static unsigned long date[MAXCALENDARFLDS];

/*=========================================================================
 * Functions
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      Calendar_md()
 * TYPE:          machine-specific implementation of native function
 * OVERVIEW:      Initializes the calendar fields, which represent the
 *                Calendar related attributes of a date.
 * INTERFACE:
 *   parameters:  none
 *   returns:     none
 * AUTHOR:        Tasneem Sayeed
 *=======================================================================*/
unsigned long * Calendar_md(void)
{
        SYSTEMTIME st;

        /* initialize */
        memset(&st, 0, sizeof(st));
        memset(&date, 0, sizeof(date));

        GetSystemTime(&st);

        // initialize calendar fields
        date[YEAR]         = st.wYear;
        date[MONTH]        = st.wMonth;
        date[DAY_OF_MONTH] = st.wDay;
        date[HOUR]         = st.wHour;
        date[MINUTE]       = st.wMinute;
        date[SECOND]       = st.wSecond;
        date[MILLISECOND] = st.wMilliseconds;
        return date;
}

/*=========================================================================
 * FUNCTION:      Yeild_md()
 * TYPE:          machine-specific implementation of native function
 * OVERVIEW:      Yield the current thread
 * INTERFACE:
 *   parameters:  none
 *   returns:     current time, in centiseconds since startup
 *=======================================================================*/

void Yield_md(void) {
    Sleep(0);
}

/*=========================================================================
 * Critical section operations for asynchronous (non-blocking) methods
 *=======================================================================*/

#if ASYNCHRONOUS_NATIVE_FUNCTIONS

/*
 * Windows mutex object
 */
CRITICAL_SECTION csect;

/*=========================================================================
 * FUNCTION:      enterSystemCriticalSection()
 * TYPE:          machine-specific implementation of native function
 * OVERVIEW:      Wait on the system mutex
 * INTERFACE:
 *   parameters:  none
 *   returns:     none
 *=======================================================================*/

void enterSystemCriticalSection(void) {
    EnterCriticalSection( &csect );
}

/*=========================================================================
 * FUNCTION:      exitSystemCriticalSection()
 * TYPE:          machine-specific implementation of native function
 * OVERVIEW:      Release the system mutex
 * INTERFACE:
 *   parameters:  none
 *   returns:     none
 *=======================================================================*/

void exitSystemCriticalSection(void) {
    LeaveCriticalSection( &csect );
}

/*=========================================================================
 * Async thread functions
 *=======================================================================*/

void fatalError(char *);

/*
 * Handle for thread blocking
 */
HANDLE EventHandle;

/*=========================================================================
 * FUNCTION:      asyncThread()
 * TYPE:          local function
 * OVERVIEW:      Main I/O thread processing loop
 * INTERFACE:
 *   parameters:  none
 *   returns:     none
 *=======================================================================*/

static void asyncThread(void) {
    for(;;) {
        if (WaitForSingleObject(EventHandle, INFINITE) == WAIT_FAILED) {
                               fatalError("Could not WaitForSingleObject");
        }
        processAcyncThread();
    }
}

/*=========================================================================
 * FUNCTION:      beginthreads()
 * TYPE:          local  function
 * OVERVIEW:      Start 5 I/O thread
 * INTERFACE:
 *   parameters:  none
 *   returns:     none
 *=======================================================================*/

static void beginthreads() {
    int i;
    EventHandle = CreateEvent(0, FALSE, FALSE, 0);
    for(i = 0 ; i < 5 ; i++) {
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)asyncThread, 0, 0, NULL);
    }
}

/*=========================================================================
 * FUNCTION:      releaseAsyncThread()
 * TYPE:          machine-specific implementation of native function
 * OVERVIEW:      Release an I/O thread
 * INTERFACE:
 *   parameters:  none
 *   returns:     none
 *=======================================================================*/

void releaseAsyncThread(void) {
    if(SetEvent(EventHandle) == FALSE)
        fatalError("Could not SetEvent");
}

#endif /* ASYNCHRONOUS_NATIVE_FUNCTIONS */

/*=========================================================================
 * FUNCTION:      runtime2_md_init()
 * TYPE:          Initialization routine
 * OVERVIEW:      Function for initializing the definitions in this file.
 * INTERFACE:
 *   parameters:  none
 *   returns:     none
 *=======================================================================*/

/* We need to make sure runtime2_md_init is only called once when
 * -jam -repeat option is supplied.
 */
static int runtime2_md_inited = FALSE;

void runtime2_md_init(void) {
    if (runtime2_md_inited) {
        return;
    }
#if ASYNCHRONOUS_NATIVE_FUNCTIONS
    InitializeCriticalSection(&csect);
    beginthreads();
#endif
    runtime2_md_inited = TRUE;
}

#define FT2INT64(ft) \
    ((ulong64)(ft).dwHighDateTime << 32 | (ulong64)(ft).dwLowDateTime)

/*=========================================================================
 * FUNCTION:      sysTimeMillis()
 * TYPE:          time
 * OVERVIEW:      Get system time from Windows.
 * INTERFACE:
 *   parameters:  none
 *   returns:     time in milliseconds
 *=======================================================================*/

/*
 * Because GetSystemTime on winCE is updated every second
 * we need to get the tick count to calculate the number
 * of milliseconds since 1970
 */
ulong64 sysTimeMillis(void) {
    static ulong64 fileTime_1_1_70 = 0, numSecBeforeWinCE = 0;
    ulong64 nowTime = 0, startWinCE = 0;
    SYSTEMTIME st0;
    FILETIME   ft0;

    if (fileTime_1_1_70 == 0) {
        /*
         * Initialize fileTime_1_1_70 -- the Win32 file time of midnight
         * 1/1/70.
         */
        memset(&st0, 0, sizeof(st0));
        st0.wYear  = 1970;
        st0.wMonth = 1;
        st0.wDay   = 1;
        SystemTimeToFileTime(&st0, &ft0);
        fileTime_1_1_70 = FT2INT64(ft0);

        /* Get current system time */
        /* This is updated every second on WinCE */
        GetSystemTime(&st0);
        SystemTimeToFileTime(&st0, &ft0);

        /* Number of milliseconds since 1601 */
        nowTime = FT2INT64(ft0) / 10000;

        /* Number of milliseconds since WinCE started */
        startWinCE = nowTime - GetTickCount();

        /* Number of milliseconds from 1970 till WinCE was started */
        numSecBeforeWinCE = startWinCE - ( fileTime_1_1_70 / 10000 );
    }
    return ( numSecBeforeWinCE + GetTickCount());
}
