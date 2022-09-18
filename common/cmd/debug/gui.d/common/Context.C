#ident	"@(#)debugger:gui.d/common/Context.C	1.24"

#include "Context.h"
#include "Ps_pane.h"
#include "Stack_pane.h"
#include "Syms_pane.h"
#include "Proclist.h"
#include "Dialogs.h"
#include "Dialog_sh.h"
#include "Dispatcher.h"
#include "Caption.h"
#include "Radio.h"
#include "Sel_list.h"
#include "Text_area.h"
#include "Text_line.h"
#include "Stext.h"
#include "Windows.h"
#include "UI.h"
#include "Menu.h"
#include "Boxes.h"
#include "Table.h"
#include "Msgtab.h"
#include "str.h"
#include "Buffer.h"
#include "Vector.h"
#include "Machine.h"
#include <ctype.h>
#include <stdio.h>

extern Buffer	buffer1;

static char *
CC_expression(char *expression, char *selection)
{
	char *p;

	buffer1.clear();
	if (selection && *selection)
	{
		buffer1.add(expression);
		while (!isalpha(*selection))
			selection++;
		p = selection + 1;
		while (isalnum(*p) || *p == '_')
			p++;
		*p = '\0';
		buffer1.add("->");
		buffer1.add(selection);
	}
	else
	{
		buffer1.add('*');
		buffer1.add(expression);
	}
	return (char *)buffer1;
}

class Expand_dialog : public Process_dialog
{
	Simple_text	*expression;
	Text_area	*result;
	char		*expr;
	int		expanded;
	Vector		vector;

public:
			Expand_dialog(Window_set *);
			~Expand_dialog() {};

	char		*make_expression(char *expression, char *result);
	void		set_expression(const char *);

			// callbacks
	void		expand(Component *, void *);
	void		collapse(Component *, void *);

			// functions inherited from Dialog_box
	void		de_message(Message *);
	void		cmd_complete();
};

Expand_dialog::Expand_dialog(Window_set *ws) : PROCESS_DIALOG(ws)
{
	static const Button	buttons[] =
	{
		{ B_non_exec,	"Expand",  'E',
				(Callback_ptr)(&Expand_dialog::expand) },
		{ B_non_exec,	"Collapse",  'o',
				(Callback_ptr)(&Expand_dialog::collapse) },
		{ B_close,  0, 0, 0 },
		{ B_help,   0, 0, 0 },
	};

	Expansion_box	*box;
	Caption		*caption;

	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
		"Expand", (Callback_ptr)Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_expand_dialog);
	box = new Expansion_box(dialog, "show value", OR_vertical);
	dialog->add_component(box);
	process_caption = new Caption(box, "Processes:", CAP_LEFT);
	process_list = new Simple_text(process_caption, "", TRUE);
	process_caption->add_component(process_list);
	box->add_component(process_caption);

	caption = new Caption(box, "Expression:", CAP_LEFT);
	expression = new Simple_text(caption, "", TRUE);
	caption->add_component(expression);
	box->add_component(caption);

	caption = new Caption(box, "Result of Evaluated Expression:", CAP_TOP_LEFT);
	result = new Text_area(caption, "result", 8, 30);
	caption->add_component(result);
	box->add_elastic(caption);

	expanded = 0;
}

void
Expand_dialog::set_expression(const char *s)
{
	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	Message	*msg;
	char	*expr = makestr(s);

	expression->set_text(expr);
	result->clear();
	vector.clear();
	vector.add(&expr, sizeof(char *));
	expanded = 0;
	dispatcher.query(this, window_set->current_process()->get_id(),
		"print -p %s %s\n",
		make_plist(total, processes, 0, level), expr);
	while ((msg = dispatcher.get_response()) != 0)
		result->add_text(msg->format());
}

void
Expand_dialog::expand(Component *, void *)
{
	dialog->clear_msg();
	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	char *expr = make_expression(((char **)vector.ptr())[expanded],
			result->get_selection());
	if (!expr)
		return;
	expression->set_text(expr);
	vector.add(&expr, sizeof(char *));

	expanded++;
	result->clear();
	dispatcher.send_msg(this, window_set->current_process()->get_id(),
		"print -p %s %s\n",
		make_plist(total, processes, 0, level), expr);
	dialog->wait_for_response();
}

char *
Expand_dialog::make_expression(char *expression, char *selection)
{
	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return NULL;
	}

	Language	lang = cur_language;

	if (cur_language == UnSpec)
	{
		Message	*msg;

		dispatcher.query(this, processes[0]->get_id(),
			"print %%db_lang\n");
		while ((msg = dispatcher.get_response()) != 0)
		{
			if (msg->get_msg_id() == MSG_print_val)
			{
				char *l;
				msg->unbundle(l);
				if (strcmp(l, "\"C\"\n") == 0)
					lang = C;
				else if (strcmp(l, "\"C++\"\n") == 0)
					lang = CPLUS;
			}
		}
	}

	switch(lang)
	{
	case C:
	case CPLUS:
		return makestr(CC_expression(expression, selection));

	case UnSpec:
	default:
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return NULL;
	}
}

void
Expand_dialog::de_message(Message *m)
{
	Msg_id	mtype = m->get_msg_id();

	if (Mtable.msg_class(mtype) == MSGCL_error)
	{
		if (mtype == ERR_cmd_pointer || mtype == ERR_asis)
			return;

		if (mtype == ERR_syntax_loc)
			dialog->error(E_ERROR, GE_syntax_error);
		else if (m->get_severity() == E_ERROR)
			dialog->error(m);
		else
			display_msg(m);
	}
	else
		result->add_text(m->format());
}

void
Expand_dialog::cmd_complete()
{
	result->position(1);	// scroll back to top of text
	dialog->cmd_complete();
}

void
Expand_dialog::collapse(Component *, void *)
{
	if (!expanded)
		return;

	dialog->clear_msg();
	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	char *expr = ((char **)vector.ptr())[expanded];
	delete expr;
	vector.drop(sizeof(char *));

	expanded--;
	expr = ((char **)vector.ptr())[expanded];
	expression->set_text(expr);

	result->clear();
	dispatcher.send_msg(this, window_set->current_process()->get_id(),
		"print -p %s %s\n",
		make_plist(total, processes, 0, level), expr);
	dialog->wait_for_response();
}

// radio list states for Context_dialog
#define	TRUNCATED	0
#define	WRAPPED		1

// Context_dialog is created from Properties dialog's "Panes" button
class Context_dialog : public Dialog_box
{
	Radio_list	*ps;
	Radio_list	*stack;
	Radio_list	*syms;
	int		ps_state;
	int		stack_state;
	int		syms_state;
public:
			Context_dialog(Context_window *);
			~Context_dialog() {};

	void		apply(Component *, void *);
	void		reset(Component *, void *);
};
		
Context_dialog::Context_dialog(Context_window *cw) : DIALOG_BOX(cw->get_window_set())
{
	static const Button	buttons[] =
	{
		{ B_apply,   0, 0, (Callback_ptr)Context_dialog::apply },
		{ B_reset,   0, 0, (Callback_ptr)Context_dialog::reset },
		{ B_cancel,  0,	0, (Callback_ptr)Context_dialog::reset },
		{ B_help,    0,	0, 0 },
	};

	static const char *choices[] =
	{
		"Truncate",
		"Wrap",
	};

	Packed_box	*box;
	Caption		*caption;

	ps_state = stack_state = syms_state = TRUNCATED;
	dialog = new Dialog_shell(cw->get_window_shell(), "Panes", 0, this,
		buttons, sizeof(buttons)/sizeof(Button),
		HELP_panes1_dialog);
	box = new Packed_box(dialog, "properties", OR_vertical);

	caption = new Caption(box, "Process Pane:", CAP_LEFT);
	ps = new Radio_list(caption, "ps pane", OR_horizontal, choices,
		sizeof(choices)/sizeof(char *), 0);
	caption->add_component(ps);
	box->add_component(caption);

	caption = new Caption(box, "Stack Pane:   ", CAP_LEFT);
	stack = new Radio_list(caption, "stack pane", OR_horizontal, choices,
		sizeof(choices)/sizeof(char *), 0);
	caption->add_component(stack);
	box->add_component(caption);

	caption = new Caption(box, "Symbols Pane:", CAP_LEFT);
	syms = new Radio_list(caption, "syms pane", OR_horizontal, choices,
		sizeof(choices)/sizeof(char *), 0);
	caption->add_component(syms);
	box->add_component(caption);

	dialog->add_component(box);
}

void
Context_dialog::apply(Component *, void *)
{
	Context_window	*cw = window_set->get_context_window();

	ps_state = ps->which_button();
	cw->get_ps_pane()->get_table()->wrap_columns(ps_state);
	stack_state = stack->which_button();
	cw->get_stack_pane()->get_table()->wrap_columns(stack_state);
	syms_state = syms->which_button();
	cw->get_syms_pane()->get_table()->wrap_columns(syms_state);
}

void
Context_dialog::reset(Component *, void *)
{
	ps->set_button(ps_state);
	stack->set_button(stack_state);
	syms->set_button(syms_state);
}

class Stop_on_function_dialog : public Process_dialog
{
	Selection_list	*objects;
	Selection_list	*functions;
	Caption		*caption;
	int		obj_selection;
	int		f_selection;
public:
			Stop_on_function_dialog(Context_window *);
			~Stop_on_function_dialog() {}

			// callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);
	void		select_cb(Selection_list *, int);

			// functions inherited from Process_dialog
	void		set_process(Boolean reset);
};

Stop_on_function_dialog::Stop_on_function_dialog(Context_window *cw)
	: PROCESS_DIALOG(cw->get_window_set())
{
	static const Button	buttons[] =
	{
		{ B_apply,	"Stop on Function", 'S',
				(Callback_ptr)(&Stop_on_function_dialog::apply) },
		{ B_cancel,	0, 0, (Callback_ptr)(&Stop_on_function_dialog::cancel) },
		{ B_help,	0, 0, 0 },
	};

	Expansion_box	*box;
	Caption		*tmp;
	const char	*initial_string = "";

	obj_selection = -1;
	f_selection = -1;

	dialog = new Dialog_shell(cw->get_window_shell(), "Stop on Function",
		(Callback_ptr)Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(Button),
		HELP_stop_on_function_dialog);
	box = new Expansion_box(dialog, "Stop on function", OR_vertical);
	dialog->add_component(box);

	process_caption = new Caption(box, "Processes:", CAP_LEFT);
	process_list = new Simple_text(process_caption, "", TRUE);
	process_caption->add_component(process_list);
	box->add_component(process_caption);

	tmp = new Caption(box, "Objects:", CAP_TOP_LEFT);
	objects = new Selection_list(tmp, "objects", 4, 1, "%s", 1,
		&initial_string, SM_single, this,
		(Callback_ptr)Stop_on_function_dialog::select_cb);
	tmp->add_component(objects);
	box->add_component(tmp);

	caption = new Caption(box, "Functions:", CAP_TOP_LEFT);
	functions = new Selection_list(caption, "functions", 8, 1, "%25s", 1,
		&initial_string, SM_single);
	caption->add_component(functions);
	box->add_elastic(caption);
}

void
Stop_on_function_dialog::set_process(Boolean reset)
{
	obj_selection = -1;
	f_selection = -1;

	if (!processes)
	{
		objects->set_list(0, 0);
		functions->set_list(0, 0);
		return;
	}

	const char	**list;
	int		total;

	if (reset)
		dialog->set_busy(TRUE);
	if ((list = processes[0]->get_objects(total)) == 0)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	objects->set_list(total, list);
	caption->set_label("Functions:");
	functions->set_list(0, 0);
	f_selection = -1;
	if (reset)
		dialog->set_busy(FALSE);
}

void
Stop_on_function_dialog::apply(Component *, void *)
{
	int		*sels;

	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	if (functions->get_selections(sels) == 0)
	{
		dialog->error(E_ERROR, GE_no_function);
		return;
	}

	f_selection = *sels;
	objects->get_selections(sels);
	obj_selection = *sels;

	// -p is needed to ensure process-specific stops
	dispatcher.send_msg(this, processes[0]->get_id(), "stop -p %s %s\n",
		make_plist(1, processes, 0, level),
		functions->get_item(f_selection, 0));
	dialog->wait_for_response();
}

void
Stop_on_function_dialog::cancel(Component *, void *)
{
	int		*selections;
	const char**	list;
	int		total;

	if (!dialog->is_pinned())
		return;

	if (!processes)
	{
		obj_selection = -1;
		objects->set_list(0,0);
		f_selection = -1;
		functions->set_list(0,0);
		return;
	}

	if (obj_selection > -1)
	{
		objects->select(obj_selection);

		list = processes[0]->get_functions(total, 0,
			objects->get_item(obj_selection, 0), 0);
		functions->set_list(total, list);
		if (f_selection > -1)
			functions->select(f_selection);
	}
	else
	{
		if ((total = objects->get_selections(selections)) > 0)
			objects->deselect(*selections);
		functions->set_list(0, 0);
	}
}

void
Stop_on_function_dialog::select_cb(Selection_list *, int selection)
{
	static char *format = "Functions from %.30s:";

	const char	**funclist;
	int		nfuncs;
	char		buf[50];
	const char	*obj;

	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	dialog->set_busy(TRUE);
	dialog->clear_msg();
	obj = objects->get_item(selection, 0);
	sprintf(buf, format, obj);
	caption->set_label(buf);

	if ((funclist = processes[0]->get_functions(nfuncs, 0, obj, 0)) == 0)
	{
		dialog->error(E_ERROR, GE_stripped_file, obj);
		dialog->set_busy(FALSE);
		return;
	}
	functions->set_list(nfuncs, funclist);
	f_selection = -1;
	dialog->set_busy(FALSE);
}

class Dump_dialog : public Process_dialog
{
	Text_line	*location;
	Text_line	*count;
	Text_area	*dump_pane;
public:
			Dump_dialog(Window_set *);
			~Dump_dialog() {};

			// callbacks
	void		do_dump(Component *, void *);
	
	void		set_location(const char *s) { location->set_text(s); }
	void		clear() { dump_pane->clear(); }

			// functions inherited from Dialog_box
	void		de_message(Message *);
};

Dump_dialog::Dump_dialog(Window_set *ws) : PROCESS_DIALOG(ws)
{
	static const Button	buttons[] =
	{
		{ B_non_exec,  "Dump",  'D', (Callback_ptr)Dump_dialog::do_dump },
		{ B_close,   0,	0, 0 },
		{ B_help,    0,	0, 0 },
	};

	Expansion_box	*box1;
	Expansion_box	*box2;
	Caption		*caption;

	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
		"Dump", (Callback_ptr)Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_dump_dialog);
	box1 = new Expansion_box(dialog, "dump box", OR_vertical);
	dialog->add_component(box1);

	process_caption = new Caption(box1, "Process:", CAP_LEFT);
	process_list = new Simple_text(process_caption, "", TRUE);
	process_caption->add_component(process_list);
	box1->add_component(process_caption);

	box2 = new Expansion_box(box1, "dump box", OR_horizontal);
	box1->add_component(box2);

	caption = new Caption(box2, "Location:", CAP_LEFT);
	location = new Text_line(caption, "location", "", 20, 1);
	caption->add_component(location);
	box2->add_elastic(caption);

	caption = new Caption(box2, "Count:", CAP_LEFT);
	count = new Text_line(caption, "count", "", 10, 1);
	caption->add_component(count);
	box2->add_component(caption);

	dump_pane = new Text_area(box1, "dump", 20, 60);
	box1->add_elastic(dump_pane);
}

void
Dump_dialog::do_dump(Component *, void *)
{
	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	char		*loc = location->get_text();
	char		*cnt = count->get_text();
	const char	*plist = make_plist(total, processes, 0, level);

	if (!loc || !*loc)
	{
		dialog->error(E_ERROR, GE_no_location);
		return;
	}
	dump_pane->clear();		

	if (cnt && *cnt)
		dispatcher.send_msg(this, window_set->current_process()->get_id(),
			"dump -p %s -c %s %s\n", plist, cnt, loc);
	else
		dispatcher.send_msg(this, window_set->current_process()->get_id(),
			"dump -p %s %s\n", plist, loc);
	dialog->wait_for_response();
}

void
Dump_dialog::de_message(Message *m)
{
	Msg_id	mtype = m->get_msg_id();

	if (Mtable.msg_class(mtype) == MSGCL_error)
	{
		if (mtype == ERR_cmd_pointer || mtype == ERR_asis)
			return;

		if (mtype == ERR_syntax_loc)
			dialog->error(E_ERROR, GE_syntax_error);
		else if (m->get_severity() == E_ERROR)
			dialog->error(m);
		else
			display_msg(m);
	}
	else if (mtype != MSG_raw_dump_header)
		dump_pane->add_text(m->format());
}

class Map_dialog : public Process_dialog
{
	Text_area	*map_pane;
public:
			Map_dialog(Window_set *);
			~Map_dialog() {};

	void		do_map();
};

Map_dialog::Map_dialog(Window_set *ws) : PROCESS_DIALOG(ws)
{
	static const Button	buttons[] =
	{
		{ B_close,   0, 0, 0 },
		{ B_help,    0,	0, 0 },
	};

	Expansion_box	*box;

	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
		"Map", (Callback_ptr)Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_map_dialog);
	box = new Expansion_box(dialog, "map box", OR_vertical);
	dialog->add_component(box);

	process_caption = new Caption(box, "Process:", CAP_LEFT);
	process_list = new Simple_text(process_caption, "", TRUE);
	process_caption->add_component(process_list);
	box->add_component(process_caption);

	map_pane = new Text_area(box, "map", 8, 75);
	box->add_elastic(map_pane);
}

void
Map_dialog::do_map()
{
	Message		*msg;

	map_pane->clear();
	dispatcher.query(this, window_set->current_process()->get_id(),
		"map -p %s\n", make_plist(total, processes, 0, level));
	// the responses look like:
	//	MSG_map_header <process_name>\n<header_string>\n
	//	MSG_map <line>\n
	//	MSG_map <line>\n
	//	...
	//	0
	while ((msg = dispatcher.get_response()) != 0)
	{
		char *s = msg->format();
		if (msg->get_msg_id() == MSG_map_header)
		{
			s = strchr(s, '\n') +  1; // chops off process name,
						  // already displayed
		}
			// this avoids putting out the final new-line,
			// which would leave an extra blank line at
			// the bottom of the pane
		else
			map_pane->add_text("\n");
		s[strlen(s)-1] = '\0';
		map_pane->add_text(s);
	}
	map_pane->position(1);
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

static const Menu_table release_pane[] =
{
    { "Running",	'R',	Set_cb,	SEN_process|SEN_proc_live|SEN_use_selection,	
				(Callback_ptr)(&Window_set::release_running_cb), },
    { "Suspended",	'S',	Set_cb,	SEN_process|SEN_use_selection,
				(Callback_ptr)(&Window_set::release_suspended_cb), },
};

static const Menu_table file_pane[] =
{
    { "Create...",	  't', Set_cb,  SEN_always,
				(Callback_ptr)(&Window_set::create_dialog_cb),
				HELP_ctxt_create_dialog },
    { "Recreate...",	  'R', Set_cb,  SEN_proc_created,
				(Callback_ptr)(&Window_set::recreate_dialog_cb),
				HELP_ctxt_recreate_dialog },
    { "Grab Core...",	  'G', Set_cb,  SEN_always,
				(Callback_ptr)(&Window_set::grab_core_dialog_cb),
				HELP_ctxt_grab_core_dialog },
    { "Grab Process...",  'P', Set_cb,	   SEN_always,
				(Callback_ptr)(&Window_set::grab_process_dialog_cb), 
				HELP_ctxt_grab_process_dialog },
    { "Release",	  'e', Menu_button, SEN_process|SEN_use_selection,
				(Callback_ptr)release_pane, HELP_release_cmd, 
				sizeof(release_pane)/sizeof(Menu_table) },
    { "Change Directory...", 'C', Set_cb, SEN_always,
				(Callback_ptr)(&Window_set::cd_dialog_cb),
				HELP_ctxt_cd_dialog },
    { "New Window Set",	'N', Window_cb,	SEN_always,	
				(Callback_ptr)(&Context_window::new_window_set_cb),
				HELP_new_window_set_cmd },
    { "Windows",	'W', Menu_button, SEN_always,	
				(Callback_ptr)windows_pane,
				HELP_windows_menu,
				sizeof(windows_pane)/sizeof(Menu_table) },
    { DISMISS,	  DISMISS_MNE, Set_cb,  SEN_always,	
				(Callback_ptr)(&Window_set::dismiss),
				HELP_dismiss_cmd },
    { "Quit",		  'Q', Set_cb,  SEN_always,	
				(Callback_ptr)(&Window_set::ok_to_quit),
				HELP_quit_cmd },
};

static const Menu_table view_pane[] =
{
    { "Expand...",	'E',	Window_cb,	
		(SEN_program_symbol|SEN_single_symbol),
		(Callback_ptr)(&Context_window::expand_dialog_cb),
		HELP_ctxt_expand_dialog },
    { "Show Value...",	'S',	Set_cb,
		SEN_symbol,
		(Callback_ptr)(&Window_set::show_value_dialog_cb),
		HELP_ctxt_show_value_dialog },
    { "Set Value...",	'V',	Set_cb,
		SEN_single_symbol,
		(Callback_ptr)(&Window_set::set_value_dialog_cb),
		HELP_ctxt_set_value_dialog },
    { "Dump...",	'D', 	Window_cb,
		(SEN_process|SEN_proc_single|SEN_use_selection
			|SEN_proc_stopped_core|SEN_symbol),
		(Callback_ptr)(&Context_window::dump_dialog_cb),
		HELP_ctxt_dump_dialog },
    { "Map...",		'M', Window_cb,
		(SEN_process|SEN_proc_single|SEN_use_selection),	
		(Callback_ptr)(&Context_window::map_dialog_cb),
		HELP_ctxt_map_dialog },
};

static const Menu_table edit_pane[] =
{
    { "Set Current",	'S', Window_cb,
		(SEN_sel_required|SEN_process|SEN_proc_single|SEN_frame|SEN_use_selection),	
		(Callback_ptr)(&Context_window::set_current),
		HELP_set_current_cmd },
    { "Export",		'x',	Window_cb,	SEN_user_symbol,
		(Callback_ptr)(&Context_window::export_syms_cb),
		HELP_export_cmd },
};

static const Menu_table prop_pane[] =
{
    { "Symbols...",  'S', Window_cb,	SEN_always,	
				(Callback_ptr)(&Context_window::setup_syms_cb),
				HELP_ctxt_symbols_dialog},
    { "Panes...",	'a', Window_cb,	SEN_always,
				(Callback_ptr)(&Context_window::setup_panes_cb),
				HELP_ctxt_panes_dialog },
    { "Source Path...",	'P', Set_cb, 	SEN_always,
				(Callback_ptr)(&Window_set::path_dialog_cb),
				HELP_ctxt_path_dialog },
    { "Language...",	'L', Set_cb,	SEN_always,	
				(Callback_ptr)(&Window_set::set_language_dialog_cb),
				HELP_ctxt_language_dialog },
    { "Granularity...", 'G', Set_cb,	SEN_always,
			(Callback_ptr)(&Window_set::set_granularity_cb),
				HELP_ctxt_granularity_dialog },
    { "Output Action...",	'O', Set_cb,	SEN_always,	
			(Callback_ptr)(&Window_set::action_dialog_cb),
				HELP_ctxt_action_dialog },
};

static const Menu_table control_pane[] =
{
    { "Run",		'R',	Set_cb,	(SEN_process|SEN_proc_stopped|SEN_use_selection), 
				(Callback_ptr)(&Window_set::run_button_cb),
				HELP_run_cmd},
    { "Return",		't',	Set_cb,	(SEN_process|SEN_proc_stopped|SEN_use_selection),
				(Callback_ptr)(&Window_set::run_r_button_cb),
				HELP_return_cmd },
    { "Run Until...",	'U',	Set_cb,	(SEN_process|SEN_proc_stopped|SEN_use_selection),
				(Callback_ptr)(&Window_set::run_dialog_cb),
				HELP_ctxt_run_dialog },
    { "Step Statement",	'S',	Set_cb,	(SEN_process|SEN_proc_stopped|SEN_use_selection),
				(Callback_ptr)(&Window_set::step_button_cb),
				HELP_step_stmt_cmd },
    { "Step Instruction", 'I',	Set_cb,	(SEN_process|SEN_proc_stopped|SEN_use_selection), 
				(Callback_ptr)(&Window_set::step_i_button_cb),
				HELP_step_instr_cmd },
    { "Next Statement",	'N',	Set_cb,	(SEN_process|SEN_proc_stopped|SEN_use_selection),
				(Callback_ptr)(&Window_set::step_o_button_cb),
				HELP_next_stmt_cmd },
    { "Next Instruction", 'x',	Set_cb, (SEN_process|SEN_proc_stopped|SEN_use_selection),
				(Callback_ptr)(&Window_set::step_oi_button_cb),
				HELP_next_instr_cmd },
    { "Step...",	'e', 	Set_cb, (SEN_process|SEN_proc_stopped|SEN_use_selection),
				(Callback_ptr)(&Window_set::step_dialog_cb),
				HELP_ctxt_step_dialog },
    { "Jump...", 	'J',	Set_cb, (SEN_process|SEN_proc_stopped|SEN_use_selection),
				(Callback_ptr)(&Window_set::jump_dialog_cb),
				HELP_ctxt_jump_dialog },
    { "Halt",		'H',	Set_cb, (SEN_process|SEN_proc_running|SEN_use_selection),
				(Callback_ptr)(&Window_set::halt_button_cb),
				HELP_halt_cmd },
};

static const Menu_table event_pane[] =
{
    { "Stop on Function...",	'F',	Window_cb,
		(SEN_process|SEN_proc_stopped|SEN_use_selection|SEN_proc_single),
		(Callback_ptr)(&Context_window::stop_on_function_cb),
		HELP_ctxt_stop_on_function_dialog },
    { "Set Watchpoint", 'W',  Window_cb,
		(SEN_process|SEN_proc_stopped|SEN_program_symbol),
		(Callback_ptr)(&Context_window::set_watchpoint_cb),
		HELP_set_watchpoint_cmd },
    { "Stop...",	'S',  Set_cb,
		(SEN_process|SEN_proc_stopped|SEN_use_selection),
		(Callback_ptr)(&Window_set::stop_dialog_cb),
		HELP_ctxt_stop_dialog },
    { "Signal...",	'i',  Set_cb,
		(SEN_process|SEN_proc_stopped|SEN_use_selection),
		(Callback_ptr)(&Window_set::signal_dialog_cb),
		HELP_ctxt_signal_dialog },
    { "Syscall...",	'y',  Set_cb,
		(SEN_process|SEN_proc_stopped|SEN_use_selection),
		(Callback_ptr)(&Window_set::syscall_dialog_cb),
		HELP_ctxt_syscall_dialog },
    { "On Stop...",	'O',  Set_cb,
		(SEN_process|SEN_proc_stopped|SEN_use_selection),
		(Callback_ptr)(&Window_set::onstop_dialog_cb),
		HELP_ctxt_on_stop_dialog },
    { "Cancel...",	'C',  Set_cb,
		(SEN_process|SEN_proc_stopped|SEN_proc_single|SEN_use_selection),
		(Callback_ptr)(&Window_set::cancel_dialog_cb),
		HELP_ctxt_cancel_dialog },
    { "Kill...",	'K',  Set_cb,
		SEN_process|SEN_proc_live|SEN_use_selection,
		(Callback_ptr)(&Window_set::kill_dialog_cb),
		HELP_ctxt_kill_dialog },
    { "Ignore Signals...",'g', Set_cb,
		(SEN_process|SEN_proc_stopped|SEN_proc_single|SEN_use_selection),
		(Callback_ptr)(&Window_set::setup_signals_dialog_cb),
		HELP_ctxt_ignore_signals_dialog },
};

// The number of entries in the help menu is hard-coded because of a
// bug in cfront 2.1
static const Menu_table help_pane[3] =
{
	{ "Context...", 'C', Set_cb, SEN_always,
				(Callback_ptr)(&Window_set::help_sect_cb) },
	{ "Table of Contents...", 'T', Set_cb, SEN_always,
				(Callback_ptr)(&Window_set::help_toc_cb) },
	{ "Version",	'V',	Set_cb,	SEN_always,
				(Callback_ptr)(&Window_set::version_cb) },
};

static const Menu_bar_table menu_table[] =
{
    { "File",     file_pane,    'F', sizeof(file_pane)/sizeof(Menu_table),
			HELP_ctxt_file_menu },
    { "Edit",     edit_pane,    'E', sizeof(edit_pane)/sizeof(Menu_table),
			HELP_ctxt_edit_menu },
    { "View",     view_pane,    'V', sizeof(view_pane)/sizeof(Menu_table),
			HELP_ctxt_view_menu },
    { "Control",  control_pane, 'C', sizeof(control_pane)/sizeof(Menu_table),
			HELP_ctxt_control_menu},
    { "Event",    event_pane,   'n', sizeof(event_pane)/sizeof(Menu_table),
			HELP_ctxt_event_menu },
    { "Properties", prop_pane,  'P', sizeof(prop_pane)/sizeof(Menu_table),
			HELP_ctxt_properties_menu },
    { "Help",	  help_pane,	'H', sizeof(help_pane)/sizeof(Menu_table) },
};

Context_window::Context_window(Window_set *ws) : BASE_WINDOW(ws, BW_context)
{
	Expansion_box	*box1;
	Divided_box	*box2;

	setup_panes_dialog = 0;
	sym_class_dialog = 0;
	stop_on_function_dialog = 0;
	dump_dialog = 0;
	map_dialog = 0;
	expand_dialog = 0;
	stype = SEL_none;

	window = new Window_shell("Context", 0, this, HELP_context_window);
	box1 = new Expansion_box(window, "context_expansion_box", OR_vertical);
	menu_bar = new Menu_bar(box1, this, window_set, menu_table,
		sizeof(menu_table)/sizeof(Menu_bar_table));
	box1->add_component(menu_bar);
	box2 = new Divided_box(box1, "context_divided_box");

	ps_pane = new Ps_pane(ws, box2);
	stack_pane = new Stack_pane(ws, box2);
	syms_pane = new Symbols_pane(ws, box2);

	box1->add_elastic(box2);
	window->add_component(box1);
}

Context_window::~Context_window()
{
	delete window;
	delete setup_panes_dialog;
	delete sym_class_dialog;
	delete ps_pane;
	delete stack_pane;
	delete syms_pane;
	delete stop_on_function_dialog;
	delete dump_dialog;
	delete map_dialog;
	delete expand_dialog;
}

void
Context_window::add_ws(int ws_id)
{
	Menu *mp;
	char buf[sizeof("Window set ")+MAX_INT_DIGITS+1];
	Menu_table item;

	// add an item to Windows menu
	mp = menu_bar->find_item("File");
	mp = mp->find_item("Windows");
	sprintf(buf, "Window set %d", ws_id);
	item.label = makestr(buf);
	item.mnemonic = 0;
	item.flags = Window_cb;
	item.sensitivity = SEN_always;
	item.callback = (Callback_ptr)(&Context_window::popup_ws);
	item.help_msg = HELP_none;
	item.cdata = ws_id;
	item.accelerator = NULL;
	mp->add_item(&item);
}

void
Context_window::popup_ws(Component *, void *data)
{
	int ws_id = (int)data;

        // popup context window of window set with id 'ws_id'
	for (Window_set *ws = (Window_set *)windows.first(); 
		ws != NULL; ws = (Window_set *)windows.next())
	{
                if (ws->get_id() == ws_id)
		{
                        ws->popup_context();
                        return;
                }
        }
}

void
Context_window::delete_ws(int ws_id)
{
	Menu *mp;
	char buf[sizeof("Window set ")+MAX_INT_DIGITS+1];

	// remove an item from Windows menu
	mp = menu_bar->find_item("File");
	mp = mp->find_item("Windows");
	sprintf(buf, "Window set %d", ws_id);
	mp->delete_item(buf);
}

void
Context_window::popup()
{
	if (_is_open) 
	{
		window->raise();
		return;
	}

	_is_open = 1;
	window->popup();
	stack_pane->popup();
	syms_pane->popup();
}

void
Context_window::popdown()
{
	_is_open = 0;
	window->popdown();
	stack_pane->popdown();
	syms_pane->popdown();
}

void
Context_window::set_current(Component *, void *)
{
	if (stype == SEL_frame)
		stack_pane->set_current();
	else if (stype == SEL_process)
	{
		Process **p = ps_pane->get_selections();
		window_set->set_current(p[0]);
	}
}

void
Context_window::setup_panes_cb(Component *, void *)
{
	if (!setup_panes_dialog)
		setup_panes_dialog = new Context_dialog(this);
	setup_panes_dialog->display();
}

void
Context_window::setup_syms_cb(Component *, void *)
{
	if (!sym_class_dialog)
		sym_class_dialog = new Symbol_class_dialog(syms_pane);
	sym_class_dialog->display();
}

void
Context_window::new_window_set_cb(Component *, void *)
{
	Window_set	*ws = new Window_set();

	if (stype != SEL_process)
		return;

	int	total = ps_pane->get_total();
	Process	**p = ps_pane->get_selections();

	for (int i = 0; i < total; i++)
		proclist.move_process(p[i], ws);
}

void
Context_window::stop_on_function_cb(Component *, void *)
{
	if (!stop_on_function_dialog)
		stop_on_function_dialog = new Stop_on_function_dialog(this);
	stop_on_function_dialog->set_plist(this, window_set->get_event_level());
	stop_on_function_dialog->display();
}

Selection_type
Context_window::selection_type()
{
	return stype;
}

int
Context_window::check_sensitivity(int sense)
{
	// applies to Recreate button only, and only thing affecting Recreate is create_args
	if (sense & SEN_proc_created)
		return (create_args != 0);

	if (sense & SEN_process)
	{
		if (stype == SEL_process)
		{
			// applies to selected processes
			int	total = ps_pane->get_total();
			Process	**plist = ps_pane->get_selections();

			// must have a single process selected?
			if ((sense&SEN_proc_single) && total > 1)
				return 0;
			for (int i = 0; i < total; i++, plist++)
			{
				if (!(*plist)->check_sensitivity(sense))
					return 0;
			}
		}
		else
		{
			// applies to current process if none selected
			if (!window_set->current_process())
				return 0;
			if (!window_set->current_process()->check_sensitivity(sense))
				return 0;
		}
	}
	if (sense & SEN_frame)
	{
		// applies to "Set Current" button only
		if (stype != SEL_frame && stype != SEL_process)
			return 0;
	}

	if (sense & (SEN_user_symbol|SEN_program_symbol))
	{
		// applies to selected symbols only
		if (stype != SEL_symbol)
			return 0;

		int	total = syms_pane->get_total();
		char	**sym_list = syms_pane->get_selections();

		// if the selected symbols are inappropriate
		// then the button is insensitive
		for (int i = 0; i < total; i++, sym_list++)
		{
			char c = (*sym_list)[0];
			if (sense & SEN_user_symbol)
			{ 
				if (c != '$')
					return 0;
			}
			else if (c == '$' || c == '%')
				return 0;
		}
	}

	if (stype != SEL_none)
	{
		// make sure all buttons that don't apply to
		// the selected object are insensitive

		if (stype == SEL_frame
			&& !(sense&(SEN_frame|SEN_process)))
			return 0;
		if (stype == SEL_symbol)
		{
			if (!(sense & (SEN_symbol|SEN_process|SEN_user_symbol|SEN_single_symbol)))
				return 0;
			if (sense&SEN_single_symbol
				&& syms_pane->get_total() > 1)
				return 0;
		}
	}

	return 1;
}

void
Context_window::set_selection(Selection_type sel)
{
	if (stype != SEL_none && stype != sel)
	{
		switch(stype)
		{
		case SEL_process:	ps_pane->deselect();	break;
		case SEL_frame:		stack_pane->deselect();	break;
		case SEL_symbol:	syms_pane->deselect();	break;
		default:					break;
		}
	}
	stype = sel;
	set_sensitivity();
}

void
Context_window::export_syms_cb(Component *, void *)
{
	syms_pane->export_symbols();
}

void
Context_window::dump_dialog_cb(Component *, void *)
{
	if (!dump_dialog)
		dump_dialog = new Dump_dialog(window_set);
	if (stype == SEL_symbol)
		dump_dialog->set_location(syms_pane->get_selections()[0]);
	dump_dialog->set_plist(this, window_set->get_command_level());
	dump_dialog->clear();
	dump_dialog->display();
}

void
Context_window::map_dialog_cb(Component *, void *)
{
	if (!map_dialog)
		map_dialog = new Map_dialog(window_set);

	map_dialog->set_plist(this, window_set->get_command_level());
	map_dialog->do_map();
	map_dialog->display();
}

void
Context_window::set_watchpoint_cb(Component *, void *)
{
	if (stype != SEL_symbol)
		return;

	char	**syms = syms_pane->get_selections();
	int	total = syms_pane->get_total();

	for (int i = 0; i < total; i++)
	{
		if (*syms[i] == '$' || *syms[i] == '%')
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			continue;
		}
		Process *proc = window_set->current_process();
		dispatcher.send_msg(this, proc->get_id(),
			"stop -p %s *%s\n", 
			window_set->get_event_level() == PROGRAM_LEVEL ? 
				proc->get_program()->get_name() : 
				proc->get_name(), 
			syms[i]);
	}
}

void
Context_window::expand_dialog_cb(Component *, void *)
{
	if (!expand_dialog)
		expand_dialog = new Expand_dialog(window_set);

	expand_dialog->set_plist(this, window_set->get_command_level());
	expand_dialog->set_expression(syms_pane->get_selections()[0]);
	expand_dialog->display();
}
