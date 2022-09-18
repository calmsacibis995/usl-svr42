#ident	"@(#)debugger:libutil/common/wait.C	1.3.1.1"

#include "utility.h"
#include "LWP.h"
#include "global.h"
#include "List.h"
#include "Interface.h"
#include "Proglist.h"
#include <sys/types.h>
#include <signal.h>
#include <termio.h>
#include <unistd.h>

// wait for current action to complete

static void stop_waiting_procs();
extern void debugtty();
extern void restore_tty();

void
wait_process()
{
	static int		waiting;
	static struct termio	ttybuf;
	static int		ttysaved;

	// we could enter wait_process while already waiting
	// if an event fires and an associated command does
	// a run or step
	if (waiting)
		return;

	waiting = 1;

	// restore users tty
	if (ttysaved)
		ioctl(0, TCSETAW, &ttybuf);
	else
		restore_tty();	// original tty settings
	
	while(!waitlist.isempty())
	{
		// block only POLL in here
		sigsuspend(&sset_P);
		proglist.reset_lists();
		if (interrupt & sigbit(SIGINT))
		{
			interrupt &= ~sigbit(SIGINT);
			stop_waiting_procs();
			break;
		}
	}
	if (get_ui_type() != ui_gui)
	{
		// save user's tty and restore debugger's
		if (ioctl(0, TCGETA, &ttybuf) == 0)
			ttysaved = 1;
		else
			ttysaved = 0;
		debugtty();
	}

	PrintaxSpeakCount = 0;
	waiting = 0;
	return;
}

static void
stop_waiting_procs()
{
	LWP	*p = (LWP *)(waitlist.first());

	// allow stop directives to be interrupted in case
	// the stop never returns due to some deadlock
	sigrelse(SIGINT);
	for(; p; p = (LWP *)(waitlist.next()))
	{
		p->stop();
	}
	sighold(SIGINT);
	waitlist.clear();
}
