/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:port/wstring/wsncat.c	1.1.1.2"
#ident  "$Header: wsncat.c 1.2 91/06/26 $"

/*
 * Concatenate s2 on the end of s1. S1's space must be large enough.
 * At most n characters are moved.
 * return s1.
 */

#include <widec.h>

wchar_t *
wsncat(s1, s2, n)
wchar_t *s1, *s2;
register n;
{
	register wchar_t *os1 = s1;

	while (*s1++) /* find end of s1 */
		;
	++n;
	--s1;
	while (*s1++ = *s2++) /* copy s2 to s1 */
		if (--n == 0) {  /* at most n chars */
			*--s1 = 0;
			break;
		}
	return(os1);
}
