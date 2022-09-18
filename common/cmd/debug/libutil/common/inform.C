#ident	"@(#)debugger:libutil/common/inform.C	1.4"

#include "Manager.h"
#include "utility.h"
#include "global.h"
#include "LWP.h"
#include "Proglist.h"
#include "Interface.h"
#include "Procctl.h"
#include "List.h"
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <termio.h>

List		waitlist;

// Walk the list of live processes and process any that need attention,
// i.e. that have stopped since we last dealt with them
// This is the signal handler for the signals coming from the
// follower processes.

int
inform_processes(int sig)
{
	termio		savebuf;
	plist		*list;
	int		clobberedcurrent = 0;
	sigset_t	original_sigset;

	// do the usual interrupt processing
	if(sig) 
	{
		if (sig != -1)
			interrupt |= sigbit(sig);
		
		sigprocmask(SIG_SETMASK, &sset_PI, &original_sigset);

		// Arrange so that if user types INTR key the current output
		// line will be flushed (with trailing ^C for free!) so that
		// they can see it and so that we know the screen state.

		if (get_ui_type() != ui_gui)
		{
			termio tmpbuf;
			if (ioctl(0, TCGETA, &savebuf)) 
			{
				tmpbuf = savebuf;
				tmpbuf.c_lflag |= NOFLSH;
				ioctl(0, TCSETAW, &tmpbuf);
			}
		}
	}

	PrintaxSpeakCount = 0;
	list = proglist.all_live();
	for (LWP *l = list++->p_lwp; l; l = list++->p_lwp)
	{
		int		 what, why;
		Procstat	pstat;

		message_manager->reset_context(l);
		if (!l->check() || (l->get_state() == es_dead))
			continue;
		if ((pstat = l->proc_ctl()->status(what, why))
			== p_dead)
		{
			l->markdead();
			if (l->waiting())
				waitlist.remove(l);
			(void)l->make_proto(P_KILL);
			printm(MSG_proc_exit, l->lwp_name());
			if (l == proglist.current_lwp())
				clobberedcurrent = 1;
		}
		else if (pstat == p_stopped)
		{
			l->clear_check();
			l->inform(what, why);
			if (l->waiting())
			{
				if (l->get_state() != es_stepping &&
					l->get_state() != es_running)
				{
					l->clear_wait();
					waitlist.remove(l);
				}
			}
			// process any held interrupts, but then
			// keep holding them
			interrupt &= ~sigbit(SIGINT);
			sigrelse(SIGINT);
			sighold(SIGINT);
			interrupt &= ~sigbit(SIGINT);
		}
	}
	message_manager->reset_context(0);
	if (clobberedcurrent) 
	{
		proglist.reset_current(1);
	}
	if (sig) 
	{
		if (get_ui_type() != ui_gui)
		{
			// Turn off output INTR-char buffering magic.
			ioctl(0, TCSETAW, &savebuf);
		}

		sigprocmask(SIG_UNBLOCK, &original_sigset, 0);
	}
	return 1;
}
