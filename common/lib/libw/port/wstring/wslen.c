/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:port/wstring/wslen.c	1.1.1.2"
#ident  "$Header: wslen.c 1.2 91/06/26 $"

/*
 * Returns the number of non-NULL characters in s.
 */

#include <widec.h>

wslen(s)
register wchar_t *s;
{
	register wchar_t *s0 = s + 1;

	while (*s++)
		;
	return(s - s0);
}
