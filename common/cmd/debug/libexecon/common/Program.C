#ident	"@(#)debugger:libexecon/common/Program.C	1.1"

// Program control.
// Control all processes and LWPs derived from a single
// executable.

#include "Program.h"
#include "Process.h"
#include "Proglist.h"
#include "LWP.h"
#include "PtyList.h"
#include <sys/types.h>
#include <signal.h>
#include <string.h>

Program::Program(Process *proc, const char *execname, const char *pname, 
	const char *args, PtyInfo *cio, time_t stime, int id)
{
	proto_mode = 0;
	first_proc = last_proc = proc;
	ename = new(char[strlen(execname) + 1]);
	strcpy((char *)ename, execname);
	progname = pname;
	arguments = new(char[strlen(args) + 1]);
	strcpy((char *)arguments, args);
	child_io = cio;
	_symfiltime = stime;
	createid = id;
	etable = 0;
	srcpath = 0;
	path_age = 0;
	namecnt = 0;
	proglist.add_program(this);
}

// constructor for proto programs - keep track of event tables
Program::Program(EventTable *et, const char *pname, 
	const char *execname, int mode)
{
	ename = new(char[strlen(execname) + 1]);
	strcpy((char *)ename, execname);
	etable = et;
	progname = pname;
	proto_mode = (char)mode;
	first_proc = last_proc = 0;
	arguments = 0;
	child_io = 0;
	_symfiltime = 0;
	createid = 0;
	srcpath = 0;
	path_age = 0;
	namecnt = 0;
	proglist.add_program(this);
}

Program::~Program()
{
	if (child_io)
		cleanup_childio();
}

void
Program::add_proc(Process *proc)
{
	if (last_proc)
		proc->append(last_proc);
	last_proc = proc;
	if (!first_proc)
		first_proc = last_proc;
}

void
Program::remove_proc(Process *proc, int nodelete)
{
	if (proc == last_proc)
		last_proc = (Process *)proc->prev();
	if (proc == first_proc)
		first_proc = proc->next();
	proc->unlink();
	if (!nodelete)
		delete proc;
}


void
Program::cleanup_childio()
{
	// allow SIGPOLL to pick up any last pending I/O
	sigrelse(SIGPOLL);
	if (child_io->dec_count() == 0)
		delete child_io;
	sighold(SIGPOLL);
}

void
Program::rename(const char *name)
{
	progname = name;
	// program names are stored both at program and lwp
	for(Process *proc = first_proc; proc; proc = proc->next())
	{
		for(LWP *l = proc->lwp_list(); l; l = l->next())
			l->rename(name);
	}
}

// return first active lwp assocated with this program
LWP *
Program::first_lwp()
{
	for (Process *proc = first_proc; proc; proc = proc->next())
	{
		for(LWP *l = proc->lwp_list(); l ; l = l->next())
		{
			if (l->get_state() != es_dead)
				return l;
		}
	}
	return 0;
}
