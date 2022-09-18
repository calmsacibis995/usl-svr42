/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:debug.d/common/sig_handle.c	1.4"

/* signal handling function
 * written in C to get around cfront problems with
 * signal structure definitions
 */

#include "Machine.h"
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "handlers.h"

sigset_t	sset_UPI, sset_P, sset_PI, sset_UP;

void
signal_setup()
{
	struct sigaction	pact;
	struct sigaction	sact;

	sigignore(SIGALRM);
	sigignore(SIGQUIT);
	sigignore(SIGCLD);	/* let children exit in peace. */

	/*
	 * SIG_INFORM and SIGPOLL are used internally in the read routine,
	 * but to protect ourselves from unnecessary grief we put
	 * SIG_INFORM and SIGPOLL on hold except in those regions of
	 * the code where we are prepared to field them. SIGINT is
	 * a great deal trickier.
	 *
	 * SIG_INFORM is a macro that allows the informing signal to be
	 * implemented differently on different systems.
	 * Care must be taken if SIGPOLL is used as SIG_INFORM, since
	 * POLL is already used to watch I/O
	 */

	sigemptyset(&sset_P);
	sigemptyset(&sset_UPI);
	sigemptyset(&sset_PI);
	sigemptyset(&sset_UP);
	sigaddset(&sset_UPI,SIG_INFORM);
	sigaddset(&sset_UPI,SIGPOLL);
	sigaddset(&sset_UPI,SIGINT); 
	sigaddset(&sset_PI,SIGINT); 
	sigaddset(&sset_PI,SIG_INFORM);
	sigaddset(&sset_P,SIGPOLL);
	sigaddset(&sset_UP,SIG_INFORM);
	sigaddset(&sset_UP,SIGPOLL);

	sigprocmask(SIG_SETMASK, &sset_UPI, 0);

	sigset(SIGINT, fault_handler());
	sigset(SIGPIPE, fault_handler());

	/* set up handlers with SIGINFO to assure
	 * reliable queuing of signals, even though
	 * we ignore siginfo structure
	 */
	pact.sa_handler = (void (*)())poll_handler();
	sigemptyset(&pact.sa_mask);
	pact.sa_flags = SA_SIGINFO;
	sact.sa_handler = (void (*)())inform_handler();
	sigemptyset(&sact.sa_mask);
	sact.sa_flags = SA_SIGINFO;
	sigaction(SIG_INFORM, &sact, 0);
	sigaction(SIGPOLL, &pact, 0);

	/* Make sure debug does not core dump on
	 * top of a user's core file
	 */
	sigset(SIGHUP, internal_error_handler());
	sigset(SIGILL, internal_error_handler());
	sigset(SIGTRAP, internal_error_handler());
	sigset(SIGEMT, internal_error_handler());
	sigset(SIGFPE, internal_error_handler());
	sigset(SIGBUS, internal_error_handler());
	sigset(SIGSEGV, internal_error_handler());
	sigset(SIGSYS, internal_error_handler());
	sigset(SIGTERM, internal_error_handler());

	/* workaround for bug in libedit with sh/csh */
	sigset(SIGTSTP, suspend_handler());
}

/* handle fatal errors from the floating-point emulation
 * package; we cannot call internal_error() directly, since
 * the fpemu package is in C and internal_error() is in C++;
 * this routine will get us there through the signal mechanism
 */
void
fpemu_error()
{
	/* send ourselves floating point signal */
	kill(getpid(), SIGFPE);
}
