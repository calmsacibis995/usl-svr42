/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:StrToOrnt.c	1.2"
/* $XConsortium: StrToOrnt.c,v 1.6 90/12/20 13:33:20 converse Exp $ */

/* 
 * Copyright 1988 by the Massachusetts Institute of Technology
 *
 *
 */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include "Converters.h"
#include "CharSet.h"


#define	done(address, type) \
	{ (*toVal).size = sizeof(type); (*toVal).addr = (caddr_t) address; }

/* ARGSUSED */
void
XmuCvtStringToOrientation(args, num_args, fromVal, toVal)
    XrmValuePtr args;		/* unused */
    Cardinal	*num_args;	/* unused */
    XrmValuePtr	fromVal;
    XrmValuePtr	toVal;
{
    static XtOrientation orient;
    static	XrmQuark  XtQEhorizontal;
    static	XrmQuark  XtQEvertical;
    static	int	  haveQuarks = 0;
    XrmQuark	q;
    char	lowerName[1000];

    if (!haveQuarks) {
	XtQEhorizontal = XrmPermStringToQuark(XtEhorizontal);
	XtQEvertical   = XrmPermStringToQuark(XtEvertical);
	haveQuarks = 1;
    }
    XmuCopyISOLatin1Lowered(lowerName, (char *) fromVal->addr);
    q = XrmStringToQuark(lowerName);
    if (q == XtQEhorizontal) {
    	orient = XtorientHorizontal;
	done(&orient, XtOrientation);
	return;
    }
    if (q == XtQEvertical) {
    	orient = XtorientVertical;
	done(&orient, XtOrientation);
	return;
    }
}
