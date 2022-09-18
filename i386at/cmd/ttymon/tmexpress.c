/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ttymon:i386at/cmd/ttymon/tmexpress.c	1.19.15.8"
#ident  "$Header: $"

#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<errno.h>
#include	<ctype.h>
#include	<string.h>
#include	<signal.h>
#include	<sys/stat.h>
#include	<utmp.h>
#include	<stropts.h>
#include	<poll.h>
#include	<pfmt.h>
#include	"ttymon.h"
#include	"tmextern.h"
#include	<sys/termio.h>
#include	<sys/stream.h>
#include	<sys/tp.h>
#include	"tmstruct.h"
#include	<mac.h>
#include	<priv.h>
#include	<deflt.h>

#define	REAL_DEVICE	"/dev/console"

static	char	devbuf[BUFSIZ];
static	char	*devname;
static	char	*ttyn	= NULL;
static	struct	sak nullsak;
static	struct	termios nulltermios;

static	int	parse_args();
static	void	ttymon_options();
static	int	getdfltsak();
static	void	getty_options();
static	void	usage();
static	char	*get_tty();
static  char	*find_ttyname();

static  int	vflag;

extern	int	MACRunning;
extern	int	B2Running;
extern	int	EnhancedSecurityInstalled;
extern  int	express_mode;
extern	char	*Consoledsf;
extern   char	*ttyname();

extern  void	killvts();
extern	void	log();
extern	void	tmchild();
extern	int	check_identity();
extern	int	turnon_canon();
extern	int	vml();
extern	int	check_sak();		/* see tmutil.c */
extern	int	set_saktype();		/* see tmutil.c */
extern	int	set_sakdef();		/* see tmutil.c */
extern	int	set_saksec();		/* see tmutil.c */
extern	int	tp_datadisconnect();
extern	pid_t	getsid(), tcgetsid();
extern	char	*getword();
extern	char	*strsave();

extern const char badopen[];
const char badusage[] = ":8:Incorrect usage\n";

/*
 * Procedure:	  ttymon_express
 *
 * Restrictions:
                 sprintf: none
                 lvlin: none
 		 strerror: none
		 lvlfile(2): none
		 devstat(2): none
		 open(2): none
*/

/*
 * ttymon_express - This is call when ttymon is invoked with args
 *		    or invoked as getty
 *		  - This special version of ttymon will monitor
 *		    one port only
 *		  - It is intended to be used when some process
 *		    wants to have a login session on the fly
 */
void
ttymon_express(argc,argv)
int	argc;
const	char	*argv[];
{
	struct	pmtab	*pmtab;
	struct	sigaction	sigact;
	struct	pollfd	pfd;
		int	npfd;
	struct	dev_alloca devddb;	/* dev alloc. attributes */
	struct	devstat	ttydevstat;	/* devstat() kernel attrs */
	struct	tp_info	tpinf;
	extern	int	Retry;
	extern	void	tpopen_device();
	extern	int	set_poll();
	extern	void	do_poll();
	extern	void	open_device();
	extern	void	read_ttydefs();
	extern	void	getty_account();
	extern	int	devalloc();
	extern	int	devstat();
#ifdef	DEBUG
	extern	char	Scratch[];
	extern	FILE	*Debugfp;
	extern	void	opendebug();
#endif

#ifdef	DEBUG
	opendebug(TRUE);
#endif

	express_mode = TRUE;

	sigact.sa_flags = 0;
	sigact.sa_handler = SIG_IGN;
	(void)sigemptyset(&sigact.sa_mask);
	(void)sigaction(SIGINT, &sigact, NULL);

	/*
	 * If we are running MAC, then we should be running at
	 * SYS_PRIVATE level so we can access the I&A database
	 * without using P_MACREAD privilege. Some invocations
	 * of ttymon express may be done at other levels by
	 * networking applications (like listen.) We'll try
	 * to set the level to SYS_PRIVATE since we should have
	 * inherited P_SETPLEVEL anyway. If this fails, we'll continue
	 * and let the authentication scheme fail us as it sees fit.
	 */

	if (MACRunning == TRUE) {
		level_t sys_private, tm_level;

		if ( (lvlproc(MAC_GET, &tm_level) == 0)
			&& (lvlin("SYS_PRIVATE", &sys_private) == 0)
			&& (lvlequal(&tm_level, &sys_private) == 0) ) {
			(void)lvlproc(MAC_SET, &sys_private);
		}
	}

	if ((PMtab = pmtab = ALLOC_PMTAB) == PNULL) {
		log(MM_HALT, ":849:%s: ALLOC_PMTAB failed","ttymon_express");
		exit(1);
	}

	if (parse_args(argc, argv, pmtab) != 0) {
		log(MM_HALT, ":850:%s: parse_args() failed","ttymon_express");
		exit(1);
	}

	read_ttydefs(NULL,FALSE);


	/* IF real device is not specified
	**	-for now assume device has already been set up
	*/
	if ((pmtab->p_realdevice == NULL) || (*(pmtab->p_realdevice) == '\0')) {
/*
		if (tcgetsid(0) != getsid(getpid()) ) {
*/
			devname = find_ttyname(0);
			if ((devname == NULL) || (*devname == '\0')) {
				log(MM_HALT, ":624:ttyname cannot find the device on fd 0");
				exit(1);
			}
			pmtab->p_realdevice = devname;
#ifdef	DEBUG
			(void)sprintf(Scratch,"ttymon_express: devname = %s",
					 devname);
			debug(Scratch);
#endif

			if (MACRunning == TRUE){
				/*
				** -Get security attrs from Device Database and
				**  set security attrs on the tty device
				*/
				if (devalloc(pmtab->p_realdevice ,DEV_GET,
				 &devddb) < 0) {
					/* log(MM_ERROR,
					":877:%s: could not GET security attrs of %s from DDB", 
                                        "ttymon_express",pmtab->p_realdevice); */

					/* set default sec. attrs */
					ttydevstat.dev_mode = DEV_STATIC;
					lvlin("SYS_RANGE_MAX", &ttydevstat.dev_hilevel);
					lvlin("SYS_PUBLIC", &ttydevstat.dev_lolevel);
				}else{
					/*
					** build sec. attrs from values in DDB
					** for device
					*/
					ttydevstat.dev_mode = devddb.mode;
					ttydevstat.dev_hilevel = devddb.hilevel;
					ttydevstat.dev_lolevel = devddb.lolevel;
				}

				/* free device control information on real
				** device
				*/
				ttydevstat.dev_relflag = DEV_SYSTEM;
				if(devstat(pmtab->p_realdevice,DEV_SET,&ttydevstat) < 0) {
					log(MM_ERROR,
					 ":851:%s: %s could not SET to DEV_SYSTEM on %s: %s",
                                         "ttymon_express","devstat()",pmtab->p_realdevice, strerror(errno));
					exit(1);
				}

				/* change the level of the device to the hilevel range */
				if(lvlfile(pmtab->p_realdevice,MAC_SET,&ttydevstat.dev_hilevel) < 0) {
					log(MM_ERROR, 
					 ":852:%s: %s could not change level %s: %s", 
                                         "ttymon_express","lvlfile()",pmtab->p_realdevice,  strerror(errno));
					exit(1);
				}

				/* default state and release flag */
				ttydevstat.dev_relflag = DEV_LASTCLOSE;
				ttydevstat.dev_state = DEV_PRIVATE;

				if(devstat(pmtab->p_realdevice,DEV_SET,&ttydevstat) < 0) {
					log(MM_ERROR, 
					":853:%s: devstat() could not SET security attrs on %s: %s", 
                                        "ttymon_express",pmtab->p_realdevice, strerror(errno));
					exit(1);
				}
			}





			/*
			 * become session leader 
		 	 * fd 0 is closed and reopened just to make sure
		 	 * controlling tty is set up right
		 	 */
			(void)setsid();
			(void)close(0);
			if (open(pmtab->p_realdevice, O_RDWR) < 0) {
				log(MM_HALT, badopen, pmtab->p_realdevice, strerror(errno));
				exit(1);
			}
/*
		}
#ifdef	DEBUG
		else {
			(void)sprintf(Scratch,
			"ttymon_express: controlling tty already setup.");
			debug(Scratch);
			(void)sprintf(Scratch,"tcgetsid = %d, getsid =%d",
					 tcgetsid(0),getsid(getpid()));
			debug(Scratch);
		}
#endif
*/
		if ((pmtab->p_modules != NULL) &&
		    (*(pmtab->p_modules) != '\0')) {
		   if (push_linedisc(0,pmtab->p_modules,pmtab->p_realdevice) == -1)
			exit(1);
		}
		if (initial_termio(0, pmtab) == -1) 
			exit(1);
	}
	else {
		if (vflag == TRUE)
			killvts(pmtab->p_realdevice);

		/* If sak type is NONE, set TP state to "trusted state", so
		** a data channel will be opened and connected immediately.
		** This is so ttymon does not have to for a pseudo SAK before
		** opening and connecting a data channel.
		*/
		if (pmtab->p_sak.sak_type == saktypeNONE){
			pmtab->p_tpstatus = tpTRUSTEDSTATE;
		}
		/* -setup TP device for real device
		** -poll ctrl channel
		** -open and connect data channel
		*/
		Retry = FALSE;
		tpopen_device(pmtab);
		if (Retry)		/* open TP device failed */
			exit(1);

		/*
		** If sak type is NONE, cat tp_datadisconnect() to
		** disconnect a pre-existing data channel that may still
		** be opened.  Since there is no TP ctrl channel to monitor
		** there may be a old data channel connected to the TP device.
		*/
		if ( B2Running == TRUE && pmtab->p_sak.sak_type == saktypeNONE){
			TP_LOADINF(tpinf, (dev_t)-1, (dev_t)-1, 0, 0, (dev_t)-1,
			 (dev_t)-1, (dev_t)-1, 0, 0, 0, 0, nullsak, 0,
			 nulltermios, nulltermios);
			(void) procprivl(CLRPRV, MACWRITE_W, 0);
			(void)tp_datadisconnect(pmtab->p_tpctrlfd, &tpinf);
			(void) procprivl(SETPRV, MACWRITE_W, 0);
		}

		/* If status of TP is not in "trusted state", poll the ctrl
		** channel for a SAK before opening and connecting a data
		** channel.  If status of TP is in "trusted state", can open
		** and connect the data channel immediately.
		*/
		if (B2Running == TRUE && pmtab->p_tpstatus != tpTRUSTEDSTATE){
			npfd = set_poll(&pfd);
			do_poll(&pfd, npfd);
			if (Retry)		/* poll failed */
				exit(1);
		}
		(void)setsid();
		(void)close(0);
		open_device(pmtab);
		if (Retry)		/* open failed */
			exit(1);
		getty_account(pmtab->p_device); /* utmp accounting */
	}
	tmchild(PMtab);
	exit(1);	/*NOTREACHED*/
}

/*
 * parse_arg	- parse cmd line arguments
 */
static	int
parse_args(argc, argv, pmtab)
int	argc;
const	char	*argv[];
struct	pmtab	*pmtab;
{
	extern	char	*lastname();

	/* initialize fields to some default first */
	pmtab->p_tag = "";
	pmtab->p_flags = 0;
	pmtab->p_identity = "";
	pmtab->p_res1 = "reserved";
	pmtab->p_res2 = "reserved";
	pmtab->p_iascheme = "login";
	if (check_identity(pmtab) != 0) {
		exit(1);
	}
	pmtab->p_ttyflags = 0;
	pmtab->p_count = 0;
	pmtab->p_server = "/usr/bin/shserv";
	pmtab->p_timeout = 0;
	pmtab->p_modules = "";
	pmtab->p_prompt = "login: ";
	pmtab->p_dmsg = "";
	pmtab->p_device = "";
	pmtab->p_realdevice = "";
	pmtab->p_status = GETTY;
	pmtab->p_tpstatus = tpNONTRUSTEDSTATE;
	pmtab->p_next = (struct pmtab *)NULL;

	if (strcmp(lastname(argv[0]), "getty") == 0) {
		pmtab->p_ttylabel = "300";
		getty_options(argc,argv,pmtab);
		/* -hard code SAK to type NONE and TPstatus to "trusted state"
		**  since getty does not know about SAK options.
		*/
		pmtab->p_sak.sak_type = saktypeNONE;
		pmtab->p_tpstatus = tpTRUSTEDSTATE;
		pmtab->p_modules = "ldterm";
	}
	else {
		pmtab->p_ttylabel = "9600";
		ttymon_options(argc,argv,pmtab);
	}
	return(0);
}

/*
 * Procedure:	  ttymon_options
 *
 * Restrictions:
                 getopt: none
*/

/*
 * 	ttymon_options - scan and check args for ttymon express 
 */

static	void
ttymon_options(argc, argv,pmtab)
int argc;
char *argv[];
struct pmtab	*pmtab;
{
	int 	c;			/* option letter */
	char 	*timeout;
	int  	gflag = 0;		/* -g seen */
	int	kflag = 0;		/* -k seen */
	int	size = 0;
	char	tbuf[BUFSIZ];
	char	*options;
	char	*saktypep = "n";	/* sak type NONE is the default SAK */
	char	*sakdefp = "";
	char	*saksecp = "";
	int	ret;

	extern	char	*optarg;
	extern	int	optind;
	extern	void	copystr();
	extern	char	*strsave();
	extern	char	*getword();



	if (EnhancedSecurityInstalled == TRUE)
		options = "vgd:ht:p:m:l:k:K:x";
	else
		options = "vgd:ht:p:m:l:";



	while ((c = getopt(argc, argv, (const char *)options)) != -1) {
		switch (c) {
		case 'g':
			gflag = 1;
			break;
		case 'd':
			pmtab->p_realdevice = get_tty(optarg);
			break;
		case 'h':
			pmtab->p_ttyflags &= ~H_FLAG;
			break;
/*
		case 'b':
			pmtab->p_ttyflags |= B_FLAG;
			pmtab->p_ttyflags |= R_FLAG;
			break;
*/
		case 't':
			timeout = optarg;
			while (*optarg) {
				if (!isdigit(*optarg++)) {
					log(MM_ERROR, ":625:Invalid argument for \"-t\" -- number expected.");
					usage(0);
				}
			}
			pmtab->p_timeout = atoi(timeout);
			break;
		case 'v':
			vflag = 1;
			break;
		case 'p':
			copystr(tbuf, optarg);
			pmtab->p_prompt = strsave(getword(tbuf,&size,TRUE));
			break;
		case 'm':
			pmtab->p_modules = optarg;
			if (vml(pmtab->p_modules) != 0) 
				usage(1);
			break;
		case 'l':
			pmtab->p_ttylabel = optarg;
			break;
		case 'k':
			kflag = 1;
			saktypep = optarg;
			break;
		case 'K':
			kflag = 1;
			sakdefp = optarg;
			break;
		case 'x':
			kflag = 1;
			saksecp = "drop";
			break;
		case '?':
			usage(0);
			break;	/*NOTREACHED*/
		}
	}
	if (optind < argc)
		usage(1);

	if (!gflag) 
		usage(1);

	/*
	 * If B2 enhanced security is running and SAK specification has not
	 * been specified and a Device Special File has been specified,
	 * get SAK specification from DEFLT file DEFLT_TTYMON_FILE.
	 */
	if ((B2Running == TRUE) && !(kflag) &&
	 (*pmtab->p_realdevice != '\0')){
		if (getdfltsak(pmtab->p_realdevice, &saktypep, &sakdefp,
		 &saksecp) == FAILURE){
			/*
			 * Use default SAK if device is the system console
			 * Device Special File.  We do not want to prevent
			 * access to the console, but display a warning
			 * message.
			 */
			if (strcmp(pmtab->p_realdevice, Consoledsf) == 0){
				log(MM_WARNING,
				 ":900:Secure Attention Key is disabled\n");
			}else{
				exit(1);
			}
		}else if ((ret = check_sak(saktypep, sakdefp, saksecp)) != 0){
			if (strcmp(pmtab->p_realdevice, Consoledsf) == 0){
				log(MM_WARNING,
				 ":900:Secure Attention Key is disabled\n");
				/*
				 * Reset default SAK definitions for Console
				 * device.
				 */
				saktypep = "n";
				sakdefp = "";
				saksecp = "";
			}else{
				log(MM_ERROR,
				 ":901:Invalid Secure Attention Specification in %s/%s for %s\n", DEFLT, DEFLT_TTYMON_FILE, pmtab->p_realdevice);
				exit(1);
			}
		/*
		 * If no default SAK is defined (ie. it was removed) for the
		 * tty device, just exit unless this is the console device.
		 */
		}else if (*saktypep == '\0'){
			if (strcmp(pmtab->p_realdevice, Consoledsf) == 0){
				log(MM_WARNING,
				 ":900:Secure Attention Key is disabled\n");
				/*
				 * Reset default SAK definitions for Console
				 * device.
				 */
				saktypep = "n";
				sakdefp = "";
				saksecp = "";
			}else{
				exit(1);
			}
		}
	}else if ((ret = check_sak(saktypep, sakdefp, saksecp)) != 0){
		if (ret == 1)
			usage(1);
		else
			exit(1);
	}


#ifdef DEBUG

	if (set_saktype(saktypep, &pmtab->p_sak.sak_type) == -1)
		dlogexit(1, "ttymon_options: set_saktype() failed");
	if (set_sakdef(sakdefp, &pmtab->p_sak) == -1)
		dlogexit(1, "ttymon_options: set_sakdef() failed");
	if (set_saksec(saksecp, &pmtab->p_sak) == -1)
		dlogexit(1, "ttymon_options: set_saksec() failed");
#else

	(void)set_saktype(saktypep, &pmtab->p_sak.sak_type);
	(void)set_sakdef(sakdefp, &pmtab->p_sak);
	(void)set_saksec(saksecp, &pmtab->p_sak);
#endif

	/*
	 * If B2 enhanced security is running, login timeout value is zero, and
	 * SAK is not NONE, get the login timeout value, TIMEOUT from the login
	 * /etc/default file.  If TIMEOUT is not defined use a hard coded
	 * default login timeout, DEFLT_LOGIN_TIMEOUT.
	 */
	if ((B2Running == TRUE) && (pmtab->p_timeout == 0) &&
	 (pmtab->p_sak.sak_type != saktypeNONE)){
		FILE	*loginfp;
		char	*linep;
		if ((loginfp = defopen(DEFLT_LOGIN_FILE)) == (FILE *)NULL){
			log(MM_WARNING, ":902:Cannot open %s/%s\n",DEFLT,
			 DEFLT_LOGIN_FILE);
			pmtab->p_timeout = DEFLT_LOGIN_TIMEOUT;
		}else if ((linep = defread(loginfp, "TIMEOUT")) ==
		 (char *)NULL){
			log(MM_WARNING, ":903:%s not found in %s/%s\n", "TIMEOUT",
		 	DEFLT, DEFLT_LOGIN_FILE);
			pmtab->p_timeout = DEFLT_LOGIN_TIMEOUT;
			(void)defclose(loginfp);
		}else{
			pmtab->p_timeout = atoi(linep);
			(void)defclose(loginfp);
		}
	}
}

/*
 * Procedure:	getdfltsak
 *
 * Restrictions:
		defopen:none
		defread:none

*/

/*
 * getdfltsak -	get Secure Attention Key definition from
 *		DEFLT/DEFLT_TTYMON_FILE
 */
static int
getdfltsak(device, saktypepp, sakdefpp, saksecpp)
char	*device;
char	**saktypepp;
char	**sakdefpp;
char	**saksecpp;
{
	FILE	*tmxfp;			/* file pointer to
					 * DEFLT/DEFLT_TTYMON_FILE file which
					 * contains SAK definitions for tty
					 * devices that are run in ttymon
					 * express mode.
					 */
	char	*saktypep;
	char	*sakdefp;
	char	*saksecp;
	char	line[BUFSIZ];
	char	*linep;
	int	state;
	int	size;


	if ((tmxfp = defopen(DEFLT_TTYMON_FILE)) == (FILE *)NULL){
		log(MM_ERROR, ":902:Cannot open %s/%s\n",DEFLT,
		 DEFLT_TTYMON_FILE);
		return (FAILURE);
	}
	if ((linep = defread(tmxfp, device)) == (char *)NULL){
		log(MM_ERROR, ":903:%s not found in %s/%s\n", device,
		 DEFLT, DEFLT_TTYMON_FILE);
		(void)defclose(tmxfp);
		return (FAILURE);
	}

	(void)strcpy(line, linep);
	linep = line;
	for (state = P_SAKTYPE; (state != SUCCESS) && (state != FAILURE);){
		switch (state){
		case P_SAKTYPE:
			saktypep = strsave(getword(linep, &size, 0));
			break;
		case P_SAKDEF:
			sakdefp = strsave(getword(linep, &size, 0));
			break;
		case P_SAKSEC:
			saksecp = strsave(getword(linep, &size, 0));
			break;
		}
		linep += size;

		if (state == P_SAKSEC){
			if (*linep == ':'){
				state = SUCCESS;
			}else{
				state = FAILURE;
			}
		}else{
			if (*linep == ':'){
				/*
				 * skip the ':'
				 */
				linep++;
				state++;
			}else{
				state = FAILURE;
			}
		}
	}

	if (state == SUCCESS){
		*saktypepp = saktypep;
		*sakdefpp = sakdefp;
		*saksecpp = saksecp;
	}else{
		log(MM_ERROR, ":904:Parsing error in %s/%s for %s\n",
		 DEFLT, DEFLT_TTYMON_FILE, device);
	}

	defclose(tmxfp);
	return (state);
}

/*
 * Procedure:	  usage
 *
 * Restrictions:
                 isatty: none
             	 pfmt: none
                 open(2): none
             	 fclose: none
*/

/*
 * usage - print out a usage message
 */

static 	void
usage(complain)
int complain;
{
	int	fd;
	char	*umsg;

	if (EnhancedSecurityInstalled == TRUE)
		umsg  = ":854:Usage: ttymon\n\tttymon -g [-h] [-d device] [-l ttylabel] [-t timeout] [-p prompt] [-m modules]\n\t\t[ -k SPECSAKtype | -k SAKtype -K SAK [-x] ]\n";
	else
		umsg  = ":1134:Usage: ttymon\n\tttymon -g [-v] [-h] [-d device] [-l ttylabel] [-t timeout] [-p prompt] [-m modules]\n";



	if (isatty(2)){
		(void)pfmt(stderr, MM_ERROR, badusage);
		(void)pfmt(stderr, MM_ACTION, umsg);
	}
	else {
		if ((fd = open(CONSOLE, O_WRONLY|O_NOCTTY)) != -1){
			FILE *sfd = fdopen(fd, "w");
			if (sfd){
				pfmt(sfd, MM_ERROR, badusage);
				pfmt(sfd, MM_ACTION, umsg);
				(void)fclose(sfd);
			}
			else
				(void)close(fd);
		}
	}
	exit(1);
}

/*
 * Procedure:	  getty_options
 *
 * Restrictions:
		 sscanf: none
*/

/*
 *	getty_options	- this is cut from getty.c
 *			- it scan getty cmd args 
 *			- modification is made to stuff args in pmtab
 */
static	void
getty_options(argc,argv,pmtab)
int argc;
char **argv;
struct	pmtab	*pmtab;
{
	char	*ptr;

	/* 
	 * the pre-4.0 getty's hang_up_line() is a no-op.
	 * For compatibility, H_FLAG cannot be set for this "getty".
	 */
	pmtab->p_ttyflags &= ~(H_FLAG);

	while(--argc && **++argv == '-') {
		for(ptr = *argv + 1; *ptr;ptr++) 
		switch(*ptr) {
		case 'h':
			break;
		case 't':
			if(isdigit(*++ptr)) {
				(void)sscanf(ptr,"%d",&(pmtab->p_timeout));
				while(isdigit(*++ptr));
				ptr--;
			} else if(--argc) {
				if(isdigit(*(ptr = *++argv)))
					(void)sscanf(ptr,"%d",&(pmtab->p_timeout));
				else {
					log(MM_ERROR, ":627:Timeout argument <%s> invalid",
						 *argv);
					exit(1);
				}
			}
			break;

		case 'c':
			log(MM_ERROR, ":628:Use \"sttydefs -l\" to check /etc/ttydefs.");
			exit(0);
		default:
			break;
		}
	}

	if(argc < 1) {
		log(MM_ERROR, ":629:No terminal line specified.");
		exit(1);
	} 
	else {
		(void)strcat(devbuf,"/dev/");
		(void)strcat(devbuf,*argv);
		pmtab->p_realdevice = devbuf;
	}

	if(--argc > 0 ) {
		pmtab->p_ttylabel = *++argv;
	} 

	/*
	 * every thing after this will be ignored
	 * i.e. termtype and linedisc are ignored
	 */
}

/*
 * Procedure:	  find_ttyname
 *
 * Restrictions:
                 getutent: none
                 stat(2): none
                 sprintf: none
		 ttyname: none
*/

/*
 * find_ttyname(fd) 	- find the name of device associated with fd.
 *			- it first tries utmp to see if an entry exists
 *			- with my pid and ut_line is defined. If ut_line
 *			- is defined, it will see if the major and minor
 *			- number of fd and devname from utmp match.
 *			- If utmp search fails, ttyname(fd) will be called.
 */
static	char	*
find_ttyname(fd)
int	fd;
{
	register o_pid_t ownpid;
	register struct utmp *u;
	static	struct	stat	statf, statu;
	static	char	buf[BUFSIZ];
	char *devname;

	ownpid = (o_pid_t)getpid();
	setutent();
	while ((u = getutent()) != NULL) {
		if (u->ut_pid == ownpid) {
			if (strlen(u->ut_line) != 0) {
				if (*(u->ut_line) != '/') {
					(void)strcpy(buf, "/dev/");
					(void)strncat(buf, u->ut_line, 
						sizeof(u->ut_line));
				}
				else {
					(void)strncat(buf, u->ut_line, 
						sizeof(u->ut_line));
				}
			}
			else
				u = NULL;
			break;
		}
	}
	endutent();
	if (	(u != NULL) &&
		(fstat(fd, &statf) == 0) && 
		(stat(buf, &statu) == 0) &&
		(statf.st_dev == statu.st_dev) &&
		(statf.st_rdev == statu.st_rdev)    ) {
#ifdef	DEBUG
			(void)sprintf(Scratch,
			"ttymon_express: find device name from utmp.");
			debug(Scratch);
#endif
			return(buf);
	}
	else {
#ifdef	DEBUG
		debug("ttymon_express: calling ttyname to find device name.");
#endif
		devname = ttyname(fd);
		return(devname);
	}
}

static	char *
get_tty(lttyn)
char	*lttyn;
{

	if (((strcmp(lttyn, "/dev/syscon") == 0) ||
	     (strcmp(lttyn, "/dev/sysconreal") == 0) ||
	     (strcmp(lttyn, "/dev/systty") == 0)) &&
	     (access("/dev/console", F_OK) == 0))
		lttyn = "/dev/console";

	return lttyn;
}

