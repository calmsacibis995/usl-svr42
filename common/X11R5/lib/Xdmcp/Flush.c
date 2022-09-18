/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xdmcp:Flush.c	1.2"
/*
 * $XConsortium: Flush.c,v 1.4 91/07/16 20:33:52 gildea Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#include <X11/Xos.h>
#include <X11/X.h>
#include <X11/Xmd.h>
#include <X11/Xdmcp.h>

#ifdef STREAMSCONN
#include <tiuser.h>
#else
#include <sys/socket.h>
#endif

int
XdmcpFlush (fd, buffer, to, tolen)
    int		    fd;
    XdmcpBufferPtr  buffer;
    XdmcpNetaddr    to;
    int		    tolen;
{
    int result;

#ifdef STREAMSCONN
    struct t_unitdata dataunit;

    dataunit.addr.buf = to;
    dataunit.addr.len = tolen;
    dataunit.opt.len = 0;	/* default options */
    dataunit.udata.buf = (char *)buffer->data;
    dataunit.udata.len = buffer->pointer;
    result = t_sndudata(fd, &dataunit);
    if (result < 0)
	return FALSE;
#else
    result = sendto (fd, buffer->data, buffer->pointer, 0,
		     (struct sockaddr *)to, tolen);
    if (result != buffer->pointer)
	return FALSE;
#endif
    return TRUE;
}
