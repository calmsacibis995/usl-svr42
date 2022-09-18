#ident	"@(#)debugger:libutil/common/run.C	1.1"
#include "utility.h"
#include "global.h"
#include "LWP.h"
#include "Proglist.h"
#include "List.h"
#include "Interface.h"
#include "Location.h"
#include "Parser.h"
#include "Frame.h"
#include "Symbol.h"
#include "Tag.h"

int
run( Proclist *procl, int ret_func, Location *location, int wait )
{
	LWP	*lwp;
	plist	*list;
	Iaddr	addr;
	int	single = 1;
	int	ret = 1;
	Symbol	func;

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
		addr = 0;
		if (!lwp->state_check(E_RUNNING|E_DEAD|E_CORE))
		{
			ret = 0;
			continue;
		}
		if (location)
		{
			if ( get_addr( lwp, location, addr, st_func, func ) == 0 )
			{
				ret = 0;
				continue;
			}
			// if stop requested on function name, 
			// go past prolog
			if (location->get_type() == lk_fcn && 
				(func.tag() != t_label))
			{
				long	off;
				location->get_offset(off);
				if (off == 0)
					addr = lwp->first_stmt(addr);
			}
		}
		else if (ret_func)
		{
			Frame	*frame = lwp->curframe();
			Iaddr	tmp;

			if (frame->retaddr(addr, tmp) == 0)
			{
				printe(ERR_return_addr, E_ERROR, 
					lwp->prog_name());
				ret = 0;
				continue;
			}
		}
		if (!lwp->run( 0, addr, vmode ))
		{
			ret = 0;
			continue;
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
