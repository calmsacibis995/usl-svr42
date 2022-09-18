/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:port/wstring/wsncpy.c	1.1.1.2"
#ident  "$Header: wsncpy.c 1.2 91/06/26 $"

/*
 * Copy s2 to s1, truncating or null-padding to always copy n characters.
 * Return s1.
 */

#include <widec.h>

wchar_t *
wsncpy(s1, s2, n)
register wchar_t *s1, *s2;
register n;
{
	register wchar_t *os1 = s1;

	n++;
	while ((--n > 0) && ((*s1++ = *s2++) != 0))
		;
	if (n > 0)
		while (--n > 0)
			*s1++ = 0;
	return(os1);
}
