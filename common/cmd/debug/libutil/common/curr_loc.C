#ident	"@(#)debugger:libutil/common/curr_loc.C	1.2"

#include "Interface.h"
#include "LWP.h"
#include "Frame.h"
#include "Iaddr.h"
#include "utility.h"
#include "Symbol.h"
#include "Source.h"
#include "Tag.h"

// return current file,
// return current function and line, if available

int
current_loc(LWP *lwp, Frame *frame, char *&filename, char *&funcname,
	long &line)
{
	Symbol	sym;
	Source	source;
	Iaddr	pc;

	if ( lwp == 0 )
	{
		printe(ERR_internal, E_ERROR, "current_loc", __LINE__);
		return 0;
	}

	if (frame == 0)
		frame = lwp->topframe();

	pc = frame->pc_value();
	sym = lwp->find_entry(pc);
	if (!sym.isnull())
		funcname = lwp->symbol_name(sym);
	else
	{
		funcname = 0;
		filename = 0;
		line = 0;
		return 0;
	}

	while(!sym.isnull() && sym.tag() != t_sourcefile)
	{
		// This loop goes infinite when the lwp is at the first
		// break (just after a create).  This test treats
		// the symptom but not the cause.
		Symbol parent = sym.parent();
		if (parent == sym) 
			sym.null();
		else 
			sym = sym.parent();
	}
	if (sym.isnull())
	{
		line = 0;
		filename = 0;
		return 1;
	}
	filename = sym.name();
	if (sym.source(source))
		source.pc_to_stmt(pc, line);
	else
		line = 0;
	return 1;
}
