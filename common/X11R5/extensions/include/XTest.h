/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:include/XTest.h	1.1"

#ifndef _XTEST_H_
#define _XTEST_H_

#include <X11/Xfuncproto.h>

_XFUNCPROTOBEGIN

Bool XTestQueryExtension(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int*		/* event_basep */,
    int*		/* error_basep */,
    int*		/* majorp */,
    int*		/* minorp */
#endif
);

Bool XTestCompareCursorWithWindow(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    Window		/* window */,
    Cursor		/* cursor */
#endif
);

Bool XTestCompareCurrentCursorWithWindow(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    Window		/* window */
#endif
);

extern XTestFakeKeyEvent(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    unsigned int	/* keycode */,
    Bool		/* is_press */,
    unsigned long	/* delay */
#endif
);

extern XTestFakeButtonEvent(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    unsigned int	/* button */,
    Bool		/* is_press */,
    unsigned long	/* delay */
#endif
);

extern XTestFakeMotionEvent(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int			/* screen */,
    int			/* x */,
    int			/* y */,
    unsigned long	/* delay */
#endif
);

void XTestSetGContextOfGC(
#if NeedFunctionPrototypes
    GC			/* gc */,
    GContext		/* gid */
#endif
);

void XTestSetVisualIDOfVisual(
#if NeedFunctionPrototypes
    Visual*		/* visual */,
    VisualID		/* visualid */
#endif
);

Status XTestDiscard(
#if NeedFunctionPrototypes
    Display*		/* dpy */
#endif
);

_XFUNCPROTOEND

#endif
