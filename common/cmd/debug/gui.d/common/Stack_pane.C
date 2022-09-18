#ident	"@(#)debugger:gui.d/common/Stack_pane.C	1.6"

#include "Context.h"
#include "Dialogs.h"
#include "Dialog_sh.h"
#include "Dispatcher.h"
#include "Windows.h"
#include "UI.h"
#include "Table.h"
#include "Boxes.h"
#include "Proclist.h"
#include "Stack_pane.h"

#include "Message.h"
#include "Msgtab.h"
#include "UIutil.h"
#include "str.h"
#include "Buffer.h"

#include <stdlib.h>
#include <stdio.h>

static Column stack_spec[] =
{
	{ 0,		1,	Col_glyph },
	{ "Frame",	5,	Col_numeric },
	{ "Function",	23,	Col_text },
	{ "Parameters",	29,	Col_wrap_text },
	{ "Location",	14,	Col_text },
};

Stack_pane::Stack_pane(Window_set *ws, Divided_box *box) : COMMAND_SENDER(ws)
{
	top_frame = cur_frame = -1;
	next_row = 0;
	has_selection = 0;
	
	pane = new Table(box, "stack", SM_single, stack_spec,
		sizeof(stack_spec)/sizeof(Column), 5, FALSE,
		(Callback_ptr)Stack_pane::select_frame,
		(Callback_ptr)Stack_pane::deselect_frame, 0, this,
		HELP_stack_pane);
	box->add_component(pane);
}

Stack_pane::~Stack_pane()
{
	window_set->change_current.remove(this,
		(Notify_func)Stack_pane::update_cb, 0);
}

void
Stack_pane::popup()
{
	update_cb(0, RC_set_current, 0, window_set->current_process());
	window_set->change_current.add(this, (Notify_func)Stack_pane::update_cb, 0);
}

void
Stack_pane::popdown()
{
	window_set->change_current.remove(this,
		(Notify_func)Stack_pane::update_cb, 0);
}

void
Stack_pane::set_current()
{
	Process		*proc = window_set->current_process();
	int		*sel;
	int		row;

	if (pane->get_selections(sel) != 1)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	row = sel[0];
	dispatcher.send_msg(0, proc->get_id(), "set %%frame = %d\n", top_frame - row);
}

void
Stack_pane::update_cb(void *, Reason_code rc, void *, Process *call_data)
{
	if (!call_data)
	{
		pane->clear();	// process died or was released
		cur_frame = -1;
		return;
	}

	if (call_data->is_incomplete())
	{
		pane->set_sensitive(FALSE);
		cur_frame = -1;
		return;
	}

	if (rc == RC_set_frame)
	{
		if (cur_frame > -1)
			pane->set_cell(top_frame - cur_frame, ST_current, Gly_blank);
		pane->set_cell(top_frame - call_data->get_frame(), ST_current, Gly_hand);
		cur_frame = call_data->get_frame();
		return;
	}

	if (call_data->get_state() == State_stopped
		|| call_data->get_state() == State_core)
	{
		window_set->get_context_window()->inc_busy();
		dispatcher.send_msg(this, call_data->get_id(), "stack\n");
	}
	else if (rc == RC_set_current)
	{
		pane->clear();
		cur_frame = -1;
	}
	else
		pane->set_sensitive(FALSE);
}

void
Stack_pane::cmd_complete()
{
	if (pane->is_delayed())
		pane->finish_updates();
	window_set->get_context_window()->dec_busy();
	cur_frame = window_set->current_process()->get_frame();
}

static Buffer buffer;

void
Stack_pane::de_message(Message *m)
{
	char		loc[BUFSIZ];
	char		*cur_flag = 0;
	char		*file = 0;
	Word		wtmp;
	char		buf[20];


	switch(m->get_msg_id())
	{
	case MSG_stack_header:
		top_frame = -1;
		next_row = 0;
		break;

	case MSG_stack_frame:
		m->unbundle(wtmp, func);
		cur_frame = (int)wtmp;
		func = makestr(func);

		if (top_frame == -1)
		{
			int max_row = pane->get_rows();
			top_frame = cur_frame;
			if (max_row > top_frame+1)
			{
				pane->delay_updates();
				pane->delete_rows(top_frame, max_row - (top_frame + 1));
			}
			else if (max_row <= top_frame)
			{
				pane->delay_updates();
				for (int i = max_row; i <= top_frame; i++)
					pane->insert_row(i, Gly_blank, 0, 0, 0, 0);
			}
			if (window_set->current_process()->get_frame() == -1)
				window_set->current_process()->set_frame(top_frame);
		}
		buffer.clear();
		buffer.add('(');
		break;

	case MSG_stack_frame_end_1:
		m->unbundle(file, wtmp);
		buffer.add(')');
		sprintf(loc, "%s@%d", file, wtmp);
set_row:
		sprintf(buf, "%d", cur_frame);
		pane->set_row(next_row,
			window_set->current_process()->get_frame()==cur_frame ? 
				Gly_hand : Gly_blank, 
			buf, func, (char *)buffer, loc);
		next_row++;
		break;

	case MSG_stack_frame_end_2:
		m->unbundle(wtmp);
		buffer.add(')');
		sprintf(loc, "%#x", wtmp);
		goto set_row;

	case MSG_stack_arg:
	case MSG_stack_arg2:
	case MSG_stack_arg3:
		buffer.add(m->format());
		break;

	default:
		if (Mtable.msg_class(m->get_msg_id()) == MSGCL_error)
			display_msg(m);
		else
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		break;
	}
}

void
Stack_pane::select_frame(Table *, void *)
{
	has_selection = 1;
	window_set->get_context_window()->set_selection(SEL_frame);
}

void
Stack_pane::deselect_frame(Table *, int)
{
	if (has_selection)
	{
		has_selection = 0;
		window_set->get_context_window()->set_selection(SEL_none);
	}
}

void
Stack_pane::deselect()
{
	if (has_selection)
	{
		has_selection = 0;
		pane->deselect_all();
	}
}

int
Stack_pane::get_frame(int frame, const char *&func, const char *&loc)
{
	if (frame < 0 || frame > top_frame)
		return 0;

	func = pane->get_cell(top_frame - frame, ST_func).string;
	loc = pane->get_cell(top_frame - frame, ST_loc).string;
	return 1;
}
