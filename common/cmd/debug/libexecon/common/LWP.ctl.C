#ident	"@(#)debugger:libexecon/common/LWP.ctl.C	1.4"

#include "LWP.h"
#include "EventTable.h"
#include "Interface.h"
#include "Machine.h"
#include "Procctl.h"
#include "Instr.h"
#include "Watchlist.h"
#include "Seglist.h"

#include <string.h>
#include <errno.h>

int
LWP::run( int clearsig, Iaddr destaddr, int talk_ctl )
{
	int	ret;
	if (sw_watch > 0)
	{
		printe(ERR_step_watch, E_WARNING, lwp_name());
		return stmt_step(STEP_INTO, clearsig, destaddr, 
			STEP_INF_QUIET, talk_ctl);
	}
	if ( clearsig ) latestsig = 0;
	if ( (state == es_running) || (state == es_stepping) )
	{
		printe(ERR_running_proc, E_ERROR, lwp_name());
		return 0;
	}
	else if ( state == es_corefile )
	{
		printe(ERR_core_cant_run, E_ERROR, lwp_name());
		return 0;
	}
	else if (( remove( hoppt ) == 0 ) ||
		( set(destpt,destaddr) == 0 ))
	{
		return 0;
	}
	// set bkpt on current location; don't want to 
	// hit it immediately - by setting state to es_breakpoint
	// we will step over it.
	if (destaddr == pc)
	{
		state = es_breakpoint;
		latestbkpt = &destpt;
	}

	retaddr = 0;
	verbosity = talk_ctl;
	goal = pg_run;

	if ( state == es_breakpoint )
	{
		ret = start(sg_stepbkpt);
	}
	else
	{
		ret = start(sg_run);
	}
	if (!ret)
		return 0;
	if (get_ui_type() == ui_gui)
		printm(MSG_proc_start, (unsigned long)this);
	return 1;
}

int 
LWP::stop()
{
	if ( state == es_corefile )
	{
		printe(ERR_core_cant_stop, E_ERROR, lwp_name());
		return 0;
	}
	ecount = 0;
	if (( remove( hoppt ) == 0)  ||
		(remove( destpt ) == 0))
	{
		return 0;
	}
	if (!pctl->stop())
	{
		printe(ERR_sys_cant_stop, E_ERROR, lwp_name());
		return 0;
	}
	state = es_suspended;
	return 1;
}

int
LWP::stmt_step( int where, int clearsig, Iaddr destaddr, int cnt, 
	int talk_ctl )
{
	int	ret;
	if ( clearsig ) latestsig = 0;
	if ( (state == es_running) || (state == es_stepping) )
	{
		printe(ERR_running_proc, E_ERROR, lwp_name());
		return 0;
	}
	else if ( state == es_corefile )
	{
		printe(ERR_core_cant_run, E_ERROR, lwp_name());
		return 0;
	}
	if (( remove( hoppt ) == 0 ) ||
		( set(destpt,destaddr) == 0 ))
	{
		return 0;
	}
	ecount = cnt;
	verbosity = talk_ctl;
	goal = where == STEP_INTO ? pg_stmt_step : pg_stmt_step_over;
	if (!find_stmt(startstmt, pc))
		return 0;
	startaddr = pc;	// first pc in current statement
	if ( state == es_breakpoint )
	{
		retaddr = 0;
		ret = start(sg_stepbkpt);
	}
	else if ( instr.is_bkpt( pc ) )
	{
		retaddr = 0;
		ret = start(sg_run);
	}
	else
	{
		retaddr = instr.retaddr( pc );
		if (destaddr)
			ret = start(sg_run);
		else
			ret = start(sg_step);
	}
	if (!ret)
		return 0;
	if (get_ui_type() == ui_gui)
		printm(MSG_proc_start, (unsigned long)this);
	return 1;
}

int
LWP::instr_step( int where, int clearsig, Iaddr destaddr, int cnt,
	int talk_ctl )
{
	int	ret;
	if ( clearsig ) latestsig = 0;
	if ( (state == es_running) || (state == es_stepping) )
	{
		printe(ERR_running_proc, E_ERROR, lwp_name());
		return 0;
	}
	else if ( state == es_corefile )
	{
		printe(ERR_core_cant_run, E_ERROR, lwp_name());
		return 0;
	}
	else if (( remove( hoppt ) == 0 ) ||
		( set(destpt,destaddr) == 0 ))
	{
		return 0;
	}
	retaddr = 0;
	ecount = cnt;
	verbosity = talk_ctl;
	goal = where == STEP_INTO ? pg_instr_step : pg_instr_step_over;
	if ( state == es_breakpoint )
	{
		ret = start(sg_stepbkpt);
	}
	else if ( instr.is_bkpt( pc ) )
	{
		ret = start(sg_run);
	}
	else
	{
		if (where == STEP_OVER)
			retaddr = instr.retaddr( pc );
		ret = start(sg_step);
	}
	if (!ret)
		return 0;
	if (get_ui_type() == ui_gui)
		printm(MSG_proc_start, (unsigned long)this);
	return 1;
}



int
LWP::start(Goal2 type)
{
	switch(type)
	{
	case sg_run:
		if ( pctl->run( latestsig ) == 0 )
		{
			printe(ERR_cant_restart,E_ERROR,lwp_name());
			return 0;
		}
		state = es_running;
		break;
	case sg_stepbkpt:
		if ( lift_bkpt(latestbkpt) == 0 )
			return 0;
		if (retaddr == 0)
		// don't reset if we have stopped for
		// a bkpt while stepping over a function
		// and we are ignoring that bkpt
			retaddr = instr.retaddr( pc );
	/*FALLTHROUGH*/
	case sg_step:
		if ( pctl->step( latestsig ) == 0 )
		{
			printe(ERR_cant_restart,E_ERROR,lwp_name());
			return 0;
		}
		state = es_stepping;
		break;
	}
	++epoch;
	goal2 = type;
	flags |= L_CHECK;
	return 1;
}


int
LWP::restart()
{
	if ((goal == pg_run) &&
		(sw_watch > 0))
	{
		printe(ERR_step_watch, E_WARNING, lwp_name());
		return stmt_step(STEP_INTO, 1, 0, 
			STEP_INF_QUIET, verbosity);
	}
	if ( state == es_breakpoint)
	{
		return start(sg_stepbkpt);
	}
	else if ( goal == pg_run )
	{
		return start(sg_run);
	}
	else if ( remove( hoppt ) == 0 )
	{
		return 0;
	}
	else if (( instr.is_bkpt( pc ) ) ||
		(destpt.addr() != 0))
	{
		return start(sg_run);
	}
	else
	{
		if (state == es_stepping || state == es_stepped)
			retaddr = instr.retaddr( pc );
		return start(sg_step);
	}
}

int
LWP::resume( int clearsig )
{
	if ( clearsig ) latestsig = 0;
	if ( (state == es_running) || (state == es_stepping) )
	{
		printe(ERR_running_proc, E_ERROR, lwp_name());
		return 0;
	}
	else if ( state == es_corefile )
	{
		printe(ERR_core_cant_run, E_ERROR, lwp_name());
		return 0;
	}
	else
	{
		return  restart();
	}
}

// Used to release grabbed processes - start process
// but leave state as dead.
int
LWP::drop_run()
{
	
	if ( pctl->run( latestsig ) == 0 )
	{
		printe(ERR_cant_restart,E_ERROR,lwp_name());
		return 0;
	}
	state = es_running;
	return 1;
}
