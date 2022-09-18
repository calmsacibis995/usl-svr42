/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xdmcp:CA8.c	1.2"
/*
 * $XConsortium: CA8.c,v 1.3 91/01/23 22:13:10 gildea Exp $
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
#include <X11/Xfuncs.h>

int
XdmcpCopyARRAY8 (src, dst)
    ARRAY8Ptr	src, dst;
{
    dst->length = src->length;
    dst->data = (CARD8 *) Xalloc (dst->length * sizeof (CARD8));
    if (!dst->data)
	return FALSE;
    bcopy (src->data, dst->data, src->length * sizeof (CARD8));
    return TRUE;
}
