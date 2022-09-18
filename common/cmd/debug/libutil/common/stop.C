#ident	"@(#)debugger:libutil/common/stop.C	1.1"

#include "utility.h"
#include "LWP.h"
#include "Proglist.h"
#include "Interface.h"
#include "Parser.h"

int
stop_process(Proclist *procl)
{
	LWP	*lwp;
	plist	*list;
	int	single = 1;
	int	ret = 1;

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
		if (!lwp->state_check(E_CORE|E_DEAD))
		{
			ret = 0;
			continue;
		}
		if (!lwp->stop())
			ret = 0;
	}
	while(!single && ((lwp = list++->p_lwp) != 0));
	return ret;
}
