/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:PropAlloc.c	1.1"
/* $XConsortium: PropAlloc.c,v 1.5 91/02/01 16:33:26 gildea Exp $ */
/* Copyright 1989 Massachusetts Institute of Technology */

/*
*/

#include "Xlibint.h"
#include "Xutil.h"
#include <stdio.h>


/*
 * Routines for allocating space for structures that are expected to get 
 * longer at some point.
 */

XSizeHints *XAllocSizeHints ()
{
    return ((XSizeHints *) Xcalloc (1, (unsigned) sizeof (XSizeHints)));
}


XStandardColormap *XAllocStandardColormap ()
{
    return ((XStandardColormap *)
	    Xcalloc (1, (unsigned) sizeof (XStandardColormap)));
}


XWMHints *XAllocWMHints ()
{
    return ((XWMHints *) Xcalloc (1, (unsigned) sizeof (XWMHints)));
}


XClassHint *XAllocClassHint ()
{
    register XClassHint *h;

    if (h = (XClassHint *) Xcalloc (1, (unsigned) sizeof (XClassHint))) 
      h->res_name = h->res_class = NULL;

    return h;
}


XIconSize *XAllocIconSize ()
{
    return ((XIconSize *) Xcalloc (1, (unsigned) sizeof (XIconSize)));
}


