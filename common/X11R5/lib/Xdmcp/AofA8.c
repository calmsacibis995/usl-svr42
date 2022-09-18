/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xdmcp:AofA8.c	1.2"
/*
 * $XConsortium: AofA8.c,v 1.2 91/01/23 22:13:07 gildea Exp $
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
XdmcpAllocARRAYofARRAY8 (array, length)
    ARRAYofARRAY8Ptr	array;
    int			length;
{
    ARRAY8Ptr	newData;

    newData = (ARRAY8Ptr) (length * sizeof (ARRAY8));
    if (!newData)
	return FALSE;
    array->length = length;
    array->data = newData;
    return TRUE;
}
