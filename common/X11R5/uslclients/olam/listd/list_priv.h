/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:listd/list_priv.h	1.3"
#endif

#ident	"@(#)list_priv.h	1.4"

/*
** list_priv.h - This is a private file that should be included by the files
** that implement the list functions.
*/


#ifndef _LIST_PRIVATE_H_INCLUDED
#define _LIST_PRIVATE_H_INCLUDED


#include "list.h"


/*
** These macros define the exit values used when a function exits because of
** a programmer error or internal error respectively.  This only happens if
** `LIST_DEBUG' is defined.
*/
#define _LIST_USE_ERROR		3
#define _LIST_INTERNAL_ERROR	4


/*
** Compile in debuging messages if `LIST_DEBUG' is defined
*/
#ifdef LIST_DEBUG

#include <stdio.h>

#define _LIST_MSG(tmpl, str)	((void)fputs("LIST: ", stderr), \
				 (void)fprintf(stderr, (tmpl), (str)), \
				 (void)putc('\n', stderr), \
				 (void)fflush(stderr))

#else	/* LIST_DEBUG */

#define _LIST_MSG(tmpl, str)

#endif	/* LIST_DEBUG */


#endif	/* _LIST_PRIVATE_H_INCLUDED */
