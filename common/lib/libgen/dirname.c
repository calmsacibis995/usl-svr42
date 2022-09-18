/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libgen:dirname.c	1.5"

/*
	Return pointer to the directory name, stripping off the last
	component of the path.
	Works similar to /bin/dirname
*/

#ifdef __STDC__
	#pragma weak dirname = _dirname
#endif
#include "synonyms.h"

#include	<string.h>

char *
dirname( s )
char	*s;
{
	register char	*p;

	if( !s  ||  !*s )			/* zero or empty argument */
		return  ".";

	p = s + strlen( s );
	while( p != s  &&  *--p == '/' )	/* trim trailing /s */
		;
	
	if ( p == s && *p == '/' )
		return "/";

	while( p != s )
		if( *--p == '/' ) {
			if ( p == s )
				return "/";
			while ( *p == '/' )
				p--;
			*++p = '\0';
			return  s;
		}
	
	return  ".";
}
