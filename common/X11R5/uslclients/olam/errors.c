/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)olam:errors.c	1.1"
/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>

/* conflict with X11/Xos.h. Must undefine SHARELIB to get the correct time.h */
#undef SHARELIB
#undef timezone
#endif
/* XOL SHARELIB - end */

/*
 *************************************************************************
 *
 * Description:
 *		The error diagnostic functions are held in this file.
 *	There are two types of error procedures and two warning procedures.
 *
 *		OlError()		- is a simple fatal error handler
 *		OlWarning()		- is a simple non-fatal error handler
 *		OlVaDisplayErrorMsg()	- Variable argument list fatal
 *					  error handler
 *		OlVaDisplayWarningMsg() - Variable argument list non-fatal
 *					  error handler
 *
 *******************************file*header*******************************
 */

						/* #includes go here	*/

#include <stdio.h>
#include <string.h>
#if defined(__STDC__) || defined(c_plusplus) || defined(__cplusplus)
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <Xt/IntrinsicP.h>

/* Note: OpenLookP.h includes Error.h */

#include <Xol/OpenLookP.h>

#include "errors.h"
#include "olamsgstrs"

