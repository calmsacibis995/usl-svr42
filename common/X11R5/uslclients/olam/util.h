/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:util.h	1.5"
#endif

/*
** util.h - This file contains definitions for utility macros.
*/


#ifndef _OLAM_UTIL_H
#define _OLAM_UTIL_H


#include <stdio.h>			/* Needed by DEBUG() */


#ifdef DEBUG
#define DEBUG_MSG(tmpl, str)	((void)fputs("OLAM DEBUG: ", stderr), \
				 (void)fprintf(stderr, (tmpl), (str)), \
				 (void)fflush(stderr))
#else
#define DEBUG_MSG(tmpl, str)
#endif

/*
** Bit manipulating macros
*/
#define BIT_IS_SET(field, bit)	(((field) & (bit)) != (unsigned)0)
#define SET_BIT(field, bit)	((field) |= (bit))
#define UNSET_BIT(field, bit)	((field) &= ~(bit))

/*
** Register help for widget `w'.
*/
#define REGISTER_HELP(w, tag, file) \
  OlRegisterHelp(OL_WIDGET_HELP, (w), (tag), OL_DISK_SOURCE, \
		 OlFindHelpFile(w, file))


#endif	/* _OLAM_UTIL_H */
