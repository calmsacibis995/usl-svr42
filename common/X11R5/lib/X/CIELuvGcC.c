/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:CIELuvGcC.c	1.1"
/* $XConsortium: CIELuvGcC.c,v 1.1 91/07/24 23:26:39 rws Exp $ */

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
 *	NAME
 *		CIELuvGcC.c
 *
 *	DESCRIPTION
 *		Source for XcmsCIELuvClipuv() gamut compression routine.
 *
 */

#include "Xlibint.h"
#include "Xcmsint.h"


/************************************************************************
 *									*
 *			 PUBLIC ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsCIELuvClipuv - Reduce the chroma for a hue and L*
 *
 *	SYNOPSIS
 */
/* ARGSUSED */
Status
XcmsCIELuvClipuv (ccc, pColors_in_out, nColors, i, pCompressed)
    XcmsCCC ccc;
    XcmsColor *pColors_in_out;
    unsigned int nColors;
    unsigned int i;
    Bool *pCompressed;
/*
 *	DESCRIPTION
 *		Reduce the Chroma for a specific hue and chroma to
 *		to bring the given color into the gamut of the 
 *		specified device.  As required of gamut compression
 *		functions, this routine returns pColor_in_out
 *		in XcmsCIEXYZFormat on successful completion.
 *		
 *		Since this routine works with the L* within
 *		pColor_in_out intermediate results may be returned
 *		even though it may be invalid.
 *
 *	RETURNS
 *		XcmsFailure - Failure
 *              XcmsSuccess - Succeeded
 *
 */
{
    Status retval;
    XcmsColor *pColor;

    /*
     * Color specification passed as input can be assumed to:
     *	1. Be in XcmsCIEXYZFormat
     *	2. Already be white point adjusted for the Screen White Point.
     *	    This means that the white point now associated with this
     *	    color spec is the Screen White Point (even if the
     *	    ccc->clientWhitePt differs).
     */

    pColor = pColors_in_out + i;

    if (ccc->visual->class < PseudoColor) {
	/*
	 * GRAY !
	 */
	_XcmsDIConvertColors(ccc, pColor, ScreenWhitePointOfCCC(ccc),
		1, XcmsCIELuvFormat);
	_XcmsDIConvertColors(ccc, pColor, ScreenWhitePointOfCCC(ccc),
		1, XcmsCIEXYZFormat);
	if (pCompressed) {
	    *(pCompressed + i) = True;
	}
	return(XcmsSuccess);
    } else {
	if (pColor->format != XcmsCIELuvFormat) {
	    if (_XcmsDIConvertColors(ccc, pColor,
		    &ccc->pPerScrnInfo->screenWhitePt, 1, XcmsCIELuvFormat)
		    == XcmsFailure) {
		return(XcmsFailure);
	    }
	}
	if (XcmsCIELuvQueryMaxC(ccc,
		degrees(XCMS_CIELUV_PMETRIC_HUE(pColor->spec.CIELuv.u_star, 
						pColor->spec.CIELuv.v_star)),
		pColor->spec.CIELuv.L_star,
		pColor) == XcmsFailure) {
	    return(XcmsFailure);
	}
	retval = _XcmsDIConvertColors(ccc, pColor,
		&ccc->pPerScrnInfo->screenWhitePt, 1, XcmsCIEXYZFormat);
	if (retval != XcmsFailure && pCompressed != NULL) {
	    *(pCompressed + i) = True;
	}
	return(retval);
    }
}
