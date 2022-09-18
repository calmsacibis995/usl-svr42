/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/localmail.c	1.3"
#ident "@(#)localmail.c	1.3 'attmail mail(1) command'"
/*
    NAME
	localmail - look up local mail names

    SYNOPSIS
	localmail [-p] [-P prefix] [-S suffix] name ...

    DESCRIPTION
	Look up the user names in /etc/passwd and in /var/mail. If they
	are local mail names or user names, then output <prefix>name<suffix>,
	where <prefix> and <suffix> are specified by the -P and -S options,
	respectively. If -p is specified, print the original name as well.
*/

#include <stdio.h>
#include "libmail.h"

const char *prefix = "";
const char *suffix = "";
const char *progname = "";		/* argv[0] */

static void usage()
{
    (void) pfmt(stdout, MM_ERROR, ":120:Incorrect usage\n");
    (void) pfmt(stdout, MM_ACTION, ":517:Usage: %s [-p] [-P prefix] [-S suffix] user-name ...\n", progname);
    (void) pfmt(stdout, MM_ACTION, ":520:\t-P\tprefix to be added\n");
    (void) pfmt(stdout, MM_ACTION, ":521:\t-S\tsuffix to be added\n");
    exit(1);
    /* NOTREACHED */
}

main(argc, argv)
    int argc;
    char *argv[];
{
    int i, printname = 0;
    progname = argv[0];
    (void) setlocale(LC_ALL, "");
    (void) setcat("uxemail");
    (void) setlabel("UX:mailalias");

    while ((i = getopt(argc, argv, "pP:S:?")) != -1)
	switch (i)
	    {
	    case 'p': printname = 1; break;
	    case 'P': prefix = optarg; break;
	    case 'S': suffix = optarg; break;
	    case '?': usage();
	    }

    for (i = optind; i < argc; i++)
	{
	if (printname)
	    (void) printf("%s\t", argv[i]);
	if (islocal(argv[i], (uid_t*)0))
	    printf("%s\n", argv[i]);
	else
	    printf("%s%s%s\n", prefix, argv[i], suffix);
	}

    return 0;
}
