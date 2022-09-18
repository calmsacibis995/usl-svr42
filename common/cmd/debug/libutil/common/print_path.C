#ident	"@(#)debugger:libutil/common/print_path.C	1.1"
#include	"utility.h"
#include	"LWP.h"
#include	"Proglist.h"
#include	"Interface.h"
#include	"SrcFile.h"
#include 	"Parser.h"
#include 	"global.h"

int
print_path( Proclist * procl, const char *fname )
{
	int single = 1;
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
		SrcFile	*srcfile;
		if ((srcfile = find_srcfile(lwp, fname)) == 0)
		{
			printe(ERR_no_source, E_ERROR, fname);
			ret = 0;
			continue;
		}
		printm(MSG_source_file, srcfile->filename());
	}
	while(!single && ((lwp = list++->p_lwp) != 0));
	return ret;
}
