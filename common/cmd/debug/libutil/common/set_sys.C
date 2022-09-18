#ident	"@(#)debugger:libutil/common/set_sys.C	1.1"

#include "utility.h"
#include "LWP.h"
#include "Proglist.h"
#include "Interface.h"
#include "Parser.h"
#include "Event.h"
#include "TSClist.h"


int
set_syscall( Proclist * procl, IntList *sys, Systype systype, 
	Node *cmd, int count, int quiet )
{
	LWP	*lwp;
	plist	*list;
	Sys_e	*eptr;
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
	if (get_ui_type() == ui_gui)
		quiet = 0;
	eid = m_event.new_id();
	for (; lwp; list++, lwp = list->p_lwp)
	{	
		if (!lwp->state_check(E_RUNNING|E_DEAD|E_CORE))
		{
			ret = 0;
			continue;
		}
		if (list->p_type & PEXEC)
			level = E_PROGRAM;
		else if (list->p_type & PPROC)
			level = E_PROCESS;
		eptr = new Sys_e(sys, systype, eid, level, quiet, count, cmd, 
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
