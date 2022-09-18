/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XcmsAllCol.c	1.1"
/* $XConsortium: XcmsAllCol.c,v 1.5 91/05/13 23:03:17 rws Exp $" */

/*
 * Code and supporting documentation (c) Copyright 1990 1991 Tektronix, Inc.
 * 	All Rights Reserved
 * 
 * This file is a component of an X Window System-specific implementation
 * of Xcms based on the TekColor Color Management System.  Permission is
 * hereby granted to use, copy, modify, sell, and otherwise distribute this
 * that this copyright, permission, and disclaimer notice is reproduced in
 * is a trademark of Tektronix, Inc.
 * 
 * for any purpose.  It is provided "as is" and with all faults.
 * 
 * TEKTRONIX DISCLAIMS ALL WARRANTIES APPLICABLE TO THIS SOFTWARE,
 * PARTICULAR PURPOSE.  IN NO EVENT SHALL TEKTRONIX BE LIABLE FOR ANY
 * RESULTING FROM LOSS OF USE, DATA, OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 *
 *
 *	NAME
 *		XcmsAllCol.c
 *
 *	DESCRIPTION
 *		Source for XcmsAllocColor
 *
 *
 */

#include "Xlibint.h"
#include "Xcmsint.h"


/*
 *	NAME
 *		XcmsAllocColor - Allocate Color
 *
 *	SYNOPSIS
 */
Status
XcmsAllocColor(dpy, colormap, pXcmsColor_in_out, result_format)
    Display *dpy;
    Colormap colormap;
    XcmsColor *pXcmsColor_in_out;
    XcmsColorFormat result_format;
/*
 *	DESCRIPTION
 *		Given a device-dependent or device-independent color
 *		specification, XcmsAllocColor will convert it to X RGB
 *		values then use it in a call to XAllocColor.
 *
 *	RETURNS
 *		XcmsFailure if failed;
 *		XcmsSuccess if it succeeded without gamut compression;
 *		XcmsSuccessWithCompression if it succeeded with gamut
 *			compression;
 *
 *		Also returns the pixel value of the color cell and a color
 *		specification of the color actually stored.
 *
 */
{
    return(_XcmsSetGetColors (XAllocColor, dpy, colormap, pXcmsColor_in_out, 1,
	    result_format, (Bool *)NULL));
}
