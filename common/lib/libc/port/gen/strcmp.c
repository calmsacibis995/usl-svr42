/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/strcmp.c	1.7"
/*LINTLIBRARY*/
/*
 * Compare strings:  s1>s2: >0  s1==s2: 0  s1<s2: <0
 */

#include "synonyms.h"
#include <string.h>

int
strcmp(s1, s2)
register const char *s1;
register const char *s2;
{

	if((unsigned char *)s1 == (unsigned char *)s2)
		return(0);
	while((unsigned char)*s1 == (unsigned char)*s2++)
		if((unsigned char)*s1++ == '\0')
			return(0);
	return((unsigned char)*s1 - (unsigned char)s2[-1]);
}
