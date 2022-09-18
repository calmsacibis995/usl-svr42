#ident	"@(#)debugger:libutil/common/create.C	1.4"
#include "Manager.h"
#include "LWP.h"
#include "Process.h"
#include "Program.h"
#include "PtyList.h"
#include "Proglist.h"
#include "Interface.h"
#include "utility.h"
#include "global.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

static void	destroy_create_session(int);
int		redir_io;	// no redir by default

int
create_process(const char *cmdline, int redirect, int follow, 
	int on_exec)
{
	LWP	 	*lwp;
	LWP		*first = 0;
	char		*cmd, *cend, *ncmd;
	int		errors = 0, len;
	int		redir;
	int		input = -1, output = -1;
	int		pfd[2];
	int		ignore_fork = 0;
	// MORE - need option to ignore threads as well

	static int	create_id;
	static char	*oldargs;
	static char	old_redirect;

	if (follow < FOLLOW_PROCS)
		ignore_fork = 1;

	switch(redirect)
	{
	default:
	case DEFAULT_IO:
		if (redir_io)
			redir = REDIR_PTY;
		else
			redir = 0;
		break;
	case REDIR_IO:
		redir = REDIR_PTY; 
		break;
	case DIRECT_IO:
		redir = 0;
		break;
	}

	if (!cmdline)
	{
		// recreate previous create session
		if (!oldargs)
		{
			printe(ERR_no_previous_create, E_ERROR);
			return 0;
		}

		destroy_create_session(create_id);
		len = strlen(oldargs); // used below
		if (redirect == DEFAULT_IO)
		{
			if (old_redirect == REDIR_IO)
				redir = REDIR_PTY;
			else if (old_redirect == DIRECT_IO)
				redir = 0;
		}
		printm(MSG_oldargs, oldargs);
	}
	else
	{
		len = strlen(cmdline);
		old_redirect = redirect;
		delete oldargs;
		oldargs = new(char[len +1]);
		strcpy(oldargs, cmdline);
		if (get_ui_type() == ui_gui)
			printm(MSG_oldargs, oldargs);
	}
	
	create_id++;
	pfd[0] = pfd[1] = -1;
	ncmd = new(char[len + 1]);
	strcpy(ncmd, oldargs);
	cend = ncmd + len;
	cmd = ncmd;

	// lwp pipeline
	// Parse cmdline; each time we get a non-quoted pipe
	// character, create a process for command up to that
	// point, setting up pipes for i/o redirection
	while(*cmd && (cmd < cend) && !errors)
	{
		register char	*p;

		while(isspace(*cmd))
			cmd++;
		if (!*cmd)
			break; 
		p = cmd;
		while(*p)
		{
			switch(*p)
			{
			case '\\':
				// escape; eat it and next char
				p++;
				if (*p)
					p++;
				continue;

			case '\'':	
				// single quote; eat until next one
				do {
					p++;
				} while(*p && *p != '\'');
				if (*p)
					p++;
				continue;
			case '"':	
				// double quote; eat until next one
				do {
					p++;
				} while(*p && *p != '"');
				if (*p)
					p++;
				continue;
			case '|':
				// pipe
				*p = '\0';
				if (pipe(pfd) == -1)
				{
					printe(ERR_sys_pipe, E_ERROR, 
						strerror(errno));
					errors++;
					break;
				}
				redir |= REDIR_OUT;
				output = pfd[1];
				break;
			default:	
				p++;
				break;
			}
		}
		lwp = new LWP();
		if (!lwp->create(cmd, proglist.next_proc(), input, 
			output, redir, create_id, on_exec, ignore_fork))
		{
			delete lwp;
			proglist.dec_proc();
			errors++;
			break;
		}
		if (!first)
			first = lwp;
		if (redir & REDIR_IN)
		{
			close(input);
		}
		else
			redir |= REDIR_IN;
		input = pfd[0];

		if (redir & REDIR_OUT)
		{
			redir &= ~REDIR_OUT;
			close(output);
			output = -1;
		}
		message_manager->reset_context(lwp);
		printm(MSG_createp, lwp->prog_name(), lwp->lwp_name());
		cmd = p + 1;
	}
	if (!errors && (redir & REDIR_PTY))
		printm(MSG_new_pty,
			lwp->process()->program()->childio()->name());
	message_manager->reset_context(0);
	delete ncmd;
	if (errors)
	{
		destroy_create_session(create_id);
		printe(ERR_create_fail, E_ERROR);
		delete oldargs;
		oldargs = 0;
		return 0;
	}
	proglist.set_current(first, 0);
	return 1;
}

static void
destroy_create_session(int id)
{
	plist	*list;

	list = proglist.all_live(id);
	for (LWP *l = list++->p_lwp; l; l = list++->p_lwp)
	{
		destroy_process(l, 1);
	}
	proglist.prune();
}
