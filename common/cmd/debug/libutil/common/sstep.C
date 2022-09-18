#ident	"@(#)debugger:libutil/common/sstep.C	1.1"

#include "utility.h"
#include "LWP.h"
#include "Proglist.h"
#include "Interface.h"
#include "Parser.h"
#include "global.h"
#include "List.h"

int
single_step( Proclist * procl, int clearsig, int cnt, int talk, 
	int level, int where, int wait )
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
		if (!lwp->state_check(E_RUNNING|E_DEAD|E_CORE))
		{
			ret = 0;
			continue;
		}
		if (level == STEP_STMT)
		{
			if (!lwp->stmt_step( where, clearsig, 
				0, cnt, talk ))
			{
				ret = 0;
				continue;
			}
		}
		else
		{
			if (!lwp->instr_step( where, clearsig, 
				0, cnt, talk ))
			{
				ret = 0;
				continue;
			}
		}
		if (wait == WAIT)
		{
			lwp->set_wait();
			waitlist.add(lwp);
		}
	}
	while(!single && ((lwp = list++->p_lwp) != 0));
	if ((wait == WAIT) && !waitlist.isempty())
		wait_process();
	return ret;
}
