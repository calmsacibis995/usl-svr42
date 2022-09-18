/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:listd/listd.h	1.3"
#endif

#ident	"@(#)listd.h	1.1"

/*
** listd.h - This file should be included by applications using a library
** compiled with `LIST_DOUBLE' defined.
*/


#ifndef _LISTD_H_INCLUDED
#define _LISTD_H_INCLUDED


#define LIST_DOUBLE			/* Make sure macros in "list.h" are */
					/* defined properly for a */
					/* doubly-linked list */

#include <list.h>


#endif	/* _LIST_H_INCLUDED */
