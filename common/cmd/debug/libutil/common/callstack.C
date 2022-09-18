#ident	"@(#)debugger:libutil/common/callstack.C	1.4"

#include "Interface.h"
#include "LWP.h"
#include "Proglist.h"
#include "Symtab.h"
#include "Source.h"
#include "Frame.h"
#include "Expr.h"
#include "Rvalue.h"
#include "Parser.h"
#include "global.h"
#include "Tag.h"
#include "utility.h"
#include <signal.h>
#include <libgen.h>

static void show_call(LWP *, Frame *, int);

int
print_stack(Proclist *procl, int how_many, int first)
{
	int	count = 0;
	int	single = 1;
	int	ret = 1;
	LWP	*lwp;
	plist	*list;

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
		if (!lwp->state_check(E_RUNNING|E_DEAD))
		{
			ret = 0;
			continue;
		}

		printm(MSG_stack_header, lwp->lwp_name(), lwp->prog_name());
		count = count_frames(lwp); // count is 1 less than
						// number of frames

		Frame *frame = lwp->topframe();
		if (first >= 0) // start with first rather than top
		{
			while(first < count)
			{
				frame = frame->caller();
				count--; 
			}
		}
		if ( how_many == 0 )
			how_many = ~0;		// big enough

		while ( frame && how_many-- && 
			!(interrupt & sigbit(SIGINT))) 
		{
			show_call(lwp, frame, count--);
			frame = frame->caller();
		}
		if (!single)
			printm(MSG_newline);
		if (interrupt & sigbit(SIGINT))
			break;
	}
	while(!single && ((lwp = list++->p_lwp) != 0));

	sighold(SIGINT);
	return ret;
}

static void
show_call(LWP *lwp, Frame *frame, int count)
{
	Iaddr	pc = frame->pc_value();
	Symtab	*symtab = lwp->find_symtab(pc);
	char	*filename = 0;
	long	line = 0;
	int	i = 0;
	int	assumed = 0;
	int	nbytes = frame->nargwds(assumed) * sizeof(int);
	int	done = 0;


	if (symtab)
	{
		Symbol	symbol;
		Source	source;

		if (symtab->find_source(pc, symbol))
		{
			filename = basename( symbol.name() );
			if (symbol.source( source ))
				source.pc_to_stmt( pc, line );
		}

		Symbol	symbol2 = lwp->find_entry( pc );
		char	*name = lwp->symbol_name(symbol2);
		if ( !name || !*name )
			name = "?";

		printm(MSG_stack_frame, (unsigned long)count, name);
		if ( (nbytes > 0) && frame->caller() ) 
		{
			Symbol	arg = symbol2.child();
			Tag	tag = arg.tag();
			Rvalue	rval;
			while ( !arg.isnull() ) 
			{
				if (tag == t_argument ) 
				{
					char	*val;
					Expr expr( arg, lwp );
					if (frame->incomplete()
						|| !expr.eval( lwp, pc, frame )
					    	|| !expr.rvalue( rval ))
						val = "???" ;
					else
						val = rval.print(lwp,
						DEBUGGER_BRIEF_FORMAT);
					printm(MSG_stack_arg, (i++>0) ?
						", " : "",
						arg.name(), val);
					//
					// round to word boundary
					//
					done+=ROUND_TO_WORD(rval.type().size());
				}
				arg = arg.sibling();
				tag = arg.tag();
			}
		}
	}
	else
		printm(MSG_stack_frame, count, "?");

	if (frame->caller() && !frame->incomplete()) 
	{
		int	n = done/sizeof(int);
		int	first = 1;
		while ( done < nbytes ) 
		{
			if (first && assumed)
			{
				// number of args is a guess
				first = 0;
				printm( MSG_stack_arg3, (i++>0)?", ":"", 
					frame->argword(n++) );
			}
			else
				printm( MSG_stack_arg2, (i++>0)?", ":"", 
					frame->argword(n++) );
			done += sizeof(int);
		}
	}

	if ( filename && *filename && line) 
	{
		printm(MSG_stack_frame_end_1, filename, line);
	} 
	else 
	{
		printm(MSG_stack_frame_end_2, pc);
	}

}
