#ident	"@(#)debugger:libutil/common/find_fcn.C	1.1"

#include "utility.h"
#include "LWP.h"
#include "Source.h"
#include "Symbol.h"
#include "Symtab.h"
#include "Frame.h"
#include "Interface.h"
#include "Expr.h"

// returns file and line number for given function

char *
find_fcn( LWP * lwp, char *file, char *func, long &line)
{
	Symbol		symbol;
	Symtab		*symtab;
	Iaddr		loaddr;
	Source		source;
	Frame		*f = lwp->curframe();

	if ( func == 0 || lwp == 0 )
	{
		printe(ERR_internal, E_ERROR, "find_fcn", __LINE__);
		return 0;
	}
	symbol = find_symbol(file, func, lwp, f->pc_value(), st_func);
	if ( symbol.isnull() )
	{
		printe(ERR_no_entry, E_ERROR, func);
		return 0;
	}

	loaddr = symbol.pc(an_lopc);
	if ( (symtab = lwp->find_symtab( loaddr )) == 0
		|| symtab->find_source( loaddr, symbol ) == 0
		|| symbol.source( source ) == 0 )
	{
		printe(ERR_no_source_info, E_ERROR, func);
		return 0;
	}
	source.pc_to_stmt( loaddr, line, -1);
	return symbol.name();
}
