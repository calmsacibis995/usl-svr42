#ident	"@(#)debugger:libutil/common/set_sig.C	1.1"

#include "utility.h"
#include "LWP.h"
#include "Proglist.h"
#include "Interface.h"
#include "Parser.h"
#include "Event.h"
#include <signal.h>


int
set_signal( Proclist * procl, sigset_t sigs, Node *cmd, int ignore, int quiet )
{
	LWP	*lwp;
	plist	*list;
	Sig_e	*eptr;
	int	eid;
	int	level = 0;
	int	success = 0;
	int	ret = 1;

	if (procl)
	{
		list = proglist.proc_list(procl);
	}
	else
	{
		list = proglist.proc_list(proglist.current_program());
	}
	lwp = list->p_lwp;
	if (!lwp)
	{
		printe(ERR_no_proc, E_ERROR);
		return 0;
	}
	if (cmd)
	{
		eid = m_event.new_id();
	}

	if (get_ui_type() == ui_gui)
		quiet = 0;

	for (; lwp; list++, lwp = list->p_lwp)
	{	
		if (!lwp->state_check(E_RUNNING|E_DEAD|E_CORE))
		{
			ret = 0;
			continue;
		}
		if (!cmd)
		// no event - just set signals
		{
			if (ignore)
			{
				if (!lwp->remove_sig(sigs, 0, 0))
					ret = 0;
			}
			else
			{
				if (!lwp->set_sig_catch(sigs, 0, 0))
					ret = 0;
			}
		}
		else
		{
			if (list->p_type & PEXEC)
				level = E_PROGRAM;
			else if (list->p_type & PPROC)
				level = E_PROCESS;
			eptr = new Sig_e(sigs, eid, level, quiet, cmd, 
				lwp->events());
			if (eptr->get_state() == E_ENABLED)
			{
				m_event.add((Event *)eptr);
				lwp->add_event((Event *)eptr);
				success++;
			}
			else
			{
				ret = 0;
				delete(eptr);
			}
		}
	}
	if (cmd)
	{
		if (success)
		{
			m_event.set_last(eid);
			printm(MSG_event_assigned, eid);
			return ret;
		}
		else
		{
			m_event.dec_id();
			return 0;
		}
	}
	else
		return ret;
}

