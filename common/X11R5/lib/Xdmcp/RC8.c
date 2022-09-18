/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xdmcp:RC8.c	1.2"
/*
 * $XConsortium: RC8.c,v 1.3 91/07/16 20:30:04 gildea Exp $
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
XdmcpReadCARD8 (buffer, valuep)
    XdmcpBufferPtr  buffer;
    CARD8Ptr	    valuep;
{
    if (buffer->pointer >= buffer->count)
	return FALSE;
    *valuep = (CARD8) buffer->data[buffer->pointer++];
    return TRUE;
}
