/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:XawInit.c	1.2"
/*
 * $XConsortium: XawInit.c,v 1.2 89/10/09 15:48:42 jim Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 * 
 * 
 * 			    XawInitializeWidgetSet
 * 
 * This routine forces a reference to vendor shell so that the one in this
 * widget is installed.  Any other cross-widget set initialization should be
 * done here as well.  All Athena widgets should include "XawInit.h" and
 * call this routine from their ClassInitialize procs (this routine may be
 * used as the class init proc).
 */

#include <X11/Intrinsic.h>
#include <X11/Vendor.h>
#include <X11/Xaw/XawInit.h>

void XawInitializeWidgetSet ()
{
    static int firsttime = 1;

    if (firsttime) {
	firsttime = 0;
	XtInitializeWidgetClass (vendorShellWidgetClass);
    }
}
