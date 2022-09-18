#ident	"@(#)debugger:gui.d/common/Proclist.C	1.20"

// GUI headers
#include "Eventlist.h"
#include "Proclist.h"
#include "Dispatcher.h"
#include "Windows.h"
#include "Events.h"
#include "UI.h"
#include "Menu.h"

// Debug headers
#include "Buffer.h"
#include "Message.h"
#include "Msgtypes.h"
#include "Machine.h"
#include "str.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

Process_list proclist;

static int
numeric_signal_comp(const void *s1, const void *s2)
{
	char	*ptr1 = ((char *[2])s1)[1];
	char	*ptr2 = ((char *[2])s2)[1];
	long	aval = strtol(ptr1, NULL, 0);
	long	bval = strtol(ptr2, NULL, 0);

	if (aval<bval) return -1;
	if (aval>bval) return 1;
	return 0;

}

// create a new process, and if necessary, create a new program
// and link it in
Process::Process(DBcontext new_id, const char *progname, const char *name,
	Process_state s, Window_set *ws, int inc, int make_current)
{
	char *cmd_line = 0;

	id = new_id;
	state = s;
	procname = makestr(name);
	window_set = ws;
	location = 0;
	function = 0;
	file = 0;
	line = 0;
	path = 0;
	frame = -1;	// 0 is a valid frame number
	incomplete = inc;
	ehead = etail = 0;

	if (in_script || has_assoc_cmd)
		touched = TRUE;
	else
	{
		touched = FALSE;
		init_events();
		do_ps(cmd_line);
	}

	if ((program = proclist.find_program(progname)) == 0)
	{
		program = new Program(progname, cmd_line);
		proclist.add_program(program);
	}
	program->add_process(this);
	window_set->add_process(this, make_current);
}

Process::~Process()
{
	//delete the list of events
	Elink	*item, *next;
	List	eventWins; // event windows needs to be updated

	for (item = etail; item; item = next)
	{
		Event*	e = item->event();

		//search through all event window, see if it contains this one
		Window_set	*win = (Window_set*)windows.first();

		for(; win; win = (Window_set*)windows.next()) 
		{
			Event_window *eWin = win->get_event_window() ;

			if( eWin )
			{
				if( eWin->contains( e->get_id() ) )
				{
					eWin->update_cb( 0, RC_set_current, 0,win->current_process());
				}
			}
		}

		e->delete_process( this );
		next = item->prev();
		item->unlink();
		delete item;
	}
	
	if (window_set)
		window_set->delete_process(this);
	delete procname;
	delete file;
	delete path;
	delete function;
	delete location;
}

//query for the events applicable to this process
void
Process::init_events()
{
	Word	event_id;
	Message	*m;
	Event	*e = 0;
	Word	count = 0;
	char	*qflag = 0;
	char	*state = 0;
	char	*condition = 0;
	char	*systype = 0;
	char	*plist = 0;
	char	*cmd = 0;

	dispatcher.query(0, id, "events -p %s\n", procname);
	while ((m = dispatcher.get_response()) != 0)
	{
		switch(m->get_msg_id())
		{
		case MSG_stop_event:
			m->unbundle(event_id, qflag, state, count,
				condition, cmd); 
			break;

		case MSG_syscall_event:
			m->unbundle(event_id, qflag, state, systype, count,
				condition, cmd);
			break;

		case MSG_signal_event:
			m->unbundle(event_id, qflag, state, condition, cmd);
			break;

		case MSG_onstop_event:
			m->unbundle(event_id, state, cmd);
			break;

		default:
			continue;
		}

		e = event_list.findEvent( (int)event_id ); 

		if (!e)
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			continue;
		}

		add_event( e );
		e->add_process( this );
	}
}

void
Process::update_done()
{
	incomplete = FALSE;
	touched = FALSE;
	window_set->update_process(this);
}

void
Process::update_location(const char *func, const char *loc)
{
	delete function;
	delete location;
	delete file;

	function = (func && *func) ? makestr(func) : 0;
	location = (loc && *loc) ? makestr(loc) : 0;

	char *p = strchr(loc, '@');
	if (p)
	{
		size_t len = p - (char *)loc;
		file = new char[len+1];
		strncpy(file, loc, len);
		file[len] = '\0';
		line = atoi(p+1);
	}
	else
	{
		file = 0;
		line = 0;
	}
}

// Only need to do a ps on the process when it is first created, grabbed,
// or exec'd, to get command line, after that all information needed should
// be available from event notification messages

void
Process::do_ps(char *&cmd_line)
{
	Message	*m;
	char	*buf;	// throwaways
	char	*func = 0;
	char	*loc = 0;
	char	*cmd = 0;
	Word	w1 = 0;

	dispatcher.query(0, 0, "ps -p %s\n", procname);

	cmd_line = 0;
	line = 0;

	while ((m = dispatcher.get_response()) != 0)
	{
		// current indicator, progname, procname, pid, func,
		//	 location, cmdline
		// should be doing ps on stopped processes and core files only

		switch(m->get_msg_id())
		{
		case MSG_ps_stopped:
		case MSG_ps_core:
			m->unbundle(buf, buf, buf, w1, func, loc, cmd);
			line = 0;
			break;

		default:
			break;
		}
	}

	if (cmd && *cmd)
		cmd_line = cmd;
	pid = w1;
	update_location(func, loc);
}

// convert the state into a string for display in the ps and status panes
const char *
Process::get_state_string()
{
	switch(state)
	{
	default:
	case State_none:
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return "Dead";

	case State_core:	return "Core";
	case State_running:	return "Running";
	case State_stepping:	return "Stepping";
	case State_stopped:	return "Stopped";
	}
}

// get the break list points for this process, whose address is between
// begin_addr and end_address, and return the event numbers in e_num
int *
Process::get_break_list(Iaddr begin_addr, Iaddr end_addr, int *&e_num)
{
	Elink	*list;
	Event	*e;
	int	line;
	
	vscratch1.clear();
	vscratch2.clear();

	for (list = ehead; list; list = list->next())
	{
		e = list->event();

		if( e->get_type() != ET_stop )
				continue;

		Breakpoint*	b = ((Stop_event*)e)->get_breakpts();
		int		n = ((Stop_event*)e)->get_nbreakpts();

		for( int count = 0; count < n; count++, b++ )
		{
			if( b->addr < begin_addr || b->addr > end_addr)
					continue;

			int tmp = e->get_id();

			vscratch1.add( &b->addr, sizeof(int));
			vscratch2.add( &tmp, sizeof(int));
		}
	
	}
	
	line	= 0;
	vscratch1.add(&line, sizeof(int));
	vscratch2.add(&line, sizeof(int));
	e_num	= (int *)vscratch2.ptr();

	return (int *)vscratch1.ptr();
}

void
Process::add_event(Event *e)
{
	Elink	*item = new Elink(e);
	Elink	*list;

	if (!ehead)
	{
		ehead = etail = item;
		return;
	}

	for (list = ehead; list; list = list->next())
		if (e->get_id() < list->event()->get_id())
			break;

	if (list)
	{
		item->prepend(list);
		if (list == ehead)
			ehead = item;
		else if (list == etail)
			etail = item;
	}
	else
	{
		item->append(etail);
		etail = item;
	}
}

void
Process::delete_event(Event *e)
{
	Elink	*item;

	for (item = ehead; item && e != item->event(); item = item->next())
		;

	if (!item)
		return;
	if (item == ehead)
		ehead = ehead->next();
	if (item == etail)
		etail = etail->prev();
	item->unlink();
	delete item;
}

int
Process::check_sensitivity(int sense)
{
	if ((sense&SEN_proc_io_redirected) && !program->is_io_redirected())
		return 0;
	if (sense & SEN_proc_live)
		return (state > State_core);
	if (sense & SEN_proc_stopped)
		return (state >= State_stopped);
	if (sense & SEN_proc_stopped_core)
		return (state >= State_stopped || state == State_core);
	if (sense & SEN_proc_running)
		return (state == State_running || state == State_stepping);
	return 1;
}

const char **
Process::get_objects(int &total)
{
	Message	*msg;
	char	*obj_name = "";

	total = 0;
	vscratch1.clear();
	dispatcher.query(0, id, "map\n");
	while((msg = dispatcher.get_response()) != 0)
	{
		Word	tmp1, tmp2, tmp3;
		char	*tmp4;
		char	*new_name;

		if (msg->get_msg_id() == MSG_map)
		{
			msg->unbundle(tmp1, tmp2, tmp3, tmp4, new_name);
			if (!new_name || !*new_name)
				continue;

			if( strcmp( new_name, "[STACK]" ) == 0 )
				continue;

			new_name = basename(new_name);
			if (strcmp(new_name, obj_name) == 0)
				continue;

			total++;
			obj_name = str(new_name);
			vscratch1.add(&obj_name, sizeof(char *));
		}
		else if (msg->get_msg_id() != MSG_map_header)
			display_msg(msg);
	}

	if (!total)
		return 0;
	return (char **)vscratch1.ptr();
}

const char **
Process::get_signals_with_status(Order order)
{
	Message	*msg;
	int	nsig = 0;

	dispatcher.query(0, id, "signal\n");
	vscratch1.clear();
	while ((msg = dispatcher.get_response()) != 0)
	{
		char	*name;
		char	buf[MAX_INT_DIGITS];
		Word	sig;

		Msg_id mid = msg->get_msg_id();
		if (mid == MSG_sig_caught || mid == MSG_sig_ignored)
		{
			msg->unbundle(sig, name);
			vscratch1.add(name, strlen(name)+1);
			(void) sprintf(buf, "%d", sig);
			vscratch1.add(buf, strlen(buf)+1);
			if (mid == MSG_sig_caught)
				vscratch1.add("caught", sizeof("caught"));
			else
				vscratch1.add("ignored", sizeof("ignored"));
			nsig++;
		}
		else // ignore all the event messages
			continue;
	}
	Nsignals = nsig;

	// get pointers
	char *ptr = (char *)vscratch1.ptr();
	vscratch2.clear();
	for (int i = 0; i < nsig; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			vscratch2.add(&ptr, sizeof(char *));
			ptr += strlen(ptr)+1;
		}
	}

	// assumes signals come in numeric order
	if (order == Alpha)
		qsort(vscratch2.ptr(), nsig, sizeof(char *[3]), alpha_comp);
	return (char **)vscratch2.ptr();
}

// This routine uses the vector package to handle the list of outstanding
// signals scheduled to be applied to a process. This is sort of like
// killing a flea with a warhead, as it's doubtful there'll ever be
// more than, oh, about 2 signals outstanding, but it handles varying
// amounts of data coming back
//
// What is done is as follows:
//	Send a 'pending' message to the debugger
//	If there are no signals pending, return an array of pointers to
//	char that holds two elements that point at blanks
//
//	If there are signals pending, then
//		stash all the responses from the debugger in the scratch vector
//		construct a vector that points to the data
//		sort the second vector
//		return the appropriate vector
const char **
Process::get_pending_signals(Order order, int &num_sigs)
{
	Message	*msg;

	dispatcher.query(0, id, "pending\n");
	vscratch1.clear();
	num_sigs = 0;
	while ((msg = dispatcher.get_response()) != 0)
	{
		char	*name;
		char	buf[MAX_INT_DIGITS];
		Word	sig;

		Msg_id mid = msg->get_msg_id();
		if (mid == MSG_signame)
		{
			msg->unbundle(sig, name);
			vscratch1.add(name, strlen(name)+1);
			(void) sprintf(buf, "%d", sig);
			vscratch1.add(buf, strlen(buf)+1);
			num_sigs++;
		}
		else if (mid == MSG_help_hdr_sigs
			|| mid == MSG_newline) // no action needed
			continue;
		else
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}

	if (num_sigs > 0)
	{
		// get pointers
		char *ptr = (char *)vscratch1.ptr();
		vscratch2.clear();
		for (int i = 0; i < num_sigs; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				vscratch2.add(&ptr, sizeof(char *));
				ptr += strlen(ptr)+1;
			}
		}

		if (order == Alpha)
			qsort(vscratch2.ptr(), num_sigs, sizeof(char *[2]), alpha_comp);
		else
			qsort(vscratch2.ptr(), num_sigs, sizeof(char *[2]), numeric_signal_comp);
	}
	else
	{	// no data!
		num_sigs = 1;
		vscratch2.clear();
		vscratch2.add("", sizeof(char *));
		vscratch2.add("", sizeof(char *));
	}
	return (char **)vscratch2.ptr();
}

const char **
Process::get_functions(int &total, const char *file, const char *obj, int dashg)
{
	Message		*msg;
	const char	*sflag = dashg ? "-s" : "";

	total = 0;
	vscratch1.clear();
	if (file)
		dispatcher.query(0, id, "functions %s -f %s\n", sflag, file);
	else if (obj)
		dispatcher.query(0, id, "functions %s -o %s\n", sflag, obj);
	else
		dispatcher.query(0, id, "functions %s\n", sflag);

	while ((msg = dispatcher.get_response()) != 0)
	{
		if (msg->get_msg_id() == MSG_function)
		{
			char *name;
			msg->unbundle(name);
			name = makestr(name);
			vscratch1.add(&name, sizeof(char *));
			total++;
		}
		else
			display_msg(msg);
	}

	if (!total)
		return 0;
	return (char **)vscratch1.ptr();
}

void
Process::get_frame_info()
{
	Message		*msg;
	char		*s;
	char		*p;
	size_t		len;

	delete function;
	delete file;
	delete location;
	function = file = location = 0;

	dispatcher.query(0, id, "print %%frame, %%func, %%file, %%line, %%loc\n");
	while ((msg = dispatcher.get_response()) != 0)
	{
		if (msg->get_msg_id() != MSG_print_val)
		{
			display_msg(msg);
			continue;
		}

		msg->unbundle(s);
		frame = (int)strtol(s, &p, 10);
		if (!p)
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			continue;
		}

		s = p;
		while (*++s == ' ');
		if (*s != '"')
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			continue;
		}
		p = strchr(++s, '"');
		len = p++ - s;
		function = new char[len+1];
		strncpy(function, s, len);
		function[len] = '\0';
		while (*p++ != '"') ;
		s = p;
		p = strchr(s, '"');
		len = p - s;
		if (len)
		{
			file = new char[len+1];
			strncpy(file, s, len);
			file[len] = '\0';
		}

		while (*++p == ' ') ;
		s = p;
		line = (int)strtol(s, &p, 10);

		if (file && line)
			location = makesf("%s@%d", file, line);
		else
		{
			while(*++p != '0') ;
			len = strlen(p);
			// last character is a newline, overwrite with null byte
			location = new char[len];
			strncpy(location, p, len-1);
			location[len-1] = '\0';
		}
	}
}

Program::Program(const char *name, const char *cmd)
{
	progname = makestr(name);
	cmd_line = cmd ? makestr(cmd) : 0;
	head = tail = 0;
	io_redirected = 0;
	used_flag = 0;
}

// add a new process to the program's list.
// It's ok to just add to the end since debugger's ids are
// assigned in increasing order
void
Program::add_process(Process *ptr)
{
	Plink *item = new Plink(ptr);
	if (tail)
		item->append(tail);
	else
		head = item;
	tail = item;
}

// remove a process that died or was released
void
Program::delete_process(Process *ptr)
{
	Plink *item;

	if (!ptr || ptr->program != this)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	for (item = head; item && ptr != item->process(); item = item->next())
		;

	if (item == head)
		head = head->next();
	if (item == tail)
		tail = tail->prev();
	item->unlink();
	delete item;
	delete ptr;
}

Process_list::Process_list()
{
	last_refd = 0;
	head = tail = 0;
}

// add a new program, keeping the list in alphabetical order
void
Process_list::add_program(Program *ptr)
{
	Plink	*item = new Plink(ptr);
	Plink	*list;

	if (!ptr || !ptr->progname)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	if (!head)
	{
		head = tail = item;
		return;
	}

	for (list = head; list; list = list->next())
	{
		if (strcmp(ptr->progname, list->program()->progname) < 1)
			break;
	}

	if (list)
	{
		item->prepend(list);
		if (list == head)
			head = item;
	}
	else
	{
		item->append(tail);
		tail = item;
	} 
}

// remove a program whose children have all died
void
Process_list::delete_program(Program *ptr)
{
	Plink *item;

	if (!ptr || ptr->head)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	for (item = head; item && ptr != item->program(); item = item->next())
		;

	if (item == head)
		head = head->next();
	if (item == tail)
		tail = tail->prev();
	item->unlink();
	delete item;
	delete ptr;
}

Process *
Process_list::find_process(DBcontext id)
{
	Plink	*p1;
	Plink	*p2;

	if (last_refd && last_refd->get_id() == id)
		return last_refd;

	for (p1 = head; p1; p1 = p1->next())
	{
		for (p2 = p1->program()->head; p2; p2 = p2->next())
		{
			if (p2->process()->id == id)
			{
				last_refd = p2->process();
				return p2->process();
			}
		}
	}
	return 0;
}

Process *
Process_list::find_process(long pid)
{
	Plink	*p1;
	Plink	*p2;

	if (last_refd && last_refd->get_pid() == pid)
		return last_refd;

	for (p1 = head; p1; p1 = p1->next())
	{
		for (p2 = p1->program()->head; p2; p2 = p2->next())
		{
			if (p2->process()->get_pid() == pid)
			{
				last_refd = p2->process();
				return p2->process();
			}
		}
	}
	return 0;
}

Process *
Process_list::find_process(const char *name)
{
	Plink	*p1;
	Plink	*p2;

	if (last_refd && strcmp(last_refd->procname, name) == 0)
		return last_refd;

	for (p1 = head; p1; p1 = p1->next())
	{
		for (p2 = p1->program()->head; p2; p2 = p2->next())
		{
			if (strcmp(p2->process()->procname, name) == 0)
			{
				last_refd = p2->process();
				return p2->process();
			}
		}
	}
	return 0;
}

Process *
Process_list::find_process(Message *m)
{
	char	*procname;
	char	*oldname;
	char	*file;
	Word	proc;
	Word	tmp;

	switch(m->get_msg_id())
	{
	case MSG_proc_fork:
		m->unbundle(oldname, procname);
		return find_process(oldname);

	case MSG_proc_start:
	case MSG_proc_stop_fcall:
		m->unbundle(proc);
		return find_process((DBcontext)proc);

	case MSG_set_frame:
		m->unbundle(proc, tmp);
		return find_process((DBcontext)proc);

	case MSG_event_enabled:
	case MSG_event_disabled:
	case MSG_event_deleted:
		m->unbundle(tmp, proc);
		return find_process((DBcontext)proc);

	case MSG_proc_killed:
	case MSG_release_run:
	case MSG_release_suspend:
	case MSG_release_core:
		m->unbundle(procname);
		return find_process(procname);

	case MSG_jump:
		m->unbundle(proc, tmp, file, tmp);
		return find_process((DBcontext)proc);

	default:
		return find_process(m->get_dbcontext());
	}
}

Program *
Process_list::find_program(const char *name)
{
	Plink	*p1;

	if (last_refd && strcmp(last_refd->program->progname, name) == 0)
		return last_refd->program;

	for (p1 = head; p1; p1 = p1->next())
	{
		if (strcmp(p1->program()->progname, name) == 0)
			return p1->program();
	}
	return 0;
}

// Handle MSG_grab_proc and MSG_new_core from debug
void
Process_list::proc_grabbed(Message *m, Window_set *ws, int first_process)
{
	char	*procname = 0;
	char	*progname = 0;

	m->unbundle(progname, procname);
	last_refd = new Process(m->get_dbcontext(), progname, procname,
		(m->get_msg_id()==MSG_new_core) ? State_core : State_stopped, ws, TRUE,
		first_process);
	// assuming MSG_grab_proc always followed by suspension message
#if DEBUG
	dump();
#endif
}

// Handle MSG_createp from debug
Process *
Process_list::new_proc(Message *m, Window_set *ws, int first_process, int io_flag)
{
	char	*procname = 0;
	char	*progname = 0;

	m->unbundle(progname, procname);
	last_refd = new Process(m->get_dbcontext(), progname, procname,
		State_stopped, ws, FALSE, first_process);
	last_refd->program->io_redirected = io_flag;
#if DEBUG
	dump();
#endif
	return last_refd;
}

// Handle MSG_proc_fork from debug, create new process and
// update parent process
void
Process_list::proc_forked(Message *m, Process *old_proc)
{
	char		*procname = 0;
	char		*tmp1, *tmp2;

	m->unbundle(tmp1, procname);
	last_refd = new Process(m->get_dbcontext(),
		old_proc->get_program()->get_name(), procname,
		State_stopped, old_proc->window_set, FALSE, 0);

	if (in_script || has_assoc_cmd)
		old_proc->touched = TRUE;
	else
	{
		old_proc->state = State_stopped;
		old_proc->do_ps(tmp2);
		old_proc->frame = -1;
		old_proc->update_done();
	}	
#if DEBUG
	dump();
#endif
}

// Handle MSG_proc_exec from debug:
// delete process from old program and add to new one
void
Process_list::proc_execed(Message *m, Process *old_proc)
{
	Program		*old_prog = old_proc->program;
	char		*progname = 0;
	char		*procname = 0;
	char		*tmp;
	int		is_current;

	is_current = (old_proc == old_proc->window_set->current_process());
	old_prog->delete_process(old_proc);
	if (!old_prog->head)
		delete_program(old_prog);

	m->unbundle(procname, tmp, progname);
	last_refd = new Process(m->get_dbcontext(), progname, procname,
		State_stopped, old_proc->window_set, FALSE, is_current);
#if DEBUG
	dump();
#endif
}

// Handle message from debug that a process died or was released
// if there are other processes for this program, get rid of the Process
// if this is the last process for this program, get rid of both
void
Process_list::remove_proc(Process *p)
{
	Program	*prog = p->program;

	prog->delete_process(p);
	if (!prog->head)
		delete_program(prog);

	if (last_refd == p)
		last_refd = 0;
#if DEBUG
	dump();
#endif
}

// received an event notification
void
Process_list::proc_stopped(Process *p)
{
	// the message was MSG_es_something
	// this assumes those messages are always followed
	// by location and source line (or disassembly)
	p->frame = -1;
	p->incomplete = 1;
	p->state = State_stopped;
}

// Handle the second message in an event notification, giving location
void
Process_list::update_location(Message *m, Process *p)
{
	Msg_id		mtype = m->get_msg_id();
	char		*f1 = 0;
	char		*f2 = 0;

	delete p->file;
	delete p->function;
	delete p->location;
	p->file = p->function = p->location = 0;

	if (mtype == MSG_loc_sym_file)
	{
		m->unbundle(f1, f2);
		p->function = makestr(f1);
		p->file = makestr(f2);
	}
	else if (mtype == MSG_loc_sym)
	{
		m->unbundle(f1);
		p->function = makestr(f1);
	}
}

// Handle the third message in an event notification, giving full path of source file
void
Process_list::set_path(Message *m, Process *p)
{
	char	*tmp = 0;

	m->unbundle(tmp);
	delete p->path;
	p->path = makestr(tmp);
}

// handle the fourth, and last, message in an event notification,
// and notify window sets of change in process state
void
Process_list::finish_update(Message *m, Process *p)
{
	Msg_id		mtype = m->get_msg_id();
	Word		line = 0;
	Word		addr = 0;
	char		*tmp;

	if (mtype == MSG_line_src)
		m->unbundle(line, tmp);
	else if (mtype == MSG_line_no_src)
		m->unbundle(line);
	else if (mtype == MSG_disassembly)
		m->unbundle(addr, tmp);
	else if (mtype == MSG_dis_line)
		m->unbundle(line, addr, tmp);
	else
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);

	if (line)
		p->line = (int)line;
	if (p->file && line)
		p->location = makesf("%s@%d", p->file, p->line);
	else
		p->location = makesf("%#x", addr);

	if (in_script || has_assoc_cmd)
		p->touched = TRUE;
	else
		p->update_done();
}

void
Process_list::set_frame(Message *m, Process *proc)
{
	Word		tmp;
	Word		frame;

	if (in_script || has_assoc_cmd)
	{
		proc->touched = TRUE;
		return;
	}

	m->unbundle(tmp, frame);
	proc->frame = (int)frame;
	delete proc->path;
	proc->path = 0;
	proc->get_frame_info();
	proc->window_set->set_frame(proc);
}

void
Process_list::proc_jumped(Message *m, Process *proc)
{
	char	*file = 0;
	Word	tmp = 0;
	Word	addr = 0;
	Word	line = 0;

	m->unbundle(tmp, addr, file, line);
	proc->line = (int)line;
	delete proc->file;
	delete proc->location;
	delete proc->path;
	proc->file = proc->path = proc->location = 0;

	if (file && *file)
		proc->file = makestr(file);
	if (proc->file && line)
		proc->location = makesf("%s@%d", file, line);
	else
		proc->location = makesf("%x", addr);
	if (in_script || has_assoc_cmd)
		proc->touched = TRUE;
	else
		proc->update_done();
}

// this is called when a process is set in motion or when a process
// stops after being run to evaulate a function call in an expression
void
Process_list::set_state(Process *ptr, Process_state s)
{
	if (!ptr)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	// debug sends stepping message first - don't override with running message
	if (s == ptr->state || (ptr->state == State_stepping && s == State_running))
		return;

	if (s == State_running || s == State_stepping)
	{
		ptr->state = s;
		delete ptr->function;
		delete ptr->file;
		delete ptr->location;
		delete ptr->path;
		ptr->frame = -1;
		ptr->function = ptr->file = ptr->path = 0;
		ptr->line = 0;
		ptr->location = 0;
		if (in_script || has_assoc_cmd)
			ptr->touched = TRUE;
		else
			ptr->update_done();
	}
	else if (s == State_stopped)
	{
		char *tmp; // calling do_ps to get location, already have command line

		ptr->state = s;
		if (in_script || has_assoc_cmd)
			ptr->touched = TRUE;
		else
		{
			ptr->do_ps(tmp);
			ptr->update_done();
		}
	}
	else
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

}

void
Process_list::rename(Message *m)
{
	Program	*prog;
	Plink	*p;
	char	*old_name = 0;
	char	*new_name = 0;
	int	reordered = 0;

	m->unbundle(old_name, new_name);
	for (p = head; p; p = p->next())
	{
		if (strcmp(p->program()->get_name(), old_name) == 0)
			break;
	}
	if (!p)
		return;

	prog = p->program();
	delete prog->progname;
	prog->progname = makestr(new_name);

	if ((p->prev() && strcmp(p->prev()->program()->get_name(), new_name) == 1)
		|| (p->next() && strcmp(p->next()->program()->get_name(), new_name) == -1))
	{
		if (p == head)
			head = head->next();
		else if (p == tail)
			tail = tail->prev();
		p->unlink();
		delete p;
		add_program(prog);
		reordered = 1;
	}

	for (p = prog->head; p; p = p->next())
	{
		Process		*proc = p->process();
		Window_set	*ws = proc->window_set;
		int		is_current = (proc == ws->current_process());

		if (reordered)
		{
			ws->delete_process(proc);
			ws->add_process(proc, is_current);
		}
		else
			ws->rename_process(proc);
	}
#if DEBUG
	dump();
#endif
}

void
Process_list::move_process(Process *proc, Window_set *ws)
{
	Window_set	*old_ws = proc->window_set;

	old_ws->delete_process(proc);
	if (!old_ws->current_process())
		old_ws->set_current(0);

	proc->window_set = ws;
	ws->add_process(proc, 1);
}

// returns 1 if there are any live processes in any window set
int
Process_list::any_live()
{
	for (Plink *prog = head; prog; prog = prog->next())
	{
		for (Plink *proc = prog->program()->head; proc; proc = proc->next())
			if (proc->process()->get_state() >= State_running)
				return 1;
	}
	return 0;
}

void
Process_list::clear_program_list()
{
	Plink	*p1;

	for (p1 = head; p1; p1 = p1->next())
		p1->program()->set_used_flag(0);
}

#if DEBUG
void
Process_list::dump()
{
	Plink	*p1;
	Plink	*p2;
	Process	*proc;

	debug("\nProcess list:\n");
	for (p1 = head; p1; p1 = p1->next())
	{
		debug("%s\n", p1->program()->get_name());
		for (p2 = p1->program()->head; p2; p2 = p2->next())
		{
			proc = p2->process();
			debug("\t%s id=%#x, state=%d, function=%s, file=%s line=%d ws=%d\n",
				proc->get_name(), proc->get_id(), proc->get_state(),
				proc->get_function(), proc->get_file(),
				proc->get_line(),
				proc->get_window_set()->get_id());
		}
	}
}
#endif

static Buffer buf;

const char *
make_plist(int total, Process **p, int use_blanks, int level)
{
	if (level == PROGRAM_LEVEL)
		proclist.clear_program_list();

	buf.clear();
	for (int i = 0; i < total; i++)
	{
		if (i)
		{
			if (use_blanks)
				buf.add(", ");
			else
				buf.add(',');
		}
		if (level == PROGRAM_LEVEL)
		{
			if (!p[i]->get_program()->get_used_flag())
			{
				p[i]->get_program()->set_used_flag(1);
				buf.add(p[i]->get_program()->get_name());
			}
		}
		else
			buf.add(p[i]->get_name());
	}
	return (char *)buf;
}

// update_all is called after running a script.
void
Process_list::update_all()
{
	Message	*m;
	Plink	*prog_link = head;
	Plink	*proc_link = 0;
	Process	*process;

	if (head)
		proc_link = head->program()->get_head();
	dispatcher.query(0, 0, "ps\n");
	while ((m = dispatcher.get_response()) != 0)
	{
		Process_state	state;
		char		*current = 0;
		char		*progname = 0;
		char		*procname = 0;
		char		*function = 0;
		char		*location = 0;
		char		*cmdline = 0;
		Word		pid = 0;

		// current indicator, progname, procname, pid, func,
		//	 location, cmdline

		switch(m->get_msg_id())
		{
		case MSG_ps_running:
			state = State_running;
			m->unbundle(current, progname, procname, pid, cmdline);
			break;

		case MSG_ps_stepping:
			state = State_stepping;
			m->unbundle(current, progname, procname, pid, cmdline);
			break;

		case MSG_ps_stopped:
			state = State_stopped;
			m->unbundle(current, progname, procname, pid, function,
				location, cmdline);
			break;

		case MSG_ps_core:
			state = State_core;
			m->unbundle(current, progname, procname, pid, function,
				location, cmdline);
			break;

		case MSG_ps_dead:
		case MSG_ps_release:
		case MSG_ps_header:
		case ERR_no_proc:
			continue;

		case MSG_ps_unknown:
		default:
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			continue;
		}

		if (!proc_link || (process = proc_link->process()) == 0)
			continue;

		// There may be processes in the ps that are not yet in the
		// gui's process list or vice versa - since debug is ahead 
		// of the gui, processes may have died, forked, etc, and the
		// messages are still in the gui's message queue.  Ignore
		// those processes for now, they will be brought up to date
		// as the messages are read
		int val = strcmp(process->get_name(), procname);
		if (val > 0)
			continue;

		if (val == 0 && (process->touched == TRUE))
		{
			process->update_location(function, location);
			if (!process->program->cmd_line)
				process->program->cmd_line = makestr(cmdline);
		}

		if (proc_link->next())
			proc_link = proc_link->next();
		else if (prog_link && prog_link->next())
		{
			prog_link = prog_link->next();
			proc_link = prog_link->program()->head;
		}
		else
			proc_link = 0;
	}

	// wait until the query is done to tell the window set the process is
	// updated, since the window set may also need to do a query to get more
	// information
	for (prog_link = head; prog_link; prog_link = prog_link->next())
	{
		for (proc_link = prog_link->program()->head; proc_link;
			proc_link = proc_link->next())
		{
			process = proc_link->process();
			if (process->touched)
			{
				process->get_frame_info();
				process->update_done();
			}
		}
	}
}
