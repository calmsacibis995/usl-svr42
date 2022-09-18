/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:i386/mystrstr.c	1.2"
/*LINTLIBRARY*/


#include	<stdio.h>

/*
	mystrstr() locates the first occurrence in the string as1 of
	the sequence of characters (excluding the terminating null
	character) in the string as2. mystrstr() returns a pointer 
	to the located string, or a null pointer if the string is
	not found. If as2 is "", the function returns as1.
*/

char *
mystrstr(as1, as2)
char *as1;
char *as2;
{
	register  char *s1,*s2;
	register char c;
	register  char *tptr;

	s1 = as1;
	s2 = as2;

	if (s2 == NULL || *s2 == '\0')
		return((char *)s1);
	c = *s2;

	while (*s1)
		if (*s1++ == c) {
			tptr = s1;
			while ((c = *++s2) == *s1++ && c) ;
			if (c == 0)
				return((char *)tptr - 1);
			s1 = tptr;
			s2 = as2;
			c = *s2;
		}
	 return(NULL);
}
