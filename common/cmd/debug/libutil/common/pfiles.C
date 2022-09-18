#ident	"@(#)debugger:libutil/common/pfiles.C	1.4"
#include "utility.h"
#include "LWP.h"
#include "Proglist.h"
#include "Interface.h"
#include "Parser.h"
#include "global.h"
#include "Symbol.h"
#include "Source.h"

int
print_files( Proclist * procl )
{
	int	single = 1;
	LWP	*lwp;
	plist	*list;
	int	ret = 1;
	Source	source;

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
		Symbol	file = lwp->first_file();
		for(; !file.isnull(); file = lwp->next_file())
		{
			if (!file.isSourceFile())
			{
				continue;
			}
			if (file.source(source))
				printm(MSG_source_file, file.name());
		}
	}
	while(!single && ((lwp = list++->p_lwp) != 0));
	return ret;
}
