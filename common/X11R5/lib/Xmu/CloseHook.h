/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:CloseHook.h	1.2"
/* $XConsortium: CloseHook.h,v 1.5 91/07/22 23:45:30 converse Exp $
 *
 * Copyright 1988 by the Massachusetts Institute of Technology
 *
 *
 * The X Window System is a Trademark of MIT.
 *
 * The interfaces described by this header file are for miscellaneous utilities
 * and are not part of the Xlib standard.
 */

#ifndef _XMU_CLOSEHOOK_H_
#define _XMU_CLOSEHOOK_H_

#include <X11/Xfuncproto.h>

typedef caddr_t CloseHook;

typedef int (*XmuCloseHookProc)(
#if NeedFunctionPrototypes
    Display*	/* dpy */,
    caddr_t	/* data */
#endif 
);

_XFUNCPROTOBEGIN

extern CloseHook XmuAddCloseDisplayHook(
#if NeedFunctionPrototypes
    Display *	/* dpy */,
    XmuCloseHookProc	/* proc */,
    caddr_t	/* arg */
#endif
);

extern Bool XmuLookupCloseDisplayHook(
#if NeedFunctionPrototypes
    Display *	/* dpy */,
    CloseHook	/* handle */,
    XmuCloseHookProc	/* proc */,
    caddr_t	/* arg  */
#endif
);

extern Bool XmuRemoveCloseDisplayHook(
#if NeedFunctionPrototypes
    Display *	/* dpy */,
    CloseHook	/* handle */,
    XmuCloseHookProc	/* proc */,
    caddr_t	/* arg */
#endif
);

_XFUNCPROTOEND

#endif /* _XMU_CLOSEHOOK_H_ */
