/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olmisc:Xt.c	1.1"
#endif

#include "Xt/IntrinsicI.h"
#include "Xol/OpenLook.h"

/*
 * Define the following if you want to be able to turn on debugging
 * information in the binary product.
 */
#define	MODAL_DEBUG
#if	defined(MODAL_DEBUG)
static Boolean		modal_debug = XtUnspecifiedBoolean;
#endif

/**
 ** XtModalCascadeActive()
 **/

Boolean
#if	OlNeedFunctionPrototypes
XtModalCascadeActive (
	Widget			w
)
#else
XtModalCascadeActive (w)
	Widget			w;
#endif
{
	XtGrabList *		grabListPtr = 
		_XtGetGrabList(_XtGetPerDisplayInput(XtDisplay(w)));

#if	defined(MODAL_DEBUG)
	if (modal_debug == XtUnspecifiedBoolean)
		modal_debug = (getenv("MODAL_DEBUG") != 0);
	if (modal_debug) {
		printf (
			"XtModalCascadeActive: %s/%x: %s\n",
			XtName(w), w,
			*grabListPtr? "ACTIVE" : "INACTIVE"
		);
	}
#endif
	return (*grabListPtr != (XtGrabList)0);
} /* XtModalCascadeActive */
