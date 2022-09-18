#ident	"@(#)debugger:libexecon/common/Breaklist.C	1.1"
#include	"Breaklist.h"
#include	"Flags.h"
#include	"Link.h"
#include	<string.h>

// A Breaklist is maintained as a binary search tree, ordered
// by address

Breaklist::~Breaklist()
{
	Breakpoint	*b = root;
	
	dispose(b);
}

void
Breaklist::dispose(Breakpoint *b)
{
	if (!b)
		return;
	dispose(b->_left);
	dispose(b->_right);
	delete(b);
}

// add a breakpoint with the given address; if there is
// already a breakpoint at that addr, add event to its 
// eventlist

Breakpoint *
Breaklist::add( Iaddr a, Notifier func, void *thisptr)
{
	Breakpoint	*b, *nb, *p = 0;

	b = root;
	while(b)
	{
		if (a > b->_addr)
		{
			p = b;
			b = b->_right;
		}
		else if (a < b->_addr)
		{
			p = b;
			b = b->_left;
		}
		else 
		{
			// already a breakpoint at that address
			// just add the event
			NotifyEvent *ne = new NotifyEvent(func, thisptr);
			if (b->_events)
				ne->prepend(b->_events);
			b->_events = ne;
			return b;
		}
	}
	nb = new Breakpoint(a, func, thisptr);
	if (!p && !b)
	{
		root = nb;
	}
	else if (a > p->_addr)
	{
		p->_right = nb;
	}
	else
	{
		p->_left = nb;
	}
	return nb;
}

// remove an event from a breakpoint
Breakpoint *
Breaklist::remove( Iaddr a, Notifier func, void *thisptr )
{
	Breakpoint	*b, *p = 0;
	NotifyEvent	*ne;

	b = root;
	while(b)
	{
		if (a > b->_addr)
		{
			p = b;
			b = b->_right;
		}
		else if (a < b->_addr)
		{
			p = b;
			b = b->_left;
		}
		else 
		{
			break;
		}
	}
	if (!b)
		return 0;

	// delete particular event
	ne = b->_events;
	for(; ne ; ne = ne->next())
	{
		if ((ne->func == func) &&
			(ne->thisptr == thisptr))
			break;
	}
	if (!ne)
		return 0;
	if (ne == b->_events)
		b->_events = ne->next();
	ne->unlink();
	delete(ne);
	return b;
}

// remove a breakpoint
int
Breaklist::remove( Iaddr a )
{
	Breakpoint	*b, *p = 0;
	int		left = 0;

	b = root;
	while(b)
	{
		if (a > b->_addr)
		{
			p = b;
			b = b->_right;
			left = 0;
		}
		else if (a < b->_addr)
		{
			p = b;
			b = b->_left;
			left = 1;
		}
		else 
		{
			break;
		}
	}
	if (!b)
		return 0;

	if (!p)
	{
		if (!b->_right && !b->_left)
			root = 0;
		else if (!b->_right)
			root = b->_left;
		else
		{
			root = b->_right;
			if (b->_left)
			{
				Breakpoint	*r;
				r = root->_left;
				while(r && r->_left)
				{
					r = r->_left;
				}
				if (r)
					r->_left = b->_left;
				else
					root->_left = b->_left;
			}
		}
	}
	else if (left)
	{
		if (b->_right)
		{
			Breakpoint	*r;
			p->_left = b->_right;
			r = b->_right;
			while(r && r->_left)
			{
				r = r->_left;
			}
			r->_left = b->_left;
		}
		else
			p->_left = b->_left;
	}
	else
	{
		if (b->_left)
		{
			Breakpoint	*r;
			p->_right = b->_right;
			r = b->_right;
			while(r && r->_left)
			{
				r = r->_left;
			}
			if (r)
				r->_left = b->_left;
			else
				p->_right = b->_left;
			
		}
		else 
			p->_right = b->_right;
	}
	delete b;
	return 1;
}


Breakpoint *
Breaklist::lookup( Iaddr a )
{
	Breakpoint	*b = root;

	while(b)
	{
		if (a > b->_addr)
		{
			b = b->_right;
		}
		else if (a < b->_addr)
		{
			b = b->_left;
		}
		else
		{
			return b;
		}
	}
	return 0;
}

Breakpoint::Breakpoint()
{
	_addr = 0;
	_left = 0;
	_right = 0;
	_flags = 0;
	ENABLE(_flags);
	_events = 0;
}

Breakpoint::Breakpoint( Iaddr a, Notifier func, void *thisptr)
{
	_addr = a;
	_left = 0;
	_right = 0;
	_flags = 0;
	ENABLE(_flags);
	_events = new NotifyEvent(func, thisptr);
}

Breakpoint::~Breakpoint()
{
	NotifyEvent	*tmp, *ne = _events;

	while(ne)
	{
		tmp = ne;
		ne = ne->next();
		tmp->unlink();
		delete tmp;
	}
}

void
Breakpoint::copy(Breakpoint	&b)
{
	_addr = b._addr;
	_left = b._left;
	_right = b._right;
	_flags = b._flags;
	memcpy( _oldtext, b._oldtext, BKPTSIZE );
	_events = 0;
	if (!b._events)
		return;
	else
	{
		NotifyEvent	*ne, *one;
		for (one = b._events; one; one = one->next())
		{
			ne = new NotifyEvent(one->func, one->thisptr);
			if (_events)
				ne->prepend(_events);
			_events = ne;
		}
	}
	return;
}
