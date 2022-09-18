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

/* conflict with X11/Xos.h. Must undefine SHARELIB to get the correct time.h */
#undef SHARELIB
#undef timezone
#endif
/* XOL SHARELIB - end */

#ifndef NOIDENT
#ident	"@(#)olexamples:tutorial/msgs.c	1.1"
#endif

/*
 *************************************************************************
 *
 * Description:
 *		This file merely includes the message strings
 *		for the OPEN LOOK client library.
 *
 *******************************file*header*******************************
 */

						/* #includes go here	*/

#include <string.h>
#include <Xt/IntrinsicP.h>

#include <ssmsgstrs>
