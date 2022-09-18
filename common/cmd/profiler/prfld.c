/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)profiler:prfld.c	1.11.6.5"
#ident	"$Header: $"

/*
 *	prfld - load profiler with sorted kernel text addresses
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/prf.h>

main( int argc, char **argv ) {
	int prf_fd;
	void prf_load( const int prf_fd );

	if(argc > 2)
		error("usage: prfld");

	if(argc > 1)	/* command used to take bootable kernel as arg	*/
		warning("extraneous argument ignored");

	if( (prf_fd = open("/dev/prf", O_RDWR)) < 0)
		error("cannot open /dev/prf");

	prf_load( prf_fd );

	exit(0);
}
