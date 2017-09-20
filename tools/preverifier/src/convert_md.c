/*
 * @(#)convert_md.c    1.2 00/11/22
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifdef UNIX 
#include <langinfo.h>
#include <iconv.h>
#include <locale.h>

#define UTF8 "UTF-8"

static iconv_t 
open_iconv(const char* to, const char* from) {
    iconv_t ic = iconv_open(to, from);
    if (ic == (iconv_t)-1) { 
        if (errno == EINVAL) { 
            /* There is a bug in some versions of Solaris in which
             * nl_langinfo() returns a string beginning with ISO, but you
             * have to remove the ISO before calling open_iconv.
             */
            if (strncmp(from, "ISO", 3) == 0) { 
                /* Try again, but with the ISO in front of the "from" */
                return open_iconv(to, from + 3);
            } else if (strncmp(to, "ISO", 3) == 0) { 
                /* Try again, but with the ISO in front of the "to" */
                return open_iconv(to + 3, from);
            } else { 
                fprintf(stderr, "%s to %s not supported on this platform\n", 
                        from, to);
            }
        } else { 
            perror("iconv_open error: ");
        }
        exit(1);
    }
    return ic;
}

static char*
get_langinfo_codeset()
{
    static char *name = NULL;

    if (name == NULL) { 
        name = nl_langinfo(CODESET);
        if (name == NULL || strlen(name) == 0) { 
            name = "ISO8859-1";
        }
    }
    return name;
}

int native2utf8(const char* from, char* to, int buflen) {
    int ret;
    size_t ileft, oleft;
    char *langinfo_codeset = get_langinfo_codeset();
    iconv_t ic;
    if (strncmp(langinfo_codeset, UTF8, 5) == 0) {
        /* don't invoke 'iconv' functions to do the
         * conversion if it's already in UTF-8 encoding 
         */
        memcpy(to, from, buflen);
        return 0;
    }

    ic = open_iconv(UTF8, get_langinfo_codeset());
    memset(to, 0, buflen);
    ileft = strlen(from);
    oleft = buflen;
    
    ret = iconv(ic, &from, &ileft, &to, &oleft);
    if (ret == (size_t)-1) {
        fprintf(stderr, "native2utf8:Failed to convert (err=%d)\n", ret);
        exit(1);
    }
    iconv_close(ic);

    return buflen-oleft;
}

int utf2native(const char* from, char* to, int buflen) {
    int ret;
    size_t ileft, oleft;
   
    char *langinfo_codeset = get_langinfo_codeset();
    iconv_t ic;

    if (strncmp(langinfo_codeset, UTF8, 5) == 0) {
        /* Don't do the conversion if it's 
         * already in UTF-8 encoding 
         * Copy over the 'from' to 'to'.
         */
        memcpy(to, from, buflen);
        return 0;
    }

    ic = open_iconv(get_langinfo_codeset(), UTF8);
    memset(to, 0, buflen);
    ileft = strlen(from);
    oleft = buflen;

    ret = iconv(ic, &from, &ileft, &to, &oleft);
    if (ret == (size_t)-1) {
        fprintf(stderr, "utf2native:Failed to convert (err=%d)\n", ret);
        exit(1);
    }
    iconv_close(ic);

    return buflen-oleft;
}

#endif
#ifdef WIN32

#include <WINDOWS.H>

#include "oobj.h"
#include "utf.h"

int native2utf8(const char* from, char* to, int buflen) {
    int len;
    unsigned short unicode[BUFSIZ];
    len = MultiByteToWideChar(CP_ACP, 0, from, -1, &unicode[0], BUFSIZ);
    unicode2utf(&unicode[0], len-1, to, buflen);
    return utfstrlen(to);
}
int utf2native(const char* from, char* to, int buflen) {
    int len, len2;
    unsigned short unicode[BUFSIZ];
    utf2unicode((char*)from, &unicode[0], BUFSIZ, &len);
    len2 = WideCharToMultiByte(CP_ACP, 0, &unicode[0], len, to,
        buflen, NULL, NULL);
    to[len2]=0;
    return len2;
}
#endif

