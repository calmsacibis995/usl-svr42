/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/substr.c	1.6.2.2"
#ident "@(#)substr.c	2.6 'attmail mail(1) command'"
#include "libmail.h"
/*
    NAME
	substr - find substring

    SYNOPSIS
	int substr(char *string, char *substring)

    DESCRIPTION
	This routine looks for substring in string.
	If found, it returns the position substring is found at,
	otherwise it returns a -1.
*/

int substr(string1, string2)
const char *string1, *string2;
{
	register int i,j, len1, len2;

	len1 = strlen(string1);
	len2 = strlen(string2);
	for (i = 0; i < len1 - len2 + 1; i++) {
		for (j = 0; j < len2 && string1[i+j] == string2[j]; j++);
		if (j == len2) return(i);
	}
	return(-1);
}
