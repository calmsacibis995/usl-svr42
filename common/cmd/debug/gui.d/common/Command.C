#ident	"@(#)debugger:gui.d/common/Command.C	1.20"

// GUI headers
#include "Dialog_sh.h"
#include "Command.h"
#include "Dispatcher.h"
#include "Dialogs.h"
#include "Windows.h"
#include "Menu.h"
#include "Caption.h"
#include "Radio.h"
#include "Text_area.h"
#include "Text_line.h"
#include "Stext.h"
#include "Toggle.h"
#include "Boxes.h"
#include "Proclist.h"
#include "Status.h"
#include "Window_sh.h"
#include "FileInfo.h"

// Debug headers
#include "Message.h"
#include "Vector.h"
#include "str.h"

#include <unistd.h>
#include <signal.h>

// The script dialog lets the user enter the path name of a script file
// (a text file containing debugger commands).  The script is executed
// when the default button is pushed

class Script_dialog : public Dialog_box
{
	Toggle_button	*echo;	// echo commands when executing script, if true
	Text_line	*file_name;	// user-supplied path name
	char		*save_name;	// save last file name for Cancel operation
	Boolean		echo_state;	// save last state of toggle for Cancel
public:
			Script_dialog(Command_window *);
			~Script_dialog() { delete save_name; }

			// button callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);
			// drag-n-drop callback
	void		drop_cb(Component *, void *);
};

Script_dialog::Script_dialog(Command_window *cw) : DIALOG_BOX(cw->get_window_set())
{
	static const Button	buttons[] =
	{
		{ B_ok,	    "Script",  'S', (Callback_ptr)Script_dialog::apply },
		{ B_cancel,  0,	0,	    (Callback_ptr)Script_dialog::cancel },
		{ B_help,    0,	0, 0 },
	};

	static Toggle_data toggle = { "Echo Commands", TRUE, 0 };

	Packed_box	*box;
	Caption		*caption;

	save_name = 0;
	echo_state = TRUE;

	dialog = new Dialog_shell(cw->get_window_shell(), "Script", 0,
		this, buttons, sizeof(buttons)/sizeof(Button),
		HELP_script_dialog, (Callback_ptr)&Script_dialog::drop_cb, 
		Drop_cb_popdown);
	box = new Packed_box(dialog, "", OR_vertical);
	dialog->add_component(box);

	caption = new Caption(box, "Script File:", CAP_LEFT);
	file_name = new Text_line(caption, "script file", "", 25, 1);
	caption->add_component(file_name);
	box->add_component(caption);

	echo = new Toggle_button(box, "echo", &toggle, 1, OR_vertical, this);
	box->add_component(echo);
}

void
Script_dialog::drop_cb(Component *comp, void *arg)
{
	char *s = dialog->get_drop_item();
	if(s)
	{
		FileInfo file(s);

		switch(file.type())
		{
		case FT_TEXT:
			/* apply */
			file_name->set_text(s);
			apply(comp, arg);
			break;
		default:
			dialog->error(E_ERROR, GE_bad_drop);
			return;
		}
	}
}

void
Script_dialog::apply(Component *, void *)
{
	char		*s;

	s = file_name->get_text();
	if (!s || !*s)
	{
		dialog->error(E_ERROR, GE_no_file);
		return;
	}

	delete save_name;
	save_name = makestr(s);

	echo_state = echo->is_set(0);
	dispatcher.send_msg(this, window_set->current_process()->get_id(),
		 "script %s %s\n", echo_state ? "" : "-q", s);
	dialog->wait_for_response();
}

void
Script_dialog::cancel(Component *, void *)
{
	file_name->set_text(save_name);
	echo->set(0, echo_state);
}

// The Input dialog lets the user give input to a process whose I/O is
// being captured by the debugger.  The input command accepts one line 
// (or part of a line) at a time.

class Input_dialog : public Process_dialog
{
	Toggle_button	*newline;	// newline suppressed if false
	Text_line	*input;		// user-supplied input string
	char		*save_string;	// save contents of input for Cancel
	Boolean		add_newline;	// save state of toggle for Cancel operation

public:
			Input_dialog(Command_window *);
			~Input_dialog() { delete save_string; }

			// Button callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);
};

Input_dialog::Input_dialog(Command_window *cw) : PROCESS_DIALOG(cw->get_window_set())
{
	static const Button	buttons[] =
	{
		{ B_ok,	    "Input",  'I', (Callback_ptr)Input_dialog::apply },
		{ B_cancel,  0,	0, (Callback_ptr)Input_dialog::cancel },
		{ B_help,    0,	0, 0 },
	};

	static Toggle_data	toggle = { "Append Newline", TRUE, 0 };

	Caption		*caption;
	Packed_box	*box;

	save_string = 0;
	add_newline = TRUE;

	dialog = new Dialog_shell(cw->get_window_shell(), "Input",
		(Callback_ptr)Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_input_dialog);
	box = new Packed_box(dialog, "", OR_vertical);
	dialog->add_component(box);

	process_caption = new Caption(box, "Program:", CAP_LEFT);
	process_list = new Simple_text(process_caption, "", TRUE);
	process_caption->add_component(process_list);
	box->add_component(process_caption);

	caption = new Caption(box, "Input:", CAP_LEFT);
	input = new Text_line(caption, "input", "", 25, 1);
	caption->add_component(input);
	box->add_component(caption);

	newline = new Toggle_button(box, "newline", &toggle, 1, OR_vertical, this);
	box->add_component(newline);
}

void
Input_dialog::apply(Component *, void *)
{
	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	delete save_string;
	save_string = makestr(input->get_text());
	window_set->get_command_window()->get_transcript()->add_text(save_string);
	add_newline = newline->is_set(0);
	if (add_newline)
		window_set->get_command_window()->get_transcript()->add_text("\n");

	dispatcher.send_msg(this, processes[0]->get_id(),
		"input %s \"%s\"\n", add_newline ? "" : "-n", save_string);
	dialog->wait_for_response();
}

void
Input_dialog::cancel(Component *, void *)
{
	input->set_text(save_string);
	newline->set(0, add_newline);
}

static const Menu_table windows_pane[] =
{
	{ "Context",	 'C', Set_cb,	SEN_always,	
				(Callback_ptr)(&Window_set::popup_context) },
	{ "Source",	 'S', Set_cb,	SEN_process,	
				(Callback_ptr)(&Window_set::popup_source) },
	{ "Disassembly", 'D', Set_cb,	SEN_process,	
				(Callback_ptr)(&Window_set::popup_disasm) },
	{ "Events",	 'E', Set_cb,	SEN_process|SEN_proc_live,	
				(Callback_ptr)(&Window_set::popup_event) },
	{ "Command",	 'o', Set_cb,	SEN_always,	
				(Callback_ptr)(&Window_set::popup_command) },
};

static const Menu_table file_pane[] =
{
	{ "Change Directory...", 'C', Set_cb, SEN_always,
				(Callback_ptr)(&Window_set::cd_dialog_cb),
				HELP_cmd_cd_dialog },
	{ "Script...",	'S', Window_cb,	SEN_always,	
				(Callback_ptr)(&Command_window::script_dialog_cb),
				HELP_cmd_script_dialog},
	{ "Windows",	'W', Menu_button, SEN_always,
				(Callback_ptr)windows_pane, HELP_cmd_windows_cmd,
				sizeof(windows_pane)/sizeof(Menu_table) },
	{ DISMISS, DISMISS_MNE, Set_cb,	SEN_always,	
				(Callback_ptr)(&Window_set::dismiss),
				HELP_cmd_dismiss_cmd },
	{ "Quit",	'Q', Set_cb,  SEN_always,	
				(Callback_ptr)(&Window_set::ok_to_quit),
				HELP_cmd_quit_cmd },
};

static const Menu_table edit_pane[] =
{
	{ "Copy",	'C', Window_cb,	SEN_sel_required,	
			(Callback_ptr)(&Command_window::copy_cb),
			HELP_cmd_copy_cmd },
	{ "Input...",	'I', Window_cb,
			(SEN_process|SEN_proc_live|SEN_proc_io_redirected),	
			(Callback_ptr)(&Command_window::input_dialog_cb),
			HELP_cmd_input_dialog },
	{ "Interrupt",	't', Window_cb, SEN_always,
			(Callback_ptr)(&Command_window::interrupt_cb),
			HELP_interrupt_cmd },
};

static const Menu_table prop_pane[] =
{
	{ "Output Action...",	'O', Set_cb,	SEN_always,	
			(Callback_ptr)(&Window_set::action_dialog_cb),
			HELP_cmd_action_dialog },
	{ "Source Path...",		'P', Set_cb,	SEN_always,
			(Callback_ptr)(&Window_set::path_dialog_cb),
			HELP_cmd_path_dialog },
	{ "Language...",	'L', Set_cb,	SEN_always,	
			(Callback_ptr)(&Window_set::set_language_dialog_cb),
			HELP_cmd_language_dialog },
};

// The number of entries in the help menu is hard-coded because of a
// bug in cfront 2.1
static const Menu_table cmd_help_pane[3] =
{
	{ "Command...", 'C', Set_cb, SEN_always,
				(Callback_ptr)(&Window_set::help_sect_cb) },
	{ "Table of Contents...", 'T', Set_cb, SEN_always,
				(Callback_ptr)(&Window_set::help_toc_cb) },
	{ "Version",	'V',	Set_cb,	SEN_always,
				(Callback_ptr)(&Window_set::version_cb) },
};

static Menu_bar_table menu_table[] =
{
    { "File",     file_pane,	'F', sizeof(file_pane)/sizeof(Menu_table),
					HELP_cmd_file_menu },
    { "Edit",     edit_pane,	'E', sizeof(edit_pane)/sizeof(Menu_table),
					HELP_cmd_edit_menu },
    { "Properties",prop_pane,	'P', sizeof(prop_pane)/sizeof(Menu_table),
					HELP_cmd_properties_menu },
    { "Help",	  cmd_help_pane,'H', sizeof(cmd_help_pane)/sizeof(Menu_table) },
};

Command_window::Command_window(Window_set *ws) : BASE_WINDOW(ws, BW_command)
{
	Expansion_box	*box;
	Caption		*caption;
	int		table_size = sizeof(menu_table)/sizeof(Menu_bar_table);

	has_selection = FALSE;
	in_continuation = FALSE;
	script_box = 0;
	input_box = 0;

	window = new Window_shell("Command", 0, this, HELP_command_window);
	box = new Expansion_box(window, "", OR_vertical);
	window->add_component(box);

	menu_bar = new Menu_bar(box, this, ws, menu_table, table_size);
	box->add_component(menu_bar);

	status_pane = new Status_pane(box);
	transcript = new Text_area(box, "transcript", 10, 80,
		(Callback_ptr)Command_window::select_cb, this,
		HELP_transcript_pane);
	box->add_elastic(transcript);

	caption = new Caption(box, prompt, CAP_LEFT);
	command = new Text_line(caption, "command line", "", 73, 1,
		(Callback_ptr)Command_window::do_command, 0, 0, this,
		HELP_command_line);
	caption->add_component(command);
	box->add_component(caption);

	// the focus would be in the Transcript pane otherwise
	window->set_focus(command);
}

Command_window::~Command_window()
{
	delete script_box;
	delete input_box;
	delete window;
	delete status_pane;
}

void
Command_window::de_message(Message *m)
{
	// gui-only messages are used to update state information and
	// are not displayed directly to the user
	if (gui_only_message(m) || m->get_msg_id() == ERR_cmd_pointer)
		return;

	if (m->get_msg_id() == MSG_proc_output)
	{
		char *s1, *s2;

		// don't display the pty id
		m->unbundle(s1, s2);
		transcript->add_text(s2);
	}
	else
		transcript->add_text(m->format());
}

void
Command_window::cmd_complete()
{
	command->clear();
}

// callback executed when the user types return in the command line
void
Command_window::do_command(Component *, const char *s)
{
	if (!in_continuation)
		transcript->add_text(prompt);
	transcript->add_text(s);
	transcript->add_text("\n");

	if (!s || !*s)
		return;

	size_t	len = strlen(s);
	if (s[len-1] == '\\')
	{
		if (!in_continuation)
		{
			in_continuation = TRUE;
			vscratch1.clear();
		}
		vscratch1.add((void *)s, len-1);
		command->clear();
		transcript->add_text(">");
	}
	else if (in_continuation)
	{
		in_continuation = FALSE;
		vscratch1.add("", 1);
		dispatcher.send_msg(this, window_set->current_process()->get_id(),
			"%s%s\n", vscratch1.ptr(), s);
	}
	else
		dispatcher.send_msg(this, window_set->current_process()->get_id(),
			"%s\n", s);
}

// callback for the Script... button in the File menu
void
Command_window::script_dialog_cb(Component *, void *)
{
	if (!script_box)
		script_box = new Script_dialog(this);
	script_box->display();
}

// callback for the Input... button in the Edit menu
void
Command_window::input_dialog_cb(Component *, void *)
{
	if (!input_box)
		input_box = new Input_dialog(this);
	input_box->set_plist(this, PROGRAM_LEVEL);
	input_box->display();
}

// update_cb is called whenever the process changes state
void
Command_window::update_cb(void *, Reason_code, void *, Process *proc)
{
	set_sensitivity();
	status_pane->update(proc);
}

// bring the window up or to the front, and register the update callback
// so the status pane will be kept up to date
void
Command_window::popup()
{
	if (_is_open) {
		window->raise();
		return;
	}

	_is_open = 1;
	window->popup();
	update_cb(0, RC_set_current, 0, window_set->current_process());
	window_set->change_current.add(this, (Notify_func)Command_window::update_cb, 0);
}

// popdown, and remove the callback, to avoid unnecessary overhead when
// the window is not visible
void
Command_window::popdown()
{
	_is_open = 0;
	window->popdown();
	window_set->change_current.remove(this, (Notify_func)Command_window::update_cb, 0);
}

int
Command_window::check_sensitivity(int sense)
{
	if ((sense&SEN_sel_required) && !has_selection)
		return 0;
	if (sense & SEN_process)
	{
		if (!window_set->current_process())
			return 0;
		return window_set->current_process()->check_sensitivity(sense);
	}
	return 1;
}

// callback for text selection and deselection in the transcript pane
void
Command_window::select_cb(Text_area *, int selection)
{
	has_selection = (selection != 0);
	set_sensitivity();
}

// callback for the Copy button - copy the text selection in the
// transcript pane to the clipboard
void
Command_window::copy_cb(Text_area *, void *)
{
	transcript->copy_selection();
}

void
Command_window::interrupt_cb(Component *, void *)
{
	kill(getppid(), SIGINT);
}
