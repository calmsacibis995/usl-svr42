#ident	"@(#)debugger:libutil/common/cancel_sig.C	1.2"
#include "utility.h"
#include "LWP.h"
#include "Proglist.h"
#include "Parser.h"
#include "Interface.h"

int
cancel_sig(Proclist *procl, int mask)
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
		if (!lwp->state_check(E_RUNNING|E_DEAD|E_CORE))
		{
			ret = 0;
			continue;
		}

		if (!mask)
		{
			if (!lwp->cancel_sig_all())
			{
				ret = 0;
				continue;
			}
		}
		else
		{
			int temp_mask = mask;
			for (int i = 0; i < NSIG-1; i++)
			{
				if (temp_mask & 0x1)
				{
					if (!lwp->cancel_sig( i+1 ))
					{
						ret = 0;
						continue;
					}
				}
				temp_mask >>= 1;
			}
		}
	}
	while(!single && ((lwp = list++->p_lwp) != 0));
	return ret;
}
