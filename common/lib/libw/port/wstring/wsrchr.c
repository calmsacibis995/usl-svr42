/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:port/wstring/wsrchr.c	1.1.1.2"
#ident  "$Header: wsrchr.c 1.2 91/06/26 $"

/*
 * Return the ptr in sp at which the character c last appears;
 * Null if not found.
 */

#define  WNULL	(wchar_t *)0
#include <widec.h>

wchar_t *
wsrchr(sp, c)
register wchar_t *sp;
register c;
{
	register wchar_t *r = WNULL;

	do {
		if (*sp == c)
			r = sp; /* found c in sp */
	} while (*sp++);
	return(r);
}
