#ident	"@(#)debugger:libutil/common/regs.C	1.1"
#include "Interface.h"
#include "utility.h"
#include "LWP.h"
#include "Proglist.h"
#include "Parser.h"
#include "global.h"
#include <signal.h>

int
printregs( Proclist * procl )
{
	int 	single = 1;
	LWP	*lwp;
	plist	*list;
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
	sigrelse(SIGINT);
	do
	{
		if (!lwp->state_check(E_RUNNING|E_DEAD))
		{
			ret = 0;
			continue;
		}
		printm(MSG_reg_header, lwp->lwp_name(), 
			lwp->prog_name());
		if (!lwp->display_regs(lwp->curframe()))
			ret = 0;
	}
	while(!single && ((lwp = list++->p_lwp) != 0) &&
		!(interrupt & sigbit(SIGINT)));

	sighold(SIGINT);
	return ret;
}
