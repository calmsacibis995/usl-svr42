#ident	"@(#)debugger:libutil/common/grab_proc.C	1.3"

#include "Procctl.h"
#include "utility.h"
#include "LWP.h"
#include "Manager.h"
#include "Proglist.h"
#include "Parser.h"
#include "Interface.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

// grab one or more live processes

int
grab_process( Filelist *files, char *loadfile, int follow )
{
	LWP 	*lwp, *first = 0;
	char	*name;
	int	i = 0;
	int	ret = 1;
	int	ignore_fork = 0;
	int	what, why;
	// MORE - need option to ignore threads as well

	if (follow < FOLLOW_PROCS)
		ignore_fork = 1;

	while((name = (*files)[i++]) != 0)
	{
		lwp = new LWP();
		if (!lwp->grab(proglist.next_proc(), name, loadfile, 0, ignore_fork))
		{
			printe(ERR_cant_grab, E_ERROR, name);
			proglist.dec_proc();
			delete lwp;
			ret = 0;
			continue;
		}
		message_manager->reset_context(lwp);
		printm(MSG_grab_proc, lwp->prog_name(), lwp->lwp_name());
		lwp->proc_ctl()->status(what, why);
		lwp->inform(what, why);

		if (!first)
			first = lwp;
	}
	if (first)
		proglist.set_current(first, 0);
	message_manager->reset_context(0);
	return ret;
}
