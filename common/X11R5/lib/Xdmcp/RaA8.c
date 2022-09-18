/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xdmcp:RaA8.c	1.2"
/*
 * $XConsortium: RaA8.c,v 1.2 91/01/23 22:14:22 gildea Exp $
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
XdmcpReallocARRAY8 (array, length)
    ARRAY8Ptr	array;
    int		length;
{
    CARD8Ptr	newData;

    newData = (CARD8Ptr) Xrealloc (array->data, length * sizeof (CARD8));
    if (!newData)
	return FALSE;
    array->length = length;
    array->data = newData;
    return TRUE;
}
