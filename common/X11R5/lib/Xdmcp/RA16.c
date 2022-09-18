/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xdmcp:RA16.c	1.2"
/*
 * $XConsortium: RA16.c,v 1.3 91/01/23 22:13:48 gildea Exp $
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
XdmcpReadARRAY16 (buffer, array)
    XdmcpBufferPtr  buffer;
    ARRAY16Ptr	    array;
{
    int	    i;

    if (!XdmcpReadCARD8 (buffer, &array->length))
	return FALSE;
    if (!array->length)
    {
	array->data = 0;
	return TRUE;
    }
    array->data = (CARD16 *) Xalloc (array->length * sizeof (CARD16));
    if (!array->data)
	return FALSE;
    for (i = 0; i < (int)array->length; i++)
    {
	if (!XdmcpReadCARD16 (buffer, &array->data[i]))
	{
	    Xfree (array->data);
	    return FALSE;
	}
    }
    return TRUE;
}
