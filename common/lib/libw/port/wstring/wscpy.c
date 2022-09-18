/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:port/wstring/wscpy.c	1.1.1.2"
#ident  "$Header: wscpy.c 1.2 91/06/26 $"

/*
 * Copy string s2 to s1. S1 must be large enough.
 * Return s1.
 */

#include <widec.h>

wchar_t *
wscpy(s1, s2)
register wchar_t *s1, *s2;
{
	register wchar_t *os1 = s1;

	while (*s1++ = *s2++)
		;
	return(os1);
}
