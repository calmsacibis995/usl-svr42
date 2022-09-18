/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xdmcp:Fill.c	1.2"
/*
 * $XConsortium: Fill.c,v 1.4 91/07/16 20:33:50 gildea Exp $
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
XdmcpFill (fd, buffer, from, fromlen)
    int		    fd;
    XdmcpBufferPtr  buffer;
    XdmcpNetaddr    from;	/* return */
    int		    *fromlen;	/* return */
{
    BYTE    *newBuf;
#ifdef STREAMSCONN
    struct t_unitdata dataunit;
    int gotallflag, result;
#endif

    if (buffer->size < XDM_MAX_MSGLEN)
    {
	newBuf = (BYTE *) Xalloc (XDM_MAX_MSGLEN);
	if (newBuf)
	{
	    Xfree (buffer->data);
	    buffer->data = newBuf;
	    buffer->size = XDM_MAX_MSGLEN;
	}
    }
    buffer->pointer = 0;
#ifdef STREAMSCONN
    dataunit.addr.buf = from;
    dataunit.addr.maxlen = *fromlen;
    dataunit.opt.maxlen = 0;	/* don't care to know about options */
    dataunit.udata.buf = (char *)buffer->data;
    dataunit.udata.maxlen = buffer->size;
    result = t_rcvudata (fd, &dataunit, &gotallflag);
    if (result < 0) {
	return FALSE;
    }
    buffer->count = dataunit.udata.len;
    *fromlen = dataunit.addr.len;
#else
    buffer->count = recvfrom (fd, buffer->data, buffer->size, 0,
			      (struct sockaddr *)from, fromlen);
#endif
    if (buffer->count < 6) {
	buffer->count = 0;
	return FALSE;
    }
    return TRUE;
}
