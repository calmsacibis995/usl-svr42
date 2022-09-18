#ident	"@(#)debugger:libutil/common/get_addr.C	1.4"

#include "utility.h"
#include "Location.h"
#include "LWP.h"
#include "Source.h"
#include "Symbol.h"
#include "Interface.h"
#include "Locdesc.h"
#include "Place.h"
#include <string.h>

// parse location description and return address associated
// with symbol or line number
//

static int
addr_line(LWP *lwp, long line, const char *fname, Iaddr & addr)
{
	Symbol		source;
	Source		lineinfo;

	if ((lwp->find_source(fname, source) == 0) ||
		(source.source( lineinfo ) == 0 ))
	{
		printe(ERR_no_source_info, E_ERROR, fname);
		return 0;
	}
	lineinfo.stmt_to_pc( line, addr );
	if ( addr == 0 )
	{
		printe(ERR_no_line, E_ERROR, fname, line);
		return 0;
	}
	return 1;
}

static int
addr_sym(LWP *lwp, Iaddr & addr, const char *file, const char *func, 
	long off, Symtype stype, Symbol & symbol)
{
	char	*name;
	symbol = find_symbol(file, func, lwp, lwp->pc_value(), stype);
	if ( symbol.isnull() )
	{
		printe(ERR_no_entry, E_ERROR, func);
		return 0;
	}
	addr = symbol.pc(an_lopc);
	if (addr == ~0)	// no lopc attribute
	{
		Locdesc locdesc;
		if (symbol.locdesc(locdesc) == 0)
		{
			printe(ERR_get_addr, E_ERROR, func);
			return 0;
		}
		Place place = locdesc.place(lwp, lwp->curframe());
		if (place.isnull() || place.kind != pAddress)
		{
			printe(ERR_get_addr, E_ERROR, func);
			return 0;
		}
		else
			addr = place.addr;
	}
	// check whether we have been given a name with no prototype info
	// and have found a prototyped name; if so may be an overloaded
	// function - warn about it
	name = lwp->symbol_name(symbol);
	if ((strchr(name, '(') != 0) && (strchr(func, '(') == 0))
	{
		Symbol	sym2 = lwp->next_global();
		if (!sym2.isnull())
		{
			int	len = strlen(func);
			char	*name2, *pos;
			name2 = lwp->symbol_name(sym2);
			if (((pos = strchr(name2, '(')) != 0) &&
				((pos - name2) == len) &&
				(strncmp(func, name2, len) == 0))
				printe(ERR_may_be_overload, E_WARNING,
					name, func);
		}
	}
	addr += (Iaddr)off;
	return 1;
}

int
get_addr( LWP *&lwp, Location* location, Iaddr & addr, Symtype stype,
	Symbol &sym)
{
	char		*file;
	char		*func;
	LWP		*l_lwp;
	Frame		*f;
	long		off;
	unsigned long	ul;

	sym.null();

	if (!lwp || !location)
		return 0;
	if (location->get_lwp(l_lwp) == 0)
		return 0;
	if (l_lwp)
		lwp = l_lwp;
	f = lwp->curframe();
	if (location->get_offset(off) == 0)
		return 0;
	if (location->get_file(lwp, f, file) == 0)
		return 0;
	switch ( location->get_type() ) 
	{
	case lk_addr:
		if (location->get_addr(lwp, f, ul) == 0)
			return 0;
		addr = (Iaddr)(ul + off);
		return 1;
	case lk_stmt:
		if (!file)
		{
			if (!current_loc(lwp, f, file, func, off) || !file)
			{
				printe(ERR_no_cur_src, E_ERROR);
				return 0;
			}
		}
		if (location->get_line(lwp, f, ul) == 0)
			return 0;
		return addr_line(lwp, (long)ul, file, addr);
	case lk_fcn:
		if (location->get_func(lwp, f, func) == 0)
			return 0;
		return addr_sym(lwp, addr, file, func, off, stype, sym);
	case lk_none:
	default:
		printe(ERR_internal, E_ERROR, "get_addr", __LINE__);
		return 0;
	}
}
