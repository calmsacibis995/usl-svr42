/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/smtplog.c	1.7.2.2"
#ident "@(#)smtplog.c	1.7 'attmail mail(1) command'"
/*
 *  smtplog.c -- Routines to do smtp logging.
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <tiuser.h>
#include <stropts.h>
#include <unistd.h>
#include <errno.h>
#include "xmail.h"
#include "smtp.h"

#ifndef	DEBUG
#define	SMTPDLOG "%s/LOG"
#else
#define	SMTPDLOG "/dev/tty"
#endif

extern int chmod proto((char *, int));

int	lastclosehour = 0;	/* hour we last closed and opened the log file */
extern char *progname;		/* Needed for logging */

void smtplog(msg)
char *msg;
{
	static FILE *logfile = NULL;
	char	logname[128];
	char	timestamp[40];
	struct	tm *now;
	long	seconds;

	(void) time(&seconds); 
	now = localtime(&seconds);
	sprintf(timestamp, "%02d/%02d %02d:%02d:%02d",
		now->tm_mon+1, now->tm_mday,
		now->tm_hour, now->tm_min, now->tm_sec);
	if (now->tm_hour != lastclosehour) {
		lastclosehour = now->tm_hour;
		if (logfile != NULL)
			fclose(logfile);
		logfile = NULL;
	}
	if (logfile == NULL) {
		(void) sprintf(logname, SMTPDLOG, SMTPQROOT);
		logfile = fopen(logname, "a");
		chmod(logname, 0666);
	}

	fprintf(logfile, "%s %-9.9s  %s\n", timestamp, progname, msg);
	fflush(logfile);
}

#ifdef TLI
void t_log(s)
char *s;
{
	register char *c;
	char buf[BUFSIZ];
	extern int t_nerr, t_errno;
	extern char *t_errlist[];

	if (t_errno <= t_nerr)
		c = t_errlist[t_errno];
	else
		c = "Unknown error";
	if (strlen(s))
		(void) sprintf(buf, "%s: %s", s, c);
	else
		(void) strcpy(buf, c);
	if (t_errno == TSYSERR) {
		(void) strcat(buf, ": ");
		(void) strcat(buf, strerror(errno));
	}
	smtplog(buf);
}
#endif
