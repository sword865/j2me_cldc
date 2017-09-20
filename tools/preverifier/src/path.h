/*
 * @(#)path.h	1.6 03/01/14
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

#ifndef    _PATH_H_
#define    _PATH_H_

#include <sys/stat.h>
#include <path_md.h>

typedef struct {
    struct {
        unsigned long locpos;
    unsigned long cenpos;
    } jar;
    char type;
    char name[1];
} zip_t; 

/*
 * Class path element, which is either a directory or zip file.
 */
typedef struct {
    enum { CPE_DIR, CPE_ZIP } type;
    union {
    zip_t *zip;
    char *dir;
    } u;
} cpe_t;

cpe_t **sysGetClassPath(void);
void pushDirectoryOntoClassPath(char* directory);
void pushJarFileOntoClassPath(zip_t *zip);
void popClassPath(void);

bool_t 
findJARDirectories(zip_t *entry, struct stat *statbuf);

#endif    /* !_PATH_H_ */
