/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:port/wstring/wstok.c	1.1.1.2"
#ident  "$Header: wstok.c 1.2 91/06/26 $"

/*
 * uses wspbrk and wsspn to break string into tokens on
 * sequentially subsequent calls. returns NULL when no
 * non-separator characters remain.
 * 'subsequent' calls are calls with first argument NULL.
 */

#define  WNULL 	(wchar_t *)0
#include <widec.h>

wchar_t *
wstok(string, sepset)
wchar_t *string, *sepset;
{
	register wchar_t *p, *q, *r;
	static wchar_t *savept = 0;

	/* first or subsequent call */
	p = ((string == WNULL) ? savept: string);

	/* return if no tokens remaining */
	if (p == WNULL)
		return(WNULL);

	/* skip leading separators */
	q = p + wsspn(p, sepset);

	/* return if no tokens remaining */
	if (*q == 0)
		return(WNULL);

	/* move past token */
	if ((r = wspbrk(q, sepset)) == WNULL)
		savept = WNULL; /* indicate this is last token */
	else {
		*r = 0;
		savept = ++r;
	}

	return(q);
}
