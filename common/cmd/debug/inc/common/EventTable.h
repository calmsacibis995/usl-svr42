/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef EventTable_h
#define EventTable_h
#ident	"@(#)debugger:inc/common/EventTable.h	1.1"

// EventTable manages the low-level events associated with a given
// LWP.  When an LWP dies or exits, its event table is saved with
// a proto program so its events can be re-instantiated.

#include "Breaklist.h"
#include "Siglist.h"
#include "TSClist.h"
#include "Ev_Notify.h"

class Event;
class LWP;
class Object;
class Program;

struct EventTable {
	Breaklist	breaklist;
	Siglist		siglist;
	TSClist		tsclist;
	NotifyEvent	*onstoplist;
	NotifyEvent	*watchlist;
	NotifyEvent	*foreignlist;
	Object 		*object;
	LWP 		*lwp;
	Program		*prog;
	Event		*firstevent; 
			EventTable();
			~EventTable();
	void		set_watchpoint(Notifier, void *);
	int		remove_watchpoint(Notifier, void *);
	void		set_onstop(Notifier, void *);
	int		remove_onstop(Notifier, void *);
	void		add_foreign(Notifier, void *);
	int		remove_foreign(Notifier, void *);
	void		remove_event(Event *);
};

EventTable		*find_et( int, char *&path );
EventTable		*dispose_et( EventTable * );

#endif

// end of EventTable.h

