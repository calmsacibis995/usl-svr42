/*
 * $Copyright:
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */
#ident	"@(#)debugger:libcmd/common/Execute.C	1.13.1.2"
#include "Event.h"
#include "Parser.h"
#include "global.h"
#include "utility.h"
#include "Interface.h"
#include "Location.h"
#include "LWP.h"
#include "Proglist.h"
#include "Shell.h"
#include "Input.h"
#include "Expr.h"
#include "Language.h"
#include "Link.h"
#include "Scanner.h"
#include "Rvalue.h"
#include "Keyword.h"
#include "TSClist.h"
#include "str.h"
#include "Buffer.h"
#include "Manager.h"
#include "Dbgvarsupp.h"
#include "PtyList.h"

#include <sgs.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/procfs.h>
#include <sys/wait.h>

class jbuf	: public Stack
{
public:
	sigjmp_buf	jb;
};

static jbuf		jmpstack;

int			vmode = V_ALL;	   // verbosity
int			wait_4_proc = WAIT; // synchronous by default
int			follow_mode = FOLLOW_ALL;
int			num_line = 10; // for dis and list
int			num_bytes = 256; // for dump
FILE			*log_file;	// session logging
int			cmd_result;
int			last_error;
char			*original_dir;	// original working dir
static Proclist		pe(1);
static int		change_working_dir(char *);
static int		print_working_dir();

#define	LOOP_CONT	2
#define	LOOP_BREAK	3

// also used by Parser for change command to determine type of
// event in order to parse remainder of command.
int
parse_event_num(char * num_text)
{
	char * actual_text = num_text;
	if (*num_text == '%' || *num_text == '$')
	{
		Debug_var_support * var_table;
		var_table = new Debug_var_support(0, 0, 1, 1, 1);
		var_table->Find(num_text);
		actual_text = var_table->Value();
		delete var_table;
		if (actual_text == 0) 
		{
			printe(ERR_eval_fail_expr, E_ERROR, num_text);
			return 0;
		}
	}
	char * end_char;
	long num = strtol(actual_text, &end_char, 0);
	if (end_char == actual_text || (num == 0))
		printe(ERR_bad_event_id, E_ERROR, actual_text);
	// event num is 0 if the conversion was unsuccessful
}

int
execute( Node *n )
{
	static int	depth = 0;
	static int	ret;
	static const char *log_file_name;

	Proclist	*procl;

	if (!n) 
	{
		cmd_result = 1;
		return 1;
	}

	if (!depth)
		ret = 1;
	++depth;

#ifdef DEBUG
	if (debugflag & 2)
		n->dump();
#endif

	if ( (n->opts & DASH('p')) &&
		(n->optype[1] == PROCLIST) ) 
		procl = n->second.procs;
	else
		procl = 0;

	switch( n->op ) 
	{
	case ALIAS:
		if ( n->opts & DASH('r') ) 
		{
			rm_alias(n->second.cp);
			break;
		} 
		if ( n->second.cp ) 
		{
			if ( n->third.tl ) 
			{
				// check for collisions and warn
				check_collisions( n->second.cp, ALIASES|COMMANDS);
				// change this alias
				make_alias(n->second.cp,
					n->third.tl);
			}
			else 
			{
				// print this alias
				print_alias(n->second.cp);
			}
		} 
		else 
		{
			// print all aliases
			print_alias(0);
		}
		break;
	case BREAK:
		if (jmpstack.is_empty())
		{
			printe(ERR_no_while, E_ERROR);
			ret = 0;
		}
		else
		{
			jbuf	*j;
			j = (jbuf *)jmpstack.pop();
			siglongjmp(j->jb, LOOP_BREAK);
		}
		break;
        case CANCEL:
		ret = cancel_sig(procl, n->third.i);
		break;
	case CD:
		ret = change_working_dir(n->second.cp);
		break;
	case CHANGE:
		{
			int		count;
			int		event;
			Eventlist	*el = n->third.events;
			int		set_quiet = -1;
			Systype		systype = NoSType;

			event = parse_event_num((*el)[0]);
			if (event == 0) break;

			if (n->optype[0] == INT)
				count = n->first.i;
			else
				count = -1;
			if (n->opts & DASH('x'))
			{
				if (n->opts & DASH('e'))
					systype = Entry_exit;
				else
					systype = Exit;
			}
			else if (n->opts & DASH('e'))
				systype = Entry;
			if (n->opts & DASH('q'))
				set_quiet = 1;
			else if (n->opts & DASH('v'))
				set_quiet = 0;

			ret = change_event(event, procl, count, systype,
				set_quiet, n->fourth.cp, n->fifth.np);
		}
		break;
	case CMDLIST:
		execute( n->first.np );
		execute( n->second.np );
		break;
	case CONTINUE:
		if (jmpstack.is_empty())
		{
			printe(ERR_no_while, E_ERROR);
			ret = 0;
		}
		else
		{
			jbuf	*j;
			j = (jbuf *)jmpstack.pop();
			jmpstack.push((Link *)j);
			siglongjmp(j->jb, LOOP_CONT);

		}
		break;
        case CREATE:
		{
			int	redir = DEFAULT_IO;
			int	follow = follow_mode;
			int	on_exec = 0;
			if (n->opts & DASH('d'))
				redir = DIRECT_IO;
			else if (n->opts & DASH('r'))
				redir = REDIR_IO;
			// undocumented option -e - stop
			// on exec - don't go past dynamic linker
			if (n->opts & DASH('e'))
				on_exec = 1;
			if (n->optype[1] == CHARSTAR)
			{
				char	*foll = n->second.cp;
				if (strcmp(foll, "none") == 0)
					follow = FOLLOW_NONE;
				else if (strcmp(foll, "all") == 0)
					follow = FOLLOW_ALL;
				else if (strcmp(foll, "procs") == 0)
					follow = FOLLOW_PROCS;
#if 0
				else if (strcmp(foll, "threads") == 0)
					follow = FOLLOW_THREADS;
#endif
				else
				{
					printe(ERR_dashf_arg, E_ERROR);
					ret = 0;
					break;
				}
			}
			ret = create_process(n->first.cp, redir, follow, on_exec);
			break;
		}
	case DIS:
		{
			unsigned int	count = (unsigned int)num_line;

			if (n->optype[2] == INT)
				count = (unsigned int)n->third.i;
			ret = disassem_cnt(procl, n->fourth.loc, count,
				(n->opts & DASH('f')));
		}
		break;
	case DELETE:
	case DISABLE:
	case ENABLE:
	{
		Event_op op;
		int	fail = 0;
		switch(n->op)
		{
		case DELETE:
			op = M_Delete;
			break;
		case DISABLE:
			op = M_Disable;
			break;
		case ENABLE:
			op = M_Enable;
			break;
		}

		if (n->opts & DASH('a'))
		{
			if (n->optype[2] == NODE)
			{
				Node	*n2 = n->third.np;
				int	etype;

				switch(n2->op) {
				case STOP:	etype = E_STOP; break;
				case SIGNAL:	etype = E_SIGNAL; break;
				case SYSCALL:	etype = E_SCALL; break;
				case ONSTOP:	etype = E_ONSTOP; break;
				default:	fail = 1; break;
				}
				if (fail)
				{
					char	*cmd;
					switch(op)
					{
					case M_Delete:
						cmd = "delete";
						break;
					case M_Disable:
						cmd = "disable";
						break;
					case M_Enable:
						cmd = "enable";
						break;
					}
					printe(ERR_event_op_bad_cmd,
						E_ERROR, cmd);
					ret = 0;
					break;
				}
				ret = m_event.event_op(procl, etype, op);
			}
			else
				ret = m_event.event_op(procl, 0, op);
			break;
		}
		else if (n->optype[2] == EVENTLIST)
		{
			int		eid, i= 0;
			Eventlist	*el = n->third.events;

			if (procl)
			{
				printe(ERR_opt_combo_exec, E_ERROR,
					"-p", "an event list");
				ret = 0;
				break;
			}

			while((*el)[i])
			{
				eid = parse_event_num((*el)[i]);
				if (eid != 0) 
					if (!m_event.event_op(eid, op))
						fail = 1;
				i++;
			}
			ret = (fail == 0);
		}
		else 
		{
			char	*cmd;
			switch(op)
			{
			case M_Delete:
				cmd = "delete";
				break;
			case M_Disable:
				cmd = "disable";
				break;
			case M_Enable:
				cmd = "enable";
				break;
			}
			printe(ERR_event_op_list, E_ERROR, cmd);
			ret = 0;
		}
		break;
	}
	case DUMP:
		{
			int	count = num_bytes;
			if (n->optype[2] == INT)
				count = n->third.i;
			ret = dump_raw(procl, n->fourth.loc, count);
		}
		break;
	case EVENTS:
		if (n->optype[2] == EVENTLIST)
		{
			int		eid, i= 0;
			Eventlist	*el = n->third.events;

			printm(MSG_event_header_f);
			while((*el)[i])
			{
				eid = parse_event_num((*el)[i]);
				if (eid != 0) 
					ret = m_event.event_op(eid, M_Display);
				i++;
			}
		}
		else
		{
			static int	set;
			// if no proclist, set to %proc - event_op
			// assumes all procs in program for display
			if (!set)
			{
				pe.ptr[0] = str("%proc");
				set = 1;
			}
			if (procl)
				ret = m_event.event_op(procl, 0, M_Display);
			else
				ret = m_event.event_op(&pe, 0, M_Display);
		}

		break;
	case EXPORT:
		Export_variable(n->third.cp);
		break;
	case FC:
		ret = ksh_fc(n->third.cp);
		break;
	case FUNCTIONS:
		ret = functions(n->third.cp, n->fourth.cp,
			(n->opts & DASH('s')));
		break;
        case GRAB:
		if (n->opts & DASH('c'))
		{
			char	*core = n->second.cp;
			if (n->opts & DASH('f'))
			{
				printe(ERR_opt_combo_exec, E_ERROR, "-f", "-c");
				ret = 0;
			}
			else if (n->opts & DASH('l'))
			{
				printe(ERR_opt_combo_exec, E_ERROR, "-l", "-c");
				ret = 0;
			}
			else 
				ret = grab_core((*n->fourth.files)[0], core);
		}
		else
		{
			int	follow = follow_mode;

			if ((n->opts & DASH('l')) && 
				(n->fourth.files->count > 1))
			{
				printe(ERR_grab_dashl, E_ERROR);
				ret = 0;
				break;
			}
			if (n->optype[4] == CHARSTAR)
			{
				char	*foll = n->fifth.cp;
				if (strcmp(foll, "none") == 0)
					follow = FOLLOW_NONE;
				else if (strcmp(foll, "all") == 0)
					follow = FOLLOW_ALL;
				else if (strcmp(foll, "procs") == 0)
					follow = FOLLOW_PROCS;
#if 0
				else if (strcmp(foll, "threads") == 0)
					follow = FOLLOW_THREADS;
#endif
				else
				{
					printe(ERR_dashf_arg, E_ERROR);
					ret = 0;
					break;
				}
			}
			ret = grab_process(n->fourth.files,
				n->third.cp,follow);
		}
		break;
        case HALT:
		ret = stop_process( procl );
		break;	
        case HELP:
		ret = do_help( n->third.cp );
		break;
	case IF:
		{
			char *e = n->first.cp;
			LWP	*l = proglist.current_lwp();
			Frame	*f = 0;
			Iaddr	pc = ~0;
			Expr exp(e, l);

			if (l)
			{
				f = l->curframe();
				pc = f->pc_value();
			}
			if (!exp.eval(l, pc, f))
			{
				ret = 0;
				break;
			}
			if (exp.exprIsTrue(l, f))
			{
				execute( n->second.np );
			}
			else if (n->optype[2] == NODE) // else part
			{
				execute( n->third.np );
			}
		}
		break;
	case INPUT:
		if ((n->opts & DASH('p')) && (n->opts & DASH('r')))
		{
			printe(ERR_option_mix, E_ERROR, "p", "r");
			ret = 0;
			break;
		}
		ret = input_pty(n->second.cp, n->third.cp, n->fourth.cp,
			((n->opts & DASH('n')) != 0));
		break;
	case JUMP:
		ret = jump(procl, n->third.loc);
		break;
        case KILL:
		{
			int	nsig = 0;
			if (n->optype[2] == INT)
			{
				int	mask = n->third.i;

				for (int i = 0; i < NSIG; i++)
				{
					nsig++;
					if (mask & 0x1)
						break;
					mask >>= 1;
				}
			}
			else
				nsig = SIGKILL;
			if (nsig == SIGKILL)
			{
				ret = destroy_process(procl, 1);
			}
			else
				ret = send_signal(procl, nsig);
			break;
		}
	case LIST:
		{
			Location	*l = 0;
			char		*re = 0;
			int		count = -1;

			if (!proglist.current_lwp())
			{
				printe(ERR_no_proc, E_ERROR);
				ret = 0;
				break;
		
			}
			if (n->opts & DASH('c'))
				count = n->second.i;
			if (n->optype[2] == LOC)
				l = n->third.loc;
			else if (n->optype[2] == CHARSTAR)
				re = n->third.cp;

			ret = list_src(proglist.current_lwp(), 
				count, l, re,
				((n->opts & DASH('d')) != 0));
			break;
		}
	case LOGOFF:
		if (log_file)
		{
			fclose(log_file);
			log_file = 0;
		}
		break;
	case LOGON:
		{
			const char *mode = "a";
			if (log_file)
			{
				printe(ERR_already_logging, E_ERROR);
				break;
			}
			if (n->third.cp)
			{
				log_file_name = n->third.cp;
				mode = "w";
			}
			if (!log_file_name)
			{
				printe(ERR_no_log_file, E_ERROR);
				ret = 0;
				break;
			}
			if ((log_file = debug_fopen(log_file_name, mode)) == 0)
			{
				printe(ERR_cant_open, E_ERROR, log_file_name, strerror(errno));
				ret = 0;
				break;
			}
		}
		break;
	case MAP:
		ret = print_map(procl);
		break;
	case ONSTOP:
		if (n->optype[3] != NODE)
			ret = m_event.event_op(procl, E_ONSTOP, 
				M_Display);
		else
			ret = set_onstop(procl, n->fourth.np);
		break;
	case PENDING:
			// gui only
			ret = pending_sigs(procl);
			break;
	case PFILES:
			// gui only
			ret = print_files(procl);
			break;
	case PPATH:
			// gui only
			ret = print_path(procl, n->third.cp);
			break;
	case PRINT:
		ret = print_expr(procl, n->third.cp, n->fourth.exp);
		break;
        case PS:
		ret = proc_status(procl);
		break;
        case PWD:
		ret = print_working_dir();
		break;
        case QUIT:
                quitflag = 1;
		break;
	case REDIR:
	{
		FILE	*fp;
		int	success = 1;
		int	fopened = 0;
		switch ( n->third.i ) 
		{
		case RedirFile:
			if ((fp = debug_fopen(n->second.cp, "w")) == 0)
			{
				printe(ERR_cant_create_file, E_WARNING, n->second.cp, strerror(errno));
				success = 0;
			}
			else
			{
				pushoutfile(fp);
				fopened = 1;
			}
			break;
		case RedirAppend:
			if ((fp = debug_fopen(n->second.cp, "a")) == 0)
			{
				printe(ERR_cant_append,E_WARNING, n->second.cp, strerror(errno));
				success = 0;
			}
			else
			{
				pushoutfile(fp);
				fopened = 1;
			}
			break;
		case RedirPipe:
			if ((fp = popen(n->second.cp, "w")) == 0)
			{
				printe(ERR_sys_pipe, E_WARNING, strerror(errno));
				success = 0;
			}
			else
				pushoutfile(fp);
			break;
		}
		if (success)
		{
			execute( n->first.np );
			popout();
			if (fopened)
				fclose(fp);
			else
				pclose(fp);
		}
		break;
	}
	case REGS:
		ret = printregs(procl);
		break;
        case RELEASE:
		ret = release_proclist(procl, ((n->opts & DASH('s')) == 0));
		break;
	case RENAME:
		ret = rename_prog(n->third.cp, n->fourth.cp);
		break;
        case RUN:
		{
			int rwait = wait_4_proc;
			if (n->opts & DASH('b'))
				rwait = NO_WAIT;
			else if (n->opts & DASH('f'))
				rwait = WAIT;
			if (n->opts & DASH('r'))
				ret = run ( procl, 1, n->third.loc, rwait );
			else
				ret = run ( procl, 0, n->third.loc, rwait );
		}
            break;
	case SAVE:
		execute( n->first.np );
		break;
        case SCRIPT:
		if (InputFile(n->third.cp, 0, ((n->opts & DASH('q')) == 0)))
		{
			printe(ERR_cant_open, E_ERROR, n->third.cp,
				strerror(errno));
			ret = 0;
			break;
		}
		if (get_ui_type() == ui_gui)
			printm(MSG_script_on);
		doscript();
		CloseInput();
		if (get_ui_type() == ui_gui)
			printm(MSG_script_off);
		break;

	case SET:
		ret = set_val(procl, n->third.cp, n->fourth.exp);
		break;
	case SHELL:
		ret = do_shell( n->first.cp, 1 );
		break;
        case SIGNAL:
		if (n->optype[2] != INT)
		{
			ret = m_event.event_op(procl, E_SIGNAL, 
				M_Display);
		}
		else
		{
			int		nsig = 0;
			int		mask = n->third.i;
			sigset_t	sigs;

			if ((n->opts & DASH('i')) && n->fourth.np)
			{
				printe(ERR_dashi_cmd, E_ERROR);
				ret = 0;
				break;
			}
			premptyset(&sigs);

			for (int i = 0; i < NSIG; i++)
			{
				nsig++;
				if (mask & 0x1)
					praddset(&sigs, nsig);
				mask >>= 1;
			}
			ret = set_signal(procl, sigs, n->fourth.np, 
				((n->opts & DASH('i')) != 0), 
				((n->opts & DASH('q')) != 0));
		}
		break;
	case STACK:
		{
			int	first = -1;
			if (n->opts & DASH('f'))
				first = n->third.i;
			ret = print_stack(procl, n->fourth.i, first);
		}
		break;
        case STEP:
		{
			int count = 1;
			int swait = wait_4_proc;
			int talk = vmode;
			int level, where;

			if ( n->optype[2] == INT )
			{
				count = n->third.i;
				if (count == 0) // step forever
					count = STEP_INF_ANNOUNCE;
			}
			if (n->opts & DASH('b'))
				swait = NO_WAIT;
			else if (n->opts & DASH('f'))
				swait = WAIT;
			if ( n->opts & DASH('q') ) 
				talk = 0;

			if ( n->opts & DASH('i') ) 
				level = STEP_INSTR;
			else
				level = STEP_STMT;
		
			if ( n->opts & DASH('o') )
				where = STEP_OVER;
			else
				where = STEP_INTO;

			ret = single_step(procl, 0, count, talk, level,
				where, swait );
			break;
		}
	case STOP:
		if ( n->optype[2] != EVENT_EXPR )
		{	// no locations
			ret = m_event.event_op(procl, E_STOP, 
				M_Display);
		}
		else
		{
			ret = set_stop( procl, n->third.stop, n->fourth.np, 
				n->first.i, ((n->opts & DASH('q')) != 0)); 
		}
	    	break;
	case SYSCALL:
		if (n->optype[2] != SYSCALLS)
		{
			ret = m_event.event_op(procl, E_SCALL, 
				M_Display);
		}
		else 
		{
			Systype	stype;

			if (n->opts & DASH('x'))
			{
				if (n->opts & DASH('e'))
					stype = Entry_exit;
				else
					stype = Exit;
			}
			else
				stype = Entry;
			ret = set_syscall(procl, n->third.sys, 
					stype, n->fourth.np, n->first.i, 
					((n->opts & DASH('q')) != 0));
		}
		break;
        case SYMBOLS:
		{
			int	mode = 0;

			if (n->opts & DASH('g'))
				mode |= SYM_GLOBALS;
			if (n->opts & DASH('f'))
				mode |= SYM_FILES;
			if (n->opts & DASH('l'))
				mode |= SYM_LOCALS;
			if (n->opts & DASH('u'))
				mode |= SYM_USER;
			if (n->opts & DASH('d'))
				mode |= SYM_BUILT_IN;
			if (n->opts & DASH('v'))
				mode |= SYM_VALUES;
			if (n->opts & DASH('t'))
				mode |= SYM_TYPES;

			// printing the locals is the default
			if (!(mode & (SYM_GLOBALS|SYM_FILES|SYM_USER|SYM_BUILT_IN)))
				mode |= SYM_LOCALS;
			ret = symbols(procl, n->third.cp, n->fifth.cp, 
					n->fourth.cp, mode);
			break;
		}
	case VERSION:
			// gui only
			printm(MSG_version, prog_name, EDEBUG_PKG, EDEBUG_REL);
			break;
	case WHILE:
		{
			// We maintain a stack of jmp buffers
			// for use with break or continue commands.
			// Right before entering the while loop
			// we do a setjmp; break or continue will
			// do a longjmp back to that spot; if break,
			// just return, else re-enter loop.

			int	i;
			jbuf *j = new jbuf;
			char *e = n->first.cp;
			jmpstack.push((Link *)j);
			if ((i =  sigsetjmp(j->jb, 1)) == LOOP_BREAK)
			{
				// break command
				delete j;
				break;
			}
			while(1)
			{
				sigrelse(SIGINT);
				if (interrupt & sigbit(SIGINT))
					break;
				LWP	*l = proglist.current_lwp();
				Frame	*f = 0;
				Iaddr	pc = ~0;
				Expr 	exp(e, l);
				if (l)
				{
					f = l->curframe();
					pc = f->pc_value();
				}
				if (!exp.eval(l, pc, f))
				{
					ret = 0;
					break;
				}
				if (exp.exprIsTrue(l, f))
				{
					if (execute( n->second.np ) != 0)
					{
						ret = 0;
						break;
					}
				}
				else
				{
					break;
				}
			}
			sighold(SIGINT);
			// if no break , we must pop stack
			(void)jmpstack.pop();
			delete j;
		}
		break;
	case NoOp:
		break;
	default:
		printe(ERR_internal, E_FATAL, "execute", __LINE__);
	}

	--depth;
	if (!depth)
	{
		proglist.reset_lists();
		if (n->op != SAVE)
			delete n;
	}

	if (n->op != CMDLIST)
		message_manager->sync_request();

	// ret is 0 if there was an error.
	// last_error is non-zero if there was an error.
	//    last_error is set if an error message was printed.
	// 0 should be returned if there was not an error.
	if (!ret && !last_error)
		cmd_result = 1;
	else
		cmd_result = last_error;

	return cmd_result;
}


static int
change_working_dir(char *newdir)
{
	// save original working dir
	if (!original_dir)
	{
		original_dir = new char[PATH_MAX];
		if (getcwd(original_dir, PATH_MAX) == 0)
		{
			printe(ERR_pwd_fail, E_ERROR, strerror(errno));
			return 0;
		}
	}
	if (!newdir)
		newdir = getenv("HOME");
	if (!newdir || !*newdir)
	{
		printe(ERR_no_new_dir, E_ERROR);
	}
	if (chdir(newdir) != 0)
	{
		printe(ERR_cwd_fail, E_ERROR, newdir, strerror(errno));
		return 0;
	}
	if (get_ui_type() == ui_gui)
		printm(MSG_cd, newdir);
	return 1;
}

static int
print_working_dir()
{
	static char	*dirname;
	if (!dirname)
		dirname = new char[PATH_MAX];
	if (getcwd(dirname, PATH_MAX) == 0)
	{
		printe(ERR_pwd_fail, E_ERROR, strerror(errno));
		return 0;
	}
	printm(MSG_pwd, dirname);
	return 1;
}

int
do_shell( const char *p, int update_last )
{
	const char	*cmd;
	static char	*oldcmd;
	PtyInfo		*pty = 0;
	ui_type		ut = get_ui_type();

	if (update_last)
	{
		// !! executes last shell cmd
		if (strcmp(p, "!") == 0)
		{
			if (!oldcmd)
			{
				printe(ERR_no_prev_cmd, E_ERROR);
				return 0;
			}
		} 
		else
		{
			delete oldcmd;
			oldcmd = new char[strlen(p) + 1];
			strcpy(oldcmd, p);
		}
		cmd = oldcmd;
	}
	else
		cmd = p;

	if (ut != ui_gui)
		restore_tty();
	pid_t pid = Shell( (char *)cmd, ut == ui_gui, pty);

	if ( pid == -1 ) 
	{
		delete pty;
		printe(ERR_fork_failed, E_ERROR, strerror(errno));
		return 0;
	}

	int status = 0;
	if ((ut == ui_gui) && pty)
		sigrelse(SIGPOLL);
	do
	{
		errno = 0;
		if (waitpid(pid, &status, 0) == pid)
			break;
	} while (errno != ECHILD);
		
	if (ut != ui_gui)
		debugtty();
	else
	{
		if (pty)
		{
			delete pty;
			sighold(SIGPOLL);
		}
	}

	if ( WIFSIGNALED(status))
	{			// signal
		unsigned sig = WTERMSIG(status);
		if (WCOREDUMP(status))
			printe(ERR_shell_cmd_core, E_ERROR, sig);
		else
			printe(ERR_shell_cmd, E_ERROR, sig);
	}
	return 1;
}

// uses the third level global buffer - the first two levels
// may be in use in event display
const char *
list_cmd(Node *n)
{
	static int	depth;
	Keyword		*k;
	char		arg_ops[MAX_ARGS];

	if (!n) 
	{
		return 0;
	}
	if (!depth)
		gbuf3.clear();

	++depth;
	for (int j = 0; j < MAX_ARGS; j++)
		arg_ops[j] = 0;

	// CMDLIST, REDIR, and SAVE don't have command strings
	if ((k = keyword(n->op)) != 0)
	{
		gbuf3.add(k->str);
		gbuf3.add(' ');
	}

	// print out options - temporarily skipping options that have
	// arguments
	if (n->opts)
	{
		int	hasopts = 0;
		int	nopts = n->opts;
		const char *p = k->opts;
		const char *r;
		while (*p)
		{
			if (*(p+1) == ':')
			{
				r = p+2;
				arg_ops[(*r - '0') -1] = *p;
				p = r+1;
			}
			else if (DASH(*p) & nopts)
			{
				if (!hasopts)
				{
					gbuf3.add('-');
					hasopts = 1;
				}
				gbuf3.add(*p++);
			}
			else
				p++;
		}
		if (hasopts)
			gbuf3.add(' ');
	}
	switch( n->op ) 
	{
	case REDIR:
		list_cmd( n->first.np );
		switch ( n->third.i ) 
		{
		case RedirFile:
			gbuf3.add("> ");
			break;
		case RedirAppend:
			gbuf3.add(">> ");
			break;
		case RedirPipe:
			gbuf3.add("| ");
			break;
		}
		gbuf3.add(n->second.cp);
		gbuf3.add(' ');
		break;
	case SAVE:
		list_cmd( n->first.np );
		break;
	case CMDLIST:
	{
		static int nest;
		if ( n->first.np )
		{
			if ( n->first.np->op == CMDLIST ) 
			{
				gbuf3.add("{ ");
				++nest;
			}
			list_cmd( n->first.np );
		} 
		else 
		{
			gbuf3.add("{ } ");
		}
		if ( n->second.np ) 
		{
			gbuf3.add("; ");
			list_cmd( n->second.np );
		} 
		else if ( nest ) 
		{
			gbuf3.add("} ");
			--nest;
		}
		break;
	}
	case LIST:
		if (n->optype[2] == LOC)
		{
			Location *l = n->third.loc;
			if ( l )
				gbuf3.add(l->print());
				gbuf3.add(' ');
		}
		else if (n->optype[2] == CHARSTAR)
		{
			char c;
			if (n->opts & DASH('d'))
				c = '/';
			else
				c = '?';
			gbuf3.add(c);
			gbuf3.add(n->third.cp);
			gbuf3.add(c);
		}
		break;
	case NoOp:
		break;
	default:
		for (int i = 0; i < MAX_ARGS; i++)
		{
			void	*opnd = n->getopnd(i+1);
			if (!opnd)
				continue;
			if (arg_ops[i] != 0)
			{
				gbuf3.add('-');
				gbuf3.add(arg_ops[i]);
				gbuf3.add(' ');
			}

			switch(n->optype[i])
			{
			case NODE:
				gbuf3.add("{ ");
				list_cmd((Node *)opnd);
				if ((n->op == IF) && (i == 0) && 
					(n->optype[2] == NODE))
						gbuf3.add(" } else");
				else
					gbuf3.add(" }");
				break;
			case CHARSTAR:
				gbuf3.add((char *)opnd);
				break;
			case STRING_OPND:
				gbuf3.add(fmtstring((char *)opnd));
				break;
			case INT:
				gbuf3.add((int)opnd);
				break;
			case TOKEN:
				gbuf3.add(print_tok_list((Token *)opnd));
				break;
			case PROCLIST:
				gbuf3.add(((Proclist *)opnd)->print());
				break;
			case LOC:
				gbuf3.add(((Location *)opnd)->print());
				break;
			case SYSCALLS:
				gbuf3.add(((IntList *)opnd)->print());
				break;
			case EXPLIST:
				gbuf3.add(((Exp *)opnd)->print(','));
				break;
			case EVENTLIST:
				gbuf3.add(((Eventlist *)opnd)->print());
				break;
			case EVENT_EXPR:
				gbuf3.add(print_stop((StopEvent *)opnd, 0));
				break;
			default:	break;
			}
			gbuf3.add(' ');
		}
		break;
	}
	--depth;
	return (char *)gbuf3;
}
