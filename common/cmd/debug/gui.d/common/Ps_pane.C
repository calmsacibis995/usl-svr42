#ident	"@(#)debugger:gui.d/common/Ps_pane.C	1.7"

#include "Component.h"
#include "Context.h"
#include "Ps_pane.h"
#include "Dialogs.h"
#include "Dialog_sh.h"
#include "UI.h"
#include "Table.h"
#include "Caption.h"
#include "Boxes.h"
#include "Text_line.h"
#include "Stext.h"
#include "Proclist.h"
#include "Windows.h"

#include "Message.h"
#include "Msgtab.h"
#include "UIutil.h"
#include "Vector.h"
#include "str.h"

#include <stdlib.h>
#include <stdio.h>

enum Cell { CURRENT, PROGRAM, ID, STATE, FUNCTION, LOCATION, COMMAND };

static Column ps_spec[] =
{
	{ 0,		1,	Col_glyph },
	{ "Program",	8,	Col_text },
	{ "ID",		6,	Col_text },
	{ "State",	8,	Col_text },
	{ "Function",	18,	Col_text },
	{ "Location",	15,	Col_text },
	{ "Command",	15,	Col_wrap_text },
};

Ps_pane::Ps_pane(Window_set *ws, Divided_box *box)
{
	window_set = ws;
	total_selections = 0;

	pane = new Table(box, "programs", SM_multiple, ps_spec,
		sizeof(ps_spec)/sizeof(Column), 3, FALSE,
		(Callback_ptr)Ps_pane::select_cb,
		(Callback_ptr)Ps_pane::deselect_cb,
		(Callback_ptr)Ps_pane::drop_proc, this,
		HELP_ps_pane);
	box->add_component(pane);
}

Ps_pane::~Ps_pane()
{
}

void
Ps_pane::add_process(int slot, Process *ptr)
{
	total_selections = 0;
	window_set->get_context_window()->set_selection(SEL_none);
	pane->insert_row(slot,
		(ptr == window_set->current_process()) ? Gly_hand : Gly_blank,
		ptr->get_program()->get_name(),
		ptr->get_name(),
		ptr->get_state_string(),
		ptr->get_function(),
		ptr->get_location(),
		ptr->get_program()->get_cmd_line());
}

void
Ps_pane::delete_process(int slot)
{
	deselect();
	total_selections = 0;
	window_set->get_context_window()->set_selection(SEL_none);
	pane->delete_rows(slot, 1);
}

void
Ps_pane::update_process(Reason_code rc, int slot, Process *ptr)
{
	if (!ptr)
		return;

	if (rc == RC_rename)
		pane->set_cell(slot, PROGRAM, ptr->get_program()->get_name());
	else
	{
		pane->set_row(slot, (ptr == window_set->current_process())
			? Gly_hand : Gly_blank,
			ptr->get_program()->get_name(),
			ptr->get_name(),
			ptr->get_state_string(),
			ptr->get_function(),
			ptr->get_location(),
			ptr->get_program()->get_cmd_line());
		window_set->get_context_window()->set_sensitivity();
	}
}

void
Ps_pane::set_current(Process *proc)
{
	Plink	*plink = window_set->get_process_list();
	int	i = 0;

	for (; plink; plink = plink->next())
	{
		if (plink->process() == proc)
			// set the new one
			pane->set_cell(i, 0, Gly_hand);
		else if (pane->get_cell(i, 0).glyph == Gly_hand)
			// unset the old one
			pane->set_cell(i, 0, Gly_blank);
		i++;
	}
	window_set->get_context_window()->set_sensitivity();
}

void
Ps_pane::select_cb(Table *, void *)
{
	total_selections++;
	window_set->get_context_window()->set_selection(SEL_process);
}

void
Ps_pane::deselect_cb(Table *, void *)
{
	if (total_selections)
	{
		total_selections--;
		window_set->get_context_window()->set_selection(total_selections
			? SEL_process : SEL_none);
	}
}

void
Ps_pane::drop_proc(Table *, const Table_calldata *data)
{
	if (!data || !data->dropped_on || data->dropped_on->get_type() != WINDOW_SHELL)
		return;

	if (!data->dropped_on->get_creator())
		return;
	Window_set *ws = ((Command_sender *)data->dropped_on->get_creator())->get_window_set();
	Process **p = (Process **)get_selections();
	for (int i = 0; i < total_selections; i++)
	{
		proclist.move_process(p[i], ws);
	}
}

void
Ps_pane::deselect()
{
	total_selections = 0;
	pane->deselect_all();
}

static Vector vec;

Process **
Ps_pane::get_selections()
{
	Plink	*plink = window_set->get_process_list();
	int	*sel;
	int	pindex = 0;

	if (total_selections <= 0 || pane->get_selections(sel) != total_selections)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return 0;
	}

	vec.clear();
	for (int i = 0; i < total_selections; i++)
	{
		while (pindex < sel[i] && plink)
		{
			pindex++;
			plink = plink->next();
		}
		if (!plink)
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return 0;
		}
		Process *p = plink->process();
		vec.add(&p, sizeof(Process *));
	}
	return (Process **)vec.ptr();
}
