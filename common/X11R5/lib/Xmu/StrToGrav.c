/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:StrToGrav.c	1.2"
/*
 * $XConsortium: StrToGrav.c,v 1.4 90/11/30 17:00:50 rws Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 */


#include <X11/Intrinsic.h>
#include <X11/Xmu/Converters.h>

#define done(address, type) \
        { (*toVal).size = sizeof(type); (*toVal).addr = (caddr_t) address; }

static struct _namepair {
    XrmQuark quark;
    char *name;
    XtGravity gravity;
} names[] = {
    { NULLQUARK, XtEForget, ForgetGravity },
    { NULLQUARK, XtENorthWest, NorthWestGravity },
    { NULLQUARK, XtENorth, NorthGravity },
    { NULLQUARK, XtENorthEast, NorthEastGravity },
    { NULLQUARK, XtEWest, WestGravity },
    { NULLQUARK, XtECenter, CenterGravity },
    { NULLQUARK, XtEEast, EastGravity },
    { NULLQUARK, XtESouthWest, SouthWestGravity },
    { NULLQUARK, XtESouth, SouthGravity },
    { NULLQUARK, XtESouthEast, SouthEastGravity },
    { NULLQUARK, XtEStatic, StaticGravity },
    { NULLQUARK, XtEUnmap, UnmapGravity },
    { NULLQUARK, XtEleft, WestGravity },
    { NULLQUARK, XtEtop, NorthGravity },
    { NULLQUARK, XtEright, EastGravity },
    { NULLQUARK, XtEbottom, SouthGravity },
    { NULLQUARK, NULL, ForgetGravity }
};

void XmuCvtStringToGravity (args, num_args, fromVal, toVal)
    XrmValuePtr args;
    Cardinal    *num_args;
    XrmValuePtr fromVal;
    XrmValuePtr toVal;
{
    static Boolean haveQuarks = FALSE;
    char lowerName[40];
    XrmQuark q;
    char *s;
    struct _namepair *np;

    if (*num_args != 0)
        XtWarningMsg("wrongParameters","cvtStringToGravity","XtToolkitError",
                  "String to Gravity conversion needs no extra arguments",
                  (String *) NULL, (Cardinal *)NULL);

    if (!haveQuarks) {
	for (np = names; np->name; np++) {
	    np->quark = XrmPermStringToQuark (np->name);
	}
	haveQuarks = TRUE;
    }

    s = (char *) fromVal->addr;
    if (strlen(s) < sizeof lowerName) {
	XmuCopyISOLatin1Lowered (lowerName, s);
	q = XrmStringToQuark (lowerName);

	for (np = names; np->name; np++) {
	    if (np->quark == q) {
		done (&np->gravity, XtGravity);
		return;
	    }
	}
    }
    XtStringConversionWarning((char *) fromVal->addr, XtRGravity);
}
