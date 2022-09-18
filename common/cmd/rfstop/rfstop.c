/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)rfstop:rfstop.c	1.11.9.3"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/rfstop/rfstop.c,v 1.1 91/02/28 19:28:45 ccs Exp $"
#include <stdio.h>
#include <sys/types.h>
#include <nserve.h>
#include <time.h>
#include <sys/rf_sys.h>
#include <errno.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define ERROR(str)	fprintf(stderr, "%s: %s\n", argv[0], str)

extern int errno;

void	sigalrm();

main( argc, argv )
int   argc;
char *argv[];
{
	char cmd[512];
	int	Lockfd;
	
	if (argc != 1) {
		ERROR("extra arguments given");
		ERROR("usage: rfstop");
		exit(1);
	}

#ifdef OLDSEC
	if (geteuid() != 0) {
		ERROR("must be super-user");
		exit(1);
	}
#endif /*OLDSEC*/

	sigset(SIGHUP,  SIG_IGN);
	sigset(SIGINT,  SIG_IGN);
	sigset(SIGQUIT, SIG_IGN);

	if (setrfslck() != 0)
		exit(1);
		
	/*
	 *	Stop all kernel functions of RFS.
	 */

	if (rfsys(RF_STOP) < 0) {
		if (errno == EBUSY)
			ERROR("remote resources currently mounted");
		else if (errno == ESRMNT)
			ERROR("remote clients are using local resources");
		else if (errno == EADV)
			ERROR("resources are still advertised");
		else if (errno == ENONET) {
			ERROR("RFS is not running");
			exit(1);
		} else
			perror(argv[0]);
		ERROR("cannot stop RFS");
		exit(1);
	}

	/*
	 *	Execute the shell script to stop the name server
	 *	process and have the name server relinquish primary
	 *	responsibilities, if necessary.
	 */

	if (system("rfadmin -p >/dev/null 2>&1") == 0x200)
		ERROR("warning: no secondary name servers active");

	sprintf(cmd, "kill `cat %s 2>/dev/null` 2>/dev/null", NSPID);

	if (system(cmd) != 0) {
		ERROR("error in killing name server");
		rfsys(RF_LASTUMSG);
		exit(1);
	}

	/*
	 *	Kill the user-level daemon that accepts masseges from
	 *	other systems by sending a "last message' signal.
	 */

	rfsys(RF_LASTUMSG);
	
	/* Finally, make sure that nserve and friends have exitted 
	   by waiting till we can acquire the nserve lock */

	if ((Lockfd = open(NSPID, O_RDWR)) < 0) {
		/* Nonexistence is OK. 
			Nserve might have died & cleaned up */
		if (errno != ENOENT) {
			fprintf(stderr,"%s: cannot open: %s: %s\n",
				argv[0], NSPID, strerror(errno));
			exit(1);
		}
		exit(0);
	} else {
		/*
		 * Lock file exists; nserve might be running
		 * Try to lock it ourself. Timeout 'can never occur'
		 *  because of setrfslock() ... short of bug in rfstart
		 *  and/or nserve. 
		 */
		(void) sigset(SIGALRM, sigalrm);
		(void) alarm(60);
		if (lockf(Lockfd,F_LOCK, 0L) == 0) {
			/* Got lock. Nserve is gone. 
			   Close file (releases lock) and we're done */
			(void) close(Lockfd);
			/* remove the NSPID file - 'used to be removed
			   by nserve but took too long' */
			(void) unlink(NSPID); 
			exit(0);
		} else {
			/* This 'cannot happen' */
			fprintf(stderr, "%s: cannot lock: %s: %s\n",
				argv[0], NSPID, strerror(errno));
			exit (1);
		}
	}
}
	
/* Dummy signal handler. Doesn't need to do anything, just exist */
void	sigalrm(sig)
/*ARGSUSED*/
	int sig;
{
}
