#ident	"@(#)debugger:gui.d/common/Dis.C	1.32"

// GUI headers
#include "Dialogs.h"
#include "Sch_dlg.h"
#include "Dialog_sh.h"
#include "Dispatcher.h"
#include "Eventlist.h"
#include "Dis.h"
#include "Status.h"
#include "Windows.h"
#include "Window_sh.h"
#include "Menu.h"
#include "Text_disp.h"
#include "Text_line.h"
#include "Stext.h"
#include "Sel_list.h"
#include "Boxes.h"
#include "Caption.h"
#include "Proclist.h"
#include "UI.h"

// Debug headers
#include "Message.h"
#include "Vector.h"
#include "Buffer.h"
#include "Machine.h"
#include "str.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

// DispLine maps addresses and source line numbers (if available) to lines
// in the Disassembly pane - there will be one entry in an array of DispLines
// for each line of disassembly
class DispLine
{
	int	srcLine;
	Iaddr	addr;
public :
		DispLine(Iaddr a, int line = 0){ srcLine = line; addr = a;}
		~DispLine(){};

	Iaddr	getAddr(){return addr;}
	int	getLine(){return srcLine;}
};

class Show_loc_dialog : public Dialog_box
{
	Disasm_window	*disasm;
	Text_line	*line;
	char		*save_string;	// saved state for cancel callback

public:
			Show_loc_dialog(Disasm_window *);
			~Show_loc_dialog(){ delete save_string; }

			// callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);

	void		set_string(const char *s){ line->set_text(s); }

};

Show_loc_dialog::Show_loc_dialog(Disasm_window *dw)
	: DIALOG_BOX(dw->get_window_set())
{
	static const Button	buttons[] =
	{
		{ B_apply,	"Show Location", 'S',
				(Callback_ptr)(&Show_loc_dialog::apply) },
		{ B_cancel,	0, 0, (Callback_ptr)(&Show_loc_dialog::cancel) },
		{ B_help,	0, 0, 0 },
	};

	Caption		*caption;

	save_string = 0;
	disasm = dw;

	dialog = new Dialog_shell(dw->get_window_shell(), "Show Location", 0, this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_dis_show_location_dialog);

	caption = new Caption(dialog, "Location:", CAP_LEFT);
	line = new Text_line(caption, "loc", "", 15, 1);

	caption->add_component(line, FALSE);
	dialog->add_component(caption);
}

void
Show_loc_dialog::cancel(Component *, void *)
{
	line->set_text(save_string);
}

void
Show_loc_dialog::apply(Component *, void *)
{
	char	*s = line->get_text();

	if (!s || !*s)
	{
		dialog->error(E_ERROR, GE_no_number);
		return;
	}

	delete save_string;
	save_string = makestr(s);

	char	*endP = 0;
	Iaddr	retLong = strtoul(s, &endP, 0);

	// ask the Disassembly window to disassemble the code
	if (retLong != ULONG_MAX && endP != 0 && *endP == '\0') 
		disasm->disasm_addr(retLong, s);
	else
	{
		dialog->error(E_ERROR, GE_invalid_addr);
		return;
	}
}

class Function_dialog : public Process_dialog
{
	Disasm_window	*disasm;
	Selection_list	*functions;
	Selection_list	*objects;

			// saved state for cancel callback
	int		obj_selection;
	int		f_selection;
public:
			Function_dialog(Disasm_window *);
			~Function_dialog()	{}

			// callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);
	void		select_cb(Selection_list *, int);
	void		drop_cb(Selection_list *, Component *);

			// inherited from Process_dialog
	void		set_process(Boolean reset);
};

Function_dialog::Function_dialog(Disasm_window *dw)
	: PROCESS_DIALOG(dw->get_window_set())
{
	static const Button	buttons[] =
	{
		{ B_apply,	"Show Function", 'S', (Callback_ptr)(&Function_dialog::apply) },
		{ B_cancel,	0, 0, (Callback_ptr)(&Function_dialog::cancel) },
		{ B_help,	0, 0, 0 },
	};

	Expansion_box	*box;
	Caption		*caption;

	obj_selection = -1;
	f_selection = -1;
	disasm = dw;

	dialog = new Dialog_shell(dw->get_window_shell(), "Show Function",
		(Callback_ptr)(&Process_dialog::dismiss_cb), this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_show_function_dialog_2);
	box = new Expansion_box(dialog, "show function", OR_vertical);
	dialog->add_component(box);

	process_caption = new Caption(box, "Process:", CAP_LEFT);
	process_list = new Simple_text(process_caption, "", TRUE);
	process_caption->add_component(process_list);
	box->add_component(process_caption);

	char	*dummy = "";	// used to initalize selection lists, real values
				// are set later in set_process, called by set_plist

	caption = new Caption(box, "Objects:", CAP_TOP_LEFT);
	box->add_component(caption);
	objects = new Selection_list(caption, "objects", 4, 1, "%s", 1, &dummy,
		SM_single, this, (Callback_ptr)Function_dialog::select_cb);
	caption->add_component(objects);

	caption = new Caption(box, "Functions:", CAP_TOP_LEFT);
	box->add_elastic(caption);
	functions = new Selection_list(caption, "functions", 8, 1, "%25s", 1,
		&dummy, SM_single, this, 0, 0, (Callback_ptr)Function_dialog::drop_cb);

	caption->add_component(functions);
}

// When the current process changes, reset the object list, with no selection,
// and clear the function list.
void
Function_dialog::set_process(Boolean reset)
{
	const char	**list;
	int		total;

	obj_selection = f_selection = -1;
	functions->set_list(0, 0);

	if (!processes)
	{
		objects->set_list(0, 0);
		return;
	}

	if (reset)
		dialog->set_busy(TRUE);
	list = processes[0]->get_objects(total);
	objects->set_list(total, list);
	if (reset)
		dialog->set_busy(FALSE);
}

void
Function_dialog::apply(Component *, void *)
{
	int	*selections;

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

	// selection list in SM_single mode ensures only one selection
	f_selection = *selections;
	objects->get_selections(selections);
	obj_selection = *selections;

	disasm->disasm_set_func(functions->get_item(f_selection, 0));
}

void
Function_dialog::cancel(Component *, void *)
{
	int		*selections;
	const char	**list;
	int		total;

	if (obj_selection > -1)
	{
		objects->select(obj_selection);

		if (processes)
		{
			list = processes[0]->get_functions(total, 0,
				objects->get_item(obj_selection, 0), 0);
			functions->set_list(total, list);
			if (f_selection > -1)
				functions->select(f_selection);
		}
		else
			functions->set_list(0, 0);
	}
	else
	{
		if ((total = objects->get_selections(selections)) > 0)
			objects->deselect(*selections);
		functions->set_list(0, 0);
	}
}

// display the selected function in the Disassembly window
// complain if the function was not dropped on the parent window
void
Function_dialog::drop_cb(Selection_list *, Component *drop_window)
{
	if (drop_window)
	{
		Base_window	*window = drop_window->get_base();

		if (window == disasm) 
		{
			dialog->clear_msg();
			// conform to prototype
			apply(0,0);
			dialog->popdown();
			return;
		}
	}
	dialog->error(E_ERROR, GE_drop_to_disasm);
}

// update the function list whenever a new object is selected in the objects list
void
Function_dialog::select_cb(Selection_list *, int selection)
{
	if (selection == obj_selection)
		return;

	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;	
	}
	dialog->set_busy(TRUE);

	int		total;
	const char	**list = processes[0]->get_functions(total,
				0, objects->get_item(selection, 0), 0);

	functions->set_list(total, list);
	dialog->set_busy(FALSE);
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
    { "Windows",	'W', Menu_button,	SEN_always,
				(Callback_ptr)windows_pane,
				HELP_dis_windows_menu,
				sizeof(windows_pane)/sizeof(Menu_table) },
    { DISMISS,	DISMISS_MNE, Set_cb,	SEN_always, 
				(Callback_ptr)(&Window_set::dismiss),
				HELP_dis_dismiss_cmd },
    { "Quit",		'Q', Set_cb,	SEN_always,	
				(Callback_ptr)(&Window_set::ok_to_quit),
				HELP_dis_quit_cmd },
};

static const Menu_table view_pane[] =
{
    { "Show Location...",  'L',	Window_cb,	SEN_process,
				(Callback_ptr)(&Disasm_window::show_loc_cb),
				HELP_dis_show_location_dialog },
    { "Show Function...", 'F',	Window_cb,	SEN_process,
				(Callback_ptr)(&Disasm_window::show_function_cb),
				HELP_dis_show_function_dialog },
    { "Search...",	'E',	Set_cb,	(SEN_process|SEN_disp_dis_required),
				(Callback_ptr)(&Window_set::search_dialog_cb),
				HELP_dis_search_dialog },
    { "Show Value...",	'S',	Set_cb,		SEN_always,
				(Callback_ptr)(&Window_set::show_value_dialog_cb),
				HELP_dis_show_value_dialog },
    { "Set Value...",	'V',	Set_cb,		SEN_always,
				(Callback_ptr)(&Window_set::set_value_dialog_cb),
				HELP_dis_set_value_dialog },
};

static const Menu_table edit_pane[] =
{
    { "Copy",		'C',	Window_cb,	SEN_line_sel_required,
				(Callback_ptr)(&Disasm_window::copy_cb),
				HELP_dis_copy_cmd },
};

static const Menu_table control_pane[] =
{
    { "Run",		'R', Set_cb,	(SEN_process|SEN_proc_stopped), 
				(Callback_ptr)(&Window_set::run_button_cb),
				HELP_dis_run_cmd },
    { "Return",		't', Set_cb, (SEN_process|SEN_proc_stopped),
				(Callback_ptr)(&Window_set::run_r_button_cb),
				HELP_dis_return_cmd },
    { "Run Until...",	'U', Set_cb,	(SEN_process|SEN_proc_stopped),
				(Callback_ptr)(&Window_set::run_dialog_cb),
				HELP_dis_run_dialog },
    { "Step Statement",	'S', Set_cb,	(SEN_process|SEN_proc_stopped),
				(Callback_ptr)(&Window_set::step_button_cb),
				HELP_dis_step_stmt_cmd },
    { "Step Instruction", 'I', Set_cb,	(SEN_process|SEN_proc_stopped), 
				(Callback_ptr)(&Window_set::step_i_button_cb),
				HELP_dis_step_instr_cmd },
    { "Next Statement",	'N', Set_cb,	(SEN_process|SEN_proc_stopped),
				(Callback_ptr)(&Window_set::step_o_button_cb),
				HELP_dis_next_stmt_cmd },
    { "Next Instruction", 'x', Set_cb,	(SEN_process|SEN_proc_stopped),
				(Callback_ptr)(&Window_set::step_oi_button_cb),
				HELP_dis_next_instr_cmd },
    { "Step...",	'e',  Set_cb, (SEN_process|SEN_proc_stopped),
				(Callback_ptr)(&Window_set::step_dialog_cb),
				HELP_dis_step_dialog },
    { "Jump...", 	'J', Set_cb, (SEN_process|SEN_proc_stopped),
				(Callback_ptr)(&Window_set::jump_dialog_cb),
				HELP_dis_jump_dialog },
    { "Halt",		'H', Set_cb, (SEN_process|SEN_proc_running),
				(Callback_ptr)(&Window_set::halt_button_cb),
				HELP_dis_halt_cmd },
};

static const Menu_table event_pane[] =
{
    { "Set Breakpoint",	'B',  Window_cb,
			(SEN_process|SEN_proc_stopped|SEN_ins_sel_required),
			(Callback_ptr)(&Disasm_window::set_break_cb),
			HELP_dis_set_breakpoint_cmd },
    { "Delete Breakpoint", 'D', Window_cb,
		(SEN_process|SEN_proc_stopped|SEN_line_breakpt_required),
		(Callback_ptr)(&Disasm_window::delete_break_cb),
		HELP_dis_delete_breakpoint_cmd },
    { "Stop...",	'S',  Set_cb, (SEN_process|SEN_proc_stopped),
			(Callback_ptr)(&Window_set::stop_dialog_cb),
			HELP_dis_stop_dialog },
    { "Signal...",	'i',  Set_cb, (SEN_process|SEN_proc_stopped),
			(Callback_ptr)(&Window_set::signal_dialog_cb),
			HELP_dis_signal_dialog },
    { "Syscall...",	'y',  Set_cb, (SEN_process|SEN_proc_stopped),
			(Callback_ptr)(&Window_set::syscall_dialog_cb),
			HELP_dis_syscall_dialog },
    { "On Stop...",	'O',  Set_cb, (SEN_process|SEN_proc_stopped),
			(Callback_ptr)(&Window_set::onstop_dialog_cb),
			HELP_dis_onstop_dialog },
    { "Cancel...",	'C',  Set_cb, (SEN_process|SEN_proc_stopped),
			(Callback_ptr)(&Window_set::cancel_dialog_cb),
			HELP_dis_cancel_dialog },
    { "Kill...",	'K',  Set_cb, (SEN_process|SEN_proc_live),
			(Callback_ptr)(&Window_set::kill_dialog_cb),
			HELP_dis_kill_dialog },
    { "Ignore Signals...",'g', Set_cb, (SEN_process|SEN_proc_stopped),
			(Callback_ptr)(&Window_set::setup_signals_dialog_cb),
			HELP_dis_ignore_signals_dialog },
};

// The number of entries in the help menu is hard-coded because of a
// bug in cfront 2.1
static const Menu_table help_pane[3] =
{
	{ "Disassembly...", 'D',        Set_cb, SEN_always,
				(Callback_ptr)(&Window_set::help_sect_cb) },
	{ "Table of Contents...", 'T',  Set_cb, SEN_always,
        			(Callback_ptr)(&Window_set::help_toc_cb) },
	{ "Version",	'V',	Set_cb,	SEN_always,
				(Callback_ptr)(&Window_set::version_cb) },
};

static Menu_bar_table menu_table[] =
{
    { "File",     file_pane,    'F', sizeof(file_pane)/sizeof(Menu_table),
				HELP_dis_file_menu },
    { "Edit",     edit_pane,    'E', sizeof(edit_pane)/sizeof(Menu_table),
				HELP_dis_edit_menu },
    { "View",     view_pane,    'V', sizeof(view_pane)/sizeof(Menu_table),
				HELP_dis_view_menu },
    { "Control",  control_pane, 'C', sizeof(control_pane)/sizeof(Menu_table),
				HELP_dis_control_menu },
    { "Event",    event_pane,   'n', sizeof(event_pane)/sizeof(Menu_table),
				HELP_dis_event_menu },
    { "Help",	  help_pane,    'H', sizeof(help_pane)/sizeof(Menu_table) },
};
			
Disasm_window::Disasm_window(Window_set *ws) : BASE_WINDOW(ws, BW_disasm)
{
	Expansion_box	*box;
	Divided_box	*box1;
	int		*breaklist = 0;

	current_func = 0;
	current_loc  = 0;
	current_line = 0;
	selected_line = 0;

	window = new Window_shell("Disassembly",
		0, this, HELP_dis_window);
	box = new Expansion_box(window, "box", OR_vertical);
	window->add_component(box);

	menu_bar = new Menu_bar(box, this, ws, menu_table,
		sizeof(menu_table)/sizeof(Menu_bar_table));
	box->add_component(menu_bar);

	status_pane = new Status_pane(box);

	box1 = new Divided_box(box,"box");
	box->add_elastic(box1);

	// Register pane
	Caption	*captionR = new Caption(box1, "Registers", CAP_TOP_CENTER);
	reg_pane = new Text_area(captionR, "register", REG_DISP_LINE, 80,
		(Callback_ptr)(&Disasm_window::select_cb), this);

	captionR->add_component(reg_pane);
	box1->add_component(captionR);

	// Disassembly pane
	caption = new Caption(box1, "Disassembly", CAP_TOP_CENTER);
	pane = new Text_display(caption, "disassembly",
		(Callback_ptr)(&Disasm_window::select_cb), this);
	pane->setup_disassembly(10, 80);
	pane->set_breaklist(breaklist);

	caption->add_component(pane);
	box1->add_component(caption);

	show_loc 	= 0;
	show_function 	= 0;
	sel_pane 	= 0;
}

Disasm_window::~Disasm_window()
{
	delete window;
	delete status_pane;
	delete show_loc;
	delete show_function;
	delete current_func;
	delete current_loc;
}

void
Disasm_window::clear()
{
	pane->set_buf("");
	caption->set_label(" ");
	reg_pane->clear();
	selected_line = 0;
	current_line = 0;
	delete current_func;
	delete current_loc;
	current_loc = 0;
	current_func = 0;
	locations.clear();
	set_sensitivity();
}

void
Disasm_window::update_cb(void *, Reason_code rc, void *, Process *proc)
{
	if (!_is_open)
		return;

	status_pane->update(proc);

	if (rc == RC_rename)
		return;

	if (!proc)
	{
		window->clear_msg();
		window->display_msg(E_WARNING, GE_no_process);
		clear();
		set_sensitivity();
		return;
	}

	if (proc->get_state() == State_running
		|| proc->get_state() == State_stepping)
	{
		window->clear_msg();
		pane->set_line(0);
		sel_pane = 0;
		current_line = selected_line = 0;
		set_sensitivity();
		return;
	}

	// update register display and location
	disp_regs();
	if (!proc->get_location() || !*proc->get_location())
	{
		clear();
		set_sensitivity();
		return;
	}
	delete current_loc;
	current_loc = makestr(proc->get_location());

	//change disassembled display when
	// 	there is no current function and current_location does not exist
	//		in the current display
	//	or the current function changes
	if (!proc->get_function() || !*proc->get_function())
	{

		char	*endP = 0;
		Iaddr	retLong = (Iaddr)strtoul(current_loc, &endP, 0);

		delete current_func;
		current_func = 0;

		// ask the Disassembly window to disasm the code
		if (retLong != ULONG_MAX && endP != 0 && *endP == '\0') 
		{
			if (has_inst(retLong))
				set_display_line();
			else
			{
				caption->set_label(" ");
				disasm_func(current_loc);
			}
		}
		else
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}
	}
	else if (rc == RC_set_current || !current_func
		|| strcmp(proc->get_function(), current_func) != 0)
	{
		delete current_func;
		current_func = makestr(proc->get_function());
		disasm_func(current_func);
		caption->set_label(current_func);
	}

	set_display_line();
	set_sensitivity();
}

int
Disasm_window::check_sensitivity(int sense)
{
	if ((sense&SEN_line_sel_required) && !selected_line)
		return 0;
	if ((sense&SEN_disp_dis_required) && locations.size() == 0)
		return 0;
	if ((sense&SEN_ins_sel_required) &&
		(selection_type() != SEL_instruction ||
		  selected_line == 0))
		return 0;
	if ((sense&SEN_line_breakpt_required) &&
		(selection_type() != SEL_instruction ||
		  selected_line == 0 ||
		  ! pane->has_breakpoint(selected_line)))
		return 0;
	if (sense & SEN_process)
	{
		if (!window_set->current_process())
			return 0;
		return window_set->current_process()->check_sensitivity(sense);
	}
	return 1;
}

//return line number if address is within the disassembly display
//return 0 otherwise
int
Disasm_window::has_inst(Iaddr address)
{
	DispLine	*p = (DispLine *)locations.ptr();
	DispLine	*endP = (DispLine *)((char *)locations.ptr() + locations.size());
	int		line = 1;

	for (; p < endP; line++, p++)
	{
		if (p->getAddr() == address)
			 return line;
	}
	return 0;
}

// break_list_cb is called from the Event_list whenever a breakpoint is set
// or deleted
void
Disasm_window::break_list_cb(void *, Reason_code_break rc, void *, Stop_event *event)
{
	Process	*proc = window_set->current_process();
	if (!proc  || !event->has_process(proc))
		return;

	Breakpoint	*breaks = event->get_breakpts();

	// an event may have more than one breakpoint - handle all the ones that
	// fall within the current function
	for (int i = event->get_nbreakpts(); i; i--, breaks++)
	{
		int line;
		if ((line = has_inst((Iaddr)breaks->addr)) == 0)
			continue;

		if (rc == BK_add)
		{
			pane->set_stop(line);
		}
		else if (rc == BK_delete)
		{
			// if there is another enabled event on this address,
			// 	do not delete the sign
			// Note that at this point, the event may or
			//  may not exist on the event list for the 
			//  process(disabled, or deleted), but
			//  the event always exists on the event list
			int	*pEventNum = 0;

			// get_break_list takes a start and end address - just
			// get the events on this instruction
			proc->get_break_list(breaks->addr, breaks->addr, pEventNum);

			for (; *pEventNum; pEventNum++)
			{
				if (*pEventNum != event->get_id()
					&& event_list.findEvent(*pEventNum)->get_state() == ES_valid)
					break; //one enabled event exists

			}
			if (!*pEventNum)
				pane->clear_stop(line);
		}
	}
	set_sensitivity();
}

void
Disasm_window::popup()
{
	if (_is_open)
	{
		window->raise();
		return;
	}

	_is_open = 1;
	window->popup();
	update_cb(0, RC_set_current, 0, window_set->current_process());
	window_set->change_current.add(this, (Notify_func)(&Disasm_window::update_cb), 0);
	event_list.change_list.add(this, (Notify_func)(&Disasm_window::break_list_cb), 0);
}

void
Disasm_window::popdown()
{
	window->popdown();
	_is_open = 0;
	window_set->change_current.remove(this, (Notify_func)(&Disasm_window::update_cb), 0);
	event_list.change_list.remove(this, (Notify_func)(&Disasm_window::break_list_cb), 0);
}

void
Disasm_window::set_break_cb(Component *, void *)
{
	if (! selected_line)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	DispLine	*p = ((DispLine *)locations.ptr()) + selected_line - 1;
	Process	*proc = window_set->current_process();

	dispatcher.send_msg(this, proc->get_id(), "stop -p %s 0x%x\n",
		(window_set->get_event_level() == PROGRAM_LEVEL)
			? proc->get_program()->get_name() : proc->get_name(), p->getAddr());
}

void
Disasm_window::delete_break_cb(Component *, void *)
{
	if (! selected_line)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}


	Process		*pp = window_set->current_process();
	int		*pEventNum = 0;
	DispLine	*p = ((DispLine *)locations.ptr()) + selected_line - 1;

	// test if there are mutiple events on the line
	pp->get_break_list(p->getAddr(), p->getAddr(), pEventNum);
	if (*pEventNum != 0 && *(pEventNum + 1) != 0)
		display_msg((Callback_ptr)(&Disasm_window::ok_to_delete), this,
			"Delete", "Cancel", GE_multiple_events);
	else if (((Stop_event *)event_list.findEvent(*pEventNum))->get_nbreakpts() > 1)
		display_msg((Callback_ptr)(&Disasm_window::ok_to_delete), this,
			"Delete", "Cancel", GE_multiple_breaks);
	else
		dispatcher.send_msg(this, window_set->current_process()->get_id(),
			"delete %d\n", *pEventNum);
}

// ok_to_delete is the callback from the notice displayed by delete_break_cb,
// asking if it is ok to delete an event with multiple breakpoints, or multiple
// events on one line
void
Disasm_window::ok_to_delete(Component *, int delete_flag)
{
	if (!delete_flag)
		return;

	DispLine	*p = ((DispLine *)locations.ptr()) + selected_line - 1;
	Process		*proc = window_set->current_process();
	int		*pEventNum = 0;

	proc->get_break_list(p->getAddr(), p->getAddr(), pEventNum);
	for (; pEventNum && *pEventNum; pEventNum++)
	{
		dispatcher.send_msg(this, proc->get_id(), "delete %d\n", *pEventNum);
	}

}

void
Disasm_window::copy_cb(Component *, void *)
{
	if (sel_pane)
		sel_pane->copy_selection();
}

void
Disasm_window::show_function_cb(Component *, void *)
{
	if (show_function == 0)
		show_function = new Function_dialog(this);

	show_function->set_plist(this, PROCESS_LEVEL);
	show_function->display();
}

void
Disasm_window::show_loc_cb(Component *, void *)
{
	if (!show_loc)
		show_loc = new Show_loc_dialog(this);
	
	if (sel_pane)
		show_loc->set_string(get_selected_addr());

	show_loc->display();
}

void
Disasm_window::select_cb(Text_display *d, int line)
{
	if (line)
		sel_pane = d;
	else
		sel_pane = 0;
	selected_line = line;
	set_sensitivity();
}

Selection_type
Disasm_window::selection_type()
{
	if (sel_pane == pane)
		return SEL_instruction;
	else if (sel_pane == reg_pane)
		return SEL_regs;
	return SEL_none;
}

char *
Disasm_window::get_selection()
{
	return sel_pane ? sel_pane->get_selection() : 0;
}

// disassemble the location 
void
Disasm_window::disasm_addr(Iaddr loc, char *s)
{
	current_line = has_inst(loc);
	
	if (current_line != 0)
	{
		pane->set_line(current_line);
		pane->position(current_line, TRUE);
		sel_pane = pane;
		selected_line = current_line;
		return;
	}

	disasm_func(s);
	caption->set_label(" ");
}

void
Disasm_window::disasm_set_func(const char *loc)
{
	delete current_func;
	current_func = makestr(loc);
	caption->set_label(loc);
	disasm_func(loc);
}

extern	Buffer	buffer1;

void
Disasm_window::disasm_func(const char *loc)
{
	Word	line,addr;
	char	*instruction;
	Message	*msg;

	int	error = 0;

	buffer1.clear();
	locations.clear();

	if (strstr(loc, "@PLT") != 0)
		dispatcher.query(this, window_set->current_process()->get_id(),
		 	"dis -f %s\n", window_set->current_process()->get_location());
	else	
		dispatcher.query(this, window_set->current_process()->get_id(),
		 	"dis -f %s\n", loc);

	Iaddr	beginAddr= 0, endAddr = 0;
	int	first = TRUE;

	while ((msg = dispatcher.get_response()) != 0)
	{
		switch(msg->get_msg_id())
		{
			case MSG_dis_line :
				msg->unbundle(line, addr, instruction);
				{
					DispLine tmpDisp((Iaddr)addr, (int)line);
					locations.add(&tmpDisp, sizeof(DispLine));
				}

				break;

			case MSG_disassembly :
				msg->unbundle(addr, instruction);
				{
					DispLine tmpDisp((Iaddr)addr);
					locations.add(&tmpDisp, sizeof(DispLine));
				}

				break;

			case MSG_dis_header :
				continue;
			default:
				error = 1;
				continue;
		}

		buffer1.add(msg->format());
		
		if (first)
		{
			beginAddr = (Iaddr)addr;
			first = FALSE;
		}
		endAddr   = (Iaddr)addr; 

	}	

	if (error == 1)
	{
		display_msg(E_WARNING, GE_bad_expr);
		return;
	}

	pane->set_buf((char *)buffer1);
	set_display_line();

	int	*breaklist = 0;
	int	*eNum;
	Process	*pp = window_set->current_process();

	// Note : both scratch vectors are used here
	breaklist = pp->get_break_list(beginAddr,endAddr, eNum);

	// translate source line number into display line number 
	for (int *pb = breaklist; *pb; pb++)
		*pb = has_inst(*pb);

	pane->set_breaklist(breaklist);
}

void
Disasm_window::set_display_line()
{
	int returnLine = has_inst(current_loc);

	current_line = selected_line = 0;

	if (returnLine != 0)
	{
		current_line = returnLine;
		selected_line = current_line;
		pane->set_line(current_line);
		pane->position(current_line, TRUE);
		sel_pane = pane;
		selected_line = current_line;
	}
}

int
Disasm_window::has_inst(const char *s)
{
	char	*endP = 0;
	Iaddr	retLong = strtoul(s, &endP, 0);
	char	*pc = 0;

	// location is a hex number or file@line
	if (retLong != ULONG_MAX && endP != 0 && *endP == '\0') 
	{
		return has_inst(retLong);
	}
	else if ((pc = strchr(current_loc, '@')) != 0)
	{
		int 		line = 1;
		DispLine	*p = (DispLine *)locations.ptr();
		DispLine	*endP = (DispLine *)((char *)locations.ptr() + locations.size());
		int		tmpLine = atoi(++pc);

		while (p < endP)
		{
			if (p->getLine() == tmpLine)
			 	return line;

			line++;	
			p++;
		}
	}
	return 0;
}

void
Disasm_window::disp_regs()
{
	Message	*msg;

	buffer1.clear();
	dispatcher.query(this, window_set->current_process()->get_id(), "regs \n");
	while ((msg = dispatcher.get_response()) != 0)
	{
		switch(msg->get_msg_id())
		{
			case MSG_int_reg_newline :
			case MSG_flt_reg :
			case MSG_int_reg :
				buffer1.add(msg->format());
				break;

			case MSG_reg_header :
				break;
			default:
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
				break;
		}
	}	

	reg_pane->clear();
	reg_pane->add_text((char *)buffer1);
	reg_pane->position(1);
}

const char *
Disasm_window::get_selected_addr()
{
	static char	buf[ sizeof(Iaddr) * 2 + 3 ];

	buf[0] = '\0';
	if (!selected_line) 
		return buf;

	DispLine	*p = ((DispLine *)locations.ptr()) + selected_line - 1;

	sprintf(buf, "%#x", p->getAddr());
	return buf;
}
