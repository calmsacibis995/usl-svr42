/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_DISPATCHER_H
#define	_DISPATCHER_H
#ident	"@(#)debugger:gui.d/common/Dispatcher.h	1.5"

#include <stdarg.h>
#include "Message.h"

class Component;
class Command_sender;
class Process;
class Window_set;

// The Dispatcher handles all the message traffic between the gui
// and the debugger engine.

// Commands are sent to the debugger via send_msg; any object that
// calls send_msg must be derived from Command_sender.  The dispatcher
// sends the results of the command back to the Command_sender
// via its de_message and cmd_complete member functions.
// de_message may be called more than once per command.
// All incoming messages except query responses are handled by process_msg()

// Queries (commands asking for information the gui needs immediately)
// are handled by calls to query() followed by get_response().
// A response may consist of more than one message, so get_response()
// should be called in a loop until it returns 0.
 
class Dispatcher
{
	Message		current_msg;		// incoming message
	Message		out_msg;		// outgoing message
	int		first_new_process;	// flags first process in a grab
	int		io_flag;		// set the io flag for new processes
	int		process_killed;		// flag telling when to call Window_set::set_current
	Window_set	*save_ws;	// save window set for make_new_processes

	void	sync_response();
public:
		Dispatcher()	{ first_new_process = 1; io_flag = 0; save_ws = 0;
				  process_killed = 0; }
		~Dispatcher()	{}

	void	process_msg();
	void	send_msg(Command_sender *, DBcontext, const char * ...);
	void	query(Command_sender *, DBcontext, const char * ...);
	Message	*get_response();
	void	send_interrupt_cb(Component *, int ok_flag);
	void	make_new_processes();
};

extern	Dispatcher	dispatcher;

#endif	// _DISPATCHER_H
