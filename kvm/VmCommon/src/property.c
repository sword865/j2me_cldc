/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * KVM
 *=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Property support
 * FILE:      property.c
 * OVERVIEW:  System dependent property lookup operations and 
 *            definitions.
 * AUTHOR:    Nik Shaylor, Java Software
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>

/*=========================================================================
 * Functions
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      getSystemProperty()
 * TYPE:          public operation
 * OVERVIEW:      Return a value of a property key
 * INTERFACE:
 *   parameters:  char* key (as a C string (ASCIZ))
 *   returns:     char* result (as a C string (ASCIZ))
 *
 * NOTE:          In the future, if the number of properties becomes
 *                larger, we should create an internal hashtable for
 *                properties.  Also, we should make it easy to extend
 *                the set of system properties in port-specific files
 *                without editing VmCommon files.
 *=======================================================================*/

char* getSystemProperty(char *key) {
    char* value = NULL;

    /*
     * Currently, we define properties simply by going
     * through a set of if statements.  If the number of
     * properties becomes any larger, we should really
     * use an internal hashtable for the key/value pairs.
     */

    if (strcmp(key, "microedition.configuration") == 0) {
        /* Important: This value should reflect the */
        /* version of the CLDC Specification supported */
        /* -- not the version number of the implementation */
        value = "CLDC-1.1";
        goto done;
    }

    if (strcmp(key, "microedition.platform") == 0) {
#ifdef PLATFORMNAME
        value = PLATFORMNAME;
#else
        value = "generic";
#endif
        goto done;
    }

    if (strcmp(key, "microedition.encoding") == 0) {
        value = "ISO-8859-1";
        goto done;
    }

    if (strcmp(key, "microedition.profiles") == 0) {
        value = "";
        goto done;
    }

done:
    return value;
}

