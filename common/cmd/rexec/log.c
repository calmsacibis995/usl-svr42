/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)rexec:log.c	1.2.2.3"
#ident  "$Header: log.c 1.2 91/06/27 $"

/*
 * logging routines for rxserver and rexec
 *
 */

#include <stdio.h>
#include <unistd.h>

#define	RX_LOGMSG_LEN	128


/* public global variables */

char	Logmsg[RX_LOGMSG_LEN];	/* buffer to use when calling sprintf() */


/* private global variables */

static FILE	*logfp;		/* log file pointer */
static int	log_flag;	/* 0 = do not log, 1 = do log */


/*
 * start_log() turns on logging
 *
 */

void
start_log()
{
#ifdef	LOG
	log_flag = 1;
#endif
}


/*
 * stop_log() turns off logging
 *
 */

void
stop_log()
{
#ifdef	LOG
	log_flag = 0;
#endif
}


/*
 * open_log() opens a log file, starts logging
 *
 */

void
open_log(logfile)
char	*logfile;
{
#ifdef	LOG
	if (logfp != (FILE *) NULL)
		(void) fclose(logfp);

	if ((logfp = fopen(logfile, "a+")) != (FILE *) NULL) {
		start_log();
		setbuf(logfp, NULL);
	}
#endif
}


/*
 * close_log() closes a log file
 *
 */

void
close_log()
{
#ifdef	LOG
	(void) fclose(logfp);
	logfp = (FILE *) NULL;
#endif
}


/*
 * log() adds a string to a file
 *
 */

void
log(str)
char	*str;
{
#ifdef	LOG
	/* possibly add time later */
	if (log_flag)
		(void) fprintf(logfp, "[%ld] %s\n", getpid(), str);
#endif
}
