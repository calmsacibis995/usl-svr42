/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:CharSet.h	1.2"
/* $XConsortium: CharSet.h,v 1.4 91/07/22 23:45:26 converse Exp $
 *
 * Copyright 1988 by the Massachusetts Institute of Technology
 *
 *
 * The X Window System is a Trademark of MIT.
 *
 * The interfaces described by this header file are for miscellaneous utilities
 * and are not part of the Xlib standard.
 */

#ifndef _XMU_CHARSET_H_
#define _XMU_CHARSET_H_

#include <X11/Xfuncproto.h>

_XFUNCPROTOBEGIN

extern void XmuCopyISOLatin1Lowered(
#if NeedFunctionPrototypes
    char *		/* dst_return */,
    _Xconst char *	/* src */
#endif
);

extern void XmuCopyISOLatin1Uppered(
#if NeedFunctionPrototypes
    char *		/* dst_return */,
    _Xconst char *	/* src */
#endif
);

extern int XmuCompareISOLatin1(
#if NeedFunctionPrototypes
    _Xconst char *	/* first */,
    _Xconst char *	/* second */
#endif
);

_XFUNCPROTOEND

#endif /* _XMU_CHARSET_H_ */
