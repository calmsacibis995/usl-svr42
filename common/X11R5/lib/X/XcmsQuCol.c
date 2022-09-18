/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XcmsQuCol.c	1.1"
/* $XConsortium: XcmsQuCol.c,v 1.4 91/05/13 23:27:56 rws Exp $" */

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
 *		XcmsQuCol.c
 *
 *	DESCRIPTION
 *		Source for XcmsQueryColors
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
 *		XcmsQueryColor - Query Color
 *
 *	SYNOPSIS
 */
Status
XcmsQueryColor(dpy, colormap, pXcmsColor_in_out, result_format)
    Display *dpy;
    Colormap colormap;
    XcmsColor *pXcmsColor_in_out;
    XcmsColorFormat result_format;
/*
 *	DESCRIPTION
 *		This routine uses XQueryColor to obtain the X RGB values
 *		stored in the specified colormap for the specified pixel.
 *		The X RGB values are then converted to the target format as
 *		specified by the format component of the XcmsColor structure.
 *
 *	RETURNS
 *		XcmsFailure if failed;
 *		XcmsSuccess if it succeeded.
 *
 *		Returns a color specification of the color stored in the
 *		specified pixel.
 */
{
    return(_XcmsSetGetColors (XQueryColor, dpy, colormap,
	    pXcmsColor_in_out, 1, result_format, (Bool *) NULL));
}
