/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    Micro Browser
 * SUBSYSTEM: Java Application Manager (JAM).
 * FILE:      jam.c
 * OVERVIEW:  Downloads and manages Java applications that are stored in
 *            JAR files. This implementation depends on a file system --
 *            all JAR files are saved into the "instapps" directory.
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

#define MAX_BUF 22
#define MAX_URL 256

/*=========================================================================
 * Variables
 *=======================================================================*/

static int numApps;
static JamApp* appList;
static JamApp* currentApp = NULL;

/*
 * The app that we just downloaded. After we download an app, we go back
 * to the main screen and highlight it. This variable tells us which app
 * to highlight.
 */
static JamApp* downloadedApp = NULL;

/*=========================================================================
 * Functions
 *=======================================================================*/

static int GetJarURL(char* jamContent, char* parentURL, char* jarURL);
static int JamRunApp(JamApp* app);

/*
 * GetInstalledApp --
 *
 *      Parse the <jamContent> and search for the currently installed
 *      version of the given application. If found, it's returned in
 *      <appPtrPtr>.
 *
 * RETURN:
 *      TRUE:   OK
 *      FALSE:  An error has occurred. Stop the installation dialog now.
 */

static int
GetInstalledApp(jamContent, parentURL, retval, appPtrPtr, verCompare_ret)
    char* jamContent;    /* Content of the JAM descriptor file */
    char* parentURL;     /* The parent directory of the URL of the
                          * JAM descriptor file. E.g., if the JAM
                          * desc file is "http://foo.com/bar/x.jam",
                          * parentURL is "http://foo.com/bar/".
                          */
    int* retval;
    JamApp** appPtrPtr;  /* [Out] If this app has already been
                          * installed on the phone (regardless of
                          * its version), it's returned here.
                          */
    int* verCompare_ret; /* [Out] If there's an existing version,
                          * how is it compared to the requested
                          * version
                          */
{
    int versionLen;
    char* version;
    JamApp* app;
    char jarURL[MAX_URL];

    version = JamGetProp(jamContent, APPLICATION_VERSION_TAG, &versionLen);
    if (version != NULL) {
        if (!JamCheckVersion(version, versionLen)) {
            *retval = JamDownloadErrorPage(JAM_INVALID_MANIFEST);
            return (FALSE);
        }
    }

    memset(jarURL, 0, MAX_URL);
    if (!GetJarURL(jamContent, parentURL, jarURL)) {
        *retval =  JamDownloadErrorPage(JAM_BAD_URL);
        return (FALSE);
    }

    if (!JamCheckSecurity(jarURL, parentURL)) {
        fprintf(stderr,
                "JAR file (%s) comes from different site than JAM (%s)\n",
                jarURL, parentURL);
        *retval = JamDownloadErrorPage(JAM_BAD_URL);
        return (FALSE);
    }

    *appPtrPtr = NULL;

    for (app = appList; app; app = app->next) {
        if (strcmp(jarURL, app->jarURL) == 0) {
            *appPtrPtr = app;
            *verCompare_ret =
                JamCompareVersion(app->version, -1, version, versionLen);
            return (TRUE);
        }
    }

    return (TRUE);
}

/*
 * Check the minimum kVM version. If not supported, return FALSE.
 */
static int
CheckKVMVersion(jamContent, retval)
    char* jamContent; /* Content of the JAM descriptor file */
    int*  retval;     /* [Out] Indicates where the browser should
                       * go if this function returns FALSE
                       */
{
    int len;
    char* appConfig;

    if ((appConfig = JamGetProp(jamContent, KVM_VERSION_TAG, &len)) != NULL) {
        int   versionLen;
        int   ok = FALSE;
        int   appConfigLen;

        char* kvmConfig = getSystemProperty("microedition.configuration");
        char* kvmVersion = strchr( kvmConfig, '-' );
        int   kvmConfigLen = (int)(kvmVersion - kvmConfig);

        char* appVersion;
        char* versionList;
        char* versionPtr;

        versionList = (char*)browser_malloc(sizeof(char) * len + 1);
        strncpy(versionList, appConfig, len);
        versionList[len] = '\0';

        versionPtr = strchr(versionList, ',');

        while (versionPtr != NULL) {
            *versionPtr = '\0';
            versionPtr = strchr(versionPtr+1, ',');
        }

        versionLen = 0;
        while ((versionLen < len) && (ok == FALSE)) {

            versionPtr = versionList + versionLen;
            appVersion = strchr( versionPtr, '-' );

            do {
                if ((appVersion != NULL) && (kvmVersion != NULL)) {

                    appConfigLen = (int)(appVersion - versionPtr);
                    if (appConfigLen != kvmConfigLen) {
                       break;
                    }

                    if (strncmp(kvmConfig, versionPtr, kvmConfigLen) != 0) {
                        break;
                    }

                    if (JamCompareVersion(kvmVersion+1, -1, appVersion+1, -1)
                        != 0) {
                        break;
                    }

                    ok = TRUE;
                }
            } while (FALSE);

            versionLen += strlen(versionPtr) + 1;
        }

        /* Free the copy */
        browser_free(versionList);

        if (!ok) {
            *retval = JamError(
                "Application won't work on this device. Choose another app.");
            return (FALSE);
        }
    }

    return (TRUE);
}

/*
 * AskInstallApp --
 *
 *      Ask the user if the app should be installed or updated.
 *
 * RETURN: TRUE == user wants to install.
 */
static int
AskInstallApp(jamContent, parentURL, oldApp, retval, jarLength_ret)
    char* jamContent; /* Content of the JAM descriptor file */
    char* parentURL;
    JamApp* oldApp;   /* The older installed version of the app */
    int* retval;
    int* jarLength_ret;
{
    int len;
    char* p;
    char size[MAX_BUF];
    char name[MAX_BUF + 10];

    if (!CheckKVMVersion(jamContent, retval)) {
        return (FALSE);
    }

    /*
     * Get the name of the application
     */
    p = JamGetProp(jamContent, APPLICATION_NAME_TAG, &len);

    if (p == NULL) {
        *retval = JamDownloadErrorPage(JAM_INVALID_MANIFEST);
        return (FALSE);
    } else {
        int maxlen = MAX_BUF - 3;
        memset(name, 0, sizeof (name));

        if (len > maxlen) {
            strnzcpy(name, p, maxlen);
            strcat(name, "...");
        } else {
            strnzcpy(name, p, len);
        }
    }

    /*
     * Get size of JAR file.
     */
    p = JamGetProp(jamContent, JAR_FILE_SIZE_TAG, &len);

    if (p == NULL) {
        *retval = JamDownloadErrorPage(JAM_INVALID_MANIFEST);
        return (FALSE);
    } else {
        if (len > 15) {
            len = 15;
        }
        strnzcpy(size, p, len);
        *jarLength_ret = atol(size);
    }

    return (TRUE);
}

static void
AddApp(JamApp* newApp, int jarSize) {

    JamApp** japp;

    ++numApps;

    /*
     * See how much storage we have used.
     */
    ASSERT(JamGetUsedSpace() <= JamGetTotalSpace());

    /*
     * Insert jam app into sorted list.
     */
    japp = &appList;
    for (; *japp != NULL; japp = &(*japp)->next) {
        if (strcmp(newApp->appName, (*japp)->appName) <= 0) {
            break;
        }
    }

    newApp->next = *japp;
    *japp = newApp;
}

static void
DeleteApp(JamApp* app, int delJarFile) {

    JamApp** japp;

    japp = &appList;

    for (; *japp != NULL; japp = &(*japp)->next) {

        if (*japp == app) {
            *japp = app->next;
            break;
        }
    }

    --numApps;

    if (delJarFile) {
        JamDeleteJARFile(app);
    }

    JamFreeApp(app);
}

static char*
FILE_Get(const char* url, int* contenttype, int* contentlength)
{
    int i;
    int fd = -1;

    struct stat stat_buf;
    const char *path;
    char* content = NULL;

    i = strlen(url);

    if (i > 4 && strcmp(&url[i - 4], ".jam") == 0) {
        *contenttype = CONTENT_JAVA_MANIFEST;
    } else {
        *contenttype = CONTENT_HTML;
    }

    path = url + 5;     /* skip "file:" */

    fd = open(path, O_RDONLY | O_BINARY);
    if (fd < 0) {
        /* 404 not found */
        goto error;
    }

    if (fstat(fd, &stat_buf) < 0) {
        goto error;
    }

    content = (char*)browser_malloc(stat_buf.st_size + 1);

    if (content == NULL) {
        goto error;
    }

    if (read(fd, content, stat_buf.st_size) != stat_buf.st_size) {
        goto error;
    }

    content[stat_buf.st_size] = 0;
    *contentlength = stat_buf.st_size;

    return content;

 error:
    JamError("Error opening file\n");
    browser_free(content);

    if (fd >= 0) {
        close(fd);
    }

    return (NULL);
}

static char*
GenericGet(const char* url, int* contentTypeP, int* contentLengthP,
           bool_t retryOnError) {
    if (strncmp(url, "http:", 5) == 0) {
        return HTTP_Get(url, contentTypeP, contentLengthP, retryOnError);
    } else if (strncmp(url, "file:", 5) == 0) {
        return FILE_Get(url, contentTypeP, contentLengthP);
    } else {
        fprintf(stderr, "Unknown type in url %s\n", url);
        return NULL;
    }
}

static JamApp*
DownloadApp(jamContent, parentURL, oldApp, retval, jarLength)
    char* jamContent;
    char* parentURL;
    JamApp* oldApp;
    int* retval;
    int jarLength;
{
    int badURL = 0;
    int contentType, contentLength;
    int useOnceLen;

    char jarURL[MAX_URL];
    char* jarContent = NULL, *jarName;

    JamApp* newApp = NULL;
    char* useOnce;

    memset(jarURL, 0, sizeof (jarURL));

    if (!GetJarURL(jamContent, parentURL, jarURL)) {
        badURL = 1;
        goto error;
    }

    newApp = (JamApp*)browser_ckalloc(sizeof (JamApp));

    if (newApp == NULL) {
        goto error;
    }

    /*
     * Download the jar into apps directory.
     */

    jarContent = GenericGet(jarURL, &contentType, &contentLength, TRUE);
    if (jarContent == NULL) {
        badURL = 1;
        goto error;
    }

    jarName = strrchr(jarURL, '/') + 1;
    ASSERT(jarName != NULL && *jarName != 0);

    newApp->appName = JamGetProp(jamContent, APPLICATION_NAME_TAG,NULL);
    if (newApp->appName == NULL) {
        goto error;
    }

    newApp->mainClass = JamGetProp(jamContent, MAIN_CLASS_TAG, NULL);
    if (newApp->mainClass == NULL) {
        goto error;
    }

    newApp->jarName = strdup(jarName);
    if (newApp->jarName == NULL) {
        goto error;
    }

    newApp->jarURL = strdup(jarURL);
    if (newApp->jarURL == NULL) {
        goto error;
    }

    newApp->version = JamGetProp(jamContent, APPLICATION_VERSION_TAG, NULL);

    if (!JamSaveJARFile(newApp, jarContent, contentLength)) {
        goto error;
    }

    browser_free(jarContent);

    /*
     * Add to list of apps
     */
    AddApp(newApp, jarLength);

    if (oldApp != NULL) {
        DeleteApp(oldApp, 0);
        JamFreeAppUsedSpace(oldApp);
    }

    useOnce = JamGetProp(jamContent, USE_ONCE_TAG, &useOnceLen);
    if (useOnce == NULL || useOnceLen != 3
        || strncmp(useOnce, "yes", useOnceLen) != 0) {

        JamSaveAppsDatabase();
    }

    return (newApp);

error:
    JamFreeApp(newApp);
    browser_free(jarContent);

    *retval = JamDownloadErrorPage(badURL);

    return (NULL);
}

/*
 * InstallApp --
 *
 *      Prompt the user and install the latest version of the app. This
 *      function may be called when an older version of the app has been
 *      installed on the phone but jamContent contains a newer version.
 *
 *      When successful, if an older version exists but a newer
 *      version is installed, the older version is deleted.
 *
 * RETURN:
 *      TRUE:   OK
 *      FALSE:  An error has occurred. Stop the installation dialog now.
 */

static int
InstallApp(jamContent, parentURL, retval, appPtrPtr)
    char *jamContent;   /* Content of the JAM descriptor file */
    char *parentURL;    /* The parent directory of the URL of the
                         * JAM descriptor file. E.g., if the JAM
                         * desc file is "http://foo.com/bar/x.jam",
                         * parentURL is "http://foo.com/bar/".
                         */
    int* retval;        /* [Out] Indicates where the browser should
                         * go if this function returns FALSE
                         */
    JamApp** appPtrPtr; /* [In] Contains the previously installed
                         * version, or NULL if this app has
                         * not been installed.
                         * [Out] Contains the newly installed app. If
                         * the app has been updated, the previous
                         * installed version is freed.
                         */
{
    int jarLength;
    JamApp* newApp;
    JamApp* oldApp;

    oldApp = *appPtrPtr;
    if (!AskInstallApp(jamContent, parentURL, oldApp, retval, &jarLength)) {
        return (FALSE);
    }

    newApp = DownloadApp(jamContent, parentURL, oldApp, retval, jarLength);
    if (newApp == NULL) {
        return (FALSE);
    }

    *appPtrPtr = newApp;

    return (TRUE);
}

static int
GetJarURL(char* jamContent, char* parentURL, char* jarURL) {

    char* p;
    char* q;
    int len;
    int parentLen;
    char expandedURL[MAX_URL];

    p = JamGetProp(jamContent, JAR_FILE_URL_TAG, &len);
    if (p == NULL) {
        goto invalid_manifest;
    }

    if (strncmp(p, "http:", 5) == 0 || strncmp(p, "file:", 5) == 0) {

        if (len >= MAX_URL) {
            goto invalid_manifest;
        }
        strnzcpy(jarURL, p, len);

    } else {

        q = strrchr(parentURL, '/');

        if (q == NULL) {
            goto invalid_manifest;
        }

        parentLen = (q - parentURL + 1);
        if ((parentLen + len) >= MAX_URL) {
            goto invalid_manifest;
        }

        strnzcpy(jarURL, parentURL, parentLen);
        strncat(jarURL, p, len);
    }

    if ((strncmp(parentURL, "http:", 5) == 0) &&
        !expandURL(jarURL, expandedURL) ) {

        goto invalid_manifest;
    }

    return (1);

 invalid_manifest:
    return (0);
}

/*
 * JamInvokeDescriptorFile --
 *
 *      Invoke the application refered to by the JAM descriptor file
 *      (whose content is stored in <jamContent>). The application may
 *      be downloaded and executed according to the following
 *      conditions:
 *
 * (1) Download:
 *
 *      If the app has already been installed, and the installed version
 *      is not older than stated by <jamContent>, the application is not
 *      downloaded.
 *
 *      Otherwise, with the user's permission we download the latest
 *      version of the application and installs it on the phone.
 *
 * (2) Execution:
 *
 *      If the app has been installed after step (1), the user is prompted
 *      whether the app should be executed. If the answer is yes,
 *      the application is executed.
 */

static int
JamInvokeDescriptorFile(char *jamContent, const char *parentURL)
{
    int retval;
    int len;
    int useOnceLen;
    int verCompare;
    char* useOnce;
    JamApp* app;

    if (JamGetProp(jamContent, APPLICATION_NAME_TAG, &len) == NULL ||
        JamGetProp(jamContent, JAR_FILE_SIZE_TAG, &len) == NULL) {
        retval = JamDownloadErrorPage(JAM_INVALID_MANIFEST);
        return retval;
    }

    if (!GetInstalledApp(jamContent, parentURL, &retval, &app, &verCompare)) {
        return (retval);
    }

    if (app == NULL || verCompare == EXI_OLDER_THAN_REQ) {
        /* Not installed yet, or needs to update: prompt for installation. */
        if (InstallApp(jamContent, parentURL, &retval, &app) && app != NULL) {
            useOnce = JamGetProp(jamContent, USE_ONCE_TAG, &useOnceLen);
            if (useOnce != NULL && useOnceLen == 3
                && strncmp(useOnce, "yes", useOnceLen) == 0) {
                retval = JamRunApp(app);
                DeleteApp(app, 1);
                return (retval);
            } else {
                downloadedApp = app;
            }
        } else {
            return (retval);
        }
    }

    retval = JamRunApp(app);
    return retval;
}

/*
 * JamRunApp --
 *
 *      Execute the installed application given by <app>
 */
static int
JamRunApp(JamApp* app)
{
     int retval = JAM_RETURN_ERR;
     char* argv[1] = {NULL};

     currentApp = app;

     UserClassPath = JamGetCurrentJar();

     argv[0] = strdup(app->mainClass);

     if (argv[0] == NULL) {
         fprintf(stderr, "out of memory\n");
         goto done;
     }

     retval = StartJVM(1, argv);

 done:
     browser_free(argv[0]);
     return (retval);
}

/*
 * Allocate browser-heap chunk that's filled with zero
 */
char* browser_ckalloc(int size) {

    char* p = browser_malloc(size);
    if (p) {
        memset(p, 0, size);
    }
    return (p);
}

/*
 * returns true iff both url are from the same web site
 */
int JamCheckSecurity(char* url1, char* url2) {
    char* p1;
    char* p2;

    if (strncmp(url1, url2, 7) != 0) {
        return (0);
    }

    if (strncmp(url2, "http://", 7) != 0 &&
        strncmp(url2, "file://", 7) != 0) {

        return (0);
    }

    for (p1=url1+7, p2=url2+7; *p1 && *p2; p1++, p2++) {
        if ((*p1 == '/' || *p1 == ':') &&
            (*p2 == '/' || *p2 == ':')) {

            return (1);
        }

        if (*p1 != *p2) {
            return (0);
        }
    }

    if ((*p1 == 0 || *p1 == '/' || *p1 == ':') &&
        (*p2 == 0 || *p2 == '/' || *p2 == ':')) {

        return (1);
    } else {
        return (0);
    }
}

/*
 * Free the memory occupied by the JamApp structure.
 */
void JamFreeApp(JamApp* app) {
    browser_free(app->appName);
    browser_free(app->jarName);
    browser_free(app->jarURL);
    browser_free(app->version);
    browser_free(app->mainClass);
    browser_free(app);
}

/*
 * [1] Read the list of installed apps from the file
 *     <demoRoot>/samples/instapps.
 * [2] Calculate used and available storage space.
 */
static void InitInstalledApps()
{
    JamApp* app;

    appList = NULL;
    numApps = 0;

    if (JamOpenAppsDatabase() == 0) {
        /*
         * No app database exists -- we don't have any installed
         * applications.
         */
        return;
    }
    while ((app = JamGetNextAppEntry()) != NULL) {
        AddApp(app, JamGetAppTotalSize(app));
    }

    JamCloseAppsDatabase();
}

/*
 * Initializes the JAM module:
 *
 * [1] Discover the location of the system classes and installed
 *     applications.
 * [2] Read the list of installed apps.
 * [3] Calculate used and available storage space.
 */
void JamInitialize(char* appsDir) {
    JamInitializeStorage(appsDir);
    InitInstalledApps();
    JamInitializeUsedSpace(appList);
    HTTP_Initialize();
}

void JamFinalize() {
}

int JamGetAppCount(void) {
    return numApps;
}

JamApp* JamGetCurrentApp(void) {
    return currentApp;
}

JamApp* JamGetAppList(void) {
    return (appList);
}

int JamDownloadErrorPage(int error) {

    switch (error) {

    case JAM_BAD_URL:
        return JamError("The URL for the application is invalid."
                        " Contact your ISP for help.");

    case JAM_INVALID_MANIFEST:
        return JAM_RETURN_ERR;

    case JAM_MISC_ERROR:
        return JAM_RETURN_ERR;

    default:
        return JamError("Couldn't install the application."
                        " Contact your ISP for help.");
    }
}

int JamRunURL(const char* url, bool_t retry) {

    int result = JAM_RETURN_ERR;
    int contentType;
    int contentLength;
    char* content;

    content = GenericGet(url, &contentType, &contentLength, retry);

    if (content != NULL) {
        if (contentType == CONTENT_JAVA_MANIFEST) {
            result = JamInvokeDescriptorFile(content, url);
        } else {
            fprintf(stderr, "URL %s has the wrong MIME type\n", url);
        }
        browser_free(content);
    } else {
        /* If content is NULL, we presume that the called function
         * has already printed an error message
         */
    }
    return result;
}

int JamError(char* msg) {
    fprintf(stderr, "%s\n", msg);
    return (JAM_RETURN_ERR);
}

