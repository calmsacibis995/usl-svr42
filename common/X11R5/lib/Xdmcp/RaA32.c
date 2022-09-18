/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xdmcp:RaA32.c	1.2"
/*
 * $XConsortium: RaA32.c,v 1.2 91/01/23 22:14:19 gildea Exp $
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
XdmcpReallocARRAY32 (array, length)
    ARRAY32Ptr	array;
    int		length;
{
    CARD32Ptr	newData;

    newData = (CARD32Ptr) Xrealloc (array->data, length * sizeof (CARD32));
    if (!newData)
	return FALSE;
    array->length = length;
    array->data = newData;
    return TRUE;
}
