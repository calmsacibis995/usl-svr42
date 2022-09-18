#ident	"@(#)debugger:gui.d/common/Dialogs.C	1.12"

#include "gui_msg.h"
#include "Context.h"
#include "Command.h"
#include "Dialogs.h"
#include "Ps_pane.h"
#include "Boxes.h"
#include "Caption.h"
#include "Dialog_sh.h"
#include "Sel_list.h"
#include "Stext.h"
#include "Toggle.h"
#include "Text_line.h"
#include "Stext.h"
#include "Radio.h"
#include "Dispatcher.h"
#include "Events.h"
#include "Source.h"
#include "Windows.h"
#include "Proclist.h"
#include "UI.h"
#include "Language.h"
#include "Message.h"
#include "Msgtab.h"
#include "FileInfo.h"
#include "str.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char	*create_args;
const char	*working_directory;
Language	cur_language;


Dialog_box::Dialog_box(Window_set *ws) : COMMAND_SENDER(ws)
{
	dialog = 0;
}

Dialog_box::~Dialog_box()
{
	delete dialog;
}

void
Dialog_box::display()
{
	dialog->popup();
}

void
Dialog_box::de_message(Message *m)
{
	Msg_id	mtype = m->get_msg_id();

	if (Mtable.msg_class(mtype) == MSGCL_error)
	{
		if (mtype == ERR_cmd_pointer || mtype == ERR_asis
			|| mtype == ERR_syntax)
			return;

		if (mtype == ERR_syntax_loc)
			dialog->error(E_ERROR, GE_syntax_error);
		else if (m->get_severity() == E_ERROR)
			dialog->error(m);
		else
			display_msg(m);
	}
}

void
Dialog_box::cmd_complete()
{
	dialog->cmd_complete();
}

Process_dialog::Process_dialog(Window_set *ws) : DIALOG_BOX(ws)
{
	total = 0;
	processes = 0;
	process_list = 0;
	process_caption = 0;
	track_current = 0;
}

Process_dialog::~Process_dialog()
{
	delete processes;
}

void
Process_dialog::set_plist(Base_window *win, int l)
{
	Process	**proc_list;

	level = l;
	if (win->get_type() == BW_context && win->selection_type() == SEL_process)
	{
		Ps_pane *ps = ((Context_window *)win)->get_ps_pane();
		total = ps->get_total();
		proc_list = ps->get_selections();
		track_current = 0;
		window_set->change_any.add(this,
			(Notify_func)(&Process_dialog::update_list_cb), 0);
	}
	else if (window_set->current_process())
	{
		total = 1;
		Process	*p = window_set->current_process();
		proc_list = &p;
		track_current = 1;
		window_set->change_current.add(this,
			(Notify_func)(&Process_dialog::update_current_cb), 0);
	}
	else
	{
		total = 0;
		track_current = 0;
		processes = 0;
		process_list->set_text("");
		set_process(FALSE);
		return;
	}

	delete processes;
	processes = new Process *[total];
	memcpy(processes, proc_list, total * sizeof(Process *));
	if (!process_list || !process_caption)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	if (level == PROGRAM_LEVEL)
	{
		process_list->set_text(make_plist(total, proc_list, 1, 1));
		if (total > 1)
			process_caption->set_label("Programs:");
		else
			process_caption->set_label("Program:");
	}
	else
	{
		process_list->set_text(make_plist(total, proc_list, 1, 0));
		if (total > 1)
			process_caption->set_label("Processes:");
		else
			process_caption->set_label("Process:");
	}
	set_process(FALSE);
}

void
Process_dialog::dismiss_cb(Component *, void *)
{
	if (track_current)
		window_set->change_current.remove(this,
			(Notify_func)(&Process_dialog::update_current_cb), 0);
	else
		window_set->change_any.remove(this,
			(Notify_func)(&Process_dialog::update_list_cb), 0);
	delete processes;
	processes = 0;
}

void
Process_dialog::update_list_cb(void *, int rc, void *, Process *process)
{
	if (rc != RC_delete)
		return;

	int list_changed = 0;

	for (int i = 0; i < total; i++)
	{
		if (processes[i] == process)
		{
			list_changed = 1;
			--total;
			if (total)
			{
				for (int j = i; j < total; j++)
					processes[j] = processes[j+1];
			}
			else
			{
				delete processes;
				processes = 0;
			}
			break;
		}
	}

	if (list_changed)
	{
		if (level == PROGRAM_LEVEL)
		{
			process_list->set_text(make_plist(total, processes, 1, 1));
			process_caption->set_label("Programs:");
		}
		else
		{
			process_list->set_text(make_plist(total, processes, 1, 0));
			process_caption->set_label("Processes:");
		}
	}
}

void
Process_dialog::update_current_cb(void *, int rc, void *, Process *process)
{
	if (rc == RC_delete || (rc == RC_set_current && !process))
	{
		delete processes;
		processes = 0;
		total = 0;
		process_list->set_text("");
		set_process(TRUE);
	}
	else if (rc == RC_set_current)
	{
		if (!processes)
			processes = new Process *[1];
		processes[0] = process;
		total = 1;
		if (level == PROGRAM_LEVEL)
			process_list->set_text(process->get_program()->get_name());
		else
			process_list->set_text(process->get_name());
		set_process(TRUE);
	}
}

void
Process_dialog::set_process(Boolean)
{
}

static const Toggle_data create_toggles[] =
{
	{ "Capture I/O",	TRUE,	0 },
	{ "Follow Children",	TRUE,	0 },
	{ "New Window Set",	FALSE,	0 },
};

Create_dialog::Create_dialog(Window_set *ws) : DIALOG_BOX(ws)
{
	static const Button	buttons[] =
	{
		{ B_ok,	    "Create",	'A', (Callback_ptr)(&Create_dialog::do_create)},
		{ B_cancel,  0,	0, (Callback_ptr)Create_dialog::cancel },
		{ B_help,    0,	0, 0 },
	};

	Caption		*caption;
	Packed_box	*box;

	follow_state = TRUE;
	io_state = TRUE;
	new_window_state = FALSE;
	save_cmd = 0;
	save_ws = window_set;

	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
		"Create", 0, this, buttons, sizeof(buttons)/sizeof(Button),
		HELP_create_dialog, (Callback_ptr)&Create_dialog::drop_cb,
		Drop_cb_stayup);
	box = new Packed_box(dialog, "create box", OR_vertical);

	caption = new Caption(box, "Command Line:", CAP_LEFT);
	cmd_line = new Text_line(caption, "command line", "", 25, 1);
	caption->add_component(cmd_line);
	box->add_component(caption);

	toggles = new Toggle_button(box, "toggles", create_toggles,
		sizeof(create_toggles)/sizeof(Toggle_data), OR_vertical);
	box->add_component(toggles);
	dialog->add_component(box);
}

void
Create_dialog::drop_cb(Component *, void *)
{
	char *s;

	if(s = dialog->get_drop_item())
	{
		cmd_line->set_text(s);
		// in anticipation of cmd args
		cmd_line->set_cursor(strlen(s));
	}
}

void
Create_dialog::do_create(Component *, void *)
{
	char		*s = cmd_line->get_text();
	const char	*f;
	const char	*r;

	if (!s || !*s)
	{
		dialog->error(E_ERROR, GE_no_cmd_line);
		return;
	}
	delete save_cmd;
	save_cmd = makestr(s);

	if (toggles->is_set(0))
	{
		io_state = TRUE;
		r = "-r";
	}
	else
	{
		io_state = FALSE;
		r = "-d";
	}

	if (toggles->is_set(1))
	{
		follow_state = TRUE;
		f = "all";
	}
	else
	{
		follow_state = FALSE;
		f = "none";
	}

	dispatcher.send_msg(this, 0, "create %s -f %s %s\n", r, f, s);
	dialog->wait_for_response();

	if (toggles->is_set(2))
	{
		new_window_state = TRUE;
		window_set = new Window_set();
	}
	else
	{
		new_window_state = FALSE;
		window_set = save_ws;
	}
}

void
Create_dialog::cancel(Component *, void *)
{
	cmd_line->set_text(save_cmd);
	toggles->set(0, follow_state);
	toggles->set(1, io_state);
	toggles->set(2, new_window_state);
}

void
set_create_args(Message *m)
{
	Window_set	*ws;
	char		*s;

	m->unbundle(s);
	delete (char *)create_args;
	create_args = makestr(s);

	for (ws = (Window_set *)windows.first(); ws; ws = (Window_set *)windows.next())
		ws->update_recreate_dialog();
}

Recreate_dialog::Recreate_dialog(Window_set *ws) : DIALOG_BOX(ws)
{
	static const Button	buttons[] =
	{
		{ B_ok,	    "Recreate",	'R', (Callback_ptr)Recreate_dialog::do_recreate},
		{ B_cancel,  0,	0, (Callback_ptr)Recreate_dialog::cancel },
		{ B_help,    0,	0, 0 },
	};

	Caption		*caption;
	Packed_box	*box;

	follow_state = TRUE;
	io_state = TRUE;
	new_window_state = FALSE;
	save_ws = window_set;

	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
		"Recreate", 0, this, buttons, sizeof(buttons)/sizeof(Button),
		HELP_recreate_dialog);
	box = new Packed_box(dialog, "recreate box", OR_vertical);

	caption = new Caption(box, "Command Line:", CAP_LEFT);
	cmd_line = new Simple_text(caption, create_args, TRUE);
	caption->add_component(cmd_line);
	box->add_component(caption);

	toggles = new Toggle_button(box, "toggles", create_toggles,
		sizeof(create_toggles)/sizeof(Toggle_data), OR_vertical);
	box->add_component(toggles);
	dialog->add_component(box);
}

void
Recreate_dialog::set_create_args(const char *s)
{
	cmd_line->set_text(s);
}

void
Recreate_dialog::do_recreate(Component *, void *)
{
	const char	*f;
	const char	*r;

	window_set = save_ws;
	if (toggles->is_set(0))
	{
		io_state = TRUE;
		r = "-r";
	}
	else
	{
		io_state = FALSE;
		r = "-d";
	}

	if (toggles->is_set(1))
	{
		follow_state = TRUE;
		f = "all";
	}
	else
	{
		follow_state = FALSE;
		f = "none";
	}

	dispatcher.send_msg(this, 0, "create %s -f %s\n", r, f);
	dialog->wait_for_response();

	if (toggles->is_set(2))
	{
		new_window_state = TRUE;
		window_set = new Window_set();
	}
	else
		new_window_state = FALSE;
}

void
Recreate_dialog::cancel(Component *, void *)
{
	toggles->set(0, follow_state);
	toggles->set(1, io_state);
	toggles->set(2, new_window_state);
}

Grab_process_dialog::Grab_process_dialog(Window_set *ws) : DIALOG_BOX(ws)
{
	static const Button	buttons[] =
	{
		{ B_apply,   "Grab Process",  'P', (Callback_ptr)Grab_process_dialog::apply},
		{ B_cancel,  0,	0, (Callback_ptr)Grab_process_dialog::cancel },
		{ B_help,    0,	0, 0 },
	};

	static const Toggle_data grab_toggles[] =
	{
		{ "Follow Children",	TRUE,	0 },
		{ "New Window Set",	FALSE,	0 },
	};

	Caption		*caption;
	Expansion_box	*box;
	char		**ps_data;
	int		num_proc;

	follow_state = TRUE;
	new_window_state = FALSE;
	save_obj = 0;
	save_ws = window_set;

	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
		"Grab Process", 0, this, buttons, sizeof(buttons)/sizeof(Button),
		HELP_grab_process_dialog);

	box = new Expansion_box(dialog, "grab box", OR_vertical);
	caption = new Caption(box, "   PID          Command    ", CAP_TOP_LEFT);

	num_proc = do_ps(&ps_data);
	ps_list = new Selection_list(caption, "Processes", 5, 2, "%-s %20w", num_proc,
			ps_data, SM_multiple, this, 0, 0,
			(Callback_ptr)(&Grab_process_dialog::drop_cb));
	caption->add_component(ps_list);
	box->add_elastic(caption);

	caption = new Caption(box, "Object File:", CAP_LEFT);
	object_file = new Text_line(caption, "Object File:", "", 25, 1);
	caption->add_component(object_file, FALSE);
	box->add_component(caption);

	toggles = new Toggle_button(box, "toggles", grab_toggles, 
		sizeof(grab_toggles)/sizeof(Toggle_data), OR_vertical);
	box->add_component(toggles);

	dialog->add_component(box);
}

void
Grab_process_dialog::setup()
{
	char	**ps_data;
	int	num_proc;

	window_set = save_ws;
	num_proc = do_ps(&ps_data);
	ps_list->set_list(num_proc, ps_data);
}

void
Grab_process_dialog::apply(Component *, void *)
{
	if (toggles->is_set(1))
	{
		new_window_state = TRUE;
		do_it(0);
	}
	else
	{
		new_window_state = FALSE;
		do_it(save_ws);
	}
}

void
Grab_process_dialog::do_it(Window_set *ws)
{
	const char	*f;
	char		buf[512];
	int		*pids = 0;
	int		npids;
	char		*obj_filename;

	delete save_obj;
	save_obj = 0;
	obj_filename = object_file->get_text();
	if (obj_filename && *obj_filename)
		save_obj = makestr(obj_filename);

	npids = ps_list->get_selections(pids);
	if (!npids)
	{
		dialog->error(E_ERROR, GE_no_process_selection);
		return;
	}
	if (obj_filename && *obj_filename != NULL && npids > 1)
	{
		dialog->error(E_ERROR, GE_only_one);
		return;
	}

	if (toggles->is_set(0))
	{
		follow_state = TRUE;
		f = "all";
	}
	else
	{
		follow_state = FALSE;
		f = "none";
	}

	buf[0] = '\0';
	for(int i = 0; i < npids; i++, pids++)
		strcat(strcat(buf, ps_list->get_item(*pids, 0)), " ");

	if (obj_filename && *obj_filename != NULL)
		dispatcher.send_msg(this, 0, "grab -f %s -l %s %s\n", f, obj_filename, buf);
	else
		dispatcher.send_msg(this, 0, "grab -f %s %s\n", f, buf);
	dialog->wait_for_response();

	if (ws)
		window_set = ws;
	else
		window_set = new Window_set();
}

void
Grab_process_dialog::cancel(Component *, void *)
{
	int	*pids = 0;
	int	npids;

	npids = ps_list->get_selections(pids);
	for (int i = 0; i < npids; i++, pids++)
		ps_list->deselect(*pids);

	if (save_obj)
		object_file->set_text(save_obj);
	else
		object_file->set_text("");

	toggles->set(0, follow_state);
	toggles->set(1, new_window_state);
}

void
Grab_process_dialog::drop_cb(Selection_list *, Component *dropped_on)
{
	if (dropped_on)
	{
		Base_window	*window = dropped_on->get_base();
		do_it(window->get_window_set());
	}
	else
		do_it(0);
	dialog->popdown();
}

Grab_core_dialog::Grab_core_dialog(Window_set *ws) : DIALOG_BOX(ws)
{
	static const Button	buttons[] =
	{
		{ B_ok,	    "Grab Core",  'G', (Callback_ptr)Grab_core_dialog::apply},
		{ B_cancel,  0,	0, (Callback_ptr)Grab_core_dialog::cancel },
		{ B_help,    0,	0, 0 },
	};

	static const Toggle_data new_set_data[] = { "New Window Set", FALSE, 0 };

	Caption		*caption;
	Packed_box	*box;

	save_core = 0;
	save_obj = 0;
	save_toggle = FALSE;
	save_ws = window_set;

	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
		"Grab Core", 0, this, buttons, sizeof(buttons)/sizeof(Button),
		HELP_grab_core_dialog, (Callback_ptr)&Grab_core_dialog::drop_cb,
		Drop_cb_stayup);
	box = new Packed_box(dialog, "grab box", OR_vertical);

	caption = new Caption(box, "Core File:  ", CAP_LEFT);
	core_file = new Text_line(caption, "core file", "", 25, 1);
	caption->add_component(core_file);
	box->add_component(caption);

	caption = new Caption(box, "Object File:", CAP_LEFT);
	object_file = new Text_line(caption, "object file", "", 25, 1);
	caption->add_component(object_file);
	box->add_component(caption);

	new_set = new Toggle_button(box, "new set", new_set_data, 1, OR_vertical);
	box->add_component(new_set);

	dialog->add_component(box);
}

void
Grab_core_dialog::drop_cb(Component *, void *)
{
	char *s = dialog->get_drop_item();

	if(s)
	{
		FileInfo file(s);

		switch(file.type())
		{
		case FT_EXEC:
			object_file->set_text(s);
			break;
		case FT_CORE:
			core_file->set_text(s);
			object_file->set_text(file.get_obj_name());
			break;
		default:
			dialog->error(E_ERROR, GE_bad_drop);
			break;
		}
	}
}

void
Grab_core_dialog::apply()
{
	char	*s1 = object_file->get_text();
	char	*s2 = core_file->get_text();
	if (!s1 || !*s1)
	{
		dialog->error(E_ERROR, GE_no_file);
		return;
	}

	if (!s2 || !*s2)
	{
		dialog->error(E_ERROR, GE_no_core_file);
		return;
	}

	delete save_obj;
	delete save_core;
	save_obj = makestr(s1);
	save_core = makestr(s2);

	dispatcher.send_msg(this, 0, "grab -c %s %s\n", save_core, save_obj);
	dialog->wait_for_response();

	if (new_set->is_set(0))
		window_set = new Window_set();
	else
		window_set = save_ws;
	save_toggle = new_set->is_set(0);
}

void
Grab_core_dialog::cancel()
{
	object_file->set_text(save_obj);
	core_file->set_text(save_core);
	new_set->set(0, save_toggle);
}

Set_language_dialog::Set_language_dialog(Window_set *ws) : DIALOG_BOX(ws)
{
	static const Button	buttons[] =
	{
		{ B_ok,	     0, 0, (Callback_ptr)Set_language_dialog::apply },
		{ B_apply,   0, 0, (Callback_ptr)Set_language_dialog::apply },
		{ B_reset,   0, 0, (Callback_ptr)Set_language_dialog::reset },
		{ B_cancel,  0,	0, (Callback_ptr)Set_language_dialog::reset },
		{ B_help,    0,	0, 0 },
	};

	static const char *language_buttons[] =
	{
		"Derived",
		"C",
		"C++",
	};

	Caption		*caption;

	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
		"Set Language", 0, this, buttons, sizeof(buttons)/sizeof(Button),
		HELP_language_dialog);

	caption = new Caption(dialog, "Language:", CAP_LEFT);
	lang_choices = new Radio_list(caption, "language", OR_horizontal,
		language_buttons, sizeof(language_buttons)/sizeof(char *),
		cur_language);
	caption->add_component(lang_choices, FALSE);
	dialog->add_component(caption);
}

void
Set_language_dialog::apply()
{
	const char	*language;

	cur_language = (Language) lang_choices->which_button();
	switch(cur_language)
	{
	case 0:		language = "\"\"";	break;
	case 1:		language = "C";		break;
	case 2:		language = "C++";	break;

	default:	
		dialog->error(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	dispatcher.send_msg(this, 0, "set %%lang = %s\n", language);
	dialog->wait_for_response();
}

void
Set_language_dialog::reset()
{
	lang_choices->set_button(cur_language);
}

void
set_lang(Message *m)
{
	Window_set	*ws;
	char		*l;

	m->unbundle(l);
	if (strcmp(l, "C") == 0)
		cur_language = C;
	else if (strcmp(l, "C++") == 0)
		cur_language = CPLUS;
	else
		cur_language = UnSpec;

	for (ws = (Window_set *)windows.first(); ws; ws = (Window_set *)windows.next())
		ws->update_language_dialog();
}

void
set_directory()
{
	Window_set	*ws;
	Message		*msg;
	char		*ptr = 0;

	dispatcher.query(0, 0, "pwd\n");
	while ((msg = dispatcher.get_response()) != 0)
	{
		if (msg->get_msg_id() == MSG_pwd)
		{
			msg->unbundle(ptr);
			delete (char *)working_directory;
			working_directory = makestr(ptr);
		}
		else
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}

	chdir(working_directory);
	for (ws = (Window_set *)windows.first(); ws; ws = (Window_set *)windows.next())
		ws->update_cd_dialog(working_directory);
}

Cd_dialog::Cd_dialog(Window_set *ws) : DIALOG_BOX(ws)
{
	static const Button	buttons[] =
	{
		{ B_ok,	    "Change Directory",  'D', (Callback_ptr)Cd_dialog::apply },
		{ B_cancel,  0,	0,	    (Callback_ptr)Cd_dialog::cancel },
		{ B_help,    0,	0, 0 },
	};

	Packed_box	*box;
	Caption		*caption;
	char		*pwd = 0;

	if (!working_directory)
		set_directory();
	save_text = 0;

	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
		"Change Directory", 0, this, buttons,
		sizeof(buttons)/sizeof(Button), HELP_cd_dialog);
	box = new Packed_box(dialog, "", OR_vertical);
	dialog->add_component(box);

	caption = new Caption(box, "Current Directory:", CAP_LEFT);
	current_directory = new Simple_text(caption, working_directory, FALSE);
	caption->add_component(current_directory);
	box->add_component(caption);

	caption = new Caption(box, "New Directory:", CAP_LEFT);
	new_directory = new Text_line(caption, "directory", "", 25, 1);
	caption->add_component(new_directory);
	box->add_component(caption);
}

void
Cd_dialog::apply(Component *, void *)
{
	char		*s;

	delete save_text;
	s = new_directory->get_text();
	save_text = makestr(s ? s : "");

	dispatcher.send_msg(this, window_set->current_process()->get_id(),
		 "cd %s\n", s);
	dialog->wait_for_response();
}

void
Cd_dialog::cancel(Component *, void *)
{
	new_directory->set_text(save_text);
}

void
Cd_dialog::update_directory(const char *s)
{
	current_directory->set_text(s);
}
