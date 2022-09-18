/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*      Copyright (c) 1989 AT&T */
/*        All Rights Reserved   */

/*      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T     */
/*      The copyright notice above does not evidence any        */
/*      actual or intended publication of such source code.     */

#ident	"@(#)libiaf:common/lib/libiaf/iaf/invoke.c	1.3.1.2"
#ident  "$Header: invoke.c 1.2 91/06/25 $"

#include <errno.h>
#include <iaf.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ulimit.h>
#include <unistd.h>
#include <wait.h>

/*
 * invoke: library routine to invoke an IAF scheme
 *
 * syntax: invoke(fd, cmd);
 *	fd = file descriptor of connection to be authenticated
 *	cmd = command line used to execute the scheme
 *
 * return value:
 *	0 = successfully authenticated
 *	-1 = authentication failed
 */
int
invoke(int fd, char *cmd)
{
	int max_fd, pid, ret;
	char pathname[BUFSIZ] = IAFDIR;
	char **sch_argv;
	void (*sigsave)();
	siginfo_t info;

	if ( (sch_argv = strtoargv(cmd)) == NULL)
		return(-1);

	if ( **sch_argv != '/' ) {
		if (strlen(pathname) + strlen(*sch_argv) + strlen("/scheme")
			>= (size_t) BUFSIZ)
			return(-1);
		(void) strcat(pathname, *sch_argv);
		(void) strcat(pathname, "/scheme");
		*sch_argv = pathname;
	} else {
		if (strlen(*sch_argv) >= (size_t) BUFSIZ)
			return(-1);
		(void) strcpy(pathname, *sch_argv);
	}

	/* invoke first forks */
	switch (pid = fork()) {

	case -1:
		return(-1);
		/* NOTREACHED */
		break;

	case  0:
		if ( fd != 0 ) {
			(void) close(0);
			/* set up stdin	*/
			if (dup(fd) != 0)
				exit(-1);
		}
		if ( (max_fd = ulimit(UL_GDESLIM, (long)0)) == -1 )
			exit(-1);
		while ( --max_fd > 0 )
			(void) close(max_fd);
		/* set up stdout and stderr	*/
		if (dup(0) != 1)
			exit(-1);
		if (dup(0) != 2)
			exit(-1);
		(void) execv(pathname, sch_argv);
		/*
		 * use kill() so parent gets serious failure indicdation
		 */
		(void) kill(getpid(), SIGKILL);
		exit(-1);
		break;

	default:

		sigsave = signal(SIGCLD, SIG_DFL);

		while (waitid((idtype_t)P_PID, (id_t) pid, &info, WEXITED)) {

			if ( errno != EAGAIN )
				return(-1);
		}

		(void) signal(SIGCLD, sigsave);

		if ((info.si_signo == SIGCLD) && (info.si_code == CLD_EXITED))
				return(info._data._proc._pdata._cld._status);

		return(-1);

	}

	/* NOTREACHED */
}

