/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XcmsStCol.c	1.3"

#include "Xlibint.h"
#include "Xcmsint.h"


/************************************************************************
 *									*
 *			PUBLIC ROUTINES					*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsStoreColor - Store Color
 *
 *	SYNOPSIS
 */
Status
XcmsStoreColor(dpy, colormap, pColor_in)
    Display *dpy;
    Colormap colormap;
    XcmsColor *pColor_in;
/*
 *	DESCRIPTION
 *		Given a device-dependent or device-independent color
 *		specification, this routine will convert it to X RGB
 *		values then use it in a call to XStoreColor.
 *
 *	RETURNS
 *		XcmsFailure if failed;
 *		XcmsSuccess if it succeeded without gamut compression;
 *		XcmsSuccessWithCompression if it succeeded with gamut
 *			compression;
 *
 *		Since XStoreColor has no return value this routine
 *		does not return the color specification of the color actually
 *		stored.
 */
{
    XcmsColor tmpColor;

    tmpColor = *pColor_in;
    return(_XcmsSetGetColors (XStoreColor, dpy, colormap,
			      &tmpColor, 1, XcmsRGBFormat, (Bool *) NULL));
}
