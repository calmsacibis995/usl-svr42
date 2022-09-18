#ident	"@(#)debugger:libint/common/GManager.C	1.2"

#include	<stdio.h>
#include	<stdarg.h>

#include	"Severity.h"
#include	"Msgtypes.h"
#include	"Msgtab.h"
#include	"GManager.h"
#include	"Message.h"
#include	"Transport.h"
#include	"Parser.h"
#include	"global.h"
#include	"UIutil.h"
#include	"Proglist.h"

#include	"libint.h"
#include	"edit.h"

GUI_Manager::GUI_Manager()
{
	dbcontext = 0;
	uicontext = 0;
	responding_to_query = 0;
	sync_needed = 0;
	cmd = 0;
	max_len = 0;

	// create sync and cmd complete messages - these will be
	// used a lot
	sync_m.bundle(MSG_sync_request, E_NONE, 0);
	cmd_complete.bundle(MSG_cmd_complete, E_NONE, 0);
}

// not in any special state, will accept any command
int
GUI_Manager::docommand()
{
	transport->get_next_message(&cur_in_msg, debug_read);
	return do_single_cmd();
}

// do_single_cmd called by both docommand and sync_request, to handle queries
int
GUI_Manager::do_single_cmd()
{
	char	*tmp;

	// save the context, subsequent output messages have the same context
	uicontext = cur_in_msg.get_uicontext();
	dbcontext = cur_in_msg.get_dbcontext();
	if (dbcontext)
		proglist.set_current((LWP *)dbcontext, 0);
	if (cur_in_msg.get_transport_type() == TT_UI_query)
		responding_to_query = 1;

	if (cur_in_msg.get_msg_length() > max_len)
	{
		delete cmd;
		max_len = (cur_in_msg.get_msg_length() > BUFSIZ)
			? cur_in_msg.get_msg_length() : BUFSIZ;
		cmd = new char[max_len];
	}

#ifndef NOCHECKS
	if (cur_in_msg.get_msg_id() != MSG_command)
		interface_error("GUI_Manager::do_single_cmd", __LINE__, 1);
#endif // NOCHECKS

	cur_in_msg.unbundle(tmp);
	strcpy(cmd, tmp);
	if (log_file && strncmp(cmd, "logoff", 6) != 0)
		fputs(cmd, log_file);

	parse_and_execute(cmd);

	// execution of the command may have produced many messages
	// cmd_complete lets ui go on to next command
	transport->send_message(&cmd_complete, 
		responding_to_query ? TT_DE_response : TT_DE_notify,
		dbcontext, uicontext);

	responding_to_query = 0;
	uicontext = 0;
	return 1;
}

// send a single message to the ui
int
GUI_Manager::send_msg(Msg_id mtype, Severity sev ...)
{
	va_list	ap;

	va_start(ap, sev);
	cur_out_msg.bundle(mtype, sev, ap);
	va_end(ap);

	// ui context is either value saved by do_single_command,
	// or 0 if message comes from an event
	dbcontext = temp_context ? temp_context
		: (DBcontext)proglist.current_lwp();
	transport->send_message(&cur_out_msg,
		responding_to_query? TT_DE_response : TT_DE_notify,
		dbcontext, uicontext);

	if (Mtable.format(mtype) != 0) // i.e. the message produces output
		sync_needed = 1;

	// return space needed for message - needed for logging
	return cur_out_msg.get_msg_length();
}

// the debugger may have produced a lot of output, so wait for the ui to sync up
void
GUI_Manager::sync_request()
{
	DBcontext	save_db_ctxt;
	UIcontext	save_ui_ctxt;

	// may have already done a sync_request and gotten a query while
	// waiting for the response.  This prevents us from getting stuck
	if (!sync_needed || responding_to_query)
		return;

	transport->send_message(&sync_m, TT_DE_notify, dbcontext, uicontext);
	sync_needed = 0;

	for (;;)
	{
		// deal with queries even while waiting for sync response
		transport->get_nonuser_message(&cur_in_msg);
		if (cur_in_msg.get_msg_id() == MSG_sync_response)
			break;

#ifndef NOCHECKS
		if (cur_in_msg.get_transport_type() != TT_UI_query)
			interface_error("GUI_Manager::sync_request", __LINE__, 1);
#endif // NOCHECKS

		// do_single_cmd resets the current context
		save_db_ctxt = dbcontext;
		save_ui_ctxt = uicontext;	
		(void) do_single_cmd();
		dbcontext = save_db_ctxt;
		uicontext = save_ui_ctxt;
	}
}

// temporarily reset the context to pass messages about the creation
// of a new process or LWP
void
GUI_Manager::reset_context(LWP *new_lwp)
{
	temp_context = (DBcontext)new_lwp;
}
