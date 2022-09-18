/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)setpgrp:setpgrp.c	1.2.2.1"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/setpgrp/setpgrp.c,v 1.1 91/02/28 19:48:43 ccs Exp $"
/*
	Set process group ID to this process ID and exec the command line
	that is the argument list.
*/


#include <stdio.h>

main( argc, argv )
int	argc;
char	*argv[];
{
	char	*cmd;

	cmd = *argv;
	if( argc <= 1 ) {
		fprintf( stderr, "Usage:  %s command [ arg ... ]\n", cmd );
		exit(1);
	}
	argv++;
	argc--;
	setpgrp();
	execvp( *argv, argv );
	fprintf( stderr, "%s: %s not executed.  ", cmd, *argv );
	perror( "" );
	exit( 1 );
}
