/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libnsl:common/lib/libnsl/rexec/rxlib.c	1.2.5.5"
#ident  "$Header: rxlib.c 1.2 91/06/26 $"

/*
 *
 * MESSAGES:
 *
 * sent by:	CLIENT			SERVER
 *		--------------------------------------
 *		RXM_OPEN_REQ
 *		RXM_OPEN_ARGS
 *		RXM_OPEN_ENVF
 *		RXM_OPEN_ENV
 *		RXM_IOCTL
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


/* LINTLIBRARY */

#include <sys/byteorder.h>
#include <signal.h>
#include <errno.h>
#include <termio.h>
#include <stdio.h>
#include <stropts.h>
#include <rx.h>
#include <unistd.h>
#include <string.h>
#include "rxmsg.h"
#include <iaf.h>
#include <cs.h>


/* private (static) routine declarations */

#if defined(__STDC__)
static	int	rxcsendopenenv(int, const char *, int);
#else
static	int	rxcsendopenenv();
#endif
static	int	rxcsendopenreq();
static	int	rxcsendopenargs();
static	int	rxcsendopenenvf();
static	int	rxcsendtcsetsioctl();
static	int	rxcsendopendone();
static	int	rxcsendclosereply();
static	int	rxcsenddata();
static	int	rxcsendsignal();
static	int	rxputm();


/* externally defined global variables */

extern	int	errno;


/* global public variables */

char	Rxenvfile[RX_MAXENVFNAME];	/* name of the environment file */
long	Rx_errno;			/* rexec errno */
int	Rx_cserrno;			/* cs errno for rexec's call */


/* global private variables */

static	int	Rx_openconn = 0;
static	struct	rx_conn {
	int	state;			/* active connection flag */
	int	netfd;			/* network file descriptor */
	long	flags;			/* rexec options */
	int	credit;			/* write msg credit */
	int	flush;			/* flush output flag/outstanding sig count */
	long	msg_type;		/* message type being read, 0 -> header */
	long	bytes_expected;		/* total number of bytes in message */
	long	bytes_received;		/* number of bytes received */
#if defined(__STDC__)
	int	(*ioctl_hand)(int, int, ...);	/* ioctl handler routine */
	ssize_t	(*write_hand)(int, const void *, size_t);	/* write */
#else
	int	(*ioctl_hand)();	/* ioctl handler routine */
	int	(*write_hand)();	/* write handler routine */
#endif
	struct rx_msg_head	rx_msg_head;	/* msg header */
	char	msg[RX_MAXMSGSZ];	/* holds a generic rexec message */
	char	*ta_buf;		/* pointer to user-supplied ta buffer */
} *Conn[RX_MAXRXCONN];
static	int	Dflag;


/*
 *	rexecve() is the program level access to rexec remote execution.
 *
 *	This routine sends the opening messages (OPEN_REQ, OPEN_ARGS,
 *	OPEN_ENVF, and OPEN_ENV, followed by OPEN_DONE)  to the rexec server.
 *
 *	Upon success, a connection handle is allocated and returned.
 *	Upon failure, -1 is returned and Rx_errno is set to an
 *	appropriate error number.
 */

int
rexecve(machine, rx_service, argv, envp, flags)
char	*machine;	/* destination host name */
char	*rx_service;	/* requested rx service */
char	*argv[];	/* service arguments in array of ptrs format */
char	*envp[];	/* user environment in array of ptrs format */
long	flags;		/* rexec options */
{
	int	cnum;		/* connection number */
	int	netfd;		/* network fd */
	char	*argstr;	/* service argument string */
	char	*envstr;	/* environment string */
	char	fullenvstr[RX_MAXENVSZ]; /* full env string */
	struct termios	tp;	/* termios structure to be sent over */

	Printf0("rexecve: starting\n");

	Rx_errno = RXE_OK;

	/*
	 *check number of open connections
	 */
	if (Rx_openconn >= RX_MAXRXCONN) {
		Rx_errno = RXE_2MANYRX;
		return -1;
	}
	if (Rx_openconn == 0) {
		for (cnum = 0; cnum < RX_MAXRXCONN; cnum++) {
			Conn[cnum] = (struct rx_conn *) NULL;
		}
	}
	/*
	 * Select a structure in which to save statii
	 */
	cnum = 0;
	while ( (Conn[cnum] != (struct rx_conn *) NULL)
		&& (Conn[cnum]->state != RXS_CLOSED) ) {
		cnum++;
	}
	if (Conn[cnum] == (struct rx_conn *) NULL) {
		Conn[cnum]
			= (struct rx_conn *) calloc(1, sizeof(struct rx_conn));
	}
	if (Conn[cnum] == (struct rx_conn *) NULL) {
		Rx_errno = RXE_2MANYRX;
		return -1;
	}

	/* check flags */

	if (flags & ~RXF_ALL) {
		Rx_errno = RXE_BADFLAGS;
		return(-1);
	}

	if (flags & RXF_DEBUG)
		Dflag = 1;
	else
		Dflag = 0;

	/* process arguments */

	if (argv == (char **) NULL) {
		Rx_errno = RXE_BADARGS;
		return(-1);
	}

	/* convert into one string */

	argstr = argvtostr(argv);

	/* process environment */

	if (envp == (char **) NULL) {
		Rx_errno = RXE_BADENV;
		return(-1);
	}

	/* convert into one string */

	envstr = argvtostr(envp);

	/* open a connection to rxserver */

	Printf0("rexecve: about to call cs_connect()\n");

	if ((netfd = cs_connect(machine, RX_SVCNAME, NULL, &Rx_cserrno)) < 0) {
		Rx_errno = RXE_CONNPROB;
		return(-1);
	}

	Printf1("rexecve: tli connection established, netfd = %d\n", netfd);

	/* push tirdwr */

	if (ioctl(netfd, I_PUSH, "tirdwr") < 0) {
		Rx_errno = RXE_TIRDWR;
		(void) close(netfd);
		return(-1);
	}

	/* send OPEN_REQ */

	Printf0("rexecve: sending OPEN_REQ\n");

	if (rxcsendopenreq(netfd, RX_VERSION, rx_service, flags) < 0) {
		Rx_errno = RXE_CONNPROB;
		(void) close(netfd);
		return(-1);
	}

	/* send OPEN_ARGS, if needed */

	if (strlen(argstr) > 0) {
		Printf0("rexecve: sending OPEN_ARGS\n");

		if (rxcsendopenargs(netfd, argstr, strlen(argstr) + 1) < 0) {
			Rx_errno = RXE_CONNPROB;
			(void) close(netfd);
			return(-1);
		}
	}

	/* send OPEN_ENVF, if set */

	if (Rxenvfile[0] != '\0') {

		Printf0("rexecve: sending OPEN_ENVF\n");

		if (rxcsendopenenvf(netfd, Rxenvfile) < 0) {
			Rx_errno = RXE_CONNPROB;
			(void) close(netfd);
			return(-1);
		}
	}

	/* send OPEN_ENV, if needed */

	if (strlen(envstr) > 0) {
		Printf0("rexecve: sending OPEN_ENV\n");

		if (rxcsendopenenv(netfd, envstr, strlen(envstr) + 1) < 0) {
			Rx_errno = RXE_CONNPROB;
			(void) close(netfd);
			return(-1);
		}
	}

	/* send TCSETS IOCTL */

	/* the termios structure should be a parameter to rexecve */

	if (ioctl(0, TCGETS, &tp) < 0)
		if (ioctl(1, TCGETS, &tp) < 0)
			if (ioctl(2, TCGETS, &tp) < 0) {

				/*
				 *	cannot get terminal parameters,
				 *	so use defaults.
				 */

				tp.c_iflag = IMAXBEL | IXANY  | IXON   |
					     ICRNL   | ISTRIP | IGNPAR |
					     BRKINT;
				tp.c_oflag = TABDLY  | ONLCR  | OPOST;
				tp.c_cflag = HUPCL   | PARENB | CREAD  |
					     CS7     | CBAUD;
				tp.c_lflag = IEXTEN  | ECHOK  | ECHO   |
					     ISIG;

				tp.c_cc[0]  = '\177';
				tp.c_cc[1]  =  '\34';
				tp.c_cc[2]  =  '\10';
				tp.c_cc[3]  = '\100';
				tp.c_cc[4]  =   '\4';
				tp.c_cc[5]  =   '\0';
				tp.c_cc[6]  =   '\0';
				tp.c_cc[7]  =   '\0';
				tp.c_cc[8]  =  '\21';
				tp.c_cc[9]  =  '\23';
				tp.c_cc[10] =  '\32';
				tp.c_cc[11] =   '\0';
				tp.c_cc[12] =  '\22';
				tp.c_cc[13] =  '\17';
				tp.c_cc[14] =  '\27';
				tp.c_cc[15] =  '\26';
				tp.c_cc[16] =   '\0';
				tp.c_cc[17] =   '\0';
				tp.c_cc[18] =   '\0';
			}

	if (rxcsendtcsetsioctl(netfd, &tp) < 0) {
		Rx_errno = RXE_CONNPROB;
		(void) close(netfd);
		return(-1);
	}

	/* send OPEN_DONE */

	Printf0("rexecve: sending OPEN_DONE\n");

	if (rxcsendopendone(netfd) < 0) {
		Rx_errno = RXE_CONNPROB;
		(void) close(netfd);
		return(-1);
	}

	/*
	 * add the new connection to a list of open connections
	 */
	Conn[cnum]->state = RXS_OPENING;
	Conn[cnum]->netfd = netfd;
	Conn[cnum]->flags = flags;
	/*
	 * invalid value, fill in later from OPEN_REPLY
	 */
	Conn[cnum]->credit = -1;
	Conn[cnum]->flush = 0;	/* no signal msgs to server outstanding */
	Conn[cnum]->msg_type = 0; /* waiting for header */
	Conn[cnum]->bytes_received = 0; /* none of it received yet */
	(void) rx_set_ioctl_hand(cnum, ioctl);
	(void) rx_set_write_hand(cnum, write);
	Rx_openconn++;

	return(cnum);
}


/*
 *	rx_set_ioctl_hand() sets the ioctl handling routine for ioctls
 *	issued by the remote service and sent over by the rxserver.
 *
 *	Upon success, 0 is returned.
 *	Upon failure, -1 is returned and Rx_errno is set to an appropriate
 *	error number.
 */
#if defined(__STDC__)
int rx_set_ioctl_hand(int cnum, int (*ioctl_hand)(int, int, ...))
#else
int
rx_set_ioctl_hand(cnum, ioctl_hand)
int	cnum;
int	(*ioctl_hand)();
#endif
{
	Printf1("rx_set_ioctl_hand: cnum = %d\n", cnum);
	if ( (Conn[cnum] == (struct rx_conn *) NULL)
		|| (Conn[cnum]->state == RXS_CLOSED) ) {
		Rx_errno = RXE_BADCNUM;
		return -1;
	}
	Conn[cnum]->ioctl_hand = ioctl_hand;
	return 0;
}


/*
 *	rx_set_write_hand() sets the write handling routine for writes
 *	issued by the remote service and sent over by the rxserver.
 *
 *	Upon success, 0 is returned.
 *	Upon failure, -1 is returned and Rx_errno is set to an appropriate
 *	error number.
 *
 */
#if defined(__STDC__)
int
rx_set_write_hand(int cnum, ssize_t (*write_hand)(int, const void *, size_t))
#else
int
rx_set_write_hand(cnum, write_hand)
int	cnum;
int	(*write_hand)();
#endif
{
	Printf1("rx_set_write_hand: cnum = %d\n", cnum);

	if ( (Conn[cnum] == (struct rx_conn *) NULL)
		|| (Conn[cnum]->state == RXS_CLOSED) ) {
		Rx_errno = RXE_BADCNUM;
		return -1;
	}
	Conn[cnum]->write_hand = write_hand;
	return 0;
}


/*
 *	rx_proc_msg() process an incoming message from rxserver.  This
 *	message may be one of:
 *		RXM_OPEN_REPLY	- connection status
 *		RXM_DATA	- data from the service
 *		RXM_WRITEACK	- credit for sending more data to service
 *		RXM_IOCTL	- ioctl message
 *		RXM_SIGNALACK	- signal acknowledgement
 *		RXM_CLOSE_REQ	- service death indication
 *		RXM_CLOSE_TA	- returned typeahead data
 *
 *	The routine will read in the message, decode it, and process it.
 *	The msg_type parameter is set to the message type (if any).
 *	The ret_code parameter is set to the length of data (for RXM_DATA), ioctl
 *	return code (for RXM_IOCTL), or service process return code (for
 *	RXM_CLOSE_REQ).
 *
 *	Since with some protocols (tcp), messages may be broken up into smaller
 *	pieces which are then delivered separately (but in the same order), the
 *	newly arrived network data may not be a complete message (or message
 *	header), so more data may need to be received before the message can be
 *	processed.
 *
 *	Upon success, for RXM_CLOSE_REQ messages, the number of typeahead
 *	characters available at the rexec server (which may be 0) to be sent
 *	back is returned.  For other types of messages, 0 is returned if
 *	the action caused by the message succeeded.
 *	Upon failure, -1 is returned and Rx_errno is set to an appropriate
 *	error number.
 *
 *	In case rx_proc_msg() does not receive a complete message, 0 is returned,
 *	unless an error has also occurred.
 *
 */

int
rx_proc_msg(cnum, msg_type, ret_code)
int	cnum;		/* connection number */
long	*msg_type;	/* message type */
long	*ret_code;	/* return code */
{
	long	type;	/* message type */
	int	need;	/* number of bytes needed for a complete message */
	int	got;	/* number of bytes received */
	struct rx_conn *ptr;	/* pointer op's are faster than indexing */

	Printf1("rx_proc_msg: cnum = %d\n", cnum);

	ptr = Conn[cnum];
	if ( (ptr == (struct rx_conn *) NULL)
		|| (ptr->state == RXS_CLOSED) ) {
		Rx_errno = RXE_BADCNUM;
		return -1;
	}

	/* default value for returned msg_type */

	*msg_type = RX_INCOMPLETE;

	/*
	 * what message were we reading?
	 */
	if (ptr->msg_type == 0) {
		/*
		 * reading header
		 */
		need = sizeof(struct rx_msg_head) - ptr->bytes_received;
		got = read(ptr->netfd
			, (char *) &(ptr->rx_msg_head) + ptr->bytes_received
			, need);
		if (got < 0) {
			Printf2("rx_proc_msg: read returned %d, errno = %d\n",
			       got, errno);
			Rx_errno = RXE_CONNPROB;
			return -1;
		}
		Printf1("rx_proc_msg: read %d bytes\n", got);
		if (got == 0) {
			Printf0("rx_proc_msg: got 0 byte message, exiting\n");
			*msg_type = RX_EOF;
			if (rx_free_conn(cnum) < 0) {
				Printf0("rx_proc_msg: rx_free_conn() failed\n");
			}
			return 0;
		}
		if (got < need) {
			ptr->bytes_received += got;
			return 0;
		}
		/*
		 * header is complete
		 */
		ptr->bytes_received = 0;
		ptr->bytes_expected = ntohl((int) ptr->rx_msg_head.msg_len);
		ptr->msg_type = ntohl((int) ptr->rx_msg_head.msg_type);
		Printf2("rx_proc_msg: got header, msgtype = %d, len = %d\n",
		       ptr->msg_type, ptr->bytes_expected);
		return 0;
	}
	/*
	 * reading message
	 */
	need = ptr->bytes_expected - ptr->bytes_received;
	got = read(ptr->netfd, ptr->msg + ptr->bytes_received, need);
	if (got < 0) {
		Printf1("rx_proc_msg: read returned %d\n", got);
		Rx_errno = RXE_CONNPROB;
		return -1;
	}

	Printf1("rx_proc_msg: read %d bytes\n", got);

	if (got == 0) {
		Printf0("rx_proc_msg: got 0 byte message, exiting\n");
		if (rx_free_conn(cnum) < 0)
			Printf0("rx_proc_msg: rx_free_conn() failed\n");
		return 0;
	}

	if (got < need) {
		ptr->bytes_received += got;
		return 0;
	}

	type = ptr->msg_type;

	/* reset message reading status */

	ptr->msg_type = 0;	/* message header is next */
	ptr->bytes_received = 0;	/* no bytes received */

	/* process message */

	switch(type) {

	case RXM_OPEN_REPLY: {
		struct open_reply	*open_reply;
		long	version;
		long	credit;

		if (ptr->state != RXS_OPENING) {
			Rx_errno = RXE_BADSTATE;
			return -1;
		}

		*msg_type = RX_PROTOCOL;

		open_reply = (struct open_reply *) ptr->msg;

		version = ntohl(open_reply->version);
		*ret_code = ntohl(open_reply->ret_code);
		credit = ntohl(open_reply->credit);

		Printf0("rx_proc_msg: RXM_OPEN_REPLY message, ");
		Printf2("version = %d, ret_code = %d, ", version, *ret_code);
		Printf1("credit = %d\n", credit);

		/* process open_reply message - check ret_code */

		if (*ret_code != RXE_OK) {
			ptr->state = RXS_CLOSED;
			Rx_errno = *ret_code;
			return -1;
		}
		/*
		 * if more than one version, may want to check version here
		 */
		ptr->state = RXS_OPEN;
		ptr->credit = credit;
		return 0;
	}

	case RXM_DATA: {
		struct data_msg		*data_msg;
		int	fd;
		long	len;
		char	*buf;

		if ((ptr->state != RXS_OPEN)
			&& (ptr->state != RXS_CLOSING)) {
			Rx_errno = RXE_BADSTATE;
			return -1;
		}

		*msg_type = RX_DATA;

		if (ptr->flush > 0) {
			Printf0("rx_proc_msg: DATA flushed, not written\n");
			return 0;
		}
		data_msg = (struct data_msg *) ptr->msg;

		fd = ntohl(data_msg->fd);
		len = ntohl(data_msg->len);
		buf = data_msg->buf;

		*ret_code = len;

		Printf2("rx_proc_msg: RXM_DATA message, fd = %d, len = %d\n",
			fd, len);

		if (((ptr->write_hand) (fd, (void *) buf, len)) != len) {
			Printf0("rx_proc_msg: problems in using write handler\n");
			Rx_errno = RXE_WRITE;
			return -1;
		}
		Printf0("rx_proc_msg: data written successfully\n");
		return 0;
	}

	case RXM_WRITEACK: {
		struct writeack_msg	*writeack_msg;
		long	credit;

		if (ptr->state != RXS_OPEN) {
			Rx_errno = RXE_BADSTATE;
			return -1;
		}
		*msg_type = RX_PROTOCOL;
		writeack_msg = (struct writeack_msg *) ptr->msg;
		credit = ntohl(writeack_msg->credit);
		Printf1("rx_proc_msg: RXM_WRITEACK message, credit = %d\n"
			, credit);
		ptr->credit += credit;
		return 0;
	}

	case RXM_IOCTL: {
		struct ioctl_msg	*ioctl_msg;
		int	fd;
		int	ioc;
		int	arglen;
		struct termio	*tio;
		struct termios	*tios;
		int	tcsbrkarg;
		struct winsize	*win;

		if (ptr->state != RXS_OPEN) {
			Rx_errno = RXE_BADSTATE;
			return -1;
		}
		*msg_type = RX_IOCTL;
		ioctl_msg = (struct ioctl_msg *) ptr->msg;

		fd = ntohl(ioctl_msg->fd);
		ioc = ntohl(ioctl_msg->ioc);
		arglen = ntohl(ioctl_msg->arglen);

		Printf1("rx_proc_msg: RXM_IOCTL, fd = %d, ", fd);
		Printf2("ioc = %d, arglen = %d\n", ioc, arglen);

		switch(ioc) {

		case TCSETS:
		case TCSETSW:
		case TCSETSF:
			tios = (struct termios *) ioctl_msg->arg;
			tios->c_iflag = ntohl(tios->c_iflag);
			tios->c_oflag = ntohl(tios->c_oflag);
			tios->c_cflag = ntohl(tios->c_cflag);
			tios->c_lflag = ntohl(tios->c_lflag);

			if (((ptr->ioctl_hand) (fd, ioc, tios)) < 0) {
				Printf1("rx_proc_msg: \
problems with ioctl handler, errno = %d\n", errno);
				Rx_errno = RXE_IOCTL;
				return -1;
			}
			break;

		case TCSETA:
		case TCSETAW:
		case TCSETAF:
			tio = (struct termio *) ioctl_msg->arg;
			tio->c_iflag = ntohs(tio->c_iflag);
			tio->c_oflag = ntohs(tio->c_oflag);
			tio->c_cflag = ntohs(tio->c_cflag);
			tio->c_lflag = ntohs(tio->c_lflag);

			if (((ptr->ioctl_hand) (fd, ioc, tio)) != 0) {
				Printf1("rx_proc_msg: \
problems with ioctl handler, errno = %d\n", errno);
				Rx_errno = RXE_IOCTL;
				return -1;
			}
			break;

		case TCSBRK:
			tcsbrkarg = ntohl(*((int *) ioctl_msg->arg));
			if (((ptr->ioctl_hand) (fd, TCSBRK, tcsbrkarg)) != 0) {
				Printf1("rx_proc_msg: \
problems with ioctl handler, errno = %d\n", errno);
				Rx_errno = RXE_IOCTL;
				return -1;
			}
			break;

		case TIOCSWINSZ:
			win = (struct winsize *) ioctl_msg->arg;
			win->ws_row = ntohs(win->ws_row);
			win->ws_col = ntohs(win->ws_col);
			win->ws_xpixel = ntohs(win->ws_xpixel);
			win->ws_ypixel = ntohs(win->ws_ypixel);

			if (((ptr->ioctl_hand) (fd, ioc, win)) != 0) {
				Printf1("rx_proc_msg: \
problems with ioctl handler, errno = %d\n", errno);
				Rx_errno = RXE_IOCTL;
				return -1;
			}
			break;

		default:
			break;
		}

		return 0;
	}

	case RXM_SIGNALACK: {
		struct signalack_msg	*signalack_msg;
		long	sig;

		if (ptr->state != RXS_OPEN) {
			Rx_errno = RXE_BADSTATE;
			return -1;
		}

		*msg_type = RX_PROTOCOL;

		signalack_msg = (struct signalack_msg *) ptr->msg;

		sig = ntohl(signalack_msg->sig);

		Printf1("rx_proc_msg: RXM_SIGNALACK, for signal %d\n", sig);

		/*
		 * restart output if no more outstanding signals
		 */
		ptr->flush--;
		return 0;
	}

	case RXM_CLOSE_REQ: {
		struct close_req	*close_req;

		if (ptr->state != RXS_OPEN) {
			Rx_errno = RXE_BADSTATE;
			return -1;
		}

		*msg_type = RX_SERVICE_DEAD;

		Printf0("rx_proc_msg: RXM_CLOSE_REQ\n");

		ptr->state = RXS_CLOSING;

		close_req = (struct close_req *) ptr->msg;

		*ret_code = ntohl(close_req->ret_code);

		return ntohl(close_req->tasize);
	}

	case RXM_CLOSE_TA: {
		struct close_ta		*close_ta;
		int	tasize;

		if (ptr->state != RXS_CLOSING) {
			Rx_errno = RXE_BADSTATE;
			return -1;
		}

		*msg_type = RX_TYPEAHEAD;

		Printf0("rx_proc_msg: RXM_CLOSE_TA\n");

		close_ta = (struct close_ta *) ptr->msg;
		tasize = ntohl(close_ta->tasize);
		memcpy(ptr->ta_buf, close_ta->tabuf, tasize);
		ptr->state = RXS_CLOSED;
		return 0;
	}

	default:
		Printf1("rx_proc_msg: UNEXPECTED message type = %d\n", type);

		Rx_errno = RXE_PROTOCOL;
		return -1;
	}
}


/*
 *	rx_signal() sends a signal to the remote process.  Only four
 *	signals are allowed: SIGHUP, SIGPIPE, SIGINT, and SIGQUIT.
 *
 *	Upon success, 0 is returned.
 *	Upon failure, -1 is returned and Rx_errno is set to an appropriate
 *	error number.
 *
 */

int
rx_signal(cnum, signum)
int	cnum;		/* connection number */
int	signum;		/* signal number */
{
	struct rx_conn *ptr;	/* pointer op's are faster than indexing */

	Printf2("rx_signal: cnum = %d, signum = %d\n", cnum, signum);

	ptr = Conn[cnum];
	if ( (ptr == (struct rx_conn *) NULL)
		|| (ptr->state != RXS_OPEN) ) {
		Rx_errno = RXE_BADCNUM;
		return -1;
	}
	if ((signum != SIGHUP) &&
	    (signum != SIGINT) &&
	    (signum != SIGPIPE) &&
	    (signum != SIGQUIT)) {
		Rx_errno = RXE_BADSIG;
		return(-1);
	}

	if (rxcsendsignal(ptr->netfd, signum) < 0) {
		return -1;
	}
	ptr->flush++;
	return 0;
}


/*
 *	rx_write() is used to send input data to the remote service.
 *	Since rexec is not allowed to block on writes to the service,
 *	the routine will check for possible block condition when
 *	writing to the network and later again when writing to the
 *	master pty by rxserver.  If the write cannot complete right
 *	away, the rx_write() will drop the request and fail.
 *
 *	Determining whether a write will block at the server is done
 *	by keeping track of "write credit".  As long as the client
 *	has a write credit of 1 or more RXM_DATA messages, the
 *	server is guaranteed to accept them and not block.  For each
 *	RXM_DATA message which the server successfully writes, it will
 *	send back an RXM_WRITEACK message "containing" additional credit.
 *	Since it is OK for the client to block on a write to stdout,
 *	this procedure is not used for data flowing in the reverse
 *	(server to client) direction.
 *
 *	Upon success, 0 is returned.
 *	Upon failure, -1 is returned and Rx_errno is set to an appropriate
 *	error number.
 *
 */

int
rx_write(cnum, buf, len)
int	cnum;	/* connection number */
char	*buf;	/* data buffer */
long	len;	/* length of data to be sent */
{
	struct rx_conn *ptr;	/* pointer op's are faster than indexing */

	Printf2("rx_write: cnum = %d, len = %d\n", cnum, len);

	ptr = Conn[cnum];
	if (ptr == (struct rx_conn *) NULL) {
		Rx_errno = RXE_BADCNUM;
		return -1;
	}
	if ( (ptr->state == RXS_OPENING)
		|| (ptr->credit == 0)) {
		Rx_errno = RXE_AGAIN;
		return -1;
	}
	if (ptr->state != RXS_OPEN) {
		Rx_errno = RXE_BADCNUM;
		return -1;
	}

	if (rxcsenddata(ptr->netfd, 0, len, buf) < 0) {
		Rx_errno = RXE_CONNPROB;
		return -1;
	}
	ptr->credit--;
	Printf0("rx_write: succeeded\n");
	return 0;
}


/*
 *	rx_ack_exit() is used by rexec to ACK the termination of the
 *	remote service and request the return of unused typeahead.
 *	The use of this routine is optional.  If the connection is
 *	dropped before calling this routine, rxserver assumes that
 *	no typeahead need be returned and exits.
 *
 *	Upon success, 0 is returned.
 *	Upon failure, -1 is returned and Rx_errno is set to an appropriate
 *	error number.
 *
 */

int
rx_ack_exit(cnum, ta_buf, ta_len)
int	cnum;		/* connection number */
char	*ta_buf;	/* buffer which will hold returned typeahead */
long	ta_len;		/* typeahead buffer length */
{
	struct rx_conn *ptr;	/* pointer op's are faster than indexing */

	Printf1("rx_ack_exit: cnum = %d\n", cnum);

	ptr = Conn[cnum];
	if ( (ptr == (struct rx_conn *) NULL) 
		|| (ptr->state != RXS_CLOSING) ) {
		Rx_errno = RXE_BADCNUM;
		return -1;
	}
	if (rxcsendclosereply(ptr->netfd, ta_len) < 0) {
		Rx_errno = RXE_CONNPROB;
		return -1;
	}
	ptr->ta_buf = ta_buf;
	return 0;
}


/*
 *	rx_free_conn() releases the client state associated with an
 *	rxserver connection.
 *
 *	Upon success, 0 is returned.
 *	Upon failure, -1 is returned and Rx_errno is set to an appropriate
 *	error number.
 *
 */

int
rx_free_conn(cnum)
int	cnum;		/* connection number */
{
	struct rx_conn *ptr;	/* pointer op's are faster than indexing */

	Printf1("rx_free_conn: cnum = %d\n", cnum);

	ptr = Conn[cnum];
	if ( (ptr == (struct rx_conn *) NULL) 
		|| (Rx_openconn == 0) || (ptr->state == RXS_CLOSED)) {
		Rx_errno = RXE_BADCNUM;
		return -1;
	}
	ptr->state = RXS_CLOSED;
	Rx_openconn--;
	(void) close(ptr->netfd);
	return 0;
}


/*
 *	rx_fd() returns the file descriptor associated with an rexec
 *	connection.
 *
 *	Upon success, 0 is returned.
 *	Upon failure, -1 is returned and Rx_errno is set to an appropriate
 *	error number.
 *
 */

int
rx_fd(cnum)
int	cnum;		/* connection number */
{
	struct rx_conn *ptr;	/* pointer op's are faster than indexing */

	ptr = Conn[cnum];
	if ( (ptr == (struct rx_conn *) NULL) 
		|| (ptr->state == RXS_CLOSED) ) {
		Rx_errno = RXE_BADCNUM;
		return -1;
	}
	return ptr->netfd;
}


/*
 *	rxcsendopenreq()
 *
 */

static int
rxcsendopenreq(netfd, version, service, flags)
int	netfd;			/* network file descriptor */
long	version;		/* rexec version number */
char	service[RX_MAXSVCSZ];	/* remote service name */
long	flags;			/* rexec options */
{
	struct open_req open_req;

	open_req.version = htonl(version);
	(void) strncpy(open_req.service, service, RX_MAXSVCSZ);
	open_req.flags = htonl(flags);

	if (rxputm(netfd, RXM_OPEN_REQ, sizeof(struct open_req),
		   (char *) &open_req) < 0) {
		Printf0("rxcsendopenreq: rxputm(RXM_OPEN_REQ) failed\n");
		return(-1);
	}

	return(0);
}


/*
 *	rxcsendopenargs()
 *
 */

static int
rxcsendopenargs(netfd, argstr, len)
int	netfd;			/* network file descriptor */
char	*argstr;		/* argument string */
int	len;			/* string length */
{
	struct open_args open_args;

	if (len > RX_MAXARGSZ)
		len = RX_MAXARGSZ;

	(void) strncpy(open_args.argstr, argstr, len);

	if (rxputm(netfd, RXM_OPEN_ARGS, RX_OPEN_ARGS_SZ(len),
		   (char *) &open_args) < 0) {
		Printf0("rxcsendopenargs: rxputm(RXM_OPEN_ARGS) failed\n");
		return(-1);
	}

	return(0);
}


/*
 *	rxcsendopenenvf()
 *
 */

static int
rxcsendopenenvf(netfd, rxenvfile)
int	netfd;			/* network file descriptor */
char	*rxenvfile;
{
	struct open_envf	open_envf;

	memcpy(open_envf.envfile, rxenvfile, RX_MAXENVFNAME);

	if (rxputm(netfd, RXM_OPEN_ENVF, sizeof(struct open_envf),
		   (char *) &open_envf) < 0) {
		Printf0("rxcsendopenenvf: rxputm(RXM_OPEN_ENVF) failed\n");
		return(-1);
	}

	return(0);
}


/*
 *	rxcsendopenenv()
 *
 */

static int
rxcsendopenenv(netfd, envstr, len)
int	netfd;			/* network file descriptor */
const char *envstr;		/* environment string */
int	len;			/* string length */
{
	struct open_env open_env;

	if (len > RX_MAXENVSZ) {
		len = RX_MAXENVSZ;
	}
	(void) strncpy(open_env.envstr, envstr, len);

	if (rxputm(netfd, RXM_OPEN_ENV, RX_OPEN_ENV_SZ(len),
		   (char *) &open_env) < 0) {
		Printf0("rxcsendopenenv: rxputm(RXM_OPEN_ENV) failed\n");
		return -1;
	}

	return 0;
}


/*
 *	rxcsendtcsetsioctl()
 *
 */

static int
rxcsendtcsetsioctl(netfd, tp)
int	netfd;			/* network file descriptor */
struct termios	*tp;		/* termios structure pointer */
{
	struct ioctl_msg	ioctl_msg;
	struct termios	*ntp = (struct termios *) ioctl_msg.arg;

	ioctl_msg.fd = htonl(0);
	ioctl_msg.ioc = htonl(TCSETS);
	ioctl_msg.arglen = htonl(sizeof(struct termios));
	ntp->c_iflag = htonl(tp->c_iflag);
	ntp->c_oflag = htonl(tp->c_oflag);
	ntp->c_cflag = htonl(tp->c_cflag);
	ntp->c_lflag = htonl(tp->c_lflag);
	(void) memcpy(ntp->c_cc, tp->c_cc, NCCS);

	if (rxputm(netfd, RXM_IOCTL, RX_IOCTL_MSG_SZ(sizeof(struct termios)),
		   (char *) &ioctl_msg) < 0) {
		Printf0("rxcsendtcsetsioctl: rxputm(RXM_IOCTL) failed\n");
		return(-1);
	}

	return(0);
}


/*
 *	rxcsendopendone()
 *
 */

static int
rxcsendopendone(netfd)
int	netfd;			/* network file descriptor */
{
	if (rxputm(netfd, RXM_OPEN_DONE, 0, NULL) < 0) {
		Printf0("rxcsendopendone: rxputm(RXM_OPEN_DONE) failed\n");
		return(-1);
	}

	return(0);
}


/*
 *	rxcsendclosereply()
 *
 */

static int
rxcsendclosereply(netfd, tasize)
int	netfd;			/* network file descriptor */
long	tasize;			/* amount of typeahead to return */
{
	struct close_reply close_reply;

	close_reply.tasize = htonl(tasize);

	if (rxputm(netfd, RXM_CLOSE_REPLY, sizeof(struct close_reply),
		   (char *) &close_reply) < 0) {
		Printf0("rxcsendclosereply: rxputm(RXM_CLOSE_REPLY) failed\n");
		return(-1);
	}

	return(0);
}


/*
 *	rxcsenddata()
 *
 */

static int
rxcsenddata(netfd, fd, len, buf)
int	netfd;			/* network file descriptor */
long	fd;			/* data's src fd */
long	len;			/* data length */
char	*buf;			/* data buffer */
{
	struct data_msg data_msg;

	data_msg.fd = htonl(fd);
	data_msg.len = htonl(len);
	(void) memcpy(data_msg.buf, buf, len);

	if (rxputm(netfd, RXM_DATA, RX_DATA_MSG_SZ(len), (char *) &data_msg) < 0) {
		Printf0("rxcsenddata: rxputm(RXM_DATA) failed\n");
		return(-1);
	}

	return(0);
}


/*
 *	rxcsendsignal()
 *
 */

static int
rxcsendsignal(netfd, signum)
int	netfd;			/* network file descriptor */
int	signum;			/* signal number */
{
	struct signal_msg signal_msg;

	signal_msg.sig = htonl(signum);

	if (rxputm(netfd, RXM_SIGNAL, sizeof(struct signal_msg),
		   (char *) &signal_msg) < 0) {
		Printf0("rxcsendsignal: rxputm(RXM_SIGNAL) failed\n");
		return(-1);
	}

	return(0);
}


/*
 *	rxputm() is an internal routine used to send rexec messages
 *	to rxserver.
 *	It assumes that netfd is an open file descriptor to rxserver.
 *	If the message cannot be written out to the network immediately,
 *	the routine fails and sets Rx_errno to RXE_WOULDBLOCK.
 *
 *	Upon success, 0 is returned.
 *	Upon failure, -1 is returned and Rx_errno is set to an appropriate
 *	error number.
 *
 */

static int
rxputm(netfd, type, len, msg)
int	netfd;
long	type;
long	len;
char	*msg;
{
	struct rx_msg_head	head;	/* rexec message header */
	int	wr;			/* write return value */

	Printf0("rxputm: writing header\n");

	head.msg_type = htonl(type);
	head.msg_len = htonl(len);

	/* write the header */
	wr = write(netfd, (char *) &head, sizeof(struct rx_msg_head));

	if (wr < 0) {

		Printf2("rxputm: write returned %d, errno = %d\n", wr, errno);

		return(-1);
	}

	if (wr != sizeof(struct rx_msg_head)) {

		Printf2("rxputm: did not complete: wrote %d out of %d\n",
			wr, sizeof(struct rx_msg_head));

		return(-1);
	}

	/* write the message, if any */

	if (len == 0) {

		Printf0("rxputm: 0-length message, nothing sent\n");

		return(0);
	}

	Printf0("rxputm: writing message\n");

	wr = write(netfd, msg, (unsigned int) len);

	if (wr < 0) {

		Printf2("rxputm: write returned %d, errno = %d\n", wr, errno);

		return(-1);
	}

	if (wr != len) {

		Printf2("rxputm: did not complete: wrote %d out of %d\n",
		       wr, len);

		return(-1);
	}

	Printf0("rxputm: success\n");

	return(0);
}
