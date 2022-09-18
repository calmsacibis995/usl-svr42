/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)profiler:prfstat.c	1.6.3.5"
#ident	"$Header: $"

/*
 *	prfstat - change and/or report profiler status
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/prf.h>

#ifdef sgn
#undef sgn
#endif

#define sgn( x )	( (x) ? ( (x) > 0 ? 1 : -1 ) : 0 )

main( int argc, char **argv ) {
	int prf;
	int prfmax;
	int prfstat;

	void prf_start( const int prf_fd );

	if((prf = open("/dev/prf", O_RDWR)) < 0)
		error("cannot open /dev/prf\n");

	if(argc > 2) {
		fprintf(stderr,"usage: prfstat  [ on | off ]\n");
		exit(1);
	}
	if(argc == 2) {
		if(strcmp("off", argv[1]) == 0)
			ioctl(prf, PRF_DISABLE, 0);
		else if(strcmp("on", argv[1]) == 0)
			prf_start( prf );
		else {
			fprintf(stderr,"prfstat: unrecognized argument.\n");
			fprintf(stderr,"usage: prfstat  [ on | off ]\n");
			exit(1);
		}
	}

	prfstat = ioctl(prf, PRF_STAT, 0);

	switch( sgn( prfstat ) ) {
	case 0:
		printf("profiling disabled.\n");
		break;
	case 1:
		printf("profiling enabled.\n");
		break;
	default:
		error("cannot determine profiling status.\n");
	}

	prfmax = ioctl(prf, PRF_MAX, 0);

	switch( sgn( prfmax ) ) {
	case 1:
		printf("%d kernel text addresses loaded.\n", prfmax );
		break;
	case 0:
		printf("no kernel text addresses loaded.\n");
		break;
	default:
		error("cannot determine number of text addresses.\n");
	}
	exit( 0 );
}

