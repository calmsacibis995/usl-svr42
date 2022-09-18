#ident	"@(#)debugger:libexecon/common/Siglist.C	1.1"

#include "Siglist.h"
#include "Ev_Notify.h"
#include "Machine.h"
#include "Proctypes.h"
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/procfs.h>

Siglist::Siglist()
{
	// start off catching all signals
	prfillset(&_sigset.signals);
	memset((void *)_events, 0, (NSIG-1) * sizeof(List *));
}

Siglist::~Siglist()
{
	int	i;

	premptyset(&_sigset.signals);
	for ( i = 0 ; i < NSIG-1 ; i++ )
	{
		NotifyEvent	*ne = _events[i];
		while(ne)
		{
			NotifyEvent	*tmp;
			tmp = ne;
			ne = ne->next();
			delete(tmp);
		}
		_events[i] = 0;
	}
}

NotifyEvent *
Siglist::events( int sig )
{
	if ( (sig <= 0) || (sig >= NSIG) )
		return 0;
	return _events[sig-1];
}


int
Siglist::ignored( int sig )
{
	return !prismember(&_sigset.signals, sig);
}

// returns 1 if the sigmask changes; else 0
int
Siglist::add( sig_ctl *sigs, Notifier func, void *thisptr)
{
	int	changed = 0;

	for (int i = 1; i < NSIG; i++)
	{
		if (prismember(&sigs->signals, i))
		{
			if (func)
			{
				NotifyEvent	*ne; 
				ne = new NotifyEvent(func, thisptr);
				if (_events[i-1])
					ne->prepend(_events[i-1]);
				_events[i-1] = ne;
			}
			if (!prismember(&_sigset.signals, i))
			{
				changed = 1;
				praddset(&_sigset.signals, i);
			}
		}
	}
	return changed;
}

// returns 1 if the sigmask changes; else 0
int
Siglist::remove( sig_ctl *sigs, Notifier func, void *thisptr )
{
	NotifyEvent	*el;
	int		changed = 0;

	for (int i = 1; i < NSIG; i++)
	{
		if (prismember(&sigs->signals, i))
		{
			el = _events[i-1];
			if (el && func)
			{
				NotifyEvent	*ne = el;
				for(; ne; ne = ne->next())
				{
					if ((ne->func == func) &&
						(ne->thisptr == thisptr))
					break;
				}
				if (!ne)
					return 0;
				if (ne == el)
				{
					_events[i-1] = ne->next();
				}
				else
					ne->unlink();
				delete(ne);
			}
			// if we are deleting an event, don't turn
			// off tracing for this signal
			if (func)
				continue;
			if (!_events[i-1] && !SIG_SPECIAL(i))
			{
				prdelset(&_sigset.signals, i);
				changed = 1;
			}
		}
	}
	return changed;
}
