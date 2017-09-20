/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 */

/*=========================================================================
 * SYSTEM:    JAM
 * SUBSYSTEM: Java Application Manager.
 * FILE:      jam.h
 * OVERVIEW:  Declaration of internal functions used strictly within
 *            the JAM prototype implementation. None of these functions
 *            are public and should not be used by outside modules.
 *
 * AUTHOR:    Ioi Lam, Consumer & Embedded, Sun Microsystems, Inc.
 *            KVM integration by Todd Kennedy and Sheng Liang
 *=======================================================================*/

#ifndef _JAM_H_
#define _JAM_H_

#ifdef __cplusplus__
extern "C" {
#endif

/*=========================================================================
 * Include files
 *=======================================================================*/

/*=========================================================================
 * Definitions and declarations
 *=======================================================================*/

#define APPLICATION_NAME_TAG    "Application-Name"
#define APPLICATION_VERSION_TAG "Application-Version"
#define KVM_VERSION_TAG         "KVM-Version"
#define JAR_FILE_URL_TAG        "JAR-File-URL"
#define JAR_FILE_SIZE_TAG       "JAR-File-Size"
#define MAIN_CLASS_TAG          "Main-Class"
#define USE_ONCE_TAG            "Use-Once"

/*
 * Standard JAM descriptor file tag names.
 */
#define ST_INFO              101
#define ST_DOWNLOAD          102

#define HIDE_APP_SIZE        0
#define SHOW_APP_SIZE        1

#define EXI_OLDER_THAN_REQ    -1
#define EXI_SAME_AS_REQ       0
#define EXI_NEWER_THAN_REQ    1

#define JAM_BAD_URL           1
#define JAM_INVALID_MANIFEST  2
#define JAM_MISC_ERROR        3

#define CONTENT_HTML          1
#define CONTENT_JAVA_MANIFEST 2

#define JAM_RETURN_OK         0
#define JAM_RETURN_ERR        -1023

/* Misc utility macros */
#define IS_SPACE(c) ((c) == ' ' || (c) == '\r' || (c) == '\n' || (c) == '\t')

#if 0
#define ASSERT(x) ((x) ? 1 : (*(int*)(0x0)))
#else
#define ASSERT(x)
#endif

#ifndef SLEEP_MILLIS
#ifdef WIN32
#  define SLEEP_MILLIS(x) (Sleep((x)))
# else
#  define SLEEP_MILLIS(x) (usleep((x)*1000))
# endif
#endif

#ifndef O_BINARY
#  define O_BINARY 0
#endif

#ifndef __P
# ifdef __STDC__
#  define __P(p) p
# else
#  define __P(p) ()
# endif
#endif /* __P */

/*=========================================================================
 * Data structures
 *=======================================================================*/

typedef struct JamApp {
    char* jarName;
    char* jarURL;
    char* appName;
    char* mainClass;
    char* version;
    struct JamApp* next;
} JamApp;

/*=========================================================================
 * JAM functions
 *=======================================================================*/

/*
 * Main function called from KVM main
 */
extern int JamRunURL __P ((const char*, bool_t));

/*
 * Application management functions (platform independent)
 */
extern void JamInitialize __P ((char*));
extern int JamGetAppCount __P ((void));
extern JamApp* JamGetAppList __P ((void));
extern JamApp* JamGetCurrentApp __P ((void));
extern void JamFreeApp __P ((JamApp*));
extern char* JamGetProp __P ((char*, char*, int*));
extern int JamCheckVersion __P ((char*, int));
extern int JamCompareVersion __P ((char*, int, char*, int));
  
extern void JamFinalize __P ((void));

extern void strnzcpy __P ((char*, const char*, int));

/*
 * Storage management functions (platform specific)
 */
extern void JamInitializeStorage __P ((const char*));
extern int JamGetFreeSpace __P ((void));
extern int JamGetUsedSpace __P ((void));
extern int JamGetTotalSpace __P ((void));
extern int JamGetAppJarSize __P ((JamApp*));
extern int JamGetAppTotalSize __P ((JamApp*));

extern int JamOpenAppsDatabase __P ((void));
extern JamApp* JamGetNextAppEntry __P ((void));
extern void JamCloseAppsDatabase __P ((void));
extern int JamSaveAppsDatabase __P ((void));

extern int JamSaveJARFile __P ((JamApp*, char*, int));
extern int JamDeleteJARFile __P ((JamApp*));

/* Internal functions */
extern char* JamGetAppsDir __P ((void));
extern char* JamGetCurrentJar __P ((void));
extern void  JamInitializeUsedSpace __P ((JamApp*));

char*   browser_ckalloc(int size);
#define browser_malloc malloc
#define browser_free  free

extern int JamDownloadErrorPage __P ((int));
extern int JamCheckSecurity __P ((char*, char*));
extern int expandURL(char*, char*);

extern int JamError __P ((char*));

extern void JamFreeAppUsedSpace __P ((JamApp*));

/*
 * HTTP functions
 */
extern void HTTP_Initialize __P ((void));
extern char* HTTP_Get __P ((const char*, int*, int*, bool_t));
extern void HTTP_Finalize __P ((void));

#ifdef __cplusplus__
} // extern "C"
#endif

#endif /* _JAM_H_ */

