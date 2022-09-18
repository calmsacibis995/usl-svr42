#ident	"@(#)debugger:libexecon/common/Event.C	1.8"


#include "Event.h"
#include "List.h"
#include "Parser.h"
#include "Buffer.h"
#include "LWP.h"
#include "Program.h"
#include "Proglist.h"
#include "EventTable.h"
#include "TSClist.h"
#include "Siglist.h"
#include "Interface.h"
#include "Proctypes.h"
#include "global.h"
#include "utility.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/procfs.h>

EventManager	m_event;


EventManager::~EventManager()
{
	free(base);
}


#define	E_BLOCKSIZE	100	// # of events allocated at once

// The EventManager class maintains events via an array of pointers
// to class Event objects.  This array is allocated in E_BLOCKSIZE
// chunks, and realloc'ed as needed.  

Event *
EventManager::add( Event *event )
{
	register Event	**e;

	count++;

	// allocate more space for pointer list
	// last pointer always null
	if (count > (ptr_count - 1))
	{
		if (base == 0)
		{
			if ((base = (Event **)
				malloc(E_BLOCKSIZE*sizeof(Event *))) == 0)
			{
				new_handler();
			}
			memset(base, 0, sizeof(Event *)*E_BLOCKSIZE);
			ptr_count = E_BLOCKSIZE;
			e = &(base[0]);
		}
		else
		{
			if ((base = (Event **)realloc((char *)base, sizeof(Event *)
				* (ptr_count + E_BLOCKSIZE))) == 0)
			{
				new_handler();
			}
			memset(base+ptr_count, 0, sizeof(Event *)*E_BLOCKSIZE);
			e = &(base[ptr_count-1]);
			ptr_count += E_BLOCKSIZE;
		}
	}
	else
	{
		// still free entries - find one
		register Event	**eend = base + ptr_count;

		e = base;
		while(*e && (e < eend))
		{
			e++;
		}
	}
	*e = event;
	event->manage_slot = e;
	return event;
}

// Copy an event, using a new event table.
// First check that we don't already have an event with this id
// for this process.
int
EventManager::copy(Event *eptr, EventTable *etable, LWP *lwp)
{
	Event		*e;
	int		eid = eptr->id;
	int		state, level, quiet;
	Node		*cmd;

	if (eptr->state == E_DELETED)
		return 1;
	for (e = lwp->first_event(); e; e = e->next())
	{
		if (e->id == eid)
			return 1;
	}
	state = eptr->state;
	level = eptr->level;
	cmd = eptr->cmd;
	quiet = eptr->quiet;

	switch(eptr->get_type())
	{
	case E_ONSTOP:
		e = (Event *)new Onstop_e(eid, level, cmd, etable);
		break;
	case E_STOP:
		e = (Event *)new Stop_e(eid, level, quiet,
			((Stop_e *)eptr)->get_count(), cmd, etable);
		((Stop_e *)e)->copy((Stop_e *)eptr);
		break;
	case E_SIGNAL:
		e = (Event *)new Sig_e(((Sig_e *)eptr)->get_sigs(), eid, level, 
			quiet, cmd, etable);
		break;
	case E_SCALL:
		e = (Event *)new Sys_e(((Sys_e *)eptr)->get_calls(), 
			((Sys_e *)eptr)->get_stype(),
			eid, level, quiet, ((Sys_e *)eptr)->get_count(), 
			cmd, etable);
		break;
	}
	if (e->get_state() == E_DELETED)
	{
		printe(ERR_event_copy, E_WARNING, eid, lwp->lwp_name());
		delete e;
		return 0;
	}
	lwp->add_event(e);
	add(e);
	return 1;
}


#define MAX_LEN	20	// length of associated cmd string

// display a single event - the mode controls the
// fullness of the display
//
// uses 3 global buffers - one for the event display itself,
// one for the list of processes, and one, indirectly, in list_cmd()
void
EventManager::display(Event *event, int mode = 0, char *proclist = 0)
{
	sigset_t	*sigs;
	sigset_t	localSig;
	IntList		*p;
	Node		*cmd = event->cmd;
	const char	*sys_type;
	const char	*dis;
	char		cmd_str[MAX_LEN];
	int		first = 1;
	const char	*cmdptr;
	const char	*quiet = " ";

        if (proclist == 0) proclist = "";

	if (cmd)
	{
		int		len;

		if (cmd->op == SAVE)
			cmd = cmd->first.np;
		// list command
		cmdptr = list_cmd(cmd);
		if (!mode)  // short print
		{
			len = strlen(cmdptr);
			if (len+1 >= MAX_LEN)
			{
				len = MAX_LEN - sizeof("...");
				strncpy(cmd_str, cmdptr, len);
				strcpy(cmd_str + len, "...");
				cmdptr = cmd_str;
			}
		}
	}
	else
		cmdptr = "";

	switch(event->state)
	{
		default:
			dis = "  ";
			break;
		case E_INVALID:
			dis = "I ";
			break;
		case E_DISABLED_INV:
			dis = "DI";
			break;
		case E_DISABLED:
			dis = "D ";
			break;
	}

	if (event->quiet)
		quiet = "Q";

	switch(event->get_type())
	{
	case E_SCALL:
		p = ((Sys_e *)event)->get_calls();
		if (!p->first())
		{
			printe(ERR_internal, E_ERROR,
				"EventManager::display" , __LINE__);
			return;
		}

		switch(((Sys_e *)event)->get_stype())
		{
		case Entry:		sys_type = "E";  break;
		case Exit:		sys_type = "X";  break;
		case Entry_exit:	sys_type = "EX"; break;
		case NoSType:
		default:		sys_type = "?";  break;
		}

		gbuf1.clear();
		do
		{
			if (first)
				first = 0;
			else
				gbuf1.add(", ");
			gbuf1.add(sysname(p->val()));
		} while(p->next());
		if (mode)
			printm(MSG_syscall_event_f, event->id, quiet, dis,
				sys_type, ((Sys_e *)event)->get_count(), 
				(char *)gbuf1, proclist, cmdptr);
		else
			printm(MSG_syscall_event, event->id, quiet, dis,
				sys_type, ((Sys_e *)event)->get_count(), 
				(char *)gbuf1, cmdptr);
		break;

	case E_ONSTOP:
		if (mode)
			printm(MSG_onstop_event_f, event->id, dis, 
				proclist, cmdptr);
		else
			printm(MSG_onstop_event, event->id, dis, cmdptr);
		break;

	case E_STOP:
		if (mode)
			printm(MSG_stop_event_f, event->id, quiet, dis,
				((Stop_e *)event)->get_count(), 
				((Stop_e *)event)->get_expr(), proclist,
				cmdptr);
		else
			printm(MSG_stop_event, event->id, quiet, dis,
				((Stop_e *)event)->get_count(), 
				((Stop_e *)event)->get_expr(), cmdptr);
		break;

	case E_SIGNAL:
		gbuf1.clear();
		localSig = ((Sig_e *)event)->get_sigs();
		sigs = &localSig;
		for(int i = 1; i < NSIG; i++)
		{
			if (prismember(sigs, i))
			{
				if (first)
					first = 0;
				else
					gbuf1.add(", ");
				gbuf1.add(signame(i));
			}
		}
		if (mode)
			printm(MSG_signal_event_f, event->id, quiet, dis,
				(char *)gbuf1, proclist, cmdptr);
		else
			printm(MSG_signal_event, event->id, quiet, dis,
				(char *)gbuf1, cmdptr);
		break;
	}
}

// display current signal status for all signals
// signal events are displayed separately
static void
display_sigs(LWP *lwp)
{
	sig_ctl		*sigs;

	printm(MSG_signal_header, lwp->lwp_name(), lwp->prog_name());
	sigs = lwp->events()->siglist.sigset();
	for (int i = 1; i < NSIG; i++)
		printm(prismember(&sigs->signals, i) ? MSG_sig_caught : MSG_sig_ignored,
			i, signame(i));
}

// perform designated operation on a single event
int
EventManager::event_op( int eid , Event_op op)
{
	Event	**elist;
	Event	*eptr;
	Event	*saveptr;
	int	found = 0;
	Event	**eend;
	int	ret = 1;

	elist = base;
	eend = base + ptr_count;

	if (!elist)
	{
		printe(ERR_no_event_id, E_ERROR, eid);
		return 0;
	}

	gbuf2.clear();
	for( ; elist < eend; elist++)
	{
		eptr = *elist;
		if (!eptr)
			continue;
		if (eptr->id == eid && 
			(eptr->state != E_DELETED))
		{
			if (op == M_Display)
			{
				if (!found)
				{
					found++;
					saveptr = eptr;
				}
				else
					gbuf2.add(", ");
				if (eptr->etable->lwp)
					gbuf2.add(eptr->etable->lwp->lwp_name());
				else
					gbuf2.add(eptr->etable->prog->prog_name());
				continue;
			}
			else
			{
				LWP	*lwp = eptr->etable->lwp;
				if (lwp)
				{
					if (!lwp->state_check(E_RUNNING))
					{
						found++;
						ret = 0;
						continue;
					}
				}
			}
			found++;
			switch(op)
			{
			case M_Delete:
				if (eptr->remove())
				{
					delete(eptr);
					*elist = 0;
					count--;
				}
				else
					ret = 0;
				break;
			// Disabling and enabling events
			// is done by setting a flag;
			// the events still happen, but
			// the user doesn't find out if
			// the disabled flag is set.
			case M_Enable:
				eptr->enable();
				break;
			case M_Disable:
				eptr->disable();
				break;
			default:
				break;
			}
		}
	}
	if (found)
	{
		if (op == M_Display)
			display(saveptr, 1, (char *)gbuf2);
		return ret;
	}
	else
	{
		printe(ERR_no_event_id, E_ERROR, eid);
		return 0;
	}
}

// perform designated operation on
// all events of a given type for a given proclist;
// if etype is 0, change all events for that proclist
int
EventManager::event_op( Proclist *procl, int etype , Event_op op)
{
	LWP	*lwp;
	plist	*list;
	Program	**protolist;
	Program	*prog;
	int	single = 1;
	int	ret = 1;

	if (procl)
	{
		single = 0;
		list = proglist.proc_list(procl, protolist);
		lwp = list++->p_lwp;
		prog = *protolist++;
	}
	else if (op == M_Display)
	{
		single = 0;
		list = proglist.proc_list(proglist.current_program());
		lwp = list++->p_lwp;
		prog = 0;
	}
	else
	{
		lwp = proglist.current_lwp();
		prog = 0;
	}
	if (!lwp && !prog)
	{
		printe(ERR_no_proc, E_ERROR);
		return 0;
	}
	for(; lwp; lwp = list++->p_lwp)
	{	

		if (op != M_Display)
		{
			if (!lwp->state_check(E_RUNNING))
			{
				ret = 0;
				if (single)
					return ret;
				continue;
			}
		}
		if (!event_op(lwp, 0, etype, op))
			ret = 0;
		if (single)
			return ret;
	}
	for(; prog; prog = *protolist++)
	{
		if (!event_op(0, prog, etype, op))
			ret = 0;
	}
	return ret;
}


int
EventManager::event_op(LWP *lwp, Program *prog, int etype, Event_op op)
{
	Event	**elist;
	Event	**eend;
	Event	*eptr;
	int	ret = 1;
		
	elist = base;
	eend = base + ptr_count;

	if (op == M_Display)
	{
		if (etype == E_SIGNAL)
		{
			if (lwp)
				display_sigs(lwp);
			if (!elist)
				return 1;
		}

		if (lwp)
			printm(MSG_events, lwp->lwp_name(), 
				lwp->prog_name());
		else
			printm(MSG_events_proto, prog->prog_name());
		printm(MSG_event_header);
	}
	if (!elist)
		return 1;
	for( ; elist < eend; elist++)
	{
		eptr = *elist;
		if (!eptr)
			continue;
		if ((!etype || eptr->get_type() == etype) 
			&& (eptr->state != E_DELETED))
		{
			if (lwp) 
			{
				if (eptr->etable->lwp != lwp)
					continue;
			}
			else
			{
				if (eptr->etable->prog != prog)
					continue;
			}
			switch(op)
			{
			case M_Delete:
				if (eptr->remove())
				{
					delete(eptr);
					*elist = 0;
					count--;
				}
				else
					ret = 0;
				break;
			case M_Enable:
				eptr->enable();
				break;
			case M_Disable:
				eptr->disable();
				break;
			case M_Display:
				display(eptr, 0);
				break;
			case M_Nop:
			default:
				break;
			}
		}
	}
	return ret;
}


// find first active (not deleted) event with given id
Event *
EventManager::find_event(int eid)
{
	register Event	**elist = base;
	register Event	**eend;
	register Event	*eptr;

	eend = base + ptr_count;

	if (!elist)
	{
		return 0;
	}

	for( ; elist < eend; elist++)
	{
		eptr = *elist;
		if (!eptr)
			continue;
		if (eptr->id == eid && 
			(eptr->state != E_DELETED))
		{
			return eptr;
		}
	}
	return 0;
}

// remove event, but don't bother going to LWP to
// disable primitive event
void
EventManager::drop_event(Event *event)
{

	Event	**eend = base + ptr_count;
	Event	**eptr = event->manage_slot;

	if ((eptr < base) || eptr >= eend)
		return;

	delete(event);
	*eptr = 0;
	count--;
	return;
}

Event::Event(int Id, int Lvl, int Quiet, Node *Cmd, EventTable *Et)
{
	id = Id;
	level = Lvl;
	quiet = Quiet;
	cmd = Cmd;
	etable = Et;
	state = E_DELETED;
	manage_slot = 0;
}

int
Event::disable()
{
	if (state == E_ENABLED)
	{
		state = E_DISABLED;
	}
	else if (state == E_INVALID)
	{
		state = E_DISABLED_INV;
	}
	else
		return 0;
	if (get_ui_type() == ui_gui)
		printm(MSG_event_disabled, id,
			(unsigned long)etable->lwp);
	return 1;
}

int
Event::enable()
{
	if (state == E_DISABLED)
	{
		state = E_ENABLED;
	}
	else if (state == E_DISABLED_INV)
	{
		state = E_INVALID;
	}
	else
		return 0;
	if (get_ui_type() == ui_gui)
		printm(MSG_event_enabled, id, 
			(unsigned long)etable->lwp);
	return 1;
}

// Null base class version
int
Event::remove()
{
	return 1;
}

// Null base class version
int
Event::trigger()
{
	return NO_TRIGGER;
}


void
Event::cleanup(int l)
{
	if (state == E_DELETED)
	{
		return;
	}
	if (level < l)
	{
		remove();
		return;
	}
	if (state == E_ENABLED)
		state = E_INVALID;
	else if (state == E_DISABLED)
		state = E_DISABLED_INV;
}

int
Event::re_init()
{
	if (state == E_INVALID)
		state = E_ENABLED;
	else if (state == E_DISABLED_INV)
		state = E_DISABLED;
	return 1;
}


// Null base class version
int
Event::get_type()
{
	return 0;
}

Sig_e::Sig_e(sigset_t Sigs, int Id, int Lvl, int Quiet, 
	Node *Cmd, EventTable *Et) : EVENT(Id, Lvl, Quiet, Cmd, Et)
{
	signals = Sigs;
	if (Et->lwp->set_sig_catch(signals, 
		(Notifier)notify_sig_e_trigger, this))
	{
		state = E_ENABLED;
	}
}

int
Sig_e::trigger()
{

	if (state != E_ENABLED)
		return NO_TRIGGER;
	m_event.set_this(id);
	if (cmd)
	{
		A_cmd		*cp;

		cp = new A_cmd;
		cp->cmd = cmd;
		cp->event = this;
		m_cmdlist.add(cp);
		if (get_ui_type() == ui_gui)
			printm(MSG_assoc_cmd);
	}
	if (!quiet)
		return TRIGGER_VERBOSE;
	else
		return TRIGGER_QUIET;
	
}

int
Sig_e::remove()
{
	LWP	*lwp = etable->lwp;
	int	success;
	if (lwp)
	{
		success = lwp->remove_sig(signals, 
			(Notifier)notify_sig_e_trigger, this);
		lwp->remove_event(this);
	}
	else
	{
		sig_ctl	sigs;
		sigs.signals = signals;
		success = etable->siglist.remove(&sigs,
			(Notifier)notify_sig_e_trigger, this);
		etable->remove_event(this);
	}
	if (success)
	{
		state = E_DELETED;
		if (get_ui_type() == ui_gui)
			printm(MSG_event_deleted, id, 
				(unsigned long)etable->lwp);
		return 1;
	}
	else
	{
		printe(ERR_internal, E_ERROR, "Sig_e::remove", __LINE__);
		return 0;
	}
}

int
Sig_e::get_type()
{
	return E_SIGNAL;
}

Sys_e::Sys_e(IntList *sys, Systype stype, int Id, int Lvl, int Quiet, 
	int count, Node *Cmd, EventTable *Et) : EVENT(Id, Lvl, Quiet, Cmd, Et)
{
	int	fail = 0;
	int	snum;

	systype = stype;
	syscalls = new IntList;
	orig_count = count;
	cur_count = 0;

	if (!sys->first())
	{
		delete syscalls;
		syscalls = 0;
		return;
	}
	do
	{
		snum = sys->val();
		syscalls->add(snum);
		if (!Et->lwp->set_sys_trace(snum, stype, 
			(Notifier)notify_sys_e_trigger, this))
		{
			fail++;
			break;
		}
	} while(sys->next());

	if (!fail)
	{
		state = E_ENABLED;
	}
	else
	{
		// failure - remove syscalls already set
		sys->first();
		while(snum != sys->val())
		{
			Et->lwp->remove_sys_trace(sys->val(), stype,
				(Notifier)notify_sys_e_trigger, this);
			sys->next();
		}
		delete syscalls;
		syscalls = 0;
	}
}

Sys_e::~Sys_e()
{
	delete syscalls;
}

int
Sys_e::trigger()
{

	if (state != E_ENABLED)
		return NO_TRIGGER;
	cur_count++;
	if (cur_count < orig_count)
		return NO_TRIGGER;
	m_event.set_this(id);
	if (cmd)
	{
		A_cmd		*cp;

		cp = new A_cmd;
		cp->cmd = cmd;
		cp->event = this;
		m_cmdlist.add(cp);
		if (get_ui_type() == ui_gui)
			printm(MSG_assoc_cmd);
	}
	if (!quiet)
		return TRIGGER_VERBOSE;
	else
		return TRIGGER_QUIET;
	
}

int
Sys_e::remove()
{
	LWP	*lwp = etable->lwp;
	int	snum;
	int	fail = 0;

	if (!syscalls->first())
	{
		state = E_DELETED;
		if (get_ui_type() == ui_gui)
			printm(MSG_event_deleted, id,
				(unsigned long)etable->lwp);
		return 1;
	}
	do
	{
		snum = syscalls->val();
		if (lwp)
		{
			if (!lwp->remove_sys_trace(snum, systype,
				(Notifier)notify_sys_e_trigger, this))
				fail = 1;
		}
		else
		{
			if (systype == Entry || 
				systype == Entry_exit)
			{
				if (!etable->tsclist.remove(snum, 0,
				(Notifier)notify_sys_e_trigger, this))

					fail = 1;
			}
			if (systype == Exit || 
				systype == Entry_exit)
			{
				if (!etable->tsclist.remove(snum, 1,
				(Notifier)notify_sys_e_trigger, this))
					fail = 1;
			}
		}
	} while(syscalls->next());
	if (lwp)
		lwp->remove_event(this);
	else
		etable->remove_event(this);
	if (fail)
	{
		printe(ERR_internal, E_ERROR, "Sys_e::remove", __LINE__);
		return 0;
	}

	state = E_DELETED;
	if (get_ui_type() == ui_gui)
		printm(MSG_event_deleted, id, 
			(unsigned long)etable->lwp);
	return 1;
}

int
Sys_e::get_type()
{
	return E_SCALL;
}

int
Sys_e::re_init()
{
	if (state == E_INVALID)
		state = E_ENABLED;
	else if (state == E_DISABLED_INV)
		state = E_DISABLED;
	cur_count = 0;
	return 1;
}

Onstop_e::Onstop_e(int Id, int Lvl,
	Node *Cmd, EventTable *Et) : EVENT(Id, Lvl, 0, Cmd, Et)
{
	Et->set_onstop((Notifier)notify_onstop_e_trigger, this);
	state = E_ENABLED;
}

int
Onstop_e::trigger()
{

	if (state != E_ENABLED)
		return NO_TRIGGER;
	m_event.set_this(id);
	if (cmd)
	{
		A_cmd		*cp;

		cp = new A_cmd;
		cp->cmd = cmd;
		cp->event = this;
		m_cmdlist.add(cp);
		if (get_ui_type() == ui_gui)
			printm(MSG_assoc_cmd);
	}
	return TRIGGER_VERBOSE;
}

int
Onstop_e::remove()
{
	if (etable->lwp)
		etable->lwp->remove_event(this);
	else
		etable->remove_event(this);
	if (etable->remove_onstop((Notifier)notify_onstop_e_trigger, this))
	{
		state = E_DELETED;
		if (get_ui_type() == ui_gui)
			printm(MSG_event_deleted, id, 
				(unsigned long)	etable->lwp);
		return 1;
	}
	return 0;
}

int
Onstop_e::get_type()
{
	return E_ONSTOP;
}

Stop_e::Stop_e(StopEvent *Stop, int Id, int Lvl, 
	int Quiet, int count, Node *Cmd, EventTable *Et) :
		EVENT(Id, Lvl, Quiet, Cmd, Et)
{
	int			i, invalid = 0;
	register StopEvent	*se;

	stop = Stop;
	orig_count = count;
	cur_count = 0;
	event_expr = print_stop(stop, Et->lwp);

	// MORE for now we set all parts of a stop event.
	// optimizations may be possible
	for(se = stop; se; se = se->next())
	{
		if ((i = se->stop_set(Et->lwp, this)) == SET_FAIL)
			break;
		else if (i == SET_INVALID)
			invalid = 1;
	}
	if (i == SET_FAIL)
	{
		// might have set part of stop expression
		for(se = stop; se; se = se->next())
		{
			// StopEvents deleted in Stop_e destructor
			se->remove();
		}
		return;
	}
	if (invalid)
		state = E_INVALID;
	else
		state = E_ENABLED;
}

Stop_e::~Stop_e()
{ 
	dispose_event(stop); 
	delete event_expr;
}

int
Stop_e::trigger()
{

	if ((state != E_ENABLED) ||
		(!stop_eval(stop)))
		return NO_TRIGGER;
	cur_count++;
	if (cur_count < orig_count)
		return NO_TRIGGER;
	m_event.set_this(id);
	if (cmd)
	{
		A_cmd		*cp;

		cp = new A_cmd;
		cp->cmd = cmd;
		cp->event = this;
		m_cmdlist.add(cp);
		if (get_ui_type() == ui_gui)
			printm(MSG_assoc_cmd);
	}
	etable->lwp->set_expr(event_expr);
	if (!quiet)
		return TRIGGER_VERBOSE;
	else
		return TRIGGER_QUIET;
}

// Event triggered by LWP that was not LWP in whose context
// event was set.  Only stop latter LWP.
int
Stop_e::trigger_foreign()
{

	LWP	*l;

	if ((!stop_eval(stop)) ||
		(state != E_ENABLED))
		return NO_TRIGGER;
	cur_count++;
	if (cur_count < orig_count)
		return NO_TRIGGER;
	m_event.set_this(id);
	if (cmd)
	{
		A_cmd		*cp;

		cp = new A_cmd;
		cp->cmd = cmd;
		cp->event = this;
		m_cmdlist.add(cp);
		if (get_ui_type() == ui_gui)
			printm(MSG_assoc_cmd);
	}
	l = etable->lwp;
	l->set_expr(event_expr);
	if (!quiet)
		l->stop_for_event(TRIGGER_VERBOSE);
	else
		l->stop_for_event(TRIGGER_QUIET);
	return TRIGGER_FOREIGN;
}

int
Stop_e::remove()
{
	register StopEvent	*se = stop;
	int			fail = 0;
	LWP			*lwp = etable->lwp;

	if (state == E_DELETED)
	{
		if (get_ui_type() == ui_gui)
			printm(MSG_event_deleted, id, 
				(unsigned long)etable->lwp);
		return 1;
	}
	for(se = stop; se; se = se->next())
	{
		// StopEvents deleted in Stop_e destructor
		if (!se->remove())
			fail = 1;
	}
	if (lwp)
		lwp->remove_event(this);
	else
		etable->remove_event(this);
	if (fail)
	{
		printe(ERR_internal, E_ERROR,
			"Stop_e::remove" , __LINE__);
		return 0;
	}
	if (get_ui_type() == ui_gui)
		printm(MSG_event_deleted, id, 
			(unsigned long)etable->lwp);
	state = E_DELETED;
	return 1;
}

int
Stop_e::get_type()
{
	return E_STOP;
}

void
Stop_e::cleanup(int l)
{
	register StopEvent	*se;
	if (level < l)
	{
		remove();
		return;
	}
	for(se = stop; se; se = se->next())
	{
		se->cleanup();
	}
	if (state == E_ENABLED)
		state = E_INVALID;
	else if (state == E_DISABLED)
		state = E_DISABLED_INV;
}

int
Stop_e::re_init()
{
	int			i, invalid = 0;
	register StopEvent	*se, *se2;

	for(se = stop; se; se = se->next())
	{
		if ((i = se->re_init()) == SET_FAIL)
		{
			for(se2 = stop; se2; se2 = se2->next())
			{
				se2->remove();
			}
			state = E_DELETED;
			return 0;
		}
		else if (i == SET_INVALID)
			invalid = 1;
	}
	if (!invalid)
	{
		if (state == E_DISABLED_INV)
			state = E_DISABLED;
		else
			state = E_ENABLED;
	}
	cur_count = 0;
	return 1;
}

void
Stop_e::invalidate()
{
	if (state == E_ENABLED)
		state = E_INVALID;
	else if (state == E_DISABLED)
		state = E_DISABLED_INV;
}

void
Stop_e::validate()
{
	register StopEvent	*se;

	for(se = stop; se; se = se->next())
	{
		if (!(se->get_flags() & E_VALID))
			return;
	}
	if (state == E_INVALID)
		state = E_ENABLED;
	else if (state == E_DISABLED_INV)
		state = E_DISABLED;
}

Stop_e::Stop_e(int Id, int Lvl, 
	int Quiet, int count, Node *Cmd, EventTable *Et) :
		EVENT(Id, Lvl, Quiet, Cmd, Et)
{
	orig_count = count;
	cur_count = 0;
	state = E_INVALID;
}

void
Stop_e::copy(Stop_e *oldevent)
{
	int		i, invalid = 0;
	StopEvent	*nse, *ose;
	char		*new_expr;

	stop = copy_tree(oldevent->stop);
	new_expr = oldevent->get_expr();
	event_expr = new(char[strlen(new_expr) + 1]);
	strcpy(event_expr, new_expr);
	for(nse = stop, ose = oldevent->stop; nse && ose; 
		nse = nse->next(), ose = ose->next())
	{
		if ((i = nse->stop_copy(etable->lwp, this, ose)) 
			== SET_FAIL)
			break;
		else if (i == SET_INVALID)
			invalid = 1;
	}
	if (i == SET_FAIL)
	{
		// might have set part of stop expression
		for(nse = stop; nse; nse = nse->next())
		{
			// StopEvents deleted in Stop_e destructor
			nse->remove();
		}
		return;
	}
	if (invalid)
		state = E_INVALID;
	else
		state = E_ENABLED;
}

int
Stop_e::disable()
{
	register StopEvent	*se;

	if (state == E_ENABLED)
	{
		state = E_DISABLED;
	}
	else if (state == E_INVALID)
	{
		state = E_DISABLED_INV;
	}
	else
		return 0;
	for(se = stop; se; se = se->next())
	{
		se->disable();
	}
	if (get_ui_type() == ui_gui)
		printm(MSG_event_disabled, id, 
			(unsigned long)etable->lwp);
	return 1;
}

int
Stop_e::enable()
{
	register StopEvent	*se;

	if (state == E_DISABLED)
	{
		state = E_ENABLED;
	}
	else if (state == E_DISABLED_INV)
	{
		state = E_INVALID;
	}
	else
		return 0;
	for(se = stop; se; se = se->next())
	{
		se->enable();
	}
	if (get_ui_type() == ui_gui)
		printm(MSG_event_enabled, id, 
			(unsigned long)etable->lwp);
	return 1;
}

// The StopEvents for a single Stop_e are connected as a list; 
// each node is either a leaf - the end of the list - 
// or one operand of an "and" or "or"
// operator.  Many functions that deal with StopEvents recurse
// down the list.
//
// The list for "a && b || c", for example, looks like
//		c  OR
//		|
//		b  AND
//		|
//		a  LEAF

// Is event true or false?
int 
Stop_e::stop_eval(StopEvent *node)
{
	if (!node)
		return 0;

	if (node->get_flags() & E_LEAF)
	{
		return(node->stop_true());
	}
	else if (node->get_flags() & E_AND)
	{
		if (stop_eval(node->next()))
		{
			return(node->stop_true());
		}
		else
		{
			return 0;
		}
	}
	else // or
	{
		if (stop_eval(node->next()))
		{
			return 1;
		}
		else
		{
			return(node->stop_true());
		}
	}
	/*NOTREACHED*/
}
