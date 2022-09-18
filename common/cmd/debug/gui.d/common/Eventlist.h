/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_EVENTLIST_H
#define _EVENTLIST_H
#ident	"@(#)debugger:gui.d/common/Eventlist.h	1.3"

#include "Link.h"
#include "Proclist.h"
#include "Notifier.h"
#include "Iaddr.h"
#include "str.h"

// cfront 2.1 requires class name in constructor, 1.2 doesn't accept it
#ifdef __cplusplus
#define EVENT	Event
#else
#define EVENT
#endif

class Message;
class Plink;

enum Event_type { ET_none, ET_stop, ET_signal, ET_syscall, ET_onstop };
enum Event_state { ES_valid, ES_disabled, ES_invalid, ES_disabled_inv };
enum Reason_code_break { BK_add, BK_delete };

class Event
{
	Event_state	state;
	int		id;
	char		*condition;
	char		*commands;

protected:
	Plink		*head;	// list of processes applicable
	Plink		*tail;	// list of processes applicable

public:
			Event(int id, const char *state,
				const char *condition, char *plist,
				const char *cmds);
	virtual		~Event();

			// access functions
	Event_state	get_state()	{ return state; }
	int		get_id()	{ return id; }
	const char	*get_condition() { return condition; }
	char		*get_plist();
	const char	*get_commands()	{ return commands; }

	void		set_state(Event_state s)
					{ state = s;}
	void		set_state(const char *);
	void		set_condition(const char *c){ delete condition; condition = makestr(c);}
	void		set_plist(char *p);
	void		set_commands(const char *c){ delete commands; commands = makestr(c) ;}

	void		add_process(Process *);
	void		delete_process(Process *);

	int		has_process(Process *);
	int		has_no_process(){ return (head == 0);}
	virtual	Event_type	get_type();
	virtual const char	*get_type_string();
};

struct Breakpoint
{
	char	*file;
	int	line;
	Iaddr	addr;
};

class Stop_event : public Event
{
	Breakpoint	*breakpts;
	int		nbreakpts;
	int		count;

public:
			Stop_event(int eid, const char *state,
				const char *condition, char *plist,
				const char *cmds, int count, int nbreakpts,
				Breakpoint *breakpts);
	virtual		~Stop_event();

	void		set_count(int c){ count = c;}
	int		get_count(){ return count;}
	int		get_nbreakpts()	{ return nbreakpts; }
	Breakpoint	*get_breakpts()	{ return breakpts; }
	Event_type	get_type();
	const char	*get_type_string();
};

class Signal_event : public Event
{
public:
			Signal_event(int eid, const char *state,
				const char *condition, char *plist,
				const char *cmds);
	virtual		~Signal_event() {}

	Event_type	get_type();
	const char	*get_type_string();
};

class Syscall_event : public Event
{
	List		syscalls;
	char		*type;
	int		count;
public:
			Syscall_event(int eid, const char *state, const char *type,
				const char *condition, char *plist,
				const char *cmds, int count);
	virtual		~Syscall_event() { delete type;}

	void		set_count(int c){ count = c;}
	int		get_count(){ return count;}
	Event_type	get_type();
	const char	*get_type_string();
};

class Onstop_event : public Event
{
public:
			Onstop_event(int eid, const char *state,
				char *plist, const char *cmds);
	virtual		~Onstop_event() {}

	Event_type	get_type();
	const char	*get_type_string();
};

// Elink is used for linked lists of events
class Elink : public Link
{
	Event	*_event;
public:
		Elink(Event *e)	{ _event = e; }
		~Elink()	{}

	Elink	*next()		{ return (Elink *)Link::next(); }
	Elink	*prev()		{ return (Elink *)Link::prev(); }
	Event	*event()	{ return _event; }
};

class Flink : public Link
{
	int	line;
	Event	*event;
public:
		Flink(int l, Event *e)	{ line = l; event = e; }
		~Flink() {}

		// access functions
	Flink	*next()		{ return (Flink *)Link::next(); }
	Flink	*prev()		{ return (Flink *)Link::prev(); }
	int	get_line()	{ return line; }
	Event	*get_event()	{ return event; }
};

class File_list : public Link
{
	char		*fname;
	Flink		*head;
	Flink		*tail;
public:
			File_list(const char *file);
			~File_list();

			// access functions
	File_list	*next()	{ return (File_list *)Link::next(); }
	File_list	*prev() { return (File_list *)Link::prev(); }
	const char	*get_name()	{ return fname; }
	Flink		*get_head()	{ return head; }

	void		add_event(int line, Stop_event *);
	void		delete_event(int line, Stop_event *);

	int		*get_break_list(Process *);
};

class Event_list
{
	List		events;
	File_list	*fhead;
	File_list	*ftail;

public:
	Notifier	change_list;

			Event_list() : change_list(this) { fhead = ftail = 0;}
			~Event_list()	{}

			// interfaces to Dispatcher
	void		new_event(Message *, Process *);
	void		change_event(Message *, Process *);
	void		delete_event(Message *, Process *);
	void		disable_event(Message *);
	void		enable_event(Message *);
	void		breakpt_set(Message *);

	Event		*findEvent(int id);
	File_list	*find_file(const char *fname, int add);
};

extern Event_list event_list;

#endif	// _EVENTLIST_H
