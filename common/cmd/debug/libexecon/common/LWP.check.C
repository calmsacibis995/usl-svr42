#ident	"@(#)debugger:libexecon/common/LWP.check.C	1.2.1.3"

#include "LWP.h"
#include "EventTable.h"
#include "Interface.h"
#include "Machine.h"
#include "Instr.h"
#include "global.h"
#include "Seglist.h"
#include "List.h"
#include "Parser.h"
#include "Ev_Notify.h"
#include "Watchlist.h"
#include "Procctl.h"
#include <sys/procfs.h>
#include <unistd.h>


// Some implementations use signals for indicating breakpoints
// and other tracing information. Other implementations use the
// fault tracing facilities of /proc.  The process class
// attempts to allow either approach through the macros
// STOP_TYPE and LATEST_STOP
//

#if STOP_TYPE == PR_SIGNALLED
#define	LATEST_STOP	latestsig
#else
#define	LATEST_STOP	latestflt
#endif

int
LWP::inform( int what, int why )
{
	regaccess.update();
	seglist->update_stack( pctl );
	pc = regaccess.getreg( REG_PC );
	if ( (why == PR_REQUESTED))
	{
		// process stopped at user request - stop command
		state = es_suspended;
		if (flags & L_IN_START)
		{
			return inform_startup(why);
		}
		else
		{
			state = es_suspended;
			if (exec_cnt)
				exec_cnt--;
			return respond_to_sus();
		}
	}
	else if ((flags & L_GRABBED) && exec_cnt)
	{
		// grab a live process - just suspend execution
		state = es_suspended;
		exec_cnt--;
		return respond_to_sus();
	}
	else if ( why == PR_SYSENTRY )
	{
		state = es_syscallent;
		latesttsc = what;
		if (!(flags & L_IGNORE_EVENTS))
			return respond_to_tsc();
		else
			return restart();
	}
	else if (why == PR_SYSEXIT)
	{
		state = es_syscallxit;
		latesttsc = what;
		if (flags & L_IN_START)
		{
			// initial execs
			return inform_startup(why);
		}
		else
		{
			if (!(flags & L_IGNORE_EVENTS))
				return respond_to_tsc();
			else
				return restart();
		}
	}
	else if ( why == STOP_TYPE )
	{
		LATEST_STOP = what;
		if (flags & L_IN_START)
		{
			return inform_startup(why);
		}
		else if ( goal2 == sg_run )
		{
			return inform_run();
		}
		else if ( goal2 == sg_step)
		{
			return inform_step();
		}
		else if ( goal2 == sg_stepbkpt )
		{
			return inform_stepbkpt();
		}
#if STOP_TYPE == PR_SIGNALLED // use signals for tracing
		else
		{
			state = es_signalled;
			if (!(flags & L_IGNORE_EVENTS))
				return respond_to_sig();
			else
				return restart();
		}
#else	// use faults for tracing
		else
		{
			printe(ERR_internal, E_ERROR, "inform", __LINE__);
			return 0;
		}
#endif
	}
#if STOP_TYPE == PR_FAULTED
	else if ( why == PR_SIGNALLED )
	{
		latestsig = what;
		if (flags & L_IN_START)
		{
			return inform_startup(why);

		}
		else
		{
			state = es_signalled;
			if (!(flags & L_IGNORE_EVENTS))
				return respond_to_sig();
			else
				return restart();
		}
	}
#endif
	else 
	{
		printe(ERR_internal, E_ERROR, "inform", __LINE__);
		return 0;
	}
}

int
LWP::inform_startup(int why)
{
	// if a breakpoint was executed, pc might need adjustment.
	// ( for example, 386 leaves the pc one instruction 
	// past the bkpt. )
	//
	pc = instr.adjust_pc();
	state = es_suspended;
	if (why == PR_SYSEXIT)
	{
		// exec
		if ( EXEC_FAILED() )
		{
			// exec() failed; shell is doing path search,
			// continue trying
			return restart();
		}
		else if ( --exec_cnt > 0 )
		{
			// exec() succeeded, but we're not to the 
			// subject process yet
			return restart();
		}
		else 
		{
			return create_exec();
		}
	}
	else if (why == STOP_TYPE)
	{
		if (goal2 == sg_run && LATEST_STOP == STOP_BKPT)
		{
			if ( pc == destpt.addr() )
			{
				// have run past dynamic linker
				LATEST_STOP = 0;
				remove( destpt );
				return 1;
			}
			else if ( pc == dynpt.addr() )
			{
				// in dynamic linker - state of process changing
				LATEST_STOP = 0;
				state = es_breakpoint;
				latestbkpt = &dynpt;
				return respond_to_dynpt();
			}
		}
		else if (LATEST_STOP == STOP_TRACE && 
			goal2 == sg_stepbkpt && goal == pg_run)
		{
			// stepped over dynpt
			LATEST_STOP = 0;
			insert_bkpt( latestbkpt );
			state = es_stepped;
			return restart();
		}
	}
	state = es_signalled;
	return restart();
}

int
LWP::inform_run()
{
	if ( LATEST_STOP != STOP_BKPT )
	{
		state = es_signalled;
		return restart();
	}
	// if stopped at a breakpoint
	// pc might be pointing at the next instruction. adjust it.
	pc = instr.adjust_pc();
	LATEST_STOP = 0;
	if ( pc == hoppt.addr() )
	{
		// get here if doing instruction step over or
		// stmt stepping through code we don't have
		// source for - destination of a hop
		remove( hoppt );
		if (retaddr == pc)
			retaddr = 0;
		state = es_stepped;
		switch ( goal )
		{
			case pg_stmt_step:
			case pg_stmt_step_over:
				return check_stmt_step();
			case pg_instr_step_over:
				return check_instr_step();
			case pg_instr_step:
			case pg_run:
			default:
				printe(ERR_internal, E_ERROR, "inform_run",
					__LINE__);
				return 0;
		}
	}
	else if ( pc == destpt.addr() )
	{
		// goal of run to address
		remove( destpt );
		if (goal == pg_run)
			state = es_suspended;
		else
			state = es_stepped;
		check_watchpoints();
		find_cur_src();
		check_onstop();
		ecount = 0;
		return show_current_location( 1, verbosity );
	}
	else if ( hw_watch && hw_watch->hw_fired(this))
	{
		state = es_suspended;
		if (check_watchpoints())
		{
			check_onstop();
			return 1;
		}
	}
	if ( (latestbkpt = etable->breaklist.lookup(pc)) != 0 )
	{
		state = es_breakpoint;
		if (!(flags & L_IGNORE_EVENTS))
			return respond_to_bkpt();
		else
			return restart();
	}
	else if ( pc == dynpt.addr() )
	{
		// in dynamic linker - state of process changing
		state = es_breakpoint;
		latestbkpt = &dynpt;
		return respond_to_dynpt();
	}
	else
	{
		// hit breakpoint but our thread doesn't wish
		// to catch it
		return restart();
	}
}

int
LWP::inform_step()
{
	if ( LATEST_STOP != STOP_TRACE )
	{
		LATEST_STOP = 0;
		return restart();
	}
	state = es_stepped;
	LATEST_STOP = 0;
	switch ( goal )
	{
		case pg_stmt_step:
		case pg_stmt_step_over:
			return check_stmt_step();
		case pg_instr_step:
		case pg_instr_step_over:
			return check_instr_step();
		case pg_run:
		default:
			printe(ERR_internal, E_ERROR, "inform_step", __LINE__);
			return 0;
	}
}

int
LWP::inform_stepbkpt()
{
	// insert breakpoint after we lifted it to step through
	insert_bkpt( latestbkpt );
	if ( LATEST_STOP != STOP_TRACE )
	{
		return restart();
	}
	LATEST_STOP = 0;
	state = es_stepped;
	switch ( goal )
	{
		case pg_run:
			if ( hw_watch && hw_watch->hw_fired(this))
			{
				if (check_watchpoints())
				{
					state = es_suspended;
					check_onstop();
					return 1;
				}
			}
			return restart();
		case pg_stmt_step:
		case pg_stmt_step_over:
			return check_stmt_step();
		case pg_instr_step:
		case pg_instr_step_over:
			return check_instr_step();
		default:
			printe(ERR_internal, E_ERROR, "inform_stepbkpt", __LINE__);
			return 0;
	}
}

int
LWP::check_stmt_step()
{
	Iaddr	addr;

	if ( (destpt.addr() == pc) || (interrupt & sigbit(SIGINT)))
	{
		// step to an address
		remove( destpt );
		check_watchpoints();
		find_cur_src();
		check_onstop();
		ecount = 0;
		return show_current_location( 1, verbosity );
	}
	else if ((goal == pg_stmt_step_over) && (retaddr != 0))
	{
		// step over function call to return address
		if (!set( hoppt, retaddr ))
			return 0;
		return start(sg_run);
	}
	else if ( find_stmt(currstmt, pc) == 0 )
	{
		return 0;
	}
	else if ( currstmt.is_unknown() )
	{
		// no source for current pc, set a target breakpoint
		// on either the return address of this function
		// or the target of a jump, if instr is a jump
		if (retaddr != 0 ) 
		{
			if ( (addr = instr.jmp_target( pc )) == 0 ) addr = retaddr;
			if (!set( hoppt, addr ))
				return 0;
			return start(sg_run);
		}
		else
			return restart();
	}
	else if (( currstmt == startstmt) && (pc != startaddr))
	{
		// we are still within original statemnt
		// if pc == startaddr, we may have come around
		// to beginning of statement again in a loop
		if (verbosity >= V_HIGH)
			show_current_location( 0, verbosity );
		return restart();
	}
	else if ( check_watchpoints())
	{
		Iaddr	newpc;
		if (destpt.addr() == 0)
		{
			// skip past function prolog
			if ((newpc = first_stmt(pc)) != pc)
			{
				set(destpt, newpc);
				return restart();
			}
		}
		remove( destpt );
		check_onstop();
		return 1;
	}
	else if ( destpt.addr() != 0 )
	{
		if (verbosity >= V_HIGH)
			show_current_location( 1, verbosity );
		if (!find_stmt(startstmt, pc))
			return 0;
		startaddr = pc;
		return restart();
	}
	else if (( ecount < 0 ) || ( --ecount > 0))
	{
		// ecount > 0 is number of steps
		// ecount < 0 means step forever
		if (ecount != STEP_INF_QUIET)
		{
			show_current_location( 1, verbosity );
		}
		if (!find_stmt(startstmt, pc))
			return 0;
		startaddr = pc;
		return restart();
	}
	else
	{
		Iaddr	newpc;
		// skip past function prolog
		if ((newpc = first_stmt(pc)) != pc)
		{
			set(destpt, newpc);
			return restart();
		}
		find_cur_src();
		check_onstop();
		return show_current_location( 1, verbosity );
	}
}

int
LWP::check_instr_step()
{
	if ( (destpt.addr() == pc) || (interrupt & sigbit(SIGINT)))
	{
		remove( destpt );
		check_watchpoints();
		find_cur_src();
		check_onstop();
		ecount = 0;
		return show_current_location( 0, verbosity );
	}
	else if ( (goal == pg_instr_step_over) && (retaddr != 0))
	{
		if (!set( hoppt, retaddr ))
			return 0;
		return start(sg_run);
	}
	else if ( check_watchpoints() )
	{
		remove( destpt );
		check_onstop();
		return 1;
	}
	else if ( destpt.addr() != 0 )
	{
		if (verbosity >= V_HIGH)
			show_current_location( 0, verbosity );
		return restart();
	}
	else if (( ecount < 0 ) || ( --ecount > 0))
	{
		// ecount > 0 is number of steps
		// ecount < 0 means step forever
		if (ecount != STEP_INF_QUIET)
		{
			show_current_location( 0, verbosity );
		}
		return restart();
	}
	else
	{
		find_cur_src();
		check_onstop();
		return show_current_location( 0, verbosity );
	}
}

// add onstop commands to cmdlist
void 
LWP::check_onstop()
{
	NotifyEvent	*ne;

	if (!etable || !(ne = etable->onstoplist))
		return;

	for (; ne; ne = ne->next())
	{
		(*ne->func)(ne->thisptr);
	}
}

// returns 1 if the state of the process is acceptable
// for the mode given; else prints a message an returns 0
int
LWP::state_check(int mode)
{
	switch(state)
	{
	case es_none:
	case es_dead:
		if (mode & E_DEAD)
		{
			printe(ERR_invalid_op_dead, E_ERROR, 
				lwp_name());
			return 0;
		}
		break;
	case es_corefile:
		if (mode & E_CORE)
		{
			printe(ERR_invalid_op_core, E_ERROR, 
				lwp_name());
			return 0;
		}
		break;
	case es_stepping:
	case es_running:
		if (mode & E_RUNNING)
		{
			printe(ERR_invalid_op_running, E_ERROR, 
				lwp_name());
			return 0;
		}
		break;
	default:
			break;
	}
	return 1;
}
