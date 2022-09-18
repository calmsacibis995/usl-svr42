#ident	"@(#)debugger:gui.d/common/Ctrl_dlg.C	1.5"

#include "Context.h"
#include "Dialogs.h"
#include "Boxes.h"
#include "Caption.h"
#include "Dialog_sh.h"
#include "Stext.h"
#include "Text_line.h"
#include "Toggle.h"
#include "Radio.h"
#include "Dispatcher.h"
#include "Windows.h"
#include "Proclist.h"
#include "str.h"

Run_dialog::Run_dialog(Window_set *ws) : PROCESS_DIALOG(ws)
{
	static const Button	buttons[] =
	{
		{ B_ok,	    "Run Until",  'R', (Callback_ptr)Run_dialog::apply},
		{ B_apply,   0, 0,  	 (Callback_ptr)Run_dialog::apply},
		{ B_cancel,  0, 0, (Callback_ptr)Run_dialog::cancel },
		{ B_help,    0, 0, 0 },
	};

	Caption		*caption;
	Packed_box	*box;

	save_string = 0;

	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
		"Run Until", (Callback_ptr)Process_dialog::dismiss_cb, this,
		 buttons, sizeof(buttons)/sizeof(Button), HELP_run_dialog);
	box = new Packed_box(dialog, "box", OR_vertical);
	dialog->add_component(box);

	process_caption = new Caption(box, "Processes:", CAP_LEFT);
	process_list = new Simple_text(process_caption, "", TRUE);
	process_caption->add_component(process_list);
	box->add_component(process_caption);

	caption = new Caption(box, "Location:", CAP_LEFT);
	location = new Text_line(caption, "location", "", 25, 1);
	caption->add_component(location);
	box->add_component(caption);
}

Run_dialog::~Run_dialog()
{
	delete save_string;
}

void
Run_dialog::apply(Component *, void *)
{
	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	char	*s = location->get_text();
	if (!s || !*s)
	{
		dialog->error(E_ERROR, GE_no_location);
		return;
	}

	delete save_string;
	save_string = makestr(s);

	dispatcher.send_msg(this, window_set->current_process()->get_id(),
		"run -b -p %s -u %s\n", make_plist(total, processes, 0, level),
		save_string);
	dialog->wait_for_response();
}

void
Run_dialog::set_location(const char *s)
{
	location->set_text(s);
	delete save_string;
	save_string = makestr(s);
}

void
Run_dialog::cancel(Component *, void *)
{
	location->set_text(save_string);
}

// values of "count" radio button in the Step dialog
#define	STEP_ONCE	0
#define	STEP_FOREVER	1
#define	STEP_N_TIMES	2

Step_dialog::Step_dialog(Window_set *ws) : PROCESS_DIALOG(ws)
{
	static const Button	buttons[] =
	{
		{ B_ok,	    "Step",  'S', (Callback_ptr)Step_dialog::apply },
		{ B_apply,   0, 0,  	  (Callback_ptr)Step_dialog::apply },
		{ B_cancel,  0, 0, (Callback_ptr)Step_dialog::reset },
		{ B_help,    0, 0, 0 },
	};

	static const char *type_buttons[] =
	{
		"Statement",
		"Instruction",
	};

	static const char *count_buttons[] =
	{
		"Single Step",
		"Step Forever",
		"Step Count Times",
	};

	static Toggle_data toggle_button_data[] =
	{
		"Over Call",	FALSE,	0
	};

	Caption		*caption;
	Packed_box	*box;
	Packed_box	*box2;
	Packed_box	*box3;

	save_count = 0;
	times_state = type_state = 0;
	step_over = FALSE;

	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
		"Step", (Callback_ptr)Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_step_dialog);
	box = new Packed_box(dialog, "step box", OR_vertical);
	dialog->add_component(box);

	process_caption = new Caption(box, "Processes:", CAP_LEFT);
	process_list = new Simple_text(process_caption, "", TRUE);
	process_caption->add_component(process_list);
	box->add_component(process_caption);

	box2 = new Packed_box(box, "box", OR_horizontal);
	box->add_component(box2);

	box3 = new Packed_box(box2, "box", OR_vertical);
	box2->add_component(box3);

	times = new Radio_list(box3, "count", OR_vertical, count_buttons,
		sizeof(count_buttons)/sizeof(char *), STEP_ONCE,
		(Callback_ptr)Step_dialog::set_times, this);
	box3->add_component(times);

	caption = new Caption(box3, "Count:", CAP_LEFT);
	count = new Text_line(caption, "count", "", 5, FALSE);
	caption->add_component(count);
	box3->add_component(caption);

	box3 = new Packed_box(box2, "box", OR_vertical);
	box2->add_component(box3);

	type = new Radio_list(box3, "type", OR_vertical, type_buttons,
		sizeof(type_buttons)/sizeof(char *), 0);
	box3->add_component(type);

	over = new Toggle_button(box3, "Over", toggle_button_data,
		sizeof(toggle_button_data)/sizeof(Toggle_data), OR_vertical);
	box3->add_component(over);
}

void
Step_dialog::apply(Component *, void *)
{
	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	int	tstate = times->which_button();
	char	*s = count->get_text();
	char	*cnt = "";

	if (tstate == STEP_N_TIMES)
	{
		if (!s || !*s)
		{
			dialog->error(E_ERROR, GE_count_required);
			return;
		}
		delete save_count;
		cnt = save_count = makestr(s);
	}
	else if (tstate == STEP_FOREVER)
		cnt = "0";

	type_state = type->which_button();
	step_over = over->is_set(0);
	times_state = tstate;

	dispatcher.send_msg(this, window_set->current_process()->get_id(),
		"step -b -p %s %s %s %s %s\n",
			make_plist(total, processes, 0, level),
			type_state ? "-i" : "",
			step_over ? "-o" : "",
			(tstate != STEP_ONCE)  ? "-c" : "",
			cnt);
	dialog->wait_for_response();
}

void
Step_dialog::reset(Component *, void *)
{
	type->set_button(type_state);
	times->set_button(times_state);
	count->set_text(save_count);
	over->set(0, step_over);
}

void
Step_dialog::set_times(Component *, void *)
{
	if (times->which_button() == 2)
	{
		count->set_editable(TRUE);
		dialog->set_focus(count);
	}
	else
	{
		count->set_editable(FALSE);
		dialog->set_focus(type);
	}
}

Jump_dialog::Jump_dialog(Window_set *ws) : PROCESS_DIALOG(ws)
{
	static const Button	buttons[] =
	{
		{ B_ok,	    "Jump",  'J', (Callback_ptr)Jump_dialog::apply},
		{ B_apply,   0, 0,  	  (Callback_ptr)Jump_dialog::apply},
		{ B_cancel,  0, 0, (Callback_ptr)Jump_dialog::cancel },
		{ B_help,    0, 0, 0 },
	};

	Caption	*caption;
	Packed_box	*box;

	save_string = 0;

	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
		"Jump", (Callback_ptr)Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_jump_dialog);
	box = new Packed_box(dialog, "jump box", OR_vertical);
	dialog->add_component(box);

	process_caption = new Caption(box, "Processes:", CAP_LEFT);
	process_list = new Simple_text(process_caption, "process list", TRUE);
	process_caption->add_component(process_list);
	box->add_component(process_caption);

	caption = new Caption(box, "Location:", CAP_LEFT);
	location = new Text_line(caption, "location", "", 25, 1);
	caption->add_component(location);
	box->add_component(caption);
}

Jump_dialog::~Jump_dialog()
{
	delete save_string;
}

void
Jump_dialog::apply(Component *, void *)
{
	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	char	*s = location->get_text();
	if (!s || !*s)
	{
		dialog->error(E_ERROR, GE_no_location);
		return;
	}

	delete save_string;
	save_string = makestr(s);

	dispatcher.send_msg(this, window_set->current_process()->get_id(),
		"jump -p %s %s\n", make_plist(total, processes, 0, level),
		save_string);
	dialog->wait_for_response();
}

void
Jump_dialog::set_location(const char *s)
{
	location->set_text(s);
	delete save_string;
	save_string = makestr(s);
}

void
Jump_dialog::cancel(Component *, void *)
{
	location->set_text(save_string);
}
