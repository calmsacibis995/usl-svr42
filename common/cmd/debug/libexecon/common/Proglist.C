/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */
#ident	"@(#)debugger:libexecon/common/Proglist.C	1.4"

#include "Proglist.h"
#include "Program.h"
#include "Process.h"
#include "Parser.h"
#include "LWP.h"
#include "Interface.h"
#include "str.h"
#include "utility.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "Dbgvarsupp.h"

// Routines to manage lists of programs, processes and lwps
// and to keep track of process identifiers and current program,
// proc, lwp.

#define	LIVE_ONLY	0
#define LIVE_CORE	1
#define ALL		2

Proglist	proglist;  // global program list

Proglist::Proglist()
{
	memset(this, 0, sizeof(*this));
	all_name = str("all");
	prog_name = str("%program");
	proc_name = str("%proc");
	lwp_name = str("%lwp");
}

void
Proglist::add_program(Program *p)
{
	Program	*prog = 0;

	if (p->is_proto())
	{
		// add proto programs at end
		if (last_program)
			p->append(last_program);
		last_program = p;
		if (!first_program)
			first_program = last_program;
			
		return;
	}
	// Maintain programs in alphabetical order
	// for ps command.
	if (first_program)
	{
		for (prog = first_program; prog; prog = prog->next())
		{
			if (prog->prog_name() &&
				(strcmp(prog->prog_name(), 
					p->prog_name()) > 0))
				break;
		}
		if (prog)
			p->prepend(prog);
		else
		{
			p->append(last_program);
			last_program = p;
		}
		if (first_program == prog)
			first_program = p;
	}
	else
	{
		last_program = p;
		first_program = last_program;
	}
}

void
Proglist::remove_program(Program *prog)
{
	if (prog == last_program)
		last_program = (Program *)prog->prev();
	if (prog == first_program)
		first_program = prog->next();
	prog->unlink();
	delete prog;
}

// Given a -p list, return pointer to a list of lwps.
// Each returned list item also indicates whether the original
// request reflected the entire program, process or just
// an lwp, and whether the lwp is for a corefile.
//
// We start with a fixed size malloc'd list and realloc if 
// necessary.
// 
// We always return a plist.  If no lwps were found, the first
// list entry is null

void
Proglist::setup_list()
{
	prune();	// get rid of dead wood in list
	if (!lwp_list)
	{
		grow_list();
	}
	if (cur_plist)
	{
		// already an active list - start a new one
		if (cnt >= list_size) 
			grow_list();
		// Skip over null of last list
		cnt++;
		cur_plist = lwp_list + cnt;
	}
	else
	{
		cnt = 0;
		cur_plist = lwp_list;
	}
	plist_head = cur_plist;
}

plist *
Proglist::proc_list(Proclist *procs)
{
	char	*cur_item = (*procs)[0];
	int	list_index = 0;

	setup_list();
	while(cur_item)
	{
		parse_item(cur_item, LIVE_CORE);
		cur_item = ((*procs)[++list_index]);
	}
	cur_plist->p_lwp = 0;
	return plist_head;
}

// This version of proc_list also returns any proto-programs
// that have been requested, either through -p all or -p progname.
// The protolist, like the lwp_list starts at at a constant
// size and is realloc'd as needed.
plist *
Proglist::proc_list(Proclist *procs, Program **&prlist)
{
	char	*cur_item = (*procs)[0];
	int	list_index = 0;

	setup_list();
	proto_cnt = 0;
	if (!protolist)
		grow_proto_list();
	cur_proto_list = protolist;
	while(cur_item)
	{
		parse_item(cur_item, ALL);
		cur_item = ((*procs)[++list_index]);
	}
	cur_plist->p_lwp = 0;
	*cur_proto_list = 0;
	prlist = protolist;
	return plist_head;
}

// all lwps in a given program
plist *
Proglist::proc_list(Program *prog)
{
	setup_list();
	if (prog)
		add_prog(prog, LIVE_CORE);
	cur_plist->p_lwp = 0;
	return plist_head;
}

// all lwps, including core files and proto programs
plist *
Proglist::all_procs(Program **&prlist)
{
	setup_list();
	proto_cnt = 0;
	if (!protolist)
		grow_proto_list();
	cur_proto_list = protolist;
	for(Program *p = first_program; p; p = p->next())
		add_prog(p, ALL);
	cur_plist->p_lwp = 0;
	*cur_proto_list = 0;
	prlist = protolist;
	return plist_head;
}

// all live lwps, excluding core files
plist *
Proglist::all_live()
{
	setup_list();
	for(Program *p = first_program; p; p = p->next())
		add_prog(p, LIVE_ONLY);
	cur_plist->p_lwp = 0;
	return plist_head;
}

// all live lwps, excluding core files
// only those matching the given create session id are listed
plist *
Proglist::all_live(int id)
{
	setup_list();
	for(Program *p = first_program; p; p = p->next())
	{
		if (p->create_id() == id)
			add_prog(p, LIVE_ONLY);
	}
	cur_plist->p_lwp = 0;
	return plist_head;
}

// give clients a way to build their own lists of lwps
plist *
Proglist::make_list()
{
	setup_list();
	user_list = 1;
	cur_plist->p_lwp = 0;
	return plist_head;
}

void
Proglist::add_list(LWP *lwp)
{
	if (!user_list)
	{
		printe(ERR_internal, E_ERROR, "Proglist::add_list",
			__LINE__);
		return;
	}
	add_lwp(0, lwp, 0);
	cur_plist->p_lwp = 0;
}

void
Proglist::parse_item(char * item, int proto)
{
	Process		*proc;
	Program		*prg;
	const char	*ename;

	ename = str(item);
	// determine type of current item
	if (ename == prog_name)	// %program
	{
		if (curr_program)
			add_prog(curr_program, LIVE_CORE);
		return;
	}
	else if (ename == proc_name) // %proc
	{
		if (curr_process)
			add_proc(curr_process, PPROC);
		return;
	}
	else if (ename == lwp_name) // %lwp
	{
		if (curr_lwp)
			add_lwp(0, curr_lwp, 0);

		return;
	}
	else if (ename == all_name)	// "all"
	{
		for(prg = first_program; prg; prg = prg->next())
		{
			add_prog(prg, proto);
		}
		return;
	}
	else if (*item == '$')
	{
		static char *expanding_variable;
		if (expanding_variable)
		{
			printe(ERR_nested_variable, E_ERROR, item, 
				expanding_variable);
			return;
		}
		expanding_variable = item;
		Debug_var_support * user_var_table;
		user_var_table = new Debug_var_support(0,0,0,0,1);
		user_var_table->Find(item);
		char * value = user_var_table->Value();
		if (!value)
		{
			printe(ERR_unknown_debug_var, E_ERROR, item);
			return;
		}

		// Don't need to copy value (Debug_var_support is not
		// reentrant) because recursion is caught.
		while (*value)
		{
		    char * next_item = value;
		    while (*value && *value != ',') value++;
		    if (*value)
		    {
		    	*value = 0;
			value ++;
		    }
		    parse_item(next_item, proto);
		}
		delete user_var_table;
		expanding_variable = 0;
		return;
	}
	else if (isdigit(*item))
	{
		// process id
		char	*p;
		pid_t	 ppid = (pid_t)strtol(item, &p, 10);
		if (!*p)
		{
			for(prg = first_program; prg; 
				prg = prg->next())
			{
				proc = prg->proclist();
				for(; proc; proc = proc->next())
				{
					if (proc->pid() == ppid)
					{
						add_proc(proc, PPROC);
						return;
					}
				}
			}
		}
		// FALLTHROUGH
	}
	// process, lwp or program name
	const char	*pname, *lname;
	if (parse_name(item, pname, lname))
	{
		// possibly proc or lwp name
		for(prg = first_program; prg; prg = prg->next())
		{
			proc = prg->proclist();
			for(; proc; proc = proc->next())
			{
				if (pname == proc->proc_name())
				{
					if (lname)
						add_lwp(proc, 0,
						lname);
					else
						add_proc(proc,
						PPROC);
					return;
				}


			}
		}
		// FALLTHROUGH
	}
	// just a name
	prg = first_program;
	for(; prg; prg = prg->next())
	{
		if (ename == prg->prog_name())
		{
			add_prog(prg, proto);
			return;
		}
	}
	// if here, no match
	printe(ERR_no_match, E_WARNING, ename);
}

// try parsing name as process or lwp id (e.g., p123 or p123.124)
int
Proglist::parse_name(char *name, const char *&pname, 
	const char *&lname)
{
	char	*p;
	char	*dot = 0;

	if (!name || *name != 'p')
		return 0;
	p = name;
	while(*++p)
	{
		if (!isdigit(*p))
		{
			if (*p == '.')
			{
				if (dot)
					break;
				else
					dot = p;
			}
			else
				break;
		}
	}
	if (*p) // perhaps a program name
		return 0;
	if (dot)
	{
		char	tmp = *dot;
		lname = str(name);
		*dot = 0;
		pname = str(name); // up to '.'
		*dot = tmp;
	}
	else
	{
		lname = 0;
		pname = str(name);
	}
	return 1;
}

// Sub-calls inlined here for efficiency.
// This code is executed frequently and must be reasonably fast.

// If proto is set and program is a proto program, we add it to
// the protolist.
void
Proglist::add_prog(Program *prg, int mode)
{
	if (prg->is_proto() && (mode == ALL))
	{
		// always need room for next item and null entry
		if (proto_cnt > proto_list_size - 2)
		{
			grow_proto_list();
			cur_proto_list = protolist + proto_cnt;
		}
		*cur_proto_list++ = prg;
		proto_cnt++;
		return;
	}
	for(Process *proc = prg->proclist(); proc;
		proc = proc->next())
	{
		if ((mode == LIVE_ONLY) && proc->is_core())
			continue;
		for(LWP *lwp = proc->lwp_list(); lwp;
			lwp = lwp->next())
		{
			Execstate	state;
			if ((state = lwp->get_state()) == es_dead)
				continue;
			// always need room for next item and null entry
			if (cnt > list_size-2)
			{
				int head_cnt = plist_head - lwp_list;
				grow_list();
				plist_head = lwp_list+ head_cnt;
				cur_plist = lwp_list + cnt;
			}
			cur_plist->p_lwp = lwp;
			if (state == es_corefile)
				cur_plist->p_type = PEXEC|PLWP|PCORE;
			else
				cur_plist->p_type = PEXEC|PLWP;
			cnt++;
			cur_plist++;
		}
	}
}

void
Proglist::add_proc(Process *proc, int type)
{
	int		ltype = type|PLWP;

	for(LWP *lwp = proc->lwp_list(); lwp; lwp = lwp->next())
	{
		Execstate	state;

		if ((state = lwp->get_state()) == es_dead)
			continue;
		// always need room for next item and null entry
		if (cnt > list_size-2)
		{
			int head_cnt = plist_head - lwp_list;
			grow_list();
			plist_head = lwp_list+ head_cnt;
			cur_plist = lwp_list + cnt;
		}
		cur_plist->p_lwp = lwp;
		if (state == es_corefile)
			cur_plist->p_type = ltype|PCORE;
		else
			cur_plist->p_type = ltype;
		cnt++;
		cur_plist++;
	}
}

void
Proglist::add_lwp(Process *proc, LWP *lwp, const char *lname)
{
	Execstate	state;
	LWP		*l;

	if (lwp)
	{
		if ((state = lwp->get_state()) == es_dead)
			return;
		l = lwp;
	}
	else
	{
		for(l = proc->lwp_list(); l; l = lwp->next())
		{
			if ((l->lwp_name() == lname) &&
				((state = l->get_state()) != es_dead))
				break;
		}
		if (!l)
			return;
	}
	
	// always need room for next item and null entry
	if (cnt > list_size-2)
	{
		int head_cnt = plist_head - lwp_list;
		grow_list();
		plist_head = lwp_list + head_cnt;
		cur_plist = lwp_list + cnt;
	}
	cur_plist->p_lwp = l;
	if (state == es_corefile)
		cur_plist->p_type = PLWP|PCORE;
	else
		cur_plist->p_type = PLWP;
	cnt++;
	cur_plist++;
}


#define PROTO_LIST_ITEMS	50

// add PROTO_LIST_ITEMS entries to list
void
Proglist::grow_proto_list()
{
	proto_list_size += PROTO_LIST_ITEMS;
	if (!protolist)
	{
		// original list growth
		protolist = (Program **)
			malloc(PROTO_LIST_ITEMS * sizeof(Program *));
	}
	else
	{
		protolist = (Program **)
			realloc(protolist, proto_list_size *
				sizeof(Program *));
	}
	if (!protolist)
	{
		new_handler();
	}
}

#define LIST_ITEMS	100

// add LIST_ITEMS entries to list
void
Proglist::grow_list()
{
	list_size += LIST_ITEMS;
	if (!lwp_list)
	{
		// original list growth
		lwp_list = (plist *)
			malloc(LIST_ITEMS * sizeof(plist));
	}
	else
	{
		lwp_list = (plist *)
			realloc(lwp_list, list_size *
				sizeof(plist));
	}
	if (!lwp_list)
	{
		new_handler();
	}
}

void
Proglist::set_current(LWP *lwp, int announce)
{
	if (!lwp || lwp->get_state() == es_dead)
	{
		curr_process = 0;
		curr_lwp = 0;
		curr_program = 0;
		printe(ERR_internal, E_ERROR, "Proglist:set_current",
			__LINE__);
		return;
	}
	curr_lwp = lwp;
	curr_process = lwp->process();
	curr_program = curr_process->program();
	if (announce)
	{
		printm(MSG_new_context, lwp->lwp_name(), 
			lwp->prog_name());
	}
}

void
Proglist::set_current(Process *proc, int announce)
{
	LWP	*l;

	if (!proc || ((l = proc->first_lwp()) == 0))
	{
		curr_process = 0;
		curr_lwp = 0;
		curr_program = 0;
		printe(ERR_internal, E_ERROR, "Proglist:set_current",
			__LINE__);
		return;
	}
	curr_lwp = l;
	curr_process = proc;
	curr_program = proc->program();
	if (announce)
	{
		printm(MSG_new_context, l->lwp_name(), 
			l->prog_name());
	}
}

void
Proglist::set_current(Program *prog, int announce)
{
	LWP	*l;

	if (!prog || ((l = prog->first_lwp()) == 0))
	{
		curr_process = 0;
		curr_lwp = 0;
		curr_program = 0;
		printe(ERR_internal, E_ERROR, "Proglist:set_current",
			__LINE__);
		return;
	}
	curr_lwp = l;
	curr_process = l->process();
	curr_program = prog;
	if (announce)
	{
		printm(MSG_new_context, l->lwp_name(), 
			l->prog_name());
	}
}

// find LWP associated with a given name; if name is a program
// or process name, find first active lwp
LWP *
Proglist::find_lwp(char *name)
{
	const char	*pname, *lname;
	const char	*ename;

	ename = str(name);
	// determine type of current item
	if (ename == prog_name)	// %program
	{
		if (curr_program)
			return(curr_program->first_lwp());
		else
			return 0;
	}
	else if (ename == proc_name) // %proc
	{
		if (curr_process)
			return(curr_process->first_lwp());
		else
			return 0;
	}
	else if (ename == lwp_name) // %lwp
	{
		return(curr_lwp);
	}
	else if (*ename == '$')
	{
		Debug_var_support * user_var_table;
		user_var_table = new Debug_var_support(0,0,0,0,1);
		user_var_table->Find((char *)ename);
		name = user_var_table->Value();
		if (!name)
		{
			printe(ERR_unknown_debug_var, E_ERROR, ename);
			return 0;
		}
		delete user_var_table;
		// FALLTHROUGH
	}
	if (isdigit(*name))
	{
		char	*p;
		LWP	*l;
		pid_t	pid = (pid_t)strtol(name, &p, 10);

		if (!*p)
			if ((l = find_lwp(pid)) != 0)
				return l;
		// FALLTHROUGH
	}
	if (parse_name(name, pname, lname))
	{
		// might be process or lwp id
		for(Program *prog = first_program; prog; 
			prog = prog->next())
		{
			for(Process *proc = prog->proclist(); proc; 
				proc = proc->next())
			{
				if (proc->proc_name() == pname)
					break;
			}
			if (proc)
			{
				if (!lname)
					return(proc->first_lwp());
				for(LWP *l = proc->lwp_list(); 
					l; l = l->next())
				{
					if ((l->lwp_name() == lname)
					&& (l->get_state() != es_dead))
						return l;
				}
			}
		}
		// FALLTHROUGH
	}
	// program
	Program	*prog;
	if ((prog = find_prog(name)) == 0)
		return 0;
	return(prog->first_lwp());
}

// find first active lwp associated with a given process id
LWP *
Proglist::find_lwp(pid_t pid)
{
	Process	*proc;
	for(Program *prog = first_program; prog; prog = prog->next())
	{
		for(proc = prog->proclist(); proc; proc = proc->next())
		{
			if ((proc->pid() == pid) &&
				(proc->count() > 0))
				return(proc->first_lwp());
		}
	}
	return 0;
}

Program *
Proglist::find_prog(const char *name)
{
	char	*p = str(name);
	for(Program *prog = first_program; prog; prog = prog->next())
	{
		// check for proto program
		if ((prog->prog_name() == p) && !prog->is_proto())
			return prog;
	}
	return 0;
}

// assumes pid or process identifier
Process *
Proglist::find_proc(const char *name)
{

	pid_t	pid;
	char	*p = 0;
	char	*s;

	if (*name == 'p')
		p = str(name);
	else
	{
		pid = (pid_t)strtol(name, &s, 10);
		if (*p)
			return 0;
	}
		
	for(Program *prog = first_program; prog; prog = prog->next())
	{
		for(Process *proc = prog->proclist(); proc; proc = proc->next())
			if (p)
			{
				if ((proc->proc_name() == p) && 
					(proc->count() > 0))
				return proc;
			}
			else
			{
				if ((proc->pid() == pid) && 
					(proc->count() > 0))
				return proc;
			}
	}
	return 0;
}

// Go through list of programs, processes, lwps.  Delete
// all dead lwps, all processes that have no live lwps
// and all programs that have no live processes.
void
Proglist::prune()
{
	Program *prog = first_program; 
	while(prog)
	{
		Process	*proc = prog->proclist();
		while(proc)
		{
			LWP	*lwp = proc->lwp_list();
			while(lwp)
			{
				if (lwp->get_state() == es_dead) // dead lwp
				{
					LWP	*tlwp = lwp;
					lwp = lwp->next();
					proc->remove_lwp(tlwp);
					continue;
				}
				lwp = lwp->next();
			}
			if (!proc->lwp_list())			// dead process
			{
				Process	*tproc = proc;
				proc = proc->next();
				prog->remove_proc(tproc);
				continue;
			}
			proc = proc->next();
		}
		if (!prog->proclist())				// dead program
		{
			if (!prog->events()) // don't remove proto prog
			{
				Program	*tprg = prog;
				prog = prog->next();
				remove_program(tprg);
				continue;
			}
		}
		prog = prog->next();
	}
}

// Reset notion of current program, process, lwp.
void
Proglist::reset_current(int announce)
{
	Process	*proc = curr_process;
	Program	*prog = curr_program;
	LWP	*lwp  = 0;

	// First look for a sibling of current lwp.
	if (curr_process)
		for(lwp = curr_process->lwp_list(); lwp; lwp = lwp->next())
		{
			if ((lwp != curr_lwp) && (lwp->get_state() != es_dead))
				goto out;
		}
	// Next look for a sibling process.
	if (curr_program)
		for(proc = curr_program->proclist(); proc; proc = proc->next())
		{
			if (proc == curr_process)
				continue;
			if ((lwp = proc->first_lwp()) != 0)
				goto out;
		}
	// Now look for a sibling program.
	lwp = 0;
	for(prog = first_program; prog; prog = prog->next())
	{
		if (prog == curr_program)
			continue;
		if ((lwp = prog->first_lwp()) != 0)
			goto out;
	}
out:
	if (!lwp)
	{
		curr_lwp = 0;
		curr_process = 0;
		curr_program = 0;
		if (announce)
			printm(MSG_no_process);
		return;
	}
	curr_lwp = lwp;
	curr_process = lwp->process();
	curr_program = curr_process->program();
	if (announce)
		printm(MSG_new_context, curr_process->proc_name(), 
			curr_program->prog_name());
}

#ifdef DEBUG
void
Proglist::print_list()
{
	printf("Current program %#x Current process %#x current lwp %#x\n",
		curr_program, curr_process, curr_lwp);
	for(Program *prg = first_program; prg; prg = prg->next())
	{
		printf("Program: %#x	", prg);
		if (prg->is_proto())
			printf("proto: %s\n", prg->exec_name()? prg->exec_name():
				"?");
		else
			printf("%s\n", prg->prog_name()? prg->prog_name(): "?");
		for(Process *proc = prg->proclist(); proc; proc = proc->next())
		{
			printf("\tProcess: %#x	", proc);
			printf("%s\n", proc->proc_name()? proc->proc_name(): "?");
			for(LWP *l = proc->lwp_list(); l; l = l->next())
			{
				printf("\t\tLWP: %#x	", l);
				if (l->get_state() == es_dead)
					printf("<dead> ");
				printf("%s\n", l->lwp_name()? l->lwp_name(): "?");
			}
		}
	}
}
#endif
