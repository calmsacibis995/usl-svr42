/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)cs:cs/log.c	1.3.2.3"
#ident  "$Header: log.c 1.2 91/06/26 $"

# include <stdio.h>
# include <unistd.h>
# include <sys/types.h>
# include <dial.h>
# include <time.h>
# include <string.h>
# include <pfmt.h>
# include <locale.h>
# include "global.h"
# include "extern.h"

static	FILE	*Lfp;	/* log file */
static	FILE	*Dfp;	/* debug file */


/*
 * openlog - open log file, sets global file pointer Lfp
 */


void
openlog()
{
	FILE *fp;		/* scratch file pointer for problems */

	Lfp = fopen(LOGFILE, "a+");
	if (Lfp == NULL) {
		fp = fopen("/dev/console", "w");
		if (fp) {
			pfmt(fp, MM_ERROR, ":130:could not open log file: %s\n",
			     LOGFILE);
		}
		exit(1);
	}

/*
 * lock logfile to indicate presence
 */

	if (lockf(fileno(Lfp), F_LOCK, 0) < 0) {
		fp = fopen("/dev/console", "w");
		if (fp) {
			pfmt(fp, MM_ERROR, ":131:could not lock log file: %s\n",
			     LOGFILE);
		}
		exit(1);
	}
}


/*
 * log - put a message into the log file
 *
 *	args:	msg - message to be logged
 */


void
log(msg)
char *msg;
{
	char *timestamp;	/* current time in readable form */
	long clock;		/* current time in seconds */
	char buf[MSGSIZE];	/* scratch buffer */
	struct tm *tms;

	(void) time(&clock);
	tms = localtime(&clock);
	(void) sprintf(buf,"%02d/%02d/%02d %02d:%02d:%02d; %5ld; %s\n",
	    tms->tm_mon+1, tms->tm_mday, tms->tm_year,
	    tms->tm_hour, tms->tm_min, tms->tm_sec, getpid(),msg);
	(void) fprintf(Lfp, buf);
	(void) fflush(Lfp);
}


/*
 * opendebug - open debugging file, sets global file pointer Dfp
 */


void
opendebug()
{
	FILE *fp;	/* scratch file pointer for problems */

	Dfp = fopen(DBGFILE, "a+");
	if (Dfp == NULL) {
		fp = fopen("/dev/console", "w");
		if (fp) {
			pfmt(fp, MM_ERROR, ":132:could not open debug file %s\n",
			     DBGFILE);
		}
		exit(1);
	}
	setbuf(Dfp,NULL);
}


/*
 * debug - put a message into debug file
 *
 *	args:	msg - message to be output
 */

void
debug(msg)
char *msg;
{
	char *timestamp;	/* current time in readable form */
	long clock;		/* current time in seconds */
	char buf[MSGSIZE];	/* scratch buffer */
	struct tm *tms;

	(void) time(&clock);
	tms = localtime(&clock);
	(void) sprintf(buf,"%02d:%02d:%02d; %5ld; %s\n",
	    tms->tm_hour, tms->tm_min, tms->tm_sec, getpid(),msg);
	(void) fprintf(Dfp, buf);
}

/*
 * TEMP FIX:  old debugging did not expect timestamps, etc with
 *            each call to debug, and sometimes only one character
 *	      at a time is printed; this is tuff to read using
 *            the debug above, so uux, uucp, etc use uudebug.
 *
 * uudebug - put a message into debug file WITHOUT ANY GARBAGE
 *
 *	args:	msg - message to be output
 */

void
uudebug(msg)
char *msg;
{
	(void) fprintf(Dfp, msg);
}
