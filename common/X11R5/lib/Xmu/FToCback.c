/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:FToCback.c	1.2"
/* static char rcsid[] =
	"$XConsortium: FToCback.c,v 1.2 90/07/15 16:18:34 rws Exp $"; */

/* 
 * Copyright 1988 by the Massachusetts Institute of Technology
 *
 *
 */

#include	<X11/Intrinsic.h>

/* ARGSUSED */
void
XmuCvtFunctionToCallback(args, num_args, fromVal, toVal)
    XrmValue	*args;		/* unused */
    Cardinal	*num_args;	/* unused */
    XrmValuePtr fromVal;
    XrmValuePtr toVal;
{
    static XtCallbackRec callback[2]; /* K&R: initialized to NULL */
    static XtCallbackList callback_list = callback;

    callback[0].callback = *(XtCallbackProc *)fromVal->addr;

    toVal->size = sizeof(XtCallbackList);
    toVal->addr = (caddr_t)&callback_list;
}
