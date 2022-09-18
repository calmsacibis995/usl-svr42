#ident	"@(#)debugger:libedit/common/debug_read.C	1.3.1.1"

// This is the top level read routine which is also
// responsible for fielding events from subject processes.
// It waits on a single character read while fielding SIGUSR1.
// If it catches SIG_INFORM, it goes off and does any appropriate
// actions and reprompts if necessary.
//

#include "utility.h"
#include "Interface.h"
#include "Input.h"
#include "global.h"
#include "Machine.h"
#include "sh_config.h"
#include "edit.h"
// shconfig.h and edit.h must come last since sh_config defines SYSCAL_L and
// edit "defines" DELETE, and they are also enums in Parser.h
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

extern void vi_refresh();
extern void emacs_refresh();
extern void prompt();

int
debug_read(int fd, void *buf, unsigned int nchar)
{
	int i;

	PrintaxGenNL = 1;	// arrange for a newline to be
				// generated before output.


	if (get_ui_type() == ui_gui)
		// don't allow SIGINT in gui
		sigprocmask(SIG_UNBLOCK, &sset_UP, 0);
	else
		sigprocmask(SIG_UNBLOCK, &sset_UPI, 0);
	PrintaxSpeakCount = 0;

	for(;;) 
	{
		interrupt &= ~(sigbit(SIG_INFORM) | sigbit(SIGPOLL));
		i = read(fd, buf, nchar);

		if (i != -1 || errno != EINTR ||
		    !interrupt & (sigbit(SIG_INFORM)|sigbit(SIGPOLL)))
			break;
		if (interrupt & sigbit(SIGINT))
		{
			interrupt &= ~sigbit(SIGINT);
			i = 0;
			break;
		}
		if (PrintaxSpeakCount) 
		{
			prompt();
			PrintaxSpeakCount = 0;

		}

	}
	if (get_ui_type() == ui_gui)
	{
		// cancel any spurious interrupts
		sigrelse(SIGINT);
		if (interrupt & sigbit(SIGINT))
			interrupt &= ~sigbit(SIGINT);
	}
	sigprocmask(SIG_SETMASK, &sset_UPI, 0);
	PrintaxGenNL = 0;
	return i;
}
