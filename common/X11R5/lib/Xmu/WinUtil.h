/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:WinUtil.h	1.2"
/* $XConsortium: WinUtil.h,v 1.6 91/07/22 23:46:21 converse Exp $
 *
 * Copyright 1988 by the Massachusetts Institute of Technology
 *
 *
 * The X Window System is a Trademark of MIT.
 *
 * The interfaces described by this header file are for miscellaneous utilities
 * and are not part of the Xlib standard.
 */

#ifndef _XMU_WINDOWUTIL_H_
#define _XMU_WINDOWUTIL_H_

#include <X11/Xutil.h>
#include <X11/Xfuncproto.h>

_XFUNCPROTOBEGIN

extern Window XmuClientWindow(
#if NeedFunctionPrototypes
    Display*	/* dpy */,
    Window 	/* win */
#endif
);

extern Bool XmuUpdateMapHints(
#if NeedFunctionPrototypes
    Display*	/* dpy */,
    Window	/* win */,
    XSizeHints*	/* hints */
#endif
);

extern Screen *XmuScreenOfWindow(
#if NeedFunctionPrototypes
    Display*	/* dpy */,
    Window 	/* w */
#endif
);

_XFUNCPROTOEND

#endif /* _XMU_WINDOWUTIL_H_ */
