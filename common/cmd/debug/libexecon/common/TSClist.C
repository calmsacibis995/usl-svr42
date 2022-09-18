#ident	"@(#)debugger:libexecon/common/TSClist.C	1.2"

#include "TSClist.h"
#include "Ev_Notify.h"
#include "List.h"
#include "Proctypes.h"
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/procfs.h>

TSClist::TSClist()
{
	// start off trapping certain special syscalls
	premptyset(&entrymask.scalls)
	premptyset(&exitmask.scalls)
	praddset(&exitmask.scalls, SYS_exec);
	praddset(&exitmask.scalls, SYS_execve);
	praddset(&exitmask.scalls, SYS_fork);
	praddset(&exitmask.scalls, SYS_vfork);

	_events = 0;
}

TSClist::~TSClist()
{
	TSCevent	*e = _events;

	premptyset(&entrymask.scalls)
	premptyset(&exitmask.scalls)
	while(_events)
	{
		e = _events;
		_events = _events->next_tsc;
		delete e;
	}
}

TSCevent::~TSCevent()
{
	NotifyEvent	*ne = entry_events;
	while(ne)
	{
		NotifyEvent	*tmp;
		tmp = ne;
		ne = ne->next();
		delete(tmp);
	}
	ne = exit_events;
	while(ne)
	{
		NotifyEvent	*tmp;
		tmp = ne;
		ne = ne->next();
		delete(tmp);
	}
}

sys_ctl *
TSClist::tracemask( int exit )
{
	if ( exit )
		return &exitmask;
	else
		return &entrymask;
}

TSCevent*
TSClist::lookup(int sys)
{
	register TSCevent	*e = _events;
	while(e)
	{
		if (sys == e->tsc)
			break;
		if (e->tsc > sys)
			return 0;
		e = e->next_tsc;
	}
	return e;
}

// find is like lookup, but adds a TSCevent if none exists already
TSCevent*
TSClist::find(int sys)
{
	register TSCevent	*e = _events; 
	TSCevent		*tmp1 = 0, *tmp2;

	if (!e)
	{
		_events = new TSCevent(sys);
		return _events;
	}
	while(e)
	{
		if (sys == e->tsc)
			return e;
		if (sys < e->tsc)
			break;
		tmp1 = e;
		e = e->next_tsc;
	}
	tmp2 = new TSCevent(sys);
	tmp2->next_tsc = e;
	if (!tmp1)
	{
		_events = tmp2;
	}
	else
	{
		tmp1->next_tsc = tmp2;
	}
	return tmp2;
}

NotifyEvent *
TSClist::events( int sys, int exit )
{
	TSCevent	*e = lookup(sys);
	if (e == 0)
		return 0;
	if ( exit )
		return e->exit_events;
	else
		return e->entry_events;
}

// returns 1 if the sysmask changes; else 0
int
TSClist::add( int sys, int exit, Notifier func, void *thisptr )
{
	int		changed = 0;
	TSCevent	*e = find(sys);
	NotifyEvent	*ne, *newevent;

	newevent = new NotifyEvent(func, thisptr);

	if (exit)
	{
		ne = e->exit_events;
		if (!ne)
		{
			praddset(&exitmask.scalls, sys);
			changed = 1;
		}
		else
		{
			newevent->prepend(ne);
		}
		e->exit_events = newevent;
	}
	else 
	{
		ne = e->entry_events;
		if (!ne)
		{
			praddset(&entrymask.scalls, sys);
			changed = 1;
		}
		else
		{
			newevent->prepend(ne);
		}
		e->entry_events = newevent;
	}
	return changed;
}

// returns 1 if the sigmask changes; else 0
int
TSClist::remove( int sys, int exit, Notifier func, void *thisptr )
{
	NotifyEvent	*ne;
	int		changed = 0;
	TSCevent 	*e = lookup(sys);

	if (exit)
	{
		ne = e->exit_events;
		for(; ne; ne = ne->next())
		{
			if ((ne->func == func) &&
				(ne->thisptr == thisptr))
			break;
		}
		if (!ne)
			return 0;
		if (ne == e->exit_events)
		{
			e->exit_events = ne->next();
		}
		else
			ne->unlink();
		delete(ne);

		if (!e->exit_events && !SPECIAL_EXIT(sys))
		{
			prdelset(&exitmask.scalls, sys);
			changed = 1;
		}
	}
	else
	{
		ne = e->entry_events;
		for(; ne; ne = ne->next())
		{
			if ((ne->func == func) &&
				(ne->thisptr == thisptr))
			break;
		}
		if (!ne)
			return 0;
		if (ne == e->entry_events)
		{
			e->entry_events = ne->next();
		}
		else
			ne->unlink();
		delete(ne);
		if (!e->entry_events)
		{
			prdelset(&entrymask.scalls, sys);
			changed = 1;
		}
	}
	return changed;
}
