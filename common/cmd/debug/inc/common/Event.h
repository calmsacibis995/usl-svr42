/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Event_h
#define Event_h
#ident	"@(#)debugger:inc/common/Event.h	1.3"

// General Event handler mechanism.  All events,
// stop, signals, system calls and onstop
// are referenced through a global event
// table, m_event, of type EventManager.
//
// Event ids are assigned consecutively and never re-used.
// Events that are assigned in several lwps each have 
// their own EventManager entries, but with the same event id.
// All events relating to a given lwp are contained on a linked list.
//
// The event mechanism has two levels.  Primitive events (signals
// or system calls to trace, locations to watch or to plant
// breakpoints on) are handled at the LWP/process level.
// The higher level events, with event ids, counts, complicated
// expressions, are handled at the Event/EventManager level.
// The lower level communicates with the upper through trigger
// functions.  Low-level events contain pointers to these
// functions (Notifiers).

#include "Frame.h"
#include "Iaddr.h"
#include "List.h"
#include "Link.h"
#include "TSClist.h"
#include <signal.h>
#include <string.h>

class	Endlist;
struct	EventTable;
class	Event;
class	Expr;
class	IntList;
class	LWP;
class	Location;
struct	Node;
struct	Proclist;
class	Program;
class	StopEvent;
class	StopExpr;
class	StopLoc;
class	WatchData;

extern void		dispose_event(StopEvent *);
extern char		*print_stop(StopEvent *, LWP *);
extern StopEvent	*copy_tree(StopEvent *);
extern List		m_cmdlist;
extern int		do_assoccmds();


// used by event_op functions
enum Event_op {
	M_Nop,
	M_Disable,
	M_Enable,
	M_Delete,
	M_Display
};

// Associated command list
struct A_cmd {
	Node	*cmd;
	Event	*event;
};

class EventManager  {
	Event	**base;
	int	count;		// number of pointers used
	int	ptr_count;	// number of pointers allocated
	int	cur_id;
	int	thisevent;	// event that just triggered
	int	lastevent;	// last event id assigned
	void	display(Event *, int mode, char * );
public:
		EventManager() { memset((void *)this, 0, sizeof(*this)); }
		~EventManager();
	Event	*add(Event *);
	Event	*find_event(int);
	int	copy(Event *, EventTable *, LWP *);
	int	event_op(int, Event_op);
	int	event_op(Proclist *, int, Event_op);
	int	event_op(LWP *, Program *, int, Event_op);
	void	drop_event(Event *);
	// Access functions
	int	new_id()  	{ return ++cur_id; } // never reused
	void	dec_id()  	{ if (cur_id > 0) --cur_id; }
	Event	**events()	{ return this->base; }
	int	this_event()	{ return thisevent; }
	int	last_event()	{ return lastevent; }
	int	event_cnt()	{ return ptr_count; }
	void	set_this(int t)	{ thisevent = t; }
	void	set_last(int l)	{ lastevent = l; }
	Event	*operator[](int i) { return (Event *)(this->base[i]); }
};

extern EventManager	m_event;

// Codes returned by notifiers
#define NO_TRIGGER	0
#define TRIGGER_QUIET	1
#define TRIGGER_VERBOSE	2
#define TRIGGER_FOREIGN	3
#define NO_TRIGGER_RM	4

// Event types
#define	E_SIGNAL	1
#define	E_SCALL		2
#define	E_STOP		3
#define	E_ONSTOP	4

// Event state
#define	E_DELETED	0
#define E_DISABLED	1
#define E_ENABLED	2
#define E_INVALID	3
#define E_DISABLED_INV	4

// Event level - program level events are copied on fork, lwp-create
// and program re-creation.  Process level events are copied
// on lwp-create.

#define E_PROCESS	1
#define E_PROGRAM	2

// There are 4 types of events: signal, system call, stop and onstop,
// all derived from the base Event class
//
class Event : public Link {
protected:
	char		quiet;	// set for quiet, else verbose
	char		level;	// process, program
	short		state;	// enabled, disabled, deleted
	int		id;	// event number
	Node		*cmd;	// associated commands
	EventTable	*etable;
	Event		**manage_slot;	// slot in EventManager table
	friend class	EventManager;
public:
			Event(int id, int level, int quiet, 
				Node *, EventTable *);
	virtual		~Event() {}
	Event		*next() { return (Event *)Link::next(); }
	virtual int	remove();
	virtual int	trigger();
	virtual void	cleanup(int level); // called when LWP dies
	virtual int	re_init(); // event applied to new LWP
	virtual int	get_type(); 
	virtual int	enable();
	virtual int	disable();
	// Access functions
	int		get_state() { return state; }
	int		is_quiet()  { return (quiet == 1); }
	int		get_level() { return level; }
	Node		*get_cmd()  { return cmd; }
	void		set_cmd(Node *c) { cmd = c; }
	void		set_quiet() { quiet = 1; }
	void		set_verbose() { quiet = 0; }
	EventTable	*get_etable() { return etable; }
	int		get_id()    { return id; }
	void		set_id(int i)    { id = i; }
};

// Traced signal events
class Sig_e : public Event {
	sigset_t	signals;
public:
			Sig_e(sigset_t, int id, int level, int quiet, 
				Node *, EventTable *);
			~Sig_e() {}
	char 		*print();
	int		trigger();
	int		remove();
	int		get_type(); 
	sigset_t	get_sigs() { return signals; }
};

// Traced system call events
class Sys_e : public Event {
	IntList		*syscalls;
	Systype		systype;	// entry, exit, entry+exit
	int		cur_count;	// # of times event has fired
	int		orig_count;	// event actions occur after
					// this many firings
public:
			Sys_e(IntList *, Systype, int id, int level, 
				int quiet, int count, 
				Node *, EventTable *);
			~Sys_e();
	int		remove();
	char 		*print();
	int		trigger();
	int		get_type(); 
	int		re_init();
	void		set_count(int c) { orig_count = c; }
	void		reset_count() { cur_count = 0; }
	int		get_count()   { return orig_count; }
	Systype		get_stype()   { return systype; }
	IntList		*get_calls()   { return syscalls; }
};

// Command lists executed whenever LWP stops
class Onstop_e : public Event {
public:
			Onstop_e(int id, int level, 
				Node *, EventTable *);
			~Onstop_e() {}
	int		remove();
	int		trigger();
	int		get_type(); 
};


// Stop events contain stop expressions linked by conjunction
// or disjunction operators.  If a part of stop expression
// relates to an LWP that is different from the LWP in whose
// context the event is set, that part of the event is considered
// "foreign".

// codes returned by stop_set
#define	SET_FAIL	0
#define SET_INVALID	1
#define SET_VALID	2

class Stop_e : public Event {
	StopEvent	*stop;
	char		*event_expr;
	int		cur_count;
	int		orig_count;
	int		stop_eval(StopEvent *);
public:
			Stop_e(StopEvent *, int id, int level, 
				int quiet, int count, 
				Node *, EventTable *);
			Stop_e(int id, int level, int quiet, int count, 
				Node *, EventTable *);
			~Stop_e();
	int		remove();
	int		trigger();
	int		trigger_foreign();
	int		get_type(); 
	void		copy(Stop_e *);
	void		cleanup(int level);
	int		re_init();
	void		invalidate();
	void		validate();
	void		set_count(int c) { orig_count = c; }
	void		reset_count() { cur_count = 0; }
	int		get_count()   { return orig_count; }
	StopEvent	*get_stop()   { return stop; }
	char		*get_expr()   { return event_expr; }
	int		enable();
	int		disable();
};


// There are 2 types of stop event expressions: location
// and expression.  They are both derived from the base class StopEvent.
// Expression StopEvents are used for both stop (x) and stop (*x)
// Stop event expressions are created by the parser and linked
// as a list.

// StopEvent flags
#define	E_LEAF		0x1
#define	E_AND		0x2
#define	E_OR		0x4
#define E_SET		0x8
#define E_FOREIGN	0x10
#define E_TRUE		0x20
#define E_VALID		0x40
#define E_TRIG_ON_CHANGE	0x80	// indicates a stop *x type event
					// change alone triggers the event

#define NODE_MASK	0x87

// Base class for different types of stop events;
class StopEvent {
protected:
	int		eflags;
	EventTable	*evtable;
	Stop_e		*sevent;
	StopEvent	*_next;
public:
			StopEvent(int flags);
	virtual		~StopEvent() {}
	virtual int	stop_true();
	virtual int	stop_set(LWP *, Stop_e *);
	virtual int	stop_copy(LWP *, Stop_e *, StopEvent *);
	virtual int	remove();
	virtual int	re_init();
	virtual void 	cleanup();
	virtual void 	disable();
	virtual void 	enable();
	virtual char 	*print(LWP *);
	virtual StopEvent *copy();
	//	Access functions
	int		get_flags() 	{ return eflags; }
	void		set_flags(int f){ eflags = f; }
	StopEvent	*next() 	{ return _next; }
	EventTable	*etable() 	{ return evtable; }
	void		append(StopEvent *nxt) { _next = nxt; }
	Stop_e		*event()	{ return sevent; }
};

// Breakpoint type stop events
// For breakpoints on line numbers or addresses, the event
// is true only when the LWP is stopped at that address.
// For breakpoints on function names, the event is true as 
// long as the function is active.

class StopLoc : public StopEvent {
	int		is_func;
	Location	*loc;
	Iaddr		addr;
	Endlist		*endlist;	// list of return addresses
public:
			StopLoc(int flags, Location *);
			~StopLoc();
	int		stop_true();
	int		stop_set(LWP *, Stop_e *);
	int		stop_copy(LWP *, Stop_e *, StopEvent *);
	int		re_init();
	void 		cleanup();
	int		remove();
	int		trigger();
	char	 	*print(LWP *);
	Endlist		*end_list() { return endlist; }
	void		set_list(Endlist *e) { endlist = e; }
	StopEvent	*copy();
};

// There are two flavors of watchpoints.  Expression watchpoints
// are true if the expression evaluates to true (non-zero for C/C++).
// The evaluation is triggered whenever any of the objects making
// up the expression changes value.
// Data watchpoints are true only when the (single) object they
// watch changes value.
//
// The identifiers in a stop expression are watched individually
// by WatchData items.


// An expr watchpoint contains a list of data watchpoints,
// 1 for each lvalue in the expression.
// If E_TRIG_ON_CHANGE is set, the event triggers when any sub data items
// change; otherwise when the expression is true

class StopExpr : public StopEvent {
	char		*exp_str;	// expression string
	Expr		*expr;
	WatchData	*data;
	List		triglist;
public:
			StopExpr(int flags, char *);
			~StopExpr() { triglist.clear(); }
	int		stop_true();
	int		stop_set(LWP *, Stop_e *);
	int		stop_copy(LWP *, Stop_e *, StopEvent *);
	int		re_init();
	int		remove();
	void 		cleanup();
	int 		trigger(int foreign);
	char 		*print(LWP *);
	StopEvent	*copy();
	Expr		*get_expr() { return expr; }
	int		eval_expr(FrameId &); // evaluate in given context
	int		recalc(WatchData *, FrameId &); // recalculate
					// watchpoints if a change in one
					// part of an expression might
					// cause a change in another;
					// like x->i, if x changes
	void		validate();
	void		invalidate();
	void 		disable();
	void 		enable();
};

// cfront 2.1 requires base class name in derived class constructor,
// 1.2 forbids it
#ifdef __cplusplus
#define EVENT		Event
#define STOPEVENT	StopEvent
#else
#define EVENT
#define STOPEVENT
#endif

#endif
// end of Event.h
