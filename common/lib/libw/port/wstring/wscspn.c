/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:port/wstring/wscspn.c	1.1.1.2"
#ident  "$Header: wscspn.c 1.2 91/06/26 $"

/*
 * Return the number of characters in the maximum leading segment
 * of string which consists solely of characters NOT from charset.
 */

#include <widec.h>

int
wscspn(string, charset)
wchar_t *string;
register wchar_t *charset;
{
	register wchar_t *p, *q;

	for (q = string; *q != 0; ++q) {
		for (p = charset; *p != 0 && *p != *q; ++p)
			;
		if (*p != 0)
			break;
	}
	return(q - string);
}
