/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/strlist.c	1.1"

#include <stdarg.h>
#include <string.h>

char *strlist(char *s1, const char *s2, ...)
{
char *string = (char *)s2;
va_list ap;

while(*s1++);
va_start(ap, s2);
while(string)
{
	--s1;
	while(*s1++ = *string++);
	string = va_arg(ap, char *);
}
va_end(ap);
return s1;
}
