#ident	"@(#)debugger:gui.d/common/Sch_dlg.C	1.4"

#include "Sch_dlg.h"
#include "Dialog_sh.h"
#include "Text_disp.h"
#include "Text_line.h"
#include "Caption.h"
#include "Dis.h"
#include "Source.h"
#include "Context.h"	// needed by constructor's get_window_shell() call
#include "Windows.h"
#include "UI.h"

#include "str.h"

Search_dialog::Search_dialog(Window_set *ws) : DIALOG_BOX(ws)
{
	static const Button	buttons[] =
	{
		{ B_apply,	"Forward",	'F',	(Callback_ptr)(&Search_dialog::apply) },
		{ B_exec,	"Backward",	'B',	(Callback_ptr)(&Search_dialog::apply) },
		{ B_cancel,	0, 0, (Callback_ptr)(&Search_dialog::cancel) },
		{ B_help,	0, 0, 0 },
	};

	Caption	*caption;

	save_string = 0;
	base_window = 0;

	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
			"Search", 0, this, 
			buttons, sizeof(buttons)/sizeof(Button),
			 HELP_search_dialog);

	caption = new Caption(dialog, "Text:", CAP_LEFT);
	string = new Text_line(caption, "text", "", 15, TRUE);
	caption->add_component(string);
	dialog->add_component(caption);
}

void
Search_dialog::apply(Component *, int mnemonic)
{
	char	*s = string->get_text();

	if (!s || !*s)
	{
		dialog->error(E_ERROR, GE_no_reg_expr);
		return;
	}

	delete save_string;
	save_string = makestr(s);

	Search_return	sr = SR_notfound;

	// warn the user if the base window is not on the screen 
	if (base_window->get_type() == BW_source)
	{
		Source_window	*sw = (Source_window *)base_window;

		if (sw->is_open())
			sr = sw->get_pane()->search(s, mnemonic == 'F');
		else
		{
			dialog->error(E_ERROR, GE_no_source_win);
			return;
		}

	}
	else if (base_window->get_type() == BW_disasm)
	{
		Disasm_window	*dw = (Disasm_window *)base_window;

		if (dw->is_open())
			sr = dw->get_pane()->search(s, mnemonic == 'F');
		else
		{
			dialog->error(E_ERROR, GE_no_disasm_win);
			return;
		}
	}
	else
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;	
	}

	switch(sr)
	{
	case SR_found:
		break;

	case SR_notfound:
		dialog->error(E_ERROR, GE_expr_not_found);
		break;

	case SR_bad_expression:
		dialog->error(E_ERROR, GE_bad_expr);
		break;

	default:
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		break;
	}
}

void
Search_dialog::cancel(Component *, void *)
{
	string->set_text(save_string);
}

void
Search_dialog::set_string(const char *s)
{
	string->set_text(s);
}

void
Search_dialog::set_base_window(Base_window *win)
{
	if (win == base_window)
		return;

	base_window = win;
	string->set_text(0);
	delete save_string;
	save_string = 0;

	if (win->get_type() == BW_source)
		dialog->set_label("Forward", 'F', "Search in Source");
	else
		dialog->set_label("Forward", 'F', "Search in Disassembly");
}
