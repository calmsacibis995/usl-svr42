/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:UNDEFINED.c	1.1"
/* $XConsortium: UNDEFINED.c,v 1.5 91/07/25 01:10:56 rws Exp $" */

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
 *		UNDEFINED.c
 *
 *	DESCRIPTION
 *		UNDEFINED Color Space
 *
 *
 */

#include "Xlibint.h"
#include "Xcmsint.h"

/*
 *      FORWARD DECLARATIONS
 */
static int ReturnZero();


/*
 *      LOCALS VARIABLES
 */

static Status (*(Fl_ReturnZero[]))() = {
    ReturnZero,
    NULL
};




/*
 *      GLOBALS
 *              Variables declared in this package that are allowed
 *		to be used globally.
 */
    /*
     * UNDEFINED Color Space
     */
XcmsColorSpace	XcmsUNDEFINEDColorSpace =
    {
	"undefined",		/* prefix */
	XcmsUndefinedFormat,	/* id */
	ReturnZero,		/* parseString */
	Fl_ReturnZero,		/* to_CIEXYZ */
	Fl_ReturnZero		/* from_CIEXYZ */
    };



/************************************************************************
 *									*
 *			PRIVATE ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		ReturnZero
 *
 *	SYNOPSIS
 */
/* ARGSUSED */
static int
ReturnZero()
/*
 *	DESCRIPTION
 *		Does nothing.
 *
 *	RETURNS
 *		0
 *
 */
{
    return(0);
}
