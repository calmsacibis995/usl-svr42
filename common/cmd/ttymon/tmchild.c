/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ttymon:common/cmd/ttymon/tmchild.c	1.18.17.7"
#ident  "$Header: tmchild.c 1.3 91/06/24 $"

#include	<stdlib.h>
#include	<stdio.h>
#include	<fcntl.h>
#include	<errno.h>
#include	<sys/types.h>
#include	<termio.h>
#include	<string.h>
#include	<signal.h>
#include	<poll.h>
#include	<unistd.h>
#include	<iaf.h>
#include	<priv.h>
#include	<pfmt.h>
#include 	"sys/stropts.h"
#include	<sys/resource.h>
#include	"sac.h"
#include	"ttymon.h"
#include	<sys/termios.h>
#include	<sys/stream.h>
#include	<sys/tp.h>
#include	"tmstruct.h"
#include	"tmextern.h"
#ifndef SNI
#ifdef	SYS_NAME
#include	<sys/utsname.h>
#endif
#endif /* ! SNI */

static void openline();
static void invoke_service();
static char	*do_autobaud();
static	struct	Gdef	*next_speed();
static int check_hup();

extern	struct	Gdef	*get_speed();
extern	pid_t	tcgetsid(), getsid();

extern const char badopen[], badioctl[], efailed[];
const char
	badpoll[] = ":605:%s: poll() failed: %s",
	badset_id[] = ":846:set_id(%s) failed: %s",
	badftermio[] = ":606:Set final termio failed";

/*
 * Procedure:	  tmchild
 *
 * Restrictions:
                 ioctl(2): none
								 strerror: none
*/

/*
 * tmchild	- process that handles peeking data, determine baud rate
 *		  and invoke service on each individual port.
 *
 */
void
tmchild(pmtab)
struct	pmtab	*pmtab;
{
	register struct Gdef *speedef;
	char	*auto_speed = NULL;
	int	first=FALSE;
	struct	sigaction sigact;

#ifdef	DEBUG
	debug("in tmchild");
#endif
	if (pmtab->p_status != GETTY) {
		child_sigcatch(); 
		(void)close(PCpipe[0]);	  /* close parent end of the pipe */
		if (ioctl(PCpipe[1], I_SETSIG, S_HANGUP) == -1) {
			log(MM_HALT, badioctl, "tmchild", "I_SETSIG", strerror(errno));
			exit(1);
		}
		/*
		 * the following check is to make sure no hangup
		 * happens before registering for SIGPOLL
		 */
		if (check_hup(PCpipe[1])) {
#ifdef	DEBUG
			debug("PCpipe hungup, tmchild exiting");
#endif
			exit(1);
		}

		/*
		 * become the session leader so that a controlling tty
		 * will be allocated.
		 */
		(void)setsid();
		first = TRUE;

	}

	speedef = get_speed(pmtab->p_ttylabel);
	openline(pmtab); 
	if ((pmtab->p_ttyflags & C_FLAG) &&
	    (State != PM_DISABLED) &&
	    (!(pmtab->p_flags & X_FLAG))) {
		/*
		 * if "c" flag is set, and the port is not disabled
		 * invoke service immediately 
		 */
		if (set_termio(0,speedef->g_fflags,NULL,FALSE,CANON) == -1) {
			log(MM_HALT, badftermio);
			exit(1);
		}
		invoke_service(pmtab);
		exit(1);	/*NOTREACHED*/
	}
	if (speedef->g_autobaud & A_FLAG) {
		auto_speed = do_autobaud(pmtab,speedef);
	}
	if ( (pmtab->p_ttyflags & (R_FLAG|A_FLAG)) ||
		(pmtab->p_status == GETTY) || (pmtab->p_timeout > 0) ) {
		write_prompt(1,pmtab,TRUE,TRUE);
		if(pmtab->p_timeout) {
			sigact.sa_flags = SA_RESETHAND | SA_NODEFER;
			sigact.sa_handler = timedout;
			(void)sigemptyset(&sigact.sa_mask);
			(void)sigaction(SIGALRM, &sigact, NULL);
			(void)alarm((unsigned)pmtab->p_timeout);
		}
	}

	/* Loop until user is successful in invoking service. */
	for(;;) {

		/* Peek the user's typed response and respond appropriately. */
		switch(poll_data(first)) {
		case GOODNAME:
#ifdef	DEBUG
			debug("got GOODNAME");
#endif	
			if (pmtab->p_timeout) {
				(void)alarm((unsigned)0);
				sigact.sa_flags = 0;
				sigact.sa_handler = SIG_DFL;
				(void)sigemptyset(&sigact.sa_mask);
				(void)sigaction(SIGALRM, &sigact, NULL);
			}
			if ((State == PM_DISABLED)||(pmtab->p_flags & X_FLAG)){
				write_prompt(1,pmtab,TRUE,FALSE);
				break;
			}
			if (set_termio(0,speedef->g_fflags,auto_speed,
				FALSE,CANON)==-1) {
				log(MM_HALT, badftermio);
				exit(1);
			}
			invoke_service(pmtab);
			exit(1);	/*NOTREACHED*/

		case BADSPEED:
			/* wrong speed! try next speed in the list. */
			speedef = next_speed(speedef);
#ifdef	DEBUG
			debug("BADSPEED: setup next speed");
#endif
			if (speedef->g_autobaud & A_FLAG) {
				if (auto_termio(0) == -1) {
					exit(1);
				}
				auto_speed = do_autobaud(pmtab,speedef);
			}
			else {
				auto_speed = NULL;
				/*
				 * this reset may fail if the speed is not 
				 * supported by the system
				 * we just cycle through it to the next one
				 */
				if (set_termio(0,speedef->g_iflags,NULL,
						FALSE,CANON) != 0) {
					log(MM_WARNING, ":607:Speed of <%s> may be not supported by the system", speedef->g_id);
				}
			}
			write_prompt(1,pmtab,TRUE,TRUE);
			break;

		case NONAME:
#ifdef	DEBUG
			debug("got NONAME");
#endif	
			write_prompt(1,pmtab,FALSE,FALSE);
			break;

		}  /* end switch */

		if (first)
			first = FALSE;
		if(pmtab->p_timeout) {
			sigact.sa_flags = SA_RESETHAND | SA_NODEFER;
			sigact.sa_handler = timedout;
			(void)sigemptyset(&sigact.sa_mask);
			(void)sigaction(SIGALRM, &sigact, NULL);
			(void)alarm((unsigned)pmtab->p_timeout);
		}
	} /* end for loop */
}

                                                                                /*
 * Procedure:	  openline
 *
 * Restrictions:
                 open(2): none
		 strerror: none
		 read(2): none
		 fchown(2): none
		 chown(2): none
		 fchmod(2): none
		 chmod(2): none
*/


static void
openline(pmtab)
struct	pmtab 	*pmtab;
{
	char	 buffer[5];
	int	 line_count;
	int	 tmpfd;

#ifdef	DEBUG
	debug("in openline");
#endif
	if (pmtab->p_status != GETTY) {
		(void)close(0);
		/* open should return fd 0, if not, then close it */
		if (open(pmtab->p_device, O_RDWR) != 0) {
			log(MM_HALT, badopen, pmtab->p_device, strerror(errno));
			exit(1);
		}
	}
	(void)close(1);
	(void)close(2);
	(void)dup(0);
	(void)dup(0);

	/*
	 * Open real/physical tty device without "O_NONBLOCK" to wait for
	 * carrier (if not already present) before proceeding.  Opening the
	 * pseudo device, the TP data channel, without O_NONBLOCK will not
	 * block if carrier is not present on the real/physical tty device
	 * since the TP driver has no concept of carrier.  The real/physcial
	 * tty device may not have carrier present, if the SAK is defined
	 * to be NONE, and the C_FLAG is set or a timeout value is set. 
	 */
	if ((tmpfd = open(pmtab->p_realdevice, O_RDWR)) == -1) {
		log(MM_HALT, badopen, pmtab->p_realdevice, strerror(errno));
		exit(1);
	}
	(void)close(tmpfd);

	if (pmtab->p_ttyflags & R_FLAG) { /* wait_read is needed */
		/* Set on privileges for read(2)'s below. */
		if (pmtab->p_count) { 
			/* 
			 * - wait for "p_count" lines 
			 * - datakit switch does not
			 *   know you are a host or a terminal
			 * - so it send you several lines of msg 
			 * - we need to swallow that msg
			 * - we assume the baud rate is correct
			 * - if it is not, '\n' will not look like '\n'
 			 *   and we will wait forever here
			 */
			for (line_count=0;line_count < pmtab->p_count;) {
				if ( read(0, buffer, 1) < 0
				     || *buffer == '\0'
				     || *buffer == '\004') {
					(void)close(0);
					exit(0);
				}
				if (*buffer == '\n')
					line_count++;
			}
		}
		else { /* wait for 1 char */
			/*
			 * NOTE: Cu on a direct line when ~. is encountered will
			 * send EOTs to the other side.  EOT=\004
			 */
			if ( read(0, buffer, 1) < 0
			  || *buffer == '\004') {
				(void)close(0);
				exit(0);
			}
		}
		if (!(pmtab->p_ttyflags & A_FLAG)) { /* autobaud not enabled */
			if (turnon_canon(0) == -1) {
				log(MM_HALT, ":608:openline: turnon_canon() failed");
				exit(1);
			}
		}
	}
	if (pmtab->p_ttyflags & B_FLAG) { /* port is bi-directional */
		/* change ownership back to root */
		(void)fchown(0, ROOTUID, Tty_gid);
		(void)chown(pmtab->p_realdevice, ROOTUID, Tty_gid);
		(void)fchmod(0, 0620);
		(void)chmod(pmtab->p_realdevice, 0620);
	}
	return;
}

/*
 * Procedure:	  write_prompt
 *
 * Restrictions:
		 write(2): none
*/

/*
 *	write_prompt	- write the msg to fd
 *			- if flush is set, flush input queue
 *			- if clear is set, write a new line 
 */
void
write_prompt(fd,pmtab,flush,clear)
int	fd;
struct	pmtab	*pmtab;
int	flush, clear;
{

#ifdef DEBUG
	debug("in write_prompt");
#endif
	if (flush)
		flush_input(fd);
	if (clear) {
		(void)write(fd,"\r\n",2);
	}
#ifdef SYS_NAME
	sys_name(fd);
#endif
	/* Print prompt/disable message. */
	if ((State == PM_DISABLED)||(pmtab->p_flags & X_FLAG))
		(void)write(fd, pmtab->p_dmsg, (unsigned)strlen(pmtab->p_dmsg));
	else
		(void)write(fd, pmtab->p_prompt,
			(unsigned)strlen(pmtab->p_prompt));
}

/*
 *	timedout	- input period timed out
 */
void
timedout()
{
	exit(1);
}

#ifndef SNI
#ifdef SYS_NAME
/*
 * Procedure:	  sys_name
 *
 * Restrictions:
		 sprintf: none
		 write(2): none
		 fopen: none
		 fgets: none
		 fclose: none
fclose: write
*/

/*
 * void sys_name() - generate a msg with system id
 *		   - print out /etc/issue file if it exists
 */
void
sys_name(fd)
int	fd;
{
	char	*ptr, buffer[BUFSIZ];
	struct	utsname utsname;
	FILE	*fp;

	if (uname(&utsname) != FAILURE) {
		(void)sprintf(buffer,"\n%s%.9s%s\r\n",
	 		gettxt(":1133", "The system's name is "), 
			utsname.nodename, ".");
		(void) write(fd,buffer,strlen(buffer));
	}

	if ((fp = fopen(ISSUEFILE,"r")) != NULL) {
		while ((ptr = fgets(buffer,sizeof(buffer),fp)) != NULL) {
			(void)write(fd,ptr,strlen(ptr));
		}
		(void)fclose(fp);
	}
}
#endif /* SYS_NAME */
#endif /* ! SNI */


/*
 *	do_autobaud	- do autobaud
 *			- if it succeed, set the new speed and return
 *			- if it failed, it will get the nextlabel
 *			- if next entry is also autobaud,
 *			  it will loop back to do autobaud again
 *			- otherwise, it will set new termio and return
 */
static	char	*
do_autobaud(pmtab,speedef)
struct	pmtab	*pmtab;
struct	Gdef	*speedef;
{
	int	done = FALSE;
	char	*auto_speed;
#ifdef	DEBUG
	debug("in do_autobaud");
#endif
	while (!done) {
		if ((auto_speed = autobaud(0,pmtab->p_timeout)) == NULL) {
			speedef = next_speed(speedef);
			if (speedef->g_autobaud & A_FLAG) {
				continue;
			}
			else {
				if (set_termio(0,speedef->g_iflags,NULL,
						TRUE,CANON) != 0) {
					exit(1);
				}
				done = TRUE;
			}
		}
		else {
			if (set_termio(0,speedef->g_iflags,auto_speed,
					TRUE,CANON) != 0) {
				exit(1);
			}
			done = TRUE;
		}
	}
#ifdef	DEBUG
	debug("autobaud done");
#endif
	return(auto_speed);
}

/*
 * 	next_speed(speedef) 
 *	- find the next entry according to nextlabel. If "nextlabel"
 *	  is not valid, go back to the old ttylabel.
 */

static	struct	Gdef *
next_speed(speedef)
struct	Gdef *speedef;
{
	struct	Gdef *sp;

	if (strcmp(speedef->g_nextid,speedef->g_id) == 0) 
		return(speedef);
	if ((sp = find_def(speedef->g_nextid)) == NULL) {
		log(MM_ERROR, ":609:%s's next speed-label (%s) is bad.",
			speedef->g_id, speedef->g_nextid);

		/* go back to the original entry. */
		if((sp = find_def(speedef->g_id)) == NULL) {
			 /* if failed, complain and quit. */
			log(MM_HALT, ":610:Unable to find (%s) again", speedef->g_id);
			exit(1);
		}
	}
	return(sp);
}

/*
 * Procedure:     inform_parent
 *
 * Restrictions:
                 write(2): none
*/

/*
 * inform_parent()	- inform ttymon that tmchild is going to exec service
 */
static	void
inform_parent(fd)
int	fd;
{
	pid_t	pid;

	pid = getpid();
	(void)write(fd, &pid, sizeof(pid));
}

static	char	 pbuf[BUFSIZ];	/* static buf for TTYPROMPT 	*/
static	char	 hbuf[BUFSIZ];	/* static buf for HOME 		*/

/*
 * Procedure:     invoke_service
 *
 * Restrictions:
                 doconfig: none
                 open(2): none
								 pfmt: none
                 fclose: none
                 setrlimit(2): none
                 strerror: none
                 sprintf: none
                 putenv: none
								 gettxt: none
                 invoke: none
                 set_id: none
                 execve(2): P_MACREAD
*/

/*
 * void invoke_service	- invoke the service
 */

static	void
invoke_service(pmtab)
struct	pmtab	*pmtab;
{
	char	 *argvp[MAXARGS];		/* service cmd args */
	int	 cnt = 0;			/* arg counter */
	int	 i, fd;
	struct	 sigaction	sigact;
	extern	 int	doconfig();
	extern	 struct rlimit	Rlimit;
	static const char
		badputenv[] = ":611:Cannot expand service <%s> environment",
    		nocontty1[] = ":612:Cannot allocate controlling tty on \"%s\",\n",
    		nocontty2[] = ":613:           there may be another session active on this port.\n";
	    		

#ifdef 	DEBUG
	debug("in invoke_service");
#endif

	if (tcgetsid(0) != getsid(getpid())) {
	    if ((fd = open(CONSOLE, O_WRONLY|O_NOCTTY)) != -1) {
	    	FILE *sfd = fdopen(fd, "w");
	    	if (sfd){
	    		pfmt(sfd, MM_WARNING, nocontty1, pmtab->p_device);
	    		pfmt(sfd, MM_NOSTD, nocontty2);
	    		fclose(sfd);
	    	} else
	    		close(fd);
	    }
	    if (strcmp("/dev/console", pmtab->p_device) != 0) {
		/* if not on console, write to stderr to warn the user also */
		(void)pfmt(stderr, MM_WARNING, nocontty1, pmtab->p_device);
		(void)pfmt(stderr, MM_NOSTD, nocontty2);
	    }
	}

	if (pmtab->p_status != GETTY) {
		inform_parent(PCpipe[1]);
		sigact.sa_flags = 0;
		sigact.sa_handler = SIG_DFL;
		(void)sigemptyset(&sigact.sa_mask);
		(void)sigaction(SIGPOLL, &sigact, NULL);
	}

	if (pmtab->p_flags & U_FLAG) {
		if (account(pmtab->p_device) != 0) {
			log(MM_HALT, ":614:invoke_service: account() failed");
			exit(1);
		}
	}

	/* parse command line */
	mkargv(pmtab->p_server,&argvp[0],&cnt,MAXARGS-1);

	if (!(pmtab->p_ttyflags & C_FLAG)) {
		(void) sprintf(pbuf, "TTYPROMPT=%s", pmtab->p_prompt);
		if (putenv(pbuf)) {
			log(MM_HALT, badputenv,	argvp[0]);
			exit(1);
		}
	}
	if (pmtab->p_status != GETTY) {
		if ((pmtab->p_dir != NULL) && (*(pmtab->p_dir) != '\0')) {
			(void) sprintf(hbuf, "HOME=%s", pmtab->p_dir);
			if (putenv(hbuf)) {
				log(MM_HALT, badputenv, argvp[0]);
				exit(1);
			}
		}
#ifdef	DEBUG
		debug("about to run config script");
#endif
		if ((i = doconfig(0, pmtab->p_tag, 0)) != 0) {
			if (i < 0) {
				log(MM_HALT, efailed, "doconfig()",
					gettxt(":615", "system error"));
			}
			else {
				log(MM_HALT, ":616:doconfig() failed on line %d of script %s",
				i,pmtab->p_tag);
			}
			exit(1);
		}
	}
        if ((pmtab->p_iascheme != NULL) && (*(pmtab->p_iascheme) != '\0')) {
		/* invoke identification & authentication scheme */
		if ((i = invoke(0, pmtab->p_iascheme)) != 0) {
			/*
			 * Assume invoke will return -1 if it cannot parse or
			 * invoke the scheme, and return something else 
			 * if the scheme fails after being exec'ed.
			 * The failed scheme should output the error message.
			 */
			if (i == -1) {
				log(MM_ERROR, 
					":847:Cannot invoke iascheme <%s>: %s",
					pmtab->p_iascheme, strerror(errno));
			}
                        exit(1);
		}
	}
        if ((pmtab->p_identity != NULL) && (*(pmtab->p_identity) != '\0')) {
                if (set_id(pmtab->p_identity) != 0) {
                	log(MM_ERROR, badset_id, pmtab->p_identity, strerror(errno));
                        exit(1);
		}
	}
	else {
                if (set_id(NULL) != 0) {
                	log(MM_ERROR, badset_id, NULL, strerror(errno));
                        exit(1);
		}
	}

	if (pmtab->p_status != GETTY) {
		sigact.sa_flags = 0;
		sigact.sa_handler = SIG_DFL;
		(void)sigemptyset(&sigact.sa_mask);
		(void)sigaction(SIGINT, &sigact, NULL);
		if (setrlimit(RLIMIT_NOFILE, &Rlimit) == -1) {
			log(MM_HALT,  efailed, "setrlimit()", strerror(errno));
			exit(1);
		}
		/* invoke the service */
		log(MM_INFO, ":848:Starting service (%s) on tp device %s muxed under tty device %s",
		 argvp[0], pmtab->p_device, pmtab->p_realdevice);
	}

	/* restore signal handlers and mask */
	(void)sigaction(SIGINT, &Sigint, NULL);
	(void)sigaction(SIGALRM, &Sigalrm, NULL);
	(void)sigaction(SIGPOLL, &Sigpoll, NULL);
	(void)sigaction(SIGCLD, &Sigcld, NULL);
	(void)sigaction(SIGTERM, &Sigterm, NULL);
#ifdef	DEBUG
	(void)sigaction(SIGUSR1, &Sigusr1, NULL);
	(void)sigaction(SIGUSR2, &Sigusr2, NULL);
#endif
	(void)sigprocmask(SIG_SETMASK, &Origmask, NULL);
	(void) procprivl(CLRPRV, MACREAD_W, 0);
	(void) execve(argvp[0], argvp, environ);

	/* exec returns only on failure! */
	(void) procprivl(SETPRV, MACREAD_W, 0);
	log(MM_HALT, ":621:tmchild: Cannot execute %s: %s", argvp[0], strerror(errno));
	exit(1);
}

/*
 * Procedure:     check_hup
 *
 * Restrictions:
                 strerror: none
*/

/*
 *	check_hup(fd)	- do a poll on fd to check if it is in hangup state
 *			- return 1 if hangup, otherwise return 0
 */

static	int
check_hup(fd)
int	fd;
{
	int	ret;
	struct	pollfd	pfd[1];

	pfd[0].fd = fd;
	pfd[0].events = POLLHUP;
	for (;;) {
		ret = poll(pfd, 1, 0);
		if (ret < 0) {
			if (errno == EINTR) 
				continue;
			log(MM_HALT, badpoll, "check_hup", strerror(errno));
			exit(1);
		}
		else if (ret > 0) {
			if (pfd[0].revents & POLLHUP) {
				return(1);
			}
		}
		return(0);
	}
}

/*
 * sigpoll()	- SIGPOLL handle for tmchild
 *		- when SIGPOLL is received by tmchild,
 *		  the pipe between ttymon and tmchild is broken.
 *		  Something must happen to ttymon.
 */
void
sigpoll()
{
#ifdef	DEBUG
	debug("tmchild got SIGPOLL, exiting");
#endif
	exit(1);
}
