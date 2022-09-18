/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/getopt.c	1.28"

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <pfmt.h>
#include "pfmt_data.h"
#include <stdlib.h>

/*
 * If building the regular library, pick up the defintions from this file
 * If building the shared library, pick up definitions from opt_data.c 
 */

int _sp = 1;
#ifdef __STDC__
static void pr_error(const char *, const char *, int);
#endif

int
getopt(argc, argv, opts)
int	argc;
char	*const *argv;
const	char *opts;
{
	register char c;
	register char *cp;

	if(_sp == 1)
		if(optind >= argc ||
		   argv[optind][0] != '-' || argv[optind][1] == '\0')
			return(EOF);
		else if(strcmp(argv[optind], "--") == NULL) {
			optind++;
			return(EOF);
		}
	optopt = c = (unsigned char)argv[optind][_sp];
	if(c == ':' || (cp=strchr(opts, c)) == NULL) {
		if(opterr)
			pr_error(argv[0], "uxlibc:1:Illegal option -- %c\n", c);
		if(argv[optind][++_sp] == '\0') {
			optind++;
			_sp = 1;
		}
		return('?');
	}
	if(*++cp == ':') {
		if(argv[optind][_sp+1] != '\0')
			optarg = &argv[optind++][_sp+1];
		else if(++optind >= argc) {
		if (opterr)
			pr_error(argv[0],
				"uxlibc:2:Option requires an argument -- %c\n",c);
			_sp = 1;
			return('?');
		} else
			optarg = argv[optind++];
		_sp = 1;
	} else {
		if(argv[optind][++_sp] == '\0') {
			_sp = 1;
			optind++;
		}
		optarg = NULL;
	}
	return(c);
}

/* Print the error message.
 * uses argv[0] as label if setlabel() was not called before
 */
static void
pr_error(cmdname, msg, opt)
const char *cmdname;
const char *msg;
int opt;
{
	int tmp_label;

	if (!__pfmt_label[0]){
		const char *lab = cmdname;
		if (strlen(lab) > (size_t)25) {
			lab = (char *)strrchr(lab, '/');
			if (lab)
				lab++;
			else
				lab = cmdname;
		}
		tmp_label = 1;
		(void)setlabel(lab);
	}
	else
		tmp_label = 0;

	(void)pfmt(stderr, MM_ERROR, msg, opt);

	if (tmp_label)
		__pfmt_label[0] = '\0';
}
