#ident	"@(#)debugger:libexecon/common/StopEvent.C	1.8"

#include "Event.h"
#include "Link.h"
#include "LWP.h"
#include "Frame.h"
#include "Expr.h"
#include "Rvalue.h"
#include "Symbol.h"
#include "Symtab.h"
#include "Source.h"
#include "Itype.h"
#include "Language.h"
#include "Location.h"
#include "Breaklist.h"
#include "EventTable.h"
#include "Interface.h"
#include "TriggItem.h"
#include "StopEvent.h"
#include "Buffer.h"
#include "fpemu.h"
#include "utility.h"
#include <string.h>

static void
do_print(StopEvent *node, LWP *l)
{
	char	*p;

	if (!node)
		return;
	do_print(node->next(), l);
	if (node->get_flags() & E_AND)
		gbuf1.add(" && ");
	else if (node->get_flags() & E_OR)
		gbuf1.add(" || ");
	p = node->print(l);
	gbuf1.add(p);
}

char *
print_stop(StopEvent *node, LWP *l)
{
	char	*p;

	if (!node)
		return 0;
	gbuf1.clear();
	do_print(node, l);
	gbuf1.add(' ');
	p = new(char[gbuf1.size()]);
	strcpy(p, (char *)gbuf1);
	return p;
}

void
dispose_event(StopEvent *node)
{
	StopEvent	*tmp;

	while(node)
	{
		tmp = node;
		node = node->next();
		delete(tmp);
	}
}

StopEvent *
copy_tree(StopEvent *otree)
{
	StopEvent	*prev = 0;
	StopEvent	*first = 0;
	StopEvent	*ntree;

	while(otree)
	{
		ntree = otree->copy();
		if (!first)
			first = ntree;
		if (prev)
			prev->append(ntree);
		prev = ntree;
		otree = otree->next();
	}
	return first;
}

StopEvent::StopEvent(int f)
{
	eflags = f;
	evtable = 0;
	_next = 0;
}

// virtual base clase null version
char *
StopEvent::print(LWP *)
{
	return 0;
}

// virtual base class null version
int
StopEvent::remove()
{
	return 0;
}

// virtual base class null version
int
StopEvent::stop_true()
{
	return 0;
}

// virtual base class null version
int
StopEvent::stop_set(LWP *, Stop_e *)
{
	return 0;
}

// virtual base class null version
int
StopEvent::stop_copy(LWP *, Stop_e *, StopEvent *)
{
	return(0);
}

// virtual base class null version
void
StopEvent::cleanup()
{
	return;
}

// virtual base class null version
int
StopEvent::re_init()
{
	return 0;
}

// virtual base class null version
StopEvent *
StopEvent::copy()
{
	return 0;
}

// virtual base class null version
void
StopEvent::disable()
{
	return;
}

// virtual base class null version
void
StopEvent::enable()
{
	return;
}

StopLoc::StopLoc(int f, Location *l) : STOPEVENT(f)
{
	loc = l;
	endlist = 0;
	addr = 0;
	is_func = 0;
}

StopLoc::~StopLoc()
{
	delete loc;
}

StopEvent *
StopLoc::copy()
{
	Location	*l = new Location(*loc);
	StopLoc *newloc = new StopLoc((eflags & NODE_MASK), l);
	return (StopEvent *)newloc;
}

char *
StopLoc::print(LWP *l)
{
	if (!l)
		return(loc->print());
	else
		return(loc->print(l, l->topframe()));
}

int
StopLoc::stop_set(LWP *p, Stop_e *eptr)
{
	Iaddr	loc_addr;
	LWP	*lwp = p;
	long	l;
	int	stopped = 0;
	Symbol	func;

	// get_addr can reset lwp if loc points to foreign LWP
	if ( get_addr( lwp, loc, loc_addr, st_func, func ) == 0 )
	{
		return SET_FAIL;
	}
	if (!lwp->in_text(loc_addr))
	{
		printe(ERR_bkpt_data, E_WARNING, loc_addr, 
			lwp->lwp_name());
	}
	evtable = lwp->events();
	sevent = eptr;

	// if stop requested on function name, go past prolog
	if (loc->get_type() == lk_fcn && (func.tag() != t_label))
	{
		loc->get_offset(l);
		if (l == 0)
		{
			is_func = 1;
			loc_addr = lwp->first_stmt(loc_addr);
		}
	}
	if (lwp != p)
	{
		// foreign
		Execstate state = lwp->get_state();
		if (state == es_stepping || state == es_running)
		{
			if (!lwp->stop())
				return SET_FAIL;
			stopped = 1;
		}
	}
	if ((lwp->set_bkpt( loc_addr, (Notifier)notify_stoploc_trigger,
		this)) == 0)
	{
		return SET_FAIL; 
	}
	if (get_ui_type() == ui_gui)
	{
		// gui wants to know file and line of bkpt
		Symtab	*symtab;
		Symbol	symbol;
		Source	source;
		long	line;

		if (((symtab = lwp->find_symtab( loc_addr )) != 0) &&
			(symtab->find_source( loc_addr, symbol ) != 0)&&
			(symbol.source( source ) != 0)) 
		{
			source.pc_to_stmt( loc_addr, line );
			if (line)
				printm(MSG_bkpt_set, loc_addr, line,
					symbol.name());
			else
				printm(MSG_bkpt_set_addr, loc_addr);
		}
		else
			printm(MSG_bkpt_set_addr, loc_addr);
	}
	eflags |= E_SET;
	if (lwp != p)
	{
		eflags |= E_FOREIGN;
		evtable->add_foreign((Notifier)notify_stop_e_clean_foreign, sevent);
		if (stopped)
		{
			lwp->resume(1);
		}
	}
	addr = loc_addr;
	eflags |= E_VALID;
	return SET_VALID;
}

int
StopLoc::stop_copy(LWP *p, Stop_e *eptr, StopEvent *oldse)
{
	StopLoc	*old = (StopLoc *)oldse;
	LWP	*lwp;
	Endlist	*el;
	int	stopped = 0;

	eflags = old->eflags;
	if (eflags & E_FOREIGN)
		evtable = old->evtable;
	else
		evtable = p->events();
	lwp = evtable->lwp;

	sevent = eptr;
	addr = old->addr;
	is_func = old->is_func;
	if (!eflags & E_SET)
		return SET_VALID;
	if (eflags & E_FOREIGN)
	{
		// foreign
		Execstate state = lwp->get_state();
		if (state == es_stepping || state == es_running)
		{
			if (!lwp->stop())
				return SET_FAIL;
			stopped = 1;
		}
	}
	if ((lwp->set_bkpt( addr, (Notifier)notify_stoploc_trigger,
		this)) == 0)
	{
		return SET_FAIL; 
	}
	if (eflags & E_FOREIGN)
	{
		evtable->add_foreign((Notifier)notify_stop_e_clean_foreign, sevent);
		if (stopped)
		{
			lwp->resume(1);
		}
	}
	if (!is_func)
		return SET_VALID;
	for(el = old->endlist; el; el = el->next())
	{
		Endlist	*elist;
		elist = new Endlist(*el);
		if (!elist->set(lwp))
		{
			delete elist;
			return SET_FAIL;
		}
		if (endlist)
			elist->prepend(endlist);
		endlist = elist;
	}
	return SET_VALID;
}

int
StopLoc::remove()
{
	LWP	*l = evtable->lwp;
	int	stopped = 0;

	if (eflags & E_FOREIGN)
	{
		Execstate state = l->get_state();

		if (state == es_stepping || state == es_running)
		{
			if (!l->stop())
				return 0;
			stopped = 1;
		}
	}
	if (eflags & E_SET)
	{
		if (l)
		{
			if (!l->remove_bkpt(addr, 
				(Notifier)notify_stoploc_trigger, this))
				return 0;
		}
		else
		{
			Breakpoint	*b;
			if ((b = evtable->breaklist.remove(addr,
				(Notifier)notify_stoploc_trigger, this)) == 0)
				return 0;
			if (!b->events())
				if (!evtable->breaklist.remove(addr))
					return 0;
		}
		if (eflags & E_FOREIGN)
			evtable->remove_foreign((Notifier)notify_stop_e_clean_foreign, sevent);
	}
	while(endlist)
	{
		Endlist	*tmp = endlist;
		// only have end breakpoints for live processes
		if (!l->remove_bkpt(endlist->endaddr, 
				(Notifier)notify_endlist_trigger, endlist))
			return 0;
		endlist = endlist->next();
		delete(tmp);
	} 
	if (stopped)
	{
		l->resume(1);
	}
	return 1;
}

int
StopLoc::stop_true()
{
	// If breakpoint was on a function, and the function
	// is active, return true;
	// Otherwise true only if we are at exact breakpoint address

	LWP	*lwp;

	lwp = evtable->lwp;

	if (is_func)
	{
		if (endlist)
			return 1;
		else
			return 0;
	}
	else
	{
		Iaddr	pc;
		Execstate state = lwp->get_state();
		if (state == es_stepping || state == es_running ||
			state == es_dead)
		{
			return 0;
		}
		pc = lwp->pc_value();
		return(addr == pc);
	}
}

int
StopLoc::trigger()
{
	if (is_func)
	{
		// new instance of function - add a breakpoint for its
		// return address
		LWP	*l = evtable->lwp;
		Frame	*f = l->topframe();
		Iaddr	raddr, tmp;
		Endlist	*elist;

		if (f->retaddr(raddr, tmp))
		{
			elist = new Endlist(raddr, this);
			if (!elist->set(l))
			{
				sevent->invalidate(); 
				delete(elist);
				return NO_TRIGGER;
			}
			if (endlist)
				elist->prepend(endlist);
			endlist = elist;
		}
		else
		{
			// can't find return addr - treat like
			// on function address breakpoint
			printe(ERR_return_addr, E_WARNING, 
				l->lwp_name());
			is_func = 0;
		}

	}
	if (eflags & E_FOREIGN)
		return(sevent->trigger_foreign());
	else
		return(sevent->trigger());
}

void
StopLoc::cleanup()
{
	// If a foreign event, remove entirely,
	// else remove breakpoints for endlist
	LWP	*lwp = evtable->lwp;
	int	stopped = 0;

	if (!(eflags & E_SET))
		return;
	if (eflags & E_FOREIGN)
	{
		Execstate state = lwp->get_state();

		if (state == es_stepping || state == es_running)
		{
			if (lwp->stop())
				stopped = 1;
		}
		lwp->remove_bkpt(addr, (Notifier)notify_stoploc_trigger, this);
		evtable->remove_foreign((Notifier)notify_stop_e_clean_foreign, sevent);
		eflags &= ~E_SET;
	}
	while(endlist)
	{
		Endlist	*tmp = endlist;
		lwp->remove_bkpt(endlist->endaddr, 
			(Notifier)notify_endlist_trigger, endlist);
		endlist = endlist->next();
		delete(tmp);
	} 
	if (stopped)
	{
		lwp->resume(1);
	}
	eflags &= ~E_VALID;
}


// if event is in a foreign process, re-initialize breakpoint
int
StopLoc::re_init()
{
	LWP	*lwp;
	int	stopped = 0;

	if (!(eflags & E_FOREIGN))
	{
		eflags |= E_VALID;
		return SET_VALID;
	}
	if (((loc->get_lwp(lwp)) == 0) ||
		!lwp || !evtable || evtable->lwp != lwp)
		return SET_FAIL;

	Execstate state = lwp->get_state();
	if (state == es_stepping || state == es_running)
	{
		if (!lwp->stop())
			return SET_FAIL;
		stopped = 1;
	}
	if ((lwp->set_bkpt( addr, (Notifier)notify_stoploc_trigger, this)) == 0)
	{
		if (stopped)
		{
			lwp->resume(1);
		}
		return SET_FAIL; 
	}
	evtable->add_foreign((Notifier)notify_stop_e_clean_foreign, sevent);
	if (stopped)
	{
		lwp->resume(1);
	}
	eflags |= (E_SET|E_VALID);
	return SET_VALID;
}

int
Endlist::set(LWP *lwp )
{
	return((lwp->set_bkpt( endaddr, 
		(Notifier)notify_endlist_trigger, this)) != 0);
}

int
Endlist::trigger()
{
	// Hit breakpoint marking return address of function;
	// Remove the breakpoint and delete this entry from
	// the StopLoc's endlist

	LWP	*lwp = sloc->etable()->lwp;
	Endlist	*elist = sloc->end_list();

	if (elist == this)
		sloc->set_list(next());
	unlink();
	delete(this);
	// bkpt removed by LWP::respond_to_bkpt
	return NO_TRIGGER_RM;
}

StopExpr::StopExpr(int f, char *e) : STOPEVENT(f)
{
	exp_str = e;
	expr = 0;
	data = 0;
}

StopEvent *
StopExpr::copy()
{
	StopExpr *newexpr = new StopExpr((eflags & NODE_MASK), exp_str);
	return (StopEvent *)newexpr;
}

int
StopExpr::eval_expr(FrameId &id)
{
	LWP	*lwp = evtable->lwp;
	Frame	*f = lwp->topframe();

	if (!expr)
		return 0;

	if (!id.isnull())
	{
		for(; f; f = f->caller())
		{
			if (id == f->id())
				break;
		}
	}
	if (!f)
		return 0;

	return expr->eval(lwp, lwp->pc_value(), f);
}

int
StopExpr::stop_set(LWP *l, Stop_e *e)
{
	TriggerItem	*item;
	WatchData	*wd;
	int		invalid = 0;
	Iaddr		pc = l->pc_value();
	Frame		*f = l->topframe();

	expr = new Expr(exp_str, l, 1);
	if (!expr->eval(l, l->pc_value(), f))
	{
		delete expr;
		expr = 0;
		printe(ERR_stop_expr, E_ERROR, l->lwp_name());
		return SET_FAIL;
	}
	expr->triggerList(l, pc, triglist);
	sevent = e;
	evtable = l->events();

	if (triglist.isempty())
	{
		eflags |= E_VALID;
		return SET_VALID;
	}

	for (item = (TriggerItem *)triglist.first(); item;
		item = (TriggerItem *)triglist.next())
	{
		int	i;
		wd = new WatchData(item);
		i = wd->stop_expr_set(l, this);
		if (i == SET_FAIL)
		{
			delete wd;
			return SET_FAIL;
		}
		else if (i == SET_INVALID)
			invalid = 1;
		if (data)
			wd->append(data);
		data = wd;
	}
	if (!invalid)
	{
		eflags |= E_VALID;
		if (!(eflags & E_TRIG_ON_CHANGE) && expr->exprIsTrue(l, f))
			eflags |= E_TRUE;
		return SET_VALID;
	}
	else
		return SET_INVALID;
}

int
StopExpr::stop_copy(LWP *l, Stop_e *eptr, StopEvent *oldse)
{
	Frame		*f = l->topframe();
	StopExpr	*old = (StopExpr *)oldse;
	WatchData	*stop;
	TriggerItem	*item;

	sevent = eptr;
	evtable = l->events();
	eflags = old->eflags;
	expr = old->expr->copyEventExpr(old->triglist, triglist, l);

	if (triglist.isempty())
	{
		eflags |= E_VALID;
		return SET_VALID;
	}

	for(stop = old->data, item = (TriggerItem *)triglist.first(); 
		stop && item ; stop = stop->next(), 
		item = (TriggerItem *)triglist.next())
	{
		WatchData	*wd = new WatchData(item);
		if (wd->stop_expr_copy(l, this, stop) == SET_FAIL)
		{
			delete wd;
			return SET_FAIL;
		}
		if (data)
			wd->append(data);
		data = wd;
	}
	if (stop || item)
		return SET_FAIL;
	return((eflags & E_VALID) ? SET_VALID : SET_INVALID);
}

void
StopExpr::validate()
{
	WatchData	*wd = data;
	for (; wd; wd = wd->next())
	{
		if (!(wd->get_flags() & E_VALID))
		{
			eflags &= ~E_VALID;
			sevent->invalidate();
			return;
		}
	}
	eflags |= E_VALID;
	sevent->validate();
}

void
StopExpr::invalidate()
{
	eflags &= ~E_VALID;
	sevent->invalidate();
}

// recalculate all lvalues and rvalues if part of an expression
// changes that could affect another part;
// for example, in x->i, if the value of x changes, it changes
// the actual address of i
int
StopExpr::recalc(WatchData *orig, FrameId &id)
{
	WatchData	*wd = data;
	for (; wd; wd = wd->next())
	{
		if (wd != orig)
		{
			// don't need to recalc the triggerItem that caused
			// us to recalc in the first place
			if (!wd->recalc(id))
			{
				return 0;
			}
		}
	}
	return 1;
}

char *
StopExpr::print(LWP *)
{
	if (!(eflags & E_TRIG_ON_CHANGE))
		return exp_str;
	char *p = new(char[strlen(exp_str) + sizeof("*")]);
	p[0] = '*';
	strcpy(p+1, exp_str);
	return p;
}

void
StopExpr::disable()
{
	WatchData	*wd = data;
	for (; wd; wd = wd->next())
	{
		wd->disable();
	}
}

void
StopExpr::enable()
{
	WatchData	*wd = data;
	for (; wd; wd = wd->next())
	{
		wd->enable();
	}
}
int
StopExpr::remove()
{
	WatchData	*tmp;

	while(data)
	{
		data->remove();
		tmp = data;
		data = data->next();
		delete tmp;
	}
	delete expr;
	expr = 0;
	return 1;
}

// Event is true if E_TRIG_ON_CHANGE is set and one of the data
// items has changed, or the expression evaluates to true.
int
StopExpr::stop_true()
{

	if ((eflags & (E_VALID|E_TRUE)) == (E_VALID|E_TRUE))
	{
		eflags &= ~E_TRUE;
		return 1;
	}
	else if (eflags & E_TRIG_ON_CHANGE)
		return 0;
	else
	{
		Frame	*f = evtable->lwp->topframe();
		if (!expr)
			return 0;
		if (!eval_expr(f->id()))
			return 0;
		return(expr->exprIsTrue(evtable->lwp, f));
	}
}

// A data item has changed.  If E_TRIG_ON_CHANGE is
// set we always trigger. Otherwise, we trigger only
// if event evaluates to true.
int
StopExpr::trigger(int foreign)
{
	Frame	*f = evtable->lwp->topframe();

	if ((eflags & E_TRIG_ON_CHANGE) ||
		(expr->exprIsTrue(evtable->lwp, f)))
	{
		eflags |= E_TRUE;
		if (foreign)
			return(sevent->trigger_foreign());
		else
			return(sevent->trigger());
	}
	else
	{
		eflags &= ~E_TRUE;
		return NO_TRIGGER;
	}
}

int
StopExpr::re_init()
{
	WatchData	*d = data;
	LWP		*l = evtable->lwp;
	Frame		*f = l->topframe();
	int		invalid = 0;

	while(d)
	{
		int	i;
		i = d->re_init();
		if (i == SET_FAIL)
			return SET_FAIL;
		else if (i == SET_INVALID)
			invalid = 1;
		d = d->next();
	}
	if (!invalid)
	{
		eflags |= E_VALID;
		if (!(eflags & E_TRIG_ON_CHANGE) && expr->exprIsTrue(l, f))
			eflags |= E_TRUE;
		return SET_VALID;
	}
	else
	{
		return SET_INVALID;
	}
}

void
StopExpr::cleanup()
{
	WatchData	*d = data;

	while(d)
	{
		d->cleanup();
		d = d->next();
	}
	eflags &= ~E_VALID;
	eflags &= ~E_TRUE;
}


WatchData::WatchData(TriggerItem *i)
{
	item = i;
	watch = 0;
	sexpr = 0;
	evtable = 0;
	flags = 0;
	_nxt = 0;
}


int
WatchData::getTriggerLvalue(Place &lval)
{
	if (!item)
		return 0;
	return item->getTriggerLvalue(lval);
}

int
WatchData::getTriggerRvalue(Rvalue &rval)
{
	if (!item)
		return 0;
	return item->getTriggerRvalue(rval);
}

int
WatchData::stop_expr_set(LWP *l, StopExpr *se)
{
	sexpr = se;
	int	i;

	evtable = item->lwp->events();
	if (item->lwp != l)
	{
		flags |= E_FOREIGN;
		evtable->add_foreign((Notifier)notify_stop_e_clean_foreign,
			sexpr->event());
	}
	watch = new Watchframe(this, item->scope, item->frame);
	if ((i = watch->init()) == SET_FAIL)
	{
		delete watch;
		watch = 0;
		return SET_FAIL;
	}
	else if (i == SET_VALID)
		flags |= E_VALID;
	return i;
}


int
WatchData::stop_expr_copy(LWP *l, StopExpr *oldexpr, WatchData *old)
{
	Watchframe	*wf;

	sexpr = oldexpr;

	flags = old->flags;
	if (flags & E_FOREIGN)
	{
		evtable = item->lwp->events();
		evtable->add_foreign((Notifier)notify_stop_e_clean_foreign,
			sexpr->event());
	}
	else
		evtable = l->events();
	for(wf = old->watch; wf; wf = wf->next())
	{
		Watchframe	*nwf = new Watchframe(this, wf->addr, wf->frame);
		if (!nwf->copy(wf))
		{
			delete nwf;
			return SET_FAIL;
		}
		if (watch)
			nwf->append(watch);
		watch = nwf;
	}
	return((flags & E_VALID) ? SET_VALID : SET_INVALID);
}


int
WatchData::remove()
{
	LWP		*lwp = evtable->lwp;
	Watchframe	*tmp;
	int		stopped = 0;

	Execstate estate = lwp->get_state();
	if (estate == es_stepping ||
		estate == es_running)
	{
		if (!lwp->stop())
			return 0;
		stopped = 1;
	}
	if (flags & E_FOREIGN)
		evtable->remove_foreign((Notifier)notify_stop_e_clean_foreign,
			sexpr->event());
	while(watch)
	{
		watch->remove();
		tmp = watch;
		watch = watch->next();
		delete tmp;
	}
	if (stopped)
		lwp->resume(1);
	return 1;
}

int
WatchData::re_init()
{
	int	i;
	LWP	*lwp;
	int	stopped = 0;


	if (flags & E_FOREIGN)
	{
		lwp = item->lwp;
		if (!lwp || !evtable || evtable->lwp != lwp)
			return SET_FAIL;
		Execstate state = lwp->get_state();
		if (state == es_stepping || state == es_running)
		{
			if (!lwp->stop())
				return SET_FAIL;
			stopped = 1;
		}
	}
	if (!watch || !sexpr ||
		((i = watch->init()) == SET_FAIL))
	{
		delete watch;
		watch = 0;
		sexpr->event()->remove();
		return SET_FAIL;
	}
	else if (i == SET_VALID)
		flags |= E_VALID;
	if (flags & E_FOREIGN)
		evtable->add_foreign((Notifier)notify_stop_e_clean_foreign,
			sexpr->event());
	if (stopped)
		lwp->resume(1);
	return i;
}

void
WatchData::cleanup()
{
	LWP		*lwp = evtable->lwp;
	int		stopped = 0;

	if (flags & E_FOREIGN)
	{
		evtable->remove_foreign((Notifier)notify_stop_e_clean_foreign,
			sexpr->event());
	}

	Execstate estate = lwp->get_state();
	if (estate == es_stepping ||
		estate == es_running)
	{
		if (!lwp->stop())
			return;
		stopped = 1;
	}
	// remove all but initial Watchframe
	Watchframe	*wf = watch;
	while(wf)
	{
		Watchframe	*tmp;

		tmp = wf;
		wf = wf->next();
		tmp->remove();	
		if ((tmp->addr != 0) && (tmp->addr != (Iaddr)-1))
		{
			if (tmp->state == S_START)
			{
				// breakpt on start of function - save
				tmp->unlink();
				watch = tmp;
			}
			else
			{
				if (watch == tmp)
					watch = watch->next();
				tmp->unlink();
				delete tmp;
			}
		}

	}
	if (stopped)
		lwp->resume(1);
}


void
WatchData::validate()
{
	flags |= E_VALID;
	sexpr->validate();
}

void
WatchData::invalidate()
{
	flags &= ~E_VALID;
	sexpr->invalidate();
}

void
WatchData::enable()
{
	Watchframe	*wf;
	LWP		*l = evtable->lwp;
	if (!l)
		return;
	for(wf = watch; wf; wf = wf->next())
	{
		if (wf->state == S_SOFT)
			l->enable_soft();
	}
}

void
WatchData::disable()
{
	Watchframe	*wf;
	LWP		*l = evtable->lwp;
	if (!l)
		return;
	for(wf = watch; wf; wf = wf->next())
	{
		if (wf->state == S_SOFT)
			l->disable_soft();
	}
}

// recalculate lvalues and rvalues if a trigger item we depend on
// changes value
int
WatchData::recalc(FrameId &id)
{
	Watchframe	*wf;

	for(wf = watch; wf; wf = wf->next())
	{
		if (wf->frame == id)
		{
			return wf->recalc();
		}
	}
	return 0;
}

Watchframe::Watchframe(WatchData *wd, Iaddr pc, FrameId &f)
{
	event = wd;
	addr = pc;
	last = 0;
	state = S_NULL;
	place = (Iaddr)-1;
	endscope = (Iaddr)-1;
	frame = f;
}

int
Watchframe::init()
{
	int	i;
	LWP	*lwp = event->evtable->lwp;
	Frame	*f = lwp->topframe();
	int	stopped = 0;
	Place	lval;
	Rvalue	rval;


	if (event->flags & E_FOREIGN)
	{
		Execstate estate = lwp->get_state();
		if (estate == es_stepping ||
			estate == es_running)
		{
			if (!lwp->stop())
				return SET_FAIL;
			stopped = 1;
		}
	}
	if ((addr != 0) && (addr != (Iaddr)-1))
	{
		// on stack, set up breakpoint for its start addr

		Iaddr	oaddr, pc;
		Symbol	entry;
		// for now, pc's for inner scope autos are
		// pc of inner scope, not function

		if  (state == S_START && endscope != (Iaddr)-1)
		{
			// re-initialize existing event - don't
			// need to recalculate scope
			oaddr = addr;
		}
		else
		{
			if (frame.isnull())
				frame = f->id();
			entry = lwp->find_entry(addr);
			if (entry.isnull())
			{
				if (stopped)
					lwp->resume(1);
				printe(ERR_expr_scope, E_ERROR, 
					lwp->lwp_name());
				return SET_FAIL;
			}
			addr = entry.pc(an_lopc);
			endscope = entry.pc(an_hipc);
			oaddr = addr;
			addr = lwp->first_stmt(addr);
			state = S_START;
		}
		if (!lwp->set_bkpt(addr, 
			(Notifier)notify_watchframe_start, this))
		{
			state = S_NULL;
			if (stopped)
				lwp->resume(1);
			return SET_FAIL;
		}
		pc = lwp->pc_value();
		if ((pc >= oaddr) && (pc <= endscope))
		{
			// we are in scope for this item, start watch
			trigger_start();
			if (stopped)
				lwp->resume(1);
			return SET_VALID;
		}
		if (stopped)
			lwp->resume(1);
		return SET_INVALID;
	}
	// not on stack, start watching
	frame.null();
	event->sexpr->eval_expr(frame);
	if ((!event->getTriggerRvalue(rval)) ||
		(!event->getTriggerLvalue(lval)))
	{
		if (stopped)
			lwp->resume(1);
		printe(ERR_stop_expr, E_ERROR, lwp->lwp_name());
		return SET_FAIL;
	}
	last = new Rvalue(rval);
	if (lval.kind == pAddress)
		place = lval.addr;
	i = lwp->set_watchpoint(place, 
		last, (Notifier)notify_watchframe_watch, this);
	if (i == WATCH_FAIL)
	{
		if (stopped)
			lwp->resume(1);
		return SET_FAIL;
	}
	else if (i == WATCH_HARD)
		state = S_HARD;
	else
		state = S_SOFT;
	if (stopped)
		lwp->resume(1);
	return SET_VALID;
}

// hit breakpoint at start of bracketing automatic watchpoint
int
Watchframe::trigger_start()
{
	Iaddr		raddr, pc, tmp;
	Frame		*f;
	Watchframe	*wf;
	LWP		*lwp = event->evtable->lwp;
	TriggerItem	*item = event->item;
	Rvalue		rval;
	Place		lval;
	int		i;

	pc = lwp->pc_value();
	f = lwp->topframe();
	if (!f->retaddr(raddr, tmp))
	{
		printe(ERR_return_addr, E_ERROR, lwp->lwp_name());
		event->sexpr->event()->invalidate();
		return NO_TRIGGER;
	}
	wf = new Watchframe(event, raddr, f->id());
	if (!lwp->set_bkpt(raddr, (Notifier)notify_watchframe_end, wf))
	{
		delete wf;
		event->sexpr->event()->invalidate();
		return NO_TRIGGER;
	}

	event->sexpr->eval_expr(wf->frame);
	if ((!event->getTriggerRvalue(rval)) ||
		(!event->getTriggerLvalue(lval)))
	{

		lwp->remove_bkpt(raddr, (Notifier)notify_watchframe_end, wf);
		delete wf;
		event->sexpr->event()->invalidate();
		return NO_TRIGGER;
	}
	wf->last = new Rvalue(rval);
	wf->endscope = endscope;
	if (lval.kind == pAddress)
	{
		wf->place = lval.addr;
		if (lwp->in_text(lval.addr))
		{
			
			printe(ERR_watch_text, E_WARNING, lval.addr, 
				lwp->lwp_name());
		}
	}
	i = lwp->set_watchpoint(wf->place, 
		wf->last, (Notifier)notify_watchframe_watch, wf);
	if (i == WATCH_FAIL)
	{
		lwp->remove_bkpt(raddr, (Notifier)notify_watchframe_end, wf);
		delete wf;
		event->sexpr->event()->invalidate();
		return NO_TRIGGER;
	}
	else if (i == WATCH_HARD)
		wf->state = S_HARD;
	else
		wf->state = S_SOFT;
	wf->prepend(event->watch);
	event->watch = wf;
	event->validate();
	return NO_TRIGGER;
}

int
Watchframe::trigger_end()
{
	LWP		*lwp = event->evtable->lwp;
	TriggerItem	*item = event->item;

	lwp->remove_watchpoint((state == S_HARD), place, 
		(Notifier)notify_watchframe_watch, this);
	if (event->watch == this)
		event->watch = next();
	if (!event->watch)
		event->invalidate();
	unlink();
	delete(this);
	// bkpt removed by LWP::respond_to_bkpt
	return NO_TRIGGER_RM;
}

int
Watchframe::trigger_watch()
{
	if (changed())
	{
		if (event->item->reinitOnChange())
		{
			if (!event->sexpr->recalc(event, frame))
			{
				event->sexpr->event()->invalidate();
				return NO_TRIGGER;
			}
		}
		return event->sexpr->trigger(event->flags & E_FOREIGN);
	}
	else
	{
		return NO_TRIGGER;
	}
}

// recalculate lvalue and rvalue because of change in some other
// trigger item we depend on
int
Watchframe::recalc()
{
	Rvalue	rval;
	Place	lval;
	LWP	*lwp = event->evtable->lwp;
	int	i;

	event->sexpr->eval_expr(frame);
	if (!event->getTriggerLvalue(lval))
		return 0;
	if ((lval.kind == pAddress) && (lval.addr == place))
			return 1; // no change
	if (!event->getTriggerRvalue(rval))
	{
		return 0;
	}
	if (!lwp->remove_watchpoint((state == S_HARD), place,
		(Notifier)notify_watchframe_watch, this))
		return 0;
	place = lval.addr;
	last = new Rvalue(rval);
	i = lwp->set_watchpoint(place, last, 
		(Notifier)notify_watchframe_watch, this);
	if (i == WATCH_FAIL)
		return 0;
	else if (i == WATCH_HARD)
		state = S_HARD;
	else
		state = S_SOFT;
	return 1;
}

int
Watchframe::changed()
{
	Itype		nitype, oitype;
	Stype		nstype, ostype;
	Rvalue		rval;
	int		diff = 0;
	LWP		*lwp = event->evtable->lwp;
	TriggerItem	*item = event->item;

	if (!event->sexpr->eval_expr(frame))
		return 0;
	if (!event->getTriggerRvalue(rval))
	{
		event->sexpr->event()->invalidate();
		return 0;
	}

	if (((ostype = last->get_Itype(oitype)) == SINVALID)
		|| ((nstype = rval.get_Itype(nitype)) == SINVALID)
		|| (ostype != nstype))
	{
		printe(ERR_stop_expr, E_ERROR, lwp->lwp_name());
		return 0;
	}
	// has value changed?
	switch(nstype) 
	{
	case Schar:	
		diff = (nitype.ichar != oitype.ichar); 
		break;
	case Sint1:	
		diff = (nitype.iint1 != oitype.iint1); 
		break;
	case Sint2:	
		diff = (nitype.iint2 != oitype.iint2); 
		break;
	case Sint4:	
		diff = (nitype.iint4 != oitype.iint4); 
		break;
	case Suchar:	
		diff = (nitype.iuchar != oitype.iuchar); 
		break;
	case Suint1:	
		diff = (nitype.iuint1 != oitype.iuint1); 
		break;
	case Suint2:	
		diff = (nitype.iuint2 != oitype.iuint2);
		break;
	case Suint4:	
		diff = (nitype.iuint4 != oitype.iuint4); 
		break;
	case Ssfloat:	
		diff = (nitype.isfloat != oitype.isfloat); 
		break;
	case Sdfloat:	
		diff = (nitype.idfloat != oitype.idfloat); 
		break;
	case Sxfloat:	
		diff = (fp_compare(nitype.ixfloat, oitype.ixfloat) != 0);
		break;
	case Saddr:	
		diff = (nitype.iaddr != oitype.iaddr);
		break;
	case Sdebugaddr:	
		diff = (nitype.idebugaddr != oitype.idebugaddr);
		break;
	case Sbase:	
		diff = (nitype.ibase != oitype.ibase); 
		break;
	case Soffset:	
		diff = (nitype.ioffset != oitype.ioffset); 
		break;
	case SINVALID:
	default:
		printe(ERR_internal, E_ERROR, "Watchframe::changed", __LINE__);
		return 0;
	}
	if (diff)
	{
		delete last;
		last = new Rvalue(rval);
	}
	return diff;
}

int
Watchframe::remove()
{
	int	fail = 0;
	LWP	*lwp = event->evtable->lwp;

	if ((addr != 0 ) && (addr != (Iaddr)-1))
	{
		if (!lwp->remove_bkpt(addr,((state == S_START) ?
			((Notifier)notify_watchframe_start) :
			((Notifier)notify_watchframe_end)) , this))
			fail = 1;
	}
	if (state != S_START)
	{
		if (!lwp->remove_watchpoint((state == S_HARD), place,
			(Notifier)notify_watchframe_watch, this))
				fail = 1;
		delete last;
		last = 0;
	}
	return(fail == 0);
}

int
Watchframe::copy(Watchframe *old)
{
	int	stopped = 0;
	int	set = 0;
	int	ok = 1;
	int	i;
	LWP	*lwp = event->evtable->lwp;

	state = old->state;
	place = old->place;
	endscope = old->endscope;

	if (event->flags & E_FOREIGN)
	{
		Execstate estate = lwp->get_state();
		if (estate == es_stepping ||
			estate == es_running)
		{
			if (!lwp->stop())
				return 0; // MORE
			stopped = 1;
		}
	}
	if ((addr != 0) && (addr != (Iaddr)-1))
	{
		// on stack, set up breakpoint
		if (!lwp->set_bkpt(addr, ((state == S_START) ?
			((Notifier)notify_watchframe_start) :
			((Notifier)notify_watchframe_end)) , this))
			ok = 0;
		set = 1;
	}
	if (state != S_START && ok)
	{
		last = new Rvalue(*(old->last));
		i = lwp->set_watchpoint(place, 
			last, (Notifier)notify_watchframe_watch, this);
		if (i == WATCH_FAIL)
		{
			if (set)
				lwp->remove_bkpt(addr, (Notifier)notify_watchframe_end, this);
			ok = 0;
		}
		else if (i == WATCH_HARD)
			state = S_HARD;
		else
			state = S_SOFT;
	}
	if (stopped)
		lwp->resume(1);
	return ok;
}
