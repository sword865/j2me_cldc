/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * JAM
 *=========================================================================
 * SYSTEM:    JAM
 * SUBSYSTEM: HTTP downloading.
 * FILE:      jamHttp.c
 * OVERVIEW:  Downloads web pages and JAR files for the browser and the
 *            Java Application Manager.
 * AUTHOR:    Ioi Lam
 *            KVM integration by Todd Kennedy and Sheng Liang
 *            Faster HTTP access, Efren Serra
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#define NOMSG
#include <windows.h>
#include <winsock2.h>
#include <ctype.h>

#else

#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <strings.h>

#endif

#include <stdio.h>
#include <stdlib.h>

/* Just enough of global.h and the other stuff so that this will compile */

#undef  TRUE
#undef  FALSE
#define NIL   0

typedef enum { FALSE = 0, TRUE = 1 } bool_t;

/*=========================================================================
 * Definitions and declarations
 *=======================================================================*/

#ifndef WIN32

  typedef int SOCKET;
  #define IS_INVALID(sock) ((sock) < 0)
  #define INVALID_SOCKET -1
  #define SOCKET_ERROR -1
  #define closesocket close

#else

  #define IS_INVALID(sock) ((sock) == INVALID_SOCKET)
  #define WSA_VERSION_REQD MAKEWORD(1,1)

#endif /* NOT WIN32 */

/* JAM-specific include files */
#include <jam.h>

#define MAX_BUF 1024
#define MAX_URL 256

#define DEFAULT_RETRY_DELAY 2500

/*=========================================================================
 * Functions
 *=======================================================================*/

static bool_t
parseURL(const char* url, char* host, int* port, char* path);

int
expandURL(char* oldURL, char* newURL) {
    int rc;
    int port;

    char host[MAX_URL];
    char path[MAX_URL];

    if ((rc = parseURL(oldURL, host, &port, path)) != 0) {
        sprintf(newURL, "http://%s:%d%s", host, port, path);
    } else {
        newURL[0] = '\0';
    }
    return (rc);
}

static bool_t
parseURL(const char* url, char* host, int* port, char* path)
{
    char* p;
    char* start;

    if (strncmp(url, "http://", 7) != 0) {
        fprintf(stderr, "bad protocol, must be http://\n");
        return FALSE;
    }

    /*
     * Get host name.
     */
    start = p = (char*)url + 7;
    while (*p && (isalnum((int)*p) || *p == '.' || *p == '-'|| *p == '_')) {
        p++;
    }

    if (p - start >= MAX_BUF) {
        fprintf(stderr, "bad host name: too long\n");
        return FALSE;
    }

    strnzcpy(host, start, p-start);

    /*
     * Get port number
     */
    if (*p == ':') {
        char num[10];

        p++;
        start = p;
        while (isdigit((int)*p)) {
            p++;
        }

        if ((start - p) > 5) {
            fprintf(stderr, "bad port number\n");
            return FALSE;
        }

        strnzcpy(num, start, p-start);
        *port = atoi(num);
        if (*port <= 0 || *port >= 65535) {
            fprintf(stderr, "bad port number %d\n", *port);
            return FALSE;
        }
    } else {
        *port = 80;
    }

    /*
     * Get path
     */
    if (*p == 0) {
        strcpy(path, "/");
    } else if (*p != '/') {
        fprintf(stderr, "bad path: must start with \"/\"\n");
    } else if (strlen(p) >= MAX_BUF) {
        fprintf(stderr, "bad path: too long\n");
    } else {
        strcpy(path, p);
    }
    return TRUE;
}

static char *
fetchURL_internal(const char *host, int port, const char *path, bool_t retry,
                  int *httpCodeP, int *contentTypeP,
                  int *contentLengthP, int *retryDelayP);

/*
 * Implement HTTP policy for retransmissions, etc. ...
 */
char*
HTTP_Get(const char* url, int* contentTypeP, int* contentLengthP, bool_t retry)
{
    int port;
    char host[MAX_URL];
    char path[MAX_URL];
    parseURL(url, host, &port, path);

    for(;;) {
        int httpCode, retryDelay;
        char *content = fetchURL_internal(host, port, path, retry,
                                          &httpCode, contentTypeP,
                                          contentLengthP, &retryDelay);
        if (httpCode == 200) {
            return content;
        }
        /* In all other cases, the content is NULL and should be ignored */
        if (httpCode == 503) {
            if (retryDelay > 0) {
                SLEEP_MILLIS(retryDelay);
            } else {
                /* retryDelay is a hint, we must sleep so that javaTest can
                 * get some cycles
                 */
                SLEEP_MILLIS(5);
            }
            continue;
        }
        return NULL;
    }
}

static char *
fetchURL_internal(const char *host, int port, const char *path, bool_t retry,
                  int *httpCodeP, int* contentTypeP,
                  int *contentLengthP, int *retryDelayP)
{
    int sock = -1;
    char buffer[1024]; /* we reuse this for sending and receiving the data */
    int length, contentLength;
    char *p;
    char *content = NULL;
    bool_t readBody;
    int retcode;

    /* Let's set the return args to default values */
    *httpCodeP = 404;           /* unable to make a connection */
    *contentLengthP = 0;
    *retryDelayP = 0;
    *contentTypeP = ((strcmp(path + strlen(path) - 4, ".jam")) == 0)
                                     ? CONTENT_JAVA_MANIFEST : CONTENT_HTML;
    contentLength = 0;

    /* Open the socket and connect to the server */
    {
        bool_t first;
        struct hostent* hep;
        struct sockaddr_in sin;

        memset((void*)&sin, 0, sizeof (sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons((short)port);

        hep = gethostbyname(host);
        if (hep == NULL) {
            fprintf(stderr, "Unable to resolve host name %s\n", host);
            return NULL;
        }
        memcpy(&sin.sin_addr, hep->h_addr, hep->h_length);
        for (first = TRUE;;) {
            sock = socket(PF_INET, SOCK_STREAM, 0);
            retcode = connect(sock, (struct sockaddr*)&sin, sizeof(sin));
            if (retcode >= 0) {
                break;
            } else if (!retry) {
                fprintf(stderr, "Unable to connect to %s:%ld\n",
                        host, (long)port);
                goto error;
            } else {
                if (first) {
                    fprintf(stderr,
                            "Unable to connect to %s:%ld.  Will retry\n",
                            host, (long)port);
                    first = FALSE;
                }
                closesocket(sock);
                SLEEP_MILLIS(DEFAULT_RETRY_DELAY);
            }

        }
    }
    memset(buffer, 0, sizeof(buffer));
    sprintf(buffer, "GET %s HTTP/1.0\n\n", path);
    retcode = send(sock, buffer, strlen(buffer), 0);

    /* Read as much as we can into "buffer". */
    memset(buffer, 0, sizeof(buffer));
    for (length = 0; length < sizeof(buffer); ) {
        retcode = recv(sock, buffer + length, sizeof(buffer) - length, 0);
        if (retcode > 0) {
            length += retcode;
        } else if (retcode == 0) {
            /* EOF */
            break;
        } else {
            fprintf(stderr, "Error reading socket\n");
            goto error;
        }
    }
    if (length <= 5) {
        goto error;
    }

    /* We're assuming that the buffer is long enough to hold the complete
     * header.  If not, well something is wrong.
     */
    p = strchr(buffer, ' ') + 1;        /* Skip past the first space */
    while (*p == ' ') p++;              /* Skip additional space */
    *httpCodeP = atoi(p);
    switch(*httpCodeP) {
        case 200:
            readBody = TRUE; break;
        case 503:
            readBody = FALSE; break;
        default:
            fprintf(stderr, "%s not available\n", path);
            goto error;
    }
    p = strchr(p, '\n') + 1;
    for (;;) {
        if (*p == '\n') {
            p++;
            break;
        } else if (*p == '\r' && p[1] == '\n') {
            p += 2;
            break;
        } else if (   strncmp("Content-length:", p, 15) == 0
                   || strncmp("Content-Length:", p, 15) == 0) {
            p += 15;
            while (*p == ' ') p++;        /* Skip additional space */
            contentLength = atoi(p);
        } else if (   strncmp("Content-type:", p, 13) == 0
                   || strncmp("Content-Type:", p, 13) == 0) {
            p += 13;
            while (*p == ' ') p++;        /* Skip additional space */
            if (     (*contentTypeP == CONTENT_HTML)
                     &&   (strncmp(p, "application/x-jam", 17) == 0) ) {
                *contentTypeP = CONTENT_JAVA_MANIFEST;
            }
        } else if (   strncmp("Retry-after:", p, 12) == 0
                   || strncmp("Retry-After:", p, 12) == 0) {
            p += 12;
            while (*p == ' ') p++;        /* Skip additional space */
            *retryDelayP = atoi(p) * 1000;
        }
        /* Skip to just past the '\r\n' or '\n' */
        p = strchr(p, '\n') + 1;
    }

    if (!readBody) {
        closesocket(sock);
        return NULL;
    }

    content = malloc(contentLength + 1);
    content[contentLength] = 0;    /* This saves us some grief on JAM */
    /* Set length to be the number of characters we're going to copy
     * into the content buffer */
    length = length - (p - buffer);
    memcpy(content, p, length);

    while (length < contentLength) {
        retcode = recv(sock, content + length, contentLength - length, 0);
        if (retcode < 0) {
            fprintf(stderr, "Bad read on socket");
            goto error;
        } else if (retcode == 0) {
            fprintf(stderr, "Unexpectedly reached EOF");
            goto error;
        } else {
            length += retcode;
        }
    }

    closesocket(sock);
    *contentLengthP = contentLength;
    return content;

error:
    if (sock >= 0) {
        closesocket(sock);
    }
    if (content != NULL) {
        browser_free(content);
    }
    return NULL;
}

void HTTP_Initialize() {
#ifdef WIN32
    WSADATA wsaData;
    if (WSAStartup(WSA_VERSION_REQD, &wsaData ) != 0) {
        fprintf(stderr, "Cannot initialize WSA\n");
        exit(1);
    }
#endif
}

