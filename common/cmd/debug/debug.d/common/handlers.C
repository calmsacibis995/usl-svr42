/*
 * $Copyright:
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ident	"@(#)debugger:debug.d/common/handlers.C	1.5"

#include "Machine.h"
#include "global.h"
#include <sys/types.h>
#include <signal.h>
#include <termio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "handlers.h"
#include "Interface.h"

extern int		FieldProcessIO(int);
extern int		inform_processes(int);
static void		fault(int);
static void		internal_error(int);
static void		handle_suspend(int);

SIG_TYP poll_handler()
{
	return (SIG_TYP) FieldProcessIO;
}

SIG_TYP inform_handler()
{
	return (SIG_TYP) inform_processes;
}

SIG_TYP fault_handler()
{
	return (SIG_TYP) fault;
}

SIG_TYP internal_error_handler()
{
	return (SIG_TYP) internal_error;
}

SIG_TYP suspend_handler()
{
	return (SIG_TYP) handle_suspend;
}

int			interrupt;

void
fault(int sig)
{
	interrupt |= sigbit(sig);
}


extern char	*msg_internal_error;
extern void	stop_interface();
extern void	restore_tty();
extern void	destroy_all();

#ifdef __cplusplus
static void (*registered_handler) (int) = 0;
#else
typedef void (*HANDLER)(...) ;
static HANDLER registered_handler = 0;
#endif

/* signal handler - we avoid all core dumps */
void
internal_error(int sig)
{

	if (registered_handler) registered_handler(sig);
	/* Doesn't return if it is just a user error */

	/* restore tty sanity */
	int flags = fcntl(0, F_GETFL, 0);
	flags &= ~O_NDELAY;
	fcntl(0, F_SETFL, flags);
	if (get_ui_type() != ui_gui)
		restore_tty();
	// restore original working dir
	if (original_dir)
		chdir(original_dir);
	destroy_all();
	stop_interface();
	write(2, msg_internal_error, strlen(msg_internal_error));
	exit(sig);
}

#ifdef __cplusplus
void add_error_handler(void (*error_handler)(int))
#else
void add_error_handler(SIG_TYP error_handler) 
#endif
{
	if (error_handler && registered_handler)
		printe(ERR_internal, E_ERROR, 
			"add_error_handler", __LINE__);
#ifdef __cplusplus
	registered_handler = error_handler;
#else
	registered_handler = (HANDLER)error_handler;
#endif
}

#ifdef __cplusplus
extern "C" {
#endif
void tty_cooked(int);
#ifdef __cplusplus
}
#endif

void
handle_suspend(int sig)
{
	// workaround for problem in libedit with sh and csh
	tty_cooked(2);  // This is what edit.c does if it runs into trouble
	sigset(SIGTSTP, SIG_DFL);
	kill(0, sig);
	sigset(SIGTSTP, suspend_handler());
	// BUG: the terminal is left in cooked mode when resumed;
	// it really should be put back to what it was.  But what
	// was it? If the debugger was:
	//   running a program in the foreground:
	//     the program should handle it; the debugger left the
	//     terminal cooked.  No problem.
	//   taking command input:
	//     this is the case that this routine handles, except that
	//     it doesn't restore it on resume (oops).  It will be restored
	//     once the debugger reprompts (acceptable for now).
	//   producing command output:
	//     the terminal was cooked when the suspend happened, and
	//     will be left that way.  No problem.
}
