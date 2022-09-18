/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:pkg_lcs/caseconv.c	1.2"
/* SCCSID(@(#)caseconv.c	7.1	LCC)	* Modified: 15:31:33 10/15/90 */

/*
 *  Case conversion routines
 */

#include "lcs.h"
#include "lcs_int.h"


lcs_isupper(c)
lcs_char c;
{
	return ((c >=  lcs_ascii('A') && c <= lcs_ascii('Z')) ||
		(c >= 0x20c0 && c <= 0x20de && c != 0x20d7));
}


lcs_islower(c)
lcs_char c;
{
	return ((c >=  lcs_ascii('a') && c <= lcs_ascii('z')) ||
		(c >= 0x20e0 && c <= 0x20fe && c != 0x20f7));
}


lcs_char
lcs_toupper(c)
lcs_char c;
{
	if (c >= lcs_ascii('a') && c <= lcs_ascii('z'))
		return c + 'A' - 'a';
	if (c >= 0x20e0 && c <= 0x20fe && c != 0x20f7)
		return c - 0x20;
	return c;
}


lcs_char
lcs_tolower(c)
lcs_char c;
{
	if (c >= lcs_ascii('A') && c <= lcs_ascii('Z'))
		return c + 'a' - 'A';
	if (c >= 0x20c0 && c <= 0x20de && c != 0x20d7)
		return c + 0x20;
	return c;
}
