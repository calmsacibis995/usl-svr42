/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xdmcp:RC32.c	1.2"
/*
 * $XConsortium: RC32.c,v 1.2 91/01/23 22:14:04 gildea Exp $
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
XdmcpReadCARD32 (buffer, valuep)
    XdmcpBufferPtr  buffer;
    CARD32Ptr	    valuep;
{
    CARD8   byte0, byte1, byte2, byte3;
    if (XdmcpReadCARD8 (buffer, &byte0) &&
        XdmcpReadCARD8 (buffer, &byte1) &&
	XdmcpReadCARD8 (buffer, &byte2) &&
	XdmcpReadCARD8 (buffer, &byte3))
    {
	*valuep = (((CARD32) byte0) << 24) |
		  (((CARD32) byte1) << 16) |
		  (((CARD32) byte2) << 8) |
		  (((CARD32) byte3));
	return TRUE;
    }
    return FALSE;
}
