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

#ident	"@(#)rexec:rxlist.c	1.2.2.3"
#ident  "$Header: rxlist.c 1.2 91/06/27 $"

/*
 * rxlist - list accessable REXEC services
 *
 * This is meant to be run as the REXEC service "rquery"
 *
 * Synopsis
 *
 *	rxlist [-l] [-h]
 *
 *	-l generates a long listing (vs. just the service name)
 *	-h generates headers
 *
 */

#include <sys/types.h>
#include <stdio.h>
#include <locale.h>
#include <unistd.h>
#include <pfmt.h>
#include <rx.h>
#include <stdlib.h>
#include <string.h>
#include "rxsvcent.h"

#define	OPTIONS	"lh"

extern	int	skipcmnt();
extern	int	getsvcent();


int
main(argc, argv)
int	argc;
char	*argv[];
{
	int	c;			/* options character */
	int	opterror = 0;		/* usage error flag */
	int	longlist = 0;		/* long listing flag */
	int	headers = 0;		/* print headers flag */
	FILE	*fp;			/* service file */
	RX_SERVICE	service;	/* service entry */
	char	cmd[RX_MAXSVCDEF];	/* service command */
	char	*cmd_tail;		/* ptr to first char after cmd */
	char	*cmd_delims = " \t\n\r";/* cmd delimiters */
	char	eof = 0;		/* eof flag */

	/* set up error message handling */

	(void) setlocale(LC_ALL, "");
	(void) setlabel("UX:rxlist");
	(void) setcat("uxnsu");

	while((c = getopt(argc, argv, OPTIONS)) != EOF)
		switch(c) {
		case 'l':
			longlist++;
			break;
		case 'h':
			headers++;
			break;
		case '?':
			opterror++;
		}

	if (opterror) {
		(void) pfmt(stderr, MM_ERROR, ":73:Syntax\n");
		(void) pfmt(stderr, MM_ACTION, ":165:%s: Usage:\n", argv[0]);
		(void) pfmt(stderr, MM_ACTION, ":166:\t%s [-l] [-h]\n", argv[0]);
		exit(1);
	}

	if ((fp = fopen(RX_SVCFILE, "r")) == (FILE *) NULL) {
		(void) pfmt(stderr, MM_ERROR,
			    ":167:%s: the list of services cannot be obtained\n",
			    argv[0]);
		(void) pfmt(stderr, MM_ACTION,
			    ":168:see administrator's guide\n");
		exit(1);
	}

	if (headers)
		if (longlist)
			(void) printf("SERVICE\tDESCRIPTION\tUTMP\tDEFINITION\n");
		else
			(void) printf("SERVICE\tDESCRIPTION\n");

	while(!eof) {
		if (skipcmnt(fp) < 0) {
			eof++;
			continue;
		}
		if (getsvcent(fp, &service, 0) < 0) {
			eof++;
			continue;
		}

		(void) strncpy(cmd, service.def, RX_MAXSVCDEF);

		if ((cmd_tail = strpbrk(cmd, cmd_delims)) != NULL)
			*cmd_tail = '\0';

		/* list executable and transparent commands */
		if ((access(cmd, X_OK) == 0) || (cmd[0] == '%')) {
			(void) printf("%s\t%s", service.name, service.descr);
			if (longlist)
				(void) printf("\t%s\t%s", service.utmp, service.def);
			(void) putchar('\n');
		}
	}

	(void) fclose(fp);

	return(0);
}
