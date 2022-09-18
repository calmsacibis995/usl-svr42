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

#ident	"@(#)rexec:rxs_ptty.c	1.5.2.3"
#ident  "$Header: rxs_ptty.c 1.2 91/06/27 $"

#include <sys/byteorder.h>
#include <termio.h>
#include <stropts.h>
#include <sys/stream.h>
#include <fcntl.h>
#include <utmpx.h>
#include <sys/types.h>
#include <pwd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <rx.h>
#include <unistd.h>
#include "rxmsg.h"


/* externally defined routines */

extern	int	grantpt();
extern	int	unlockpt();
extern	char	*ptsname();
extern	void	log();
extern	int	rxputm();


/* local routines */

static	int	rxsendioctl();


/* externally defined global variables */

extern	int	Net_fd;		/* network connection fd */
extern	char	Logmsg[];
extern	int	Termios_received; /* received termios structure flag */
extern	struct termios	Termios;  /* termios buffer */
extern	long	Flags;		/* service flags */

#define	RX_SLAVENAMELEN	32


/* locally defined global variables */

int	Ptty_open;	/* ptty open flag */
int	Ptty_fd;	/* master ptty fd */
int	Slaveptty_fd;	/* slave ptty fd */
char	Slaveptty_name[RX_SLAVENAMELEN]; /* slave ptty device file name */


/*
 * makepttypair()
 *
 * This function creates and opens a master/slave pair of pseudo ttys.
 * It returns 0 for success, -1 for failure
 *
 */

int
makepttypair(u)
char	*u;			/* utmp field from service definition */
{
	char	*ttyname;		/* file name of slave pseudo tty */
	int	makeut = (u[0] == 'u');	/* make utmp entry flag */
	int	found = 0;		/* utmp entry found flag */
	struct utmpx ut, *utp;		/* utmp entry structures */
	pid_t	mypid = getpid();	/* my process id */
	struct passwd *pwp;		/* password file entry */

	if ((Ptty_fd = open("/dev/ptmx", O_RDWR)) == -1) {
		(void) sprintf(Logmsg,
			       "ptty: open ptmx failed, errno = %d",
			       errno);
		log(Logmsg);
		return(-1);
	}
	if (grantpt(Ptty_fd) == -1) {
		log("ptty: grantpt failed");
		return(-1);
	}
	if (unlockpt(Ptty_fd) == -1) {
		log("ptty: unlockpt failed");
		return(-1);
	}
	if ((ttyname = ptsname(Ptty_fd)) == NULL) {
		log("ptty: ptsname failed");
		return(-1);
	}
	(void) strncpy(Slaveptty_name, ttyname, RX_SLAVENAMELEN);
	if ((Slaveptty_fd = open(Slaveptty_name, O_RDWR)) == -1) {
		(void) sprintf(Logmsg,
			       "ptty: could not open pts %s, errno = %d",
			       Slaveptty_name, errno);
		log(Logmsg);
		return(-1);
	}

	if (ioctl(Slaveptty_fd, I_PUSH, "ptem") == -1) {
		log("ptty: push ptem failed");
		return(-1);
	}
	if (ioctl(Slaveptty_fd, I_PUSH, "ldterm") == -1) {
		log("ptty: push ldterm failed");
		return(-1);
	}

	/*
	 *	Note that since the following ioctl() is performed
	 *	before the pckt module is pushed, it will not be sent back
	 *	to the client.  This is what we want.
	 *
	 */

	if (Termios_received)
		if (ioctl(Slaveptty_fd, TCSETS, &Termios) == -1) {
			log("ptty: TCSETS failed");
			return(-1);
		}

	if (ioctl(Ptty_fd, I_PUSH, "pckt") == -1) {
		log("ptty: push pckt failed");
		return(-1);
	}

	/* disable ldterm input processing on server end */
	if (ioctl(Ptty_fd, TIOCREMOTE, 1) == -1) {
		log("ptty: ioctl(TIOCREMOTE) failed");
		return(-1);
	}

	Ptty_open = 1;

	log("ptty: pttys created");

	/* update utmp entry */

	log("ptty: updating utmp entry");

	while((!found) && ((utp = getutxent()) != (struct utmpx *) NULL)) {
		found = (utp->ut_pid == mypid);
	}

	if (!found) {
		log("ptty: could not find utmp entry to update");
		endutxent();
		goto skiputmp;
	}

	memcpy(&ut, utp, sizeof(ut));

	/*
	 *	if utmp flag is set in the service definition,
	 *	fill in the user and line fields in the utmp entry,
	 *	otherwise, make utmp entry invisible to the who command
	 *	by changing the entry type to INIT_PROCESS
	 *
 	 */

	if (makeut) {
		if ((pwp = getpwuid(getuid())) == (struct passwd *) NULL) {
			log("ptty: could not get my login name");
			endutxent();
			goto skiputmp;
		}

		strcpy(ut.ut_user, pwp->pw_name);
		strcpy(ut.ut_line, Slaveptty_name + strlen("/dev/"));
	} else {
		ut.ut_type = INIT_PROCESS;
	}

	if (pututxline(&ut) == (struct utmpx *) NULL) {
		log("ptty: utmp update failed");
		endutxent();
		goto skiputmp;
	}

	endutxent();

	log("ptty: utmp entry updated");

	skiputmp:

	endpwent();

	return(0);
}

void
ptty_hup()
{
	log("ptty: HUP from ptty");
	Ptty_open = 0;
}


/*
 * ptty_msg()
 *
 * This routine processes messages coming from the master ptty
 *
 */

void
ptty_msg()
{
	struct strbuf	ctl;
	struct strbuf	data;
	u_char		ctlbuf;
	char		databuf[RX_MAXDATASZ];
	int		flags = 0;
	struct iocblk	ioc;
	struct termio	tio;
	struct termios	tios;
	struct data_msg	data_msg;
	int		gm_ret;
	int		int_val;	/* for single int value ioctls */
	struct winsize	win;

	log("ptty: data from ptty");

	ctl.maxlen = 1;
	ctl.buf = (char *) &ctlbuf;
	data.maxlen = RX_MAXDATASZ;
	data.buf = databuf;
	if ((gm_ret = getmsg(Ptty_fd, &ctl, &data, &flags)) < 0) {
		(void) sprintf(Logmsg, "ptty: getmsg() failed, errno = %d",
			       errno);
		log(Logmsg);
		return;
	}
	log("ptty: Got a message from the application");
	switch(ctlbuf) {

	case M_DATA:
		(void) sprintf(Logmsg,
			       "ptty: \tM_DATA message, len = %d", data.len);
		log(Logmsg);
		if (data.len == 0) {
			log("ptty: 0-length message, treating as HUP");
			ptty_hup();
			return;
		}
		data_msg.fd = htonl(1);		/* fd for stdout */
		data_msg.len = htonl(data.len);
		(void) memcpy(data_msg.buf, data.buf, (unsigned int) data.len);
		(void) rxputm(Net_fd, RXM_DATA, sizeof(data_msg),
			      (char *) &data_msg);
		while(gm_ret & MOREDATA) {
			ctl.maxlen = 0; /* no ctrl part */
			ctl.buf = (char *) &ctlbuf;
			data.maxlen = RX_MAXDATASZ;
			data.buf = databuf;
			if ((gm_ret = getmsg(Ptty_fd, &ctl, &data, &flags))
			    < 0) {
				(void) sprintf(Logmsg,
					"ptty: getmsg(MOREDATA) failed, errno = %d",
					errno);
				log(Logmsg);
				return;
			}
			(void) sprintf(Logmsg,
				       "ptty: \tadditional DATA message, len = %d",
				       data.len);
			log(Logmsg);
			data_msg.len = htonl(data.len);
			(void) memcpy(data_msg.buf, data.buf,
				      (unsigned int) data.len);
			(void) rxputm(Net_fd, RXM_DATA, sizeof(data_msg),
				      (char *) &data_msg);
		}
		break;

	case M_IOCTL:
		log("ptty: \tM_IOCTL message, ioctl type - ");
		memcpy(&ioc, data.buf, sizeof(struct iocblk));

		switch(ioc.ioc_cmd) {

		case TCSETS:
		case TCSETSW:
		case TCSETSF:
			log("ptty: TCSETS");
			memcpy(&tios, data.buf + sizeof(struct iocblk), sizeof(struct termios));
			(void) sprintf(Logmsg, "ptty: iflag = 0%o",
				       tios.c_iflag);
			log(Logmsg);
			(void) sprintf(Logmsg, "ptty: oflag = 0%o",
				       tios.c_oflag);
			log(Logmsg);
			(void) sprintf(Logmsg, "ptty: cflag = 0%o",
				       tios.c_cflag);
			log(Logmsg);
			(void) sprintf(Logmsg, "ptty: lflag = 0%o",
				       tios.c_lflag);
			log(Logmsg);

			if (!(Flags & RXF_STDOUTTERM))
				/* do not reset these on the client */
				tios.c_oflag |= (OPOST | ONLCR | XTABS);

			tios.c_iflag = htonl((int) tios.c_iflag);
			tios.c_oflag = htonl((int) tios.c_oflag);
			tios.c_cflag = htonl((int) tios.c_cflag);
			tios.c_lflag = htonl((int) tios.c_lflag);
			(void) rxsendioctl((long) ioc.ioc_cmd,
				    (long) sizeof(struct termios), (char *) &tios);
			break;

		case TCSETA:
		case TCSETAW:
		case TCSETAF:
			log("ptty: TCSETA");
			memcpy(&tio, data.buf + sizeof(struct iocblk), sizeof(struct termio));
			(void) sprintf(Logmsg, "ptty: iflag = 0%o",
				       tio.c_iflag);
			log(Logmsg);
			(void) sprintf(Logmsg, "ptty: oflag = 0%o",
				       tio.c_oflag);
			log(Logmsg);
			(void) sprintf(Logmsg, "ptty: cflag = 0%o",
				       tio.c_cflag);
			log(Logmsg);
			(void) sprintf(Logmsg, "ptty: lflag = 0%o",
				       tio.c_lflag);
			log(Logmsg);

			if (!(Flags & RXF_STDOUTTERM))
				/* do not reset these on the client */
				tio.c_oflag |= (OPOST | ONLCR | XTABS);

			tio.c_iflag = htons((int) tio.c_iflag);
			tio.c_oflag = htons((int) tio.c_oflag);
			tio.c_cflag = htons((int) tio.c_cflag);
			tio.c_lflag = htons((int) tio.c_lflag);
			(void) rxsendioctl((long) ioc.ioc_cmd,
				    (long) sizeof(struct termio), (char *) &tio);
			break;

		case TCSBRK:
			log("ptty: TCSBRK");
			int_val = *((int *) (data.buf + sizeof(struct iocblk)));
			int_val = htonl(int_val);
			(void) rxsendioctl((long) ioc.ioc_cmd,
				    (long) sizeof(int), (char *) &int_val);
			break;

		case TIOCSWINSZ:
			log("ptty: TIOCSWINSZ");
			memcpy(&win, data.buf + sizeof(struct iocblk), sizeof(struct winsize));
			win.ws_row = htons((int) win.ws_row);
			win.ws_col = htons((int) win.ws_col);
			win.ws_xpixel = htons((int) win.ws_xpixel);
			win.ws_ypixel = htons((int) win.ws_ypixel);
			(void) rxsendioctl((long) ioc.ioc_cmd,
				    (long) sizeof(struct winsize), (char *) &win);
			break;

		default:
			(void) sprintf(Logmsg, "ptty: other (x%x)",
				       ioc.ioc_cmd);
			log(Logmsg);
			break;
		}
		break;

	case M_FLUSH:
		/* could log this in the future */
		break;

	default:
		(void) sprintf(Logmsg,
			       "ptty: \tunexpected message type (%d) from pckt - ignored",
			       ctlbuf);
		log(Logmsg);
		break;
	}
}


/*
 * rxsendioctl()
 *
 * This routine sends an IOCTL message to the client
 *
 */

static int
rxsendioctl(iocnum, arglen, arg)
long	iocnum;
long	arglen;
char	*arg;
{
	struct ioctl_msg ioctl_msg;

	ioctl_msg.fd = htonl(1);
	ioctl_msg.ioc = htonl((int) iocnum);
	ioctl_msg.arglen = htonl((int) arglen);
	(void) memcpy(ioctl_msg.arg, arg, (unsigned int) arglen);
	return(rxputm(Net_fd, RXM_IOCTL, sizeof(ioctl_msg),(char *) &ioctl_msg));
}
