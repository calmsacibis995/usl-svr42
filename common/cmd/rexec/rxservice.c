/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)rexec:rxservice.c	1.5.2.6"
#ident  "$Header: rxservice.c 1.3 91/06/28 $"

/*
 * rxservice - add and remove REXEC services
 *
 * Synopsis
 *
 *	rxservice -a servicename [-d description] [-u] servicedef
 *	rxservice -r servicename ...
 *	rxservice -l
 *
 *	-a add an REXEC service
 *	servicename = name of the REXEC serice
 *	-d service description follows
 *	description = text describing the REXEC service
 *	-u make a utmp entry upon invocation of service
 *	servicedef = REXEC service definition
 *	-r remove one or more REXEC services
 *	-l list all defined REXEC services
 *
 */

#include <stdio.h>
#include <locale.h>
#include <unistd.h>
#include <pfmt.h>
#include <sys/types.h>
#include <mac.h>
#include <sys/stat.h>
#include <rx.h>
#include <string.h>
#include <stdlib.h>
#include "rxsvcent.h"

#define	OPTIONS	"a:d:url"
#define	ACT_ADD		1
#define	ACT_REMOVE	2
#define	ACT_LIST	3


/* externally defined routines */

extern	int	skipcmnt();
extern	int	getsvcent();
extern	int	putsvcent();


static void
add(nsvc)
RX_SERVICE	*nsvc;
{
	FILE	*fp;		/* service file pointer */
	RX_SERVICE	svc;	/* service description */
	int	eof = 0;	/* eof flag */

	/* service definition must start with a full pathname or macro */
	if ((nsvc->def[0] != '/') && (nsvc->def[0] != '%')) {
		(void) pfmt(stderr, MM_ERROR,
			    ":195:Invalid service definition\n");
		exit(1);
	}

	if ((fp = fopen(RX_SVCFILE, "r+")) == (FILE *) NULL) {
		(void) pfmt(stderr, MM_ERROR, ":196:Cannot open services file\n");
		exit(1);
	}

	while (!eof) {
		if (skipcmnt(fp) < 0) {
			eof++;
			continue;
		}
		if (getsvcent(fp, &svc, 0) < 0) {
			eof++;
			continue;
		}
		if (strcmp(svc.name, nsvc->name) == 0) {
			(void) pfmt(stderr, MM_ERROR,
				    ":197:The service is already defined\n");
			exit(1);
		}
	}

	if (putsvcent(fp, nsvc) < 0) {
		(void) pfmt(stderr, MM_ERROR,
			    ":198:The services file could not be updated\n");
		exit(1);
	}

	(void) fclose(fp);
}


static void
filecopy(fp1, fp2, offset, length)
FILE 	*fp1, *fp2;
long	offset, length;
{
	long	oldoffset;	/* place to save offset for fp1 */
	char	*buf;		/* buffer for copy */

	oldoffset = ftell(fp1);

	(void) fseek(fp1, offset, SEEK_SET);

	buf = malloc((unsigned int) length);

	(void) fread(buf, length, 1, fp1);
	(void) fwrite(buf, length, 1, fp2);

	free(buf);

	(void) fseek(fp1, oldoffset, SEEK_SET);
}


static void
rem(namev, namec)
char	*namev[];
int	namec;
{
	FILE	*fp, *fp2;	/* service file and tmp file pointers */
	RX_SERVICE	svc;	/* service description */
	int	i;		/* tmp counter */
	int	mark;		/* mark for deletion flag */
	long	lastpos, curpos;/* positions in the file */
	int	eof = 0;	/* eof flag */
	int	*todov;		/* "not done" flags */
	level_t	lvlno;		/* file level number */
	int	tmpfd;		/* temporary fd */

	if ((fp = fopen(RX_SVCFILE, "r")) == (FILE *) NULL) {
		(void) pfmt(stderr, MM_ERROR, ":196:Cannot open services file\n");
		exit(1);
	}

	/* create temp file */
	if ((tmpfd = creat(RX_TMPSVCFILE, 0644)) < 0) {
		(void) pfmt(stderr, MM_ERROR, ":199:Cannot create temp file\n");
		(void) fclose(fp);
		exit(1);
	}
	(void) close(tmpfd);
	(void) lvlin("SYS_PUBLIC", &lvlno);
	(void) lvlfile(RX_TMPSVCFILE, MAC_SET, &lvlno);

	/* open temp file */
	if ((fp2 = fopen(RX_TMPSVCFILE, "w")) == (FILE *) NULL) {
		(void) pfmt(stderr, MM_ERROR, ":199:Cannot create temp file\n");
		(void) fclose(fp);
		exit(1);
	}

	lastpos = 0;

	todov = malloc(sizeof(int) * namec);
	for (i = 0; i < namec; i++)
		todov[i] = 1;

	while(!eof) {
		if (skipcmnt(fp) < 0) {
			/* comment is the last thing in the file */
			eof++;
			curpos = ftell(fp);
			continue;
		}
		curpos = ftell(fp);
		if (getsvcent(fp, &svc, 0) < 0) {
			eof++;
			continue;
		}
		mark = 0;
		for (i = 0; i < namec; i++)
			/* if this name is to be deleted, mark it */
			if (strcmp(svc.name, namev[i]) == 0) {
				mark++;
				todov[i] = 0;
			}
		if (mark) {
			/* catch up - copy everything in the file from the     */
			/* last deletion (or beginning) up to (but not         */
			/* including) the current entry which is to be deleted */

			if (curpos > lastpos)
				filecopy(fp, fp2, lastpos, (curpos - lastpos));

			lastpos = ftell(fp);
		}
	}

	if (curpos > lastpos)
		filecopy(fp, fp2, lastpos, (curpos - lastpos));

	(void) fclose(fp);
	(void) fclose(fp2);
	(void) rename(RX_TMPSVCFILE, RX_SVCFILE);

	mark = 0;
	for(i = 0; i < namec; i++)
		if (todov[i]) {
			(void) pfmt(stderr, MM_ERROR,
				    ":200:Service not found - %s\n", namev[i]);
			mark++;
		}
	if (mark)
		exit(1);
}


static void
list()
{
	FILE	*fp;		/* service file pointer */
	struct stat statbuf;	/* stat structure */

	if ((fp = fopen(RX_SVCFILE, "r")) == (FILE *) NULL) {
		(void) pfmt(stderr, MM_ERROR, ":196:Cannot open services file\n");
		exit(1);
	}

	if (fstat(fileno(fp), &statbuf) < 0)
		exit(1);

	filecopy(fp, stdout, 0, statbuf.st_size);

	(void) fclose(fp);
}


int
main(argc, argv)
int	argc;
char	*argv[];
{
	extern char	*optarg;	/* getopt() option argument */
	extern int	optind;		/* getopt() options index */
	int	c;			/* options character */
	int	opterror = 0;		/* usage error flag */
	int	action = 0;		/* command to run */
	RX_SERVICE	service;	/* service entry */

	/* set up error message handling */

	(void) setlocale(LC_ALL, "");
	(void) setlabel("UX:rxservice");
	(void) setcat("uxnsu");

	(void) strcpy(service.descr, "");
	(void) strcpy(service.utmp, "-");

	while((c = getopt(argc, argv, OPTIONS)) != EOF)
		switch(c) {

		case 'a':
			if (action)
				opterror++;
			else {
				int	i;
				int	notalnum = 0;

				action = ACT_ADD;
				if (strlen(optarg) > RX_MAXSVCSZ) {
					(void) pfmt(stderr, MM_ERROR,
						    ":201:Service name too long\n");
					exit(1);
				}
				for (i = 0; optarg[i] != NULL; i++)
					notalnum += !isgraph(optarg[i]);
				if (notalnum) {
					(void) pfmt(stderr, MM_ERROR,
						    ":205:Invalid service name\n");
					exit(1);
				}
				(void) strncpy(service.name, optarg, RX_MAXSVCSZ);
			}
			break;

		case 'd':
			if (action != ACT_ADD)
				opterror++;
			else {
				if (strlen(optarg) > RX_MAXSVCDESCR) {
					(void) pfmt(stderr, MM_WARNING,
						    ":202:Service description too long - truncating to %d characters\n", RX_MAXSVCDESCR);
					service.descr[RX_MAXSVCDESCR] = '\0';
				}
				(void) strncpy(service.descr, optarg, RX_MAXSVCDESCR);
			}
			break;

		case 'u':
			if (action != ACT_ADD)
				opterror++;
			else
				(void) strcpy(service.utmp, "u");
			break;

		case 'r':
			if (action)
				opterror++;
			else
				action = ACT_REMOVE;
			break;

		case 'l':
			if (action)
				opterror++;
			else
				action = ACT_LIST;
			break;

		case '?':
			opterror++;
			break;
		}

	if (!opterror)
		switch(action) {

		case ACT_ADD:
			/* should get one extra argument - service definition */
			if (optind != (argc - 1))
				opterror++;
			else {
				if (strlen(argv[optind]) > RX_MAXSVCDEF) {
					(void) pfmt(stderr, MM_ERROR,
						    ":203:Service definition too long\n");
					exit(1);
				}
				(void) strncpy(service.def, argv[optind],
					       RX_MAXSVCDEF);
				add(&service);
			}
			break;

		case ACT_REMOVE:
			/* should get one or more extra arguments - serice name(s) */
			if (optind >= argc)
				opterror++;
			else
				rem(&argv[optind], (argc - optind));
			break;

		case ACT_LIST:
			/* should not get any more arguments */
			if (optind < argc)
				opterror++;
			else
				list();
			break;

		default:
			opterror++;
			break;
		}

	if (opterror) {
		(void) pfmt(stderr, MM_ERROR, ":73:Syntax\n");
		(void) pfmt(stderr, MM_ACTION, ":204:\t%s: Usage:\n\t\t%s -a servicename [-d description] [-u] servicedef\n\t\t%s -r servicename ...\n\t\t%s -l\n", argv[0], argv[0], argv[0], argv[0]);
		exit(1);
	}

	exit(0);
}
