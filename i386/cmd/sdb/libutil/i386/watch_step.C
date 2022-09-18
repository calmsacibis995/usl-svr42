#ident	"@(#)sdb:libutil/i386/watch_step.C	1.1"

#include "utility.h"
#include "DbregAccess.h"

extern DbregAccess *proc_dbreg;	// debug register set for this process
				// global because it is needed in Process.ev.C

void
watch_step(int cnt, Process *process, char *buf, int vmode, int size)
{
	if ( cnt == -1 ) {	// try to use hw data breakpoint
		Expr *expr = new Expr(buf);

		proc_dbreg = new DbregAccess(process);
		if ( proc_dbreg->set_hw_watch(expr, size) != -1 )
			run(process, 1, 0, 0);
		else {		// hw watchpoints failed, use sw watchpoints
			delete proc_dbreg;
			proc_dbreg = NULL;
			if ( set_watch(process, buf) != 0 )
				stmt_step(process,1,0,cnt,vmode);
		}
	}
	else if ( set_watch(process, buf) != 0 )
		stmt_step(process,1,0,cnt,vmode);

	return;
}

void
deact_hw_watch()
{
	/* if hw watchpoints active now done in check_watchpoints deactivate */
	if ( proc_dbreg ) {
		delete proc_dbreg;
		proc_dbreg = NULL;
	}
	return;
}

int
interpret_wpt()
{
	// interpret watchpoint status a watchpoint has been hit.
	// We must return !=1 to stop process
	if ( proc_dbreg && proc_dbreg->interpret_status() == 1 )
		return 0;
}
