/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)rmntstat:rmntstat.c	1.4.15.2"
#ident  "$Header: rmntstat.c 1.2 91/06/27 $"
#include <stdio.h>
#include <unistd.h>
/* ARGSUSED */
main(argc, argv)
	int    	argc;
	char	**argv;
{
	if( execv("/usr/lib/fs/rfs/dfmounts",argv) == -1){
		perror("execv");
		exit(1);
	}
	exit(0);
	/* NOTREACHED */
}
