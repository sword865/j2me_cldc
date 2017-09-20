/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * KVM
 *=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Main program
 * FILE:      main.c
 * OVERVIEW:  Main program for command-line based environments
 * AUTHOR:    Antero Taivalsaari, Sun Labs
 *            Edited by Doug Simon 11/1998
 *            JAM integration by Sheng Liang
 *
 * NOTE:      KVM does not have a portable main() function.  This is
 *            because the VM may be used in very different kinds of 
 *            target environments.  Some of the environments may provide
 *            command line support, while many don't; some environments
 *            may launch the VM from a GUI or a micro-browser, etc.
 *
 *            The portable VM startup and shutdown operations are defined
 *            in file VmCommon/src/StartJVM.c. The main() function defined
 *            in this file is applicable only to those target systems that
 *            support VM startup from a command line.
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>

#if USE_JAM
#include <jam.h>
#endif

/*=========================================================================
 * Functions
 *=======================================================================*/

void printHelpText() {
    fprintf(stdout, "Usage: kvm <-options> <classfile>\n");
    fprintf(stdout, "Options:\n");
    fprintf(stdout, "  -version\n");
    fprintf(stdout, "  -classpath <filepath>\n");
    fprintf(stdout, "  -heapsize <size> (e.g. 65536 or 128k or 1M)\n");

#if ENABLE_JAVA_DEBUGGER
    fprintf(stdout, "  -debugger\n");
    fprintf(stdout, "  -suspend\n");
    fprintf(stdout, "  -nosuspend\n");
    fprintf(stdout, "  -port <port number>\n");
#endif /* ENABLE_JAVA_DEBUGGER */

#if INCLUDEDEBUGCODE

#define PRINT_TRACE_OPTION(varName, userName) \
    fprintf(stdout, "  " userName "\n");
FOR_EACH_TRACE_FLAG(PRINT_TRACE_OPTION)
    fprintf(stdout, "  -traceall (activates all tracing options above)\n");
#endif /* INCLUDEDEBUGCODE */
}

#ifndef BUILD_VERSION
#define BUILD_VERSION "generic"
#endif

int main (int argc, char* argv[]) {
    int result;

#if USE_JAM
    char *jamInstalledAppsDir = "./instapps";
#endif

    JamEnabled = FALSE;
    JamRepeat = FALSE;
    RequestedHeapSize = DEFAULTHEAPSIZE;

#if ENABLE_JAVA_DEBUGGER
    suspend = TRUE;
#endif
        
    while (argc > 1) {    
        if (strcmp(argv[1], "-version") == 0) {
            fprintf(stdout, "Version: %s\n", BUILD_VERSION);
            exit(1);           
        } else if (strcmp(argv[1], "-help") == 0) {
            printHelpText();
            exit(0);

#if ENABLE_JAVA_DEBUGGER
        } else if (strcmp(argv[1], "-debugger") == 0) {
            debuggerActive = TRUE;
            argv++; argc--;
        } else if (strcmp(argv[1], "-suspend") == 0) {
            suspend = TRUE;
            argv++; argc--;
        } else if (strcmp(argv[1], "-nosuspend") == 0) {
            suspend = FALSE;
            argv++; argc--;
        } else if ((strcmp(argv[1], "-port") == 0) && (argc > 2)) {
            debuggerPort = (short)atoi(argv[2]);
            argv+=2; argc -=2;
#endif /* ENABLE_JAVA_DEBUGGER */

        } else if ((strcmp(argv[1], "-heapsize") == 0) && (argc > 2)) {
            char *endArg;
            long heapSize = strtol(argv[2], &endArg, 10);
            switch (*endArg) { 
                case '\0':            break;
                case 'k': case 'K':   heapSize <<= 10; break;
                case 'm': case 'M':   heapSize <<= 20; break;
                default:              printHelpText(); exit(1);
            }

            /* In principle, KVM can run with just a few kilobytes */
            /* of heap space.  We use 16 kilobytes as the minimum  */
            /* as that value is useful for some test cases.        */
            /* The maximum heap size allowed by the KVM garbage    */
            /* collector is 64 megabytes.  In practice, the        */
            /* collector is optimized only for small heaps, and    */
            /* is likely to have noticeable GC pauses with heaps   */
            /* larger than a few megabytes */
            if (heapSize < 16 * 1024) { 
                fprintf(stderr, KVM_MSG_USES_16K_MINIMUM_MEMORY "\n");
                heapSize = 16 * 1024;
            }
            if (heapSize > 64 * 1024 * 1024) {
                fprintf(stderr, KVM_MSG_USES_64M_MAXIMUM_MEMORY "\n");
                heapSize = 64 * 1024 * 1024;
            }

            /* Make sure the heap size is divisible by four */
            heapSize -= heapSize%CELL;
           
            argv+=2; argc -=2;
            RequestedHeapSize = heapSize;
        } else if ((strcmp(argv[1], "-classpath") == 0) && argc > 2) {
            if (JamEnabled) {
                fprintf(stderr, KVM_MSG_CANT_COMBINE_CLASSPATH_OPTION_WITH_JAM_OPTION);
                exit(1);
            }
            UserClassPath = argv[2];
            argv+=2; argc -=2;

#if INCLUDEDEBUGCODE

#define CHECK_FOR_OPTION_IN_ARGV(varName, userName)  \
        } else if (strcmp(argv[1], userName) == 0) { \
            varName = 1;                             \
            argv++; argc--;                          

        FOR_EACH_TRACE_FLAG(CHECK_FOR_OPTION_IN_ARGV)

        } else if (strcmp(argv[1], "-traceall") == 0) {
#           define TURN_ON_OPTION(varName, userName) varName = 1;
            FOR_EACH_TRACE_FLAG(TURN_ON_OPTION)
            argv++; argc--;

#endif /* INCLUDEDEBUGCODE */

#if USE_JAM
        } else if (strcmp(argv[1], "-jam") == 0) {
            JamEnabled = TRUE;
            argv++; argc--;
        } else if (JamEnabled && (strcmp(argv[1], "-repeat") == 0)) {

#if ENABLE_JAVA_DEBUGGER
            if (debuggerActive) {
                fprintf(stderr,
                    KVM_MSG_CANT_COMBINE_DEBUGGER_OPTION_WITH_REPEAT_OPTION);
                exit(1);
            }
#endif /* ENABLE_JAVA_DEBUGGER */
            JamRepeat = TRUE;
            argv++; argc--;
        } else if (JamEnabled && (strcmp(argv[1], "-appsdir") == 0)
                  && argc > 2) {
            jamInstalledAppsDir = argv[2];
            argv+=2; argc-=2;
#endif /* USE_JAM */

        } else {
            break;
        }
    }

    /* Skip program name */
    argc--;
    argv++;

    if (UserClassPath == NULL && !JamEnabled) { 
        UserClassPath = getenv("classpath");
        if (UserClassPath == NULL) { 
            /* Just in case environment variable reading is case sensitive */
            UserClassPath = getenv("CLASSPATH");
            /* Use "." as the default classpath */
            if (UserClassPath == NULL) { 
                UserClassPath = ".";
            }
        }
    }

#if USE_JAM
    if (JamEnabled) {
        if (argc != 1 ) {
            fprintf(stderr, 
                    KVM_MSG_EXPECTING_HTTP_OR_FILE_WITH_JAM_OPTION);
            exit(1);
        }
        JamInitialize(jamInstalledAppsDir);
        do {
            result = JamRunURL(argv[0], JamRepeat);
            if (result == JAM_RETURN_ERR) {
                break;
            }
        } while(JamRepeat);
        JamFinalize();
    } else
#endif /* USE_JAM */

    {
 
        /* Call the portable KVM startup routine */
        result = StartJVM(argc, argv);

#if ENABLEPROFILING
        /* By default, the VM prints out profiling information */
        /* upon exiting if profiling is turned on. */
        printProfileInfo();
#endif /* ENABLEPROFILING */

        /* If no classfile was provided, print help text */
        if (result == -1) printHelpText();
    }

    return result;
}

