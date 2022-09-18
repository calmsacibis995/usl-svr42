#ident	"@(#)debugger:gui.d/common/Source.C	1.28"

// GUI headers
#include "Dialogs.h"
#include "Dialog_sh.h"
#include "Sch_dlg.h"
#include "Dispatcher.h"
#include "Eventlist.h"
#include "Source.h"
#include "Status.h"
#include "Windows.h"
#include "Window_sh.h"
#include "Menu.h"
#include "Text_disp.h"
#include "Text_line.h"
#include "Radio.h"
#include "Sel_list.h"
#include "Stext.h"
#include "Boxes.h"
#include "Caption.h"
#include "Proclist.h"
#include "UI.h"

// Debug headers
#include "Message.h"
#include "Vector.h"
#include "str.h"

#include <stdio.h>
#include <stdlib.h>

class Open_dialog : public Process_dialog
{
	Source_window	*source;
	Selection_list	*files;
	int		selection;	// state saved for cancel

	void		do_it(Source_window *);
public:
			Open_dialog(Source_window *);
			~Open_dialog() {}

			// callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);
	void		drop_cb(Selection_list *, Component *);

			// get list of files from debug
	char		**get_files(int &);

			// inherited from Process_dialog
	void		set_process(Boolean reset);
};

Open_dialog::Open_dialog(Source_window *sw) : PROCESS_DIALOG(sw->get_window_set())
{
	static const Button	buttons[] =
	{
		{ B_apply,	"Open",	'O',	(Callback_ptr)(&Open_dialog::apply) },
		{ B_cancel,	0, 0,		(Callback_ptr)(&Open_dialog::cancel) },
		{ B_help,	0, 0, 0 },
	};

	Expansion_box	*box;
	const char	*initial_string = "";

	selection = -1;
	source = sw;

	dialog = new Dialog_shell(sw->get_window_shell(), "Open",
		(Callback_ptr)Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_open_dialog);
	box = new Expansion_box(dialog, "box", OR_vertical);
	dialog->add_component(box);

	process_caption = new Caption(box, "Process:", CAP_LEFT);
	process_list = new Simple_text(process_caption, "", TRUE);
	process_caption->add_component(process_list);
	box->add_component(process_caption);

	files = new Selection_list(box, "files", 7, 1, "%s", 1,
		&initial_string, SM_single, this, 0, 0,
		(Callback_ptr)Open_dialog::drop_cb);
	box->add_elastic(files);
}

// get list of files from debug
char **
Open_dialog::get_files(int &nfiles)
{
	Message	*msg;
	char	*fname;

	nfiles = 0;
	vscratch1.clear();

	dispatcher.query(this, window_set->current_process()->get_id(), "pfiles\n");
	while ((msg = dispatcher.get_response()) != 0)
	{
		if (msg->get_msg_id() != MSG_source_file)
		{
			display_msg(msg);
			continue;
		}
		msg->unbundle(fname);
		fname = str(fname);
		vscratch1.add(&fname, sizeof(char *));
		nfiles++;
	}

	qsort((char **)vscratch1.ptr(), nfiles, sizeof(char *), alpha_comp);
	return (char **)vscratch1.ptr();
}

// update the list of files when the current process changes
void
Open_dialog::set_process(Boolean reset)
{
	char	**flist;
	int	nfiles;

	if (!processes)
	{
		files->set_list(0,0);
		return;
	}

	if (reset)
		dialog->set_busy(TRUE);
	if ((flist = get_files(nfiles)) == 0)
		dialog->error(E_ERROR, GE_no_source, processes[0]->get_name());
	else
		files->set_list(nfiles, flist);
	selection = -1;
	if (reset)
		dialog->set_busy(FALSE);
}

// open the selected file in the given source window.  if sw is null,
// (meaning the selection was dropped on the workspace) create a new
// source window
void
Open_dialog::do_it(Source_window *sw)
{
	Message		*msg;
	const char	*fname;
	int		nsels;
	int		*selections;
	char		*path = 0;

	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	if ((nsels = files->get_selections(selections)) == 0)
	{
		dialog->error(E_ERROR, GE_file_needed);
		return;
	}

	selection = *selections;
	fname = files->get_item(selection, 0);
	dispatcher.query(this, processes[0]->get_id(), "ppath %s\n", fname);
	while ((msg = dispatcher.get_response()) != 0)
	{
		if (msg->get_msg_id() != MSG_source_file)
		{
			display_msg(msg);
			continue;
		}
		msg->unbundle(path);
	}

	if (path)
	{
		if (!sw)
			sw = new Source_window(window_set, FALSE);
		sw->popup();
		sw->set_file(fname, path);
		sw->get_pane()->position(1, FALSE);
		sw->set_sensitivity();
	}
}

// button push callback - open the selected file in the parent source window
void
Open_dialog::apply(Component *, void *)
{
	do_it(source);
}

void
Open_dialog::cancel(Component *, void *)
{
	int	nsels;
	int	*selections;

	if ((nsels = files->get_selections(selections)) != 0)
		files->deselect(*selections);
	if (selection > -1)
		files->select(selection);
}

// find out where the selection was dropped, and open the file in that
// source window.  If it was not dropped on a source window, sw will be
// null, which will make do_it create a new, secondary source window
void
Open_dialog::drop_cb(Selection_list *, Component *drop_window)
{
	Source_window	*sw = 0;

	if (drop_window)
	{
		Base_window	*window = drop_window->get_base();
		if (window && window->get_type() == BW_source
			&& window->get_window_set() == window_set)
			sw = (Source_window *)window;
	}

	dialog->clear_msg();
	do_it(sw);
	dialog->popdown();
}

class Show_line_dialog : public Dialog_box
{
	Source_window	*source;
	Text_line	*line;
	char		*save_string;	// saved for cancel operation

public:
			Show_line_dialog(Source_window *);
			~Show_line_dialog() { delete save_string; }

			// callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);
};

Show_line_dialog::Show_line_dialog(Source_window *sw)
	: DIALOG_BOX(sw->get_window_set())
{
	static const Button	buttons[] =
	{
		{ B_apply,	"Show Line", 'S',
				(Callback_ptr)(&Show_line_dialog::apply) },
		{ B_cancel,	0, 0, (Callback_ptr)(&Show_line_dialog::cancel) },
		{ B_help,	0, 0, 0 },
	};

	Packed_box	*box;
	Caption		*caption;

	save_string = 0;
	source = sw;

	dialog = new Dialog_shell(sw->get_window_shell(), "Show Line", 0, this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_show_line_dialog);
	box = new Packed_box(dialog, "show line", OR_vertical);

	caption = new Caption(box, "Line:", CAP_LEFT);
	line = new Text_line(caption, "line", "", 5, 1);

	caption->add_component(line, FALSE);
	box->add_component(caption);
	dialog->add_component(box);
}

void
Show_line_dialog::cancel(Component *, void *)
{
	line->set_text(save_string);
}

// position the file in the parent source window at the given line
void
Show_line_dialog::apply(Component *, void *)
{
	char	*s = line->get_text();
	char	*p;
	int	l;

	if (!s || !*s)
	{
		dialog->error(E_ERROR, GE_no_number);
		return;
	}

	l = (int) strtol(s, &p, 10);
	if (p == s || *p)
	{
		dialog->error(E_ERROR, GE_no_number);
		return;
	}

	if (l <= 0 || l > source->get_pane()->get_last_line())
	{
		dialog->error(E_ERROR, GE_out_of_bounds);
		return;
	}

	source->get_pane()->position(l, TRUE);
	delete save_string;
	save_string = makestr(s);
}

class Show_function_dialog : public Process_dialog
{
	Source_window	*source;
	Selection_list	*objects;
	Selection_list	*functions;
	Radio_list	*choice;

			// state information saved for cancel
	int		obj_selection;
	int		f_selection;
	int		use_object;

	void		do_it(Source_window *);
	void		set_list(const char *object, const char *file);

public:
			Show_function_dialog(Source_window *);
			~Show_function_dialog()	{}

			// callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);
	void		select_cb(Selection_list *, int);
	void		drop_cb(Selection_list *, Component *);
	void		set_function_type(Radio_list *, int);

			// inherited from Process_dialog
	void		set_process(Boolean reset);
};

Show_function_dialog::Show_function_dialog(Source_window *sw)
	: PROCESS_DIALOG(sw->get_window_set())
{
	static const Button	buttons[] =
	{
		{ B_apply,	"Show Function", 'S', (Callback_ptr)(&Show_function_dialog::apply) },
		{ B_cancel,	0, 0, (Callback_ptr)(&Show_function_dialog::cancel) },
		{ B_help,	0, 0, 0 },
	};

	static char *radio_buttons[] =
	{
		"Current file",
		"Selected object",
	};

	Expansion_box	*box;
	Caption		*caption;
	const char	*initial_string = "";	// blank data for selection list
				// real data is filled in later - after set_plist

	obj_selection = -1;
	f_selection = -1;
	source = sw;
	use_object = (sw->get_current_file() == 0);

	dialog = new Dialog_shell(sw->get_window_shell(), "Show Function", 
		(Callback_ptr)(&Process_dialog::dismiss_cb), this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_show_function_dialog_1);
	box = new Expansion_box(dialog, "show function", OR_vertical);
	dialog->add_component(box);

	process_caption = new Caption(box, "Process:", CAP_LEFT);
	process_list = new Simple_text(process_caption, " ", TRUE);
	process_caption->add_component(process_list);
	box->add_component(process_caption);

	caption = new Caption(box, "Functions from:", CAP_LEFT);
	choice = new Radio_list(caption, "function type", OR_vertical,
		radio_buttons, sizeof(radio_buttons)/sizeof(char *), use_object,
		(Callback_ptr)Show_function_dialog::set_function_type, this);
	caption->add_component(choice, FALSE);
	box->add_component(caption);

	caption = new Caption(box, "Objects:", CAP_TOP_LEFT);
	box->add_component(caption);
	objects = new Selection_list(caption, "objects", 4, 1, "%s", 1, 
		&initial_string, SM_single, this,
		(Callback_ptr)Show_function_dialog::select_cb);
	caption->add_component(objects);

	caption = new Caption(box, "Functions:", CAP_TOP_LEFT);
	box->add_elastic(caption);
	functions = new Selection_list(caption, "functions", 8, 1, "%25s", 1,
		&initial_string, SM_single, this, 0, 0,
		(Callback_ptr)Show_function_dialog::drop_cb);
	caption->add_component(functions);
}

// update the selection lists when the current process changes
void
Show_function_dialog::set_process(Boolean reset)
{
	const char	**list;
	int		total = 0;

	obj_selection = -1;
	f_selection = -1;

	if (!processes)
	{
		objects->set_list(0, 0);
		functions->set_list(0, 0);
		return;
	}

	if (reset)
		dialog->set_busy(TRUE);
	list = processes[0]->get_objects(total);
	objects->set_list(total, list);

	if (!source->get_current_file())
	{
		use_object = 1;
		choice->set_button(use_object);
	}

	if (use_object)	// since there is no selected object, blank out the list of files
		functions->set_list(0, 0);
	else
	{
		list = processes[0]->get_functions(total, source->get_current_file(), 0, 1);
		functions->set_list(total, list);
	}
	if (reset)
		dialog->set_busy(FALSE);
}

// Display the function in the given source window.  If sw is NULL,
// meaning the function was dragged and dropped onto the workspace,
// create a new secondary source window
void
Show_function_dialog::do_it(Source_window *sw)
{
	Message		*msg;
	const char	*fname;
	int		*selections;
	Word		line = 0;
	char		*file = 0;
	char		*tmp;

	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;	
	}

	if (functions->get_selections(selections) == 0)
	{
		dialog->error(E_ERROR, GE_no_function);
		return;
	}

	// selection list ensures that there is at most one selection
	f_selection = *selections;
	fname = functions->get_item(f_selection, 0);
	use_object = choice->which_button();
	if (use_object)
	{
		objects->get_selections(selections);
		obj_selection = *selections;
	}

	// find the file and line number for the selected function
	dispatcher.query(this, processes[0]->get_id(), "list -c1 %s\n", fname);
	while ((msg = dispatcher.get_response()) != 0)
	{
		switch (msg->get_msg_id())
		{
		case MSG_line_src:
			msg->unbundle(line, tmp);
			break;

		case ERR_newer_file:
			display_msg(msg);
			break;
	
		default:
			dialog->error(E_ERROR, GE_no_source, fname);
			break;
		}
	}
	if (!line)
		return;

	dispatcher.query(this, processes[0]->get_id(), "print %%list_file\n");
	while ((msg = dispatcher.get_response()) != 0)
	{
		if (msg->get_msg_id() == MSG_print_val)
			msg->unbundle(file);
		else
			dialog->error(msg);
	}

	if (file)
	{
		// strip off quotes
		if (*file == '"')
		{
			file++;
			char *p = strchr(file, '"');
			if(p)
				*p = '\0';
		}
		if (!sw)
			sw = new Source_window(window_set, FALSE);
		sw->popup();
		sw->set_file(file, 0);
		sw->get_pane()->position((int)line-1, FALSE);
		sw->set_sensitivity();
	}
}

// Display the function in the parent source window
void
Show_function_dialog::apply(Component *, void *)
{
	do_it(source);
}

// reset the radio button and both selection lists
void
Show_function_dialog::cancel(Component *, void *)
{
	int	*selections;

	if (use_object != choice->which_button())
	{
		f_selection = -1;
		set_function_type(0, use_object);
	}
	else if (obj_selection > -1)
	{
		const char	**funclist;
		int		nfuncs;

		objects->select(obj_selection);
		funclist = processes[0]->get_functions(nfuncs, 0,
				objects->get_item(obj_selection,0), 1);
		functions->set_list(nfuncs, funclist);
		f_selection = -1;
	}
	else
	{
		if ((total = objects->get_selections(selections)) > 0)
			objects->deselect(*selections);
		functions->set_list(0, 0);
	}
}

// Display the selected function in the dropped on window
void
Show_function_dialog::drop_cb(Selection_list *, Component *drop_window)
{
	Source_window	*sw = 0;

	if (drop_window)
	{
		Base_window	*window = drop_window->get_base();
		if (window && window->get_type() == BW_source
			&& window->get_window_set() == window_set)
			sw = (Source_window *)window;
	}
	dialog->clear_msg();
	do_it(sw);
	dialog->popdown();
}

// Reset the function selection list whenever the user switches between
// the current file and the current object
void
Show_function_dialog::set_function_type(Radio_list *radio, int button)
{
	const char	**funclist = 0;
	int		nfuncs = 0;

	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;	
	}

	if (!radio)	// i.e. this was called from cancel, not from the radio button
		choice->set_button(button);

	dialog->set_busy(TRUE);
	dialog->clear_msg();
	if (button)
	{
		int	*selections;
		if (objects->get_selections(selections) == 1)
		{
			funclist = processes[0]->get_functions(nfuncs, 0,
				objects->get_item(*selections, 0), 1);
			if (!funclist)
			{
				dialog->error(E_ERROR, GE_no_source,
					objects->get_item(*selections, 0));
				functions->set_list(0, 0);
				dialog->set_busy(FALSE);
				return;
			}
		}
	}
	else
	{
		if (!source->get_current_file())
		{
			dialog->error(E_ERROR, GE_no_current_file);
			choice->set_button(1);
			dialog->set_busy(FALSE);
			return;
		}
		funclist = processes[0]->get_functions(nfuncs,
			source->get_current_file(), 0, 1);
	}

	use_object = button;
	functions->set_list(nfuncs, funclist);
	f_selection = -1;
	dialog->set_busy(FALSE);
}

// Reset the file selection list whenever the user selects a different object
void
Show_function_dialog::select_cb(Selection_list *, int selection)
{
	const char	**funclist;
	int		nfuncs;

	if (!use_object)
		return;

	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;	
	}

	dialog->clear_msg();
	funclist = processes[0]->get_functions(nfuncs, 0, objects->get_item(selection, 0), 1);
	functions->set_list(nfuncs, funclist);
	f_selection = -1;
	if (!funclist)
		dialog->error(E_ERROR, GE_no_source,
			objects->get_item(selection, 0));
}

static const Menu_table windows_pane[] =
{
    { "Context",  'C', Set_cb,	SEN_always,	
					(Callback_ptr)(&Window_set::popup_context) },
    { "Source",   'S', Set_cb,	SEN_process,
					(Callback_ptr)(&Window_set::popup_source) },
    { "Disassembly",   'D', Set_cb,	SEN_process,
					(Callback_ptr)(&Window_set::popup_disasm) },
    { "Events",	  'E', Set_cb,	SEN_process|SEN_proc_live,	
					(Callback_ptr)(&Window_set::popup_event) },
    { "Command",  'o', Set_cb,	SEN_always,	
					(Callback_ptr)(&Window_set::popup_command) },
};

static const Menu_table file_pane[] =
{
    { "Open...",	'O', Window_cb,	SEN_process, 
				(Callback_ptr)(&Source_window::open_dialog_cb),
				HELP_source_open_dialog },
    { "New Source",	'N', Window_cb,	SEN_process,
				(Callback_ptr)(&Source_window::new_source_cb),
				HELP_new_source_cmd },
    { "Windows",	'W', Menu_button,	SEN_always,
				(Callback_ptr)windows_pane,
				HELP_source_windows_menu,
				sizeof(windows_pane)/sizeof(Menu_table) },
    { DISMISS,	DISMISS_MNE, Set_cb,	SEN_always, 
				(Callback_ptr)(&Window_set::dismiss),
				HELP_source_dismiss_cmd },
    { "Quit",		'Q', Set_cb,	SEN_always,	
				(Callback_ptr)(&Window_set::ok_to_quit),
				HELP_source_quit_cmd },
};

static const Menu_table view_pane[] =
{
    { "Show Line...",    'L',	Window_cb,	SEN_file_required,
				(Callback_ptr)(&Source_window::show_line_cb),
				HELP_source_show_line_dialog },
    { "Show Function...", 'F',	Window_cb,	SEN_process,
				(Callback_ptr)(&Source_window::show_function_cb),
				HELP_source_show_function_dialog },
    { "Search...",	'e',	Set_cb,	SEN_file_required,
				(Callback_ptr)(&Window_set::search_dialog_cb),
				HELP_source_search_dialog },
    { "Show Value...",	'V',	Set_cb,		SEN_always,	
				(Callback_ptr)(&Window_set::show_value_dialog_cb),
				HELP_source_show_value_dialog },
    { "Set Value...",	'S',	Set_cb,		SEN_always,
				(Callback_ptr)(&Window_set::set_value_dialog_cb),
				HELP_source_set_value_dialog },
};

static const Menu_table edit_pane[] =
{
    { "Copy",		'C',	Window_cb,	SEN_sel_required,
				(Callback_ptr)(&Source_window::copy_cb),
				HELP_copy_cmd },
};

static const Menu_table prop_pane[] =
{
	{ "Source Path...",  'P',	Set_cb,		SEN_always, 
				(Callback_ptr)(&Window_set::path_dialog_cb),
				HELP_source_path_dialog },
	{ "Language...", 'L',	Set_cb,	SEN_always,	
				(Callback_ptr)(&Window_set::set_language_dialog_cb),
				HELP_source_language_dialog },
	{ "Granularity...", 'G', Set_cb,	SEN_always,
			(Callback_ptr)(&Window_set::set_granularity_cb),
				HELP_source_granularity_dialog },
};

static const Menu_table control_pane[] =
{
    { "Run",		'R', Set_cb,	(SEN_process|SEN_proc_stopped), 
				(Callback_ptr)(&Window_set::run_button_cb),
				HELP_source_run_cmd },
    { "Return",		't', Set_cb, (SEN_process|SEN_proc_stopped),
				(Callback_ptr)(&Window_set::run_r_button_cb),
				HELP_source_return_cmd },
    { "Run Until...",	'U', Set_cb,	(SEN_process|SEN_proc_stopped),
				(Callback_ptr)(&Window_set::run_dialog_cb),
				HELP_source_run_dialog },
    { "Step Statement",	'S', Set_cb,	(SEN_process|SEN_proc_stopped),
				(Callback_ptr)(&Window_set::step_button_cb),
				HELP_source_step_stmt_cmd },
    { "Step Instruction", 'I', Set_cb,	(SEN_process|SEN_proc_stopped), 
				(Callback_ptr)(&Window_set::step_i_button_cb), 
				HELP_source_step_instr_cmd },
    { "Next Statement",	'N', Set_cb,	(SEN_process|SEN_proc_stopped),
				(Callback_ptr)(&Window_set::step_o_button_cb),
				HELP_source_next_stmt_cmd },
    { "Next Instruction", 'x', Set_cb,	(SEN_process|SEN_proc_stopped),
				(Callback_ptr)(&Window_set::step_oi_button_cb),
				HELP_source_next_instr_cmd },
    { "Step...",	'e',  Set_cb, (SEN_process|SEN_proc_stopped),
				(Callback_ptr)(&Window_set::step_dialog_cb),
				HELP_source_step_dialog },
    { "Jump...", 	'J', Set_cb, (SEN_process|SEN_proc_stopped),
				(Callback_ptr)(&Window_set::jump_dialog_cb),
				HELP_source_jump_dialog },
    { "Halt",		'H', Set_cb, (SEN_process|SEN_proc_running),
				(Callback_ptr)(&Window_set::halt_button_cb),
				HELP_source_halt_cmd },
};

static const Menu_table event_pane[] =
{
    { "Set Breakpoint",	'B',  Window_cb,
			(SEN_process|SEN_proc_stopped|SEN_sel_required),
			(Callback_ptr)(&Source_window::set_break_cb),
			HELP_set_breakpoint_cmd },
    { "Delete Breakpoint", 'D', Window_cb,
		(SEN_process|SEN_proc_stopped|SEN_sel_required|SEN_breakpt_required),
		(Callback_ptr)(&Source_window::delete_break_cb),
		HELP_delete_breakpoint_cmd },
    { "Stop...",	'S',  Set_cb, (SEN_process|SEN_proc_stopped),
			(Callback_ptr)(&Window_set::stop_dialog_cb),
			HELP_source_stop_dialog },
    { "Signal...",	'i',  Set_cb, (SEN_process|SEN_proc_stopped),
			(Callback_ptr)(&Window_set::signal_dialog_cb),
			HELP_source_signal_dialog },
    { "Syscall...",	'y',  Set_cb, (SEN_process|SEN_proc_stopped),
			(Callback_ptr)(&Window_set::syscall_dialog_cb),
			HELP_source_syscall_dialog },
    { "On Stop...",	'O',  Set_cb, (SEN_process|SEN_proc_stopped),
			(Callback_ptr)(&Window_set::onstop_dialog_cb),
			HELP_source_onstop_dialog },
    { "Cancel...",	'C',  Set_cb, (SEN_process|SEN_proc_stopped),
			(Callback_ptr)(&Window_set::cancel_dialog_cb),
			HELP_source_cancel_dialog },
    { "Kill...",	'K',  Set_cb, (SEN_process|SEN_proc_live),
			(Callback_ptr)(&Window_set::kill_dialog_cb),
			HELP_source_kill_dialog },
    { "Ignore Signals...",'g', Set_cb, (SEN_process|SEN_proc_stopped),
			(Callback_ptr)(&Window_set::setup_signals_dialog_cb),
			HELP_source_ignore_signals_dialog },
};

// The number of entries in the help menu is hard-coded because of a
// bug in cfront 2.1
static const Menu_table help_pane[3] =
{
	{ "Source...", 'S',     Set_cb, SEN_always,
				(Callback_ptr)(&Window_set::help_sect_cb) },
	{ "Table of Contents...", 'T', Set_cb, SEN_always,
				(Callback_ptr)(&Window_set::help_toc_cb) },
	{ "Version",	'V',	Set_cb,	SEN_always,
				(Callback_ptr)(&Window_set::version_cb) },
};

static Menu_bar_table menu_table[] =
{
    { "File",     file_pane,    'F', sizeof(file_pane)/sizeof(Menu_table),
				HELP_source_file_menu },
    { "Edit",     edit_pane,    'E', sizeof(edit_pane)/sizeof(Menu_table),
				HELP_source_edit_menu },
    { "View",     view_pane,    'V', sizeof(view_pane)/sizeof(Menu_table),
				HELP_source_view_menu },
    { "Control",  control_pane, 'C', sizeof(control_pane)/sizeof(Menu_table),
				HELP_source_control_menu },
    { "Event",    event_pane,   'n', sizeof(event_pane)/sizeof(Menu_table),
				HELP_source_event_menu },
    { "Properties", prop_pane,  'P', sizeof(prop_pane)/sizeof(Menu_table),
				HELP_source_properties_menu },
    { "Help",	  help_pane,    'H', sizeof(help_pane)/sizeof(Menu_table) },
};
			
Source_window::Source_window(Window_set *ws, Boolean p) : BASE_WINDOW(ws, BW_source)
{
	Expansion_box	*box;

	current_file = 0;
	current_path = 0;
	current_line = 0;
	selected_line = 0;
	file_ptr = 0;
	flink = 0;

	pane = 0;
	caption = 0;

	show_line = 0;
	show_function = 0;
	open_box = 0;

	registered = FALSE;
	primary = p;
	window = new Window_shell(primary ? "Source" : "Source *",
		0, this, HELP_source_window);
	box = new Expansion_box(window, "box", OR_vertical);
	window->add_component(box);

	menu_bar = new Menu_bar(box, this, ws, menu_table, 
		sizeof(menu_table)/sizeof(Menu_bar_table));
	box->add_component(menu_bar);

	status_pane = new Status_pane(box);

	caption = new Caption(box, " ", CAP_TOP_CENTER);
	pane = new Text_display(caption, "source",
		(Callback_ptr)(&Source_window::select_cb), this);
	pane->setup_source_file(0, 10, 80);	// initialize text window size
	pane->set_breaklist(0);			// initialize breaks

	caption->add_component(pane);
	box->add_elastic(caption);

	if (!primary)
	{
		window_set->inc_open_windows();
		window_set->source_windows.add(this);
	}
}

Source_window::~Source_window()
{
	delete show_line;
	delete show_function;
	delete open_box;

	delete window;
	delete status_pane;
	delete current_path;

	if (!primary)
	{
		// open_windows already decremented in Window_set::dismiss
		window_set->source_windows.remove(this);
	}
}

void
Source_window::clear()
{
	//if called inside constructor, caption is null
	if (caption)
		caption->set_label(" ");
	if (pane)
		pane->clear();
	selected_line = 0;
	current_line = 0;
	current_file = 0;
	delete current_path;
	current_path = 0;
	file_ptr = 0;
	set_sensitivity();
}

void
Source_window::update_cb(void *, Reason_code rc, void *, Process *cdata)
{
	if (!_is_open)
		return;

	status_pane->update(cdata);
	if (rc == RC_rename)
		return;

	// The source window is only registered with the Event list's notifier -
	// to get notifications of new or deleted breakpoints - while there is
	// a live (non-core) current process.  
	if (rc == RC_set_current)
	{
		if (!cdata || cdata->get_state() == State_core)
		{
			if (registered)
			{
				registered = FALSE;
				event_list.change_list.remove(this,
					(Notify_func)(&Source_window::break_list_cb), 0);
			}
		}
		else if (!registered)
		{
			registered = TRUE;
			event_list.change_list.add(this,
				(Notify_func)(&Source_window::break_list_cb), 0);
		}
	}
		
	if (!cdata)
	{
		window->clear_msg();
		window->display_msg(E_WARNING, GE_no_proc_no_source);
		clear();	// clear calls set_sensitivity
		return;
	}

	if (!primary)
	{
		// secondary source windows are not updated when the process
		// state changes, except to update the current line indicator
		if (current_file == cdata->get_file())
		{
			current_line = cdata->get_line();
			pane->set_line(current_line);
		}
		else	// file where process stopped no longer matches displayed file
			pane->set_line(0);
		set_sensitivity();
		return;
	}

	if (cdata->get_state() == State_running
		|| cdata->get_state() == State_stepping)
	{
		// if this is a new current process, the displayed file from
		// the previous process is no longer valid.  Otherwise, the
		// process has just been set running - the file is still ok
		// to display, but there is no current line.
		if  (rc == RC_set_current)
			clear();
		else
		{
			current_line = 0;
			pane->set_line(0);
			set_sensitivity();
		}
		return;
	}

	// state == stopped
	if (!cdata->get_file())
	{
		window->clear_msg();
		window->display_msg(E_WARNING, GE_no_source, cdata->get_function());
		clear();
		return;
	}

	current_file = cdata->get_file();
	current_line = cdata->get_line();

	if (set_path(rc == RC_set_current))
	{
		if (current_path)
		{
			pane->set_file(current_path);
			caption->set_label(current_path ? current_path : " ");
			file_ptr = event_list.find_file(current_file, 0);
			pane->set_breaklist(file_ptr ? file_ptr->get_break_list(cdata) : 0);
		}
		else
		{
			clear();
			return;
		}
	}
	if (current_line)
	{
		pane->set_line(current_line);
		pane->position(current_line, TRUE);
	}
	set_sensitivity();
}

int
Source_window::check_sensitivity(int sense)
{
	if ((sense&SEN_file_required) && !current_file)
		return 0;
	if ((sense&SEN_sel_required) && !selected_line)
		return 0;
	if ((sense&SEN_breakpt_required) && !pane->has_breakpoint(selected_line))
		return 0;
	if (sense & SEN_process)
	{
		if (!window_set->current_process())
			return 0;
		return window_set->current_process()->check_sensitivity(sense);
	}
	return 1;
}

// An event with breakpoints is being added or deleted
// Check each breakpoint in the event to see if it effects the current file
void
Source_window::break_list_cb(void *, Reason_code_break rc, void *, Stop_event *event)
{
	Process	*proc = window_set->current_process();
	if (!proc || !event->has_process(proc))
		return;

	Breakpoint	*breaks = event->get_breakpts();

	for (int i = event->get_nbreakpts(); i; i--, breaks++)
	{
		File_list	*fptr = event_list.find_file(breaks->file, 0);

		if (strcmp(fptr->get_name(), current_path) != 0 &&
			strcmp(fptr->get_name(), current_file) != 0)
			continue;

		if (rc == BK_add)
		{
			if (!file_ptr)
				file_ptr = fptr;
			pane->set_stop(breaks->line);
		}
		else if (rc == BK_delete)
		{
			int	*events = 0;
			int	*breaklist = proc->get_break_list(breaks->addr,
					breaks->addr, events);

			if (!events)
			{
				window->clear_msg();
				window->display_msg(E_ERROR, GE_internal,
					__FILE__, __LINE__);
				continue;
			}

			for (; *events; events++)
			{
				if (*events != event->get_id()
					&& event_list.findEvent(*events)->get_state() == ES_valid)
					break; // one enabled event exists
			}

			if (!*events) // didn't break out of loop
				pane->clear_stop(breaks->line);
		}
		else
		{
			window->clear_msg();
			window->display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}
	}
	set_sensitivity();
}

void
Source_window::popup()
{
	if (_is_open)
	{
		window->raise();
		return;
	}

	_is_open = 1;
	window->popup();

	Process	*proc = window_set->current_process();
	update_cb(0, RC_set_current, 0, proc);
	if (proc && proc->get_state() != State_core)
	{
		registered = TRUE;
		event_list.change_list.add(this,
			(Notify_func)(&Source_window::break_list_cb), 0);
	}
	window_set->change_current.add(this, (Notify_func)(&Source_window::update_cb), 0);
}

void
Source_window::popdown()
{
	window->popdown();
	_is_open = 0;
	window_set->change_current.remove(this, (Notify_func)(&Source_window::update_cb), 0);
	if (registered)
	{
		registered = FALSE;
		event_list.change_list.remove(this,
			(Notify_func)(&Source_window::break_list_cb), 0);
	}
	if (!primary)
		delete this;
}

void
Source_window::set_break_cb(Component *, void *)
{
	Process	*proc = window_set->current_process();
	dispatcher.send_msg(this, proc->get_id(), "stop -p %s %s@%d\n",
		(window_set->get_event_level() == PROGRAM_LEVEL)
			? proc->get_program()->get_name() : proc->get_name(),
		current_file, selected_line);
}

void
Source_window::delete_break_cb(Component *, void *)
{
	if (!file_ptr)
	{
		window->clear_msg();
		window->display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	for (flink = file_ptr->get_head(); flink; flink = flink->next())
	{
		if (flink->get_line() >= selected_line)
			break;
	}

	if (flink->get_line() > selected_line)
	{
		window->clear_msg();
		window->display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	// if one entry in the chain and the next both point to the same line,
	//	there are multiple breakpoints on the same line - ask for
	// 	confirmation before deleteing them,
	// else if the stop event has multiple breakpoints at different places
	//	ask for confirmation (this would have to come from an expression
	//	entered in the Stop dialog - it couldn't happen with Set Breakpoint)
	// else delete
	if (flink->next() && flink->next()->get_line() == selected_line)
		display_msg((Callback_ptr)(&Source_window::ok_to_delete), this,
			"Delete", "Cancel", GE_multiple_events);
	else if (((Stop_event *)flink->get_event())->get_nbreakpts() > 1)
		display_msg((Callback_ptr)(&Source_window::ok_to_delete), this,
			"Delete", "Cancel", GE_multiple_breaks);
	else
		dispatcher.send_msg(this, window_set->current_process()->get_id(),
			"delete %d\n", flink->get_event()->get_id());
}

// ok_to_delete is called only from delete_break_cb, above, which sets flink
// to point to the entry to be deleted
void
Source_window::ok_to_delete(Component *, int delete_flag)
{
	if (!delete_flag)
		return;

	for ( ; flink && flink->get_line() == selected_line; flink = flink->next())
		dispatcher.send_msg(this, window_set->current_process()->get_id(),
			"delete %d\n", flink->get_event()->get_id());
}

void
Source_window::copy_cb(Component *, void *)
{
	pane->copy_selection();
}

void
Source_window::open_dialog_cb(Component *, void *)
{
	if (!open_box)
		open_box = new Open_dialog(this);
	open_box->set_plist(this, PROCESS_LEVEL);
	open_box->display();
}

void
Source_window::show_function_cb(Component *, void *)
{
	if (!show_function)
		show_function = new Show_function_dialog(this);
	show_function->set_plist(this, PROCESS_LEVEL);
	show_function->display();
}

void
Source_window::show_line_cb(Component *, void *)
{
	if (!show_line)
		show_line = new Show_line_dialog(this);
	show_line->display();
}

void
Source_window::new_source_cb(Component *, void *)
{
	Source_window	*sw = new Source_window(window_set, FALSE);

	sw->popup();
	sw->set_file(current_file,current_path);
	sw->set_current_line(current_line);
	sw->get_pane()->position(pane->get_position(), FALSE);
}

void
Source_window::set_file(const char *file, const char *path)
{
	if (strcmp(file, current_file) == 0)
		return;

	int	*breaklist = 0;
	current_file = str(file);
	if (strcmp(current_file, window_set->current_process()->get_file()) == 0)
		current_line = window_set->current_process()->get_line();
	else
		current_line = 0;

	if (path)
	{
		delete current_path;
		current_path = makestr(path);
	}
	else
		(void) set_path(TRUE);

	if (!current_path)
	{
		clear();
		return;
	}

	pane->set_file(current_path);
	caption->set_label(current_path);
	if (current_line)
		pane->set_line(current_line);

	file_ptr = event_list.find_file(current_file, 0);
	pane->set_breaklist(file_ptr ? file_ptr->get_break_list(window_set->current_process()) : 0);
}

void
Source_window::select_cb(Text_display *, int line)
{
	selected_line = line;
	set_sensitivity();
}

Selection_type
Source_window::selection_type()
{
	if (selected_line)
		return SEL_src_line;
	return SEL_none;
}

char *
Source_window::get_selection()
{
	return pane->get_selection();
}

Boolean
Source_window::set_path(Boolean must_set)
{
	Message		*msg;
	Process		*cdata = window_set->current_process();
	Boolean		path_changed = must_set;
	const char	*ppath;

	if (!cdata || !current_file)
	{
		delete current_path;
		current_path = 0;
		return 1;
	}

	ppath = cdata->get_path();
	if (!must_set && current_file == cdata->get_file() && ppath)
	{
		if (strcmp(current_path, ppath) != 0)
		{
			delete current_path;
			current_path = makestr(ppath);
			path_changed = TRUE;
		}
	}
	else
	{
		dispatcher.query(this, cdata->get_id(), "ppath %s\n", current_file);
		while ((msg = dispatcher.get_response()) != 0)
		{
			if (msg->get_msg_id() == MSG_source_file)
			{
				char	*buf = 0;

				msg->unbundle(buf);
				if (strcmp(current_path, buf) != 0)
				{
					path_changed = TRUE;
					delete current_path;
					current_path = makestr(buf);
				}
			}
			else if (msg->get_msg_id() == ERR_no_source)
			{
				delete current_path;
				current_path = 0;
				window->clear_msg();
				window->display_msg(msg);
				path_changed = TRUE;
			}
			else
				display_msg(msg);
		}
	}
	return path_changed;
}
