#ident	"@(#)debugger:libexecon/common/LWP.sym.C	1.4"

#include <stdlib.h>
#include "LWP.h"
#include "Seglist.h"
#include "Symtab.h"
#include "Source.h"
#include "Interface.h"
#include "Instr.h"
#include "str.h"

// keep track of most recent function, symtab and source line
// to save needing to look them up constantly.
int
LWP::find_cur_src()
{
	Symbol	func;
	Source	source;

	dot = pc;	// used for disassembly

	if ((pc >= lopc) && (pc < hipc))
	{
		// still in address range of current function
		if (!last_src.isnull())
		{
			if (!last_src.source(source))
			{
				printe(ERR_internal, E_ERROR,
					"LWP::find_cur_src()", __LINE__);
				last_src.null();
				return 0;
			}
			current_srcfile = last_src.name();
			source.pc_to_stmt( pc, currstmt.line );
		}
		return 1;
	}
	if ((last_sym = seglist->find_symtab(pc)) == 0)
	{
		current_srcfile = 0;
		lopc = hipc = 0;
		last_src.null();
		currstmt.unknown();
		return 1;
	}
	func = last_sym->find_entry( pc );
	if (func.isnull())
	{
		current_srcfile = 0;
		lopc = hipc = 0;
		last_src.null();
		currstmt.unknown();
		return 1;
	}
	lopc = func.pc(an_lopc);
	hipc = func.pc(an_hipc);
	if (!last_sym->find_source(pc, last_src))
	{
		current_srcfile = 0;
		last_src.null();
		currstmt.unknown();
		return 1;
	}
	current_srcfile = last_src.name();
	if (!last_src.source(source))
	{
		last_src.null();
		currstmt.unknown();
		return 0;
	}
	source.pc_to_stmt( pc, currstmt.line );
	return 1;
}

// name of executable or shared object containing a given pc
const char *
LWP::object_name( Iaddr addr )
{
	return seglist->object_name( addr );
}

Symtab *
LWP::find_symtab( Iaddr addr )
{
	if ((addr >= lopc) && (addr < hipc) && last_sym)
		return last_sym;
	return seglist->find_symtab( addr );
}

Symtab *
LWP::find_symtab( const char *name )
{
	return seglist->find_symtab( name );
}

Symbol
LWP::find_entry( Iaddr addr )
{
	Symtab *symtab;

	if ((addr >= lopc) && (addr < hipc) && last_sym)
		symtab = last_sym;
	else if ((symtab = seglist->find_symtab(addr)) == 0)
	{
		Symbol	symbol;
		return symbol;
	}
	return(symtab->find_entry( addr ));
}

Symbol
LWP::find_symbol( Iaddr addr )
{
	Symtab *symtab;

	if ((pc >= lopc) && (pc < hipc) && last_sym)
		symtab = last_sym;
	else if ((symtab = seglist->find_symtab(pc)) == 0)
	{
		Symbol	symbol;
		return symbol;
	}
	return(symtab->find_symbol( addr ));
}

Symbol
LWP::find_scope( Iaddr addr )
{
	Symtab *symtab;

	if ((addr >= lopc) && (addr < hipc) && last_sym)
		symtab = last_sym;
	else if ((symtab = seglist->find_symtab(addr)) == 0)
	{
		Symbol	symbol;
		return symbol;
	}
	return(symtab->find_scope( addr ));
}

int
LWP::find_source( const char * name, Symbol & symbol )
{
	return seglist->find_source( name, symbol );
}

Symbol
LWP::find_global(const char * name )
{
	return seglist->find_global( name );
}

Symbol
LWP::next_global()
{
	return seglist->next_global();
}

Symbol
LWP::prev_global()
{
	return seglist->prev_global();
}

long
LWP::current_line()
{
	return currstmt.line;
}

int
LWP::set_current_stmt( const char * filename, long line )
{
	current_srcfile = str( filename );
	currstmt.line = line;
	return 1;
}

//
// get symbol name
//
char *
LWP::symbol_name( Symbol symbol )
{
	char	*ptr, *name;
	int	offset;
	Iaddr	addr, newaddr;
	Symbol	newsymbol;

	name = symbol.name();
	//
	// if there are no static shared libs, return symbol name unchanged
	//
	if ( seglist->has_stsl() == 0 )
		return name;
	//
	// if name is ".bt?" get real name of function from the branch table.
	//
	ptr = name;
	if ( (name[0] == '.') && (name[1] == 'b') && (name[2] == 't') ) 
	{
		offset  = atoi(ptr+3);		// offset in branch table
		addr = symbol.pc(an_lopc);
		newaddr = instr.fcn2brtbl( addr, offset);
		newsymbol = find_entry(newaddr);
		if ( newsymbol.isnull() == 0 )
			name = newsymbol.name();
	}	
	return name;
}	

Symbol
LWP::current_function()
{
	Symbol	symbol;
	Symtab	*symtab;

	if ((pc >= lopc) && (pc < hipc) && last_sym)
		return last_sym->find_entry(pc);
	else if ((symtab = seglist->find_symtab(pc)) != 0)
		return symtab->find_entry(pc);
	return symbol;
}

Symbol
LWP::first_file()
{
	return seglist->first_file();
}

Symbol
LWP::next_file()
{
	return seglist->next_file();
}

int
LWP::find_stmt( Stmt & stmt, Iaddr pc )
{
	Symtab *	symtab;
	Symbol		symbol;
	Source		source;

	if ((pc >= lopc) && (pc < hipc) && last_sym)
	{
		symtab = last_sym;
		if (last_src.isnull())
		{
			stmt.unknown();
			return 1;
		}
		if ( last_src.source(source) == 0 )
		{
			stmt.unknown();
			printe(ERR_internal, E_ERROR,
				"LWP::find_stmt()", __LINE__);
			return 0;
		}
	}
	else
	{
		if (((symtab = seglist->find_symtab(pc)) == 0) ||
			( symtab->find_source(pc, symbol) == 0 ) ||
			( symbol.source(source) == 0 ))
		{
			stmt.unknown();
			return 1;
		}
	}
	source.pc_to_stmt(pc, stmt.line);
	return 1;
}

// If addr is beginning address of a function, return address
// of first line past function prolog (1st real statement).
// If addr is not beginning of function, or we have no line info
// for the function, or the function has no executable statements,
// just return addr.
//
Iaddr
LWP::first_stmt(Iaddr addr)
{
	Source	source;
	Symtab	*symtab;
	Symbol	symbol, func;
	Iaddr	naddr, lo, hi;
	long	line;
	int	tmp;

	find_cur_src();
	if ((addr >= lopc) && (addr < hipc))
	{
		// within range of current function
		if (addr != lopc)
			return addr;
		if (last_src.isnull())
		{
			return instr.fcn_prolog(addr, 1, tmp, 0);
		}
		if (last_src.source(source) == 0)
		{
			last_src.null();
			return instr.fcn_prolog(addr, 1, tmp, 0);
		}
		hi = hipc;
	}
	else
	{
		if ((symtab = find_symtab(addr)) == 0)
			return addr;
		func = symtab->find_entry( addr );
		if (func.isnull())
			return addr;
		lo = func.pc(an_lopc);
		hi = func.pc(an_hipc);
		if (addr != lo)
			return addr;
		if ((symtab->find_source(addr, symbol) == 0)
			|| symbol.isnull() 
			|| (symbol.source(source) == 0))
		{
			return instr.fcn_prolog(addr, 1, tmp, 0);
		}
	}
	// find next statement past beginning
	source.pc_to_stmt(addr+1, line, 1); 
	source.stmt_to_pc(line, naddr, 0);
	if ((naddr == 0) || (naddr >= hi)) // no such statement
		return instr.fcn_prolog(addr, 1, tmp, 0);
	return naddr;
}
