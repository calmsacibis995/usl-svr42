/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/parse.c	1.12.2.3"
#ident "@(#)parse.c	2.21 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	parse - parse the command line

    SYNOPSIS
	int parse(int argc, char **argv, int *pparse_err)

    DESCRIPTION
	Parse the command line.
	Return index of first non-option field (i.e. user)
*/

int parse(argc, argv, pparse_err)
int	argc;
char	**argv;
int	*pparse_err;
{
	register int	 	c;
	register char		*tmailsurr;
	int			optcnt = 0;

	/*
		"mail +" means to print in reverse order and is
		equivalent to "mail -r"
	*/
	if ((argc > 1) && (argv[1] != 0) && (argv[1][0] == '+')) {
		if (ismail) {
			argv[1] = "-r";
		} else {
			(*pparse_err)++;
		}
	}

	while ((c = getopt(argc, argv, "deF:f:hm:PpqrstT:wx:#?")) != EOF) {
		switch (c) {
		/*
			Set debugging level...
		*/
		case 'x':
			debug = flgx = atoi(optarg);
			if (debug < 0) {
				/* Keep trace file even if successful */
				debug = -debug;
			}
			break;

		/*
			for backwards compatability with mailx...
		*/
		case 's':
			/* ignore this option */
			break;

		/*
			debug name translations while in progress
		*/
		case 'd':
			flgd++;
			optcnt++;
			break;

		/*
			do not print mail
 		*/
		case 'e':
			if (ismail) {
				flge = 1;
			} else {
				(*pparse_err)++;
			}
			optcnt++;
			break;

		/*
			use alternate file as mailfile
		*/
		case 'f':
			if (ismail) {
				flgf = optarg;
				if (optarg[0] == '-') {
					errmsg(E_SYNTAX,":390:File names must not begin with '-'\n");
			   		done(0);
				}
			} else {
				(*pparse_err)++;
			}
			optcnt++;
			break;

		/*
			Print headers first
		*/
		case 'h':
			if (ismail) {
				flgh = 1;
			} else {
				(*pparse_err)++;
			}
			optcnt++;
			break;

		/*
			Install/Remove Forwarding
 		*/
		case 'F':
			if (ismail) {
                        	flgF = optarg;	/* Indicate Forwarding */
			} else {
				(*pparse_err)++;
			}
			optcnt++;
			break;

		/* 
			print without prompting
		*/
		case 'p':
			if (ismail) {
				flgp++;
			} else {
				(*pparse_err)++;
			}
			optcnt++;
			break;

		/*
			override selective display default setting
			when reading mail...
		*/
		case 'P':
			if (ismail) {
				flgP++;
			}
			optcnt++;
			break;

		/* 
			terminate on deletes
		*/
		case 'q':
			if (ismail)
				flgq = 1;
			else
				(*pparse_err)++;
			optcnt++;
			break;

		/* 
			print by first in, first out order
		*/
		case 'r':
			if (ismail) {
				flgr = 1;
			} else {
				(*pparse_err)++;
			}
			optcnt++;
			break;

		/*
			add To: line to letters
		*/
		case 't':
			flgt = 1;
			optcnt++;
			break;

		/*
			test mode; display mailsurr transformations but
			don't actually send any messages. Allows specification
			of dummy mailsurr file for testing.
		*/
		case 'T':
			flgT++;
			tmailsurr = optarg;
			if ((tmailsurr != (char *)NULL) &&
			    (*tmailsurr != '\0')) {
				mailsurr = tmailsurr;
			}
			break;

		/*
			don't wait on sends
		*/
		case 'w':
			flgw = 1;
			break;

		/*
			set message-type:
		*/
		case 'm':
			flgm = optarg;
			if (flgm[0] == '\0' || flgm[0] == '-') {
				(*pparse_err)++;
			}
			break;

		/*
			print final name translations
		*/
		case '#':
			flglb++;
			optcnt++;
			break;
	
		/*
			bad option
		*/
		case '?':
			(*pparse_err)++;
			break;
		}
	}

	if (flgF) {
		if (optcnt != 1) {
			errmsg(E_SYNTAX,
				":391:Forwarding is mutually exclusive of other options\n");
			(*pparse_err)++;
		}

		if (argc != optind) {
			pfmt(stderr, MM_ERROR, 
				":127:Too many arguments for forwarding\n");
			pfmt(stderr, MM_ACTION, 
				":392:To forward to multiple users say '%s -F \"user1 user2 ...\"'\n",
				progname);
			error = E_SYNTAX;
			(*pparse_err)++;
		}
	}

	if (argc == optind) {
	    if (flgT) {
		errmsg(E_SYNTAX,
			":481:-%c option used but no recipient(s) specified.\n", 'T');
		(*pparse_err)++;
	    }
	    if (flgd) {
		errmsg(E_SYNTAX,
			":481:-%c option used but no recipient(s) specified.\n", 'T');
		(*pparse_err)++;
	    }
	    if (flglb) {
		errmsg(E_SYNTAX,
			":481:-%c option used but no recipient(s) specified.\n", '#');
		(*pparse_err)++;
	    }
	    if (flgm) {
		errmsg(E_SYNTAX,
			":481:-%c option used but no recipient(s) specified.\n", 'm');
		(*pparse_err)++;
	    }
	}

	if (ismail && ((*pparse_err) > 0)) {
		pfmt(stderr, MM_ACTION,
			":398:Usage: %s [-ehpPqrtw] [-x debuglevel] [-f file]\n", progname);
		pfmt(stderr, MM_NOSTD, ":423:or\t[-x debuglevel] [-m message_type] [-tw] persons\n");
		pfmt(stderr, MM_NOSTD, ":424:or\t[-x debuglevel] [-m message_type] -T file persons\n");
		pfmt(stderr, MM_NOSTD, ":425:or\t[-x debuglevel] -d persons\n");
		pfmt(stderr, MM_NOSTD, ":426:or\t[-x debuglevel] -# persons\n");
		pfmt(stderr, MM_NOSTD, ":427:or\t[-x debuglevel] -F 'user(s)' or '|cmd' or '>|cmd'\n");
		done(0);
	}

	return (optind);
}
