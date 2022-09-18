#ident	"@(#)debugger:libutil/common/send_sig.C	1.1"
#include "utility.h"
#include "LWP.h"
#include "Proglist.h"
#include "Interface.h"
#include "Parser.h"
#include "Procctl.h"

int
send_signal( Proclist * procl, int signo )
{
	LWP	*lwp;
	plist	*list;
	int	ret = 1;
	int	single = 1;

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
		if (!lwp->proc_ctl()->kill(signo))
			ret = 0;
	}
	while(!single && ((lwp = list++->p_lwp) != 0));
	return ret;
}
