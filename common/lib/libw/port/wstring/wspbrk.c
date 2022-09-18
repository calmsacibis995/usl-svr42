/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:port/wstring/wspbrk.c	1.1.1.2"
#ident  "$Header: wspbrk.c 1.2 91/06/26 $"

/*
 * Return ptr to first occurance of any character from 'brkset'
 * in the wchar_t array 'string'; NULL if none exists.
 */

#define  WNULL	(wchar_t *)0
#include <widec.h>

wchar_t *
wspbrk(string, brkset)
register wchar_t *string, *brkset;
{
	register wchar_t *p;

	do {
		for (p = brkset; *p && *p != *string; ++p)
			;
		if (*p)
			return(string);
	} while (*string++);
	return(WNULL);
}
