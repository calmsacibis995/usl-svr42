/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/bklib.d/bld_ddev.c	1.1.6.2"
#ident  "$Header: bld_ddev.c 1.2 91/06/21 $"

#include	<stdio.h>

extern void *malloc();

static
int
ncolons( str )
char *str;
{
	register n = 0;
	char *p;

	if( str ) {
		for( p = str; *p; p++ )
			if( *p == ':' ) n++;
	}
	return( n );
}

/* Copy <string> to <buffer>, escaping ':' */
static
char *
copy( buffer, string )
char *buffer, *string;
{
	if( string ) {
		while( *string ) {
			if( *string == ':' )
				*buffer++ = '\\';
			*buffer++ = *string++;
		}
	}

	return( buffer );
}

/*
	Build a <group>:<device>:<dchar>:<labels> string,
	escaping the ':'s.
*/
char *
bld_ddevice( dgroup, ddevice, dchar, dlabels )
char *dgroup, *ddevice, *dchar, *dlabels;
{
	register size, nescapes = 0;
	char *buffer, *ptr;

	nescapes = ncolons( dgroup ) + ncolons( ddevice )
		+ ncolons( dchar ) + ncolons( dlabels );

	size = nescapes + 3;	/* + 3 is for the colon separators*/
	if (dgroup != NULL) size += strlen( dgroup );
	if (ddevice != NULL) size += strlen( ddevice );
	if (dchar != NULL) size += strlen( dchar );
	if (dlabels != NULL) size +=   strlen( dlabels );

	if( !(buffer = (char *)malloc( size + 1 ) ) )
		/* no memory */
		return( buffer );

	if( !nescapes ) {
		/* No colons to escape */
		(void) sprintf( buffer, "%s:%s:%s:%s",
			(dgroup != NULL) ? dgroup : (char *)"",
			(ddevice != NULL) ? ddevice : (char *)"",
			(dchar != NULL) ? dchar : (char *)"",
			(dlabels != NULL ) ? dlabels : (char *)"");

	} else {
		ptr = copy( buffer, dgroup );
		*ptr++ = ':';

		ptr = copy( ptr, ddevice );
		*ptr++ = ':';

		ptr = copy( ptr, dchar );
		*ptr++ = ':';

		ptr = copy( ptr, dlabels );
		*ptr = '\0';
	}

	return( buffer );
}
