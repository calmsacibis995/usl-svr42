/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef StopEvent_h
#define StopEvent_h

#ident	"@(#)debugger:libexecon/common/StopEvent.h	1.1"

#include "Frame.h"
#include "Iaddr.h"
#include "Link.h"
#include "Place.h"
#include "Rvalue.h"

class EventTable;
class StopLoc;
class StopExpr;
class TriggerItem;

// Definitions used locally for implementing stop events.

// Location stop events whose location is a function address
// maintain a list of return addresses for that function.
// Each instance of a recursive function will have an Endlist item.
class	Endlist : public Link {
	Iaddr		endaddr;
	StopLoc		*sloc;
	friend class	StopLoc;
public:	
			Endlist(Iaddr i, StopLoc *s)
				{ endaddr = i; sloc = s; }
			Endlist(Endlist &old)
				{ endaddr = old.endaddr; sloc = old.sloc; }
			~Endlist() {}
	inline int	set(LWP *);
	int		trigger();
	Endlist		*next()	{ return (Endlist *)Link::next(); }
};


// If a data watchpoint refers to an automatic, we bracket
// the start of the enclosing function and its return address 
// with breakpoints.
// Watching stack addresses in recursive functions is done
// by instantiating a new Watchframe each time the function is entered.
//

// used for state field
#define S_NULL	0
#define S_START	1
#define S_SOFT	2
#define S_HARD	3

class Watchframe : public Link {
	int		state;
	WatchData	*event;
	Iaddr		addr;
	Iaddr		place;
	Iaddr		endscope;
	Rvalue		*last;
	FrameId		frame;
	friend class	WatchData;
public:
			Watchframe(WatchData *, Iaddr, FrameId &);
			~Watchframe() { delete last; }
	Watchframe	*next()	{ return (Watchframe *)Link::next(); }
	int		copy(Watchframe *);
	int		init();
	int		trigger_watch();
	int		trigger_start();
	int		trigger_end();
	int		changed();
	int		remove();
	int		recalc(); // recalculate
				// watchpoints if a change in one
				// part of an expression might
				// cause a change in another;
				// like x->i, if x changes
};

// Data watchpoints
// These are sub_items of StopExpr events.
// Each WatchData instance monitors a single lvalue.
//
class WatchData  {
	int		flags;
	TriggerItem	*item;
	Watchframe	*watch;
	StopExpr	*sexpr;
	WatchData	*_nxt;
	EventTable	*evtable;
	friend class	Watchframe;
public:
			WatchData(TriggerItem *);
			~WatchData() {}
	int		stop_expr_set(LWP *,  StopExpr *);
	int		stop_expr_copy(LWP *, StopExpr *, WatchData *);
	int		re_init();
	int		remove();
	void 		cleanup();
	int		getTriggerRvalue(Rvalue &);
	int		getTriggerLvalue(Place &);
	TriggerItem	*get_item() { return item; }
	void		append(WatchData *nxt) { _nxt = nxt; }
	WatchData	*next() { return _nxt; }
	int		get_flags() { return flags; }
	void		validate();
	void		invalidate();
	void		disable();
	void		enable();
	int		recalc(FrameId &); // recalculate watchpoints 
};

#endif
