/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xdmcp:RC16.c	1.2"
/*
 * $XConsortium: RC16.c,v 1.2 91/01/23 22:14:01 gildea Exp $
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
XdmcpReadCARD16 (buffer, valuep)
    XdmcpBufferPtr  buffer;
    CARD16Ptr	    valuep;
{
    CARD8   high, low;

    if (XdmcpReadCARD8 (buffer, &high) &&
        XdmcpReadCARD8 (buffer, &low))
    {
	*valuep = (((CARD16) high) << 8) | ((CARD16) low);
	return TRUE;
    }
    return FALSE;
}
