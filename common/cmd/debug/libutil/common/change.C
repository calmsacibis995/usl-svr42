#ident	"@(#)debugger:libutil/common/change.C	1.4.1.1"

// Change event
// Delete old event and create a new one with the same event id.

#include "Event.h"
#include "EventTable.h"
#include "LWP.h"
#include "Parser.h"
#include "Proglist.h"
#include "TSClist.h"
#include "Interface.h"
#include <signal.h>


int
change_event( int event, Proclist *procl, int count, Systype systype,
	int set_quiet, void *event_expr, Node *cmd)
{
	Event		**elist;
	Event		*eptr;
	int		found = 0;
	int		first = 1;
	LWP		*lwp;
	plist		*list, *old_list;
	int		ret = 1;
	int		new_event = 0;
	int		i, cnt;

	char		old_op;
	int		old_count;
	Systype		old_stype;
	Node		*old_cmd;
	int		old_level;
	int		old_quiet = 0;
	StopEvent	*old_stop;
	sigset_t	old_sigs;
	IntList		*old_sys;
	int		old_state;


	// do we need to delete old event?
	int delete_old = (procl || (systype != NoSType) || event_expr);

	// Go through all events with this id; if
	// we are deleting,  we must save the old information on
	// the event.  If we are not deleting,
	// change part of event that was specified.

	// proglist.make_list and add_list provide an external way to
	// create a list of LWP's.  Every make_list should be balanced
	// with a list_done
	if (!procl)
		old_list = proglist.make_list();
	elist = m_event.events();
	if (!elist)
	{
		printe(ERR_no_event_id, E_ERROR, event);
		if (!procl)
			proglist.list_done();
		return 0;
	}
	cnt = m_event.event_cnt();

	if (get_ui_type() == ui_gui)
		set_quiet = -1;

	for(i = 0; i < cnt; i++)
	{
		eptr = m_event[i];
		if (!eptr)
			continue;
		if (eptr->get_id() == event && 
			(eptr->get_state() != E_DELETED))
		{
			found++;
			old_op = eptr->get_type();
			if (first)
			{
				// check for syntax - must do here
				// since it's first time we
				// know type of event
				if (old_op == E_ONSTOP ||
					old_op == E_SIGNAL)
				{
					if (count >= 0)
					{
						printe(ERR_opt_change,
							E_ERROR, "-c");
						return 0;
					}
					if ((set_quiet >= 0) &&
						(old_op == E_ONSTOP))
					{
						printe(ERR_opt_change,
						E_ERROR,
						(set_quiet ? "-q" : "-v"));
						return 0;
					}
				}
				if ((systype != NoSType) &&
					(old_op != E_SCALL))
				{
					printe(ERR_opt_change, E_ERROR,
						"-ex");
					return 0;
				}
			}
			if (!eptr->get_etable()->lwp)
			{
				printe(ERR_cannot_change, E_ERROR, event);
				if (!procl)
					proglist.list_done();
				return 0;
			}
			if (!delete_old)
			{
				if (!eptr->get_etable()->lwp->state_check(E_RUNNING))
				{
					return 0;
				}
				switch(old_op)
				{
				case E_ONSTOP:
				case E_SIGNAL:
					break;
				case E_SCALL:
					if (count >= 0)
					{
						((Sys_e *)eptr)->set_count(count);
						((Sys_e *)eptr)->reset_count();
					}
					break;
				case E_STOP:
					if (count >= 0)
					{
						((Stop_e *)eptr)->set_count(count);
						((Stop_e *)eptr)->reset_count();
					}
					break;
				}
				if (set_quiet == 1)
					eptr->set_quiet();
				else if (set_quiet == 0)
					eptr->set_verbose();
				if (cmd)
					eptr->set_cmd(cmd);
			}
			else
			{
				if (!procl)
					proglist.add_list(eptr->get_etable()->lwp);
				if (first)
				{
					first = 0;	
					old_level = eptr->get_level();
					old_cmd = eptr->get_cmd();
					old_state = eptr->get_state();
					switch(old_op)
					{
					case E_ONSTOP:	break;
					case E_STOP:
						old_count = ((Stop_e *)eptr)->get_count();
						old_stop = ((Stop_e *)eptr)->get_stop();
						break;
					case E_SIGNAL:
						old_sigs = ((Sig_e *)eptr)->get_sigs();
						break;
					case E_SCALL:
						old_stype = ((Sys_e *)eptr)->get_stype();
						old_sys = ((Sys_e *)eptr)->get_calls();
						old_count = ((Sys_e *)eptr)->get_count();
						break;
					}
				}
			}
		}
	}
	if (!found)
	{
		if (!procl)
			proglist.list_done();
		printe(ERR_no_event_id, E_ERROR, event);
		return 0;
	}
	if (!delete_old)
	{
		if (get_ui_type() == ui_gui)
			printm(MSG_event_changed, event);
		return 1;
	}
	if (count >= 0)
		old_count = count;
	if (set_quiet != -1)
		old_quiet = set_quiet;
	if (cmd)
		old_cmd = cmd;
	if (systype != NoSType)
		old_stype = systype;
	if (event_expr)
	{
		switch(old_op)
		{
		default:
			break;
		case E_STOP:
			old_stop = (StopEvent *)event_expr;
			break;
		case E_SIGNAL:
		{
			int	mask = (int)event_expr;

			sigemptyset(&old_sigs);
			for(int i = 0; i < NSIG-1; i++)
			{
				if (mask & 0x1)
					sigaddset(&old_sigs, i+1);
				mask >>= 1;
			}
			break;
		}
		case E_SCALL:
			old_sys = (IntList *)event_expr;
			break;
		}
	}
	else 
	{
		if (old_op == E_SCALL)
		{
			// copy IntList
			IntList	*tmp = old_sys;
			old_sys = new IntList;
			tmp->first();
			do
			{
				old_sys->add(tmp->val());
			} while(tmp->next());
		}
	}

	if (procl)
	{
		list = proglist.proc_list(procl);
	}
	else
	{
		list = old_list;
	}
	lwp = list->p_lwp;
	if (!lwp)
	{
		if (!procl)
			proglist.list_done();
		printe(ERR_no_proc, E_ERROR);
		return 0;
	}

	// create new event, with bogus number for now
	for (; lwp; list++, lwp = list->p_lwp)
	{	
		if (!lwp->state_check(E_RUNNING|E_DEAD|E_CORE))
		{
			ret = 0;
			continue;
		}

		if (list->p_type & PEXEC)
			old_level = E_PROGRAM;
		else if (list->p_type & PPROC)
			old_level = E_PROCESS;
		switch(old_op)
		{
		default:	break;
		case E_ONSTOP:
			eptr = new Onstop_e(-1, old_level, old_cmd, 
				lwp->events());
			break;
		case E_STOP:
			StopEvent	*se = copy_tree(old_stop);
			eptr = (Event *)new Stop_e(se, -1, old_level,
				old_quiet, old_count, old_cmd, lwp->events());
			break;
		case E_SIGNAL:
			eptr = (Event *)new Sig_e(old_sigs, -1, old_level,
				old_quiet, old_cmd, lwp->events());
			break;
		case E_SCALL:
			eptr = (Event *)new Sys_e(old_sys, old_stype, -1, 
				old_level, old_quiet, old_count,
				old_cmd, lwp->events());
			break;
		}
		if (eptr->get_state() != E_DELETED)
		{
			lwp->add_event(eptr);
			m_event.add(eptr);
			new_event++;
		}
		else
		{
			delete eptr;
			ret = 0;
		}
	}

	if (ret)
	{
		// everything's ok, so now delete old event
		// and reset new event's number
		if (!m_event.event_op(event, M_Delete))
		{
			m_event.event_op(-1, M_Delete);
			if (!procl)
				proglist.list_done();
			return 0;
		}

		cnt = m_event.event_cnt();

		for(i = 0; i < cnt; i++)
		{
			eptr = m_event[i];
			if (!eptr)
				continue;
			if (eptr->get_id() == -1)
			{
				eptr->set_id(event);
				if (old_state == E_DISABLED ||
					old_state == E_DISABLED_INV)
					eptr->disable();
			}
		}

		if (get_ui_type() == ui_gui)
			printm(MSG_event_changed, event);
	}
	else if (new_event)
		m_event.event_op(-1, M_Delete);

	if (!procl)
		proglist.list_done();
	return ret;
}
