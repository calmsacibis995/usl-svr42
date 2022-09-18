/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/validdays.c	1.3.6.2"
#ident  "$Header: validdays.c 1.2 91/06/21 $"

#define TRUE	1
#define FALSE	0

main( argc, argv )
int argc;
char *argv[];
{
	unsigned char *ptr;
	unsigned char *p_dayrange();

	int ok = FALSE;
	int begin;
	int end;

	void exit();

	if( argc != 2 )
		exit( 1 );

	ptr = (unsigned char *) argv[1];
	while( ptr = p_dayrange( ptr, &begin, &end ) ) {
		if (begin < 0 || begin > 6 || end < 0 || end > 6)
			exit(1);
		if( !*ptr ) {
			ok = TRUE;
			break;
		}
		ptr++;
	}

	exit( !ok );
}
