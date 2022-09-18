/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/uucollapse.c	1.2"
#ident "@(#)uucollapse.c	1.1 'attmail mail(1) command'"
#include "libmail.h"
/*
    NAME
	uucollapse - collapse a bang-path mail address, removing loops

    SYNOPSIS
	uucollapse address ...

    DESCRIPTION
	Uucollapse scans a mail address written in uucp bang format and
	collapses it, removing loops and cycles. For example,

		sysa!sysb!sysa!user

	can be collapsed down to

		sysa!user

	because sysb can only talk with one sysa. Similarly,

		sysa!sysa!user

	is collapsed down to to

		sysa!user

*/

main(argc, argv)
int argc;
char **argv;
{
    int mainret = 0;
    if (argc == 1)
	{
	(void) pfmt(stdout, MM_ERROR, ":120:Incorrect usage\n");
	(void) pfmt(stdout, MM_ACTION, ":508:Usage: %s address ...\n", argv[0]);
	exit(1);
	/* NOTREACHED */
	}

    while (*++argv)
	{
	int ret;
	char buf[1024];
	strncpy(buf, *argv, sizeof(buf));
	ret = bang_collapse(buf);
	switch (ret)
	    {
	    case  0:
		(void) printf("%s %s\n", *argv, buf);
		break;
	    case -1:
		(void) fprintf (stderr, "Invalid address: %s\n", *argv);
		mainret = 1;
		break;
	    case -2:
		(void) fprintf (stderr, "No memory: %s\n", *argv);
		mainret = 1;
		break;
	    }
	}

    return mainret;
}
