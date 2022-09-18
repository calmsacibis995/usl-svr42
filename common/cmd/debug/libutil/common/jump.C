#ident	"@(#)debugger:libutil/common/jump.C	1.4"

#include "utility.h"
#include "Iaddr.h"
#include "LWP.h"
#include "Proglist.h"
#include "Interface.h"
#include "Frame.h"
#include "Symbol.h"
#include "Tag.h"

// change program counter 

int
jump(Proclist *procl, Location *location)
{
	Iaddr	addr;
	LWP	*lwp;
	plist	*list;
	Symbol	func;
	int	ret = 1;
	int	single = 1;

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
		if (!lwp->state_check(E_RUNNING|E_DEAD|E_CORE))
		{
			ret = 0;
			continue;
		}

		if ( get_addr( lwp, location, addr, st_func, func ) == 0 )
		{
			ret = 0;
			continue;
		}

		func = lwp->find_entry(addr);

		while(!func.isnull() && (!IS_ENTRY(func.tag())))
			func = func.parent();
		if (!func.isnull())
		{
			Iaddr	lo, hi, pc;
			if (func.tag() == t_label)
				func = func.parent();
			pc = lwp->pc_value();
			lo = func.pc(an_lopc);
			hi = func.pc(an_hipc);
	
			if (pc < lo || pc >= hi)
				printe(ERR_non_local_jump, E_WARNING,
					lwp->lwp_name());
		}
		if (!lwp->set_pc(addr))
		{
			ret = 0;
			continue;
		}
		if (get_ui_type() == ui_gui)
		{
			char	*filename;
			char	*funcname;
			long	line;

			if (!current_loc(lwp, lwp->topframe(), filename, 
				funcname, line))
			{
				printm(MSG_jump, (unsigned long)lwp,
					addr, "", 0);
				continue;
			}
			if (!filename)
			{
				printm(MSG_jump, (unsigned long)lwp,
					addr, "", 0);
				continue;
			}
			printm(MSG_jump, (unsigned long)lwp,
				addr, filename, line);
		}
	}
	while(!single && ((lwp = list++->p_lwp) != 0));

	return ret;
}
