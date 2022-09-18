/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ttymon:common/cmd/ttymon/tmsac.c	1.10.8.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/ttymon/tmsac.c,v 1.1 91/02/28 20:16:29 ccs Exp $"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pfmt.h>
#include "ttymon.h"	
#include "sac.h" 

static	int	sfd;	/* fd for sacpipe */

extern	char	Scratch[];
extern	void	log();
extern	void	logexit();

extern const char badopen[];

/*
 * Procedure:	  openpid
 *
 * Restrictions:
                 open(2): none
             	 lockf: none
            	 strerror: none
                 sprintf: none
                 write(2): none
*/

/*
 *	openpid	- open the pid and put ttymon's pid in it
 *		- put an advisory lock on the file 
 *		- to prevent another instance of ttymon in same directory
 *		- SAC also makes use of the lock
 *		- fd 0 is reserved for pid file
 */
void
openpid()
{
	int lckfd;

	(void)close(0);
	/* open for read first, otherwise, may delete the pid already there*/
	if ((lckfd = open(PIDFILE, O_RDONLY)) != -1) {
	     	if (lockf(lckfd, F_TEST, 0L) == -1) {
			logexit(1,":658:Pid file is locked. ttymon may already be running!");
		}
		(void)close(lckfd);
	}
	if ((lckfd = open(PIDFILE, O_WRONLY|O_CREAT|O_TRUNC, 0644 )) != 0) {
		logexit(1, badopen, PIDFILE, strerror(errno));
	}
	if (lockf(lckfd, F_LOCK, 0L) == -1)  {
		logexit(1, ":659:Lock pid file failed: %s", strerror(errno));
	}

	(void)sprintf(Scratch, "%ld", getpid());
	(void)write(lckfd, Scratch, (unsigned)(strlen(Scratch)+1));
#ifdef	DEBUG
	dlog("fd(pid)\t = %d",lckfd);
#endif
}

/*
 * Procedure:	  openpipes
 *
 * Restrictions:
                 open(2): none
             	 strerror: none
*/

/*
 * openpipes() -- open pmpipe and sacpipe to communicate with SAC
 *	       -- Pfd is a global file descriptors for pmpipe
 *	       -- sfd is a file descriptors for sacpipe
 */

void
openpipes()
{
	extern	int Pfd;

	sfd = open(SACPIPE, O_WRONLY);
	if (sfd < 0) {
		logexit(1, badopen, SACPIPE, strerror(errno));
	}

	Pfd = open(PMPIPE, O_RDWR|O_NONBLOCK); 
	if (Pfd < 0) {
		logexit(1, badopen, PMPIPE, strerror(errno));
	}
#ifdef	DEBUG
	dlog("fd(sacpipe)\t = %d",sfd);
	dlog("fd(pmpipe)\t = %d",Pfd);
#endif
}

/*
 * remove_env(env) - remove an environment variable from the environment
 */
static	void
remove_env(env)
char	*env;
{
	extern	char	**environ;
	char	**p;
	char	**rp = NULL;

	p = environ;
	if (p == NULL)
		return;
	while (*p) {
		if (strncmp(*p, env,strlen(env)) == 0)
			rp = p;
		p++;
	}
	if (rp) {
		*rp = *--p;
		*p = NULL;
	}
}

/*
 * get_environ() -- get env variables PMTAG, ISTATE
 *		 -- set global variables Tag, State
 */

void
get_environ()
{
	char 	*istate;	/* initial state */
	extern 	char State, *Tag;

	if ((Tag = getenv("PMTAG")) == NULL)
		logexit(1, ":660:PMTAG is missing"); 

	if ((istate = getenv("ISTATE")) == NULL) 
		logexit(1, ":661:ISTATE is missing");

	State = (!strcmp(istate, "enabled")) ? PM_ENABLED : PM_DISABLED;

	/*
	 * remove the environment variables so they will not
	 * be passed to the children
	 */
	remove_env("ISTATE");
	remove_env("PMTAG");
}

/*
 * Procedure:	  sacpoll
 *
 * Restrictions:
                 read(2): none
                 strerror: none
                 write(2): none
*/

/*
 * sacpoll	- the event handler when sac event is posted
 */
void
sacpoll()
{
	int 	ret;
	char	oldState;
	struct 	sacmsg sacmsg;
	struct 	pmmsg pmmsg;
	sigset_t	cset;
	sigset_t	tset;
	extern	char	State, *Tag;
	extern	int Pfd;
	extern 	state_change();
	extern 	int Reread_flag;
	extern	void	sigchild();

#ifdef	DEBUG
	debug("in sacpoll");
#endif

	/* we don't want to be interrupted by sigchild now */
	(void)sigprocmask(SIG_SETMASK, NULL, &cset);
	tset = cset;
	(void)sigaddset(&tset, SIGCLD);
	(void)sigprocmask(SIG_SETMASK, &tset, NULL);

	/*
	 *	read sac messages, one at a time until no message
	 *	is left on the pipe.
	 *	the pipe is open with O_NONBLOCK, read will return -1
	 *	and errno = EAGAIN if nothing is on the pipe
	 */
	for (;;) {

		ret = read(Pfd, &sacmsg, sizeof(sacmsg));
		if (ret < 0) {
			switch(errno) {
			case EAGAIN:
				/* no more data on the pipe */
				(void)sigprocmask(SIG_SETMASK, &cset, NULL);
				return;
			case EINTR:
				break;
			default: 
				logexit(1, ":662:Read error on pmpipe: %s",
					strerror(errno));
				break;  /*NOTREACHED*/
			}
		}
		else if (ret == 0) {
			/* no more data on the pipe */
			(void)sigprocmask(SIG_SETMASK, &cset, NULL);
			return;
		}
		else {
			static const char gotmsg[] = ":663:Got %s message";

			pmmsg.pm_size = 0;
			(void) strcpy(pmmsg.pm_tag, Tag);
			pmmsg.pm_maxclass = TM_MAXCLASS;
			pmmsg.pm_type = PM_STATUS;
			switch(sacmsg.sc_type) {
			case SC_STATUS:
				break;
			case SC_ENABLE:
				log(MM_INFO, gotmsg, "SC_ENABLE");
				oldState = State;
				State = PM_ENABLED;
				if (State != oldState) { 
#ifdef	DEBUG
					debug("state changed to ENABLED");
#endif
					state_change();
				}
				break;
			case SC_DISABLE:
				log(MM_INFO, gotmsg, "SC_DISABLE");
				oldState = State;
				State = PM_DISABLED;
				if (State != oldState) { 
#ifdef	DEBUG
					debug("state changed to DISABLED");
#endif
					state_change();
				}
				break;
			case SC_READDB:
				log(MM_INFO, gotmsg, "SC_READDB");
				Reread_flag = 1;
				break;
			default:
				log(MM_INFO, ":664:Got unknown message");
				pmmsg.pm_type = PM_UNKNOWN;
				break;
			} /* end switch */
			pmmsg.pm_state = State;

			while (write(sfd, &pmmsg, sizeof(pmmsg)) != sizeof(pmmsg)) {
				if (errno == EINTR)
					continue;
				log(MM_ERROR, ":665:Sanity response to SAC failed: %s",
					strerror(errno));
				break;
			}
		}
	} /* end for loop */
}
