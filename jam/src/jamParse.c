/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 * 
 */

/*=========================================================================
 * SYSTEM:    Micro Browser
 * SUBSYSTEM: Java Application Manager.
 * FILE:      jamParse.c
 * OVERVIEW:  Parse contents of JAM files.
 * AUTHOR:    Ioi Lam, Consumer & Embedded, Sun Microsystems, Inc.
 *            KVM integration by Todd Kennedy and Sheng Liang
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
 * Functions
 *=======================================================================*/

/*
 * JamGetProp
 *
 *      Parse a text buffer of property name-value pairs and return
 *      the value of the property identified by <propName>. <buffer>
 *      is a NUL-terminated string containing text in the following format:
 *
 *              "Prop-Name1: prop value 1\nProp-Name2: prop value 2..."
 *
 * Return-value:
 *
 *      A pointer into the buffer of the first non-space character of
 *      the property value. <*length> returns the length of the property
 *      value, excluding any trailing white-space characters.
 *
 *      If the given property is not found in <buffer>, NULL is returned.
 */
char* JamGetProp(char* buffer, char* name, int* length) {
    char* p;
    char* retval;
    int len;

    for (p=buffer;*p;) {
        if (strncmp(p, name, strlen(name)) != 0) {
            while (*p) {
                if (*p == '\n') {
                    ++p;
                    break;
                }
                ++ p;
            }
            continue;
        }
        p += strlen(name);
        while (*p && *p != '\n' && IS_SPACE(*p)) {
            p++;
        }
        if (*p == ':') {
            p++;
        }
        while (*p && *p != '\n' && IS_SPACE(*p)) {
            p++;
        }
        retval = p;
        while (*p && *p != '\n') {
            p++;
        }
        while (p > retval && IS_SPACE(*p)) {
            p--;
        }
        len = (p - retval + 1);

        if (length == NULL) { /* DUP the string */
            char * newStr;
            if ((newStr = browser_malloc(len + 1)) == NULL) {
                return NULL;
            }
            strnzcpy(newStr, retval, len);
            newStr[len] = 0;
            for (p=newStr; *p; p++) {
                if (*p == '\r') {
                    *p = '\0';
                }
            }
            return newStr;
        } else {
            *length = len;
            return retval;
        }
    }
    return NULL;
}

/*
 * 1 == OK
 * 0 = BAD version
 */
static int getVersion(char* ver, int verLen,
                      int *major, int *minor, int *micro) {
    int ma = 0, mi = 0, mc = 0;

    char* p;
    char* dot;

    *major = -1;
    *minor = -1;
    *micro = -1;

    if (ver == NULL) {
        return 0;
    }

    if (verLen == -1) {
        verLen = strlen(ver);
    }
                          
    /*
     * Get major version
     */
    for (p=ver; *p && *p != '.' && p - ver < verLen; p++) {
        if (*p >= '0' && *p <= '9') {
            ma *= 10;
            ma += *p - '0';
        } else {
            return 0;
        }
    }
    if (*p != '.') {
        return 0;
    }
    if (p - ver > 3) {
        return 0;
    }
    dot = p;

    /*
     * Get minor version.
     */
    for (p=dot+1; *p && *p != '.' && p - ver < verLen; p++) {
        if (*p >= '0' && *p <= '9') {
            mi *= 10;
            mi += *p - '0';
        } else {
            return 0;
        }
    }
    if (p - dot > 3) {
        return 0;
    }

    /*
     * Get micro version; if it exists..
     */
    if (*p == '.') {
        dot = p;
        for (p=dot+1; *p && p - ver < verLen; p++) {
            if (*p >= '0' && *p <= '9') {
                mc *= 10;
                mc += *p - '0';
            } else {
                return 0;
            }
        }
        if (p - dot > 3) {
            return 0;
        }
    }

    *major = ma;
    *minor = mi;
    *micro = mc;

    return 1;
}

/*
 * 1 == OK
 * 0 = BAD version
 */
int JamCheckVersion(char* ver, int verLen) {
    int major, minor, micro;

    return getVersion(ver, verLen, &major, &minor, &micro);
}

/*
 * Returns < 0 if ver1 < ver2
 * Returns = 0 if ver1 = ver2
 * Returns > 0 if ver1 > ver2
 *
 * A version must be "<major>.<minor>".
 * Both <major> and <minor> must be a string of 1 to 3 decimal digits.
 */
int JamCompareVersion(char* ver1, int ver1Len, char* ver2, int ver2Len) {
    int major1, minor1, micro1;
    int major2, minor2, micro2;
    int error1, error2;

    error1 = (getVersion(ver1, ver1Len, &major1, &minor1, &micro1) != 1);
    error2 = (getVersion(ver2, ver2Len, &major2, &minor2, &micro2) != 1);

    /*
     * (1) If both version strings are invalid, treat them as the same
     *     version.
     * (2) If only the second version is valid, treat the second version
     *     as newer.
     * (3) If only the first version is valid, treat the first version
     *     as newer.
     */
    if (error1 && error2) {
        return 0;
    }
    if (error1) {
        return -1;
    }
    if (error2) {
        return 1;
    }

    if (major1 < major2) {
        return -1;
    }
    if (major1 > major2) {
        return  1;
    }
    if (minor1 < minor2) {
        return -1;
    }
    if (minor1 > minor2) {
        return  1;
    }
    if (micro1 < micro2) {
        return -1;
    }
    if (micro1 > micro2) {
        return  1;
    }

    return 0;
}

void strnzcpy(char* dst, const char* src, int len) {
  strncpy(dst, src, len);
  dst[len] = '\0';
}

