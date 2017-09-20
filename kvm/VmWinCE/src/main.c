/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * K Virtual Machine
 *=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Main program
 * FILE:      main.c
 * OVERVIEW:  Main program & system initialization
 * AUTHOR:    Kinsley Wong
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <windows.h>
#include <wince_io.h>
#include <kvmutil.h>

int KVMFlags();
int CmdLineStart(LPWSTR lpCmdLine);

/* Main program */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine, int nShowCmd) {

    int exitCode;
    int output;
    int console_stdout;

    if (wcslen(lpCmdLine) == 0) {
        return StartKVMUtil(hInstance);
    }

    KVMFlags();

    output = GetDWFromReg(TEXT( "output" ));
    console_stdout = GetDWFromReg(TEXT("stdout"));

    if (console_stdout) {
        openConsole();
    }

    if (output) {
        openstdio();
    }

    exitCode = CmdLineStart(lpCmdLine);

    if (output) {
        closestdio();
    }

    if (console_stdout) {
        closeConsole();
    }
    return exitCode;
}

/*
 * Retrieves all the KVMUtil options and set all the kvm option
 */
int KVMFlags() {

    /* Gets the heapsize. if heapsize is 0 set heapsize to 64K */
    RequestedHeapSize = GetDWFromReg(TEXT("heapsize"));
    if (RequestedHeapSize == 0) {
        RequestedHeapSize = DEFAULTHEAPSIZE;
    }

#if INCLUDEDEBUGCODE
    tracebytecodes           = GetDWFromReg(TEXT("tracebytecodes"));
    traceclassloading        = GetDWFromReg(TEXT("traceclassloading"));
    traceclassloadingverbose = GetDWFromReg(TEXT("traceclassloadingverbose"));
#if ENABLE_JAVA_DEBUGGER
    tracedebugger            = GetDWFromReg(TEXT("tracedebugger"));
#endif
    traceexceptions          = GetDWFromReg(TEXT("traceexceptions"));
    traceevents              = GetDWFromReg(TEXT("traceevents"));
    traceframes              = GetDWFromReg(TEXT("traceframes"));
    tracegarbagecollection   = GetDWFromReg(TEXT("tracegarbagecollection"));
    tracegarbagecollectionverbose = GetDWFromReg(TEXT("tracegarbagecollectionverbose"));
    tracememoryallocation    = GetDWFromReg(TEXT("tracememoryallocation"));
    tracemethodcalls         = GetDWFromReg(TEXT("tracemethodcalls"));
    tracemethodcallsverbose  = GetDWFromReg(TEXT("tracemethodcallsverbose"));
    tracemonitors            = GetDWFromReg(TEXT("tracemonitors"));
    tracenetworking          = GetDWFromReg(TEXT("tracenetworking"));
    tracestackchunks         = GetDWFromReg(TEXT("tracestackchunks"));
    tracestackmaps           = GetDWFromReg(TEXT("tracestackmaps"));
    tracethreading           = GetDWFromReg(TEXT("tracethreading"));
    traceverifier            = GetDWFromReg(TEXT("traceverifier"));
#endif

#if ENABLE_JAVA_DEBUGGER
    suspend        = TRUE;
    debuggerActive = GetDWFromReg(TEXT("debuggerActive"));
    debuggerPort   = GetDWFromReg(TEXT("debuggerPort"));
    suspend        = GetDWFromReg(TEXT("debuggerSuspend"));
#endif

    return 0;
}

/*
 * Read the arguments from the command line and start the VM
 */
int CmdLineStart(LPWSTR lpCmdLine) {

    char temp[MAX_PATH];
    char *argv[MAX_PATH];
    char *token;

    int argc;
    int i = 0;

    WideCharToMultiByte(CP_ACP, 0,
        lpCmdLine, -1, temp, sizeof( temp ), NULL, NULL);

    argc = 0;
    token = strtok(temp, " ");
    argv[argc] = token;

    while(token != NULL) {
        token = strtok(NULL, " ");
        argv[++argc] = token;
    }

    while ( argc > 1 ) {
        if ((strcmp( argv[i], "-classpath") == 0) && argc > 2) {
            UserClassPath = argv[i+1];
            i+=2; argc -=2;
        } else {
            break;
        }
    }

    if (UserClassPath == NULL) {
        UserClassPath = ".";
    }

    return StartJVM(argc, &argv[i]);
}
