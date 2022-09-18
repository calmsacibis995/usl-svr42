/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:follow.d/common/follow.c	1.2"

/* Minimal follower process. 
 * Sends SIGUSR1 to debugger when subject stops. 
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/procset.h>
#include <sys/procfs.h>
#include <unistd.h>

static int	debugger;
static int	fd = -1;
static char	*filename;


static void
self_exit(int sig)
{
	kill(debugger, SIGUSR1);
	exit(0);
}

static void
myintr(int sig)
{
	if (sig == SIGUSR2)
		exit(0);
}

static int
open_proc()
{
	do {
		errno = 0;
		fd = open( filename, O_RDONLY );
	} while ( errno == EINTR );
	return ( errno == 0 );
}

static int 
err_handle(int err)
{
	int	nfd;

	if (err != EAGAIN)
		return 0;
	
	do {
		errno = 0;
		nfd = open( filename, O_RDONLY);
	} while (errno == EINTR);
	if (errno)
		return 0;
	close(fd);
	fd = nfd;
	return 1;
} 

static int
wait_for()
{
	do {
		errno = 0;
		ioctl( fd, PIOCWSTOP, 0 );
	} while ( errno == EINTR || err_handle(errno));
	return ( errno == 0 );
}

static int
get_status()
{
	struct prstatus	prstat;
	do {
		errno = 0;
		ioctl( fd, PIOCSTATUS, &prstat );
	} while ( errno == EINTR );
	return ( errno == 0 );
}

main(int argc, char **argv)
{
	sigset_t		set, nset;
	struct sigaction	act;

	sigset(SIGINT, SIG_IGN);
	
	if (argc != 3) 
	{
		fprintf(stderr, "%s: invalid argument list.\n",argv[0]);
		exit(1);
	}

	filename = argv[1];
	if (!open_proc())
	{
		fprintf(stderr, "%s: couldn't open %s: %s\n",argv[0], argv[1], strerror(errno));
		exit(2);
	}

	(void)sscanf(argv[2], "%d", &debugger);
	if (debugger == 0) 
	{
		fprintf(stderr, "%s: couldn't find debugger\n", argv[0]);
		exit(1);
	}


	sigemptyset(&set);
	sigemptyset(&nset);
	/* block SIGUSR1 when not suspended */
	sigaddset(&set, SIGUSR1);
	sigprocmask(SIG_BLOCK, &set, 0);

	act.sa_handler = myintr;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;

	sigaction(SIGUSR1, &act, 0);
	sigaction(SIGUSR2, &act, 0);


	for(;;) 
	{
		sigsuspend(&nset);

		(void)wait_for(fd);

		sigsend(P_PID, debugger, SIGUSR1);

		if (!get_status(fd))
		{
			sigset(SIGALRM, (SIG_TYP)self_exit);
			alarm(2);
		}
	}
}
