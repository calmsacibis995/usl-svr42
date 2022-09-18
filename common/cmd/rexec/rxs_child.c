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

#ident	"@(#)rexec:rxs_child.c	1.4.2.3"
#ident  "$Header: rxs_child.c 1.2 91/06/27 $"

#include <sys/types.h>
#include <sys/byteorder.h>
#include <fcntl.h>
#include <wait.h>
#include <priv.h>
#include <rx.h>
#include <stdio.h>
#include <unistd.h>
#include "rxmsg.h"

/* externally defined routines */

extern	void	log();
extern	void	close_log();
extern	int	rxputm();


/* local routines */

static	int	rxsendclosereq();
static	int	getta();


/* externally defined global variables */

extern	int	State;		/* server state */
extern	int	Net_open;	/* network connection open flag */
extern	int	Net_fd;		/* network fd */
extern	int	Ptty_open;	/* ptty open flag */
extern	int	Ptty_fd;	/* master ptty fd */
extern	int	Pipe_open;	/* error pipe open flag */
extern	int	Pipe_fd;	/* parent side error pipe fd */
extern	int	Slaveptty_fd;	/* slave ptty fd */
extern	char	Slaveptty_name[]; /* slave ptty file name */
extern	int	Childpipe_fd;	/* child side error pipe fd */
extern	long	Flags;		/* service flags */
extern	char	**Svcargv;	/* service arguments */
extern	char	*Svcenvp[];	/* service environment */
extern	char	Logmsg[];


/* locally defined global variables */

int	Child_died;		/* child died flag */
char	Tabuf[RX_MAXTASZ];	/* type ahead buffer */
int	Tasize;			/* bytes of typeahead in buffer */


/* private global variables */

static	int	Stat_loc;	/* wait() status */


/*
 * sigch_hand()
 *
 * This routine handles the SIGCLD signal (signifying child's death)
 *
 */

void
sigch_hand(signo)
int signo;
{
 	log("child: sigch_hand: Child died!");
	Child_died = 1;
	return;
}


/*
 * child_actions()
 *
 * This routine performs the actions of the child process before execve()
 *
 */

void
child_actions()
{
	/* close network fd (fd 0 becomes free at this point) */
	(void) close(Net_fd);

	(void) sprintf(Logmsg, "child: slaveptty_fd = %d", Slaveptty_fd);
	log(Logmsg);

	/* close master side pty */
	
	if (Ptty_open) {
		Ptty_open = 0;
		(void) close(Ptty_fd);
	}

	if (Pipe_open) {
		Pipe_open = 0;
		(void) close(Pipe_fd);
	}

	/* this time, at most 2 file descriptors are open: */
	/* slaveptty_fd and possibly pipe_fd */

	if (setsid() < 0) {
		exit(1);
	}

	/* re-open slave pty to become controlling tty */
	if (open(Slaveptty_name, O_RDWR) != 0) {
		/* should not happen (this is the 1st open since close(Net_fd) */
		exit(1);
	}

	/* close the old (non-controlling) pty */
	(void) close(Slaveptty_fd);

	log("child: closing log");

	/* close log */
	close_log();

	/* make fds 1 and 2 */
	if (dup2(0, 1) < 0)
		exit(1);

	/* if stderr is on a separate stream,      */
	/* pipe_fd >= slaveptty_fd + 2 >= 2 + 2 >= 4 */
	/* and we have to make fd 2 be childpipe_fd  */

	if (Flags & RXF_SEPERR) {
		if (dup2(Childpipe_fd, 2) < 0)
			/* no log at this point */
			exit(1);

		(void) close(Childpipe_fd);
	} else {
		if (dup2(0, 2) < 0)
			/* no log at this point */
			exit(1);
	}

	/* clear any priveleges */
	(void) procprivl(CLRPRV, pm_max(P_ALLPRIVS), 0);

	/* exec command */
	(void) execve(*Svcargv, Svcargv, Svcenvp);

	/* should not get here */
	exit(1);
}


/*
 * child_death()
 *
 * This routine performs the necessary housekeeping upon child's termination
 *
 */

void
child_death()
{
	/* don't do this more than once */
	Child_died = 0;

	log("child: service died and output finished");

	/* child is already dead, wait() will simply provide termination status */
	(void) wait(&Stat_loc);

	(void) sprintf(Logmsg, "child: \tstatus = %x", Stat_loc);
	log(Logmsg);
	log("child: state = RXS_CLOSING");
	State = RXS_CLOSING;

	/* find out the size and read any leftover typeahead */

	Tasize = getta(Tabuf);

	if (Tasize < 0)
		Tasize = 0;

	/* send close_req message or fail/exit */

	if (Net_open)
		(void) rxsendclosereq((long) (WIFEXITED(Stat_loc) ?
				       WEXITSTATUS(Stat_loc) : 0), (long) Tasize);

	/* if no typeahead, close connection */

	if ((Net_open) && (Tasize == 0)) {
		log("child: no typeahead, closing connection");
		Net_open = 0;
		State = RXS_CLOSED;
		(void) close(Net_fd);
	}
}


/*
 * rxsendclosereq()
 *
 * This routine sends a CLOSE_REQ message to the client
 *
 */

int
rxsendclosereq(ret_code, tasize)
long	ret_code;
long	tasize;
{
	struct close_req close_req;

	(void) sprintf(Logmsg,
		"child: closereq: net_fd = %d, ret_code = %ld, tasize = %ld",
		Net_fd, ret_code, tasize);
	log(Logmsg);

	close_req.ret_code = htonl((int) ret_code);
	close_req.tasize = htonl((int) tasize);

	if (rxputm(Net_fd, RXM_CLOSE_REQ, sizeof(struct close_req),
		   (char *) &close_req) < 0) {
		log("child: closereq: rxputm(RXM_CLOSE_REQ) failed");
		return(-1);
	}

	return(0);
}


/*
 * getta()
 *
 * This routine retrieves unused typeahead from the pseudo tty
 *
 */

static int
getta(buf)
char	*buf;
{
	int	tasize = 0, cc, maxbuf = RX_MAXTASZ;

	/* re-open slave ptty */
	Slaveptty_fd = open(Slaveptty_name, O_RDONLY);

	/* and close master */
	(void) close(Ptty_fd);

	(void) fcntl(Slaveptty_fd, F_SETFL, O_NONBLOCK);
	while ((cc = read(Slaveptty_fd, buf+tasize, maxbuf)) > 0) {
		tasize += cc;
		maxbuf -= cc;
	}

	(void) sprintf(Logmsg, "child: getta got %d bytes", tasize);
	log(Logmsg);

	(void) close(Slaveptty_fd);

	return(tasize);
}
