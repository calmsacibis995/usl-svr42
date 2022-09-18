#ident	"@(#)debugger:libutil/common/dis.C	1.4"

#include "utility.h"
#include "LWP.h"
#include "Proglist.h"
#include "Interface.h"
#include "Parser.h"
#include "Symbol.h"
#include "Attribute.h"
#include "global.h"
#include  <signal.h>

int
disassem_cnt( Proclist * procl, Location *l, unsigned int count, 
	int disp_func)
{
	Iaddr	addr, nextaddr;
	Iaddr	hi = ~0;
	int 	single = 1;
	LWP	*lwp;
	plist	*list;
	int	ret = 1;
	Symbol	func;

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
	sigrelse(SIGINT);
	do
	{
		if (interrupt & sigbit(SIGINT))
			break;
		if (!lwp->state_check(E_RUNNING|E_DEAD))
		{
			ret = 0;
			continue;
		}
		if (l)
		{
			if (!get_addr(lwp, l, addr, st_func, func))
			{
				ret = 0;
				continue;
			}
		}
		else
			addr = lwp->dot_value();
		printm(MSG_dis_header, lwp->lwp_name(), lwp->prog_name());
		if (disp_func)
		{
			func = lwp->find_entry(addr);
			if (!func.isnull())
			{
				addr = func.pc(an_lopc);
				hi = func.pc(an_hipc) - 1;
				count = ~0;
			}
			else
				count = num_line;
		}
		unsigned int i = 0;
		while(addr <= hi && i < count)
		{
			if (interrupt & sigbit(SIGINT))
				break;
			if (!lwp->disassemble(addr, 1, &nextaddr))
			{
				ret = 0;
				break;
			}
			addr = nextaddr;
			i++;
		}
		lwp->set_dot(addr);
		if (!single)
			printm(MSG_newline);
	}
	while(!single && ((lwp = list++->p_lwp) != 0));

	sighold(SIGINT);
	return ret;
}
