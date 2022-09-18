#ident	"@(#)debugger:libutil/common/destroy.C	1.1"

#include "utility.h"
#include "Procctl.h"
#include "Interface.h"
#include "LWP.h"
#include "Proglist.h"
#include "Parser.h"
#include <errno.h>
#include <string.h>
#include <signal.h>

int
destroy_process(LWP *lwp, int announce)
{
	if ( lwp == 0 )
	{
		return 0;
	}
	if (lwp->get_state() == es_dead)
		return 1;
	if (lwp->get_state() == es_corefile) 
	{
		lwp->markdead();
	}
	else
	{
		if ( lwp->proc_ctl()->stop() == 0 )
		{
			return 0;
		}
		lwp->markdead();
		if ( lwp->make_proto(P_KILL) == 0 )
		{
			return 0;
		}
		if ( lwp->proc_ctl()->kill( SIGKILL ) == 0 )
		{
			printe(ERR_sys_kill, E_ERROR, lwp->prog_name(), strerror(errno));
			return 0;
		}
	}
	if (announce)
		printm(MSG_proc_killed, lwp->proc_name());
	return 1;
}


int
destroy_process(Proclist *procl, int announce)
{
	int 	single = 1;
	int 	current_changed = 0;
	LWP	*lwp;
	plist	*list;
	int	ret = 1;

	if (procl)
	{
		single = 0;
		list = proglist.proc_list(procl);
		lwp = list++->p_lwp;
	}
	else
	{
		lwp = proglist.current_lwp();
	}
	if (!lwp)
	{
		printe(ERR_no_proc, E_ERROR);
		return 0;
	}
	do
	{
		if (lwp == proglist.current_lwp())
			current_changed = 1;

		if (!destroy_process(lwp, announce))
		{
			ret = 0;
			continue;
		}
	}
	while(!single && ((lwp = list++->p_lwp) != 0));

	if (current_changed)
		proglist.reset_current(1);
	proglist.prune();
	return ret;
}

void
destroy_all()
{
	plist		*list = proglist.all_live();
	// excludes core files

	for (LWP *l = list++->p_lwp; l; l = list++->p_lwp)
	{
		Execstate state = l->get_state();
		if ((state == es_dead) || (state == es_none))
			continue;
		if (l->is_child()) 
		{
			destroy_process(l, 0);
		} 
		else 
		{
			if (state == es_corefile)
				continue;
			release_process(l, 1);
		}
	}
	proglist.prune();
}
