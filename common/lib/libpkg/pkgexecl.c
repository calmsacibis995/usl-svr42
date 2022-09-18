/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libpkg:common/lib/libpkg/pkgexecl.c	1.8.10.6"
#ident "$Header: $"

#include <stdio.h>
#include <string.h>
#include <varargs.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pkglocs.h>
#include <limits.h>

extern char	**environ;
extern int	errno;

extern	char	*pkginst;
extern	char	*prog;

extern void	exit(),
		progerr();
extern pid_t	fork();
extern pid_t	waitpid();
extern int	execve();

#define MAXARGS	16
#define	MAXTRY	5

/*VARARGS*/
int
pkgexecl(va_alist)
va_dcl
{
	va_list ap;
	char	*pt, *arg[MAXARGS+1];
	char	*filein, *fileout, *filerr;
	int	n, status, upper, lower;
	pid_t	pid;
	void	(*func)();
	int	try = 0;

	va_start(ap);
	filein = va_arg(ap, char *);
	fileout = va_arg(ap, char *);
	filerr = va_arg(ap, char *);
	if(!strcmp(filerr, "NULL"))
		filerr = NULL;

	n = 0;
	while(pt = va_arg(ap, char *)) {
		arg[n++] = pt;
	}
	arg[n] = NULL;
	va_end(ap);

	while((pid  = fork()) == -1) {
		if( errno == EAGAIN || errno == ENOMEM) {
			if(++try < MAXTRY) {
				(void) fprintf(stderr,
					"pkgexev: fork failed, pid=%d retrying fork()\n");
				(void) sleep(4);
				continue;
			}
			progerr("bad fork(), errno=%d", errno);
			return(-1);
		}
	}
	if(pid) {
		func = signal(SIGINT, SIG_IGN);
		n = waitpid(pid, &status, 0);
		if(n != pid) {
			progerr("wait for %d failed, pid=%d errno=%d", 
				pid, n, errno);
			return(-1);
		}
		upper = (status>>8) & 0177;
		lower = status & 0177;
		(void) signal(SIGINT, func);
		return(lower ? (-1) : upper);
	}
	/* CHILD */
	for (status = 3; status < 101; status++)
		close (status);

	/*
	 * If any of the passed in file args are not null, reopen stream
	 * to that file.  If filerr is set, then logmode is in effect.
	 * In this case filerr is the name of the error logfile.
	 */
	if(filein)
		(void) freopen(filein, "r", stdin);
	if(fileout)
		(void) freopen(fileout, "w", stdout);
	if(filerr) {
		(void) freopen(filerr, "a", stderr);
	}
	(void) execve(arg[0], arg, environ);
	progerr("exec of %s failed, errno=%d", arg[0], errno);
	exit(99);
	/*NOTREACHED*/
}
