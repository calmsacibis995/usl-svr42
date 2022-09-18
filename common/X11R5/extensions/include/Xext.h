/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:include/Xext.h	1.1"
/*
 * $XConsortium: Xext.h,v 1.2 91/07/12 10:28:17 rws Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 */

#ifndef _XEXT_H_
#define _XEXT_H_

#include <X11/Xfuncproto.h>

_XFUNCPROTOBEGIN

extern int (*XSetExtensionErrorHandler())(
#if NeedFunctionPrototypes
    int (*handler)()
#endif
);

extern int XMissingExtension(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    _Xconst char*	/* ext_name */
#endif
);

_XFUNCPROTOEND

#define X_EXTENSION_UNKNOWN "unknown"
#define X_EXTENSION_MISSING "missing"

#endif /* _XEXT_H_ */
