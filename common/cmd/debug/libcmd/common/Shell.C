#ident	"@(#)debugger:libcmd/common/Shell.C	1.2"

// Invoke the shell to run a UNIX command

#include "Shell.h"
#include "Dbgvarsupp.h"
#include "Interface.h"
#include "PtyList.h"
#include "Program.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>

// invoke shell to create a UNIX command, return pid
// command is the command string to be executed
// redir is 1 if I/O is to be redirected to a pseudo-tty

pid_t
Shell(char *command, int redir, PtyInfo *&pty)
{
	char 			*shpath = 0;
	char 			*shname = 0;
	char			*var;
	pid_t 			pid;
	Debug_var_support	*user_var_table;

	// Get variable textual value.
	user_var_table = new Debug_var_support(0,0,0,0,1);
	user_var_table->Find("$SHELL");
	var = user_var_table->Value();
	delete user_var_table;
	if (var && (*var == '/') &&
		(access(var, X_OK) == 0))
	{
		shpath = new(char[strlen(var) + 1]);
		strcpy(shpath, var);
		shname = strrchr(shpath, '/') + 1;
	}
	else
	{
		shpath = "/usr/bin/sh";
		shname = "sh";
	}
	if (redir)
	{
		pty = new PtyInfo;
		if (pty->is_null())
		{
			return(-1);
		}
	}
	else
		pty = 0;

	switch (pid = fork()) 
	{
	case -1:		// utter failure
		return(-1);
	case 0:			// child exec()s the shell
		// make ourselves a process group leader so we can
		// wait for all children to finish
		if (redir) 
		{		
			redirect_childio(pty->pt_fd());
		}

		execl(shpath, shname, "-c", command, 0);
		_exit(255);	// utter failure
				// call _exit to avoid flushing buffers
	}
	return(pid);
}
