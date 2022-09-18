/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XcmsCvColW.c	1.1"
/* $XConsortium: XcmsCvColW.c,v 1.4 91/05/13 23:19:33 rws Exp $" */

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
 *
 *	NAME
 *		XcmsCvColW.c
 *
 *	DESCRIPTION
 *		<overall description of what the package does>
 *
 *
 */

#include "Xlibint.h"
#include "Xcmsint.h"


/************************************************************************
 *									*
 *			 API PRIVATE ROUTINES				*
 *									*
 ************************************************************************/


/*
 *	NAME
 *		_XcmsConvertColorsWithWhitePt - Convert XcmsColor structures
 *
 *	SYNOPSIS
 */
Status
_XcmsConvertColorsWithWhitePt(ccc, pColors_in_out, pWhitePt, nColors,
	newFormat, pCompressed)
    XcmsCCC ccc;
    XcmsColor *pColors_in_out;
    XcmsColor *pWhitePt;
    unsigned int nColors;
    XcmsColorFormat newFormat;
    Bool *pCompressed;
/*
 *	DESCRIPTION
 *		Convert XcmsColor structures between device-independent
 *		and/or device-dependent formats but allowing the calling
 *		routine to specify the white point to be associated
 *		with the color specifications (overriding
 *		ccc->clientWhitePt).
 *
 *		This routine has been provided for use in white point
 *		adjustment routines.
 *
 *	RETURNS
 *		XcmsFailure if failed,
 *		XcmsSuccess if succeeded without gamut compression,
 *		XcmsSuccessWithCompression if succeeded with gamut
 *			compression.
 *
 */
{
    if (ccc == NULL || pColors_in_out == NULL ||
	    pColors_in_out->format == XcmsUndefinedFormat) {
	return(XcmsFailure);
    }

    if (nColors == 0 || pColors_in_out->format == newFormat) {
	/* do nothing */
	return(XcmsSuccess);
    }

    if (XCMS_DI_ID(pColors_in_out->format) && XCMS_DI_ID(newFormat)) {
	/*
	 * Device-Independent to Device-Independent Conversion
	 */
	return(_XcmsDIConvertColors(ccc, pColors_in_out, pWhitePt, nColors,
		newFormat));
    }
    if (XCMS_DD_ID(pColors_in_out->format) && XCMS_DD_ID(newFormat)) {
	/*
	 * Device-Dependent to Device-Dependent Conversion
	 */
	return(_XcmsDDConvertColors(ccc, pColors_in_out, nColors, newFormat,
		pCompressed));
    }

    /*
     * Otherwise we have:
     *    1. Device-Independent to Device-Dependent Conversion
     *		OR
     *    2. Device-Dependent to Device-Independent Conversion
     */

    if (XCMS_DI_ID(pColors_in_out->format)) {
	/*
	 *    1. Device-Independent to Device-Dependent Conversion
	 */
	/* First convert to CIEXYZ */
	if (_XcmsDIConvertColors(ccc, pColors_in_out, pWhitePt, nColors,
		XcmsCIEXYZFormat) == XcmsFailure) {
	    return(XcmsFailure);
	}
	/* Then convert to DD Format */
	return(_XcmsDDConvertColors(ccc, pColors_in_out, nColors, newFormat,
		pCompressed));
    } else {
	/*
	 *    2. Device-Dependent to Device-Independent Conversion
	 */
	/* First convert to CIEXYZ */
	if (_XcmsDDConvertColors(ccc, pColors_in_out, nColors,
		XcmsCIEXYZFormat, pCompressed) == XcmsFailure) {
	    return(XcmsFailure);
	}
	/* Then convert to DI Format */
	return(_XcmsDIConvertColors(ccc, pColors_in_out, pWhitePt, nColors,
		newFormat));
    }
}
