/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:StrToShap.c	1.3"
/* $XConsortium: StrToShap.c,v 1.4 91/09/18 14:23:52 converse Exp $ */

/* 
 * Copyright 1988 by the Massachusetts Institute of Technology
 *
 *
 */

#include <X11/Intrinsic.h>
#include "Converters.h"
#include "CharSet.h"

/* ARGSUSED */
#define	done(type, value) \
	{							\
	    if (toVal->addr != NULL) {				\
		if (toVal->size < sizeof(type)) {		\
		    toVal->size = sizeof(type);			\
		    return False;				\
		}						\
		*(type*)(toVal->addr) = (value);		\
	    }							\
	    else {						\
		static type static_val;				\
		static_val = (value);				\
		toVal->addr = (XtPointer)&static_val;		\
	    }							\
	    toVal->size = sizeof(type);				\
	    return True;					\
	}


Boolean XmuCvtStringToShapeStyle(dpy, args, num_args, from, toVal, data)
    Display *dpy;
    XrmValue *args;		/* unused */
    Cardinal *num_args;		/* unused */
    XrmValue *from;
    XrmValue *toVal;
    XtPointer *data;		/* unused */
{
    if (   XmuCompareISOLatin1((char*)from->addr, XtERectangle) == 0
	|| XmuCompareISOLatin1((char*)from->addr, "ShapeRectangle") == 0)
	done( int, XmuShapeRectangle );
    if (   XmuCompareISOLatin1((char*)from->addr, XtEOval) == 0
	|| XmuCompareISOLatin1((char*)from->addr, "ShapeOval") == 0)
	done( int, XmuShapeOval );
    if (   XmuCompareISOLatin1((char*)from->addr, XtEEllipse) == 0
	|| XmuCompareISOLatin1((char*)from->addr, "ShapeEllipse") == 0)
	done( int, XmuShapeEllipse );
    if (   XmuCompareISOLatin1((char*)from->addr, XtERoundedRectangle) == 0
	|| XmuCompareISOLatin1((char*)from->addr, "ShapeRoundedRectangle") == 0)
	done( int, XmuShapeRoundedRectangle );
    {
	int style = 0;
	char ch, *p = (char*)from->addr;
	while (ch = *p++) {
	    if (ch >= '0' && ch <= '9') {
		style *= 10;
		style += ch - '0';
	    }
	    else break;
	}
	if (ch == '\0' && style <= XmuShapeRoundedRectangle)
	    done( int, style );
    }
    XtDisplayStringConversionWarning( dpy, (char*)from->addr, XtRShapeStyle );
    return False;
}
