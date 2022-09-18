/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mailinfo.c	1.5.2.6"
#ident "@(#)mailinfo.c	1.9 'attmail mail(1) command'"
/*
    NAME
	mailinfo - access mail configuration information

    SYNOPSIS
	mailinfo -n	# retrieve full system name, system.domain
	mailinfo -d	# retrieve domain name
	mailinfo -s	# retrieve system name, $CLUSTER or uname
	mailinfo -u	# retrieve uname
	mailinfo X	# look up $X in mailcnfg

    DESCRIPTION
	/usr/lib/mail/surrcmd/mailinfo looks up information
	from the mail databases.
*/

#include "libmail.h"

const char *progname = "";

void usage()
{
    pfmt(stderr, MM_ACTION, ":431:Usage: %s -n | -d | -s | -u | config-name ...\n", progname);
    (void) pfmt (stderr, MM_NOSTD, ":432:\t-n\tprint full system name, system.domain\n");
    (void) pfmt (stderr, MM_NOSTD, ":433:\t-d\tprint domain name\n");
    (void) pfmt (stderr, MM_NOSTD, ":434:\t-s\tprint system name\n");
    (void) pfmt (stderr, MM_NOSTD, ":435:\t-u\tprint uname\n");
    exit(1); /* NOTREACHED */
}

main(argc, argv)
int argc;
char **argv;
{
    int c;
    progname = argv[0];
    
    (void) setlocale(LC_ALL, "");
    (void) setlabel("UX:mailinfo");
    (void) setcat("uxemail");

    if (argc == 1)
	usage();

    while ((c = getopt(argc, argv, "ndsu?")) != -1)
	switch (c)
	    {
	    case 'n': (void) printf("%s%s\n", mailsystem(0), maildomain()); break;
	    case 'd': (void) printf("%s\n", maildomain()); break;
	    case 's': (void) printf("%s\n", mailsystem(0)); break;
	    case 'u': (void) printf("%s\n", mailsystem(1)); break;
	    case '?': usage();
	    }

    for ( ; optind < argc; optind++)
	(void) printf("%s\n", mgetenv(argv[optind]));

    return 0;
}
