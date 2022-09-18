#ident	"@(#)debugger:libutil/common/rel_proc.C	1.1"
#include "utility.h"
#include "Procctl.h"
#include "LWP.h"
#include "Proglist.h"
#include "Interface.h"
#include "Parser.h"

int
release_process( LWP * lwp, int run )
{
	if (lwp == 0)
		return 0;
	Execstate state = lwp->get_state();
	if (state == es_dead)
	{
		printe(ERR_invalid_op_dead, E_ERROR, lwp->lwp_name());
		return 0;
	}
	else if (state == es_corefile)
	{
		lwp->markdead();
		printm(MSG_release_core, lwp->lwp_name());
		return 1;
	}
	if (lwp->proc_ctl()->stop() == 0 )
		return 0;
	if ( lwp->make_proto(P_RELEASE) == 0 )
	{
		return 0;
	}
	if (run && (lwp->drop_run() == 0))
		return 0;
	lwp->markdead();
	if (run)
		printm(MSG_release_run, lwp->lwp_name());
	else
		printm(MSG_release_suspend, lwp->lwp_name());
	return 1;
}

int
release_proclist( Proclist *procl, int run)
{
	int	single = 1, rel_current = 0;
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
			rel_current = 1;
		if (!release_process(lwp, run))
			ret = 0;
	}
	while(!single && ((lwp = list++->p_lwp) != 0));

	if (rel_current)
	{
		proglist.reset_current(1);
	}
	proglist.prune();
	return ret;
}
