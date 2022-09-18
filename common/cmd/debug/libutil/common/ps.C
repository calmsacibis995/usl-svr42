#ident	"@(#)debugger:libutil/common/ps.C	1.2"

#include "utility.h"
#include "LWP.h"
#include "Proglist.h"
#include "Program.h"
#include "global.h"
#include "Interface.h"
#include "Procctl.h"
#include "Parser.h"
#include <signal.h>
#include <sys/types.h>
#include <stdio.h>

#define NAME_LEN	10	// length of program name for output

static void
print_status(LWP *lwp)
{
	const char	*pname;
	char		*func_name = 0;
	char		*file = 0;
	long		line = 0;
	Execstate	state;
	const char	*current = " ";	// assume non-current
	Procstat	pstat;
	int		what, why;
	char		location[16];

	// add '*' to denote current lwp
	if (lwp == proglist.current_lwp())
		current = "*";

	pname = lwp->prog_name();
	state = lwp->get_state();
	if (state != es_corefile)
		pstat = lwp->proc_ctl()->status(what, why);
	else
		pstat = p_unknown;
	if (state != es_corefile && pstat == p_dead)
		printm(MSG_ps_dead,pname,lwp->process()->program()->command());
	else if (state == es_corefile || pstat == p_stopped)
	{
		current_loc(lwp, 0, file, func_name, line);
		if (!func_name)
			func_name = "";

		if (line && file)
		{
			// use basename
			char	*fptr = strrchr(file, '/');
			if (fptr)
				file = fptr + 1;
			sprintf(location, "%.10s@%d", file, line);
		}
		else
		{
			sprintf(location, "%#x", lwp->pc_value());
		}
		printm((state == es_corefile) ? MSG_ps_core : MSG_ps_stopped,
			current, pname, lwp->lwp_name(),
			lwp->process()->pid(), func_name, location, 
			lwp->process()->program()->command());
	}
	else if (state == es_running)
		printm(MSG_ps_running, current, pname, 
			lwp->lwp_name(), lwp->process()->pid(),
			lwp->process()->program()->command());
	else if (state == es_stepping)
		printm(MSG_ps_stepping, current, pname, 
			lwp->lwp_name(), lwp->process()->pid(),
			lwp->process()->program()->command());
	else 
		printm(MSG_ps_unknown, current, pname, 
			lwp->lwp_name(), lwp->process()->pid(),
			lwp->process()->program()->command());
}

int 
proc_status(Proclist *procl)
{
	LWP	*lwp;
	plist	*list;
	Program	**protolist;
	Program	*prog;

	if (procl)
	{
		list = proglist.proc_list(procl, protolist);
	}
	else
	{
		list = proglist.all_procs(protolist);
	}
	lwp = list->p_lwp;
	lwp = list++->p_lwp;
	prog = *protolist++;
	if (!lwp && !prog)
	{
		printe(ERR_no_proc, E_WARNING);
		return 0;
	}
	sigrelse(SIGINT);
	printm(MSG_ps_header);

	// We have lists of live procs and proto programs; walk them
	// in parallel and print in alphabetical order.
	// Each list is already internally alphabetized.

	while(lwp && prog &&
		!(interrupt & sigbit(SIGINT)))
	{
		if (strcmp(lwp->prog_name(), prog->prog_name()) < 0)
		{
			print_status(lwp);
			lwp = list++->p_lwp;
		}
		else
		{
			printm(MSG_ps_dead,prog->prog_name(),prog->exec_name());
			prog = *protolist++;
		}
	}
	// print remaining entries - one list already empty
	for(; lwp;  lwp = list++->p_lwp)
	{
		if (interrupt & sigbit(SIGINT))
			break;
		print_status(lwp);
	}
	for(; prog;  prog = *protolist++)
	{
		if (interrupt & sigbit(SIGINT))
			break;
		printm((prog->mode() == P_RELEASE) ? MSG_ps_release :
			MSG_ps_dead, prog->prog_name(), 
			prog->exec_name());
	}
	sighold(SIGINT);
	return 1;
}
