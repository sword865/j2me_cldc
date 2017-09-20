/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/*=========================================================================
 * KVM
 *=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: resource access (Generic Connection framework)
 * FILE:      resource.c
 * OVERVIEW:  This file implements the native functions for
 *            a Generic Connection protocol that is used for
 *            accessing external resources.
 * AUTHOR:    Nik Shaylor
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <global.h>

/*=========================================================================
 * Forward declarations
 *=======================================================================*/

void Java_com_sun_cldc_io_ResourceInputStream_open(void);
void Java_com_sun_cldc_io_ResourceInputStream_close(void);
void Java_com_sun_cldc_io_ResourceInputStream_read(void);

/*=========================================================================
 * Functions
 *=======================================================================*/

 /*=========================================================================
  * FUNCTION:      Object open(String) (STATIC)
  * CLASS:         com.sun.cldc.io.ResourceInputStream
  * TYPE:          virtual native function
  * OVERVIEW:      Open resource stream
  * INTERFACE (operand stack manipulation):
  *   parameters:  this
  *   returns:     the integer value
  *=======================================================================*/

void Java_com_sun_cldc_io_ResourceInputStream_open(void) {
    STRING_INSTANCE string = popStackAsType(STRING_INSTANCE);
    char           *name;
    FILEPOINTER     fp;

    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(STRING_INSTANCE, _string, string);
        long buflen = _string->length + 1;
        DECLARE_TEMPORARY_ROOT(char *, buf,
                               (char *)mallocHeapObject((buflen+CELL-1)>>log2CELL,
                               GCT_NOPOINTERS));

        name = getStringContentsSafely(_string, buf, buflen);
        if (buf == NULL) {
            THROW(OutOfMemoryObject);
        } else {
            fp = openResourcefile(name);
            if (fp == NULL) {
                raiseException(IOException);
            } else {
                pushStackAsType(FILEPOINTER, fp);
            }
        }
    END_TEMPORARY_ROOTS
}

 /*=========================================================================
  * FUNCTION:      void close(Object) static   [Object is fp]
  * CLASS:         com.sun.cldc.io.ResourceInputStream
  * TYPE:          virtual native function
  * OVERVIEW:      Read an integer from the resource
  * INTERFACE (operand stack manipulation):
  *   parameters:  this
  *   returns:     the integer value
  *=======================================================================*/

void Java_com_sun_cldc_io_ResourceInputStream_close(void) {
    START_TEMPORARY_ROOTS
        DECLARE_TEMPORARY_ROOT(FILEPOINTER, fp, popStackAsType(FILEPOINTER));
        if (fp == NULL) {
            raiseException(IOException);
        } else {
            closeClassfile(&fp);
        }
    END_TEMPORARY_ROOTS
    /* Java code will set the handle to NULL */
}

 /*=========================================================================
  * FUNCTION:      read()I (VIRTUAL)
  * CLASS:         com.sun.cldc.io.ResourceInputStream
  * TYPE:          virtual native function
  * OVERVIEW:      Read an integer from the resource
  * INTERFACE (operand stack manipulation):
  *   parameters:  this
  *   returns:     the integer value
  *=======================================================================*/

void Java_com_sun_cldc_io_ResourceInputStream_read(void) {
    START_TEMPORARY_ROOTS
        int result;
        DECLARE_TEMPORARY_ROOT(FILEPOINTER, fp, popStackAsType(FILEPOINTER));
        if (fp == NULL) {
            raiseException(IOException);
        } else {
            result = loadByteNoEOFCheck(&fp);
            pushStack(result);
        }
    END_TEMPORARY_ROOTS
}

 /*=========================================================================
  * FUNCTION:      readAll()I (VIRTUAL)
  * CLASS:         com.sun.cldc.io.ResourceInputStream
  * TYPE:          virtual native function
  * OVERVIEW:      Read an array of bytes from the stream
  * INTERFACE (operand stack manipulation):
  *   parameters:  this, byte array, offset, filepos, len
  *   returns:     the number of bytes read
  *=======================================================================*/

void Java_com_sun_cldc_io_ResourceInputStream_readBytes(void) {
    int result;
    int length = popStack();
    int pos = popStack();
    int offset = popStack();
    BYTEARRAY bytes = popStackAsType(BYTEARRAY);
    FILEPOINTER fp = popStackAsType(FILEPOINTER);

    if (fp == NULL || bytes == NULL) {
        raiseException(IOException);
    } else {
        result = loadBytesNoEOFCheck(&fp, (char *)&bytes->bdata[offset], pos, length);
        pushStack(result);
    }
}

 /*=========================================================================
  * FUNCTION:      size()I (VIRTUAL)
  * CLASS:         com.sun.cldc.io.ResourceInputStream
  * TYPE:          virtual native function
  * OVERVIEW:      return amount of data on this stream
  * INTERFACE (operand stack manipulation):
  *   parameters:  this
  *   returns:     the number of bytes available
  *=======================================================================*/

void Java_com_sun_cldc_io_ResourceInputStream_size(void) {
    int result;
    FILEPOINTER fp = popStackAsType(FILEPOINTER);
    if (fp == NULL) {
        raiseException(IOException);
    } else {
        result = getBytesAvailable(&fp);
        pushStack(result);
    }
}
