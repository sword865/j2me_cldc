/*
 * @(#)path_md.h	1.4 03/01/14
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

#ifndef _PATH_MD_H_
#define _PATH_MD_H_

#define    DIR_SEPARATOR        '/'

#ifdef UNIX
#define    LOCAL_DIR_SEPARATOR        '/'
#define PATH_SEPARATOR          ':'

#include <dirent.h>
#endif
#ifdef WIN32

#define    LOCAL_DIR_SEPARATOR        '\\'
#define PATH_SEPARATOR          ';'

#include <direct.h>
struct dirent {
    char d_name[1024];
};

typedef struct {
    struct dirent dirent;
    char *path;
    HANDLE handle;
    WIN32_FIND_DATA find_data;
} DIR;

DIR *opendir(const char *dirname);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);

#endif

#endif /* !_PATH_MD_H_ */
