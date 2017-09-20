/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 */

/*=========================================================================
 * SYSTEM:    Micro Browser
 * SUBSYSTEM: Java Application Manager.
 * FILE:      jamStorage.c
 * OVERVIEW:  JAM storage management. This particular implementation
 *            is a simulated environment that uses the file system to
 *            simulate persistent storage on a small device.
 * AUTHOR:    Ioi Lam, Consumer & Embedded, Sun Microsystems, Inc.
 *            KVM integration by Todd Kennedy and Sheng Liang
 *            Faster HTTP access, Efren Serra
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* JAM-specific include files */
#include <global.h>
#include <jam.h>

/*=========================================================================
 * Definitions and declarations
 *=======================================================================*/

#define STRNCMP1(s1, s2) strncmp((s1), (s2), strlen((s1)))

/*
 * Maximum num of bytes that will appear on a single line of the apps
 * database file (list.txt)
 */
#define MAX_LINE 256

/*
 * Maximum num of bytes that can appear on a file system pathname
 */
#define JAM_MAX_PATH 1024

/*=========================================================================
 * Variables
 *=======================================================================*/

static char appsDir[250];
static int  totalSpace = -1;
static int  usedSpace  = 0;

static FILE* AppDBFILE = NULL;

/*=========================================================================
 * Functions
 *=======================================================================*/

static void _GetStoredFileName(char* buf, char* name);
static void _InitTotalSpace(void);

static void
ensure_dir_exists(const char *dir) {

    struct stat stat_buf;
    char *parent;
    char *q;
    if (dir[0] == 0) {
        return;
    }
    parent = strdup(dir);
    q = strrchr(parent, '/');
    if (q) {
        *q = 0;
        ensure_dir_exists(parent);
    }
    if (stat(dir, &stat_buf) < 0) {
        int res;

#ifdef WINDOWS
        res = mkdir(dir);
#endif
#ifdef UNIX
        res = mkdir(dir, 0755);
#endif

        if (res != 0) {
            fprintf(stderr, "Can't create directory: %s. Consider using the -jam -appsdir <other-dir> option.\n", dir);
            exit(1);
        }
    }
    browser_free(parent);
}

static void
_InitTotalSpace(void) {

    /* Initialize total space */
    char* env;
    env = getenv("JAM_SPACE");
    if (env != 0) {
        totalSpace = atoi(env);
    } else {
        totalSpace = 65536;
    }

    if (totalSpace < 4000) {
        totalSpace = 4000;
    }
}

void
JamInitializeStorage(const char *dir) {

    strcpy(appsDir, dir);
    ensure_dir_exists(dir);

#ifdef WINDOWS
    /*
     * Windows crashes if we have a mix of forward and backward slashes...
     */
    { 
        char* p;
        for (p = appsDir; *p; p++) {
            if (*p == '\\') {
                *p = '/';
            }
        }
    }
#endif /* WINDOWS */

    _InitTotalSpace();
}

void JamInitializeUsedSpace(JamApp* app) {

    /* Initialize used space */
    JamApp* appPtr;
    if (app == NULL) {
        return;
    }

    usedSpace = 0;
    for (appPtr = app; appPtr != NULL; appPtr = appPtr->next) {
        usedSpace += JamGetAppTotalSize(appPtr);
    }
}

/*
 * Return the size of the simulated local storage space (in number of bytes).
 */
int JamGetTotalSpace(void) {
    return (totalSpace);
}

/*
 * Return the size of the simulated local storage space that are currently
 * in use (in number of bytes).
 */
int JamGetUsedSpace(void) {
    return (usedSpace);
}

/*
 * Update UsedSpace
 */
void JamFreeAppUsedSpace(JamApp *app) {

    /*
     * Update the new storage usage.
     */
    usedSpace -= JamGetAppTotalSize(app);
}

/*
 * Return the size of the simulated local storage space that are not
 * currently in use (in number of bytes).
 */
int JamGetFreeSpace(void) {
    return (totalSpace - usedSpace);
}

/*
 * Returns the size of the JAR file, as stored in the local persistent
 * storage, of the given <app>,
 */
int JamGetAppJarSize(JamApp* app) {

    struct stat stat_buf;
    char path[JAM_MAX_PATH];

    _GetStoredFileName(path, app->jarName);
    if (stat(path, &stat_buf) < 0) {
        return (-1);
    } else {
        return (stat_buf.st_size);
    }
}

int JamGetAppTotalSize(JamApp* app) {
    return (JamGetAppJarSize(app));
}

/*
 * Save the app's JAR file.
 */
int JamSaveJARFile(JamApp* app, char* jarContent, int len) {

    char path[JAM_MAX_PATH];
    FILE* fp = NULL;

    if (JamGetFreeSpace() < len) {
        fprintf(stderr, "not enough storage: needs %d, has %d\n",
                len, JamGetFreeSpace());
        goto error;
    }

    _GetStoredFileName(path, app->jarName);
    if ((fp = fopen(path, "wb+")) == NULL) {
        goto error;
    }
    
    if (fwrite(jarContent, 1, len, fp) != len) {
        goto error;
    }
    
    fclose(fp);

    /* OK */
    usedSpace += len;
    return (1);

 error:
    if (fp) {
        fclose(fp);
    }

    return (0);
}

int
JamDeleteJARFile(JamApp* app) {
    char path[JAM_MAX_PATH];

    /*
     * Update the new storage usage.
     */
    JamFreeAppUsedSpace(app);

    /*
     * Remove the JAR file.
     */
    _GetStoredFileName(path, app->jarName);
    unlink(path);

    return (1);
}

/*
 * Return the filesystem path of the given file in the simulated
 * storage area.
 */
static void _GetStoredFileName(char* buf, char* name) {
    sprintf(buf, "%s/%s", appsDir, name);
}

char* JamGetCurrentJar() {
    static char currentJar[JAM_MAX_PATH];

    _GetStoredFileName(currentJar, JamGetCurrentApp()->jarName);
    return (currentJar);
}

char* JamGetAppsDir() {
    return (appsDir);
}

/*
 * Opens the "database of installed application" for reading.
 */
int JamOpenAppsDatabase()
{
  char path[JAM_MAX_PATH];
  _GetStoredFileName(path, "list.txt");
  if ((AppDBFILE = fopen(path, "r")) == NULL) {
      return (0);
  }

  return (1);
}

/*
 * Read the next app from the "database of installed applications"
 *
 * The database is in the following format:
 *
 * BEGIN_APP
 *      JAR-NAME=<jarName>
 *      MAIN-CLASS=<mainClass>
 *      JAR-URL=<jarURL>
 *      APP-NAME=<appName>
 *      VERSION=<version>
 * END_APP
 */

JamApp* JamGetNextAppEntry() {

    FILE *fp = AppDBFILE;
    char line[MAX_LINE+1];
    JamApp *app = NULL;
    char * value;
    int i;

    for (;;) {
        if (fgets(line, MAX_LINE, fp) == NULL) {
            /* EOF */
            return NULL;
        }

        for (i=0; i<MAX_LINE; i++) {
            /* Skip (DOS or Unix) newline chars */
            if (line[i] == '\n') {
                line[i] = 0;
            }
            if (line[i] == '\r') {
                line[i] = 0;
            }
        }
        line[MAX_LINE] = 0;

        if (app == NULL) {
            if (STRNCMP1("BEGIN_APP", line) == 0) {
                app = (JamApp*)browser_ckalloc(sizeof(JamApp));
                if (app == NULL) {
                    return NULL;
                }
            }
            continue;
        }

        if (STRNCMP1("END_APP", line) == 0) {
            return app;            
        }

        if ((value = strchr(line, '=')) == NULL) {
            /* Blank line */
            continue;
        }
        value += 1;

        if (STRNCMP1("JAR-NAME", line) == 0) {
            app->jarName = strdup(value);
        }
        else if (STRNCMP1("MAIN-CLASS", line) == 0) {
            app->mainClass = strdup(value);
        }
        else if (STRNCMP1("JAR-URL", line) == 0) {
            app->jarURL = strdup(value);
        }
        else if (STRNCMP1("APP-NAME", line) == 0) {
            app->appName = strdup(value);
        }
        else if (STRNCMP1("VERSION", line) == 0) {
            app->version = strdup(value);
        }
    }
}

void JamCloseAppsDatabase() {
    fclose(AppDBFILE);
}

/*
 * Save the content of the application list into storage.
 */
int JamSaveAppsDatabase(void) {
    char path[256];
    FILE *fp;
    JamApp *app;

    _GetStoredFileName(path, "list.txt");
    if ((fp = fopen(path, "wb+")) == NULL) {
        /* JamDownloadErrorPage(); */
        return 0; /* TODO_ERROR!*/
    }
    for (app = JamGetAppList(); app; app = app->next) {
        fprintf(fp, "BEGIN_APP\n");
        fprintf(fp, "JAR-NAME=%s\n", app->jarName);
        fprintf(fp, "MAIN-CLASS=%s\n", app->mainClass);
        fprintf(fp, "JAR-URL=%s\n", app->jarURL);
        fprintf(fp, "APP-NAME=%s\n", app->appName);

        if (app->version != NULL && app->version[0] != '\0') {
            fprintf(fp, "VERSION=%s\n", app->version);
        }
        fprintf(fp, "END_APP\n");
    }
    fclose(fp);

    return 1;
}

