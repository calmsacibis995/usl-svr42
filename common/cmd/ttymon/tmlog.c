/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ttymon:common/cmd/ttymon/tmlog.c	1.13.12.4"
#ident "$Header: tmlog.c 1.2 91/04/18 $"

/*
 * error/logging/cleanup functions for ttymon.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <priv.h>
#include <pfmt.h>
#include <errno.h>
#include <sys/termios.h>
#include <sys/stream.h>
#include <sys/tp.h>

#include "ttymon.h"
#include "tmstruct.h"
#include "tmextern.h"

static	FILE	*logfp = NULL;		/* for log file		*/

# ifdef DEBUG
static	FILE	*debugfp = NULL;	/* for debug file	*/
# endif

extern	time_t	time();
extern	char	*ctime();

const char
	datefmt[] = "%a %b %e %H:%M:%S %Y",
	datefmtid[] = ":638";

/*
 * Procedure:	  openlog
 *
 * Restrictions:
                 open(2): none
             	 fcntl(2): none
                 pfmt: none
                 strerror: none
                 fclose: none
*/

void
openlog()
{
	int	fd, ret;
	char	logfile[BUFSIZ];
	extern	char	*Tag;

	/* the log file resides in /var/saf/pmtag/ */
	(void)strcpy(logfile, LOGDIR);
	(void)strcat(logfile, Tag);
	(void)strcat(logfile, "/");
	(void)strcat(logfile, LOGFILE);
	logfp = NULL;
	(void)close(0);
	/*
	 * P_MACWRITE, P_DACWRITE - override read-only bits
	 * P_MACREAD, P_DACREAD   - ensure search access
	 */
	fd = open(logfile, O_WRONLY|O_CREAT|O_APPEND,0444);
	if (fd != -1)
		if ((ret = fcntl(fd, F_DUPFD, 3)) == 3) {
			/* set close-on-exec flag */
			if (fcntl(ret, F_SETFD, 1) == 0) {
				logfp = fdopen(ret, "a+");
			}
		}

	if (!logfp) {
		if ((fd = open(CONSOLE, O_WRONLY|O_NOCTTY)) != -1) {
			FILE *sfd = fdopen(fd, "w");
			if (sfd){
				pfmt(sfd, MM_HALT, ":148:Cannot create %s: %s\n",
					logfile, strerror(errno));
				fclose(sfd);
			}
			else
				close(fd);
		}
		exit(1);
	}
	log(MM_NOGET|MM_NOSTD, " ");
	log(MM_INFO, ":637:********** ttymon starting **********");

#ifdef	DEBUG
	(void)dlog("fd(log)\t = %d",fileno(logfp));
#endif
}
/*
 * Procedure:	  log
 *
 * Restrictions:
		 cftime: none
		 gettxt: none
		 pfmt: none
		 putc: none
		 fflush: none
		 isatty: none
		 open(2): none
		 fclose: none
*/

/*
 * log(msg) - put a message into the log file
 *	    - if logfp is NULL, write message to stderr or CONSOLE
 */

void
log(sev, msg, a1, a2, a3, a4, a5, a6, a7, a8, a9)
int sev;
char *msg, *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9;
{
	char timestamp[30];	/* current time in readable form */
	long clock;		/* current time in seconds */
	int	fd;
	FILE	*sfd;
	extern	time_t	time();

	if (logfp) {
		(void) time(&clock);
		cftime(timestamp, gettxt(datefmtid, datefmt), &clock);
		(void) pfmt(logfp, MM_NOSTD, ":866:%s; %ld; ", timestamp, getpid());
		(void) pfmt(logfp, sev, msg, a1, a2, a3, a4, a5, a6, a7, a8, a9);
		(void) putc('\n', logfp);
		(void) fflush(logfp);
	}
	else if (isatty(2)) {
		(void) pfmt(stderr, sev, msg, a1, a2, a3, a4, a5, a6, a7, a8, a9);
		(void) putc('\n', stderr);
		(void) fflush(stderr);
	}
	else {
	     if ((fd = open(CONSOLE, O_WRONLY|O_NOCTTY)) != -1 &&
	        (sfd = fdopen(fd, "w")) != NULL) {
		(void) pfmt(sfd, sev, msg, a1, a2, a3, a4, a5, a6, a7, a8, a9);
		(void) putc('\n', sfd);
		(void) fclose(sfd);
		(void) close(fd);
	     } else
		 exit(1);
	}
	
}

void
logexit(exitcode, msg, a1, a2, a3, a4, a5, a6, a7, a8, a9)
int exitcode;
char *msg, *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9;
{
	if (msg) 
		log(MM_HALT, msg, a1, a2, a3, a4, a5, a6, a7, a8, a9);
	log(MM_HALT, ":639:********** ttymon exiting ***********");
	exit(exitcode);
}


# ifdef DEBUG

/*
 * Procedure:	  dlog
 *
 * Restrictions:
                 cftime: none
                 gettxt: none
                 fprintf: none
                 putc: none
                 isatty: none
                 fflush: none
             	 open(2): none
             	 pfmt:none
							 fclose: none
*/

/*
 * dlog(msg) - put a debugging message into the log file
 *	     - if logfp is NULL, write message to stderr or CONSOLE
 */

dlog(msg, a1, a2, a3, a4, a5, a6, a7, a8, a9)
char *msg, *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9;
{
	char timestamp[30];	/* current time in readable form */
	long clock;		/* current time in seconds */
	extern	time_t	time();
	extern	int 	cftime();
	int	fd;
	FILE *sfd;

	if (logfp) {
		(void) time(&clock);
		cftime(timestamp, gettxt(datefmtid, datefmt), &clock);
		(void) fprintf(logfp,"%s; %ld; ", timestamp, getpid());
		(void) fprintf(logfp, msg, a1, a2, a3, a4, a5, a6, a7, a8, a9);
		(void) putc('\n', logfp);
		(void) fflush(logfp);
	}
	else if (isatty(2)) {
		(void) fprintf(stderr, msg, a1, a2, a3, a4, a5, a6, a7, a8, a9);
		(void) putc('\n', stderr);
	}
	else {
	     if ((fd = open(CONSOLE, O_WRONLY|O_NOCTTY)) != -1 &&
	        (sfd = fdopen(fd, "w")) != NULL) {
		(void) pfmt(sfd, MM_INFO|MM_NOGET, msg, a1, a2, a3, a4, a5, a6, a7, a8, a9);
		(void) putc('\n', sfd);
		(void) fclose(sfd);
	     } else
		 exit(1);
	}
	
}

void
dlogexit(exitcode, msg, a1, a2, a3, a4, a5, a6, a7, a8, a9)
int exitcode;
char *msg, *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9;
{
	if (msg) 
		dlog(msg, a1, a2, a3, a4, a5, a6, a7, a8, a9);
	dlog("********** ttymon exiting ***********");
	exit(exitcode);
}

/*
 * Procedure:	  opendebug
 *
 * Restrictions:
		 fopen: none
		 open(2): none
		 fcntl(2): none
*/

/*
 * opendebug - open debugging file, sets global file pointer debugfp
 * 	arg:   getty - if TRUE, ttymon is in getty_mode and use a different
 *		       debug file
 */

void
opendebug(getty_mode)
int	getty_mode;
{
	int  fd, ret;
	char	debugfile[BUFSIZ];
	extern	char	*Tag;

	if (!getty_mode) {
		(void)strcpy(debugfile, LOGDIR);
		(void)strcat(debugfile, Tag);
		(void)strcat(debugfile, "/");
		(void)strcat(debugfile, DBGFILE);
		if ((debugfp = fopen(debugfile, "a+")) == NULL)
			dlogexit(1,"open debug file failed");
	}
	else {
		if ((fd = open(EX_DBG, O_WRONLY|O_APPEND|O_CREAT)) < 0)
			dlogexit(1, "open %s failed, errno = %d", EX_DBG, errno);

		if (fd >= 3) 
			ret = fd;
		else {
			if ((ret = fcntl(fd, F_DUPFD, 3)) < 0)
				dlogexit(1, "F_DUPFD fcntl failed, errno = %d", errno);
		}
		if ((debugfp = fdopen(ret, "a+")) == NULL)
			dlogexit(1, "fdopen failed, errno = %d", errno);
		if (ret != fd)
			(void)close(fd);
	}
	/* set close-on-exec flag */
	if (fcntl(fileno(debugfp), F_SETFD, 1) == -1)
		dlogexit(1, "F_SETFD fcntl failed, errno = %d", errno);
}

/*
 * Procedure:	  debug
 *
 * Restrictions:
                 ctime: none
                 sprintf: none
                 fprintf: none
                 fflush: none
fflush: write
*/

/*
 * debug(msg) - put a message into debug file
 */

void
debug(msg)
char *msg;
{
	char *timestamp;	/* current time in readable form */
	long clock;		/* current time in seconds */
	char buf[BUFSIZ];	/* scratch buffer */

	(void) time(&clock);
	timestamp = ctime(&clock);
	*(strchr(timestamp, '\n')) = '\0';
	(void) sprintf(buf, "%s; %ld; %s\n", timestamp, getpid(), msg);
	(void) fprintf(debugfp, buf);
	(void) fflush(debugfp);
}
#endif
