/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mailx:popen.c	1.5.2.3"
#ident "@(#)popen.c	1.10 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * npopen() and npclose()
 *
 * stolen from C library, modified to use SHELL variable
 */


#include "rcv.h"

#define	tst(a,b) (*mode == 'r'? (b) : (a))
#define	RDR	0
#define	WTR	1

#ifdef preSVr4
extern FILE *fdopen();
extern int execlp(), fork(), pipe(), close(), fcntl();
#endif
static pid_t popen_pid[20];

FILE *
npopen(cmd, mode)
const char *cmd, *mode;
{
	int	p[2];
	register pid_t pid;
	register int myside, yourside;
	char *Shell;

	if ((Shell = value("SHELL")) == NULL || *Shell=='\0')
#ifdef preSVr4
		Shell = "/bin/sh";
#else
		Shell = "/usr/bin/sh";
#endif
	if(pipe(p) < 0)
		return(NULL);
	myside = tst(p[WTR], p[RDR]);
	yourside = tst(p[RDR], p[WTR]);
	if((pid = fork()) == 0) {
		/* myside and yourside reverse roles in child */
		int	stdio;

		stdio = tst(0, 1);
		(void) close(myside);
		(void) close(stdio);
		(void) fcntl(yourside, 0, stdio);
		(void) close(yourside);
		(void) execlp(Shell, Shell, "-c", cmd, (char *)0);
		pfmt(stderr, MM_ERROR, badexec, cmd, strerror(errno));
		fflush(stderr);
		_exit(1);
	}
	if(pid == (pid_t)-1)
		return(NULL);
	popen_pid[myside] = pid;
	(void) close(yourside);
	return(fdopen(myside, mode));
}

int
npclose(ptr)
FILE	*ptr;
{
	register int f;
	register pid_t r;
	int status;
	void (*istat)(), (*qstat)();

	f = fileno(ptr);
	(void) fclose(ptr);
	istat = sigset(SIGINT, SIG_IGN);
	qstat = sigset(SIGQUIT, SIG_IGN);
	while((r = wait(&status)) != popen_pid[f] && r != (pid_t)-1)
		;
	if(r == (pid_t)-1)
		status = -1;
	(void) sigset(SIGINT, istat);
	(void) sigset(SIGQUIT, qstat);
	return(status);
}
