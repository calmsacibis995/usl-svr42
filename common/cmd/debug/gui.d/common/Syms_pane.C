#ident	"@(#)debugger:gui.d/common/Syms_pane.C	1.12"

#include "Context.h"
#include "Dialogs.h"
#include "Dialog_sh.h"
#include "Dispatcher.h"
#include "Windows.h"
#include "UI.h"
#include "Table.h"
#include "Toggle.h"
#include "Boxes.h"
#include "Proclist.h"
#include "Syms_pane.h"

#include "Message.h"
#include "Msgtab.h"
#include "UIutil.h"
#include "str.h"

#include <stdlib.h>
#include <stdio.h>

static Column syms_spec[] =
{
	{ "Name",	18,	Col_text },
	{ "Location",	15,	Col_text },
	{ "Type",	17,	Col_wrap_text },
	{ "Value",	24,	Col_wrap_text },
};

Symbols_pane::Symbols_pane(Window_set *ws, Divided_box *box) : COMMAND_SENDER(ws)
{
	symbol_types = SYM_local;
	next_row = 0;
	total_selections = 0;
	reading_symbols = 0;
	pane = new Table(box, "symbols", SM_multiple, syms_spec,
		sizeof(syms_spec)/sizeof(Column), 5, FALSE,
		(Callback_ptr)Symbols_pane::select_symbol,
		(Callback_ptr)Symbols_pane::deselect_symbol, 0, this,
		HELP_syms_pane);
	box->add_component(pane);
}

Symbols_pane::~Symbols_pane()
{
	delete pane;
}

void
Symbols_pane::popup()
{
	update_cb(0, RC_set_current, 0, window_set->current_process());
	window_set->change_current.add(this,
		(Notify_func)Symbols_pane::update_cb, 0);
}

void
Symbols_pane::popdown()
{
	window_set->change_current.remove(this,
		(Notify_func)Symbols_pane::update_cb, 0);
}

void
Symbols_pane::set_sym_type(int s)
{
	symbol_types = s;
	update_cb(0, RC_change_state, 0, window_set->current_process());
}

void
Symbols_pane::select_symbol(Table *, void *)
{
	total_selections++;
	window_set->get_context_window()->set_selection(SEL_symbol);
}

void
Symbols_pane::deselect_symbol(Table *, int)
{
	if (total_selections)
	{
		total_selections--;
		window_set->get_context_window()->set_selection(total_selections
			? SEL_symbol : SEL_none);
	}
}

void
Symbols_pane::deselect()
{
	total_selections = 0;
	pane->deselect_all();
}

void
Symbols_pane::update_cb(void *, Reason_code rc, void *, Process *call_data)
{
	char		buf[20];

	if (!symbol_types)
	{
		next_row = 0;
		pane->clear();
		return;
	}

	if (!call_data)
	{
		if (!(symbol_types&(SYM_debugger|SYM_user)))
		{
			next_row = 0;
			pane->clear();
			return;
		}

		window_set->get_context_window()->inc_busy();
		strcpy(buf, "-tv");
		if (symbol_types&SYM_debugger)
			strcat(buf, " -d");
		if (symbol_types&SYM_user)
			strcat(buf, " -u");
		pane->set_sensitive(FALSE);
		dispatcher.send_msg(this, 0, "symbols %s\n", buf);
		return;
	}

	if (rc == RC_set_current)
	{
		next_row = 0;
		pane->clear();
	}

	if ((call_data->get_state() != State_stopped
		&& call_data->get_state() != State_core)
		|| call_data->is_incomplete())
	{
		pane->set_sensitive(FALSE);
		return;
	}

	strcpy(buf, "-tv");

	if (symbol_types&SYM_local)
		strcat(buf, " -l");
	if (symbol_types&SYM_global)
		strcat(buf, " -g");
	if (symbol_types&SYM_file)
		strcat(buf, " -f");
	if (symbol_types&SYM_debugger)
		strcat(buf, " -d");
	if (symbol_types&SYM_user)
		strcat(buf, " -u");

	window_set->get_context_window()->inc_busy();
	dispatcher.send_msg(this, call_data->get_id(), "symbols %s\n", buf);
}

void
Symbols_pane::cmd_complete()
{
	if (!reading_symbols)
		return;

	int	max_rows = pane->get_rows();

	if (!next_row)
		pane->clear();
	else if (next_row < max_rows)
		pane->delete_rows(next_row, max_rows - next_row);
	else if (pane->is_delayed())
		pane->finish_updates();
	window_set->get_context_window()->dec_busy();
	reading_symbols = 0;
}

void
Symbols_pane::de_message(Message *m)
{
	char	loc[BUFSIZ];
	char	*type = 0;
	char	*name = 0;
	char	*file = 0;
	char	*line = 0;
	char	*value = 0;
	char	*tmp = 0;

	switch(m->get_msg_id())
	{
	case MSG_sym_header:
	case MSG_sym_type_header:
	case MSG_sym_val_header:
	case MSG_sym_type_val_header:
		next_row = 0;
		reading_symbols = 1;
		if (total_selections)
		{
			total_selections = 0;
			window_set->get_context_window()->set_selection(SEL_none);
		}
		break;

	case MSG_symbol_type_val:
	case MSG_symbol_type_val_assume:
		m->unbundle(name, file, line, type, value);
		if (line && *line)
			sprintf(loc, "%s@%s", file, line);
		else
			strcpy(loc, file);
		if (next_row >= pane->get_rows())
		{
			if (!pane->is_delayed())
				pane->delay_updates();
			pane->insert_row(next_row, name, loc, type, value);
		}
		else
			pane->set_row(next_row, name, loc, type, value);
		next_row++;
		break;

	case ERR_short_read:
		break;

	default:
		if (Mtable.msg_class(m->get_msg_id()) == MSGCL_error)
			display_msg(m);
		else
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		break;
	}
}

char **
Symbols_pane::get_selections()
{
	int	*sel;

	if (total_selections < 0 || pane->get_selections(sel) != total_selections)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return 0;
	}

	vscratch1.clear();
	for (int i = 0; i < total_selections; i++)
	{
		const char *s = pane->get_cell(sel[i],0).string;
		vscratch1.add(&s, sizeof(char *));
	}
	return (char **)vscratch1.ptr();
}

void
Symbols_pane::export_symbols()
{
	int	*sel;

	if (total_selections < 0 || pane->get_selections(sel) != total_selections)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	for (int i = 0; i < total_selections; i++)
		dispatcher.send_msg(this, window_set->current_process()->get_id(),
			"export %s\n", pane->get_cell(sel[i],0).string);
}

Symbol_class_dialog::Symbol_class_dialog(Symbols_pane *p)
	: DIALOG_BOX(p->get_window_set())
{
	static const Button	buttons[] =
	{
		{ B_ok,	     0, 0, (Callback_ptr)Symbol_class_dialog::apply },
		{ B_apply,   0, 0, (Callback_ptr)Symbol_class_dialog::apply },
		{ B_reset,   0, 0, (Callback_ptr)Symbol_class_dialog::reset },
		{ B_cancel,  0, 0, (Callback_ptr)Symbol_class_dialog::reset },
		{ B_help,    0, 0, 0 },
	};

	static const Toggle_data toggle1[] =
	{
		{ "Global",	FALSE,	0 },
		{ "File",	FALSE,	0 },
		{ "Local",	TRUE,	0 },
	};

	static const Toggle_data toggle2[] =
	{
		{ "Debugger",	FALSE,	0 },
		{ "User",	FALSE,	0 },
	};

	Expansion_box	*box;

	parent = p;
	dialog = new Dialog_shell(window_set->get_context_window()->get_window_shell(),
		"Symbols", 0, this, buttons,
		sizeof(buttons)/sizeof(Button), HELP_symbols_dialog);
	box = new Expansion_box(dialog, "symbols box", OR_horizontal);

	program_settings = new Toggle_button(box, "symbols", toggle1,
		sizeof(toggle1)/sizeof(Toggle_data), OR_vertical, this);
	box->add_elastic(program_settings);

	debug_settings = new Toggle_button(box, "symbols", toggle2,
		sizeof(toggle2)/sizeof(Toggle_data), OR_vertical, this);
	box->add_elastic(debug_settings);
	dialog->add_component(box);
}

void
Symbol_class_dialog::apply()
{
	int	state;

	state = 0;
	if (program_settings->is_set(0))
		state |= SYM_global;
	if (program_settings->is_set(1))
		state |= SYM_file;
	if (program_settings->is_set(2))
		state |= SYM_local;
	if (debug_settings->is_set(0))
		state |= SYM_debugger;
	if (debug_settings->is_set(1))
		state |= SYM_user;
	parent->set_sym_type(state);
}

void
Symbol_class_dialog::reset()
{
	int	state = parent->get_sym_type();

	program_settings->set(0, (state&SYM_global) != 0);
	program_settings->set(1, (state&SYM_file) != 0);
	program_settings->set(2, (state&SYM_local) != 0);
	debug_settings->set(0, (state&SYM_debugger) != 0);
	debug_settings->set(1, (state&SYM_user) != 0);
}
