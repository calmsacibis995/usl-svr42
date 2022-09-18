/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:port/wstring/wsncmp.c	1.1.1.2"
#ident  "$Header: wsncmp.c 1.2 91/06/26 $"

/*
 * Compare strings (at most n characters)
 * 	returns:  s1>s2: >0  s1==s2: 0  s1<s2: <0
*/

#include <widec.h>

int
wsncmp(s1, s2, n)
register wchar_t *s1, *s2;
register n;
{
	if (s1 == s2)
		return(0);

	n++;
	while (--n > 0 && *s1 == *s2++)
		if (*s1++ == 0)
			return(0);
	return((n == 0) ? 0 : (*s1 - *--s2));
}
