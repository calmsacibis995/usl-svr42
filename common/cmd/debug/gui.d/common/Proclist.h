/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_PROCLIST_H
#define _PROCLIST_H
#ident	"@(#)debugger:gui.d/common/Proclist.h	1.12"

// GUI headers
#include "Notifier.h"
#include "UI.h"

// Debug headers
#include "Message.h"
#include "Link.h"
#include "Iaddr.h"

class Elink;
class Event;
class Program;
class Window_set;

#define	PROGRAM_LEVEL	1
#define PROCESS_LEVEL	2

// Process_state includes all the states that the gui needs to know about.
// created is separate from stopped because stack and symbols
// are meaningless when the process is first stopped.
// execed is the same as created (and forked the same as stopped),
// except that they don't make the new process the current process.
// grabbed is also a separate state because special handling
// is needed when the process is first grabbed.

// Even though the command line debugger displays dead programs in ps,
// and the user can examine a dead program's events, at least for now the gui
// is not tracking dead programs since it makes no sense to try to make
// one the current process, and currently that is the only way
// to see the program's events.

enum Process_state
{
	State_none,
	State_core,
	State_running,
	State_stepping,
	State_stopped,
};

// The Process list keeps a linked list of programs,
// each program has a linked list of processes (which may be empty,
// if they all died).  When a process is added, deleted, or changes state,
// the Process list makes the updates to the process; the process then
// informs the appropriate window set.

class Process
{
	DBcontext	id;		// magic cookie identifies process for debug
	Process_state	state;		// current state
	Program		*program;	// pointer to parent
	Window_set	*window_set;	// owner
	long		pid;		// UNIX system process id
	char		*procname;	// process name (p1, etc.)
	char		*function;	// current function, may be 0
	char		*file;		// current file name, may be 0
	char		*path;		// full path of file
	char		*location;	// character representation of location
	int		line;		// current line, may be 0
	int		frame;		// current frame number
	int		incomplete;	// remainder of event notification (location)
					// has not been received yet
	Boolean		touched;	// state changed in script
	Elink		*ehead;		// list of pointers to event objects
	Elink		*etail;

			// query debug for command line, etc.
	void		do_ps(char *&cmd);

	friend		Process_list;
	friend		Program;
public:
			Process(DBcontext, const char *progname,
				const char *procname, Process_state,
				Window_set *, int incomplete,
				int make_current);
			~Process();

			// access functions
	DBcontext	get_id()		{ return id; }
	Process_state	get_state()		{ return state; }
	Program		*get_program()		{ return program; }
	Window_set	*get_window_set()	{ return window_set; }
	const char	*get_name()		{ return procname; }
	long		get_pid()		{ return pid; }
	const char	*get_function()		{ return function; }
	const char	*get_file()		{ return file; }
	const char	*get_path()		{ return path; }
	int		get_line()		{ return line; }
	int		get_frame()		{ return frame; }
	void		set_frame(int f)	{ frame = f; }
	const char	*get_location()		{ return location; }
	const char	*get_state_string();
	int		is_incomplete()		{ return incomplete; }
	Elink		*get_events()		{ return ehead; }
	void		touch()			{ touched = TRUE; }

	const char	**get_objects(int &total);
	const char	**get_signals_with_status(Order);
	const char	**get_pending_signals(Order, int &);
	const char	**get_functions(int &total, const char *file,
				const char *obj, int dashg);

	void		get_frame_info();

			// functions to maintain event list
	void		add_event(Event *e);
	void		delete_event(Event *e);
	int		*get_break_list(Iaddr start, Iaddr end, int *&event_list);
	void		init_events();

	void		update_location(const char *function, const char *location);
	void		update_done();	// all the messages from an event notification
					// have been processed, and the window set is
					// informed of the state change

	int		check_sensitivity(int sense);
};

// Plink is used for linked lists of processes or programs
class Plink : public Link
{
	void	*data;
public:
		Plink(void *p)	{ data = p; }
		~Plink()	{}

	Plink	*next()		{ return (Plink *)Link::next(); }
	Plink	*prev()		{ return (Plink *)Link::prev(); }
	Process	*process()	{ return (Process *)data; }
	Program	*program()	{ return (Program *)data; }
};

class Program
{
	Plink		*head;		// list of processes
	Plink		*tail;
	char		*progname;	// program name
	char		*cmd_line;	// exec command line
	int		io_redirected;
	int		used_flag;

	friend		Process_list;
	friend		Process;
public:
			Program(const char *name, const char *cmd_line);
			~Program() { delete cmd_line; delete progname; }

			// access functions;
	const char	*get_name()		{ return progname; }
	const char	*get_cmd_line()		{ return cmd_line; }
	int		is_io_redirected()	{ return io_redirected; }
	Plink		*get_head()		{ return head; }
	Plink		*get_tail()		{ return tail; }
	int		get_used_flag()		{ return used_flag; }
	void		set_used_flag(int u)	{ used_flag = u; }

			// functions to maintain process list
	void		add_process(Process *);
	void		delete_process(Process *);
};

// The process list maintains the state for all processes under the debugger's
// control.  There is only one master process list, but there are subsidiary
// lists of processes with each window set; those lists are driven by
// changes to the master list.

class Process_list
{
	Plink		*head;		// linked list of programs
	Plink		*tail;
	Process		*last_refd;	// saved to help lookup performance

public:
			Process_list();
			~Process_list() {}

			// interfaces to Dispatcher; these drive process list updates
	Process		*new_proc(Message *, Window_set *, int first_process, int io_flag);
	void		proc_grabbed(Message *, Window_set *, int first_process);
	void		proc_forked(Message *, Process *);
	void		proc_execed(Message *, Process *);
	void		remove_proc(Process *);
	void		update_location(Message *, Process *);
	void		set_path(Message *, Process *);
	void		finish_update(Message *, Process *);
	void		proc_stopped(Process *);
	void		rename(Message *);
	void		set_frame(Message *, Process *);
	void		proc_jumped(Message *, Process *);
	void		update_all();

			// interfaces for GUI objects to change state
	void		set_state(Process *, Process_state);
	void		move_process(Process *, Window_set *);

			// find the given process or program
	Process		*find_process(DBcontext);
	Process		*find_process(const char *name);
	Process		*find_process(Message *);
	Process		*find_process(long);
	Program		*find_program(const char *name);

	void		add_program(Program *);
	void		delete_program(Program *);

	int		any_live(); // search for any process in any window set
	void		clear_program_list();

#if DEBUG
	void		dump();
#endif
};

extern Process_list proclist;

extern const char	*make_plist(int total, Process **, int use_blanks = 0,
				int level = PROCESS_LEVEL);

#endif	// _PROCLIST_H
