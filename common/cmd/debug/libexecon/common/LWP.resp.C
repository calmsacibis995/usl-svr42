#ident	"@(#)debugger:libexecon/common/LWP.resp.C	1.7"

#include "Event.h"
#include "LWP.h"
#include "EventTable.h"
#include "Interface.h"
#include "Instr.h"
#include "Proctypes.h"
#include "Procctl.h"
#include "Seglist.h"
#include "List.h"
#include "global.h"
#include "Watchlist.h"
#include <sys/syscall.h>


int
LWP::respond_to_sus()
{
	check_watchpoints();
	find_cur_src();
	check_onstop();
	return show_current_location( 1, verbosity );
}

int
LWP::respond_to_sig()
{
	NotifyEvent	*ne;
	int		found = 0;
	int		show = 0;

	if ( etable == 0 )
	{
		printe(ERR_internal, E_ERROR, "LWP::respond_to_sig", __LINE__);
		return 0;
	}
	else if ( etable->siglist.ignored(latestsig) )
	{
		return restart();
	}

	ne = etable->siglist.events(latestsig); 
	if (!ne)
	{
		found++;
		show = 1;
	}
	else
	{
		for (; ne; ne = ne->next())
		{
			int	i;
			if ((i = (*ne->func)(ne->thisptr)) == NO_TRIGGER)
				continue;
			found++;
			if (i == TRIGGER_VERBOSE)
				show = 1;
		}
	}
	if (found)
	{
		check_watchpoints();
		find_cur_src();
		check_onstop();
		if (show)
			return show_current_location( 1, verbosity );
		else
			return 1;
	}
	else
		return restart();
}

int
LWP::respond_to_tsc()
{
	int		found = 0;
	NotifyEvent	*ne;
	int		show = 0;
	int		exit = ( state == es_syscallxit) ? 1 : 0;
	int		ret_val = 1;

	if ( etable == 0 )
	{
		printe(ERR_internal, E_ERROR, "LWP::respond_to_tsc", __LINE__);
		return 0;
	}

	ne = etable->tsclist.events(latesttsc, exit);
	if (exit && ((latesttsc == SYS_fork) || (latesttsc == SYS_vfork)))
	{
		if (FORK_FAILED())
		{
			if (!ne)
				return(restart());
			ret_val = 1;
		}
		else if (flags & L_IGNORE_FORK)
		{
			// process forked but we are ignoring children
			pid_t npid = SYS_RETURN_VAL();
			ret_val = release_child(npid);
			if (!ne)
				return(restart());
		}
		else
		{
			// process has forked; must create new process
			ret_val = proc_fork();
			if (!ne)
				return ret_val;
		}
	}
	else if (exit && ((latesttsc == SYS_exec) || (latesttsc == SYS_execve)))
	{
		if (EXEC_FAILED())
		{
			if (!ne)
				return(restart());
			ret_val = 1;
		}
		else
		{
			ret_val = proc_exec();
			if (!ret_val)
				printe(ERR_proc_exec, E_ERROR, lwp_name());
			if (!ne)
				return ret_val;
		}
	}
	else if (!ne)
		return restart();

	for (; ne; ne = ne->next())
	{
		int	i;
		if ((i = (*ne->func)(ne->thisptr)) == NO_TRIGGER)
			continue;
		found++;
		if (i == TRIGGER_VERBOSE)
			show = 1;
	}
	if (found)
	{
		check_watchpoints();
		find_cur_src();
		check_onstop();
		if (show)
			return show_current_location( 1, verbosity );
		else
			return ret_val;
	}
	else
		return restart();
}

int
LWP::respond_to_bkpt()
{
	NotifyEvent	*ne, *ne2, *rm_list = 0;
	int		found = 0;
	int		show = 0;

	ne = latestbkpt->events(); 
	for (; ne; ne = ne->next())
	{
		int	i;
		i = (*ne->func)(ne->thisptr);
		switch(i)
		{
			case NO_TRIGGER:
				break;
			case TRIGGER_VERBOSE:
				show = 1;
				/* FALLTHROUGH */
			case TRIGGER_QUIET:
				found++;
				break;
			case TRIGGER_FOREIGN:
			// event triggered but context for
			// event was not this LWP; we keep
			// going
				if (flags & L_WAITING)
				{
					flags &= ~L_WAITING;
					waitlist.remove(this);
				}
				break;
			case NO_TRIGGER_RM:
			// we want to remove this notify-event
			// from this breakpoint - we do it here
			// rather than in the StopEvent code
			// so as not to corrupt the list while
			// we are reading it

			ne2 = new NotifyEvent(ne->func, ne->thisptr);
			if (rm_list)
				ne2->prepend(rm_list);
			rm_list = ne2;
			break;
		}
	}
	while(rm_list)
	{
		NotifyEvent	*tmp;

		remove_bkpt(latestbkpt->_addr, rm_list->func,
			rm_list->thisptr);
		tmp = rm_list;
		rm_list = rm_list->next();
		delete tmp;
	}
	if (found)
	{
		retaddr = 0;	// we can reset retaddr on next 
				// start - if we are ignoring
				// this bkpt, we don't want
				// to reset in case we were
				// stepping over this func
		find_cur_src();
		check_onstop();
		if (show)
			show_current_location( 1, verbosity );
		check_watchpoints();
	}
	else
	{
		if (!check_watchpoints())
			return restart();
		check_onstop();
	}
	return 1;
}

// Stop due to event triggered by a foreign process
int
LWP::stop_for_event(int mode)
{
	if (!stop())
		return 0;
	
	state = es_watchpoint;
	find_cur_src();

	check_onstop();
	if (mode == TRIGGER_VERBOSE)
		show_current_location( 1, verbosity );
	check_watchpoints();
	return 1;
}

int
LWP::respond_to_dynpt()
{
	pc = instr.adjust_pc();
	if ( seglist->buildable( pctl ) )
	{
		seglist->build( pctl, ename );
	}
	return start(sg_stepbkpt);
}

int
LWP::check_watchpoints()
{
	NotifyEvent	*ne;
	int		show = 0;
	int		found = 0;

	if (etable == 0)
	{
		printe(ERR_internal, E_ERROR, "LWP::check_watchpoints",
			__LINE__);
		return 0;
	}

	ne = etable->watchlist;
	for (; ne; ne = ne->next())
	{
		int	i;
		i = (*ne->func)(ne->thisptr);
		switch(i)
		{
			default:
				break;
			case NO_TRIGGER:
				break;
			case TRIGGER_VERBOSE:
				show = 1;
				/* FALLTHROUGH */
			case TRIGGER_QUIET:
				found++;
				break;
			case TRIGGER_FOREIGN:
				if (flags & L_WAITING)
				{
					flags &= ~L_WAITING;
					waitlist.remove(this);
				}
				break;
		}
	}
	if (found)
	{
		Execstate ostate = state;

		state = es_watchpoint;
		find_cur_src();
		if (show)
			show_current_location( 1, verbosity );
		state = ostate;
		return 1;
	}
	return 0;
}
