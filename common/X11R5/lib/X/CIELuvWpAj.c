/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:CIELuvWpAj.c	1.1"
/* $XConsortium: CIELuvWpAj.c,v 1.4 91/05/13 22:22:56 rws Exp $" */

/*
 * Code and supporting documentation (c) Copyright 1990 1991 Tektronix, Inc.
 * 	All Rights Reserved
 * 
 * This file is a component of an X Window System-specific implementation
 * of XCMS based on the TekColor Color Management System.  Permission is
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
 *		CIELuvWpAj.c
 *
 *	DESCRIPTION
 *		This file contains routine(s) that support white point
 *		adjustment of color specifications in the CIE CIELuv.color
 *		space.
 */

#include "Xlibint.h"
#include "Xcmsint.h"

/*
 *	EXTERNS
 */

extern Status _XcmsConvertColorsWithWhitePt();



/************************************************************************
 *									*
 *			 PUBLIC ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsCIELuvWhiteShiftColors
 *
 *	SYNOPSIS
 */
Status
XcmsCIELuvWhiteShiftColors(ccc, pWhitePtFrom, pWhitePtTo, destSpecFmt,
	pColors_in_out, nColors, pCompressed)
    XcmsCCC ccc;
    XcmsColor *pWhitePtFrom;
    XcmsColor *pWhitePtTo;
    XcmsColorFormat destSpecFmt;
    XcmsColor *pColors_in_out;
    unsigned int nColors;
    Bool *pCompressed;
/*
 *	DESCRIPTION
 *		Adjusts color specifications in an array of XcmsColor
 *		structures for white point differences.
 *
 *	RETURNS
 *		XcmsFailure if failed,
 *		XcmsSuccess if succeeded without gamut compression,
 *		XcmsSuccessWithCompression if succeeded with
 *			gamut compression.
 */
{
    if (pWhitePtFrom == NULL || pWhitePtTo == NULL || pColors_in_out == NULL) {
	return(XcmsFailure);
    }

    /*
     * Convert to CIELuv using pWhitePtFrom
     *    We can ignore return value for compression because we are converting
     *    to XcmsCIELuvFormat which is device-independent, not device-dependent.
     */
    if (_XcmsConvertColorsWithWhitePt(ccc, pColors_in_out, pWhitePtFrom,
	    nColors, XcmsCIELuvFormat, pCompressed) == XcmsFailure) {
	return(XcmsFailure);
    }

    /*
     * Convert from CIELuv to destSpecFmt using pWhitePtTo
     */
    return(_XcmsConvertColorsWithWhitePt(ccc, pColors_in_out, pWhitePtTo,
	    nColors, destSpecFmt, pCompressed));
}
