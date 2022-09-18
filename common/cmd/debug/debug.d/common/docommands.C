#ident	"@(#)debugger:debug.d/common/docommands.C	1.3"
// main command loop

#include "global.h"
#include "Input.h"
#include "Interface.h"
#include "Manager.h"
#include "Parser.h"
#include "utility.h"
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

int		quitflag;

#ifndef ALIAS_FILE
#define ALIAS_FILE "/debug_alias"
#endif
#ifndef ALIAS_PATH
#define ALIAS_PATH "/usr/ccs/lib/debug_alias"
#endif

static void
defaults_file(int fd)
{
	const char	*line;

	if (get_ui_type() == ui_gui)
		printm(MSG_script_on);
	InputFile(fd, 0);
	while ((line = GetLine()) != 0)
		parse_and_execute(line);
	CloseInput();
	if (get_ui_type() == ui_gui)
		printm(MSG_script_off);
}

void
do_defaults(int dflt, const char *alias_path)
{
	int	alias_file;
	char	*apath;
	int	alloc = 0;

	if (alias_path)
	{
		apath = new(char[strlen(alias_path)+sizeof(ALIAS_FILE)+1]);
		strcpy(apath, alias_path);
		strcat(apath, ALIAS_FILE);
		alloc = 1;
	}
	else
	{
		apath = ALIAS_PATH;
	}
	if ((alias_file = debug_open(apath, O_RDONLY)) == -1)
	{
		printe(ERR_sys_no_alias, E_WARNING, apath, 
			strerror(errno));
	}
	else
	{
		defaults_file(alias_file);
		close(alias_file);
	}
	if (dflt >= 0)
	{
		defaults_file(dflt);
	}
	InputFile(fileno(stdin), 1);
	if (alloc)
		delete apath;
}

extern void	do_assoccmds();

void
docommands()
{
	while (quitflag == 0) 
	{
		interrupt &= ~sigbit(SIGINT);
		if (!message_manager->docommand())
			break;
		do_assoccmds();
	}
	printm(MSG_quit);
}

void
doscript()
{
	const char	*line;

	while ((line = GetLine()) != 0 && quitflag == 0) 
	{
		if (InputEcho())
			printm(MSG_input_line, Pprompt, line);
		parse_and_execute(line);
		do_assoccmds();
	}
}
