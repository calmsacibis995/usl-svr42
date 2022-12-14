/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libgen:strrspn.c	1.2"

#ifdef __STDC__
	#pragma weak strrspn = _strrspn
#endif
#include "synonyms.h"

/*
	Trim trailing characters from a string.
	Returns pointer to the first character in the string
	to be trimmed (tc).
*/

#include	<string.h>


char *
strrspn( string, tc )
const char	*string;
const char	*tc;	/* characters to trim */
{
	char	*p;

	p = (char *)string + strlen( string );  
	while( p != (char *)string )
		if( !strchr( tc, *--p ) )
			return  ++p;
		
	return  p;
}
