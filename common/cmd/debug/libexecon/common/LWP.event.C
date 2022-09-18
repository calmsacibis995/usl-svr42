#ident	"@(#)debugger:libexecon/common/LWP.event.C	1.2.1.1"

#include "Event.h"
#include "LWP.h"
#include "EventTable.h"
#include "Symtab.h"
#include "Source.h"
#include "Flags.h"
#include "Interface.h"
#include "Machine.h"
#include "List.h"
#include "Procctl.h"
#include "Seglist.h"
#include "Watchlist.h"

#include <sys/procfs.h>
#include <sys/fault.h>

int
LWP::set( Breakpoint & b, Iaddr addr )
{
	if ( IS_INSERTED( b._flags ) )
	{
		if (!lift_bkpt( &b ))
			return 0;
	}
	if ( addr == 0 )
	{
		REMOVE( b._flags );
		b._addr = 0;
		return 1;
	}
	else
	{
		b._addr = addr;
		return insert_bkpt( &b );
	}
}

// remove breakpoints used internally in LWP class
// (as opposed to those in the EventTable)
int
LWP::remove( Breakpoint & b )
{
	if ( IS_INSERTED( b._flags ) )
	{
		if (!lift_bkpt( &b ))
			return 0;
	}
	b._addr = 0;
	return 1;
}

// Use an event table that might have been part
// of an earlier process instance that died.
// Re-initializes events
int
LWP::use_et( EventTable * e )
{
	Breakpoint	*b;
	Event		*eptr;


	if ((e == 0) ||
		( etable != 0 ) ||
		( e->lwp != 0 ) ||
		( state == es_corefile ))
	{
		printe(ERR_internal, E_ERROR, "LWP::use_et", __LINE__);
		return 0;
	}
	if ( state == es_corefile )
	{
		etable = e;
		e->lwp = this;
		return 1;
	}
	if ( pctl->trace_sigs(e->siglist.sigset()) == 0 )
	{
		printe(ERR_no_signals, E_ERROR, lwp_name());
		return 0;
	}
	if (flags & L_IGNORE_FORK)
	{
		if (!e->tsclist.events(SYS_fork, 1))
			prdelset(e->tsclist.tracemask(1), SYS_fork);
		// always trap vfork so we can sanely release child
	}
	else 
	{
		// might have been deleted by an earlier process
		// that used this event table
		praddset(e->tsclist.tracemask(1), SYS_fork);
		praddset(e->tsclist.tracemask(1), SYS_vfork);
	}
	if (( pctl->sys_entry(e->tsclist.tracemask(0)) == 0) ||
		( pctl->sys_exit(e->tsclist.tracemask(1)) == 0))
	{
		printe(ERR_no_syscalls, E_ERROR, lwp_name());
		return 0;
	}
	etable = e;
	e->lwp = this;
	e->prog = _process->_program;

	b = etable->breaklist.first();
	if (!insert_all_bkpt(b))
		return 0;

	firstevent = etable->firstevent;

	for(eptr = firstevent; eptr; eptr = eptr->next())
	{
		if (!eptr->re_init())
		{
			printe(ERR_event_reset, E_WARNING,
				eptr->get_id(), lwp_name());
		}
	}

	return 1;
}

// Remove all process events and trace flags for released
// processes, dead or killed processes and processes that exec
int
LWP::cleanup_et(int mode, int delete_events)
{
	Breakpoint 	*b;

	if (mode == P_RELEASE)
	{
		sig_ctl		no_sigs;
		sys_ctl		no_tsc;
		flt_ctl		no_faults;

		premptyset( &no_sigs.signals );
		premptyset( &no_tsc.scalls );
		premptyset( &no_faults.faults );

		if (etable == 0)
		{
			return 1;
		}
		if ( pctl->trace_sigs(&no_sigs) == 0 )
		{
			printe(ERR_no_signals, E_ERROR, lwp_name() );
			return 0;
		}
		if ( (pctl->sys_entry(&no_tsc) == 0 ) ||
			(pctl->sys_exit( &no_tsc) == 0 ))
		{
			printe(ERR_no_syscalls, E_ERROR, lwp_name() );
			return 0;
		}
		if ( pctl->trace_traps(&no_faults) == 0 )
		{
			printe(ERR_sys_no_faults, E_ERROR, lwp_name() );
			return 0;
		}
		if ((remove(destpt) == 0) ||
			(remove(hoppt) == 0) ||
			(remove(dynpt) == 0))
			return 0;
	}
	else if (mode == P_EXEC)
	{

		REMOVE(destpt._flags);
		REMOVE(hoppt._flags);
		REMOVE(dynpt._flags);
	}

	if (hw_watch)
	{
		delete(hw_watch);
		hw_watch = 0;
	}

	if (delete_events)
	{
		Event	*eptr = firstevent;
		for(; eptr; eptr = eptr->next())
		{
			m_event.drop_event(eptr);
		}
		etable->object = 0;
	}
	else
	{
		Event	*eptr = firstevent;

		while(eptr)
		{
			Event	*tmp = eptr->next();
			if (eptr->get_level() < E_PROGRAM)
			{
				if (eptr == firstevent)
					firstevent = tmp;
				eptr->unlink();
			}
			eptr->cleanup(E_PROGRAM);
			eptr = tmp;
		}
		etable->firstevent = firstevent;
	}
	if (etable->foreignlist)
	{
		NotifyEvent	*ne = etable->foreignlist;
		// invalidate and remove foreign events
		while(ne)
		{
			NotifyEvent	*tmp = ne;
			(*ne->func)(ne->thisptr);
			ne = ne->next();
			delete(tmp);
		}
	}

	b = etable->breaklist.first();
	if (mode == P_RELEASE)
	{
		if (!lift_all_bkpt(b, 0))
			return 0;
		if ( state == es_breakpoint )
			state = es_suspended;
	}
	else
	{
		remove_all_bkpt(b);
	}

	firstevent = 0;
	etable = dispose_et( etable );
	return 1;
}


// copy event tables for thread create or fork
int
LWP::copy_et(LWP *olwp, int mode)
{
	Event		*eptr;
	EventTable	*oe = olwp->etable;
	int		changed	= 0;
	Breakpoint	*b;

	etable = new EventTable;
	etable->object = oe->object;
	etable->lwp = this;
	etable->prog = _process->_program;

	// delete ignored signals
	for (int i = 1; i < NSIG; i++)
	{
		if (!prismember(&(oe->siglist.sigset()->signals), i))
		{
			changed++;
			prdelset(&(etable->siglist.sigset()->signals), i);

		}
	}
	if (changed)
	{
		if (!pctl->trace_sigs( etable->siglist.sigset()))
		{
			delete etable;
			printe(ERR_no_signals, E_ERROR, lwp_name() );
			return 0;
		}
	}
	hoppt.copy(olwp->hoppt);
	destpt.copy(olwp->destpt);
	dynpt.copy(olwp->dynpt);

	b = oe->breaklist.first();

	// may be overkill, but trying to avoid incorrect state
	if (!lift_all_bkpt(b, 1))
		return 0;

	// copy each event of appropriate type
	for (eptr = olwp->firstevent; eptr; eptr = eptr->next())
		if (mode & eptr->get_level())
			if (!m_event.copy(eptr, etable, this))
				return 0;
	return 1;
}

// set up to trap single steps and breakpoints
int
LWP::default_traps()
{
#if STOP_TYPE == PR_SIGNALLED
	return 1;
#else
	flt_ctl	fs;

	premptyset(&fs.faults);
	praddset(&fs.faults, TRACEFAULT);  // trace trap - for single step
	praddset(&fs.faults, BKPTFAULT); // machine specific trap for brkpt

	if (!pctl->trace_traps(&fs))
	{
		printe(ERR_sys_no_faults, E_ERROR, lwp_name() );
		return 0;
	}
	return 1;
#endif
}

int
LWP::insert_bkpt( Breakpoint * b )
{
	if (( b == 0 ) ||
		( IS_INSERTED(b->_flags) ) ||
		( IS_DISABLED(b->_flags) ) )
	{
		printe(ERR_internal, E_ERROR, "LWP::insert_bkpt", __LINE__);
		return 0;
	}
	if ((pctl->read( b->addr(), b->oldtext(), BKPTSIZE) != BKPTSIZE) ||
		(pctl->write( b->addr(), BKPTTEXT, BKPTSIZE) != BKPTSIZE))
	{
		printe(ERR_sys_no_breakpt, E_ERROR, b->addr(), lwp_name());
		return 0;
	}
	INSERT( b->_flags );
	return 1;
}

int
LWP::lift_bkpt( Breakpoint * b, int other_lwp )
{
	if (( b == 0 ) ||
		( !IS_INSERTED(b->_flags) ))
	{
		printe(ERR_internal, E_ERROR, "LWP::lift_bkpt", __LINE__);
		return 0;
	}
	if (( state != es_dead ) &&
		( pctl->write( b->addr(), b->oldtext(), 
			BKPTSIZE) != BKPTSIZE ))
	{
		printe(ERR_sys_breakpt, E_ERROR, b->addr(), lwp_name() );
		return 0;
	}
	// don't reset state if lifting breakpoint in newly forked process
	if (!other_lwp)
		REMOVE( b->_flags );
	return 1;
}

// If user sets breakpoint at one of our internally maintained 
// addresses, we must be careful to maintain our internal versions.
Breakpoint *
LWP::set_bkpt( Iaddr addr, Notifier func, void *thisptr)
{
	Breakpoint	*b;
	int		hpthere,dpthere;

	if ( etable == 0 )
	{
		printe(ERR_internal, E_ERROR, "LWP::set_bkpt", __LINE__);
		return 0;
	}
	if ( state == es_corefile )
	{
		printm(ERR_invalid_op_core, E_ERROR, procname);
		return 0;
	}
	b = etable->breaklist.add( addr, func, thisptr );
	if (hpthere = (addr == hoppt.addr())) 
	{
		if (remove( hoppt )==0) 
			return 0;
	}
	if (dpthere = (addr == destpt.addr())) 
	{
		if (remove( destpt )==0)
			return 0;
	}
	if ( !IS_INSERTED(b->_flags) && (insert_bkpt(b) == 0) )
	{
		return 0;
	}
	if ( dpthere && (set(destpt,addr) == 0) )
	{
		return 0;
	}
	if ( hpthere && (set(hoppt,addr) == 0) )
	{
		return 0;
	}

	// set bkpt on current location; don't want to 
	// hit it immediately - by setting state to es_breakpoint
	// we will step over it.
	if (addr == pc)
	{
		state = es_breakpoint;
		latestbkpt = b;
	}

	return b;
}

// If user removes breakpoint at one of our internally maintained 
// addresses, we must be carefult to maintain our internal versions.
int
LWP::remove_bkpt( Iaddr addr, Notifier func, void *thisptr)
{
	int		dpthere, hpthere;
	Breakpoint	*b;

	if ( etable == 0 )
	{
		printe(ERR_internal, E_ERROR, "LWP::remove_bkpt", __LINE__);
		return 0;
	}

	// still events active for this breakpoint?
	if ((b = etable->breaklist.remove( addr, func, thisptr )) == 0)
		return 0;

	if (b->_events)
		return 1;

	if (hpthere = (addr == hoppt.addr())) 
	{
		if (remove( hoppt )==0) 
			return 0;
	}
	if (dpthere = (addr == destpt.addr())) 
	{
		if (remove( destpt )==0)
			return 0;
	}
	if ( (state != es_corefile) && (IS_INSERTED(b->_flags)) &&
		  (lift_bkpt(b) == 0) )
	{
		return 0;
	}

	if ( (state == es_breakpoint) && (pc == addr))
	{
		state = es_suspended;
	}


	if ( dpthere && (set(destpt,addr) == 0) )
	{
		return 0;
	}
	if ( hpthere && (set(hoppt,addr) == 0) )
	{
		return 0;
	}
	if ( etable->breaklist.remove( addr ) == 0 )
	{
		printe(ERR_internal, E_ERROR, "LWP::remov_bkpt", __LINE__);
		return 0;
	}
	return 1;
}


int
LWP::set_sig_catch( sigset_t sigs, Notifier func, void *thisptr )
{
	sig_ctl		*nsigs;
	sig_ctl		osigs;

	osigs.signals = sigs;

	if (etable == 0)
	{
		printe(ERR_internal, E_ERROR, "LWP::set_sig_catch", __LINE__);
		return 0;
	}

	// siglist.add returns 1 if the addition changed the current
	// mask
	if (etable->siglist.add(&osigs, func, thisptr))
	{
		nsigs = etable->siglist.sigset();
		if (!pctl->trace_sigs( nsigs))
		{
			printe(ERR_no_signals, E_ERROR, lwp_name() );
			return 0;
		}
	}
	return 1;
}

int
LWP::remove_sig( sigset_t sigs, Notifier func, void *thisptr )
{
	sig_ctl	*nsigs;
	sig_ctl	osigs;

	osigs.signals = sigs;

	if (etable == 0)
	{
		printe(ERR_internal, E_ERROR, "LWP::remove_sig", __LINE__);
		return 0;
	}
	// siglist.remove returns 1 if the addition changed the current
	// mask
	if (etable->siglist.remove(&osigs, func, thisptr))
	{
		nsigs = etable->siglist.sigset();
		if (!pctl->trace_sigs( nsigs ))
		{
			printe(ERR_no_signals, E_ERROR, lwp_name() );
			return 0;
		}
	}
	return 1;
}

int
LWP::cancel_sig( int signo )
{
	if (!pctl->cancel_sig( signo ))
	{
		printe(ERR_sys_cancel_sig, E_ERROR, lwp_name() );
		return 0;
	}
	return 1;
}

int
LWP::cancel_sig_all()
{
	if (!pctl->cancel_all_sig())
	{
		printe(ERR_sys_cancel_sig, E_ERROR, lwp_name() );
		return 0;
	}
	return 1;
}

char *
LWP::text_nobkpt( Iaddr addr )
{
	char *		s;
	Breakpoint *	b;

	s = 0;
	if ( addr == hoppt.addr() )
	{
		s = hoppt.oldtext();
	}
	else if ( addr == destpt.addr() )
	{
		s = destpt.oldtext();
	}
	else if ( (etable != 0 ) && ((b = etable->breaklist.lookup(addr)) != 0) )
	{
		s = b->oldtext();
	}
	else if ( addr == dynpt.addr() )
	{
		s = dynpt.oldtext();
	}
	return s;
}

int
LWP::set_sys_trace(int i, Systype systype, Notifier func, void *thisptr)
{
	sys_ctl	*nsys;
	int	errors = 0;

	// tsclist.add returns 1 if the addition changed the current
	// mask

	if (etable == 0)
	{
		printe(ERR_internal, E_ERROR, "LWP::set_sys_trace", __LINE__);
		return 0;
	}
	if (systype == Entry || systype == Entry_exit)
	{
		if (etable->tsclist.add(i, 0, func, thisptr))
		{
			nsys = etable->tsclist.tracemask(0);
			errors += ( pctl->sys_entry( nsys ) == 0);
		}
	}
	if (systype == Exit || systype == Entry_exit)
	{
		if (etable->tsclist.add(i, 1, func, thisptr))
		{
			nsys = etable->tsclist.tracemask(1);
			errors += ( pctl->sys_exit( nsys ) == 0);
		}
	}
	return(errors ? 0 : 1);
}

int
LWP::remove_sys_trace(int i, Systype systype, Notifier func, void *thisptr)
{
	sys_ctl	*nsys;
	int	errors = 0;

	// tsclist.remove returns 1 if the addition changed the current
	// mask

	if ( etable == 0 )
	{
		printe(ERR_internal, E_ERROR, "LWP::remove_sys_trace", __LINE__);
		return 0;
	}

	if (systype == Entry || systype == Entry_exit)
	{
		if (etable->tsclist.remove(i, 0, func, thisptr))
		{
			nsys = etable->tsclist.tracemask(0);
			errors += ( pctl->sys_entry( nsys ) == 0);
		}
	}
	if (systype == Exit || systype == Entry_exit)
	{
		if (etable->tsclist.remove(i, 1, func, thisptr))
		{
			nsys = etable->tsclist.tracemask(1);
			errors += ( pctl->sys_exit( nsys ) == 0);
		}
	}
	return(errors ? 0 : 1);
}

void
LWP::add_event(Event *event)
{
	if (firstevent)
		event->prepend(firstevent);
	firstevent = event;
}

void
LWP::remove_event(Event *event)
{
	if (event == firstevent)
		firstevent = event->next();
	event->unlink();
}

void
LWP::remove_all_bkpt(Breakpoint *b)
{
	if (!b)
		return;
	remove_all_bkpt(b->_left);
	remove_all_bkpt(b->_right);
	REMOVE(b->_flags);
}

int
LWP::lift_all_bkpt(Breakpoint *b, int other_lwp)
{
	if (!b)
		return 1;
	if (!lift_all_bkpt(b->_left, other_lwp) ||
		!lift_all_bkpt(b->_right, other_lwp))
		return 0;
	if ( IS_INSERTED(b->_flags) && (lift_bkpt(b, other_lwp) == 0 ) )
	{
		return 0;
	}
	return 1;
}

int 
LWP::insert_all_bkpt(Breakpoint *b)
{
	if (!b)
		return 1;
	if (!insert_all_bkpt(b->_left) ||
		!insert_all_bkpt(b->_right))
		return 0;
	if ( (!IS_INSERTED(b->_flags)) && insert_bkpt(b) == 0 )
	{
		return 0;
	}
	return 1;
}

// set watchpoint - returns 1 for hardware watch, 0 for software
int
LWP::set_watchpoint(Iaddr pl, Rvalue *rv, Notifier func, void *thisptr)
{
	if ( etable == 0 )
	{
		printe(ERR_internal, E_ERROR, "LWP::set_watchpoint", __LINE__);
		return WATCH_FAIL;
	}
	etable->set_watchpoint(func, thisptr);
	if (pl != (Iaddr)-1)
	{
		if (!hw_watch)
			hw_watch = new HW_Watch;
		if (hw_watch->add(pl, rv, this, thisptr))
			return WATCH_HARD;
	}
	sw_watch++;
	return WATCH_SOFT;
}

int
LWP::remove_watchpoint(int hw, Iaddr pl, Notifier func, void *thisptr)
{
	if ( etable == 0 )
	{
		printe(ERR_internal, E_ERROR, "LWP::remove_watchpoint", __LINE__);
		return 0;
	}
	if (hw)
	{
		if ((state != es_dead) && (!hw_watch || 
			!hw_watch->remove(pl, this, thisptr)))
			return 0;
	}
	else
		sw_watch--;
	return(etable->remove_watchpoint(func, thisptr));
}

// return set of pending signals
int
LWP::pending_sigs()
{
	if (state == es_dead || state == es_corefile || !pctl)
		return 0;
	return(pctl->pending_sigs());
}
