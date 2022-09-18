/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pcintf:pkg_lcs/stricmp.c	1.2"
/* SCCSID(@(#)stricmp.c	7.1	LCC)	* Modified: 15:35:21 10/15/90 */
/*
 *  stricmp function
 */

#include <ctype.h>


stricmp(s1, s2)
register char *s1, *s2;
{
	if (s1 == s2)
		return 0;
	while (tolower(*s1) == tolower(*s2)) {
		if (*s1++ == '\0')
			return 0;
		s2++;
	}
	return *s1 - *s2;
}
