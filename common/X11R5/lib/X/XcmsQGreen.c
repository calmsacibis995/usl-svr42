/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XcmsQGreen.c	1.1"
/* $XConsortium: XcmsQGreen.c,v 1.2 91/06/07 09:56:46 rws Exp $ */

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
 *		XcmsQGreen.c - Query Green
 *
 *	DESCRIPTION
 *		Routine to obtain a color specification for full
 *		green intensity and zero red and blue intensities.
 *
 *
 */

#include "Xlibint.h"
#include "Xcms.h"



/************************************************************************
 *									*
 *			PUBLIC INTERFACES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsQueryGreen
 *
 *	SYNOPSIS
 */

Status
XcmsQueryGreen(ccc, target_format, pColor_ret)
    XcmsCCC ccc;
    XcmsColorFormat target_format;
    XcmsColor *pColor_ret;
/*
 *	DESCRIPTION
 *		Returns the color specification in the target format for
 *		full intensity green and zero intensity red and blue.
 *
 *	RETURNS
 *		Returns XcmsSuccess, if failed; otherwise XcmsFailure
 *
 */
{
    XcmsColor tmp;

    tmp.format = XcmsRGBiFormat;
    tmp.pixel = 0;
    tmp.spec.RGBi.red = 0.0;
    tmp.spec.RGBi.green = 1.0;
    tmp.spec.RGBi.blue = 0.0;
    if (XcmsConvertColors(ccc, &tmp, 1, target_format, NULL) != XcmsSuccess) {
	return(XcmsFailure);
    }
    bcopy((char *)&tmp, (char *)pColor_ret, sizeof(XcmsColor));
    return(XcmsSuccess);
}
