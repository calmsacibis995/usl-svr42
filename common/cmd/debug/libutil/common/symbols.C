#ident	"@(#)debugger:libutil/common/symbols.C	1.5"

#include "utility.h"
#include "LWP.h"
#include "Proglist.h"
#include "Frame.h"
#include "Interface.h"
#include "Msgtab.h"
#include "Symbol.h"
#include "Symtab.h"
#include "global.h"
#include "Tag.h"
#include "NameList.h"
#include "Iaddr.h"
#include "Parser.h"
#include "Rvalue.h"
#include "Source.h"
#include "Expr.h"
#include "Language.h"
#include "Dbgvarsupp.h"

#include <libgen.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

struct SymEntry
{
	Symbol		*sym;
	const char	*location;
	long		line;
			SymEntry(Symbol &s, const char *loc, long l) {
				sym = new Symbol(s); location = loc;
				line = l; }
			~SymEntry() { delete sym; }
};

int
cmp(SymEntry **sym1, SymEntry **sym2)
{
	return(strcmp((*sym1)->sym->name(), (*sym2)->sym->name()));
}


class SymList
{
	SymEntry	**list;
	int		count;
	int		size;
public:
			SymList();
			~SymList();
	void		add(Symbol&, const char *location, long line);
	int		printall(LWP *, int mode);
	void		clear();
};

#define LISTENT	200

SymList::SymList()
{
	// initialize list with default number of entries
	// we never shrink list
	if ((list = (SymEntry **)calloc(LISTENT, 
		sizeof(SymEntry *))) == 0)
	{
		new_handler();
		return;
	}
	size = LISTENT;
	count = 0;
}

SymList::~SymList()
{
	clear();
	free(list);
}

void
SymList::clear()
{
	register SymEntry	**lptr = list;
	while(*lptr)
	{
		delete(*lptr);
		*lptr = 0;
		lptr++;
	}
	count = 0;
}

void
SymList::add(Symbol &s, const char *loc, long line)
{
	SymEntry	*sym = new SymEntry(s, loc, line);
	
	if (count >= (size - 1))
	{
		// always leave a null entry at end
		if ((list = (SymEntry **)realloc((char *)list,
			(sizeof(SymEntry *) * (size + LISTENT)))) == 0)
		{
			new_handler();
		}
		memset(list+size, 0, sizeof(SymEntry *) * LISTENT);
		size += LISTENT;
	}
	list[count++] = sym;
}

static int
print_sym(LWP *lwp, Symbol s, const char *location, long line, int mode)
{
	char	line_str[6];
	char	*l;
	int	bad_val = 0;
	int	assumed;
	
	if (line)
	{
		sprintf(line_str, "%d", line);
		l = line_str;
	}
	else
		l = "";
	assumed = s.type_assumed(1);
		
	if (mode & SYM_VALUES)
	{
		Rvalue rval;
		char   *val;
		// get type and expression first so any error messages come
		// out on a separate line and don't mess up the output
		Expr exp(s, lwp);
		if (!exp.eval(lwp) || !exp.rvalue(rval))
		{
			bad_val = 1;
			val = (char *)Mtable.format(ERR_evaluation);
		}
		else
		{
			val = rval.print(lwp, DEBUGGER_BRIEF_FORMAT);
		}
		if (mode & SYM_TYPES)
		{
			if (assumed)
				printm(MSG_symbol_type_val_assume, 
					s.name(), location, l,
					print_type(s, C), val);
			else
				printm(MSG_symbol_type_val, s.name(), 
					location, l,
					print_type(s, C), val);
		}
		else
		{
			if (assumed)
				printm(MSG_symbol_val_assume, s.name(), location, l, val);
			else
				printm(MSG_symbol_val, s.name(), location, l, val);
		}
	}
	else if (mode & SYM_TYPES)	// MORE - get lang from process?
		if (assumed)
			printm(MSG_symbol_type_assume, s.name(), 
				location, l, print_type(s, C));
		else
			printm(MSG_symbol_type, s.name(), location, 
				l, print_type(s, C));
	else
		printm(MSG_symbol, s.name(), location, l);
	return (!bad_val);
}

int
SymList::printall(LWP *lwp, int mode)
{
	int		ret = 1;
	SymEntry	*symentry, **lptr;

	if (!count)
		return 1;

	qsort(list, count, sizeof(SymEntry *), (int (*)(const void *,
		const void *))cmp);

        lptr = list;
	symentry = *lptr;
	for ( ; symentry; symentry = *(++lptr))
	{
		if (!print_sym(lwp, *(symentry->sym), symentry->location, 
			symentry->line, mode))
			ret = 0;
		if (interrupt & sigbit(SIGINT))
			break;
	}
	return ret;
}

// print out global names from the a.out, or from a shared object

static int
get_globals(LWP *lwp, const char *obj_name, const char *pattern, int mode, SymList *list)
{
	const char	*s_name;
	NameEntry	*np;
	Symtab		*stp;
	Symbol		sym;
	int		ret = 1;

	if (!obj_name)
	{
		Iaddr	pc = lwp->curframe()->pc_value();
		stp = lwp->find_symtab(pc);
		obj_name = lwp->object_name(pc);
	}
	else 
		stp = lwp->find_symtab(obj_name);
	if (!stp)
	{
		printe(ERR_no_sym_info, E_ERROR, lwp->lwp_name());
		return 0;
	}

	for(np = stp->first_global(); np; np = stp->next_global())
	{
		s_name = np->name();
		if (!s_name || (pattern && !gmatch(s_name, pattern)))
			continue;
		sym = stp->global_symbol(np);

		if (list)
			list->add(sym, obj_name, 0);
		else 
			if (!print_sym(lwp, sym, obj_name, 0, mode))
				ret = 0;
		if (interrupt & sigbit(SIGINT))
			break;
	}
	return ret;
}

// print out the local names visible from the current scope

static char *file_loc;
static char *func_loc;

static int
inner_names(LWP * lwp, Symbol& inner_scope, int mode, 
	const char *pattern, const char *filename, SymList *symlist)
{
	Symbol		scope, s;
	const char	*s_name;
	Tag		tag;
	int		flevel = 0;
	Source		source;
	const char	*funcname;
	const char	*fname;
	const char	*location;

	// find the current source file - needed to print the location
	for(scope = inner_scope; !scope.isnull(); scope = scope.parent())
	{
		tag = scope.tag(); // returns t_none if no tag.
		if (tag == t_sourcefile)
		{
			scope.source(source);
			fname = scope.name();
			break;
		}
		else if (IS_ENTRY(tag))
			funcname = scope.name();
	}

	// if printing locals, work from the inner scope out
	// if printing file statics only, scope is already set at the
	// file level from the preceeding for loop
	if (mode&SYM_LOCALS)
		scope = inner_scope;
	else if (scope.isnull())
	{
		printe(ERR_no_cur_src, E_WARNING);
		return 0;
	}

	for(; !scope.isnull(); scope = scope.parent())
	{
		long	line = 0;
		tag = scope.tag(); // returns t_none if no tag.

		if (tag == t_sourcefile && !(mode&SYM_FILES))
			break;

		// the tag must be one of these if it has a scope
		switch (tag)
		{
		case t_sourcefile:
			flevel = 1;
			break;

		case t_block:
		case t_entry:
		case t_subroutine:
		case t_global_sub:
		case t_extlabel:
			break;

		// don't need to scan struct/union members
		case t_structuretype:
		case t_uniontype:
			continue;

		default:
			printe(ERR_unexpected_tag, E_WARNING, tag);
		    	continue;
		}

		s = scope.child();
		if (s.isnull())
			continue;

		// The location for a file symbol is the file name
		// For a local, it's the function name and line
		// where the line appears only if the symbol is declared in
		// in a inner block.  The location is the same for each
		// symbol in that scope
		if (flevel)
		{
			if (filename && (strcmp(filename, fname) != 0))
			{
				// file symbols requested in different
				// file
				scope = lwp->first_file();
				for(; !scope.isnull(); 
					scope = lwp->next_file())
				{
					fname = scope.name();
					if (strcmp(fname, filename) == 0)
						break;
				}
				if (scope.isnull())
				{
					printe(ERR_no_source, E_ERROR, 
						filename);
					return 0;
				}
				s = scope.child();
				if (s.isnull())
					break;
			}
			location = file_loc
				= new char[strlen(fname) + 1];
			strcpy(file_loc, fname);
		}
		else
		{
			if (!func_loc)
			{
				func_loc = new char[strlen(funcname) + 1];
				strcpy(func_loc, funcname);
			}
			location = func_loc;
			Iaddr	lowpc = scope.pc(an_lopc);

			source.pc_to_stmt(lowpc, line);
		}

		// loop through the scope's children, which are the variables
		while(!s.isnull() && !(interrupt & sigbit(SIGINT)))
		{
			tag = s.tag(); 

			switch(tag)
			{
			default:
				break;

				// global variables and functions are covered
				// by get_globals
			case t_argument:
			case t_local_variable:
			case t_label:
			case t_entry:
			case t_subroutine:
				if (((s_name = s.name()) == 0) 
					|| (pattern && !gmatch(s_name, pattern)))
					break;
				symlist->add(s, location, line);
				break;

			}
			s = s.sibling();
		}
	}
	return 1;
}

static int
print_debug_vars(LWP *lwp, const char *pattern, int mode)
{
	char	*name;
	int	ret = 1;
	char	*l = "";
	char	*loc = "debugger";
	Debug_var_support *var_table = new Debug_var_support(
		lwp, lwp ? lwp->curframe() : 0, 
		0, mode&SYM_BUILT_IN, mode&SYM_USER);

	for (var_table->First(); name = var_table->Name(); var_table->Next())
	{
		if (pattern && !gmatch(name, pattern)) 
			continue;
		if (mode & SYM_VALUES)
		{
			char * value = var_table->Value();
			if (!value)
			{
				value = (char *)Mtable.format(ERR_evaluation);
				ret = 0;
			}
			if (mode & SYM_TYPES)	
				printm(MSG_symbol_type_val, name, loc, l, "", value);
			else
				printm(MSG_symbol_val, name, loc, l, value);
		}
		else if (mode & SYM_TYPES)	
			printm(MSG_symbol_type, name, loc, l, "");
		else
			printm(MSG_symbol, name, loc, l);
		if (interrupt & sigbit(SIGINT))
		{
			break;
		}
	}
	delete var_table;
	return ret;
}

int
symbols(Proclist *procl, const char *obj_name, const char *pattern, 
	const char *filename, int mode)
{
	Frame	*frame;
	Iaddr	pc;
	Symtab	*symtab;
	Symbol	top_scope;
	int	single = 1;
	LWP	*lwp;
	plist	*list;
	int	ret = 1;
	static SymList	*symlist;
	
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
	if (!lwp && (mode & (SYM_FILES|SYM_LOCALS|SYM_GLOBALS)))
		// print debug vars even with no lwp
	{
		printe(ERR_no_proc, E_ERROR);
		return 0;
	}
	sigrelse(SIGINT);
	do
	{
		Msg_id	msgid;

		if (mode & (SYM_FILES|SYM_LOCALS|SYM_GLOBALS))
		{
			if (!lwp->state_check(E_RUNNING|E_DEAD))
			{
				ret = 0;
				continue;
			}
		}
		if ((mode & (SYM_VALUES|SYM_TYPES)) == (SYM_VALUES|SYM_TYPES))
			msgid = MSG_sym_type_val_header;
		else if (mode & SYM_TYPES)
			msgid = MSG_sym_type_header;
		else if (mode & SYM_VALUES)
			msgid = MSG_sym_val_header;
		else
			msgid = MSG_sym_header;
		if (lwp)
			printm(msgid, lwp->lwp_name(), lwp->prog_name());
		else
			printm(msgid, "(no process)", "(no program)");

		if (mode & (SYM_BUILT_IN|SYM_USER))
		{
			if (!print_debug_vars(lwp, pattern, mode))
				ret = 0;
			if (interrupt & sigbit(SIGINT))
				break;
		}
		if (mode & (SYM_FILES|SYM_LOCALS))
		{
			frame =  lwp->curframe();
			pc = frame->pc_value();
			if ((symtab = lwp->find_symtab(pc)) == 0)
			{
				printe(ERR_no_sym_info, E_ERROR,
					lwp->lwp_name());
				ret = 0;
				continue;
			}
			if (!symlist)
				// symlist collects symbols in 
				// alphabetical order
				symlist = new SymList;
			top_scope = symtab->find_scope(pc);
			if (!inner_names(lwp, top_scope, mode, pattern,
				filename, symlist))
				ret = 0;
			if (mode & SYM_GLOBALS)
			{
				if (!get_globals(lwp, obj_name, pattern,
					mode, symlist))
					ret = 0;
			}
			if (!symlist->printall(lwp, mode))
				ret = 0;
			symlist->clear();
			delete file_loc;
			delete func_loc;
			file_loc = func_loc = 0;
		}
		else if (mode & SYM_GLOBALS)
		{
			// globals are already in alphabetical order
			if (!get_globals(lwp, obj_name, pattern, mode, 0))
				ret = 0;
		}
		if (interrupt & sigbit(SIGINT))
			break;
	}
	while(!single && ((lwp = list++->p_lwp) != 0));

	sighold(SIGINT);
	return ret;
}
