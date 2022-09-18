#ident	"@(#)debugger:libutil/common/set_fcn.C	1.2"
#include "utility.h"
#include "LWP.h"
#include "Frame.h"
#include "Symbol.h"
#include "Interface.h"
#include <string.h>

int
set_curr_func( LWP *lwp, const char *name)
{
	Symbol	symbol;
	Iaddr	addr;
	Frame	*cframe;
	char	*sname;
	int	found = 0;
	int	frameno;

	if ( name == 0 )
	{
		printe(ERR_internal, E_ERROR, "set_curr_func", __LINE__);
		return 0;
	}
	frameno = count_frames(lwp);
	for(cframe = lwp->topframe(); cframe; 
		cframe = cframe->caller(), frameno--)
	{
		addr = cframe->pc_value();
		symbol = lwp->find_entry(addr);
		if (symbol.isnull())
			continue;
		sname = lwp->symbol_name(symbol);
		if (strcmp(name, sname) == 0)
		{
			found = 1;
			break;
		}
	}
	if (!found)
	{
		printe(ERR_active_func, E_ERROR, name);
		return 0;
	}

	if (!lwp->setframe(cframe))
		return 0;

	if (get_ui_type() == ui_gui)
		printm(MSG_set_frame, (unsigned long)lwp, frameno);

	return 1;
}
