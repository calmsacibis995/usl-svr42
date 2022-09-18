/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kdb.cmd:i386/cmd/kdb/kdb.c	1.1"

/*
 * This program causes a kernel debugger,
 * if any is installed, to be invoked.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/sysi86.h>
#include <errno.h>


extern long strtol();

char *_Progname;


main(argc, argv)
	int	argc;
	char	*argv[];
{
	_Progname = argv[0];

	if (argc == 1)
		return invoke_kdb();

	fprintf(stderr, "Usage: %s\n", _Progname);
	return 1;
}


int
invoke_kdb()
{
	if (sysi86(SI86TODEMON) < 0) {
		if (errno == EPERM || errno == EACCES)
			fprintf(stderr, "%s: Permission denied\n", _Progname);
		else
			perror(_Progname);
		return 1;
	}
	return 0;
}
