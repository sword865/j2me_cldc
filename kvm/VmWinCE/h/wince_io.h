/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*
 * Deletes filename
 */
void unlink(char *filename);

/*
 * Ensures STDOUT and STDERR are open
 */
void ensurestdoutopen(void);
void ensurestderropen(void);

/*
 * Opens and Closes stdout and stderr for writing
 */
void openstdio(void);
void closestdio(void);
void closestdout(void);
void closestderr(void);

/*
 * Opens and Closes the console window
 */
void openConsole(void);
int  closeConsole(void);

