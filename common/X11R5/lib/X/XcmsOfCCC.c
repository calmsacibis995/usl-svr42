/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XcmsOfCCC.c	1.1"
/* $XConsortium: XcmsOfCCC.c,v 1.1 91/05/13 22:37:17 rws Exp $ */

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
 *		XcmsOfCCC.c - Color Conversion Context Querying Routines
 *
 *	DESCRIPTION
 *		Routines to query components of a Color Conversion
 *		Context structure.
 *
 *
 */

#include "Xlib.h"
#include "Xcms.h"



/************************************************************************
 *									*
 *			PUBLIC INTERFACES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		XcmsDisplayOfCCC
 *
 *	SYNOPSIS
 */

Display *
XcmsDisplayOfCCC(ccc)
    XcmsCCC ccc;
/*
 *	DESCRIPTION
 *		Queries the Display of the specified CCC.
 *
 *	RETURNS
 *		Pointer to the Display.
 *
 */
{
    return(ccc->dpy);
}


/*
 *	NAME
 *		XcmsVisualOfCCC
 *
 *	SYNOPSIS
 */

Visual *
XcmsVisualOfCCC(ccc)
    XcmsCCC ccc;
/*
 *	DESCRIPTION
 *		Queries the Visual of the specified CCC.
 *
 *	RETURNS
 *		Pointer to the Visual.
 *
 */
{
    return(ccc->visual);
}


/*
 *	NAME
 *		XcmsScreenNumberOfCCC
 *
 *	SYNOPSIS
 */

int
XcmsScreenNumberOfCCC(ccc)
    XcmsCCC ccc;
/*
 *	DESCRIPTION
 *		Queries the screen number of the specified CCC.
 *
 *	RETURNS
 *		screen number.
 *
 */
{
    return(ccc->screenNumber);
}


/*
 *	NAME
 *		XcmsScreenWhitePointOfCCC
 *
 *	SYNOPSIS
 */

XcmsColor *
XcmsScreenWhitePointOfCCC(ccc)
    XcmsCCC ccc;
/*
 *	DESCRIPTION
 *		Queries the screen white point of the specified CCC.
 *
 *	RETURNS
 *		Pointer to the XcmsColor containing the screen white point.
 *
 */
{
    return(&ccc->pPerScrnInfo->screenWhitePt);
}


/*
 *	NAME
 *		XcmsClientWhitePointOfCCC
 *
 *	SYNOPSIS
 */

XcmsColor *
XcmsClientWhitePointOfCCC(ccc)
    XcmsCCC ccc;
/*
 *	DESCRIPTION
 *		Queries the client white point of the specified CCC.
 *
 *	RETURNS
 *		Pointer to the XcmsColor containing the client white point.
 *
 */
{
    return(&ccc->clientWhitePt);
}
