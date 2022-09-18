#ident	"@(#)debugger:libexecon/common/Process.C	1.1"

// Process control.
// Control all LWPs derived from a single address space.

#include "Process.h"
#include "LWP.h"
#include "Procctl.h"
#include "Seglist.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/procfs.h>

Process::Process(LWP *lwp, pid_t pid, const char *pname)
{
	procname = pname;
	ppid = pid;
	_first_lwp = last_lwp = lwp;
	if (lwp)
	{
		next_lwp = lwp_cnt = 1;
	}
	else
	{
		next_lwp = lwp_cnt = 0;
	}
	_program = 0;
	core = 0;
	textctl = 0;
	seglist = 0;
}

Process::~Process()
{
	if (textctl)
	{
		textctl->close();
		delete textctl;
	}
	if (core)
	{
		core->close();
		delete core;
	}
	delete seglist;
}

void
Process::add_lwp(LWP *lwp)
{
	lwp_cnt++;
	next_lwp++;
	if (last_lwp)
		lwp->append(last_lwp);
	last_lwp = lwp;
	if (!_first_lwp)
		_first_lwp = last_lwp;
}

void
Process::remove_lwp(LWP *lwp)
{
	if (lwp == last_lwp)
		last_lwp = (LWP *)lwp->prev();
	if (lwp == _first_lwp)
		_first_lwp = lwp->next();
	lwp->unlink();
	delete lwp;
}

// return first active lwp assocated with this process
LWP *
Process::first_lwp()
{
	for(LWP *l = _first_lwp; l ; l = l->next())
	{
		if (l->get_state() != es_dead)
			return l;
	}
	return 0;
}
