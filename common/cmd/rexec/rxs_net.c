/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)rexec:rxs_net.c	1.9.2.6"
#ident  "$Header: rxs_net.c 1.2 91/06/27 $"

#include <sys/types.h>
#include <sys/byteorder.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <termio.h>
#include <pwd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <rx.h>
#include <string.h>
#include <stdlib.h>
#include "rxmsg.h"
#include "rxsvcent.h"

#define	RX_INITCREDIT	1

#define	ENV_HOME	"HOME="
#define	ENV_LOGNAME	"LOGNAME="
#define	ENV_SHELL	"SHELL="
#define	ENV_PATH	"PATH="
#define	ENV_TZ		"TZ="

#define	PATHVAR		"PATH"
#define TZVAR		"TZ"
#define	ENVDELIM	'='

#define	BINSHELL	"/bin/sh"


/* externally defined routines */

extern	void	log();
extern	void	close_log();
extern	int	makepttypair();
extern	int	makepipe();
extern	void	child_actions();
extern	void	sigch_hand();
extern	int	skipcmnt();
extern	int	getsvcent();
extern	void	settarget();
extern	int	rxputm();
extern	char	**strtoargv();


/* local routines */

static	void	prepare_service();
static	int	rxsendopenreply();
static	int	rxsendcloseta();
static	int	rxsendwriteack();
static	int	rxsendsignalack();


/* externally defined global variables */

extern	int	State;		/* server state number */
extern	int	Ptty_fd;	/* master ptty fd */
extern	int	Slaveptty_fd;	/* slave ptty fd */
extern	int	Pipe_open;	/* error pipe open flag */
extern	int	Childpipe_fd;	/* child side error pipe fd */
extern	char	*Tabuf;		/* unused typeahead buffer */
extern	long	Tasize;		/* amount of typeahead in buffer */
extern	char	Logmsg[];
extern	int	Child_died;	/* child died flag */


/* locally defined global variables */

int	Net_fd;			/* network fd */
int	Net_open;		/* network connection open flag */
long	Flags;			/* service flags */
char	**Svcargv;		/* service arguments */
char	*Svcenvp[RX_MAXENVS];	/* service environment */
int	Buffer_empty;		/* service buffer empty flag */
int	Termios_received = 0;	/* termios info received flag */
struct termios	Termios;	/* termios structure buffer */


/* private global variables */

static	int	Child_pid;		/* child's pid */
static	char	Service[RX_MAXSVCSZ];	/* service name */
static	char	Argstr[RX_MAXARGSZ];	/* complete argument string */
static	char	Envstr[RX_MAXENVSZ];	/* complete environment string */
static	char	Envfile[RX_MAXENVFNAME];/* environment file name */
static	int	St_msg_type = 0;	/* current message type, 0 = header */
static	int	St_bytes_expected;	/* number of bytes in message/header */
static	int	St_bytes_received;	/* number of bytes already received */
static	char	*St_first_byte;		/* pointer to first byte of message */
static	struct rx_msg_head	Rx_msg_head;
static	struct open_req		Open_req;
static	struct open_env		Open_env;
static	struct open_envf	Open_envf;
static	struct open_args	Open_args;
static	struct open_reply	Open_reply;
static	struct close_req	Close_req;
static	struct close_reply	Close_reply;
static	struct close_ta		Close_ta;
static	struct data_msg		Data_msg;
static	struct writeack_msg	Writeack_msg;
static	struct signal_msg	Signal_msg;
static	struct ioctl_msg	Ioctl_msg;
static	int	Data_buffer_len;	/* amount of used buffer space */
static	char	Data_buffer[RX_MAXDATASZ];  /* data buffer */
static	char	*Data_bufferp;		/* data buffer */
#define	ENVLEN	256
static	char	envhome[ENVLEN];	/* environment for HOME */
static	char	envlogname[ENVLEN];	/* environment for LOGNAME */
static	char	envshell[ENVLEN];	/* environment for SHELL */
static	char	envpath[ENVLEN];	/* environment for PATH */
static	char	envtz[ENVLEN];		/* environment for TZ */


/*
 * net_hup()
 *
 * This routine is called when a hangup occurs on the network connection fd
 *
 */

void
net_hup()
{
	log("net: HUP from network");

	/* send HUP to child */
	if (State == RXS_OPEN) {
		log("net: killing child (first offense)");
		if (ioctl(Ptty_fd, TIOCSIGNAL, SIGHUP) < 0) {
			(void) sprintf(Logmsg,
				"net: ioctl(TIOCSIGNAL, SIGHUP) failed, errno = %d\n",
				errno);
			log(Logmsg);
		}
	}

	log("net: exiting");
	close_log();
	exit(1);
}


/*
 * net_msg()
 *
 * This routine is called when data is detected on the network connection fd
 *
 */

void
net_msg()
{
	int	len;
	int	need;		/* number of bytes needed for a complete message */
	int	got;		/* number of bytes received */
	int	wr;		/* bytes written */
	int	signum;		/* signal number */
	long	maxtasize;	/* amount of requested typeahead */
	int	oldstate = State; /* saved state number */
	struct termios	*tp;	/* temporary termios ptr */

	log("net: data from network");

	/* are we in the middle of a message? */
	if (St_msg_type) {

		need = St_bytes_expected - St_bytes_received;
		if ((got = read(Net_fd, St_first_byte + St_bytes_received,
				need)) < 0) {
			(void) sprintf(Logmsg, "net: read %d bytes", got);
			log(Logmsg);
			return;
		}
		(void) sprintf(Logmsg, "net: read %d bytes", got);
		log(Logmsg);

		if (got == 0) {
			log("net: treating as a HUP");
			net_hup();
		}

		/* incomplete message? */
		if (got != need) {

			St_bytes_received += got;
			return;
		}

		/* got a complete message */
		switch(St_msg_type) {

		case RXM_DATA:
			if (State != RXS_OPEN) {
				log("net: UNEXPECTED message");
				St_msg_type = 0;
				St_bytes_received = 0;
				return;
			}
			len = ntohl((int) Data_msg.len);
			log("net: \twriting data to master pty");
			wr = write(Ptty_fd, Data_msg.buf, len);
			if (wr == len) {
				log("net: \tsucceeded");
				(void) rxsendwriteack(Net_fd, 1);
			} else if (wr < 0) {
				if (errno == EWOULDBLOCK) {
					log("net: \tEWOULDBLOCK");
					if (Buffer_empty) {
						Data_buffer_len = len;
						memcpy(Data_buffer,
						       Data_msg.buf,
						       len);
						Data_bufferp = Data_buffer;
						Buffer_empty = 0;
					}
				} else {
					(void) sprintf(Logmsg,
						       "net: \terrno = %d",
						       errno);
					log(Logmsg);
				}
			} else {
				(void) sprintf(Logmsg,
					       "net: \tincomplete - errno = %d",
					       errno);
				log(Logmsg);
			}

			break;

		case RXM_SIGNAL:
			if (State != RXS_OPEN) {
				log("net: UNEXPECTED message");
				St_msg_type = 0;
				St_bytes_received = 0;
				return;
			}
			signum = ntohl((int) Signal_msg.sig);
			(void) sprintf(Logmsg,
				       "net: sending signal %d to proc %d",
				       signum, Child_pid);
			log(Logmsg);

			/* send signal to ctrling process at slave via master */
			if (ioctl(Ptty_fd, TIOCSIGNAL, signum) < 0) {
				log("net: failed to send signal");
			}

			/* send signal ack message */
			(void) rxsendsignalack(Net_fd, signum);

			break;

		case RXM_CLOSE_REPLY:
			if (State != RXS_CLOSING) {
				log("net: UNEXPECTED message");
				St_msg_type = 0;
				St_bytes_received = 0;
				return;
			}
			maxtasize = ntohl((int) Close_reply.tasize);

			(void) sprintf(Logmsg,
				"net: CLOSE_REPLY - asked for %ld, have %ld",
				maxtasize, Tasize);
			log(Logmsg);

			/* send typeahead message */
			(void) rxsendcloseta(Net_fd, (Tasize < maxtasize)?
							Tasize : maxtasize,
							Tabuf);
			break;

		case RXM_OPEN_REQ:
			if (State != RXS_OPENING) {
				log("net: UNEXPECTED message");
				St_msg_type = 0;
				St_bytes_received = 0;
				return;
			}
			(void) strncpy(Service, Open_req.service, RX_MAXSVCSZ);
			Flags = ntohl((int) Open_req.flags);
			(void) sprintf(Logmsg,
				"net: OPEN_REQ - service = %s, flags = %ld",
				Service, Flags);
			log(Logmsg);

			/* in case we don't get these RXM_OPEN_x msgs */
			Argstr[0] = '\0';
			Envstr[0] = '\0';
			Envfile[0] = '\0';
			break;

		case RXM_OPEN_ARGS:
			log("net: OPEN_ARGS");
			(void) strcat(Argstr, Open_args.argstr);
			break;

		case RXM_OPEN_ENV:
			log("net: OPEN_ENV");
			(void) strcat(Envstr, Open_env.envstr);
			break;

		case RXM_OPEN_ENVF:
			log("net: OPEN_ENVF");
			(void) strncpy(Envfile, Open_envf.envfile, RX_MAXENVFNAME);
			break;

		case RXM_IOCTL:
			/*
			 *	This message is not normally sent
			 *	to the server -- it is only sent
			 *	during the opening stage and contains
			 *	the initial termios flags for the
			 *	pseudo terminals
			 *
			 */

			log("net: RXM_IOCTL");

			if (State != RXS_OPENING) {
				log("net: UNEXPECTED message");
				St_msg_type = 0;
				St_bytes_received = 0;
				return;
			}

			if ((ntohl(Ioctl_msg.ioc) != TCSETS) ||
			    (ntohl(Ioctl_msg.arglen) !=
			      sizeof(struct termios))) {
				log("net: got unexpected IOCTL");
				St_msg_type = 0;
				St_bytes_received = 0;
				return;
			}

			tp = (struct termios *) Ioctl_msg.arg;
			Termios.c_iflag = ntohl(tp->c_iflag);
			Termios.c_oflag = ntohl(tp->c_oflag);
			Termios.c_cflag = ntohl(tp->c_cflag);
			Termios.c_lflag = ntohl(tp->c_lflag);
			memcpy(Termios.c_cc, tp->c_cc, NCCS);

			Termios_received = 1;

			if (!(Flags & RXF_STDOUTTERM))
				Termios.c_oflag &= ~(ONLCR | XTABS);

			break;

		case RXM_OPEN_DONE:
			log("net: UNEXPECTED message - RXM_OPEN_DONE");
			break;

		case RXM_OPEN_REPLY:
			log("net: UNEXPECTED message - RXM_OPEN_REPLY");
			break;

		case RXM_CLOSE_REQ:
			log("net: UNEXPECTED message - RXM_CLOSE_REQ");
			break;

		case RXM_CLOSE_TA:
			log("net: UNEXPECTED message - RXM_CLOSE_TA");
			break;

		case RXM_WRITEACK:
			log("net: UNEXPECTED message - RXM_WRITEACK");
			break;

		default:
			(void) sprintf(Logmsg,"net: UNKNOWN message type -- %d",
				       St_msg_type);
			log(Logmsg);
			break;
		}

		St_msg_type = 0;
		St_bytes_received = 0;
	} else {

		/* we are reading the header */
		need = sizeof(struct rx_msg_head) - St_bytes_received;
		if ((got = read(Net_fd, (&Rx_msg_head) + St_bytes_received,
				need)) < 0) {
			(void) sprintf(Logmsg,
				       "net: read failed, errno = %d",
				       errno);
			log(Logmsg);
			return;
		}
		(void) sprintf(Logmsg, "net: read %d bytes", got);
		log(Logmsg);

		if (got < need) {
			St_bytes_received += got;
			return;
		}

		/* header is complete */
		St_bytes_received = 0;
		St_bytes_expected = ntohl((int) Rx_msg_head.msg_len);
		St_msg_type = ntohl((int) Rx_msg_head.msg_type);
		(void) sprintf(Logmsg,
			       "net: got header, msgtype = %d, len = %d",
			       St_msg_type, St_bytes_expected);
		log(Logmsg);

		switch(St_msg_type) {

		case RXM_OPEN_REQ:
			St_first_byte = (char *) &Open_req;
			break;

		case RXM_OPEN_ARGS:
			St_first_byte = (char *) &Open_args;
			break;

		case RXM_OPEN_ENVF:
			St_first_byte = (char *) &Open_envf;
			break;

		case RXM_OPEN_ENV:
			St_first_byte = (char *) &Open_env;
			break;

		case RXM_OPEN_DONE:
			if (State != RXS_OPENING) {
				log("net: UNEXPECTED message");
				return;
			}
			log("net: OPEN_DONE");
			log("net: state = RXS_OPEN");
			State = RXS_OPEN;
			St_msg_type = 0; /* there is no message body */
			St_first_byte = (char *) &Rx_msg_head;
			break;

		case RXM_OPEN_REPLY:
			St_first_byte = (char *) &Open_reply;
			break;

		case RXM_CLOSE_REQ:
			St_first_byte = (char *) &Close_req;
			break;

		case RXM_CLOSE_REPLY:
			St_first_byte = (char *) &Close_reply;
			break;

		case RXM_CLOSE_TA:
			St_first_byte = (char *) &Close_ta;
			break;

		case RXM_DATA:
			St_first_byte = (char *) &Data_msg;
			break;

		case RXM_WRITEACK:
			St_first_byte = (char *) &Writeack_msg;
			break;

		case RXM_SIGNAL:
			St_first_byte = (char *) &Signal_msg;
			break;

		case RXM_IOCTL:
			St_first_byte = (char *) &Ioctl_msg;
			break;

		default:
			log("net: Invalid message type");
			break;

		}
	}

	if ((oldstate == RXS_OPENING) && (State == RXS_OPEN)) {

		prepare_service();

		/* catch child's death */
		(void) signal(SIGCLD, sigch_hand);

		/* ready to fork */

		log("net: about to fork");
		if ((Child_pid = fork()) < 0) {
			log("net: fork failed");
			(void) rxsendopenreply(Net_fd, RXE_BADSTART, 0);
			exit(1);
		}

		if (Child_pid == 0)
			child_actions();

		(void) close(Slaveptty_fd);

		if (Pipe_open)
			(void) close(Childpipe_fd);
	}
}


/*
 * net_try_to_write()
 *
 * This routine attempts to write what's left of the data buffer
 * to the service.  If the buffer is written out in full, a
 * write acknowledgement message is sent to the client.
 *
 */

void
net_try_to_write()
{
	int	cc;

	cc = write(Ptty_fd, Data_bufferp, Data_buffer_len);

	if (cc <= 0)
		return;

	Data_bufferp += cc;

	if ((Data_buffer_len -= cc) == 0) {
		Buffer_empty++;
		(void) rxsendwriteack(Net_fd, 1);
	}

	return;
}


/*
 * prepare_service()
 *
 * This routine gets the service ready to run
 *
 */

static void
prepare_service()
{
	FILE	*sf;		/* service description file pointer */
	static RX_SERVICE svc;	/* service entry */
	int	found = 0;	/* found flag */
	int	envc;		/* environment count */
	struct passwd *passwd;	/* user info structure */
	int	efd;		/* environment file fd */
	struct stat statbuf;	/* environment file stat info */
	char	*efmem;		/* environment file memory buffer */
	char	*efcp;		/* environment file character pointer */
	char	*eflp;		/* environment file line pointer */
	int	incomment;	/* in comment flag */
	char	**rxportv;	/* array of rxported variables */
	char	rxportc;	/* count of rxported variables */

	(void) sprintf(Logmsg, "net: looking for service %s", Service);
	log(Logmsg);

	/* look for service definition */
	if ((sf = fopen(RX_SVCFILE, "r")) == (FILE *) NULL) {
		log("net: could not open services file");
		(void) rxsendopenreply(Net_fd, RXE_NOSVCFILE, 0);
		exit(1);
	}

	settarget(Service, Argstr);

	while(!found) {
		if (skipcmnt(sf) < 0)
			break;
		if (getsvcent(sf, &svc, 1) < 0)
			break;
		if (strcmp(svc.name, Service) == 0)
			found++;
	}

	(void) fclose(sf);

	if (!found) {
		log("net: service not found");
		(void) rxsendopenreply(Net_fd, RXE_NOSVC, 0);
		exit(1);
	}

	Svcargv = strtoargv(svc.def);

	(void) sprintf(Logmsg, "net: service path name is '%s'", Svcargv[0]);
	log(Logmsg);

	/* check if service is executable */
	if (access(Svcargv[0], F_OK | X_OK) < 0) {
		log("net: cannot execute command");
		(void) rxsendopenreply(Net_fd, (long) ((errno == ENOENT)?
					    RXE_NOSVC : RXE_NOTAUTH), 0);
		exit(1);
	}

	/* open pseudo ttys */
	if (makepttypair(svc.utmp) < 0) {
		/* makepttypair() makes appropriate log entry */
		(void) rxsendopenreply(Net_fd, RXE_NOPTS, 0);
		exit(1);
	}

	/* make error pipe, if needed */
	if (Flags & RXF_SEPERR) {
		if (makepipe() < 0) {
			log("net: cannot make pipe");
			(void) rxsendopenreply(Net_fd, RXE_PIPE, 0);
			exit(1);
		}
	}

	/* setup environment for the service */

	envc = 0;

	/* the following default variables are always set */

	strcpy(envhome, ENV_HOME);
	strcpy(envlogname, ENV_LOGNAME);
	strcpy(envshell, ENV_SHELL);

	if ((passwd = getpwuid(getuid())) != (struct passwd *) NULL) {
		Svcenvp[envc++] = strcat(envhome, passwd->pw_dir);
		(void) sprintf(Logmsg, "net: setting %s", Svcenvp[envc - 1]);
		log(Logmsg);
		Svcenvp[envc++] = strcat(envlogname, passwd->pw_name);
		(void) sprintf(Logmsg, "net: setting %s", Svcenvp[envc - 1]);
		log(Logmsg);
		Svcenvp[envc++] = strcat(envshell, passwd->pw_shell);
		(void) sprintf(Logmsg, "net: setting %s", Svcenvp[envc - 1]);
		log(Logmsg);
	} else {
		Svcenvp[envc++] = strcat(envshell, BINSHELL);
		(void) sprintf(Logmsg, "net: setting %s", Svcenvp[envc - 1]);
		log(Logmsg);
	}
	endpwent();

	strcpy(envpath, ENV_PATH);
	Svcenvp[envc++] = strcat(envpath, getenv(PATHVAR));
	(void) sprintf(Logmsg, "net: setting %s", Svcenvp[envc - 1]);
	log(Logmsg);

	strcpy(envtz, ENV_TZ);
	Svcenvp[envc++] = strcat(envtz, getenv(TZVAR));
	(void) sprintf(Logmsg, "net: setting %s", Svcenvp[envc - 1]);
	log(Logmsg);

	/* read the environment file, if any */

	if (Envfile[0] == '\0')
		goto skipfile;

	(void) sprintf(Logmsg,"net: using environment file %s", Envfile);
	log(Logmsg);

	if ((efd = open(Envfile, O_RDONLY)) < 0) {
		log("net: could not open environment file");
		goto skipfile;
	}

	if (fstat(efd, &statbuf) < 0) {
		log("net: could not stat environment file");
		(void) close(efd);
		goto skipfile;
	}

	if (statbuf.st_size <= 0) {
		log("net: environment file empty");
		(void) close(efd);
		goto skipfile;
	}

	/*
	 *	need to allocate one extra byte in case the file does
	 *	not end with a newline
	 */

	efmem = (char *) malloc(statbuf.st_size) + 1;

	if (read(efd, efmem, statbuf.st_size) != statbuf.st_size) {
		log("net: error reading environment file");
		(void) close(efd);
		goto skipfile;
	}

	(void) close(efd);

	efcp = efmem;
	eflp = efmem;
	incomment = 0;

	while((efcp - efmem) <= statbuf.st_size) {

		/* detect commented out lines */

		if ((efcp == eflp) && (*efcp == '#'))
			incomment++;

		if (*efcp == '\n') {
			if (!incomment) {
				*efcp = '\0';
				Svcenvp[envc++] = eflp;
				(void) sprintf(Logmsg, "net: setting %s",
					       Svcenvp[envc - 1]);
				log(Logmsg);
			} else
				incomment = 0;
			eflp = efcp + 1;
		}
		efcp++;
	} /* while */

	*efcp = '\0';

	skipfile:

	/* introduce RXPORTed environment variables */

	rxportv = strtoargv(Envstr);
	rxportc = 0;

	while(rxportv[rxportc] != NULL) {
		Svcenvp[envc++] = rxportv[rxportc++];
		(void) sprintf(Logmsg, "net: setting %s", Svcenvp[envc - 1]);
		log(Logmsg);
	} /* while */
	
	Svcenvp[envc] = NULL;

	/* if we are here, then service is ready to be run */
	(void) rxsendopenreply(Net_fd, RXE_OK, RX_INITCREDIT);
}


/*
 * rxsendopenreply()
 *
 * This routine sends an OPEN_REPLY message to the client
 *
 */

static int
rxsendopenreply(netfd, ret_code, credit)
int	netfd;
long	ret_code;
long	credit;
{
	struct open_reply Open_reply;

	(void) sprintf(Logmsg, "net: openreply: netfd = %d, ret_code = %ld",
		netfd, ret_code);
	log(Logmsg);

	Open_reply.version = htonl(RX_VERSION);
	Open_reply.ret_code = htonl((int) ret_code);
	Open_reply.credit = htonl((int) credit);

	if (rxputm(netfd, RXM_OPEN_REPLY, sizeof(struct open_reply),
		   (char *) &Open_reply) < 0) {
		return(-1);
	}

	log("net: openreply: sent RXM_OPEN_REPLY message to client");

	return(0);
}


/*
 * rxsendcloseta()
 *
 * This routine sends a CLOSE_TA message to the client
 *
 */

static int
rxsendcloseta(netfd, tasize, tabuf)
int	netfd;
long	tasize;
char	*tabuf;
{
	struct close_ta Close_ta;

	(void) sprintf(Logmsg, "net: closeta: netfd = %d, tasize = %ld",
		netfd, tasize);
	log(Logmsg);

	Close_ta.tasize = htonl((int) tasize);
	(void) memcpy(Close_ta.tabuf, tabuf, (unsigned int) tasize);

	if (rxputm(netfd, RXM_CLOSE_TA, sizeof(struct close_ta),
		   (char *) &Close_ta) < 0) {
		log("net: closeta: rxputm(RXM_CLOSE_TA) failed");
		return(-1);
	}

	return(0);
}


/*
 * rxsendwriteack()
 *
 * This routine sends a WRITEACK message to the client
 *
 */

static int
rxsendwriteack(netfd, credit)
int	netfd;
long	credit;
{
	struct writeack_msg	writeack_msg;

	(void) sprintf(Logmsg,
		"net: writeack: netfd = %d, credit = %ld, child_died = %d",
		netfd, credit, Child_died);
	log(Logmsg);

	if (Child_died)
		/* do not ack any more messages */
		writeack_msg.credit = htonl((int) 0);
	else
		writeack_msg.credit = htonl((int) credit);

	if (rxputm(netfd, RXM_WRITEACK, sizeof(struct writeack_msg),
		   (char *) &writeack_msg) < 0) {
		log("net: writeack: rxputm(RXM_WRITEACK) failed");
		return(-1);
	}

	return(0);
}


/*
 * rxsendsignalack()
 *
 * This routine sends a SIGNALACK message to the client
 *
 */

static int
rxsendsignalack(netfd, signum)
int	netfd;
long	signum;
{
	struct signalack_msg	signalack_msg;

	(void) sprintf(Logmsg,
		"net: signalack: netfd = %d, signum = %ld", netfd, signum);
	log(Logmsg);

	signalack_msg.sig = htonl(signum);

	if (rxputm(netfd, RXM_SIGNALACK, sizeof(struct signalack_msg),
		   (char *) &signalack_msg) < 0) {
		log("net: signalack: rxputm(RXM_SIGNALACK) failed");
		return(-1);
	}

	return(0);
}
