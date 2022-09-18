#ident	"@(#)debugger:gui.d/common/Windows.C	1.31"

// GUI headers
#include "Boxes.h"
#include "Caption.h"
#include "Dialog_sh.h"
#include "Menu.h"
#include "Radio.h"
#include "Text_line.h"
#include "Text_area.h"
#include "Stext.h"
#include "Command.h"
#include "Context.h"
#include "Events.h"
#include "Source.h"
#include "Dis.h"
#include "Sch_dlg.h"
#include "Windows.h"
#include "Dispatcher.h"
#include "Proclist.h"
#include "Ps_pane.h"
#include "Stack_pane.h"
#include "Syms_pane.h"
#include "UI.h"

// Debug headers
#include "Machine.h"
#include "Message.h"
#include "Msgtab.h"
#include "Buffer.h"
#include "str.h"

#include <ctype.h>
#include <stdio.h>
#include <limits.h>

List		windows;

static int	next_window;
extern Buffer	buffer1;

// make sure selected text does not extend past a newline
static char *
truncate_selection(char *s)
{
	if (s)
	{
		char *p = strchr(s, '\n');
		if (p)
			*p = '\0';
	}
	return s;
}

// popup, popdown, selection_type, and check_sensitivity are pure virtual functions,
// but the actual functions are needed for cfront 1.2
void
Base_window::popup()
{
}

void
Base_window::popdown()
{
}

Selection_type
Base_window::selection_type()
{
	return SEL_none;
}

int
Base_window::check_sensitivity(int)
{
	return 1;
}

// Note that set_sensitivity is not recursive - currently only three
// levels are supported
void
Base_window::set_sensitivity()
{
	if (!_is_open)
		return;

	Menu	**menup = menu_bar->get_menus();
	int	total = menu_bar->get_nbuttons();
	int	i, j;

	for (i = 0; i < total; i++, menup++)
	{
		const Menu_table	*table = (*menup)->get_table();

		for (j = 0; j < (*menup)->get_nbuttons(); j++, table++)
			(*menup)->set_sensitive(j, is_sensitive(table->sensitivity));

		Menu *sub_menu = (*menup)->first_child();
		for ( ; sub_menu; sub_menu = (*menup)->next_child())
		{
			table = sub_menu->get_table();
			for (j = 0; j < sub_menu->get_nbuttons(); j++, table++)
				sub_menu->set_sensitive(j,
					is_sensitive(table->sensitivity));
		}
	}
}

int
Base_window::is_sensitive(int sense)
{
	if (sense == SEN_always)
		return 1;
	if (sense == SEN_invalid)
		return 0;

	return check_sensitivity(sense);
}

void
Base_window::de_message(Message *m)
{
	Msg_id	mtype = m->get_msg_id();

	if (Mtable.msg_class(mtype) == MSGCL_error)
		display_msg(m);
	else if (!gui_only_message(m))
		window->display_msg(m);
}
	
Window_set::Window_set() : COMMAND_SENDER(this), change_current(this),
	change_any(this)
{
	event_action = A_beep;
	output_action = A_raise;
	open_windows = 0;
	id = ++next_window;
	windows.add(this);

	head = tail = 0;
	cur_process = 0;

	contextwin = new Context_window(this);
	popup_context();

	commandwin = new Command_window(this);
	sourcewin = 0;
	disasmwin = 0;
	eventwin = 0;

	create_box = 0;
	recreate_box = 0;
	grab_process = 0;
	grab_core = 0;
	set_language = 0;
	set_value = 0;
	show_value = 0;
	run_box = 0;
	step_box = 0;
	jump_box = 0;
	stop_box = 0;
	signal_box = 0;
	syscall_box = 0;
	onstop_box = 0;
	cancel_box = 0;
	kill_box = 0;
	setup_signals = 0;
	path_box = 0;
	granularity_box = 0;
	cd_box = 0;
	search_box = 0;
	action_box = 0;

	event_level = PROGRAM_LEVEL;
	command_level = PROCESS_LEVEL;

	// update window menus in all other window_sets
	Window_set *ws;
	ws = (Window_set *)windows.first();
	while(ws != NULL)
	{
		if(ws != this)
		{
			ws->get_context_window()->add_ws(id);
			// update window menu for self
			contextwin->add_ws(ws->get_id());
		}
		ws = (Window_set *)windows.next();
	}
}

Window_set::~Window_set()
{
	if (head)
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	
	windows.remove(this);

	delete create_box;
	delete recreate_box;
	delete grab_process;
	delete grab_core;
	delete set_language;
	delete set_value;
	delete show_value;
	delete run_box;
	delete step_box;
	delete jump_box;
	delete stop_box;
	delete signal_box;
	delete syscall_box;
	delete onstop_box;
	delete cancel_box;
	delete kill_box;
	delete setup_signals;
	delete path_box;
	delete granularity_box;
	delete cd_box;
	delete search_box;
	delete action_box;

	delete contextwin;
	delete commandwin;
	delete sourcewin;
	delete eventwin;
	delete disasmwin;

	Source_window	*s =  (Source_window *)source_windows.first();
	for ( ; s; s = (Source_window *)source_windows.next())
		delete s;

	/* update window menus in all other window_sets */
	Window_set *ws;
	ws = (Window_set *)windows.first();
	while(ws != NULL)
	{
		ws->get_context_window()->delete_ws(id);
		ws = (Window_set *)windows.next();
	}
}

// Inform lets the user know when an event occurs or a process generates output,
// but inform is called for all messages, because it was easier to decide here
// which ones to handle than to clutter up the Dispatcher with that knowledge.
void
Window_set::inform(Message *m)
{
	Msg_id		mtype = m->get_msg_id();

	if (Mtable.msg_class(mtype) == MSGCL_error && !in_script)
	{
		// if the message has a uicontext - that is, the error is a result
		// of a command from a Command_sender object - the object will have
		// already handled the error.  The Dispatcher calls the object's
		// de_message before calling inform
		if (!m->get_uicontext())
			display_msg(m);
		return;
	}

	// gui-only messages are used to update state information and
	// are not displayed directly to the user
	if (gui_only_message(m))
		return;

	switch (mtype)
	{
	case ERR_cmd_pointer:	// pertinent only to cmd line
		break;

	case MSG_version:	// the result of pressing Version in the Help menu
		display_msg(m);
		break;

	// event notifications or other process state changes
	case MSG_proc_killed:
	case MSG_proc_fork:
	case MSG_proc_exec:
	case MSG_proc_exit:
	case MSG_es_suspend:
	case MSG_es_signal:
	case MSG_es_sysent:
	case MSG_es_sysxit:
	case MSG_es_stop:
	case MSG_es_stepped:
		if (!in_script)
			switch (event_action)
			{
			case A_message:	display_msg(m);		break;
			case A_beep:	beep();			break;
			case A_raise:	popup_command();	break;

			case A_none:
			default:
				break;
			}
		// in addition to the beep, etc., event notifications are
		// always displayed in the bottom of the window, and in
		// the command window
		//FALL-THROUGH

	case MSG_createp:
	case MSG_new_core:
	case MSG_grab_proc:
	case MSG_release_run:
	case MSG_release_suspend:
	case MSG_step_not_done:
		if (contextwin->is_open())
		{
			contextwin->get_window_shell()->clear_msg();
			contextwin->get_window_shell()->display_msg(m);
		}
		if (sourcewin->is_open())
		{
			sourcewin->get_window_shell()->clear_msg();
			sourcewin->get_window_shell()->display_msg(m);
		}
		if (eventwin->is_open())
		{
			eventwin->get_window_shell()->clear_msg();
			eventwin->get_window_shell()->display_msg(m);
		}
		if (disasmwin->is_open())
		{
			disasmwin->get_window_shell()->clear_msg();
			disasmwin->get_window_shell()->display_msg(m);
		}
		//FALL-THROUGH

	case MSG_loc_sym_file:
	case MSG_loc_sym:
	case MSG_loc_unknown:
	case MSG_line_src:
	case MSG_disassembly:
	case MSG_dis_line:
	case MSG_line_no_src:
	case MSG_es_core:
	case MSG_core_state:
	case MSG_core_state_addr:
		if (in_script || m->get_uicontext() != commandwin)
			commandwin->de_message(m);
		break;

	case MSG_proc_output:
		if (in_script)
			commandwin->de_message(m);
		else
		{
			if (m->get_uicontext() != commandwin)
				commandwin->de_message(m);
			switch (output_action)
			{
			case A_message:	display_msg(E_NONE, GM_output);	break;
			case A_beep:	beep();				break;
			case A_raise:	popup_command();		break;

			case A_none:
			default:
				break;
			}
		}
		break;

	default:
		if (in_script || !m->get_uicontext())
			commandwin->de_message(m);
		break;
	}
}

// set_current may be called with ptr == 0 from delete_process if the
// current process is being deleted.  In that case, set_current picks
// an arbitrary process off the list to be the new current process
void
Window_set::set_current(Process *ptr)
{
	if (!ptr)
	{
		for (Plink *link = head; link; link = link->next())
		{
			if (link->process()->get_state() != State_none)
			{
				ptr = link->process();
				break;
			}
		}
	}

	cur_process = ptr;
	contextwin->get_ps_pane()->set_current(ptr);
	if (in_script || has_assoc_cmd)
	{
		if (ptr)
			ptr->touch();
		else
			change_current.notify(RC_set_current, 0);
	}
	else
	{
		change_current.notify(RC_set_current, ptr);
		change_any.notify(RC_set_current, ptr);
	}
}

// delete_process removes a process from the window set.
void
Window_set::delete_process(Process *proc)
{
	Plink	*ptr;
	int	index;

	if (!proc || !(ptr = find_process(proc, index)))
		return;

	contextwin->get_ps_pane()->delete_process(index);
	if (ptr == head)
		head = head->next();
	if (ptr == tail)
		tail = tail->prev();
	ptr->unlink();
	delete ptr;

	if (proc == cur_process)
		cur_process = 0;
	change_any.notify(RC_delete, proc);
}

void
Window_set::add_process(Process *proc, int make_current)
{
	Plink	*ptr = new Plink(proc);
	int	index = 0;

	// assumes process name always looks like pnum
	long	id1 = strtol(proc->get_name()+1, NULL, 10);

	if (head)
	{	
		for (Plink *link = head; link; link = link->next(), index++)
		{
			Process	*p2 = link->process();
			long	id2 = strtol(p2->get_name()+1, NULL, 10);
			int	val = strcmp(proc->get_program()->get_name(),
						p2->get_program()->get_name());

			// programs names are ordered alphabetically,
			// processes are numeric
			if (val < 0 || (val == 0 && id1 < id2))
			{
				ptr->prepend(link);
				if (link == head)
					head = ptr;
				break;
			}
		}
		if (!link)
		{
			ptr->append(tail);
			tail = ptr;
		}
	}
	else
		head = tail = ptr;

	contextwin->get_ps_pane()->add_process(index, proc);
	if (make_current || !cur_process)
		set_current(proc);
}

void
Window_set::update_process(Process *proc)
{
	Plink	*ptr;
	int	index = 0;

	if (!proc || !(ptr = find_process(proc, index)))
		return;

	contextwin->get_ps_pane()->update_process(RC_change_state, index, proc);
	if (cur_process == proc && !in_script && !has_assoc_cmd)
		change_current.notify(RC_change_state, proc);
	if (!in_script && !has_assoc_cmd)
		change_any.notify(RC_change_state, proc);
}

// rename_process is called if renaming doesn't affect the process's ordering
// in the list, otherwise delete_process is called followed by add_process
void
Window_set::rename_process(Process *proc)
{
	Plink	*ptr;
	int	index = 0;

	if (!proc || !(ptr = find_process(proc, index)))
		return;

	contextwin->get_ps_pane()->update_process(RC_rename, index, proc);
	if (cur_process == proc)
		change_current.notify(RC_rename, proc);
	change_any.notify(RC_rename, proc);
}

void
Window_set::set_frame(Process *p)
{
	if (!in_script && !has_assoc_cmd)
	{
		if (p == cur_process)
			change_current.notify(RC_set_frame, p);
		change_any.notify(RC_set_frame, p);
	}
}

int
Window_set::get_frame(int frame, const char *&function, const char *&location)
{
	return contextwin->get_stack_pane()->get_frame(frame, function, location);
}

Plink *
Window_set::find_process(Process *proc, int &index)
{
	Plink	*ptr;

	for (ptr = head, index = 0; ptr; ptr = ptr->next(), index++)
	{
		if (ptr->process() == proc)
			break;
	}
	return ptr;
}

void
Window_set::popup_event()
{
	if (!eventwin)
		eventwin = new Event_window(this);
	if (!eventwin->is_open())
		open_windows++;
	eventwin->popup();
}

void
Window_set::popup_source()
{
	if (!sourcewin)
		sourcewin = new Source_window(this, TRUE);
	if (!sourcewin->is_open())
		open_windows++;
	sourcewin->popup();
}

void
Window_set::popup_disasm()
{
	if (!disasmwin)
		disasmwin = new Disasm_window(this);
	if (!disasmwin->is_open())
		open_windows++;
	disasmwin->popup();
}

// The context and command windows always exist - they are created along
// with the window set
void
Window_set::popup_context()
{
	if (!contextwin->is_open())
		open_windows++;
	contextwin->popup();
}

void
Window_set::popup_command()
{
	if (!commandwin->is_open())
		open_windows++;
	commandwin->popup();
}

// ok_to_quit is called if there are any live processes left.  If puts
// put a notice asking for confirmation
void
Window_set::ok_to_quit()
{
	if (proclist.any_live())
		display_msg((Callback_ptr)(&Window_set::quit_cb), this,
			"Quit", "Cancel", GE_ok_to_quit);
	else
		dispatcher.send_msg(this, 0, "quit\n");
}

// quit_cb is the callback from the confirmation notice.  quit_flag gives the
// user's response - 1 for "Quit", 0 for "Cancel".  If the user presses
// "Quit", send a message to debug.  The gui will exit when it gets the
// acknowledgement back
void
Window_set::quit_cb(Component *, int quit_flag)
{
	if (quit_flag)
		dispatcher.send_msg(this, 0, "quit\n");
}

// called when help button is pressed for a general description of
// a specific section e.g. Context, Command, Source, etc.
void
Window_set::help_sect_cb(Component *, Base_window *w)
{
	Window_shell *ws = w->get_window_shell();
	Help_id help = ws->get_help_msg();

	if(!help)
		return;
	display_help(ws->get_widget(), HM_section, help);
}

// called when help button is pressed for a table of contents
// for a specific section
void
Window_set::help_toc_cb(Component *, Base_window *w)
{
	Window_shell *ws = w->get_window_shell();
	Help_id help = ws->get_help_msg();

	if(!help)
		return;
	display_help(ws->get_widget(), HM_toc, help);
}

void
Window_set::dismiss(Component *, Base_window *w)
{
	if (open_windows > 1)
	{
		--open_windows;
		w->popdown();
	}
	else if (windows.first() == windows.last())
		ok_to_quit();
	else if (head)
		display_msg(E_ERROR, GE_cant_close);
	else	// last window in set with no active processes, destroy window set
	{
		w->popdown();
		delete this;
	}
}

void
Window_set::apply_to_process(Control_type ct, Base_window *win)
{
	const char	*cmd;
	const char	*plist;

	// gui processes are all run in the background, unless started
	// from a script or the command line
	switch (ct)
	{
	case CT_run:			cmd = "run -b";		break;
	case CT_return:			cmd = "run -b -r";	break;
	case CT_step_statement:		cmd = "step -b";	break;
	case CT_step_instruction:	cmd = "step -b -i";	break;
	case CT_next_statement:		cmd = "step -b -o";	break;
	case CT_next_instruction:	cmd = "step -b -o -i";	break;
	case CT_halt:			cmd = "halt";		break;
	case CT_release_running:	cmd = "release";	break;
	case CT_release_suspended:	cmd = "release -s";	break;
	}

	if (win->get_type() == BW_context && win->selection_type() == SEL_process)
	{
		Ps_pane	*ps = ((Context_window *)win)->get_ps_pane();
		plist = make_plist(ps->get_total(), ps->get_selections(), 0, command_level);
	}
	else
		plist = make_plist(1, &cur_process, 0, command_level);

	dispatcher.send_msg(this, cur_process->get_id(), "%s -p %s\n", cmd, plist);
}

// Callbacks for the command buttons in the Control menu
void
Window_set::run_button_cb(Component *, Base_window *win)
{
	apply_to_process(CT_run, win);
}

void
Window_set::run_r_button_cb(Component *, Base_window *win)
{
	apply_to_process(CT_return, win);
}

void
Window_set::step_button_cb(Component *, Base_window *win)
{
	apply_to_process(CT_step_statement, win);
}

void
Window_set::step_i_button_cb(Component *, Base_window *win)
{
	apply_to_process(CT_step_instruction, win);
}

void
Window_set::step_o_button_cb(Component *, Base_window *win)
{
	apply_to_process(CT_next_statement, win);
}

void
Window_set::step_oi_button_cb(Component *, Base_window *win)
{
	apply_to_process(CT_next_instruction, win);
}

void
Window_set::halt_button_cb(Component *, Base_window *win)
{
	apply_to_process(CT_halt, win);
}

void
Window_set::release_running_cb(Component *, Base_window *win)
{
	apply_to_process(CT_release_running, win);
}

void
Window_set::release_suspended_cb(Component *, Base_window *win)
{
	apply_to_process(CT_release_suspended, win);
}

// callbacks to bring up dialogs.  dialog objects are created as needed
void
Window_set::create_dialog_cb(Component *, Base_window *)
{
	if (!create_box)
		create_box = new Create_dialog(this);
	create_box->display();
}

void
Window_set::recreate_dialog_cb(Component *, Base_window *)
{
	if (!recreate_box)
		recreate_box = new Recreate_dialog(this);
	recreate_box->display();
}

// The GUI received MSG_create_args.  Recreate dialogs in all window sets
// are updated.
void
Window_set::update_recreate_dialog()
{
	if (recreate_box)
		recreate_box->set_create_args(create_args);
}

void
Window_set::grab_process_dialog_cb(Component *, Base_window *)
{
	if (grab_process)
		grab_process->setup();	// get current ps list
	else
		grab_process = new Grab_process_dialog(this);
	grab_process->display();
}

void
Window_set::grab_core_dialog_cb(Component *, Base_window *)
{
	if (!grab_core)
		grab_core = new Grab_core_dialog(this);
	grab_core->display();
}

void
Window_set::set_language_dialog_cb(Component *, Base_window *)
{
	if (!set_language)
		set_language = new Set_language_dialog(this);
	set_language->display();
}

// The GUI received MSG_set_language.  Set Language dialogs in all window sets
// are updated.
void
Window_set::update_language_dialog()
{
	if (set_language)
		set_language->reset();
}


// callback for the Change Directory... button in the File menu
void
Window_set::cd_dialog_cb(Component *, void *)
{
	if (!cd_box)
		cd_box = new Cd_dialog(this);
	cd_box->display();
}

// The GUI received MSG_cd.  Change Directory dialogs in all window sets
// are updated.
void
Window_set::update_cd_dialog(const char *s)
{
	if (cd_box)
		cd_box->update_directory(s);
}

// The expression field in the Set Value dialog is initialized with selected
// text from the Source or Disassembly windows or with a symbol selected in
// the Symbol Pane
void
Window_set::set_value_dialog_cb(Component *, Base_window *win)
{
	char	*expr = 0;

	if (!set_value)
		set_value = new Set_value_dialog(this);
	if (win->get_type() == BW_context && win->selection_type() == SEL_symbol)
	{
		const char	*sym = 0;

		// The sensitivity handling ensures that only one symbol is selected
		sym = (((Context_window *)win)->get_syms_pane()->get_selections())[0];
		set_value->set_expression(sym);
	}
	else if (win->get_type() == BW_source && win->selection_type() == SEL_src_line)
	{
		expr = ((Source_window *)win)->get_selection();
		set_value->set_expression(truncate_selection(expr));
	}
	else if (win->get_type() == BW_disasm && win->selection_type() != SEL_none)
	{
		// selection may be from either pane
		expr = ((Disasm_window*)win)->get_selection();
		set_value->set_expression(truncate_selection(expr));
	}

	set_value->set_plist(win, command_level);
	set_value->display();
}

// The expression field in the Show Value dialog is initialized with selected
// text from the Source or Disassembly windows or with multiple symbols from
// the Symbol Pane
void
Window_set::show_value_dialog_cb(Component *, Base_window *win)
{
	char		*text = 0;

	if (!show_value)
		show_value = new Show_value_dialog(this);

	if (win->get_type() == BW_context && win->selection_type() == SEL_symbol)
	{
		char	**syms = ((Context_window *)win)->get_syms_pane()->get_selections();
		int	total = ((Context_window *)win)->get_syms_pane()->get_total();
		buffer1.clear();
		for (int i = 0; i < total; i++)
		{
			if (i)
				buffer1.add(", ");
			buffer1.add(syms[i]);
		}
		text = (char *)buffer1;
	}
	else if (win->get_type() == BW_source && win->selection_type() == SEL_src_line)
		text = ((Source_window *)win)->get_selection();
	else if (win->get_type() == BW_disasm && win->selection_type() != SEL_none)
		text = ((Disasm_window*)win)->get_selection();

	if (text)
		show_value->set_expression(truncate_selection(text));
	show_value->set_plist(win, command_level);
	show_value->clear_result();
	show_value->display();
}

// The Run dialog is initialized with the location of the selected text
// in the Source or Disassembly windows
void
Window_set::run_dialog_cb(Component *, Base_window *win)
{
	const char	*plist = 0;

	if (!run_box)
		run_box = new Run_dialog(this);

	if (win->get_type() == BW_source && win->selection_type() == SEL_src_line)
	{
		char		buf[PATH_MAX+MAX_INT_DIGITS+2];
		Source_window	*sw = (Source_window *)win;

		sprintf(buf, "%s@%d", sw->get_current_file(), sw->get_selected_line());
		run_box->set_location(buf);
	}
	else if (win->get_type() == BW_disasm && win->selection_type() == SEL_instruction)
	{
		Disasm_window *dw = (Disasm_window *)win;

		run_box->set_location(dw->get_selected_addr());
	}

	run_box->set_plist(win, command_level);
	run_box->display();
}

void
Window_set::step_dialog_cb(Component *, Base_window *win)
{
	if (!step_box)
		step_box = new Step_dialog(this);
	step_box->set_plist(win, command_level);
	step_box->display();
}

// The Jump dialog is initialized with the location of the selected text
// in the Source or Disassembly windows
void
Window_set::jump_dialog_cb(Component *, Base_window *win)
{
	if (!jump_box)
		jump_box = new Jump_dialog(this);

	if (win->get_type() == BW_source && win->selection_type() == SEL_src_line)
	{
		char		buf[PATH_MAX+MAX_INT_DIGITS+2];
		Source_window	*sw = (Source_window *)win;
		sprintf(buf, "%s@%d", sw->get_current_file(),
			sw->get_selected_line());
		jump_box->set_location(buf);
	}
	else if (win->get_type() == BW_disasm && win->selection_type() == SEL_instruction)
	{
		Disasm_window *dw = (Disasm_window *)win;

		jump_box->set_location(dw->get_selected_addr());
	}

	jump_box->set_plist(win, command_level);
	jump_box->display();
}

void
Window_set::search_dialog_cb(Component *, Base_window *bw)
{
	char *s = 0;
	// search_box is shared by the source, and disasm
	if (!search_box)
		search_box = new Search_dialog(this);
	search_box->set_base_window(bw);

	if (bw->get_type() == BW_source && bw->selection_type() == SEL_src_line)
		search_box->set_string(truncate_selection(((Source_window*)bw)->get_selection()));
	else if (bw->get_type() == BW_disasm && bw->selection_type() != SEL_none)
		search_box->set_string(truncate_selection(((Disasm_window*)bw)->get_selection()));

	search_box->display();
}

// The Stop dialog is initialized with the location of the selected text
// in the Source or Disassembly windows
void
Window_set::stop_dialog_cb(Component *, Base_window *bw)
{
	// stop_box is shared with the Change command - reset so it looks like
	// a new event dialog again
	if (stop_box)
	{
		stop_box->setChange(0);
		stop_box->dialog->set_label("Stop", 'S');
		stop_box->reset(0,0);
	}
	else
		stop_box = new Stop_dialog(this);

	if (bw->get_type() == BW_source && bw->selection_type() == SEL_src_line)
	{
		char	buf[PATH_MAX+MAX_INT_DIGITS+2];
		Source_window	*sw = (Source_window *)bw;

		sprintf(buf, "%s@%d", sw->get_current_file(), sw->get_selected_line());
		stop_box->set_expression(buf);
	}
	else if (bw->get_type() == BW_disasm && bw->selection_type() == SEL_instruction)
	{
		Disasm_window *dw = (Disasm_window *)bw;

		stop_box->set_expression(dw->get_selected_addr());
	}

	stop_box->set_plist(bw, event_level);
	stop_box->display();
}

void
Window_set::signal_dialog_cb(Component *, Base_window *bw)
{
	// signal_box is shared with the Change command - reset so it looks like
	// a new event dialog again
	if (signal_box)
	{
		signal_box->setChange( 0 );
		signal_box->dialog->set_label("Signal", 'i');
		signal_box->reset(0,0);
	}
	else
		signal_box = new Signal_dialog(this);

	signal_box->set_plist(bw, event_level);
	signal_box->display();
}

void
Window_set::syscall_dialog_cb(Component *, Base_window *bw)
{
	// syscall_box is shared with the Change command - reset so it looks like
	// a new event dialog again
	if (syscall_box)
	{
		syscall_box->setChange( 0 );
		syscall_box->dialog->set_label("Syscall", 'y');
		syscall_box->reset(0,0);
	}
	else
		syscall_box = new Syscall_dialog(this);

	syscall_box->set_plist(bw, event_level);
	syscall_box->display();
}

void
Window_set::onstop_dialog_cb(Component *, Base_window *bw)
{
	// onstop_box is shared with the Change command - reset so it looks like
	// a new event dialog again
	if (onstop_box)
	{
		onstop_box->setChange( 0 );
		onstop_box->dialog->set_label("Onstop", 'O');
		onstop_box->reset(0,0);
	}
	else
		onstop_box = new Onstop_dialog(this);

	onstop_box->set_plist(bw, event_level);
	onstop_box->display();
}

void
Window_set::cancel_dialog_cb(Component *, Base_window *bw)
{
	if (!cancel_box)
		cancel_box = new Cancel_dialog(this);
	cancel_box->set_plist(bw, command_level);
	cancel_box->display();
}

void
Window_set::kill_dialog_cb(Component *, Base_window *bw)
{
	if (!kill_box)
		kill_box = new Kill_dialog(this);
	kill_box->set_plist(bw, command_level);
	kill_box->display();
}

void
Window_set::setup_signals_dialog_cb(Component *, Base_window *bw)
{
	if (!setup_signals)
		setup_signals = new Setup_signals_dialog(this);
	setup_signals->set_plist(bw, command_level);
	setup_signals->display();
}

void
Window_set::path_dialog_cb(Component *, Base_window *bw)
{
	if (!path_box)
		path_box = new Path_dialog(this);
	path_box->set_plist(bw, PROGRAM_LEVEL);
	path_box->display();
}

void
Window_set::set_granularity_cb(Component *, Base_window *)
{
	if (!granularity_box)
		granularity_box = new Granularity_dialog(this);
	granularity_box->display();
}

// callback for the Output Action... button
void
Window_set::action_dialog_cb(Component *, Base_window *)
{
	if (!action_box)
		action_box = new Action_dialog(this);
	action_box->display();
}

void
Window_set::version_cb(Component *, Base_window *)
{
	dispatcher.send_msg(this, 0, "version\n");
}

Set_value_dialog::Set_value_dialog(Window_set *ws) : PROCESS_DIALOG(ws)
{
	static const Button	buttons[] =
	{
		{ B_ok,	    "Set Value",  'S', (Callback_ptr)(&Set_value_dialog::apply) },
		{ B_cancel,  0,	0, (Callback_ptr)(&Set_value_dialog::cancel) },
		{ B_help,    0,	0, 0 },
	};

	Packed_box	*box;
	Caption		*caption;

	save_expr = 0;
	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
		"Set Value", (Callback_ptr)Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_set_value_dialog);

	box = new Packed_box(dialog, "set value box", OR_vertical);
	dialog->add_component(box);

	process_caption = new Caption(box, "Processes:", CAP_LEFT);
	process_list = new Simple_text(process_caption, "", TRUE);
	process_caption->add_component(process_list);
	box->add_component(process_caption);

	caption = new Caption(box, "Expression:", CAP_LEFT);
	expression = new Text_line(caption, "expression", "", 40, 1);
	caption->add_component(expression);
	box->add_component(caption);
}

void
Set_value_dialog::set_expression(const char *s)
{
	expression->set_text(s);
}

// Unlike most other Process_dialogs, the Set Value dialog does not
// require a current process or process selection (so the user may set
// debugger or user variables in an empty window set).
void
Set_value_dialog::apply(Component *, void *)
{
	char	*expr = expression->get_text();
	if (!expr || !*expr)
	{
		dialog->error(E_ERROR, GE_no_expression);
		return;
	}

	delete save_expr;
	save_expr = makestr(expr);

	if (processes)
		dispatcher.send_msg(this, processes[0]->get_id(),
			"set -p %s %s\n", make_plist(total, processes, 0, level), expr);
	else
		dispatcher.send_msg(this, 0, "set %s\n", expr);
	dialog->wait_for_response();
}

void
Set_value_dialog::cancel(Component *, void *)
{
	expression->set_text(save_expr);
}

void
Set_value_dialog::cmd_complete()
{
	Window_set*	win = get_window_set();
	Context_window	*context = win->get_context_window();
	Disasm_window	*dis = win->get_disasm_window();
	
	if (context->is_open())
		context->get_syms_pane()->update_cb(0, RC_set_current, 0,
			win->current_process());
	if (dis && dis->is_open())
		dis->disp_regs();
	dialog->cmd_complete();
}

// choices in list of formats
#define DEFAULT_FMT	0
#define DECIMAL_FMT	1
#define UNSIGNED_FMT	2
#define OCTAL_FMT	3
#define HEX_FMT		4
#define FLOAT_FMT	5
#define STRING_FMT	6
#define CHAR_FMT	7
#define USER_FMT	8

Show_value_dialog::Show_value_dialog(Window_set *ws) : PROCESS_DIALOG(ws)
{
	static const Button	buttons[] =
	{
		{ B_non_exec,	"Show Value",  'V',
				(Callback_ptr)(&Show_value_dialog::do_it) },
		{ B_close,  0, 0, 0 },
		{ B_help,   0, 0, 0 },
	};

	// *_FMT values must be kept in sync with this array
	static char	*choices[] =
	{
		"Default",
		"Decimal",
		"Unsigned",
		"Octal",
		"Hex",
		"Float",
		"String",
		"Character",
		"Other",
	};

	Expansion_box	*box;
	Expansion_box	*box2;
	Caption		*caption;

	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
		"Show Value", (Callback_ptr)Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_show_value_dialog);
	box = new Expansion_box(dialog, "show value", OR_vertical);
	dialog->add_component(box);

	process_caption = new Caption(box, "Processes:", CAP_LEFT);
	process_list = new Simple_text(process_caption, "", TRUE);
	process_caption->add_component(process_list);
	box->add_component(process_caption);

	caption = new Caption(box, "Expression:", CAP_LEFT);
	expression = new Text_line(caption, "expression", "", 25, 1);
	caption->add_component(expression);
	box->add_component(caption);

	box2 = new Expansion_box(box, "show value", OR_horizontal);
	box->add_elastic(box2);

	caption = new Caption(box2, "Format", CAP_TOP_CENTER);
	formats = new Radio_list(caption, "formats", OR_vertical, choices,
		sizeof(choices)/sizeof(char *), 0,
		(Callback_ptr)Show_value_dialog::set_format_type, this);
	caption->add_component(formats, FALSE);
	box2->add_component(caption);

	caption = new Caption(box2, "Result of Evaluated Expression:", CAP_TOP_LEFT);
	result = new Text_area(caption, "result", 8, 30);
	caption->add_component(result);
	box2->add_elastic(caption);

	caption = new Caption(box, "Specify format:", CAP_LEFT);
	format_line = new Text_line(caption, "format", "", 20, FALSE);
	caption->add_component(format_line);
	box->add_component(caption);
}

void
Show_value_dialog::clear_result()
{
	result->clear();
	dialog->set_focus(expression);
}

void
Show_value_dialog::set_expression(const char *s)
{
	expression->set_text(s);
}

// Unlike most other Process_dialogs, the Show Value dialog does not
// require a current process or process selection (so the use may view
// debugger or user variables in an empty window set)
void
Show_value_dialog::do_it(Component *, void *)
{
	const char	*fmt;
	char		*expr = expression->get_text();

	dialog->clear_msg();
	if (!expr || !*expr)
	{
		dialog->error(E_ERROR, GE_no_expression);
		return;
	}
	if ((fmt = get_format()) == (char *)-1)
		return;

	result->clear();
	if (fmt && processes)
	{
		dispatcher.send_msg(this, processes[0]->get_id(),
			"print -f\"%s\\n\" -p %s %s\n", fmt,
			make_plist(total, processes, 0, level), expr);
	}
	else if (fmt)
	{
		dispatcher.send_msg(this, 0, "print -f\"%s\\n\" %s\n", fmt, expr);
	}
	else if (processes)
	{
		dispatcher.send_msg(this, processes[0]->get_id(),
			"print -p %s %s\n", make_plist(total, processes, 0, level),
			expr);
	}
	else
	{
		dispatcher.send_msg(this, 0, "print %s\n", expr);
	}
	dialog->wait_for_response();
}

const char *
Show_value_dialog::get_format()
{
	static char	*buf;
	static size_t	maxlen;
	char		*fmt;
	char		*p1;
	char		*p2;
	size_t		len;

	switch (formats->which_button())
	{
	case DECIMAL_FMT:		return "%d";
	case UNSIGNED_FMT:		return "%u";
	case OCTAL_FMT:			return "%#o";
	case HEX_FMT:			return "%#x";
	case FLOAT_FMT:			return "%g";
	case STRING_FMT:		return "%s";
	case CHAR_FMT:			return "%c";

	case USER_FMT:
		fmt = format_line->get_text();
		if (!fmt || !*fmt)
		{
			dialog->error(E_ERROR, GE_no_format);
			return (char *)-1;
		}

		len = strlen(fmt)+1;
		if (len*2 > maxlen)
		{
			delete buf;
			maxlen = len*2 + 40;	// give some room to grow
			buf = new char[maxlen];
		}

		for (p1 = fmt, p2 = buf; *p1; p1++)
		{
			if (*p1 == '"')
				*p2++ = '\\';
			*p2++ = *p1;
		}
		*p2 = '\0';
		return buf;

	case DEFAULT_FMT:
	default:	return 0;
	}
}

void
Show_value_dialog::de_message(Message *m)
{
	Msg_id	mtype = m->get_msg_id();

	if (Mtable.msg_class(mtype) == MSGCL_error)
	{
		if (mtype == ERR_cmd_pointer || mtype == ERR_asis)
			return;

		if (mtype == ERR_syntax_loc)
			dialog->error(E_ERROR, GE_syntax_error);
		else
			dialog->error(m);
	}
	else if (!gui_only_message(m))
		result->add_text(m->format());
}

// scroll back to the top of the text area, in case the result is
// bigger than the available space
void
Show_value_dialog::cmd_complete()
{
	result->position(1);
	dialog->cmd_complete();
}

void
Show_value_dialog::set_format_type(Component *, void *)
{
	if (formats->which_button() == USER_FMT)
	{
		format_line->set_editable(TRUE);
		dialog->set_focus(format_line);
	}
	else
	{
		format_line->set_editable(FALSE);
		dialog->set_focus(expression);
	}
}

// values correspond to radio button number
#define GLOBAL_PATH	0
#define PROGRAM_PATH	1

Path_dialog::Path_dialog(Window_set *ws) : PROCESS_DIALOG(ws)
{
	static const Button	buttons[] =
	{
		{ B_apply,	0, 0, (Callback_ptr)(&Path_dialog::apply) },
		{ B_reset,	0, 0, (Callback_ptr)(&Path_dialog::reset) },
		{ B_cancel,	0, 0, (Callback_ptr)(&Path_dialog::reset) },
		{ B_help,	0, 0, 0 },
	};

	// this array must be kept in sync with *_PATH values
	static char *radio_buttons[] =
	{
		"Global path",
		"Program-specific path",
	};

	Expansion_box	*box;
	Caption		*caption;

	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
		"Source Path", (Callback_ptr)Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_path_dialog);
	box = new Expansion_box(dialog, "path box", OR_vertical);

	process_caption = new Caption(box, "Program:", CAP_LEFT);
	process_list = new Simple_text(process_caption, "", TRUE);
	process_caption->add_component(process_list);
	box->add_component(process_caption);

	state = GLOBAL_PATH;
	caption = new Caption(box, "Set:", CAP_LEFT);
	choice = new Radio_list(caption, "path type", OR_vertical, radio_buttons,
		sizeof(radio_buttons)/sizeof(char *), 0,
		(Callback_ptr)Path_dialog::set_path_type, this);
	caption->add_component(choice, FALSE);
	box->add_component(caption);

	path_area = new Text_area(box, "path", 10, 20);
	box->add_elastic(path_area);
	dialog->add_component(box);
}

void
Path_dialog::apply(Component *, void *)
{
	state = choice->which_button();
	if ((state == PROGRAM_PATH) && !processes)
	{
		dialog->error(E_ERROR, GE_no_process);
		return;
	}

	char	*buf = path_area->get_text();

	if (!buf)
		buf = "";

	for (char *ptr = buf; *ptr; ptr++)
	{
		if (isspace(*ptr))
			*ptr = ':';
	}

	dispatcher.send_msg(this, processes ? processes[0]->get_id() : 0,
		"set %%%s = \"%s\"\n", state ? "path" : "global_path", buf);
	dialog->wait_for_response();
}

void
Path_dialog::set_path_type(Radio_list *radio, int new_state)
{
	Message	*msg;
	char	*buf = 0;

	if (new_state == PROGRAM_PATH && !processes)
	{
		choice->set_button(0);
		dialog->error(E_ERROR, GE_no_process);
		return;
	}

	if (!radio) // was called from reset(), rather than radio button callback
		choice->set_button(new_state);

	dispatcher.query(this, processes ? processes[0]->get_id() : 0,
		"print %%%s\n", new_state ? "path" : "global_path");
	while ((msg = dispatcher.get_response()) != 0)
	{
		if (msg->get_msg_id() == MSG_print_val)
			msg->unbundle(buf);
		else
			display_msg(msg);
	}

	path_area->clear();
	if (!buf)
		return;

	if (*buf == '"')
		buf++;
	for (char *ptr = buf; *ptr; ptr++)
	{
		if (*ptr == ':')
			*ptr = '\n';
		else if (*ptr == '"')
			*ptr = '\0';
	}
	path_area->add_text(buf);
}

void
Path_dialog::reset(Component *, void *)
{
	set_path_type(0, state);
}

void
Path_dialog::set_process(Boolean)
{
	if (processes)
	{
		state = choice->which_button();
	}
	else
	{
		state = GLOBAL_PATH;
		choice->set_button(state);
	}
	set_path_type(0, state);
}

// values correspond to entries in radio button list
#define	PROCESS_GRANULARITY	0
#define PROGRAM_GRANULARITY	1

Granularity_dialog::Granularity_dialog(Window_set *ws) : DIALOG_BOX(ws)
{
	static const Button	buttons[] =
	{
		{ B_apply,	0, 0, (Callback_ptr)(&Granularity_dialog::apply) },
		{ B_reset,	0, 0, (Callback_ptr)(&Granularity_dialog::reset) },
		{ B_cancel,	0, 0, (Callback_ptr)(&Granularity_dialog::reset) },
		{ B_help,	0, 0, 0 },
	};

	// this array must be kept in sync with *_GRANULARITY values
	static char *radio_buttons[] =
	{
		"Process Only",
		"Parent Program",
	};

	Packed_box	*box;
	Caption		*caption;

	event_state = PROGRAM_GRANULARITY;
	command_state = PROCESS_GRANULARITY;

	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
		"Granularity", 0, this, buttons,
		sizeof(buttons)/sizeof(Button), HELP_granularity_dialog);
	box = new Packed_box(dialog, "granularity box", OR_vertical);

	caption = new Caption(box, "Events apply to:", CAP_LEFT);
	event_choice = new Radio_list(caption, "event buttons", OR_vertical, radio_buttons,
		sizeof(radio_buttons)/sizeof(char *), 1, 0, this);
	caption->add_component(event_choice);
	box->add_component(caption);

	caption = new Caption(box, "Other Commands apply to:", CAP_LEFT);
	command_choice = new Radio_list(caption, "command buttons", OR_vertical,
		radio_buttons, sizeof(radio_buttons)/sizeof(char *), 0, 0, this);
	caption->add_component(command_choice);
	box->add_component(caption);

	dialog->add_component(box);
}

void
Granularity_dialog::apply(Component *, void *)
{
	event_state = event_choice->which_button();
	command_state = command_choice->which_button();

	window_set->set_event_level((event_state == PROCESS_GRANULARITY)
		? PROCESS_LEVEL : PROGRAM_LEVEL);
	window_set->set_command_level((command_state == PROCESS_GRANULARITY)
		? PROCESS_LEVEL : PROGRAM_LEVEL);
}

void
Granularity_dialog::reset(Component *, void *)
{
	event_choice->set_button(event_state);
	command_choice->set_button(command_state);
}

Action_dialog::Action_dialog(Window_set *ws) : DIALOG_BOX(ws)
{
	static const Button	buttons[] =
	{
		{ B_apply,   0, 0, (Callback_ptr)Action_dialog::apply },
		{ B_reset,   0, 0, (Callback_ptr)Action_dialog::reset },
		{ B_cancel,  0,	0, (Callback_ptr)Action_dialog::reset },
		{ B_help,    0,	0, 0 },
	};

	static const char *choices[] =
	{
		"Open and Raise",
		"Beep",
		"Alert box",
		"No action",
	};

	Packed_box	*box;
	Caption		*caption;

	dialog = new Dialog_shell(window_set->get_context_window()->get_window_shell(),
		"Output Action", 0, this, buttons, sizeof(buttons)/sizeof(Button),
		HELP_action_dialog);
	box = new Packed_box(dialog, "box", OR_vertical);
	dialog->add_component(box);

	caption = new Caption(box, "Event action:", CAP_LEFT);
	event_action = new Radio_list(caption, "event action", OR_vertical,
		choices, sizeof(choices)/sizeof(char *),
		(int)window_set->get_event_action());
	caption->add_component(event_action);
	box->add_component(caption);

	caption = new Caption(box, "Output action:", CAP_LEFT);
	output_action = new Radio_list(caption, "output action", OR_vertical,
		choices, sizeof(choices)/sizeof(char *),
		(int)window_set->get_output_action());
	caption->add_component(output_action);
	box->add_component(caption);
}

void
Action_dialog::apply(Component *, void *)
{
	window_set->set_event_action((Action)event_action->which_button());
	window_set->set_output_action((Action)output_action->which_button());
}

void
Action_dialog::reset(Component *, void *)
{
	event_action->set_button(window_set->get_event_action());
	output_action->set_button(window_set->get_output_action());
}
