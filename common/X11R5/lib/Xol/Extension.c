/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)mouseless:Extension.c	1.1"
#endif

/*
 *************************************************************************
 *
 * Description:
 * 		This file contains routines to manipulate class extensions
 * 
 ****************************file*header**********************************
 */

						/* #includes go here	*/
#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Public  Procedures
 *
 **************************forward*declarations***************************
 */

					/* public procedures		*/

OlClassExtension	_OlGetClassExtension();

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define NULL_EXTENSION	((OlClassExtension)NULL)

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * _OlGetClassExtension - this routine gets a class extension from a
 * class's extension list.
 ****************************procedure*header*****************************
 */
OlClassExtension
_OlGetClassExtension(extension, record_type, version)
	OlClassExtension extension;	/* first extension		*/
	XrmQuark	record_type;	/* type to look for		*/
	long		version;	/* if non-zero, look for it	*/
{
	while (extension != NULL_EXTENSION &&
		!(extension->record_type == record_type &&
		  (!version || version == extension->version)))
	{
		extension = (OlClassExtension) extension->next_extension;
	}
	return (extension);
} /* END OF _OlGetClassExtension() */

