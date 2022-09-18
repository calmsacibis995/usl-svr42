/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:port/wstring/wscmp.c	1.1.1.2"
#ident  "$Header: wscmp.c 1.2 91/06/26 $"

/*
 * Compare strings:  s1>s2: >0  s1==s2: 0  s1<s2: <0
*/

#include <widec.h>

int
wscmp(s1, s2)
register wchar_t *s1, *s2;
{
	if (s1 == s2)
		return(0);

	while (*s1 == *s2++)
		if (*s1++ == 0)
			return(0);
	return(*s1 - *--s2);
}
