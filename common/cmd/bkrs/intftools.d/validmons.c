/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/validmons.c	1.2.5.2"
#ident  "$Header: validmons.c 1.2 91/06/21 $"

#define TRUE	1
#define FALSE	0
#define NULL	0

void exit();

main( argc, argv )
int argc;
char *argv[];
{
	int ok = FALSE;
	unsigned char *ptr;
	int begin;
	int end;
	unsigned char *p_monrange();

	if( argc != 2 )
		exit( 1 );

	ptr = (unsigned char *) argv[1];
	while( ptr = p_monrange( ptr, &begin, &end ) ) {
		if( !*ptr ) {
			ok = TRUE;
			break;
		}
		ptr++;
	}

	exit( !ok );
}

unsigned char *
p_monrange( string, begin, end )
unsigned char *string;
int *begin, *end;
{
	unsigned char *ptr;
	unsigned char *p_range();

	ptr = p_range( string, begin, end );
	if( *begin > 0 && *begin <= 12 && *end > 0 && *end <= 12 )
		return( ptr );
	else return( NULL );
}
