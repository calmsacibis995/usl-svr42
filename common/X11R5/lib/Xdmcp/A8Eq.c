/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xdmcp:A8Eq.c	1.2"
/*
 * $XConsortium: A8Eq.c,v 1.3 91/01/23 22:12:56 gildea Exp $
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
XdmcpARRAY8Equal (array1, array2)
    ARRAY8Ptr	array1, array2;
{
    int	i;

    if (array1->length != array2->length)
	return FALSE;
    for (i = 0; i < (int)array1->length; i++)
	if (array1->data[i] != array2->data[i])
	    return FALSE;
    return TRUE;
}
