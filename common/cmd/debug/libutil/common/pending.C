#ident	"@(#)debugger:libutil/common/pending.C	1.2"
#include "utility.h"
#include "LWP.h"
#include "Proglist.h"
#include "Parser.h"
#include "Interface.h"
#include <signal.h>

int
pending_sigs(Proclist *procl)
{
	int	single = 1;
	int	ret = 1;
	LWP	*lwp;
	plist	*list;

	if (procl)
	{
		single = 0;
		list = proglist.proc_list(procl);
		lwp = list++->p_lwp;
	}
	else
	{
		lwp = proglist.current_lwp();
	}
	if (!lwp)
	{
		printe(ERR_no_proc, E_ERROR);
		return 0;
	}
	do
	{
		int mask;
		
		if (!lwp->state_check(E_RUNNING|E_DEAD|E_CORE))
		{
			ret = 0;
			continue;
		}

		mask = lwp->pending_sigs();

		for (int i = 0; i < NSIG-1; i++)
		{
			if (mask & 0x1)
			{
				printm(MSG_signame, i+1, signame(i+1));
			}
			mask >>= 1;
		}
	}
	while(!single && ((lwp = list++->p_lwp) != 0));
	return ret;
}
