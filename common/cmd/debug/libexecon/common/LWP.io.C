#ident	"@(#)debugger:libexecon/common/LWP.io.C	1.6"

#include "Interface.h"
#include "LWP.h"
#include "Process.h"
#include "Procctl.h"
#include "Parser.h"
#include "Seglist.h"
#include "Segment.h"
#include "Source.h"
#include "SrcFile.h"
#include "Symtab.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int
LWP::read( Iaddr addr, Stype stype, Itype & itype )
{
	Segment *seg;

	if ( state != es_corefile )
	{
		return pctl->read( addr, &itype, stype_size(stype) );
	}
	else if ( (seg = seglist->find_segment( addr )) == 0 )
	{
		return 0;
	}
	else
	{
		return seg->read( addr, stype, itype );
	}
}

int
LWP::write( Iaddr addr, Stype stype, const Itype & itype )
{
	if ( state != es_corefile )
	{
		return pctl->write( addr, (void *)&itype, stype_size(stype) );
	}
	return 0;
}

int
LWP::read( Iaddr addr, int len, char * buf )
{
	Segment *	seg;

	if ( state != es_corefile )
	{
		return pctl->read( addr, buf, len );
	}
	else if ( (seg = seglist->find_segment( addr )) == 0 )
	{
		return 0;
	}
	else
	{
		return seg->read( addr, buf, len );
	}
}

int
LWP::write( Iaddr addr, void * buf, int len )
{
	if ( state != es_corefile )
	{
		return pctl->write( addr, buf, len );
	}
	return 0;
}

int
LWP::disassemble( Iaddr addr, int symbolic, Iaddr *next )
{
	int	inst_size;
	Symbol	sym;
	char	*dinstr;
	Iaddr	offset = 0;
	char	*name = 0;
	long	line = 0;


	sym = find_entry(addr);
	if (!sym.isnull())
	{
		offset = addr - ( (Iaddr) sym.pc(an_lopc) );
		name = symbol_name( sym );
	}

	if (symbolic)
	{
		if (name)
		{
			// try to get line number
			Symbol	sym1;
			Source	source;

			sym1 = sym.parent();
			if (!sym1.isnull() && (sym1.tag() == t_sourcefile) &&
				sym1.source(source))
			{
				source.pc_to_stmt(addr, line, 0);
			}
		}
	}

	if ( (dinstr = instr.deasm( addr, inst_size, symbolic, name, offset))
		!= 0 && (inst_size > 0))
	{
		if (next) 
			*next = addr + (Iaddr)inst_size;
		if (line)
			printm(MSG_dis_line, line, addr, dinstr);
		else
			printm(MSG_disassembly, addr, dinstr);
		return 1;
	}
	else if (dinstr != 0)
	{
		// bad opcode
		printm(MSG_disassembly, addr, dinstr);
		if (next) 
			*next = addr + 1;
		return 1;
	}
	return 0;
}

// Use execstate to index into the table to get the message id for that state.
static Msg_id state_tab[es_watchpoint+1] = 
{
	MSG_invalid,
	MSG_es_core,
	MSG_invalid,
	MSG_invalid,
	MSG_invalid,
	MSG_es_stepped,
	MSG_es_suspend,
	MSG_es_signal,
	MSG_es_stop,
	MSG_es_sysent,
	MSG_es_sysxit,
	MSG_es_stop,
};

void
LWP::stateinfo(int level, const char *entryname, const char *filename)
{
	const char *spaceer = " ";

	switch( state )
	{
	case es_stepped:
		if (level < V_REASON)
			return;
		if (ecount != 0)
			printm(MSG_step_not_done, procname);
		else
			printm(state_tab[state],procname);
		break;
	case es_suspended:
		if (level < V_REASON)
			return;
		printm(state_tab[state],procname);
		break;

	case es_signalled:
		printm(state_tab[state],latestsig, signame(latestsig),procname);
		break;

	case es_syscallent:
	case es_syscallxit:
		printm(state_tab[state],latesttsc,sysname(latesttsc), procname);
		break;

	case es_corefile:
		if (level < V_REASON)
			return;
		printm(state_tab[es_corefile]);
		break;

	case es_breakpoint:
	case es_watchpoint:
		if (strlen(latestexpr) > (size_t)55)
		{	
			// add a new-line to keep it legible
			spaceer = "\n";
		}

		printm(state_tab[state],latestexpr, spaceer, procname);
		break;

	default:
		printe(ERR_internal, E_ERROR, "LWP::stateinfo", __LINE__);
		break;
	}

	if (entryname && filename)
		printm(MSG_loc_sym_file, entryname, filename);
	else if (entryname)
		printm(MSG_loc_sym, entryname);
	else
		printm(MSG_loc_unknown);
	if (state == es_corefile && _process && _process->core)
	{
		_process->core->core_state();
	}
}

Iaddr
LWP::getreg( RegRef regref )
{
	return regaccess.getreg( regref );
}

int
LWP::readreg( RegRef regref, Stype stype, Itype & itype )
{
	return regaccess.readreg( regref, stype, itype );
}

int
LWP::writereg( RegRef regref, Stype stype, Itype & itype )
{
	if ( regaccess.writereg( regref, stype, itype ) == 0 )
	{
		return 0;
	}
	else if ( regref == REG_PC )
	{
		pc = itype.iaddr;
		state = es_suspended;
	}
	return 1;
}

int
LWP::display_regs(Frame *f)
{
	return regaccess.display_regs(f);
}

int
LWP::setframe(Frame *frame)
{
	if ( !frame || !frame->valid() ) 
	{
		printe(ERR_internal, E_ERROR, "LWP::setframe", __LINE__);
		return 0;
	}
	cur_frame = frame;
	pc = frame->pc_value();
	find_cur_src();
	return 1;
}

Frame *
LWP::curframe()
{
	if ( !cur_frame || !cur_frame->valid() )
		cur_frame = topframe();

	return cur_frame;
}

Frame *
LWP::topframe()
{
	if ( !top_frame || !top_frame->valid() ) 
	{	// if out of date
		while ( top_frame ) 
		{	// delete all frames
			Frame	*f;

			f = top_frame;
			top_frame = (Frame *)top_frame->next();
			delete f;
		}
		top_frame = new Frame(this);	// create new top frame
		cur_frame = top_frame;
	}
	return top_frame;
}

int
LWP::print_map()
{
	if (state == es_corefile)
	{
		if (_process)
			return seglist->print_map(_process->core);
		else
			return 0;
	}
	else
		return seglist->print_map(pctl);
}


int
LWP::show_current_location( int showsrc, int level )
{
	Source		source;
	Symtab 		*symtab;
	Symbol		symbol;
	Symbol		entry;
	SrcFile 	*srcfile;
	char 		*s;
	char		*ename;
	long		line;

	if (!level)
		return 1;

	if ((symtab = find_symtab( pc )) == 0)
	{
		if (level >= V_EVENTS)
			stateinfo(level, 0, 0);
		disassemble( pc, 0, 0 );
		return 1;
	}

	entry = symtab->find_entry( pc );
	ename = entry.isnull() ? 0 : symbol_name(entry);
	if (symtab->find_source( pc, symbol ) == 0)
	{
		if (level >= V_EVENTS)
			stateinfo(level, ename, 0);
		disassemble( pc, 1, 0 );
		return 1;
	}

	if (level >= V_EVENTS)
		stateinfo(level, ename, symbol.name());
	if (!showsrc || symbol.source( source ) == 0)
	{
		disassemble( pc, 1, 0 );
		return 1;
	}

	source.pc_to_stmt( pc, line );
	if ( line == 0 )
		disassemble( pc, 1, 0 );
	else if ((srcfile = find_srcfile(this, symbol.name())) == 0
		|| (s = srcfile->line( (int)line )) == 0)
		printm(MSG_line_no_src, line);
	else
	{
		if (get_ui_type() == ui_gui)
			printm(MSG_source_file, srcfile->filename());
		printm(MSG_line_src, line, s );
	}

	return 1;
}

// is addr in a text segment
int
LWP::in_text( Iaddr addr)
{
        return seglist->in_text(addr);
}

// is addr in stack segment
int
LWP::in_stack( Iaddr addr)
{
        return seglist->in_stack(addr);
}

int
LWP::set_pc(Iaddr addr)
{
	if (regaccess.set_pc(addr))
	{
		dot = pc = addr;
		state = es_suspended;
		find_cur_src();
		return 1;
	}
	return 0;
}
