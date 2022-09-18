/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:StrToBS.c	1.2"
/* $XConsortium: StrToBS.c,v 1.2 90/12/20 13:27:50 converse Exp $ */

/* 
 * Copyright 1988 by the Massachusetts Institute of Technology
 *
 *
 */

#include <X11/Intrinsic.h>
#include "Converters.h"
#include "CharSet.h"

#define	done(address, type) \
	{ (*toVal).size = sizeof(type); (*toVal).addr = (caddr_t) address; }

/* ARGSUSED */
void
XmuCvtStringToBackingStore (args, num_args, fromVal, toVal)
    XrmValue	*args;		/* unused */
    Cardinal	*num_args;	/* unused */
    XrmValuePtr fromVal;
    XrmValuePtr toVal;
{
    char	lowerString[1024];
    XrmQuark	q;
    static int	backingStoreType;
    static XrmQuark XtQEnotUseful, XtQEwhenMapped, XtQEalways, XtQEdefault;
    static int haveQuarks = 0;

    if (*num_args != 0)
        XtWarning("String to BackingStore conversion needs no extra arguments");
    if (!haveQuarks) {
	XmuCopyISOLatin1Lowered (lowerString, XtEnotUseful);
	XtQEnotUseful = XrmStringToQuark(lowerString);
	XmuCopyISOLatin1Lowered (lowerString, XtEwhenMapped);
	XtQEwhenMapped = XrmStringToQuark(lowerString);
	XmuCopyISOLatin1Lowered (lowerString, XtEalways);
	XtQEalways = XrmStringToQuark(lowerString);
	XmuCopyISOLatin1Lowered (lowerString, XtEdefault);
	XtQEdefault = XrmStringToQuark(lowerString);
	haveQuarks = 1;
    }
    XmuCopyISOLatin1Lowered (lowerString, (char *) fromVal->addr);
    q = XrmStringToQuark (lowerString);
    if (q == XtQEnotUseful) {
	backingStoreType = NotUseful;
	done (&backingStoreType, int);
    } else if (q == XtQEwhenMapped) {
    	backingStoreType = WhenMapped;
	done (&backingStoreType, int);
    } else if (q == XtQEalways) {
	backingStoreType = Always;
	done (&backingStoreType, int);
    } else if (q == XtQEdefault) {
    	backingStoreType = Always + WhenMapped + NotUseful;
	done (&backingStoreType, int);
    } else {
        XtStringConversionWarning((char *) fromVal->addr, "BackingStore");
    }
}
