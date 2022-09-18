/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xauth:xauth.c	1.1"
/*
 * $XConsortium: xauth.c,v 1.17 89/12/07 10:39:55 rws Exp $
 *
 * xauth - manipulate authorization file
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Jim Fulton, MIT X Consortium
 */

#include "xauth.h"


/*
 * global data
 */
char *ProgramName;			/* argv[0], set at top of main() */
int verbose = -1;			/* print certain messages */
Bool ignore_locks = False;		/* for error recovery */
Bool break_locks = False;		/* for error recovery */

/*
 * local data
 */

static char *authfilename = NULL;	/* filename of cookie file */
static char *defcmds[] = { "source", "-", NULL };  /* default command */
static int ndefcmds = 2;
static char *defsource = "(stdin)";

/*
 * utility routines
 */
static void usage ()
{
    static char *prefixmsg[] = {
"",
"where options include:",
"    -f authfilename                name of authority file to use",
"    -v                             turn on extra messages",
"    -q                             turn off extra messages",
"    -i                             ignore locks on authority file",
"    -b                             break locks on authority file",
"",
"and commands have the following syntax:",
"",
NULL };
    static char *suffixmsg[] = {
"A dash may be used with the \"merge\" and \"source\" to read from the",
"standard input.  Commands beginning with \"n\" use numeric format.",
"",
NULL };
    char **msg;

    fprintf (stderr, "usage:  %s [-options ...] [command arg ...]\n",
	     ProgramName);
    for (msg = prefixmsg; *msg; msg++) {
	fprintf (stderr, "%s\n", *msg);
    }
    print_help (stderr, NULL, "    ");	/* match prefix indentation */
    fprintf (stderr, "\n");
    for (msg = suffixmsg; *msg; msg++) {
	fprintf (stderr, "%s\n", *msg);
    }
    exit (1);
}


/*
 * The main routine - parses command line and calls action procedures
 */
main (argc, argv)
    int argc;
    char *argv[];
{
    int i;
    char *sourcename = defsource;
    char **arglist = defcmds;
    int nargs = ndefcmds;
    int status;

    ProgramName = argv[0];

    for (i = 1; i < argc; i++) {
	char *arg = argv[i];

	if (arg[0] == '-') {
	    char *flag;

	    for (flag = (arg + 1); *flag; flag++) {
		switch (*flag) {
		  case 'f':		/* -f authfilename */
		    if (++i >= argc) usage ();
		    authfilename = argv[i];
		    continue;
		  case 'v':		/* -v */
		    verbose = 1;
		    continue;
		  case 'q':		/* -q */
		    verbose = 0;
		    continue;
		  case 'b':		/* -b */
		    break_locks = True;
		    continue;
		  case 'i':		/* -i */
		    ignore_locks = True;
		    continue;
		  default:
		    usage ();
		}
	    }
	} else {
	    sourcename = "(argv)";
	    nargs = argc - i;
	    arglist = argv + i;
	    if (verbose == -1) verbose = 0;
	    break;
	}
    }

    if (verbose == -1) {		/* set default, don't junk stdout */
	verbose = (isatty(fileno(stdout)) != 0);
    }

    if (!authfilename) {
	authfilename = XauFileName ();	/* static name, do not free */
	if (!authfilename) {
	    fprintf (stderr,
		     "%s:  unable to generate an authority file name\n",
		     ProgramName);
	    exit (1);
	}
    }
    if (auth_initialize (authfilename) != 0) {
	/* error message printed in auth_initialize */
	exit (1);
    }

    status = process_command (sourcename, 1, nargs, arglist);

    (void) auth_finalize ();
    exit ((status != 0) ? 1 : 0);
}


