/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/strchr.c	1.7"
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
#include "synonyms.h"
#include <string.h>
#include <stddef.h>

/*
 * Return the ptr in sp at which the character c appears;
 * NULL if not found
 */

char *
strchr(sp, c)
register const char *sp;
register int c;
{
	register char ch = (char)c;
	do {
		if(*sp == ch)
			return((char *)sp);
	} while(*sp++);
	return(NULL);
}
