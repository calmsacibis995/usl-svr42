/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ttymon:common/cmd/ttymon/tmhandler.c	1.14.19.5"
#ident  "$Header: tmhandler.c 1.2 91/06/24 $"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <termio.h>
#include <signal.h>
#include <wait.h>
#include <priv.h>
#include <sys/types.h>
#include <sys/stropts.h>
#include <sys/termios.h>
#include <pfmt.h>
#include "ttymon.h"
#include <sys/stream.h>
#include <sys/tp.h>
#include "tmstruct.h"
#include "tmextern.h"
#include "sac.h"

extern	int	Retry;
static	void	tp_got_msg();
static	struct	pmtab	*find_pid();
static	void	kill_children();
static	void	do_pmtab_changed();

static 	struct	pmtab	*find_fd();
static	struct	pmtab	*tp_find_fd();
static	void		mark_service();
static	void		queue_pid();
static	struct	pidtab	*dequeue_pid();
extern  void	sigalarm();
extern	void	tmchild();
extern	void	alloc_pollarray();
extern	void	tpclose_ctrl();			/* see ttymon.c */
extern	void	tpclose_device();		/* see ttymon.c */
extern	void	tpreinit_realdev();		/* see ttymon.c */
extern	int	tpopen_data();			/* see ttymon.c */
extern	int	tpctrl_termio();		/* see tmterm.c */

static	char	Tp_errormsg[256];
static	int	Tp_errno;
static	struct	sak nullsak;
static	struct	termios nulltermios;
static	struct	pidtab *pidtab = NULL;	/* head pointer to pidtab linked lint	*/




extern	void	tp_geterror();
extern	int	tp_defsak();
extern	int	tp_datadisconnect();

extern	int	getmsg();

extern const char badopen[], badpoll[], efailed[], badopendata[];
const  char
	errmsg[] = ":856:%s: %s",
	badtptermio[] = ":857:%s: tpctrl_termio() failed on ctrl channel for (%s)";

/*
 * Procedure:	  fork_tmchild
 *
 * Restrictions:
                 strerror: none
*/

/*
 *	fork_tmchild	- fork child on the device
 */
static	void
fork_tmchild(pmptr)
struct	pmtab	*pmptr;
{
	pid_t	pid;
	sigset_t	cset;
	sigset_t	tset;
	struct pmtab	*pmp;

#ifdef	DEBUG
	debug("in fork_tmchild");
#endif
	pmptr->p_inservice = FALSE;

	/* protect following region from SIGCLD and SIGPOLL*/
	(void)sigprocmask(SIG_SETMASK, NULL, &cset);
	tset = cset;
	(void)sigaddset(&tset, SIGCLD);
	(void)sigaddset(&tset, SIGPOLL);
	(void)sigprocmask(SIG_SETMASK, &tset, NULL);
	if( (pid=fork()) == 0 ) {
	 	/* The CHILD */
		/*
		 * Close all reference to Trusted Path ctrl channels.
		 * The tmchild has no need to access and should never
		 * have any TP ctrl channels open because if the parent
		 * ttymon closes a TP ctrl channel it expects it to be
		 * disconnected from the TP device.  If a tmchild were
		 * to keep the TP ctrl channel open and a tmchild existed
		 * when the parent ttymon closed a TP ctrl channel and the
		 * parent ttymon subsequently attempts to connect another TP
		 * ctrl channel to the TP device, before the tmchild has
		 * execed into a service, the connect will fail since the
		 * TP ctrl channel was held open by tmchild and was never
		 * disconnected from the TP device when the parent ttymon
		 * closed the original TP ctrl channel.
		 *
		 * Also close all reference to TP data channels except for
		 * the one pointed to by pmptr.
		 */
		for (pmp = PMtab; pmp; pmp = pmp->p_next) {
			if (pmp->p_tpctrlfd != 0){
				(void)close(pmp->p_tpctrlfd);
				pmp->p_tpctrlfd = 0;
			}
			if ((pmp != pmptr) && (pmp->p_fd != 0)){
				(void)close(pmp->p_fd);
				pmp->p_fd = 0;
			}
		}
		tmchild(pmptr); 
		/* tmchild should never return */
		logexit(1, ":630:tmchild() for <%s> returns unexpectedly", pmptr->p_device);
	}
	else if (pid < 0) {
		log(MM_ERROR, efailed, "fork()", strerror(errno));
		pmptr->p_status = VALID;
		pmptr->p_pid = 0;
		Retry = TRUE;
	}
	else {
		/*
		 * The PARENT - store pid of child and close the device
		 */
		pmptr->p_pid = pid;
	}
	if (pmptr->p_fd > 0) {
		(void)close(pmptr->p_fd); 
		pmptr->p_fd = 0; 
	}
	(void)sigprocmask(SIG_SETMASK, &cset, NULL);
}

/*
 * got_carrier - carrier is detected on the stream
 *	       - depends on the flags, different action is taken
 *	       - R_FLAG - wait for data
 *	       - C_FLAG - if port is not disabled, fork tmchild
 *	       - A_FLAG - wait for data 
 *	       - otherwise - write out prompt, then wait for data
 */
void
got_carrier(pmptr)
struct	pmtab	*pmptr;
{
	flush_input(pmptr->p_fd);
	if (pmptr->p_ttyflags & R_FLAG) {
		return;
	}
	else if ((pmptr->p_ttyflags & C_FLAG) &&
		(State != PM_DISABLED) &&
		(!(pmptr->p_flags & X_FLAG))) {
		fork_tmchild(pmptr);
	}
	else if (pmptr->p_ttyflags & A_FLAG) {
		return;
	}
	else if (pmptr->p_timeout) {
		fork_tmchild(pmptr);
	}
	else {
		write_prompt(pmptr->p_fd,pmptr,TRUE,TRUE);
	}
}

/*
 * got_data - data is detected on the stream, fork tmchild
 */
static void
got_data(pmptr)
struct	pmtab	*pmptr;
{
	fork_tmchild(pmptr);
}

/*
 * got_hup - stream hangup is detected, close the device
 */
static void
got_hup(pmptr)
struct	pmtab	*pmptr;
{
#ifdef	DEBUG
	debug("in got hup");
#endif
	/* -hangup may have been detected on the ctrl channel first and the
	**  file descriptor p_fd closed
	** -so check for p_fd > 0 before calling close since we do not want
	**  to close file descriptor 0
	*/
	if (pmptr->p_fd > 0){
		(void)close(pmptr->p_fd);
		pmptr->p_fd = 0;
		pmptr->p_inservice = 0;
		pmptr->p_tpdataconnid = 0;
		Retry = TRUE;
	}
}

/*
 * Procedure:	  tp_got_msg
 *
 * Restrictions:
		 getmsg(2): none
		 strerror: none
		 tp_datadisconnect: P_MACWRITE
*/


/* tp_got_msg(pmptr)
**
** -gets TP protocol messages from the ctrl channel which will indicate either
**	-a SAK has been detected by TP
**	 OR
**	-hangup has been detected by TP
**
**
** IF a HANGUP is detected
**	IF ttymon "owns" the data channel
**		-disconnect the data channel
**	ELSE
**		-ignore HANGUP since some other application (eg. the connection
**		 server) "owns" the data channel
**
**	-Handle any changes to the pmtab if p_status == CHANGED
**	-Close ctrl change and purge pmtab entry if p_status == NOTVALID
**	IF SAK for device is defined as NONE and device is not bi-directional
**		-set p_tpstatus to tpTRUSTEDSTATE (this allow a data channel
**		 to be connected without waiting for a pseudo SAK to be
**		 detected).
**
**
** IF a SAK is detected
**	-disconnect the data channel
**	-Handle any changes to the pmtab (p_status == CHANGED)
**	IF p_status == NOTVALID
**		-Close ctrl change and purge pmtab entry
**	ELSE
**		-set p_tpstatus to tpTRUSTEDSTATE 
**			-this will allow open_all() (in ttymon.c) to call
**			 open_device()
**			-open_device() sets up the data channel so
**			 Identification and Authentication of the user can begin
**
**	NOTE: If SAK is entered to login (p_pid == 0 && p_inservice == FALSE)
**		the TP mux is not dismantled.  If SAK is entered to logout
**		(p_pid != 0 && p_inservice == TRUE) the mux is dismantled.
**		This distintion is important for terminals that access the
**		system via modems or equipment that emulates modem signals.
**		When entering the SAK for login, the TP mux is already
**		configured.  If the TP mux was then dismantled, DTR on the
**		systems comminications board would be dropped, which would in
**		turn cause the modem to drop it's connection.
*/

static void
tp_got_msg(pmptr)

struct	pmtab *pmptr;
{

	struct	tpproto tpproto;	/* buffer for retrieving TP protocol */ 
	struct	tp_info tpinf;
	struct	strbuf	ctlbuf = {	/* buffer for getmsg(2) */
		sizeof(struct tpproto),	/* maxlen  max buffer length */
		0,			/* len     length of data returned */
		(char *)NULL		/* *buf    pointer to buffer */
	};
	int	flag=0;			/* flag for getmsg(2) */

	sigset_t	oset;		/* to save current mask of blocked
					** signals
					*/
	sigset_t	nset;		/* to define new mask of blocked
					** signals for duration of function
					*/
	int	unlinkmux = FALSE;	/* Flag to indicate whether or not to
					** dismantle TP mux after a SAK is
					** detected.  If SAK was entered to
					*/


	/* -critical code section: need to hold SIGCLD since this function
	**  alters data structures that sig_child() relies on and alters
	*/
	(void)sigprocmask(SIG_SETMASK, (sigset_t *)NULL, &oset);
	nset = oset;
	(void)sigaddset(&nset, SIGCLD);
	(void)sigprocmask(SIG_SETMASK, &nset, (sigset_t *)NULL);


	/* -get message from ctrl channel
	*/

	ctlbuf.buf = (char *)&tpproto;

	while (getmsg(pmptr->p_tpctrlfd, &ctlbuf, (struct strbuf *)NULL,&flag)
	 == -1){
		if (errno == EINTR){
			continue;
		}else if (errno == EAGAIN){
#ifdef DEBUG
			dlog("tp_got_msg:getmsg failed (%s) realdev = %s\n",
			strerror(errno), pmptr->p_realdevice);
#endif
			(void)sigprocmask(SIG_SETMASK, &oset, (sigset_t *)NULL);
			return;
		}else if (errno == EBADMSG){
			/*
			** -something wrong with the Stream, so dismantle
			**  multiplexor.  Set Retry to TRUE so the TP mux
			**  will be re-assembled again.
			*/
			Retry = TRUE;
			tpclose_device(pmptr);
		}
		log(MM_ERROR, ":858:%s: getmsg() failed (%s) realdev = %s",
                "tp_got_msg",strerror(errno), pmptr->p_realdevice);
		(void)sigprocmask(SIG_SETMASK, &oset, (sigset_t *)NULL);
		return;
	}
#ifdef DEBUG
		dlog("tp_got_msg:got to switch(tpproto.tpp_type)");
		dlog(
		 "\tctlbuf.maxlen = %d\n\tctlbuf.len = %d\n\ttpproto.tpp_type = %d",
		 ctlbuf.maxlen,ctlbuf.len,tpproto.tpp_type);
#endif


	switch (tpproto.tpp_type){

	case TP_M_HANGUP:

		/* IF ttymon "owns" or initially "owned" the data channel
		**	-disconnect data channel
		**	-reset initial termio settings on the real device
		** ELSE {some other application "owns" the data channel}
		**	-ignore hangup: do not want to interrupt service if
		**	 data channel is not "owned" by ttymon
		**
		** -ownership is detected by the following
		**
		**	IF (p_fd > 0)
		**		-indicates ttymon has data channel open for
		**		 polling
		**		-NOTE: also the following should be true
		**		 (p_pid == 0 && p_inservice != TRUE &&
		**		  p_tpdataconnid != 0)
		**
		**	ELSE IF (p_pid > 0)
		**		-indicates either tmchild OR the service
		**		 tmchild execed {if p_inservice == TRUE}
		**		 has control over the data channel
		**		-NOTE: also the following should be true
		**		 (p_tpdataconnid != 0)
		**
		** -NOTE: in the case where ttymon is monitoring both the ctrl
		**  and data channel (i.e. p_trctrlfd > 0 && p_fd > 0), and
		**  a hangup occurs, both the ctrl and data channel will be
		**  notified of the hangup
		**
		**	IF the data channel hangup is processed first
		**		-got_hup() will close the data channel (p_fd)
		**		 and set p_fd = 0 and p_pid = 0
		**		-when the hangup for the ctrl channel is
		**		 processed in this function, the hangup will
		**		 be ignored since p_fd == 0 and p_pid == 0
		**		-this condition is OK since ttymon had the
		**		 only open file descriptor to the data channel
		**		 and the data channel was automatically
		**		 disconnected upon the last close in got_hup()
		*/

		if (pmptr->p_status == GETTY){
			exit(1);
		}
		if ( (pmptr->p_fd > 0) || (pmptr->p_pid > 0) ){
			if (pmptr->p_fd > 0){
				/* NOTE: (p_pid == 0) && (p_inservice != TRUE)
				** && (p_tpdataconnid != 0 ) should be true
				*/
				(void)close(pmptr->p_fd);
				pmptr->p_tpdataconnid = 0;
			}
			else if (pmptr->p_pid > 0){
				/* NOTE: (p_fd == 0) && (p_tpdataconnid != 0 )
				** should be true
				*/

				if (pmptr->p_tpdataconnid > 0){
					TP_LOADINF(tpinf, (dev_t)-1, (dev_t)-1,
					 0, 0, (dev_t)-1, (dev_t)-1, (dev_t)-1,
					 0, 0, pmptr->p_tpdataconnid, 0,
					 nullsak, 0, nulltermios, nulltermios);

					(void) procprivl(CLRPRV, MACWRITE_W, 0);
					if (tp_datadisconnect(pmptr->p_tpctrlfd,
					 &tpinf) == -1){
						(void) procprivl(SETPRV, MACWRITE_W, 0);
						tp_geterror(&Tp_errno,
						 sizeof(Tp_errormsg),&Tp_errormsg);
						if (Tp_errno != ENXIO){
							log(MM_ERROR,
							 ":859:%s: %s: %s",
							 "tp_got_msg",Tp_errormsg, strerror(Tp_errno));
						}
					}
					(void) procprivl(SETPRV, MACWRITE_W, 0);
				}else{
					/* NOTE: in the future may want to do a
					** hard tp_datadisconnect.
					**
					** For now if a data channel is
					** connected even though p_tpdataconnid
					** does not indicate that fact,
					** tp_dataconnect() in
					** open_device() will fail. {This case
					** will occur only if the SAK is defined
					** as NONE}
					*/
					log(MM_ERROR, 
					 ":860:%s: no data connection id for fid %d",
					 "tp_got_msg",pmptr->p_pid);
				}

				/* -queue pid on pidtab.  It will be cleaned up
				**  by sigchild()
				*/
				queue_pid(pmptr);
			}
			/* -reset termio settings on the ctrl channel
			*/
			if (tpctrl_termio(pmptr->p_tpctrlfd, pmptr) == -1){
				log(MM_ERROR, badtptermio, "tp_got_msg:TP_M_HANGUP",pmptr->p_realdevice);
				tpclose_ctrl(pmptr);
			}
			/* IF saktype == saktypeNONE && ttyflags != B_FLAG
			**	-mark tpstatus as if it were in a trusted state
			**	 tpTRUSTEDSTATE
			**	-setting this status here will by-pass ttymon
			**	 checks that require a SAK to be entered before
			**	 a TP can be instantiated
			*/
			if ((pmptr->p_sak.sak_type == saktypeNONE) &&
			 (!(pmptr->p_ttyflags & B_FLAG)))
			        pmptr->p_tpstatus = tpTRUSTEDSTATE;

			tpreinit_realdev(pmptr);

			pmptr->p_fd = 0;
			pmptr->p_inservice = 0;
			pmptr->p_pid = 0;
			pmptr->p_tpdataconnid = 0;
		}
		/* ELSE
		**	-ignore hangup
		**	-NOTE: p_tpdataconnid should be == 0
		*/


		if (pmptr->p_status == CHANGED){
			do_pmtab_changed(pmptr);
			pmptr->p_reason = 0;
			pmptr->p_status = VALID;
		}else if (pmptr->p_status == NOTVALID){
			tpclose_device(pmptr);
			purge();
		}
		break;


	case TP_M_SAK:

		/* -sak detected
		** -disconnect data channel under all circumstances by
		**  specifying connid of 0 in tp_datadisconnect()
		*/ 
		TP_LOADINF(tpinf, (dev_t)-1, (dev_t)-1, 0, 0, (dev_t)-1,
		 (dev_t)-1, (dev_t)-1, 0, 0, 0, 0, nullsak, 0, nulltermios,
		 nulltermios);
		(void) procprivl(CLRPRV, MACWRITE_W, 0);
		while (tp_datadisconnect(pmptr->p_tpctrlfd, &tpinf) != 0){
			(void) procprivl(SETPRV, MACWRITE_W, 0);
			tp_geterror(&Tp_errno, sizeof(Tp_errormsg), Tp_errormsg);
			if (Tp_errno == EINTR) {
				continue;
			}
			else if (Tp_errno != ENXIO){
				log(MM_ERROR, errmsg, "tp_got_msg",Tp_errormsg);
			}
			break;
		}
		(void) procprivl(SETPRV, MACWRITE_W, 0);

		if (pmptr->p_status == GETTY){
			pmptr->p_tpstatus = tpTRUSTEDSTATE;
			return;
		}

		if (pmptr->p_fd > 0)
			(void)close(pmptr->p_fd);
		if (pmptr->p_pid > 0)
			/* -queue pid on pidtab.  It will be cleaned up
			**  by sigchild()
			*/
			queue_pid(pmptr);


		/* determine whether the TP mux should be dismantled or not */

		if ((pmptr->p_pid != 0) && (pmptr->p_inservice == TRUE))
			unlinkmux = TRUE;

		pmptr->p_fd = 0;
		pmptr->p_inservice = 0;
		pmptr->p_pid = 0;
		pmptr->p_tpdataconnid = 0;

		if (pmptr->p_status == CHANGED){
			do_pmtab_changed(pmptr);
			pmptr->p_reason = 0;
			pmptr->p_status = VALID;
		}
		if (pmptr->p_status == NOTVALID){
			tpclose_device(pmptr);
			purge();
		}else{
			if (pmptr->p_tpctrlfd > 0)
				pmptr->p_tpstatus = tpTRUSTEDSTATE;
		}
		tpreinit_realdev(pmptr);
		/*if (unlinkmux == TRUE)
			tpclose_device(pmptr);*/

		break;
	default:
		log(MM_ERROR, ":861:%s: invalid tp protocol type = %d",
			"tp_got_msg",tpproto.tpp_type);
		break;
	}

	Retry = TRUE;
	(void)sigprocmask(SIG_SETMASK, &oset, (sigset_t *)NULL);
	return;
}



/*
 * Procedure:	  queue_pid
 *
 * Restrictions:
                 strerror: none
*/

/* queue_pid(pmptr)
**
** -queue pmtab entry p_pid and p_flags value on an entry in the pidtab
*/
static void
queue_pid(pmptr)
	struct	pmtab *pmptr;
{
	struct	pidtab *pidptr;

#ifdef	DEBUG
	debug("in queue_pid()");
#endif

	if ((pidptr = ALLOC_PIDTAB) == (struct pidtab *)NULL)
		logexit(1, ":894:%s:Out of memory: %s", "queue_pid",
		 strerror(errno));

	pidptr->p_pid = pmptr->p_pid;
	pidptr->p_flags = pmptr->p_flags;
	pidptr->p_next = pidtab;
	pidtab = pidptr;
}

 
/*
 * Procedure:	  do_poll
 *
 * Restrictions:
		 strerror: none
*/

/*
**	do_poll()
**
**	-poll ctrl and data channels
**	-NOTE: data is not polled unless
**		1. TP is in a trusted state (indicated by
**		   p_tpstatus == tpTRUSTEDSTATE
**		2. default SAK is saktypeNONE
**	IF POLLPRI OR POLLIN received on ctrl channel
**		-read ctrl channel for SAK or HANGUP
**	IF POLLIN received on data channel
**		-fork tmchild
**	IF POLLHUP received on data channel
**		-close data channel
*/
void
do_poll(fdp,nfds)
struct 	pollfd *fdp; 
int 	nfds;
{
	int	i,n;
	struct	pmtab	*pmptr;
	struct	pollfd	*pollp;
	struct	pollfd	tpctrl_pollfd;
	sigset_t	oset;		/* to save current mask of blocked
					** signals
					*/
	sigset_t	nset;		/* to define new mask of blocked
					** signals for duration of function
					*/

	(void)sigprocmask(SIG_SETMASK, (const sigset_t *)NULL, &oset);
	nset = oset;
	(void)sigaddset(&nset, SIGCLD);

	n = poll(fdp, (unsigned long)nfds, -1);	/* blocked poll */

	/* -critical code section: SIGCLD needs to be held to prevent a race
	**  condition.
	**
	**  When the TP driver detects a SAK or Hangup, a notification of that
	**  fact is sent up the ctrl channel and a M_HANGUP message is sent up
	**  the data channel.  The poll(2) above returns when the notification
	**  message reaches the ctrl channel's Stream Head, and ttymon gets
	**  SIGCLD when the controlling process, whose controlling terminal is
	**  the data channel, dies (assuming the normal case where the process
	**  is not catching or ignoring SIGHUP).
	**
	**  The description enclosed by ### is left for historical purposes.
	**  It describes the scenario when Trusted Path Drivers were
	**  dismantled after a login session terminated and rebuilt before
	**  starting the next login session:
	**##################################################################### 
	**  There is a window during which the race can occur.  The window is
	**  between the time the poll(2) returns and the time a pmtab entry
	**  is found (via tp_find_fd()) for the file descriptor(s) causing
	**  the poll(2) to return.  The race is to find the pmtab entry before
	**  the signal handler for SIGCLD, sigchild() gets called.
	**
	**  If sigchild() gets called before the pmtab
	**  entry is found, tp_find_fd() does not find a pmtab entry and
	**  tp_got_msg() does not get called.  If the notification message on
	**  the ctrl channel indicated a SAK had been entered and tp_got_msg()
	**  did not get called, the pmtab entry would not have its state 
	**  changed to tpTRUSTEDSTATE and a data channel for the TP device
	**  would not be set up to start I&A.
	**  From the users point of view another SAK would have to be entered
	**  to get a login: prompt
	**
	**  Need to hold SIGCLD since this function
	**  accesses p_tpctrlfd field (via tp_find_fd()) to find the pmtab
	**  entry that is associated with fd that caused poll(2) to return, and
	**  sig_child() alters (set p_tpctrlfd value to 0 when it dismantles
	**  a TP multiplexor).
	**
	**  NOTE: sigchild() will poll(2) the p_tpctrlfd of the terminated
	**  process and call tp_got_msg() if a message exists on the ctrl
	**  channel.  This is needed to handle two additional conditions.
	**  1. There is another small window between the time the poll(2) in
	**  this function and the SIGCLD is blocked via sigprocmask()
	**  2. If the poll(2) in this function is interrupted via SIGCLD and
	**  a SAK was entered on the tty device associated with the process
	**  that died, the SAK notification will never be detected after
	**  sigchild() dismantles the multiplexor.
	**######################################################################
	**
	** -Still hold SIGCLD since functions called (eg. tp_got_msg()) alters
	**  pmtab fields that sigchild() relies on and alters.
	**
	**  NOTE: since the TP devices are no longer dismantled, we re-poll
	**  the TP ctrl channel file descriptor again before calling
	**  tp_got_msg() in case the poll(2) returned but we did not get to
	**  block SIGCLD before it was posted and we started to handle it
	**  (in sigchild()).  This is necessary so that we do not end up
	**  calling tp_got_msg() again when there is no message to get.
	**  
	*/
	(void)sigprocmask(SIG_SETMASK, (const sigset_t *)&nset,
	 (sigset_t *)NULL);

#ifdef	DEBUG
	debug("poll return");
#endif

	if (n < 0) {
		(void)sigprocmask(SIG_SETMASK, (const sigset_t *)&oset,
		 (sigset_t *)NULL);
		if (errno == EINTR)	/* interrupt by signal */
			return;
		logexit(1, badpoll, "do_poll", strerror(errno));
	}
	for (i = 0; (i < nfds)&&(n); i++,fdp++) {
		if (fdp->revents != 0) {
			n--;
			if ((pmptr = tp_find_fd(fdp->fd)) != PNULL){
				int	r;
				/* -NOTE: not checking for POLLHUP because
				**  M_HANGUP is not sent up ctrl channel.
				**  Instead, the notification that a M_HANGUP
				**  has been detected is packaged in a M_PROTO
				**  message
				*/
				fdp->revents = 0;
				while ((r = poll(fdp, (ulong)1, 0))
				 < 0){
					if ((errno == EAGAIN) ||
					 (errno == EINTR)){
						continue;
					}
					break;
				}
				if (r > 0){
					if (fdp->revents &(POLLPRI|POLLIN)){
						/* something on the ctrl channel */
						tp_got_msg(pmptr);
					}else if (!(fdp->revents & POLLNVAL)){
						log(MM_ERROR, 
						 ":862:%s:tp: invalid poll type %d ",
						 "do_poll",fdp->revents);
					}
				}
			} else if ((pmptr = find_fd(fdp->fd)) != PNULL) {
				if (fdp->revents & POLLHUP) {
					got_hup(pmptr);
				}
				else if (fdp->revents & POLLIN) {
#ifdef	DEBUG
					debug("got POLLIN");
#endif
					got_data(pmptr);
				}
			} else {
				/* -do the poll(2) again (this time not blocked)
				**  a SIGCLD may have posted and handled before
				**  we had the chance to block it 
				** -if a p_tpctrlfd is no longer opened
				**  (ie. it was closed in sigchild(), it will
				**  have POLLNVAL in its events.
				*/
				fdp->revents = 0;
				n = poll(fdp, (unsigned long)1, 0);

				if (!(fdp->revents & POLLNVAL)){
					log(MM_ERROR,
					 ":863:%s: Cannot find fd %d in pmtab","do_poll",fdp->fd);
				}
			}
		}
	}
	(void)sigprocmask(SIG_SETMASK, (const sigset_t *)&oset,
	 (sigset_t *)NULL);
}

/*
 * Procedure:	  sigchild
 *
 * Restrictions:
                 tp_datadisconnect:P_MACWRITE
                 strerror: none
                 sprintf: none
		 chown(2): none
		 chmod(2): none
*/

/*
** sigchild()	- handler for SIGCLD
**
** -find the pid of dead child
** IF pid is found in PMtab table
**	-clean utmp if U_FLAG is set
**	-disconnect data channel if connected
**	-hangup the line if H_FLAG is set
**	-zero out p_fd, p_pid, p_inservice, and p_tpdataconnid
**	IF SAK is defined as NONE and device is not bi-directional
**		-set p_tpstatus = tpTRUSTEDSTATE
**
**	IF status != NOTVALID
**		-Handle any changes to the pmtab if (p_status == CHANGED)
**		-zero out p_reason
**		-set status to VALID
**		IF ttymon state == PM_DISABLED
**			-close tp device
**	ELSE status == NOTVALID
**		-close tp device
**		-purge entry from PMtab table
** ELSE IF pid is found on pidtab table
**	-clean utmp if U_FLAG is set
**	-purge pid entry from pidtab table
*/
void
/*ARGSUSED*/
sigchild(n)
int	n;	/* this is declared to make cc happy, but it is not used */
{
	struct	pmtab	*pmptr;
	struct	pidtab	*pidptr;
	struct	tp_info	tpinf;
	struct	sigaction	sigact;
	int 	status;
	pid_t 	pid;
	struct	pollfd	tpctrl_pollfd;
	int	r;

#ifdef	DEBUG
	debug("in sigchild");
#endif
	while (1) {

	/* find the process that died */
	pid = waitpid(-1, &status, WNOHANG);
	if (pid == -1 && errno == EINTR)
		continue;
	if (pid <= 0)
		return;

		if ((pmptr = find_pid(pid)) != (struct pmtab *)NULL) {
			if (pmptr->p_flags & U_FLAG)
				cleanut(pid,status);
			if (pmptr->p_ttyflags & H_FLAG){
				(void)hang_up_line(pmptr->p_tpctrlfd);
				if (tpctrl_termio(pmptr->p_tpctrlfd, pmptr)
				 == -1){
					log(MM_ERROR, badtptermio, "sigchild:H_FLAG",pmptr->p_realdevice);
				/*	tpclose_ctrl(pmptr); */
				}
			}
			/* IF saktype == saktypeNONE && ttyflags != B_FLAG
			**	-mark tpstatus as if it were in a trusted state
			**	 tpTRUSTEDSTATE
			**	-setting this status here will by-pass ttymon
			**	 checks that require a SAK to be entered before
			**	 a TP can be instantiated
			*/
			if ((pmptr->p_sak.sak_type == saktypeNONE) &&
			 (!(pmptr->p_ttyflags & B_FLAG)))
				pmptr->p_tpstatus = tpTRUSTEDSTATE;

			pmptr->p_fd = 0;
			pmptr->p_pid = 0;
			pmptr->p_inservice = 0;
			pmptr->p_tpdataconnid = 0;
			Retry = TRUE;

			/* -poll p_tpctrlfd to see if a message was sent up
			**  the ctrl channel.  If so call tp_got_msg() to
			**  handle it.
			**  NOTE: poll(2) put here (after p_fd, p_pid, and
			**  p_inservice fields are set to 0) so that
			**  tp_got_msg() will not put this pmtab entry on the
			**  pidtab since sigchild() has already found the
			**  pmtab entry
			*/
			tpctrl_pollfd.fd = pmptr->p_tpctrlfd;
			tpctrl_pollfd.events = POLLIN | POLLPRI;
			while ((r = poll(&tpctrl_pollfd, (ulong)1, 0)) < 0){
				if ((errno == EAGAIN) || (errno == EINTR)){
					continue;
				}
				break;
			}
			if (r > 0){
				/* something on the ctrl channel */
				tp_got_msg(pmptr);
			}


			if (pmptr->p_status == CHANGED){
				do_pmtab_changed(pmptr);
				pmptr->p_reason = 0;
			}
			if (pmptr->p_status == NOTVALID){
				tpclose_device(pmptr);
				purge();
			}
			else{
				pmptr->p_status = VALID;
				tpreinit_realdev(pmptr);
			}

			/* Change mode of real/physical tty device to
			 * 0666 if this is a bi-directional port or 0620 if
			 * this is not a bi-directional port.
			 * The real/physical tty device is set since the
			 * Connection Server demon checks descretionary access
			 * on the real/physical device, not the TP device.  
			 * Changing the modes on the real/physical device is
			 * done here and not in open_device() (in ttymon.c)
			 * as it use to be done, because open_device()
			 * is typically not called (unless SAK is defined to
			 * be NONE) until after SAK has been entered.
			 * If the device is defined to be a bi-directional
			 * device, the time between login session termination
			 * and the time the user enters the SAK, the modes of
			 * the real/physical device should be 0666.
		 	 */
			if (pmptr->p_ttyflags & B_FLAG) {
				(void)chmod(pmptr->p_realdevice,0666);
			}else{
				(void)chmod(pmptr->p_realdevice,0620);
			}

			(void)chown(pmptr->p_realdevice,ROOTUID,Tty_gid);
		}
		else if ((pidptr = dequeue_pid(pid)) != (struct pidtab *)NULL){
			if (pidptr->p_flags & U_FLAG)
				cleanut(pid, status);
			free(pidptr);
		}
		else {
			/*
			** This shoud not happen because
			**	-NOTVALID pmtab entries are not purged if a
			**	 child  process exists
			*/
#ifdef	DEBUG
			(void)sprintf(Scratch,
			 "cannot find dead child (%ld) in pmtab", pid);
			debug(Scratch);
			dlog(Scratch);
#endif
			cleanut(pid,status);
			return;
		}
	}
}



/* dequeue_pid(pid)
**
** -return a struct pidtab * entry for pid from pidtab
** -NOTE: the pidtab entry is taken off the queue, the caller must free it
*/
static struct pidtab *
dequeue_pid(pid)
	pid_t pid;
{
	struct	pidtab *prevp, *pidptr;

	for (prevp = pidptr = pidtab; pidptr != (struct pidtab *)NULL;
	 prevp = pidptr, pidptr = pidptr->p_next){

		if (pidptr->p_pid == pid){
			if (prevp == pidptr)
				pidtab = pidptr->p_next;
			else
				prevp->p_next = pidptr->p_next;
			break;
		}
		else
			continue;
	}
	return (pidptr);
}


/*
 *	sigterm	- handler for SIGTERM
 *		- dismantle all TP muxes
 *			--if ttymon "owns" (i.e. has knowledge of "ownership"
 *			  of the TP device ((p_fd > 0) || (p_pid > 0));
 *			  (See tp_got_msg()) and the child is not in service. 
 *
 *			  NOTE: the ownership check is put here so the TP data
 *			  channel can be disconnected from the TP device before
 *			  the TP mux is dismantled.  This will reduce the
 *			  number of erroneous I_PUNLINK:Device Busy failure
 *			  messages from tpclose_device() that was previously
 *			  caused just by calling tpclose_device() based on the
 *			  else condition below.
 *
 *			  The NOTE: listed below may be true,
 *				-no child may exist or
 *				- port not in service
 *			  but a TP data channel may be connected to a TP device
 *			  that ttymon has knowledge of.  This can occur if
 *			  device being monitored is SAK for the TP device is
 *			  defined to be NONE.
 *				-and not defined as bi-directional (B_FLAG)
 *				-and the following flags and conditions hold
 *				 true:
 *					-waiting for data (R_FLAG) is set, or
 *					 doing autobaud (A_FLAG) set, or a
 *					 login timeout value other then zero
 *					 is defined.
 *			  
 *			--else (ttymon has no knowledge of "ownership") if
 *			  no child exists or child is not in service
 *
 *			  NOTE: This case may fail to dismantle TP device
 *			  (indicated by an I_PUNLINK failure message) if a
 *			  data channel is connected to the TP device.
 *			  ttymon would not have this knowledge (i.e. data
 *			  channel being connected) if this invocation of
 *			  ttymon occured while there was a login session
 *			  on the TP device or the TP device was "owned" by
 *			  another application (e.g. BNU on
 *			  bi-directional lines).
 *
 *			  NEED A BETTER SOLUTION TO HANDLE THIS CASE
 */
void
sigterm()
{
	struct pmtab *pmptr;
	struct tp_info tpinf;
	/*
	 * closing PCpipe will cause attached non-service children
	 * to get SIGPOLL and exit
	 */
	(void)close(PCpipe[0]);
	(void)close(PCpipe[1]);



	for (pmptr = PMtab; pmptr; pmptr = pmptr->p_next) {

		if ( (pmptr->p_fd > 0) || (pmptr->p_pid > 0) ){

			/* NOTE: (p_pid == 0) && (p_inservice != TRUE)
			** should be true
			*/
			if (pmptr->p_fd > 0){
				(void)close(pmptr->p_fd);
				tpclose_device(pmptr);

			/* NOTE: (p_fd == 0) should be true */
			}else if ((pmptr->p_pid > 0) &&
			 (pmptr->p_inservice != TRUE)){
				TP_LOADINF(tpinf, (dev_t)-1, (dev_t)-1, 0, 0,
				 (dev_t)-1, (dev_t)-1, (dev_t)-1, 0, 0,
				 pmptr->p_tpdataconnid, 0, nullsak, 0,
				 nulltermios, nulltermios);
				(void) procprivl(CLRPRV, MACWRITE_W, 0);
#ifndef DEBUG
				/*
				** Do not need to check return since if it
				** fails with other than Tp_errno == ENXIO,					** tpclose_device() will indicate a I_PUNLINK
				** failure.
				*/
				(void) tp_datadisconnect(pmptr->p_tpctrlfd,
				 &tpinf);
#else
				if (tp_datadisconnect(pmptr->p_tpctrlfd, &tpinf)
				 == -1){
					(void) procprivl(SETPRV, MACWRITE_W, 0);
					tp_geterror(&Tp_errno,
					 sizeof(Tp_errormsg), &Tp_errormsg);
					if (Tp_errno != ENXIO){
						log(MM_ERROR, ":859:%s: %s: %s",
						 "tp_got_msg",Tp_errormsg,
						 strerror(Tp_errno));
					}
				}
#endif
				(void) procprivl(SETPRV, MACWRITE_W, 0);
				tpclose_device(pmptr);
			}
		}else if ((pmptr->p_pid == 0) || (pmptr->p_inservice != TRUE)){
			tpclose_device(pmptr);
		}
	}
	logexit(1,":632:Caught SIGTERM");

}

/*
 *	state_change	- this is called when ttymon changes
 *			  its internal state between enabled and disabled
 */
void
state_change()
{
	struct pmtab *pmptr;

#ifdef	DEBUG
	debug("in state_change");
#endif

	/* 
	 * closing PCpipe will cause attached non-service children 
	 * to get SIGPOLL and exit
	 */
	(void)close(PCpipe[0]);
	(void)close(PCpipe[1]);

	/* reopen PCpipe */
	setup_PCpipe();

	/*
	 * also close all open ports so ttymon can start over
	 * with new internal state. dismantle TP muxes if no child.
	 * dismantling TP muxes for attached non-service children is handled
	 * in sigchild() 
	 */
	for (pmptr = PMtab; pmptr; pmptr = pmptr->p_next) {
		if ((pmptr->p_fd > 0) && (pmptr->p_pid == 0)) {
			(void)close(pmptr->p_fd);
			pmptr->p_fd = 0;
		}
		if (pmptr->p_pid == 0)
			tpclose_device(pmptr);
	}
	Retry = TRUE;

}

/*
**	re_read	- reread pmtab
**		- kill tmchild if entry changed
**		- purge pmtab entries that are not valid (NOTE: entries are
**		  not purged from PMtab if a child exists
**/
void
re_read()
{
	sigset_t	cset;
	sigset_t	tset;

	(void)sigprocmask(SIG_SETMASK, NULL, &cset);
	tset = cset;
	(void)sigaddset(&tset, SIGCLD);
	(void)sigprocmask(SIG_SETMASK, &tset, NULL);
	if (Nlocked > 0) {
		alarm(0);
		Nlocked = 0;
	}
	read_pmtab();
	kill_children();
	purge();
	(void)sigprocmask(SIG_SETMASK, &cset, NULL);

	alloc_pollarray();
	Retry = TRUE;
}

/*
 *	find_pid(pid)	- find the corresponding pmtab entry for the pid
 */
static	struct pmtab *
find_pid(pid)
pid_t	pid;
{
	struct pmtab *pmptr;

	for (pmptr = PMtab; pmptr; pmptr = pmptr->p_next) {
		if (pmptr->p_pid == pid) {
			return(pmptr);
		}
	}
	return((struct pmtab *)NULL);
}

/*
 *	find_fd(fd)	- find the corresponding pmtab entry for the fd
 */
static struct pmtab *
find_fd(fd)
int	fd;
{
	struct pmtab *pmptr;

	for (pmptr = PMtab; pmptr; pmptr = pmptr->p_next) {
		if (pmptr->p_fd == fd) {
			return(pmptr);
		}
	}
	return((struct pmtab *)NULL);
}


/* tp_find_fd(fd)
**
** -search PMtab table entries p_tpctrlfd field for a matching value in fd
*/
static struct pmtab *
tp_find_fd(fd)
	int fd;
{
	struct	pmtab	*pmptr;

	for (pmptr = PMtab; pmptr; pmptr = pmptr->p_next) {
		if (pmptr->p_tpctrlfd == fd) {
			return(pmptr);
		}
	}
	return(PNULL);
}

                                                                                /*
 * Procedure:	  kill_children
 *
 * Restrictions:
                 kill(2): none
*/

/* kill_children()
**
** If p_status != VALID
**	-If no child (p_pid == 0) and data channel (p_fd) is open, close it
**	-If we have a child and it is not inservice yet (p_inservice == FALSE),
**	 kill the child
**	-Handle any changes to the pmtab if (p_status == CHANGED)
**	-close ctrl channels if status is NOTVALID 
*/
static void
kill_children()
{
	struct pmtab *pmptr;

	for (pmptr = PMtab; pmptr; pmptr = pmptr->p_next) {
		if (pmptr->p_status == VALID)
			continue;
		if (pmptr->p_pid == 0){
			if (pmptr->p_fd > 0){
				(void)close(pmptr->p_fd);
				pmptr->p_fd = 0;
				pmptr->p_tpdataconnid = 0;
			}
			if (pmptr->p_status == NOTVALID){
				tpclose_device(pmptr);
			}
		}
		else{ /* p_pid > 0) */
			if ((pmptr->p_fd == 0)&&(pmptr->p_inservice == FALSE)) {
				(void)kill(pmptr->p_pid, SIGTERM);
			}
		}
		if (pmptr->p_status == CHANGED){
			do_pmtab_changed(pmptr);
			pmptr->p_reason = 0;
			pmptr->p_status = VALID;
		}
	}
}


/*
 * Procedure:	  do_pmtab_changed
 *
 * Restrictions:
		 tp_defsak: P_MACWRITE
		 strerror: None
*/

/* do_pmtab_changed()
**
** -Handle various reasons for a changed pmtab entry
**
** -NOTE: ctrl channel may be closed in this function
*/
static void
do_pmtab_changed(pmptr)
	struct	pmtab *pmptr;
{
	struct	tp_info tpinf;

	if (pmptr->p_reason & REASrealdevice){
		tpclose_ctrl(pmptr);
		pmptr->p_reason &= ~REASrealdevice;
	} else if ((pmptr->p_reason & REASsakchg) && (pmptr->p_tpctrlfd > 0)){
		TP_LOADINF(tpinf, (dev_t)-1, (dev_t)-1, 0, 0, (dev_t)-1,
		 (dev_t)-1, (dev_t)-1, 0, 0, 0, 0, pmptr->p_sak, 0, nulltermios,
		 nulltermios);
		(void) procprivl(CLRPRV, MACWRITE_W, 0);
		if (tp_defsak(pmptr->p_tpctrlfd, &tpinf) == -1){
			(void) procprivl(SETPRV, MACWRITE_W, 0);
			tp_geterror(&Tp_errno,sizeof(Tp_errormsg),Tp_errormsg);
			log(MM_ERROR, ":864:%s: %s, %s\n",
				"do_pmtab_changed",Tp_errormsg, strerror(Tp_errno));
			tpclose_ctrl(pmptr);
		}else if (tpctrl_termio(pmptr->p_tpctrlfd,
		 pmptr) == -1){
			log(MM_ERROR, badtptermio,
			 "do_pmtab_changed:after REASsakchg",
			 pmptr->p_realdevice);
			tpclose_ctrl(pmptr);
		}else
			pmptr->p_reason &= ~REASsakchg;
		(void) procprivl(SETPRV, MACWRITE_W, 0);
	}
}



static	void
mark_service(pid)
pid_t	pid;
{
	struct	pmtab	*pmptr;
#ifdef	DEBUG
	debug("in mark_service");
#endif
	if ((pmptr = find_pid(pid)) == NULL) {
		log(MM_ERROR, ":865:%s: Cannot find child (%ld) in pmtab","mark_service",pid);
		return;
	}
	pmptr->p_inservice = TRUE;
	return;
}

/*
 * Procedure:	  read_pid
 *
 * Restrictions:
		 read(2): none
                 strerror: none
*/

/*
 * read_pid(fd)	- read pid info from PCpipe
 */
static	void
read_pid(fd)
int	fd;
{
	int	ret;
	pid_t	pid;

	for (;;) {
		if ((ret = read(fd,&pid,sizeof(pid))) < 0) {
			if (errno == EINTR)
				continue;
			if (errno == EAGAIN) 
				return;
			logexit(1, ":635:Read error on PCpipe: %s", strerror(errno));
		}
		if (ret == 0)
			return;
		if (ret != sizeof(pid)) {
			logexit(1, ":636:read() returns incorrect size, ret = %d", ret);
		}
		mark_service(pid);
	}
}

/*
 * Procedure:	  sigpoll_catch
 *
 * Restrictions:
                 strerror: none
*/

/*
 * sipoll_catch()	- signal handle of SIGPOLL for ttymon
 *			- it will check both PCpipe and pmpipe
 */
void
sigpoll_catch()
{
	int	ret;
	struct	pollfd	pfd[2];

#ifdef	DEBUG
	debug("in sigpoll_catch");
#endif

	pfd[0].fd = PCpipe[0];
	pfd[1].fd = Pfd;
	pfd[0].events = POLLIN;
	pfd[1].events = POLLIN;
	if ((ret = poll(pfd, 2, 0)) < 0) {
		logexit(1, badpoll, "sigpoll_catch", strerror(errno));
	}
	if (ret > 0) {
		if (pfd[0].revents & POLLIN) 
			read_pid(pfd[0].fd);
		if (pfd[1].revents & POLLIN)
			sacpoll();
	}
}

/*ARGSUSED*/
/*
 * Procedure:	  sigalarm
 *
 * Restrictions:
		 sprintf: none
*/

void
sigalarm(signo)
int	signo;
{
	struct pmtab *pmptr;
	struct sigaction sigact;
	int	fd;
	extern	int	check_session();

#ifdef	DEBUG
	(void)sprintf(Scratch,
	"in sigalarm, Nlocked = %d", Nlocked);
	debug(Scratch);
#endif
	for (pmptr = PMtab; pmptr; pmptr = pmptr->p_next) {
		if ((pmptr->p_status == SESSION) && (pmptr->p_fd == 0)) {
			if ((fd = tpopen_data(pmptr, O_RDWR|O_NONBLOCK)) == -1){
				log(MM_ERROR, badopendata, "sigalarm",pmptr->p_realdevice);
				pmptr->p_status = VALID;
				Retry = TRUE;
				return;
			}
			else { 
				if (check_session(fd) == 0) {
					Nlocked--;
					pmptr->p_fd = fd;
					Retry = TRUE;
				}
				else
					(void)close(fd);
			}
		/*
		 * This is a Temporary fix designed to "reinitialize"
		 * bi-directional real/physical tty devices.  When the
		 * bi-directional real/physical tty device is used as an
		 * outgoing device, it was not "reinitialized" after the device
		 * was finished being used.  Since the real/physical tty devices
		 * are linked * under a Trusted Path device, "reinitializing"
		 * the real/physical device is done by opening, open(2) and
		 * immediately closing, close(2), the real/physical tty device.
		 * Openning the device will raise/assert DTR on the device
		 * which is necessary for the device to get carrier, DCD, if
		 * the real/physical tty device is connected to a modem, or
		 * for a real/physical tty device, connected at the other end,
		 * to get carrier if the devices are directly connected via a
		 * null modem.  The close has no effect on the real/physical
		 * tty device since is linked underneath a TP device.
		 *
		 * Carrier is necessary inorder to has data written to the
		 * real/physical tty device to be sent out, and carrier is
		 * also necessary on an in comming inorder to establish
		 * a connection, if in comming device is connected to a modem.
		 */
		}else if (pmptr->p_ttyflags & B_FLAG){
			if (pmptr->p_tpctrlfd != 0){
				tpreinit_realdev(pmptr);
			}
			Nlocked--;
		}
	}
	if (Nlocked > 0) {
		sigact.sa_flags = SA_RESETHAND | SA_NODEFER;
		sigact.sa_handler = sigalarm;
		(void)sigemptyset(&sigact.sa_mask);
		(void)sigaction(SIGALRM, &sigact, NULL);
		(void)alarm(ALARMTIME);
	}
	else {
		sigact.sa_flags = 0;
		sigact.sa_handler = SIG_IGN;
		(void)sigemptyset(&sigact.sa_mask);
		(void)sigaction(SIGALRM, &sigact, NULL);
	}
}
