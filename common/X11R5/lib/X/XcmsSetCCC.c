/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XcmsSetCCC.c	1.2"

#include "Xlibint.h"
#include "Xcms.h"



/************************************************************************
 *									*
 *			PUBLIC INTERFACES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsSetWhitePoint
 *
 *	SYNOPSIS
 */

Status
XcmsSetWhitePoint(ccc, pColor)
    XcmsCCC ccc;
    XcmsColor *pColor;
/*
 *	DESCRIPTION
 *		Sets the Client White Point in the specified CCC.
 *
 *	RETURNS
 *		Returns XcmsSuccess if succeeded; otherwise XcmsFailure.
 *
 */
{
    if (pColor == NULL || pColor->format == XcmsUndefinedFormat) {
	ccc->clientWhitePt.format = XcmsUndefinedFormat;
    } else if (pColor->format != XcmsCIEXYZFormat &&
	    pColor->format != XcmsCIEuvYFormat &&
	    pColor->format != XcmsCIExyYFormat) {
	return(XcmsFailure);
    } else {
	bcopy((char *)pColor, (char *)&ccc->clientWhitePt, sizeof(XcmsColor));
    }
    return(XcmsSuccess);
}


/*
 *	NAME
 *		XcmsSetCompressionProc
 *
 *	SYNOPSIS
 */

XcmsCompressionProc
#if NeedFunctionPrototypes
XcmsSetCompressionProc(
    XcmsCCC ccc,
    XcmsCompressionProc compression_proc,
    XPointer client_data)
#else
XcmsSetCompressionProc(ccc, compression_proc, client_data)
    XcmsCCC ccc;
    XcmsCompressionProc compression_proc;
    XPointer client_data;
#endif
/*
 *	DESCRIPTION
 *		Set the specified CCC's compression function and client data.
 *
 *	RETURNS
 *		Returns the old compression function.
 *
 */
{
    XcmsCompressionProc old = ccc->gamutCompProc;

    ccc->gamutCompProc = compression_proc;
    ccc->gamutCompClientData = client_data;
    return(old);
}


/*
 *	NAME
 *		XcmsSetWhiteAdjustProc
 *
 *	SYNOPSIS
 */

XcmsWhiteAdjustProc
#if NeedFunctionPrototypes
XcmsSetWhiteAdjustProc(
    XcmsCCC ccc,
    XcmsWhiteAdjustProc white_adjust_proc,
    XPointer client_data )
#else
XcmsSetWhiteAdjustProc(ccc, white_adjust_proc, client_data)
    XcmsCCC ccc;
    XcmsWhiteAdjustProc white_adjust_proc;
    XPointer client_data;
#endif
/*
 *	DESCRIPTION
 *		Set the specified CCC's white_adjust function and client data.
 *
 *	RETURNS
 *		Returns the old white_adjust function.
 *
 */
{
    XcmsWhiteAdjustProc old = ccc->whitePtAdjProc;

    ccc->whitePtAdjProc = white_adjust_proc;
    ccc->whitePtAdjClientData = client_data;
    return(old);
}
