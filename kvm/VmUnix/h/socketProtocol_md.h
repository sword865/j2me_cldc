/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * KVM
 *=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: networking (Generic Connection framework)
 * FILE:      socketProtocol_md.h (Unix)
 * OVERVIEW:  This file contains the function prototypes
 *            that are needed for supporting the "socket:" and
 *            "serversocket:" Generic Connection protocols.
 */

/*=========================================================================
 * Function Prototypes
 *=======================================================================*/

void prim_com_sun_cldc_io_j2me_socket_Protocol_setNonBlocking(int fd);
int prim_com_sun_cldc_io_j2me_socket_Protocol_getIpNumber(char *host);
int prim_com_sun_cldc_io_j2me_socket_Protocol_open0(char *name, int port, char **exception);
int prim_com_sun_cldc_io_j2me_socket_Protocol_read0(int fd, char *p, int len);
int prim_com_sun_cldc_io_j2me_socket_Protocol_available0(int fd);
int prim_com_sun_cldc_io_j2me_socket_Protocol_write0(int fd, char *p, int len);
int prim_com_sun_cldc_io_j2me_socket_Protocol_close0(int fd);
int prim_com_sun_cldc_io_j2me_serversocket_Protocol_open0(int port, char **exception);
int prim_com_sun_cldc_io_j2me_serversocket_Protocol_accept(int fd);
int prim_com_sun_cldc_io_j2me_serversocket_Protocol_close(int fd);
void networkInit(void);
int netError(void);
