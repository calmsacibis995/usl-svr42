/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:StrToLong.c	1.2"
/*
 * $XConsortium: StrToLong.c,v 1.3 90/10/26 16:43:40 dave Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 */


#include <X11/Intrinsic.h>
#include "Converters.h"

#define done(address, type) \
        { (*toVal).size = sizeof(type); (*toVal).addr = (caddr_t) address; }

void XmuCvtStringToLong (args, num_args, fromVal, toVal)
    XrmValuePtr args;
    Cardinal    *num_args;
    XrmValuePtr fromVal;
    XrmValuePtr toVal;
{
    static long l;

    if (*num_args != 0)
        XtWarningMsg("wrongParameters","cvtStringToLong","XtToolkitError",
                  "String to Long conversion needs no extra arguments",
                  (String *) NULL, (Cardinal *)NULL);
    if (sscanf((char *)fromVal->addr, "%ld", &l) == 1) {
        done(&l, long);
    } else {
        XtStringConversionWarning((char *) fromVal->addr, XtRLong);
    }
}
