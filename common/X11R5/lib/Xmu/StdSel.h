/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:StdSel.h	1.2"
/* $XConsortium: StdSel.h,v 1.3 91/07/22 23:46:07 converse Exp $
 *
 * Copyright 1988 by the Massachusetts Institute of Technology
 *
 *
 * The X Window System is a Trademark of MIT.
 *
 * The interfaces described by this header file are for miscellaneous utilities
 * and are not part of the Xlib standard.
 */

#ifndef _XMU_SELECTION_H_
#define _XMU_SELECTION_H_

#include <X11/Xfuncproto.h>

_XFUNCPROTOBEGIN

extern Boolean XmuConvertStandardSelection(
#if NeedFunctionPrototypes
    Widget		/* w */,
    Time		/* time */,
    Atom*		/* selection */,
    Atom*		/* target */,
    Atom*		/* type_return */,
    caddr_t *		/* value_return */,
    unsigned long *	/* length_return */,
    int *		/* format_return */
#endif
);

_XFUNCPROTOEND

#endif /* _XMU_SELECTION_H_ */


