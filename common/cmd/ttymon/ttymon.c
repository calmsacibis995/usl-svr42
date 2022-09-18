/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ttymon:common/cmd/ttymon/ttymon.c	1.20.16.4"
#ident  "$Header: $"
/***************************************************************************
 * Command: ttymon
 * Inheritable Privileges: P_DEV,P_SETUID,P_SETPLEVEL,P_SETFLEVEL,P_SYSOPS,
 *												 P_MACWRITE,P_DACREAD,P_DACWRITE,P_OWNER,P_COMPAT,
 *												 P_FILESYS,P_AUDIT,P_MACREAD,P_DRIVER
 *       Fixed Privileges: None
 * Notes:
 *
 ***************************************************************************/
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/stropts.h>
#include <sys/resource.h>
#include <limits.h>
#include <pwd.h>
#include <grp.h>
#include <mac.h>
#include <priv.h>
#include <locale.h>
#include <pfmt.h>

# include "sac.h"
# include "ttymon.h"
# include <sys/termios.h>
# include <sys/stream.h>
# include <sys/tp.h>
# include "tmstruct.h"
# include "tmextern.h"


extern	int	Retry;
extern	int	MACRunning;			/* see tmglobal.c */
extern	int	B2Running;			/* see tmglobal.c */
extern	int	EnhancedSecurityInstalled;	/* see tmglobal.c */
extern	int	express_mode;

extern	int	is_macrunning();		/* see tmutil.c */	
extern	int	is_b2running();			/* see tmutil.c */	
extern	int	is_enhancedsecurityinstalled();	/* see tmutil.c */	
extern	char	*strsave();
extern	void	do_poll();		/* see tmhandler.c */
extern	void	re_read();		/* see tmhandler.c */
extern	int	check_session();
extern	void	sigalarm();
extern	int	tpctrl_termio();	/* see tmterm.c */



extern	int	tp_devopen();		/* see tpi_ops.c */
extern	int	tp_chanopen();		/* see tpi_ops.c */
extern	int	tp_dataconnect();	/* see tpi_ops.c */
extern	int	tp_makedevice();	/* see tpi_ops.c */
extern	void	tp_geterror();		/* see tpi_ops.c */


extern	int	devalloc();
extern	int	devstat();




static	int	Tp_errno;
static	char	Tp_errormsg[256];
static	int	Initialized;
static	struct	termios nulltermios;
static	struct	sak nullsak;
static	struct	pollfd *pollp;		/* ptr to an array of poll struct        */


static	struct Gdef default_setting = {	/* default terminal settings	*/
		"default",
		"9600",
		"9600 sane",
		0,
		/* 
		 * next label is set to 4800 so we can start searching ttydefs.
		 * if 4800 is not in ttydefs, we will loop back to use default_setting 
		 */
		"4800"
};


static	void	initialize();
static	void	enable_access();
static	void	tpopen_all();
	void	tpopen_device();
	int	tpopen_data();
	void	tpclose_ctrl();
	void	tpclose_device();
	void	tpreinit_realdev();
static	void	open_all();
	void	open_device();
	int	set_poll();
static	int	check_spawnlimit();
static	int	mod_ttydefs();
static	void	free_defs();
	struct	Gdef	*get_speed();
	void	setup_PCpipe();
	void	alloc_pollarray();



const char
	badopen[] = ":396:Cannot open %s: %s",
	badioctl[] = ":712:%s: ioctl() %s failed: %s",
	badioctlfd[] = ":713:%s: ioctl() %s failed, fd = %d: %s",
	badfcntl[] = ":714:fcntl() %s failed: %s",
	efailed[] = ":715:%s failed: %s",
	nomem[] = ":643:Out of memory: %s",
	badopendata[] = ":876:%s: opening data channel failed for (%s)";

/*
 * Procedure:	  main
 *
 * Restrictions:
                 setlocale: none
*/

/*
 * 	ttymon	- a port monitor under SAC
 *		- monitor ports, set terminal modes, baud rate
 *		  and line discipline for the port
 *		- invoke service on port if connection request received
 *		- Usage: ttymon
 *			 ttymon -g [options]
 *			 Valid options are
 *			 -h
 *			 -d device
 *			 -l ttylabel
 *			 -t timeout
 *			 -m modules
 *			 -p prompt
 *		- if enhanced security is installed usage is extended with the
 *		  following
 *			 -k SAKtype -K SAK [-x]
 *			 -k SPECSAKtype
 *
 *		- ttymon without args is invoked by SAC
 *		- ttymon -g is invoked by process that needs to
 *		  have login service on the fly
 */

main(argc, argv)
int	argc;
char	*argv[];
{
	int	nfds;
	extern	char	*lastname();

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:ttymon");

	/* -determine if we are running with MAC
	** -determine if we are running with B2 enhanced security
	** -determine if enhanced security is installed on system
	*/
	MACRunning = is_macrunning();
	B2Running = is_b2running();
	EnhancedSecurityInstalled = is_enhancedsecurityinstalled();


	if ((argc > 1) || (strcmp(lastname(argv[0]), "getty") == 0)) {
		if (argc == 1)
			setlabel("UX:getty");
		ttymon_express(argc,argv);
		exit(1);	/*NOTREACHED*/
	}
	/* remember original signal mask and dispositions */
	(void)sigprocmask(SIG_SETMASK, NULL, &Origmask);
	(void)sigaction(SIGINT, (struct sigaction *)NULL, &Sigint);
	(void)sigaction(SIGALRM, (struct sigaction *)NULL, &Sigalrm);
	(void)sigaction(SIGPOLL, (struct sigaction *)NULL, &Sigpoll);
	(void)sigaction(SIGCLD, (struct sigaction *)NULL, &Sigcld);
	(void)sigaction(SIGTERM, (struct sigaction *)NULL, &Sigterm);
#ifdef	DEBUG
	(void)sigaction(SIGUSR1, (struct sigaction *)NULL, &Sigusr1);
	(void)sigaction(SIGUSR2, (struct sigaction *)NULL, &Sigusr2);
#endif
	initialize();

	for (;;) {
		nfds = set_poll(pollp);
		if (!Reread_flag) {
			if (nfds > 0)
				do_poll(pollp,nfds);
			else
				(void)pause();
		}
		/*
		 * READDB messages may arrive during poll or pause.
		 * So the flag needs to be checked again.
		 */
		if (Reread_flag) {
			Reread_flag = FALSE;
			re_read();
		}
		while (Retry) {
			Retry = FALSE;
			tpopen_all();
			open_all();

		}
	}
}

/*
 * Procedure:	  initialize
 *
 * Restrictions:
                 setrlimit(2): none
		 gettxt: none
		 ioctl(2): none
		 strerror: none
		 ulimit(2): none
		 getpwnam: none
		 getgrnam:P_MACREAD
		 sprintf: none
*/

static	void
initialize()
{
	struct	pmtab	*tp;
	register struct passwd *pwdp;
	register struct	group	*gp;
	struct	rlimit rlimit;
	extern	struct	rlimit	Rlimit;
	int     maxfiles;       /* max. number of open files per process */
	extern	 gid_t	Tty_gid;
	extern 	 int	setrlimit(), getrlimit();

#ifdef 	DEBUG
	extern	opendebug();
#endif
	Initialized = FALSE;
	/*
	 * get_environ() must be called first, 
	 * otherwise we don't know where the log file is
	 */
	get_environ();
	openlog();
	openpid();
	openpipes();
	setup_PCpipe();

	log(MM_INFO, ":716:PMTAG:\t\t %s",Tag);
	log(MM_INFO, ":717:Starting state: %s", 
		(State == PM_ENABLED) ? gettxt(":718", "enabled") :
					gettxt(":719", "disabled"));

#ifdef 	DEBUG
	opendebug(FALSE);
	debug("***** ttymon in initialize *****");
	dlog("debug mode is \t on");
#endif

	catch_signals();

	/* register to receive SIGPOLL when data comes to pmpipe */
	if (ioctl(Pfd, I_SETSIG, S_INPUT) < 0) {
		logexit(1, ":720:ioctl() I_SETSIG on pmpipe failed: %s", strerror(errno));
	}
	sacpoll(); /* this is needed because there may be data already */
	
	maxfiles = (int)ulimit(4, 0L);	/* get max number of open files */
	if (maxfiles < 0) {
		logexit(1, efailed, "ulimit(4,0L)", strerror(errno));
	}
	if (getrlimit(RLIMIT_NOFILE, &Rlimit) == -1) {
		logexit(1, efailed, "getrlimit()", strerror(errno));
	}
	rlimit.rlim_cur = rlimit.rlim_max = Rlimit.rlim_max;
	if (setrlimit(RLIMIT_NOFILE, &rlimit) == -1) {
		logexit(1, efailed, "setrlimit()", strerror(errno));
	}
	maxfiles = rlimit.rlim_cur;
	Maxfds = maxfiles - FILE_RESERVED;
	log(MM_INFO, ":721:Max open files\t = %d", maxfiles);
	log(MM_INFO, ":722:Max ports ttymon can monitor = %d", Maxfds);

	read_pmtab();

	/*
	** setup poll array 
	*/
	alloc_pollarray();


	(void) mod_ttydefs();	/* just to initialize mtime */
	if (check_version(TTYDEFS_VERS, TTYDEFS) != 0) {
		logexit(1, ":723:Check /etc/ttydefs version failed");
	}
	read_ttydefs(NULL,FALSE);

	/* initialize global variable, Tty_gid */
	(void) procprivl(CLRPRV, MACREAD_W, 0);
	if ((gp = getgrnam(TTY)) == NULL) {
		(void) procprivl(SETPRV, MACREAD_W, 0);
		log(MM_ERROR, ":724:No group entry for <tty>, default is used");
		}
	else {
		(void) procprivl(SETPRV, MACREAD_W, 0);
		Tty_gid = gp->gr_gid;
		}
	endgrent();
	endpwent();
#ifdef	DEBUG
	(void)sprintf(Scratch,"Tty_gid = %ld",Tty_gid);
	debug(Scratch);
#endif

	log(MM_INFO, ":725:Initialization Completed");

	/* open the devices ttymon monitors */
	Retry = TRUE;
	while (Retry) {
		Retry = FALSE;
		tpopen_all();	/* TP !!! */
		for (tp = PMtab; tp; tp = tp->p_next) {
			if ((tp->p_status > 0) && (tp->p_fd == 0) &&
			  (tp->p_pid == 0) && (tp->p_tpctrlfd != 0) &&
			  (!((State == PM_DISABLED) && 
			  ((tp->p_dmsg == NULL)||(*(tp->p_dmsg) == '\0')) ))
			  && (tp->p_tpstatus == tpTRUSTEDSTATE) ) {
				open_device(tp);
				if (tp->p_fd > 0) 
					got_carrier(tp);
			}
		}
	}
	Initialized = TRUE;
}


static void
tpopen_all()
{

	struct pmtab *pmp;
	struct	sigaction	sigact; /* for bi-directional Temporary fix */

	for (pmp = PMtab;pmp;pmp = pmp->p_next){

		/* IF saktype == saktypeNONE && ttyflags != B_FLAG
		**	-mark tpstatus as if it were in a trusted state
		**	 tpTRUSTEDSTATE
		**	-setting this status here will by-pass ttymon
		**	 checks that require a SAK to be entered before
		**	 a TP can be instantiated
		** NOTE:If saktype is NONE and it is bi-directional device,
		** the TP mechanism is designed to send "SAK" notification
		** when the first M_DATA message arrives to the TP driver
		** when no data channel is connected.
		*/
		if ((pmp->p_sak.sak_type == saktypeNONE) &&
		 (!(pmp->p_ttyflags & B_FLAG)))
			pmp->p_tpstatus = tpTRUSTEDSTATE;
		


		/* -open a TP device if the following holds
		**	-p_muxid == 0 indicates that no TP mechanism has
		**	 been setup for the tty device (as far as this
		**	 instaniation of ttymon is concerned)

		*/
		if ((pmp->p_status > 0) && (pmp->p_muxid == 0) &&
		 (!((State == PM_DISABLED) &&
		 ((pmp->p_dmsg == NULL) || (*(pmp->p_dmsg) == '\0')))))
			tpopen_device(pmp);


		/*
		** -This is a Temporary fix for bi-directional devices.
		**  (See tmhandler.c:sigalarm() for details.
		*/
		if (pmp->p_ttyflags & B_FLAG){
			Nlocked++;
			if (Nlocked == 1) {
				sigact.sa_flags = SA_RESETHAND | SA_NODEFER;
				sigact.sa_handler = sigalarm;
				(void)sigemptyset(&sigact.sa_mask);
				(void)sigaction(SIGALRM, &sigact, NULL);
				(void)alarm(ALARMTIME);
			}
		}
	}
}


/*
 * Procedure:	  tpopen_device
 *
 * Restrictions:
                 lvlin: none
		 strerror: none
		 lvlfile(2): none
		 devstat(2): none
		 tp_devopen(2): none
		 fcntl(2): none
		 chown(2): none
		 chmod(2): none
*/

/* tpopen_device(pmp)
**
** -allocates TP device via tp_devopen()
*/

void
tpopen_device(pmp)
	struct pmtab *pmp;
{
	struct dev_alloca devddb;	/* dev alloc. attributes */
	struct devstat	ttydevstat;	/* devstat() kernel attrs */
	struct tp_info	tpinf;
	int		ctrlfd;
	int		oflag;		/* open flags */


	if (pmp->p_status != GETTY){
		if (check_spawnlimit(pmp) == -1){
			pmp->p_status = NOTVALID;
			pmp->p_tpstatus = tpNONTRUSTEDSTATE;
			pmp->p_reason = REASspawnlimit;
			log(MM_ERROR,
			 ":726:Service <%s> is respawning too rapidly",
			 pmp->p_tag);
			return;
		}
	}



#ifdef	DEBUG
	debug("in open_device");
#endif
	if (MACRunning == TRUE){
		/*
		** -Get security attrs from Device Database and set security
		**  attrs on the tty device
		*/
		if (devalloc(pmp->p_realdevice ,DEV_GET, &devddb) < 0) {
			log(MM_ERROR,
			 ":877:%s: could not GET security attrs of %s from DDB\n",
			 "tpopen_device",pmp->p_realdevice);
			/* set default sec. attrs */
			ttydevstat.dev_mode = DEV_STATIC;
			lvlin("SYS_RANGE_MAX", &ttydevstat.dev_hilevel);
			lvlin("SYS_RANGE_MIN", &ttydevstat.dev_lolevel);
		}else{
			/* build sec. attrs from values in DDB for device */
			ttydevstat.dev_mode = devddb.mode;
			ttydevstat.dev_hilevel = devddb.hilevel;
			ttydevstat.dev_lolevel = devddb.lolevel;
		}
		/*
		** first free device control information on real device
		** so hi and lo range can be set on the next devstat(2)
		** call
		*/
		ttydevstat.dev_relflag = DEV_SYSTEM;
		if(devstat(pmp->p_realdevice,DEV_SET,&ttydevstat) < 0) {
			log(MM_ERROR,
			 ":851:%s: %s could not SET to DEV_SYSTEM on %s: %s",
			 "tpopen_device","devstat()",pmp->p_realdevice, strerror(errno));
			Retry = TRUE;
			return;
		}

		/* change the level of the device to the hilevel range */
		if(lvlfile(pmp->p_realdevice,MAC_SET,&ttydevstat.dev_hilevel) < 0) {
			log(MM_ERROR,
			":878:%s: %s could not SET level on %s: %s",
			"tpopen_device","lvlfile()",pmp->p_realdevice, strerror(errno));
			Retry = TRUE;
			return;
		}

		/* default state and release flag */
		ttydevstat.dev_relflag = DEV_LASTCLOSE;
		ttydevstat.dev_state = DEV_PRIVATE;

		if(devstat(pmp->p_realdevice,DEV_SET,&ttydevstat) < 0) {
			log(MM_ERROR,
			 ":853:%s: devstat() could not SET security attrs on %s: %s",
			 "tpopen_device",pmp->p_realdevice, strerror(errno));
			Retry = TRUE;
			return;
		}
	}

	if (express_mode == TRUE && B2Running == FALSE) {
		enable_access(pmp);
		return;
	}

	/* -setup TP device via tp_devopen()
	**	-tp_devopen() returns a file descriptor to the ctrl channel of
	**	 the TP device
	**	-args to tp_devopen()
	**		-path name of device to be muxed under (i.e. the real
	**		 device
	**		-return parameter pointer for TP device information.
	**		 the return paramter is also an input parameter which
	**		 gets loaded with the following relevant information:
	**			-default SAK definition
	**	-NOTE: tp_devopen() also does recovery of TP devices associated
	**	 with the real device 
	*/
	TP_LOADINF(tpinf, (dev_t)-1, (dev_t)-1, 0, 0, (dev_t)-1, (dev_t)-1,
	 (dev_t)-1, 0, 0, 0, 0, pmp->p_sak, 0, nulltermios, nulltermios);
	if ((ctrlfd = tp_devopen(pmp->p_realdevice, &tpinf)) == -1){
		tp_geterror(&Tp_errno, sizeof(Tp_errormsg), Tp_errormsg);
		log(MM_ERROR, ":879:%s: %s\nTP setup failed: %s",
		 "tpopen_device",Tp_errormsg, strerror(Tp_errno));
		Retry = TRUE;
		return;
	}
	pmp->p_muxid = tpinf.tpinf_muxid;
	/*
	** -set close-on-exec and O_NONBLOCK flag
	*/
	if (fcntl(ctrlfd, F_SETFD, 1) == -1) {
		logexit(1, ":880:%s: fcntl() F_SETFD failed: %s",
			"tpopen_device",strerror(errno));
	}
	oflag = fcntl(ctrlfd, F_GETFL);
	(void)fcntl(ctrlfd, F_SETFL, oflag);
	pmp->p_tpctrlfd = ctrlfd;


	/* -set termio on real device via the ctrl channel for SAK detection
	** -IF setting termio fails
	**	-close ctrl channel: this will disconnect ctrl channel from
	**	 TP device but will not dismantle it
	**	-setting p_muxid to 0 will allow the attempt to connect a ctrl
	**	 channel to the TP device to occur on the next pass of the
	**	 main loop, since a check is made for p_muxid == 0
	*/
	if (tpctrl_termio(pmp->p_tpctrlfd, pmp) == -1){
		log(MM_ERROR,
		 ":857:%s: tpctrl_termio() failed on ctrl channel for (%s)",
		 "tpopen_device",pmp->p_realdevice);
		tpclose_ctrl(pmp);
		Retry = TRUE;
		return;
	}

	enable_access(pmp);
}

/*
 * enable_access
 */

static void
enable_access(pmp)
struct	pmtab	*pmp;
{

	/* Set modes to 0666 if this is a bi-directional port and 0620 if
	 * it is not a bi-directional port.  The real/physical tty device
	 * is set since the Connection Server demon checks descretionary
	 * access on on the real/physical device, not the TP device. 
	 *
	 * Change ownership of real/physical tty device to root.
	 */
	if (pmp->p_ttyflags & B_FLAG) {
		(void)chmod(pmp->p_realdevice,0666);
	}else{
		(void)chmod(pmp->p_realdevice,0620);
	}

	(void)chown(pmp->p_realdevice,ROOTUID,Tty_gid);
}


/*
 * Procedure:	  tpopen_data
 *
 * Restrictions:
                 tp_chanopen: none
                 strerror: none
                 tp_dataconnect: none
                 tp_makedevice: none
                 open(2): none
		 lvlin: none
		 flvlfile(2): none
		 fdevstat(2): none
*/

/* tpopen_data(pmptr, openflags)
**
** -open a data channel 
** -connect  data channel to the TP device for realdevice
** -create file system name for data channel
** -copy file system name for data channel to p_device (need name for utmp)
**
** -returns open file descriptor to data channel if successful
** -returns -1 on failure
*/
int
tpopen_data(pmptr, openflags)
	struct	pmtab	*pmptr;
	int		openflags;
{
	int	fd;
	int	tmpfd;
	char	buf[PATH_MAX];
	struct	tp_info tpinf;
	struct dev_alloca devddb;	/* dev alloc. attributes */
	struct devstat	ttydevstat;	/* devstat() kernel attrs */

	if (express_mode == TRUE && B2Running == FALSE)
		return(open(pmptr->p_realdevice, openflags));

	if ((fd = tp_chanopen(TPC_DATA,openflags)) == -1){
		tp_geterror(&Tp_errno,sizeof(Tp_errormsg),Tp_errormsg);
		log(MM_ERROR,
		 ":881:%s\n\topen (%s) failed on data channel: %s",
		 Tp_errormsg,pmptr->p_device, strerror(Tp_errno));
		Retry = TRUE;
		return (-1);
	}

	TP_LOADINF(tpinf, (dev_t)-1, (dev_t)-1, 0, 0, (dev_t)-1, (dev_t)-1,
	 (dev_t)-1, 0, 0, 0, 0, nullsak,
	 TPINF_DISCONNDATA_ONHANGUP|TPINF_FAILUNLINK_IFDATA, nulltermios,
	 nulltermios);
	if (tp_dataconnect(pmptr->p_realdevice, pmptr->p_tpctrlfd, fd, &tpinf)
	 == -1){
		tp_geterror(&Tp_errno,sizeof(Tp_errormsg),Tp_errormsg);
		if (Tp_errno != EBUSY)
		log(MM_ERROR,
		 ":882:%s\n\t connect failed on data channel for (%s): %s",
		 Tp_errormsg,pmptr->p_device, strerror(Tp_errno));
		(void)close(fd);
		/*
		** EBUSY indicates a data channel is already connected.  Do not
		** set Retry = TRUE since this will can cause the device to
		** go out of service when retry limit is exceeded.  An attempt
		** to connect another data channel will be made when its
		** associated ctrl channel get SAK or Hangup notification.
		*/ 
		if (Tp_errno != EBUSY)
			Retry = TRUE;
		return (-1);
	}
	pmptr->p_tpdataconnid = tpinf.tpinf_connid;

	if (tp_makedevice(fd, sizeof(buf),buf) == -1){
		tp_geterror(&Tp_errno,sizeof(Tp_errormsg),Tp_errormsg);
		log(MM_ERROR, ":883:%s\n\t for (%s): %s",
		 Tp_errormsg,pmptr->p_device, strerror(Tp_errno));
		(void)close(fd);
		Retry = TRUE;
		return (-1);
	}
	/*
	** -Re-open the TP data channel via its Device Special File (DSF)
	**  name:
	**	-This is done so that the device control information (dci)
	**	 (assuming MAC is running) is set on the specfs file system's
	**	 snode associated DSF instead of the internal snode created
	**	 as a result of opening the data channel via its data channel
	**	 clone device.  This is important since most MAC checks will
	**	 be done on the DSF.
	**	-This is done so that system calls that change file attributes
	**	 (eg. chown(2), chmod(2)) will be done on the TP data channel's
	**	 DSF and not the internally generated snode from the clone
	**	 device which.
	**
	** -If this is a GETTY dup(2) the file descriptor to the
	**  re-opened data channel DSF since GETTY mode of ttymon
	**  expects file descriptor 0 to be opened to the device
	*/
	if ((tmpfd = open(buf, openflags)) == -1){
		log(MM_ERROR,
		 ":884:%s: Re-open of %s via its Device Special File failed: %s",
		 "tpopen_data",buf, strerror(errno));
		Retry = TRUE;
		return (-1);
	}
	(void)close(fd);
	if (pmptr->p_status == GETTY){
		if ((fd = dup(tmpfd)) == -1){
			log(MM_ERROR,
			 ":885:%s: dup of file descriptor opened on %s via its Device Special File failed: %s",
			 "tpopen_data",buf, strerror(errno));
			(void)close(fd);
			(void)close(tmpfd);
			Retry = TRUE;
			return (-1);
		}
		(void)close(tmpfd);
	}else{
		fd = tmpfd;
	}

	if (MACRunning == TRUE){
		/*
		** -Get security attrs from Device Database and set security
		**  attrs on the tty device
		*/
		if (devalloc(pmptr->p_realdevice, DEV_GET, &devddb) < 0) {
			log(MM_ERROR,
			 ":886:%s: could not GET security attrs of %s from DDB for TP device %s",
			 "tpopen_data",pmptr->p_realdevice, buf);
			/* set default sec. attrs */
			ttydevstat.dev_mode = DEV_STATIC;
			lvlin("SYS_RANGE_MAX", &ttydevstat.dev_hilevel);
			lvlin("SYS_RANGE_MIN", &ttydevstat.dev_lolevel);
		}else{
			/* build sec. attrs from values in DDB for device */
			ttydevstat.dev_mode = devddb.mode;
			ttydevstat.dev_hilevel = devddb.hilevel;
			ttydevstat.dev_lolevel = devddb.lolevel;
		}
		/* free device control information on real device */
		ttydevstat.dev_relflag = DEV_SYSTEM;
		if(fdevstat(fd,DEV_SET,&ttydevstat) < 0) {
			 log(MM_ERROR,
			 ":851:%s: %s could not SET to DEV_SYSTEM on %s: %s",
			 "tpopen_data","fdevstat()",buf, strerror(errno));
			Retry = TRUE;
			return (-1);
		}

		/* change the level of the device to the hilevel range */
		if(flvlfile(fd,MAC_SET,&ttydevstat.dev_hilevel) < 0) {
			 log(MM_ERROR,
			 ":852:%s: %s could not change level %s: %s",
			 "tpopen_data","flvlfile()",buf, strerror(errno));
			Retry = TRUE;
			return (-1);
		}

		/* default state and release flag */
		ttydevstat.dev_relflag = DEV_LASTCLOSE;
		ttydevstat.dev_state = DEV_PRIVATE;

		if(fdevstat(fd,DEV_SET,&ttydevstat) < 0) {
			log(MM_ERROR,
			 ":853:%s: devstat() could not SET security attrs on %s: %s",
			 "tpopen_data",buf, strerror(errno));
			Retry = TRUE;
			return (-1);
		}
	}else{
		/*
		** -Since MAC is not running, just set the level on the file
		**  to the level id (lid) == SYS_PRIVATE.
		*/
		level_t	level = 1;
		(void)flvlfile(fd, MAC_SET, &level);
	}

	/* -free up space for previous data device if one exists
	** -save name of new data device
	*/
	if (pmptr->p_device != (char *)NULL)
		free(pmptr->p_device);
	pmptr->p_device = strsave(buf);
	return (fd);
}



/*
 * Procedure:	  tpclose_device
 *
 * Restrictions:
                 ioctl(2):P_MACWRITE
                 strerror: none
*/

/* tpclose_device(pmptr)
**
** IF there is an open file descriptor for the ctrl channel
**	-unlink it via P_UNLINK
**	-call tpclose_ctrl()
*/
void
tpclose_device(pmptr)
	struct	pmtab	*pmptr;
{

	if (express_mode == TRUE && B2Running == FALSE)
		return;
	if (pmptr->p_tpctrlfd != 0){
		(void) procprivl(CLRPRV, MACWRITE_W, 0);
		if (ioctl(pmptr->p_tpctrlfd, I_PUNLINK, pmptr->p_muxid) < 0){
			(void) procprivl(SETPRV, MACWRITE_W, 0);
			log(MM_ERROR,
			 ":887:%s: I_PUNLINK failed on real device %s: %s",
			 "tpclose_device",pmptr->p_realdevice, strerror(errno));
		}
		(void) procprivl(SETPRV, MACWRITE_W, 0);
		tpclose_ctrl(pmptr);
	}
}

/* tpclose_ctrl(pmptr)
**
** IF there is an open file descriptor for the ctrl channel
**	-close it
*/
void
tpclose_ctrl(pmptr)
	struct	pmtab	*pmptr;
{

	if (pmptr->p_tpctrlfd != 0){
		(void)close(pmptr->p_tpctrlfd);
		pmptr->p_tpctrlfd = 0;
	}
	pmptr->p_muxid = 0;
}

/* tpreinit_realdev(pmptr)
**
** Open and close the real/physical tty device for the given pmtab entry.
** This will effectively raise outgoing DTR for the real/physical tty device
** if it had gone low when the login session terminated.  An asserted
** outgoing DTR is needed inorder to use the real/physical tty device.
*/
void
tpreinit_realdev(pmptr)
	struct	pmtab	*pmptr;
{
	int	tmpfd;

	/*
	** If open gets interrputed, open it again.  It is very important
	** that the open succeeds.
	*/
	while (((tmpfd = open(pmptr->p_realdevice, O_RDWR|O_NONBLOCK|O_NOCTTY))
	 == -1) && (errno == EINTR)); /* NULL STATEMENT */
	(void)close(tmpfd);
}




/*
 *	open_all - open devices in pmtab if the entry is
 *	         - valid, fd == 0, pid == 0, and tpstatus == tpTRUSTEDSTATE
 */
static void
open_all()
{
	struct	pmtab	*tp;
	int	check_modtime;
	static	void	free_defs();
	sigset_t cset;
	sigset_t tset;

#ifdef	DEBUG
	debug("in open_all");
#endif
	check_modtime = TRUE;

	for (tp = PMtab; tp; tp = tp->p_next) {
		if ((tp->p_status > 0) && (tp->p_fd == 0) && (tp->p_pid == 0)
		 && (tp->p_tpctrlfd != 0) && ( !((State == PM_DISABLED)  
		 && ((tp->p_dmsg == NULL)||(*(tp->p_dmsg) == '\0'))) )
		 && (tp->p_tpstatus == tpTRUSTEDSTATE) ) {
			/* 
			 * if we have not check modification time and
			 * /etc/ttydefs was modified, need to re-read it
			 */
			if (check_modtime && mod_ttydefs()) {
				check_modtime = FALSE;
				(void)sigprocmask(SIG_SETMASK, NULL, &cset);
				tset = cset;
				(void)sigaddset(&tset, SIGCLD);
				(void)sigprocmask(SIG_SETMASK, &tset, NULL);
				free_defs();
#ifdef	DEBUG
				debug("/etc/ttydefs is modified, re-read it");
#endif
				read_ttydefs(NULL,FALSE);
				(void)sigprocmask(SIG_SETMASK, &cset, NULL);
			}
			open_device(tp);
			if (tp->p_fd > 0) 
				got_carrier(tp);
		}
		else if ((tp->p_status == SESSION) && (tp->p_fd > 0)
		 && (!((State == PM_DISABLED)
		 && (tp->p_dmsg == NULL)||(*(tp->p_dmsg) == '\0')) )
		 && (tp->p_tpstatus == tpTRUSTEDSTATE)){
			if (check_modtime && mod_ttydefs()) {
				check_modtime = FALSE;
				(void)sigprocmask(SIG_SETMASK, NULL, &cset);
				tset = cset;
				(void)sigaddset(&tset, SIGCLD);
				(void)sigprocmask(SIG_SETMASK, &tset, NULL);
				free_defs();
#ifdef	DEBUG
				debug("/etc/ttydefs is modified, re-read it");
#endif
				read_ttydefs(NULL,FALSE);
				(void)sigprocmask(SIG_SETMASK, &cset, NULL);
			}
			tp->p_status = VALID;
			open_device(tp);
			if (tp->p_fd > 0) 
				got_carrier(tp);
		}
	}
}

/*
 * Procedure:	  open_device
 *
 * Restrictions:
                 fcntl(2): none
                 strerror: none
                 sprintf: none
                 fchown(2): none
		 chown(2): none
                 fchmod(2): none
                 chmod(2): none
*/

/*
 *	open_device(pmptr)	- open the device
 *				- check device lock
 *				- change owner of device
 *				- push line disciplines
 *				- set termio
 */

void
open_device(pmptr)
struct	pmtab	*pmptr;
{
	int	fd;
	struct	sigaction	sigact;

#ifdef	DEBUG
	debug("in open_device");
#endif

	if (pmptr->p_status == GETTY) {
		if ((fd = tpopen_data(pmptr, O_RDWR)) == -1){
			log(MM_ERROR, badopendata, "open_device",pmptr->p_realdevice);
			exit(1);
		}
	}
	else  {
		if (check_spawnlimit(pmptr) == -1) {
			pmptr->p_status = NOTVALID;
			pmptr->p_tpstatus = tpNONTRUSTEDSTATE;
			pmptr->p_reason |= REASspawnlimit;
			/* -close down ctrl channel
			**	-this is to stop it from being polled
			**	-NOTE: TP mux is not dimantled
			** -set p_muxid = 0
			**	-this will allow ctrl channel to be open again
			**	 if and when p_status changes to VALID (via
			**	 re_read()
			*/
			tpclose_device(pmptr);
			log(MM_ERROR, ":726:Service <%s> is respawning too rapidly",pmptr->p_tag);
			return;
		}
		if (pmptr->p_fd > 0) { /* file already open */
			fd = pmptr->p_fd;
			pmptr->p_fd = 0;
		}
		/* -open & connect a data channel to TP device 
		*/
		else{
			/* -set tpstatus to tpNONTRUSTEDSTATE
			**  this will require another SAK to be detected
			**  before a data channel can be connected again
			*/
			pmptr->p_tpstatus = tpNONTRUSTEDSTATE;
			if ((fd = tpopen_data(pmptr, O_RDWR|O_NONBLOCK)) == -1){
				log(MM_ERROR, badopendata, "open_device",pmptr->p_realdevice);
				return;
			}
		}
		/* set close-on-exec flag */
		if (fcntl(fd, F_SETFD, 1) == -1) {
			logexit(1, badfcntl, "F_SETFD", strerror(errno));
		}
		if (check_session(fd) != 0) {
			if ((Initialized) && (pmptr->p_inservice != SESSION)){
				log(MM_WARNING, ":727:Active session exists on <%s>",
					pmptr->p_device);
			}
			else {  
				/* 
				 * this may happen if a service is running
				 * and ttymon dies and is restarted,
				 * or another process is running on the
				 * port.
				 */
				pmptr->p_status = SESSION;
				pmptr->p_inservice = 0;
				(void)close(fd);
				Nlocked++;
				if (Nlocked == 1) {
					sigact.sa_flags = SA_RESETHAND | SA_NODEFER;
					sigact.sa_handler = sigalarm;
					(void)sigemptyset(&sigact.sa_mask);
					(void)sigaction(SIGALRM, &sigact, NULL);
					(void)alarm(ALARMTIME);
				}
				return;
			}
		}
		pmptr->p_inservice = 0;
	}


#ifdef DEBUG
	(void)sprintf(Scratch,"open_device (%s), fd = %d",pmptr->p_device,fd);
	debug(Scratch);
#endif
	/*
	 * Set initial ownership and mode of the Trusted Path data channel to
	 * root and 0620 respectively
	 */
	(void)fchown(fd,ROOTUID,Tty_gid);
	(void)fchmod(fd,0620);

	if ((pmptr->p_modules != NULL)&&(*(pmptr->p_modules) != '\0')) {
		if (push_linedisc(fd,pmptr->p_modules,pmptr->p_device) == -1) {
			Retry = TRUE;
			(void)close(fd);
			pmptr->p_tpdataconnid = 0;
			return; 
		}
	}

	if (initial_termio(fd, pmptr) == -1)  {
		Retry = TRUE;
		(void)close(fd);
		pmptr->p_tpdataconnid = 0;
		return;
	}

	pmptr->p_fd = fd;
}

/*
 *	set_poll(fdp)	- put all fd's in a pollfd array
 *			- set poll event to POLLIN and POLLMSG
 *			- return number of fd to be polled
 */

int
set_poll(fdp)
struct pollfd *fdp;
{
	struct	pmtab	*tp;
	int 	nfd = 0;

	for (tp = PMtab; tp; tp = tp->p_next) {
		if (tp->p_fd > 0)  {
			fdp->fd = tp->p_fd;
			fdp->events = POLLIN;
			fdp++;
			nfd++;
		}
		if (tp->p_tpctrlfd > 0){
			fdp->fd = tp->p_tpctrlfd;
			fdp->events = POLLIN|POLLPRI;
			fdp++;
			nfd++;
		}
	}
	return(nfd);
}

/*
 *	check_spawnlimit	- return 0 if spawnlimit is not reached
 *				- otherwise return -1
 */
static	int
check_spawnlimit(pmptr)
struct	pmtab	*pmptr;
{
	long	now;
	extern	time_t	time();

	(void)time(&now);
	if (pmptr->p_time == 0L)
		pmptr->p_time = now;
	if (pmptr->p_respawn >= SPAWN_LIMIT) {
		if ((now - pmptr->p_time) < SPAWN_INTERVAL) {
			pmptr->p_time = now;
			pmptr->p_respawn = 0;
			/*
			 * If time difference is less than zero,
			 * somebody must have changed the time backward.
			 * So start over.
			 */
			if ((now - pmptr->p_time) < 0) 
				return(0);
			else
				return(-1);
		}
		pmptr->p_time = now;
		pmptr->p_respawn = 0;
	}
	pmptr->p_respawn++;
	return(0);
}

/*
 * Procedure:	  mod_ttydefs
 *
 * Restrictions:
		 stat(2): none
*/

/*
 * mod_ttydefs	- to check if /etc/ttydefs has been modified
 *		- return TRUE if file modified
 *		- otherwise, return FALSE
 */
static	int
mod_ttydefs()
{
	struct	stat	statbuf;
	static	long	mtime = 0; /* last modification time of ttydefs  */
	if (stat(TTYDEFS, &statbuf) == -1) {
		/* if stat failed, don't bother reread ttydefs */
		return(FALSE);
	}
	if ((long)statbuf.st_mtime != mtime) {
		mtime = (long)statbuf.st_mtime;
		return(TRUE);
	}
	return(FALSE);
}

/*
 *	free_defs - free the Gdef table
 */
static	void
free_defs()
{
	int	i;
	struct	Gdef	*tp;
	tp = &Gdef[0];
	for (i=0; i<Ndefs; i++,tp++) {
		free(tp->g_id);
		free(tp->g_iflags);
		free(tp->g_fflags);
		free(tp->g_nextid);
		tp->g_id = NULL;
		tp->g_iflags = NULL;
		tp->g_fflags = NULL;
		tp->g_nextid = NULL;
	}
	Ndefs = 0;
	return;
}

/*
 * struct Gdef *get_speed(ttylabel) 
 *	- search "/etc/ttydefs" for speed and term. specification 
 *	  using "ttylabel". If "ttylabel" is NULL, default
 *	  to default_setting
 * arg:	  ttylabel - label/id of speed settings.
 */

struct Gdef *
get_speed(ttylabel)
char	*ttylabel;
{
	register struct Gdef *sp;

	if ((ttylabel != NULL) && (*ttylabel != '\0')) {
		if((sp = find_def(ttylabel)) == NULL) {
			log(MM_ERROR, ":728:Unable to find <%s> in \"%s\"",
			    ttylabel,TTYDEFS);
			sp = &default_setting; /* use default */
		}
	} else sp = &default_setting; /* use default */
	return(sp);
}

/*
 * Procedure:	  setup_PCpipe
 *
 * Restrictions:
                 strerror: none
                 fcntl(2): none
                 ioctl(2): none
*/

/*
 * setup_PCpipe()	- setup the pipe between Parent and Children
 *			- the pipe is used for a tmchild to send its
 *			  pid to inform ttymon that it is about to
 *			  invoke service
 *			- the pipe also serves as a mean for tmchild
 *			  to detect failure of ttymon
 */
void
setup_PCpipe()
{
	int	flag = 0;
	static const char name[] = "setup_PCpipe";

	if (pipe(PCpipe) == -1 ) {
		logexit(1, efailed, "pipe()", strerror(errno));
	}
	
	/* set close-on-exec flag */
	if (fcntl(PCpipe[0], F_SETFD, 1) == -1) {
		logexit(1, badfcntl, "F_SETFD", strerror(errno));
	}
	if (fcntl(PCpipe[1], F_SETFD, 1) == -1) {
		logexit(1, badfcntl, "F_SETFD", strerror(errno));
	}

	/* set O_NONBLOCK flag */
	if (fcntl(PCpipe[0], F_GETFL, flag) == -1) {
		logexit(1, badfcntl, "F_GETFL", strerror(errno));
	}
	flag |= O_NONBLOCK;
	if (fcntl(PCpipe[0], F_SETFL, flag) == -1) {
		logexit(1, badfcntl, "F_SETFL", strerror(errno));
	}

	/* set message discard mode */
	if (ioctl(PCpipe[0], I_SRDOPT, RMSGD) == -1) {
		logexit(1, badioctl, name, "I_SRDOPT RMSGD", strerror(errno));
	}

	/* register to receive SIGPOLL when data come */
	if (ioctl(PCpipe[0], I_SETSIG, S_INPUT) == -1) {
		logexit(1, badioctl, name, "I_SETSIG S_INPUT", strerror(errno));
	}

#ifdef 	DEBUG
	dlog("PCpipe[0]\t = %d", PCpipe[0]);
	dlog("PCpipe[1]\t = %d", PCpipe[1]);
#endif
}

/*
 * Procedure:	  alloc_pollarray
 *
 * Restrictions:
                 strerror: none
*/

/* sets up poll array ... mallocs struct pollfd if needed */
void
alloc_pollarray(){

	static int npollfd = 0;		/* maximum number of file descriptors
					** to poll
					*/ 

	if ((Nentries * 2) > npollfd){
#ifdef DEBUG
		if (npollfd > 0)
			debug("Nentries > npollfd, reallocating pollfds");
#endif
		/*need to allocate pollfd structure*/

		if (pollp)
			free(pollp);
		npollfd = (Nentries * 2) + 20;
		if (npollfd > Maxfds)
			npollfd = Maxfds;

		if ((pollp = (struct pollfd *)malloc((unsigned)(npollfd * sizeof(struct pollfd)))) == (struct pollfd *)NULL)
			logexit(1, nomem, strerror(errno));
	}
}
