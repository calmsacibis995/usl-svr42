/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)rexec:rexec.c	1.8.2.4"
#ident  "$Header: rexec.c 1.2 91/06/27 $"

/*
 * Usage:
 *		rexec host service [arguments]
 *		service host [arguments]
 *		(where service is linked to rexec)
 *
 */


#include <sys/types.h>
#include <sys/stat.h>
#include <locale.h>
#include <pfmt.h>
#include <string.h>
#include <stropts.h>
#include <errno.h>
#include <poll.h>
#include <sys/termios.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <rx.h>
#include <cs.h>

/* externally defined routines */

extern	void	rxperror();


/* externally defined global variables */

extern	char	Rxenvfile[];
extern	long	Rx_errno;
extern	int	Rx_cserrno;


/* private global variables */

static	int		Cnum;		/* connection number/handle */
static	struct stat	Statout,	/* file status buffers */
			Staterr;
static	int		Try_to_write;
static	int		Signal_caught;

#define	OPTIONS	"cmue:s:"

#define	RXPORTVAR	"RXPORT"
#define	ENVDELIM	'='
#define	RXPORTDELIM	','
#define	RXPORTALL	"*"


static void
sig_hand(signo)
int	signo;		/* signal number */
{
	/*
	 *	assuming only the allowed 4 signals invoke this routine
	 */

	Signal_caught = signo;
	(void) signal(signo, sig_hand);

	return;
}


/*
 *	restore_and_exit() -	restores the terminal state and exits
 */

static void
restore_and_exit(rc, tios)
int	rc;	/* return code */
struct termios	*tios;	/* terminal state */
{
	if (tios != (struct termios *) NULL)
		ioctl(1, TCSETS, tios);
	exit(rc);
}


int
main(argc, argv, envp)
int argc;
char *argv[];
char *envp[];
{
	extern char	*optarg;	/* getopt() option argument */
	extern int	optind;		/* getopt() options index */
	int	c;			/* getopt() option character */
	int	opterror = 0;		/* options error flag */
	char	*machine;		/* requested rexec host */
	char	*service;		/* requested rexec service */
	long	flags = 0;		/* rexec options */
	int	Dflag = 0;		/* debugging flag */
	struct pollfd	pollfd[2];	/* fd array for poll() */
	int	nfds;			/* number of fds to poll */
	int	poll_stdin = 1;		/* poll stdin fd flag */
	int	stdin_eof = 0;		/* stdin eof flag */
	int	stdin_tty = isatty(0);	/* input is coming from a tty */
	int	pfd;			/* polled fd number */
	char	databuf[RX_MAXMSGSZ];	/* buffer to read stdin */
	long	datalen;		/* length of stdin input */
	int	saved_ret_code;		/* service ret code */
	char	tabuf[RX_MAXTASZ];	/* buffer for returned typeahead chars */
	int	netfd;			/* network connection fd */
	int	i;			/* counter */
	char	**myenvp = envp;	/* private envp pointer */
	int	renvc = 0;		/* environment count */
	char	*renvp[RX_MAXENVS];	/* environment string pointer buffer */
	char	*rxportp;		/* pointer to RXPORT value */
	char	*rxportendp;		/* pointer to end of RXPORT value */
	char	*rxportvar;		/* pointer to a name in RXPORT */
	char	*rxportval;		/* environment value of name */
	char	*rxenv;			/* buffer for env. name=val string */
	struct termios	saved_termios;	/* saved terminal state */
	struct termios	*termiosp = &saved_termios;

	(void) setlocale(LC_ALL, "");
	(void) setlabel("UX:rexec");
	(void) setcat("uxnsu");

	Rxenvfile[0] = '\0';

	if ((service = strrchr(argv[0], '/')) == NULL)

		/*
		 *	argv0 contains no '/'s
		 */

		if (strcmp(argv[0], "rexec") == 0)

			/*
			 *	argv0 is "rexec"
			 */

			service = NULL;
		else

			/*
			 *	argv0 is a service name
			 */

			service = argv[0];
	else {
		/*
		 *	argv0 contains '/'s, skip last one
		 */

		service++;
		if (strcmp(service, "rexec") == 0)
			/* argv0 is foo/rexec */
			service = NULL;
	}

	if (service != NULL)
		if (strcmp(service, "debug") == 0) {
			Dflag = 1;
			service = NULL;
			flags |= RXF_DEBUG;
		}

	while ((c = getopt(argc, argv, OPTIONS)) != EOF) {

		switch(c) {

		case 'c': /* accept for RDS UNIX compatibility, but ignore */
		case 'm': /* accept for RDS UNIX compatibility, but ignore */
		case 'u': /* accept for RDS UNIX compatibility, but ignore */
			break;

		case 'e': /* environment file name follows */
			if (strlen(optarg) >= (unsigned int) RX_MAXENVFNAME) {
				(void) pfmt(stderr, MM_ERROR,
					    ":161:Environment file name too long\n");
				exit(1);
			}
			(void) strncpy(Rxenvfile, optarg, RX_MAXENVFNAME);
			break;

		case 's': /* service name follows (RDS UNIX usage) */
			service = optarg;
			break;

		case '?': /* unknown option */
			opterror++;
			break;
		}
	}

	/*
	 *	get host name
	 */

	if (optind == argc)
		opterror++;
	else
		machine = argv[optind++];

	/*
	 *	get service name, if not supplied with argv[0] or -s
	 */

	if (service == NULL)
		if (optind == argc)
			opterror++;
		else
			service = argv[optind++];

	if (opterror) {
		(void) pfmt(stderr, MM_ERROR, ":162:Usage\n");
		(void) pfmt(stderr, MM_ACTION, ":163:rexec host service [arguments]\n");
		(void) pfmt(stderr, MM_ACTION, ":164:service host [arguments]\n");
		exit(1);
	}

	/*
	 *	is stdout == stderr?
	 */

	(void) fstat(fileno(stdout), &Statout);
	(void) fstat(fileno(stderr), &Staterr);
	if ((Statout.st_ino != Staterr.st_ino) ||
	    (Statout.st_dev != Staterr.st_dev) ||
	    (Statout.st_rdev != Statout.st_rdev)) {
		Printf0("rexec: stderr is different from stdout\n");
		flags |= RXF_SEPERR;
	}

	/*
	 *	is stdout a terminal?
	 */

	if (isatty(fileno(stdout)) != 0) {
		Printf0("rexec: stdout is going to a terminal\n");
		flags |= RXF_STDOUTTERM;
	}

	/*
	 *	get explicitly RXPORTed variables
	 */

	renvc = 0;

	if ((rxportp = getenv(RXPORTVAR)) != NULL) {
		/*
		 *	we can't alter the RXPORT variable since it itself
		 *	may need to be exported, so we dup it.
		 */

		rxportp = strdup(rxportp);
		rxportendp = rxportp + strlen(rxportp);

		while(rxportp <= rxportendp) {
			/*
			 *	set the beginning of the variable name
			 */

			rxportvar = rxportp;

			/*
			 *	get the variable name
			 */

			while((*rxportp != '\0') && (*rxportp != RXPORTDELIM))
				rxportp++;

			/*
			 *	change the delimiter to a NULL
			 */

			*rxportp = '\0';

			/*
			 *	RXPORT everything?
			 */

			if (strcmp(rxportvar, RXPORTALL) == 0) {
				/*
				 *	copy all the variables in envp
				 */

				for(myenvp = envp; *myenvp != NULL; myenvp++)
					renvp[renvc++] = strdup(*myenvp);
			} else {
				/*
				 *	get the variable value
				 */

				if ((rxportval = getenv(rxportvar)) != NULL) {
					/*
					 *	set up an environment string
					 *	var = val.  The +2 is for
					 *	'=' and trailing '\0'.
					 */

					rxenv = malloc(strlen(rxportvar) +
						       strlen(rxportval) + 2);
					sprintf(rxenv, "%s%c%s", rxportvar,
						ENVDELIM, rxportval);
					renvp[renvc++] = rxenv;
				} /* if */
			} /* if else */

			rxportp++;
		}
	}

	renvp[renvc++] = NULL;

	if ((Cnum = rexecve(machine, service, &argv[optind], renvp, flags)) < 0) {
		rxperror(Rx_errno);
		if (Rx_errno == RXE_CONNPROB)
			cs_perror("", Rx_cserrno);
		exit(1);
	}

	Printf0("rexec: connection established\n");

	Signal_caught = 0;

	(void) signal(SIGHUP, sig_hand);
	(void) signal(SIGINT, sig_hand);
	(void) signal(SIGPIPE, sig_hand);
	(void) signal(SIGQUIT, sig_hand);

	if (ioctl(1, TCGETS, termiosp) < 0)
		termiosp = (struct termios *) NULL;

	while((netfd = rx_fd(Cnum)) >= 0) {

		/*
		 *	deal with signals now
		 */

		if (Signal_caught) {
			(void) rx_signal(Cnum, Signal_caught);
			Signal_caught = 0;
		}

		/*
		 *	determine which fds should be polled
		 */

		nfds = 0;

		/*
		 *	always polling network
		 */

		pollfd[nfds].fd = netfd;
		pollfd[nfds].events = POLLIN;
		pollfd[nfds].revents = 0;
		nfds++;

		if ((poll_stdin) && (!stdin_eof)) {
			pollfd[nfds].fd = fileno(stdin);
			pollfd[nfds].events = POLLIN;
			pollfd[nfds].revents = 0;
			nfds++;
		}

		Printf0("rexec: polling\n");

		/*
		 *	do poll
		 */

		if (poll(pollfd, nfds, INFTIM) < 0) {

			if (errno != EINTR)
				Printf1("rexec: poll error, errno = %d\n",
					errno);

			/*
			 *	revents field is invalid due to poll() failure
			 */

			continue;
		}

		for (pfd = 0; pfd < nfds; pfd++) {

			/*
			 *	for better performance
			 */

			if (pollfd[pfd].revents == 0)
				continue;

			if ((pollfd[pfd].fd == netfd) &&
			    (pollfd[pfd].revents & POLLIN)) {

				long	msg_type;	/* message type */
				int	ret_code;	/* return code */
				long	talen;		/* amount of typeahead */

				Printf0("rexec: got data on netfd\n");

				if ((talen = rx_proc_msg(Cnum, &msg_type,
							 (long *) &ret_code))
							< 0) {
					rxperror(Rx_errno);
					restore_and_exit(1, termiosp);
				}

				switch(msg_type) {

				case RX_INCOMPLETE:
					Printf0("rexec: incomplete message\n");
					break;

				case RX_PROTOCOL:
					Printf0("rexec: processed PROTOCOL\n");

					if ((rx_fd(Cnum) < 0) &&
					    (ret_code != RXE_OK)) {
						rxperror(ret_code);
						restore_and_exit(1, termiosp);
					}

					if (!poll_stdin)
						Try_to_write = 1;

					break;

				case RX_SERVICE_DEAD:
					Printf0("rexec: processed SERVICE_DEAD\n");
				
					/*
					 *	if no typeahead, just exit
					 */

					if (talen == 0)
						restore_and_exit(ret_code, termiosp);

					saved_ret_code = ret_code;
					if (rx_ack_exit(Cnum, tabuf, RX_MAXTASZ)
						< 0) {
						Printf0("rexec: rx_ack_exit() failed\n");

						restore_and_exit(ret_code, termiosp);
					}
					poll_stdin = 0;
					break;

				case RX_TYPEAHEAD:
					Printf0("rexec: processed TYPEAHEAD\n");

					/*
					 *	stuff unused typeahead back
					 */

					i = 0;

					while(talen--)
						(void) ioctl(fileno(stdin), TIOCSTI,
							     tabuf[i++]);

					if (rx_free_conn(Cnum) < 0)
						Printf0("rexec: rx_free_conn() failed\n");
					restore_and_exit(saved_ret_code, termiosp);
					break;

				case RX_DATA:
					Printf0("rexec: processed DATA\n");
					break;

				case RX_IOCTL:
					Printf0("rexec: processed IOCTL\n");
					break;

				case RX_EOF:
					Printf0("rexec: processed EOF\n");
					break;

				default:
					Printf0("rexec: processed unknown msg\n");
					break;

				} /* switch */

			} else if ((pollfd[pfd].fd == netfd) &&
			           (pollfd[pfd].revents & POLLHUP)) {

				Printf0("rexec: got HANGUP on netfd\n");

				if (rx_free_conn(Cnum) < 0)
					Printf0("rexec: rx_free_conn() failed\n");

			} else if (poll_stdin && (!stdin_eof) &&
				   (pollfd[pfd].fd == fileno(stdin)) &&
				   (pollfd[pfd].revents & POLLIN)) {

				Printf0("rexec: got data on fd 0\n");

				datalen = read(0, databuf, RX_MAXDATASZ);

				Printf1("rexec: read %d bytes from fd 0\n",
					(int) datalen);

				if ((datalen == 0) && (!stdin_tty))
					stdin_eof = 1;

				if ((rx_write(Cnum, databuf, datalen) < 0) &&
				    (Rx_errno == RXE_AGAIN)) {

					Printf0( "rexec: write would block, try later\n");

					poll_stdin = 0;

				} else {

					Printf0("rexec: wrote to netfd\n");

				}
			} else if (poll_stdin && (!stdin_eof) &&
				   (pollfd[pfd].fd == fileno(stdin)) &&
				   (pollfd[pfd].revents & POLLHUP)) {

				Printf0("rexec: got HANGUP on stdin!\n");

				stdin_eof = 1;
				datalen = 0;

				if ((rx_write(Cnum, databuf, datalen) < 0) &&
				    (Rx_errno == RXE_AGAIN)) {

					Printf0( "rexec: write would block, try later\n");

					poll_stdin = 0;

				} else {

					Printf0("rexec: wrote 0-len msg to netfd\n");

				}

			} /* else if */
		} /* for */

		if (Try_to_write) {

			Try_to_write = 0;

			Printf0("rexec: trying to write\n");

			if ((rx_write(Cnum, databuf, datalen) < 0) &&
			    (Rx_errno == RXE_AGAIN)) {

				Printf0("rexec: write would block, try later\n");

			} else
				poll_stdin = 1;
		}
	} /* while */

	Printf0("rexec: abnormal termination\n");
	restore_and_exit(-1, termiosp);
}
