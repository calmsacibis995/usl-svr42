/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _COMMAND_H
#define _COMMAND_H
#ident	"@(#)debugger:gui.d/common/Command.h	1.5"

#include "Component.h"
#include "Windows.h"

class Message;
class Process;
class Script_dialog;
class Input_dialog;
class Status_pane;
class Action_dialog;

// The Command window has three components:
//	the status pane, giving the status of the current process
//	the transcript pane, which displays the event history and I/O from	
//		all programs in the window set, and
//	the command line, which gives the user access to the debugger's
//		command line interface
// There is one command window per window set.

class Command_window : public Base_window
{
	Status_pane	*status_pane;
	Text_area	*transcript;
	Text_line	*command;

	Script_dialog	*script_box;
	Input_dialog	*input_box;

	Boolean		has_selection;	// true if text is selected in the
					// transcript pane
	Boolean		in_continuation;	// true if escaped newline typed
						// in command line
public:
			Command_window(Window_set *);
			~Command_window();

			// access functions
	Text_area	*get_transcript()	{ return transcript; }

			// Component callbacks
	void		script_dialog_cb(Component *, void *);
	void		input_dialog_cb(Component *, void *);
	void		select_cb(Text_area *, int);
	void		copy_cb(Text_area *, void *);
	void		do_command(Component *, const char *);
	void		interrupt_cb(Component *, void *);

			// framework callbacks
	void		update_cb(void *server, Reason_code, void *, Process *);

			// inherited from Command_sender
	void		de_message(Message *);
	void		cmd_complete();

			// functions inherited from Base_window
	int		check_sensitivity(int sense);
	void		popup();
	void		popdown();
};

#endif	// _COMMAND_H
