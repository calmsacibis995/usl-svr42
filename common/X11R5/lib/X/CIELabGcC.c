/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:CIELabGcC.c	1.1"
/* $XConsortium: CIELabGcC.c,v 1.1 91/07/24 23:26:14 rws Exp $ */

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
 *		CIELabGcC.c
 *
 *	DESCRIPTION
 *		Source for XcmsCIELabClipuv() gamut compression routine.
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
 *		XcmsCIELabClipab - Reduce the chroma for a hue and L*
 *
 *	SYNOPSIS
 */
/* ARGSUSED */
Status
XcmsCIELabClipab (ccc, pColors_in_out, nColors, i, pCompressed)
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
		1, XcmsCIELabFormat);
	_XcmsDIConvertColors(ccc, pColor, ScreenWhitePointOfCCC(ccc),
		1, XcmsCIEXYZFormat);
	if (pCompressed) {
	    *(pCompressed + i) = True;
	}
	return(XcmsSuccess);
    } else {
	if (pColor->format != XcmsCIELabFormat) {
	    if (_XcmsDIConvertColors(ccc, pColor,
		    &ccc->pPerScrnInfo->screenWhitePt, 1, XcmsCIELabFormat)
		    == XcmsFailure) {
		return(XcmsFailure);
	    }
	}
	if (XcmsCIELabQueryMaxC(ccc,
		degrees(XCMS_CIELAB_PMETRIC_HUE(pColor->spec.CIELab.a_star, 
						pColor->spec.CIELab.b_star)),
		pColor->spec.CIELab.L_star,
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
