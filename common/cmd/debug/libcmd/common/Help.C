#ident	"@(#)debugger:libcmd/common/Help.C	1.5"

#include "Keyword.h"
#include "Parser.h"
#include "Interface.h"
#include "Scanner.h"
#include "TSClist.h"
#include "str.h"

// WARNING: this table is dependent on the order of the
// command enumeration
static const Msg_id help[] = {
	MSG_invalid,		// NoOp
	MSG_invalid,		// REDIR
	MSG_invalid,		// CMDLIST
	MSG_help_bang,		// SHELL
	MSG_invalid,		// SAVE
	MSG_help_alias,		// ALIAS
	MSG_help_break,		// BREAK
	MSG_help_cancel,	// CANCEL
	MSG_help_cd,		// CD
	MSG_help_change,	// CHANGE
	MSG_help_cont,		// CONTINUE
	MSG_help_create,	// CREATE
	MSG_help_delete,	// DELETE
	MSG_help_dis,		// DIS
	MSG_help_disable,	// DISABLE
	MSG_help_dump,		// DUMP
	MSG_help_enable,	// ENABLE
	MSG_help_event,		// EVENT
	MSG_help_export,	// EXPORT
	MSG_help_fc,		// FC
	MSG_invalid,		// FUNCTIONS - gui only
	MSG_help_grab,		// GRAB
	MSG_help_halt,		// HALT
	MSG_help_help,		// HELP
	MSG_help_if,		// IF
	MSG_help_input,		// INPUT
	MSG_help_jump,		// JUMP
	MSG_help_kill,		// KILL
	MSG_help_list,		// LIST
	MSG_help_logoff,	// LOGOFF
	MSG_help_logon,		// LOGON
	MSG_help_map,		// MAP
	MSG_help_onstop,	// ONSTOP
	MSG_invalid,		// PENDING - gui only
	MSG_invalid,		// PFILES - gui only
	MSG_invalid,		// PPATH - gui only
	MSG_help_print,		// PRINT
	MSG_help_ps,		// PS
	MSG_help_pwd,		// PWD
	MSG_help_quit,		// QUIT
	MSG_help_regs,		// REGS
	MSG_help_release,	// RELEASE
	MSG_help_rename,	// RENAME
	MSG_help_run,		// RUN
	MSG_help_script,	// SCRIPT
	MSG_help_set,		// SET
	MSG_help_signal,	// SIGNAL
	MSG_help_stack,		// STACK
	MSG_help_step,		// STEP
	MSG_help_stop,		// STOP
	MSG_help_symbols,	// SYMBOLS
	MSG_help_syscall,	// SYSCALL
	MSG_invalid,		// VERSION
	MSG_help_while,		// WHILE
	MSG_last		// must be last
};


struct Topic {
	const char	*key;
	Msg_id		help;
};

static Topic topic[] = {
	"%db_lang",	MSG_help_percent_db_lang,
	"%file",	MSG_help_percent_file,
	"%follow",	MSG_help_percent_follow,
	"%frame",	MSG_help_percent_frame,
	"%func",	MSG_help_percent_func,
	"%global_path",	MSG_help_percent_global_path,
	"%lang",	MSG_help_percent_lang,
	"%lastevent",	MSG_help_percent_lastevent,
	"%list_file",	MSG_help_percent_list_file,
	"%list_line",	MSG_help_percent_list_line,
	"%line",	MSG_help_percent_line,
	"%loc",		MSG_help_percent_loc,
	"%mode",	MSG_help_percent_mode,
	"%num_bytes",	MSG_help_percent_num_bytes,
	"%num_lines",	MSG_help_percent_num_lines,
	"%path",	MSG_help_percent_path,
	"%proc",	MSG_help_percent_proc,
	"%program",	MSG_help_percent_program,
	"%prompt",	MSG_help_percent_prompt,
	"%redir",	MSG_help_percent_redir,
	"%result",	MSG_help_percent_result,
	"%thisevent",	MSG_help_percent_thisevent,
	"%verbose",	MSG_help_percent_verbose,
	"%wait",	MSG_help_percent_wait,
	"assoccmd",	MSG_help_assoc,
	"block",	MSG_help_block,
	"expr",		MSG_help_expr,
	"format",	MSG_help_format,
	"location",	MSG_help_location,
	"pattern",	MSG_help_pattern,
	"proclist",	MSG_help_proclist,
	"procname",	MSG_help_procname,
	"redirection",	MSG_help_redir,
	"regexp",	MSG_help_regexp,
	"scope",	MSG_help_scope,
	"signames",	MSG_last,		// special
	"stop_expr",	MSG_help_stop_expr,
	"sysnames",	MSG_last,		// special
	"uservars",	MSG_help_uservars,
	0,		MSG_invalid
};

int
do_help( register char *p )
{
	int	nest = 0;
	char	line[(17 * 5) + 1];	// 5 items per line
	char	*buf;
	if ( !p || !*p ) 
	{	// list all commands and topics

		printm(MSG_help_hdr_commands);
		register int n = 0;
		buf = line;
		for ( register Keyword *k = keywordtable ; k->str ; ++k ) 
		{
			if ( help[k->op] == MSG_invalid )
				continue;
			if ( ++n%5 == 0 )
			{
				// end of line
				buf += sprintf(buf, "%-.16s", k->str);
				printm(MSG_help_topics, line);
				buf = line;
			}
			else
				buf += sprintf(buf, "%-16.16s", k->str);
		}
		if ( n%5 != 0 )
			printm(MSG_help_topics, line);
		printm(MSG_help_hdr_topics);
		n = 0;
		buf = line;
		for( Topic *t = topic ; t->key ; ++t ) 
		{
			if (t->help == MSG_invalid)
				continue;
			if ( ++n%5 == 0 )
			{
				// end of line
				buf += sprintf(buf, "%-.16s", t->key);
				printm(MSG_help_topics, line);
				buf = line;
			}
			else
				buf += sprintf(buf, "%-16.16s", t->key);
		}
		if ( n%5 != 0 )
			printm(MSG_help_topics, line);
		printm(MSG_help_get_alias);
		printm(MSG_help_get_help);
		return 1;
	} 
	p = str(p);
again:	Op op = NoOp;
	Keyword *k = keyword(p);
	if (k)
		op = k->op;
	if (op != NoOp) 
	{
		if ( (help[op] != MSG_invalid) &&
			(help[op] != MSG_last))
		{
			printm(help[op]);
		} 
		else 
		{
			printe(ERR_help_no_help, E_ERROR, p);
			return 0;
		} 
	}
	else
	{
		Topic *t = topic;
		for( ; t->key ; ++t ) 
		{
			if ( !strcmp( p, t->key ) )
				break;
		}
		if (!t->key)
		{
			// maybe it's an alias...
			Token *tl = find_alias(p);
			if ( tl ) 
			{
				if ( ++nest >= 20 ) 
				{
					printe(ERR_alias_recursion, 
						E_ERROR, p);
					return 0;
			    	} 
				else 
				{
					char *pp = print_tok_list(tl);
					char *pspace;
					printm(MSG_alias, p, pp);
					if ((pspace = strchr(pp, ' ')) != 0)
						*pspace = 0;
					p = str(pp);
					goto again;
				}
			} 
			else 
			{
				printe(ERR_help_bad_cmd, E_ERROR, p);
				return 0;
			}
		}
		else if ( (t->help != MSG_invalid) && 
			(t->help != MSG_last) )
		{
			printm(t->help);
		} 
		else
		{	// special topic
			if ( !strcmp(t->key, "signames") ) 
			{
				printm(MSG_help_hdr_sigs);
				int n = 0;
				for ( int i = 1 ; i<=MAXSIG ; i++ ) 
				{
					char *s = signames(1<<(i-1));
					if ( s && *s ) 
					{
						printm(MSG_signame, i, s);
						if ( ++n%5 == 0 )
							printm(MSG_newline);
					}
			    	}
				if ( n%5 != 0 )
					printm(MSG_newline);
			}
			else if ( !strcmp(t->key, "sysnames") ) 
			{
				printm(MSG_help_hdr_sys);
				dump_syscalls();
			} 
			else 
			{
				printe(ERR_help_bad_cmd, E_ERROR,
					t->key);
				return 0;
			}
		}
	} 
	return 1;
}
