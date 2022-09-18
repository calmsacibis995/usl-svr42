#ident	"@(#)debugger:gui.d/common/Dispatcher.C	1.15"

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

// Debug headers
#include "Msgtypes.h"
#include "Message.h"
#include "Transport.h"
#include "Severity.h"
#include "Vector.h"

// GUI headers
#include "UI.h"
#include "Eventlist.h"
#include "Dispatcher.h"
#include "Proclist.h"
#include "Windows.h"
#include "Command.h"
#include "Dialogs.h"

#include <unistd.h>
#include <signal.h>

Transport transport(fileno(stdin), fileno(stdout), read, write, exit, ts_gui);
Dispatcher	dispatcher;
int		in_script;
int		has_assoc_cmd;

// vector is used to keep a list of all the newly created processes between
// one MSG_cmd_complete and the next MSG_new_pty - that will give the set
// of processes that are associated with that pseudo-terminal
static Vector	vector;

// pack the arguments into the message for transfer to debug
static void
pack(Message &msg, Msg_id mid ...)
{
	va_list	ap;

	msg.clear();
	va_start(ap, mid);
	msg.bundle(mid, E_NONE, ap);
	va_end(ap);
}

// received a sync request from debug, send back a sync response
void
Dispatcher::sync_response()
{
	static Message	*sync_m;

	if (!sync_m)
	{
		sync_m = new Message;
		sync_m->bundle(MSG_sync_response, E_NONE, 0);
	}
	transport.send_message(sync_m, TT_UI_notify, 0, 0);
}

// sprintf the format string and arguments into a command for debug
// The pointer to the object that generated the message is passed
// as the uicontext in the message, so that on getting the response,
// the dispatcher will send the incoming message back to the same object
void
Dispatcher::send_msg(Command_sender *obj, DBcontext context, const char *fmt ...)
{
	const char	*cmdline;
	va_list		ap;

	va_start(ap, fmt);
	cmdline = do_vsprintf(fmt, ap);
	va_end(ap);

	pack(out_msg, MSG_command, cmdline);
	transport.send_message(&out_msg, TT_UI_user_cmd, context, obj);
}

// direct a message from debug to the appropriate places:
// first, to the framework object (dialog, pane, etc) that generated the command,
// next, to the window set that owns the process, so that event notifications
//	are always displayed,
// and then,
//	if the message indicates a change in process state,
// 		to the process list
//	or a change in an event,
//		to the event list
void
Dispatcher::process_msg()
{
	Msg_id		mtype;
	Command_sender	*obj;
	Process		*process;
	Window_set	*ws;
	Message		*new_msg;

	current_msg.clear();
	transport.get_next_message(&current_msg);
	mtype = current_msg.get_msg_id();
	obj = (Command_sender *)current_msg.get_uicontext();

	if (mtype == MSG_quit)
		ui_exit(0);

	if (mtype == MSG_sync_response)	// only gui should generate this
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	// Find the window set where the message should be handled
	// A new process will not have a process object, but one
	// resulting from a create or grab will have a uicontext (command_sender)
	// A message for a newly forked process goes to its parent's window

	process = proclist.find_process(&current_msg);
	if (in_script && obj && obj->get_window_set())
		ws = obj->get_window_set();
	if (process && process->get_window_set())
		ws = process->get_window_set();
	else if (obj && obj->get_window_set())
		ws = obj->get_window_set();
	else if ((ws = (Window_set *)windows.first()) == 0)
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);

	// Messages resulting from commands are handled by the object
	// that generated the command
	// Event notifications will have a process id (the dbcontext)
	// but no object, since they are not the result of specific commands,
	// and are displayed in the command window of the appropriate window set,
	// as is output from commands typed in the command line pane
	if (obj && !in_script)
	{
		if (mtype == MSG_cmd_complete)
			obj->cmd_complete();
		else if (mtype != MSG_sync_request)
			obj->de_message(&current_msg);
	}
	ws->inform(&current_msg);

	// all messages that indicate changes in process state (or event state)
	// go to the process list (or event list), regardless of what
	// object generated them
	switch(mtype)
	{
		case MSG_createp:
			if (process)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
			{
				new_msg = new Message;
				*new_msg = current_msg;
				vector.add(&new_msg, sizeof(Message *));
			}
			save_ws = ws;
			break;

		case MSG_new_pty:
			if (vector.size() == 0)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
				io_flag = 1;
			break;

		case MSG_proc_fork:
			if (!process)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
				proclist.proc_forked(&current_msg, process);
			break;

		case MSG_proc_exec:
			if (!process)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
				proclist.proc_execed(&current_msg, process);
			break;

		case MSG_new_core:
			if (process)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
				proclist.proc_grabbed(&current_msg, ws, 1);
			break;

		case MSG_grab_proc:
			if (process)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
			{
				// if this is the first process grabbed in the
				// current grab command, the process becomes
				// the current process in its window set
				proclist.proc_grabbed(&current_msg, ws,
					first_new_process);
				first_new_process = 0;
			}
			break;

		// not an error to not have a process - this message could appear
		// during a create if one process in a pipeline fails
		case MSG_proc_killed:
			if (process)
			{
				proclist.remove_proc(process);
				if (obj)	// result of a command, like kill
					process_killed = 1;
				else		// process exited while running
					ws->set_current(0);
			}
			break;

		case MSG_proc_exit:
		case MSG_release_run:
		case MSG_release_suspend:
		case MSG_release_core:
			if (!process)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
			{
				proclist.remove_proc(process);
				if (obj)	// result of a command, like kill
					process_killed = 1;
				else		// process exited while running
					ws->set_current(0);
			}
			break;

		// These messages are generated when an event triggers, but may
		// also be generated for other reasons.  is_incomplete() says
		// they are needed to complete the event notification.
		// Ditto for the next group and MSG_source_file
		case MSG_line_src:
		case MSG_disassembly:
		case MSG_dis_line:
		case MSG_line_no_src:
			if (!process)
			{
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
				display_msg(&current_msg);
			}
			else if (process->is_incomplete())
				proclist.finish_update(&current_msg, process);
			break;

		case MSG_loc_sym_file:
		case MSG_loc_sym:
		case MSG_loc_unknown:
			if (!process)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else if (process->is_incomplete())
				proclist.update_location(&current_msg,	process);
			break;

		case MSG_source_file:
			if (!process)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else if (process->is_incomplete())
				proclist.set_path(&current_msg,	process);
			break;

		case MSG_es_stepped:
		case MSG_es_suspend:
		case MSG_es_signal:
		case MSG_es_sysent:
		case MSG_es_sysxit:
		case MSG_es_stop:
			if (!process)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
				proclist.proc_stopped(process);
			break;

		case ERR_step_watch:
			if (!process)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
				proclist.set_state(process, State_stepping);
			break;

		case MSG_proc_start:
			if (process)
				proclist.set_state(process, State_running);
			else
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			break;

		// the process was running to evaluate a function call in an expression
		case MSG_proc_stop_fcall:
			if (process)
				proclist.set_state(process, State_stopped);
			else
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			break;

		case MSG_set_frame:
			if (!process)
			{
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
				break;
			}
			proclist.set_frame(&current_msg, process);
			break;

		case MSG_rename:
			proclist.rename(&current_msg);
			break;

		case MSG_event_assigned:
			if (!process)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			event_list.new_event(&current_msg, process);
			break;

		case MSG_bkpt_set:
		case MSG_bkpt_set_addr:
			event_list.breakpt_set(&current_msg);
			break;

 		case MSG_event_changed:
			event_list.change_event(&current_msg, process);
 			break;
 
		case MSG_event_deleted:
			event_list.delete_event(&current_msg, process);
			break;

		case MSG_event_enabled:
			event_list.enable_event(&current_msg);
			break;

		case MSG_event_disabled:
			event_list.disable_event(&current_msg);
			break;

		// debug is waiting for an acknowledgement from the gui,
		// so it won't get too far ahead in spitting out output
		case MSG_sync_request:
			sync_response();
			// if the command was a create command, the new process
			// messages are saved until the end
			if (vector.size())
				make_new_processes();
			first_new_process = 1;
			if (process_killed)
			{
				Window_set	*ws = (Window_set *)windows.first();
				for ( ; ws; ws = (Window_set *)windows.next())
				{
					if (!ws->current_process())
						ws->set_current(0);
				}
				process_killed = 0;
			}
			break;

		case MSG_cmd_complete:
			break;

		// if a create fails, all processes are killed.
		case ERR_create_fail:
			vector.clear();
			io_flag = 0;
			break;

		// keep the Recreate dialog up-to-date
		case MSG_oldargs:
			set_create_args(&current_msg);
			first_new_process = 1;
			break;

		// keep the Language dialog up-to-date
		case MSG_set_language:
			set_lang(&current_msg);
			break;

		// update current context after a jump command
		case MSG_jump:
			if (!process)
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			else
				proclist.proc_jumped(&current_msg, process);
			break;

		// update current working directory after cd command
		case MSG_cd:
			set_directory();
			break;

		case MSG_script_on:
			in_script++;
			break;

		case MSG_script_off:
			in_script--;
			if (!in_script)
			{
				has_assoc_cmd = 0;
				proclist.update_all();
			}
			break;

		case MSG_assoc_cmd:
			if (!has_assoc_cmd)
			{
				if (ws->get_event_action() != A_raise)
					display_msg((Callback_ptr)Dispatcher::send_interrupt_cb,
						this, "OK", "Interrupt", GM_assoc_cmd);
				send_msg(0, 0, "\n");
			}
			has_assoc_cmd++;
			break;

		default:
			break;
	}

}

// ask debug for information needed before the gui can continue
// the response is handled by get_response()
void
Dispatcher::query(Command_sender *obj, DBcontext context, const char *fmt ...)
{
	const char	*cmdline;
	va_list		ap;

	va_start(ap, fmt);
	cmdline = do_vsprintf(fmt, ap);
	va_end(ap);

	pack(out_msg, MSG_command, cmdline);
	transport.send_message(&out_msg, TT_UI_query, context, obj);
}

Message *
Dispatcher::get_response()
{
	Msg_id	mtype;

	for (;;)
	{
		transport.get_response(&current_msg);
		mtype = current_msg.get_msg_id();

		if (mtype == MSG_sync_request)
			sync_response();
		else if (mtype == MSG_cmd_complete)
		{
			transport.query_done();
			return 0;
		}
		else
			return &current_msg;
	}
}

void
Dispatcher::send_interrupt_cb(Component *, int ok_flag)
{
	if (!ok_flag)
		kill(getppid(), SIGINT);
}

// handle all the MSG_createp messages resulting from a create command
void
Dispatcher::make_new_processes()
{
	Message	**mptr = (Message **)vector.ptr();
	int	total = vector.size()/sizeof(Message *);

	for (int i = 0; i < total; i++, mptr++)
	{
		proclist.new_proc(*mptr, save_ws, (i == 0), io_flag);
		delete *mptr;
	}
	vector.clear();
	io_flag = 0;
}
