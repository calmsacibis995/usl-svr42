/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libTL:common/lib/libTL/utils.c	1.3.5.2"
#ident  "$Header: utils.c 1.2 91/06/25 $"

#include <stdio.h>

Malloc( size )
int size;
{
	char *addr;
	addr = (char *) malloc( (unsigned)size );

	printf( "malloc( %d ) = 0x%x\n", size, addr );	

	return( (long)addr );
}

Free( addr )
char *addr;
{

	printf( "free( 0x%x )\n", addr );

	(void) free( addr );
}

Realloc( ptr, size )
char *ptr;
int size;
{
	char *addr;
	addr = (char *) realloc( ptr, size );

	printf( "ralloc( 0x%x, %d ) = 0x%x\n", ptr, size, addr );	

	return( (long)addr );
}
