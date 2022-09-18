/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:Initer.h	1.2"
/* $XConsortium: Initer.h,v 1.4 91/07/22 23:45:59 converse Exp $
 *
 * Copyright 1988 by the Massachusetts Institute of Technology
 *
 *
 * The X Window System is a Trademark of MIT.
 *
 * The interfaces described by this header file are for miscellaneous utilities
 * and are not part of the Xlib standard.
 */

#ifndef _XMU_INITER_H_
#define _XMU_INITER_H_

#include <X11/Xfuncproto.h>

typedef void (*XmuInitializerProc)(
#if NeedFunctionPrototypes
    XtAppContext	/* app_context */,
    caddr_t		/* data */
#endif
);

_XFUNCPROTOBEGIN

extern void XmuCallInitializers(
#if NeedFunctionPrototypes
    XtAppContext	/* app_context */
#endif
);

extern void XmuAddInitializer(
#if NeedFunctionPrototypes
    XmuInitializerProc	/* func */,
     caddr_t	/* data */
#endif
);

_XFUNCPROTOEND

#endif /* _XMU_INITER_H_ */
