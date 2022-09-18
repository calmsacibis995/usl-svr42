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

#ident	"@(#)fdetach:fdetach.c	1.1.2.1"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/fdetach/fdetach.c,v 1.1 91/02/28 17:08:44 ccs Exp $"
#include <stdio.h>
#include <errno.h>
/*
 * Unmount a STREAM from the command line.
 */

main(argc, argv)
	int argc;
	char **argv;
{
	if (argv[1] == NULL)
	{
		printf("usage: fdetach pathname\n");
		exit(-1);
	}
	if (fdetach(argv[1]) < 0)
	{
		perror("fdetach");
		exit(-1);
	}
	exit(0);
}
