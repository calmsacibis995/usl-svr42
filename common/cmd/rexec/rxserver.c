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

#ident	"@(#)rexec:rxserver.c	1.5.2.4"
#ident  "$Header: rxserver.c 1.2 91/06/27 $"

/*
 * Usage:
 *		rxserver (from _pmtab, no arguments)
 *
 * MESSAGES:
 *
 * sent by:	CLIENT			SERVER
 *		--------------------------------------
 *		RXM_OPEN_REQ
 *		RXM_OPEN_ARGS
 *		RXM_OPEN_ENVF
 *		RXM_OPEN_ENV
 *		RXM_OPEN_DONE
 *					RXM_OPEN_REPLY
 *		--------------------------------------
 *		RXM_DATA		RXM_DATA
 *					RXM_WRITEACK
 *		--------------------------------------
 *		RXM_SIGNAL
 *					RXM_SIGNALACK
 *		--------------------------------------
 *					RXM_IOCTL
 *		--------------------------------------
 *					RXM_CLOSE_REQ
 *		RXM_CLOSE_REPLY
 *					RXM_CLOSE_TA
 *		--------------------------------------
 *
 */


#include <sys/types.h>
#include <sys/stropts.h>
#include <poll.h>
#include <errno.h>
#include <rx.h>
#include <stdio.h>
#include <unistd.h>
#include "rxmsg.h"


/* externally defined routines */

extern	void	open_log();
extern	void	log();
extern	void	close_log();
extern	void	net_hup();
extern	void	net_msg();
extern	void	ptty_hup();
extern	void	ptty_msg();
extern	void	pipe_hup();
extern	void	pipe_msg();
extern	void	child_death();


/* externally defined global variables */

extern	int	errno;		/* system error code */
extern	int	Net_open;	/* network connection open flag */
extern	int	Net_fd;		/* network fd */
extern	int	Ptty_open;	/* ptty open flag */
extern	int	Ptty_fd;	/* master ptty fd */
extern	int	Pipe_open;	/* error pipe open flag */
extern	int	Pipe_fd;	/* parent side error pipe fd */
extern	int	Child_died;	/* child died flag */
extern	char	Logmsg[];	/* scratch buffer for log messages */
extern	int	Buffer_empty;	/* buffer for ptty empty flag */


/* locally defined global variables */

int	State;		/* server state */


main(argc)
int argc;
{
	int	nfds;			/* number of file descriptors to poll */
	struct pollfd pollfd[3];	/* fds' descriptions for poll() */
	int	pr;			/* poll() return value */
	int	i;			/* general counter */

	open_log(RX_LOGFILE);

	if (argc != 1) {
		log("rxserver: arguments supplied when none expected");
		exit(1);
	}

	log("rxserver: starting");

	Net_open = 1;
	Net_fd = 0;
	Child_died = 0;
	Buffer_empty = 1;

	if( ioctl(Net_fd,I_FIND,"tirdwr") == 0 ){
                log("tirdwr must be pushed on the stream");
                exit(1);
        }

	(void) close(1);
	(void) close(2);

	log("rxserver: closed file descriptors 1 and 2");

	log("rxserver: state = RXS_OPENING");

	State = RXS_OPENING;

	while(Net_open) {

		/*
		 *	determine which fds should be polled
		 */

		nfds = 0;

		if (Net_open) {
			pollfd[nfds].fd = Net_fd;
			pollfd[nfds].events = POLLIN;
			pollfd[nfds].revents = 0;
			nfds++;
			(void) sprintf(Logmsg,
				       "rxserver: poll network, net_fd = %d",
				       Net_fd);
			log(Logmsg);
		}

		if ((Net_open && Ptty_open) || (!Buffer_empty)) {
			pollfd[nfds].fd = Ptty_fd;
			pollfd[nfds].events = 0;
			pollfd[nfds].revents = 0;
			if (Net_open && Ptty_open)
				pollfd[nfds].events |= POLLIN;
			if (!Buffer_empty)
				pollfd[nfds].events |= POLLOUT;
			nfds++;
			(void) sprintf(Logmsg, "rxserver: poll ptty, fd = %d",
				       Ptty_fd);
			log(Logmsg);
		}

		if (Net_open && Pipe_open) {
			pollfd[nfds].fd = Pipe_fd;
			pollfd[nfds].events = POLLIN;
			pollfd[nfds].revents = 0;
			nfds++;
			(void) sprintf(Logmsg, "rxserver: poll pipe, fd = %d",
				       Pipe_fd);
			log(Logmsg);
		}

		/*
		 *	do poll
		 */

		pr = poll(pollfd, nfds, INFTIM);

		/*
		 *	if there was a serious error, log it
		 */

		if ((pr < 0) && (errno != EINTR)) {
			(void) sprintf(Logmsg,
				"rxserver: poll() failed, errno = %d\n", errno);
			log(Logmsg);
			continue;
		}

		/*
		 *	digest new data
		 */

		for (i = 0; i < nfds; i++) {

			/*
			 *	for improved performance
			 */

			if (pollfd[i].revents == 0)
				continue;

			/*
			 *	check network
			 */

			if (Net_open && (pollfd[i].fd == Net_fd))

				if (pollfd[i].revents & POLLIN)

					net_msg();

				else if (pollfd[i].revents & POLLHUP)

					net_hup();

			/*
			 *	check master ptty
			 */

			if (Net_open && Ptty_open && (pollfd[i].fd == Ptty_fd))

				if (pollfd[i].revents & POLLIN)

					ptty_msg();

			/*
			 *	check service
			 */

			if ((!Buffer_empty) && (pollfd[i].fd == Ptty_fd))

				if (pollfd[i].revents & POLLOUT)

					/*
				 	 *	try to write from buffer
				 	 */

					net_try_to_write();

			/*
			 *	check error pipe
			 */

			if (Net_open && Pipe_open && (pollfd[i].fd == Pipe_fd))

				if (pollfd[i].revents & POLLIN)

					pipe_msg();

				else if (pollfd[i].revents & POLLHUP)

					pipe_hup();

		} /* for */

		/*
		 *	Have we read all of the service's data?
		 */

		if (Child_died && !Ptty_open && !Pipe_open)

			child_death();

	} /* while(Net_open) */

	log("rxserver: exiting");
	close_log();

	exit(0);
}
