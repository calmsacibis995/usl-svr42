/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xdmcp:Whead.c	1.2"
/*
 * $XConsortium: Whead.c,v 1.3 91/01/23 22:14:45 gildea Exp $
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

int
XdmcpWriteHeader (buffer, header)
    XdmcpBufferPtr  buffer;
    XdmcpHeaderPtr  header;
{
    BYTE    *newData;

    if ((int)buffer->size < 6 + (int)header->length)
    {
	newData = (BYTE *) Xalloc (XDM_MAX_MSGLEN * sizeof (BYTE));
	if (!newData)
	    return FALSE;
	Xfree (buffer->data);
	buffer->data = newData;
	buffer->size = XDM_MAX_MSGLEN;
    }
    buffer->pointer = 0;
    if (!XdmcpWriteCARD16 (buffer, header->version))
	return FALSE;
    if (!XdmcpWriteCARD16 (buffer, header->opcode))
	return FALSE;
    if (!XdmcpWriteCARD16 (buffer, header->length))
	return FALSE;
    return TRUE;
}
