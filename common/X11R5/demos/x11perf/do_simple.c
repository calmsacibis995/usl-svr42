/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5x11perf:do_simple.c	1.1"
/*****************************************************************************
Copyright 1988, 1989 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved



******************************************************************************/

#ifndef VMS
#include <X11/Xatom.h>
#else
#include <decw$include/Xatom.h>
#endif

#include "x11perf.h"

static Atom XA_PK_TEMP;
static Window root;

void DoNoOp(xp, p, reps)
    XParms  xp;
    Parms   p;
    int     reps;
{
    int     i;

    for (i = 0; i != reps; i++) {
	XNoOp(xp->d);
    }
}


void DoGetAtom(xp, p, reps)
    XParms  xp;
    Parms   p;
    int     reps;
{
    char    *atom;
    int     i;

    for (i = 0; i != reps; i++) {
	atom = XGetAtomName (xp->d, 1);
    }
}

int InitGetProperty(xp, p, reps)
    XParms  xp;
    Parms   p;
    int     reps;
{
    int foo = 41;

    root = RootWindow (xp->d, 0);
    XA_PK_TEMP = XInternAtom (xp->d, "_PK_TEMP", False);
    XChangeProperty (
	    xp->d, root, XA_PK_TEMP, XA_INTEGER, 32,
	    PropModeReplace, (unsigned char *)&foo, sizeof (int));
    return reps;
}

void DoGetProperty(xp, p, reps)
    XParms  xp;
    Parms   p;
    int     reps;
{
    char   *atom;
    int     i, status;
    int     actual_format;
    unsigned long actual_length, bytes_remaining;
    unsigned char *prop;
    
    Atom actual_type;

    for (i = 0; i != reps; i++) {
	status = XGetWindowProperty (
		xp->d, root, XA_PK_TEMP, 0, sizeof (int),
		False, AnyPropertyType, &actual_type, &actual_format,
		&actual_length, &bytes_remaining, &prop);
    }
}
