#ident	"@(#)debugger:libutil/common/print_map.C	1.1"
#include	"utility.h"
#include	"LWP.h"
#include	"Proglist.h"
#include	"Interface.h"
#include 	"Parser.h"
#include 	"global.h"

int
print_map( Proclist * procl )
{
	int single = 1;
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
	do
	{
		// MORE - map okay on running proc?
		if (!lwp->state_check(E_DEAD))
		{
			ret = 0;
			continue;
		}
		printm(MSG_map_header, lwp->lwp_name());
		if (!lwp->print_map())
			ret = 0;
	}
	while(!single && ((lwp = list++->p_lwp) != 0));
	return ret;
}
