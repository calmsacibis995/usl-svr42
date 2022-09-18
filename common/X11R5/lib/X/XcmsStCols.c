/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XcmsStCols.c	1.1"
/* $XConsortium: XcmsStCols.c,v 1.7 91/05/13 23:29:57 rws Exp $" */

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
 *		XcmsStCols.c
 *
 *	DESCRIPTION
 *		Source for XcmsStoreColors
 *
 *
 */

#include "Xlibint.h"
#include "Xcmsint.h"


/************************************************************************
 *									*
 *			PUBLIC ROUTINES					*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsStoreColors - Store Colors
 *
 *	SYNOPSIS
 */
Status
XcmsStoreColors(dpy, colormap, pColors_in,
	nColors, pCompressed)
    Display *dpy;
    Colormap colormap;
    XcmsColor *pColors_in;
    unsigned int nColors;
    Bool *pCompressed;
/*
 *	DESCRIPTION
 *		Given device-dependent or device-independent color
 *		specifications, this routine will convert them to X RGB
 *		values then use it in a call to XStoreColors.
 *
 *	RETURNS
 *		XcmsFailure if failed;
 *		XcmsSuccess if it succeeded without gamut compression;
 *		XcmsSuccessWithCompression if it succeeded with gamut
 *			compression;
 *
 *		Since XStoreColors has no return value, this routine
 *		does not return color specifications of the colors actually
 *		stored.
 */
{
    XcmsColor Color1;
    XcmsColor *pColors_tmp;
    Status retval;

    /*
     * Make copy of array of color specifications so we don't
     * overwrite the contents.
     */
    if (nColors > 1) {
	pColors_tmp = (XcmsColor *) Xmalloc(nColors * sizeof(XcmsColor));
    } else {
	pColors_tmp = &Color1;
    }
    bcopy((char *)pColors_in, (char *)pColors_tmp,
	    nColors * sizeof(XcmsColor));

    /*
     * Call routine to store colors using the copied color structures
     */
    retval = _XcmsSetGetColors (XStoreColors, dpy, colormap,
	    pColors_tmp, nColors, XcmsRGBFormat, pCompressed);

    /*
     * Free copies as needed.
     */
    if (nColors > 1) {
	Xfree((char *)pColors_tmp);
    }

    /*
     * Ah, finally return.
     */
    return(retval);
}
