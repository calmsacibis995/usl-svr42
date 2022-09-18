#ident	"@(#)debugger:gui.d/common/Event_dlg.C	1.10"

// GUI headers
#include "Context.h"
#include "Dialogs.h"
#include "Boxes.h"
#include "Caption.h"
#include "Dialog_sh.h"
#include "Sel_list.h"
#include "Toggle.h"
#include "Table.h"
#include "Text_line.h"
#include "Text_area.h"
#include "Stext.h"
#include "Radio.h"
#include "Dispatcher.h"
#include "Events.h"
#include "Windows.h"
#include "Ps_pane.h"
#include "Proclist.h"
#include "UI.h"
#include "Eventlist.h"

// debug headers
#include "str.h"
#include "Buffer.h"
#include "Machine.h"
#include "Vector.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>

// buffer1 is not declared in any header file because of conflicts with
// the graphics headers
extern Buffer		buffer1;
static const char	*order_buttons[] = { "Name", "Number" };

// strip the open ('{') and close ('}') curly braces off an event's command list
// before displaying the list in a Change event dialog
static char *
strip_curlies(const char *s)
{
	char	*ptr;
	char	*first = 0;
	char	*last = 0;

	if (!s)
		return 0;

	vscratch1.clear();
	vscratch1.add((char *)s, strlen(s)+1);
	ptr = (char *)vscratch1.ptr();

	first = strchr(ptr, '{');
	last = strrchr(ptr, '}');

	if (first != 0 && last != 0)
	{
		*last = '\0';
		return ptr+1;
	}
	else	
		return ptr;
}

// Set the Signal or Syscall selection list from the event condition
// This function is called to initialize the Change dialogs
static void
set_selections(const char *condition, Selection_list *list, int nentries)
{
	// deselect the old selections
	int	*sels;
	int	nsels = list->get_selections(sels);
	for (; nsels; sels++, nsels--)
		list->deselect(*sels);

	// use the condition string to select the syscall list
	vscratch1.clear();
	vscratch1.add((char *)condition, strlen(condition)+1);
	char *ptr = strtok((char *)vscratch1.ptr(), " ,");
	
	while (ptr != NULL)
	{
		if (strlen(ptr) != 0)
		{
			for (int i= 0; i < nentries; i++)
			{
				if (strcmp(ptr, list->get_item(i, 0)) == 0)
				{
					list->select(i);
					break;
				}
			}
		}
		ptr = strtok(NULL, " ,");
	}
}

Stop_dialog::Stop_dialog(Window_set *ws) : PROCESS_DIALOG(ws)
{
	static const Button	buttons[] =
	{
		{ B_ok,	    "Stop",  'S', (Callback_ptr)Stop_dialog::apply},
		{ B_apply,   0, 0,  	  (Callback_ptr)Stop_dialog::apply},
		{ B_cancel,  0, 0, (Callback_ptr)Stop_dialog::reset },
		{ B_help,    0, 0, 0 },
	};

	Packed_box	*box;
	Caption		*caption;

	save_expr = save_cmd = save_count = 0;
	doChange = 0;
	eventId = 0;

	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
		"Stop", (Callback_ptr)Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_stop_dialog);
	box = new Packed_box(dialog, "stop box", OR_vertical);
	dialog->add_component(box);

	process_caption = new Caption(box, "Programs:", CAP_LEFT);
	process_list = new Simple_text(process_caption, "", TRUE);
	process_caption->add_component(process_list);
	box->add_component(process_caption);

	caption = new Caption(box, "Expression: ", CAP_LEFT);
	expr = new Text_line(caption, "expression", "", 25, 1);
	caption->add_component(expr);
	box->add_component(caption);

	caption = new Caption(box, "Commands: ", CAP_LEFT);
	commands = new Text_line(caption, "commands", "", 25, 1);
	caption->add_component(commands);
	box->add_component(caption);

	caption = new Caption(box, "Count:       ", CAP_LEFT);
	count = new Text_line(caption, "count", "1", 8, 1);
	caption->add_component(count, FALSE);
	box->add_component(caption);
}

// Create the stop event
void
Stop_dialog::apply(Component *, int)
{
	char		*cmd;
	char		*stop_expr;
	char		*cnt;

	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	cmd = commands->get_text();
	stop_expr = expr->get_text();
	cnt = count->get_text();

	if (!stop_expr || !*stop_expr)
	{
		dialog->error(E_ERROR, GE_no_stop_expr);
		return;
	}

	// change cmd always preserves the process list of the event
	if (doChange)
	{
		if (!cnt || !*cnt)
			cnt = "1";
		if (cmd && *cmd)
			dispatcher.send_msg(this, window_set->current_process()->get_id(),
				"change %d -c%s %s {%s}\n", eventId, cnt,
				stop_expr, cmd);
		else
			dispatcher.send_msg(this, window_set->current_process()->get_id(),
				"change %d -c%s %s\n", eventId, cnt, stop_expr);
	}
	else
	{
		// save values for reset
		delete save_cmd;
		delete save_expr;
		delete save_count;
		save_count = 0;
		save_cmd = 0;
		save_expr = makestr(stop_expr);
		if (cmd && *cmd)
			save_cmd = makestr(cmd);

		// a blank count is equivalent to count of 1
		if (cnt && *cnt)
			save_count = makestr(cnt);
		else
			cnt = "1";

		//create event, rather than change event
		//	In context window, it uses all the selected processes
		//	In other windows, it uses the current process
		const char *	plist = make_plist(total, processes, 0, level);
	
		if (save_cmd)
			dispatcher.send_msg(this, window_set->current_process()->get_id(),
				"stop -p %s -c%s %s {%s}\n", plist, cnt,
				stop_expr, cmd);
		else
			dispatcher.send_msg(this, window_set->current_process()->get_id(),
				"stop -p %s -c%s %s\n", plist, cnt, stop_expr);
	}

	dialog->wait_for_response();
}

// fillContents is used by the Change dialog to initialize the components
void
Stop_dialog::fillContents(Event *event)
{
	char	buf[MAX_INT_DIGITS+1];

	expr->set_text(event->get_condition());

	if (((Stop_event *)event)->get_count())
	{
		sprintf(buf, "%d", ((Stop_event *)event)->get_count());
		count->set_text(buf);
	}
	else
		count->set_text("1");
	commands->set_text(strip_curlies(event->get_commands()));
}

void
Stop_dialog::reset(Component *, int)
{
	if (doChange)
	{
		Event	*event = event_list.findEvent(eventId);
		if (event)
			fillContents(event);
		else
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}
	else
	{
		expr->set_text(save_expr);
		commands->set_text(save_cmd);
		count->set_text(save_count);
	}
}

// set_expression is called from the menu button callback to initialize
// the expression with a location if a line is selected in the Source or
// Disassembly windows
void
Stop_dialog::set_expression(const char *s)
{
	delete save_expr;
	save_expr = s ? makestr(s) : 0;
	expr->set_text(save_expr);
}

Signal_dialog::Signal_dialog(Window_set *ws) : PROCESS_DIALOG(ws)
{
	static const Button	buttons[] =
	{
		{ B_ok,	    "Signal",  'S', (Callback_ptr)Signal_dialog::apply },
		{ B_apply,   0, 0,  	    (Callback_ptr)Signal_dialog::apply },
		{ B_cancel,  0,	0, (Callback_ptr)Signal_dialog::reset },
		{ B_help,    0,	0, 0 },
	};

	Expansion_box	*ebox;
	Expansion_box	*ebox2;
	Caption		*caption;

	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
		"Signal", (Callback_ptr)Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_signal_dialog);
	ebox = new Expansion_box(dialog, "syscall box", OR_vertical);

	process_caption = new Caption(ebox, "Programs:", CAP_LEFT);
	process_list = new Simple_text(process_caption, "", TRUE);
	process_caption->add_component(process_list);
	ebox->add_component(process_caption);

	ebox2 = new Expansion_box(ebox, "syscall box", OR_horizontal);
	ebox->add_elastic(ebox2);
	dialog->add_component(ebox);

	order = Numeric;
	siglist = new Selection_list(ebox2, "signal list", 5, 2, "%s %-s", Nsignals,
		get_signals(order), SM_multiple);
	ebox2->add_component(siglist);

	caption = new Caption(ebox2, "Order list by", CAP_TOP_CENTER);
	ordering = new Radio_list(caption, "order", OR_vertical, order_buttons,
		sizeof(order_buttons)/sizeof(char *), 1,
		(Callback_ptr)Signal_dialog::set_order, this);
	caption->add_component(ordering, FALSE);
	ebox2->add_elastic(caption);

	caption = new Caption(ebox, "Commands:", CAP_LEFT);
	commands = new Text_line(caption, "commands", "", 20, 1);
	caption->add_component(commands);
	ebox->add_component(caption);

	doChange = 0;
	eventId  = 0;
	save_cmd = 0;
	nselections = 0;
	selections = 0;
}

// Create the Signal event
void
Signal_dialog::apply(Component *, int)
{
	char		*cmd;
	int		*signals = 0;
	int		nsigs;
	int		i;

	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	nsigs = siglist->get_selections(signals);
	if (!nsigs)
	{
		dialog->error(E_ERROR, GE_no_signals);
		return;
	}

	cmd = commands->get_text();
	if (!cmd || !*cmd)
	{
		dialog->error(E_ERROR, GE_no_cmd_list);
		return;
	}

	buffer1.clear();
	for (i = 0; i < nsigs; i++)
	{
		if (i > 0)
			buffer1.add(' ');
		buffer1.add(siglist->get_item(signals[i], 0));
	}

	if (doChange)
		dispatcher.send_msg(this, window_set->current_process()->get_id(),
			"change %d %s {%s}\n", eventId, (char *)buffer1, cmd);
	else
	{
		// save values for reset
		delete save_cmd;
		save_cmd = makestr(cmd);
		order = (Order)ordering->which_button();
		delete selections;
		nselections = nsigs;
		selections = new int[nsigs];
		memcpy(selections, signals, sizeof(int) * nsigs);

		dispatcher.send_msg(this, window_set->current_process()->get_id(),
			"signal -p %s %s {%s}\n", make_plist(total, processes, 0, level),
			(char *)buffer1, cmd);
	}
	dialog->wait_for_response();
}

void
Signal_dialog::set_order(Component *, Order o)
{
	siglist->set_list(Nsignals, get_signals(o));
	if (doChange)
	{
		Event	*event = event_list.findEvent(eventId);
		if (event)
			set_selections(event->get_condition(), siglist, Nsignals);
		else
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}
}

void
Signal_dialog::reset(Component *, int)
{
	int	i;
	if (ordering->which_button() != order)
	{
		ordering->set_button(order);
		siglist->set_list(Nsignals, get_signals(order));
	}

	if (doChange)
	{
		Event	*event = event_list.findEvent(eventId);
		if (event)
			fillContents(event);
		else
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}
	else
	{
		if (ordering->which_button() == order)
		{
			int	*sig;
			int	nsig = siglist->get_selections(sig);
			for (i = 0; i < nsig; i++)
				siglist->deselect(sig[i]);
		}

		for (i = 0; i < nselections; i++)
			siglist->select(selections[i]);
		commands->set_text(save_cmd);
	}
}

void
Signal_dialog::fillContents(Event *event)
{
	// use the condition string to select the signal list
	set_selections(event->get_condition(), siglist, Nsignals);
	commands->set_text(strip_curlies(event->get_commands()));
}

// Combinations of Entry and Exit checkboxes
#define	NONE_SET	0
#define	ENTRY_SET	1
#define EXIT_SET	2
#define BOTH_SET	3

Syscall_dialog::Syscall_dialog(Window_set *ws) : PROCESS_DIALOG(ws)
{
	static const Button	buttons[] =
	{
		{ B_ok,	    "Syscall",  'S', (Callback_ptr)(&Syscall_dialog::apply) },
		{ B_apply,   0, 0,  	     (Callback_ptr)(&Syscall_dialog::apply) },
		{ B_cancel,  0,	0, (Callback_ptr)(&Syscall_dialog::reset) },
		{ B_help,    0,	0, 0 },
	};

	static const Toggle_data toggles[] =
	{
		{ "Entry",	TRUE,	0 },
		{ "Exit",	FALSE,	0 },
	};

	Packed_box	*pbox;
	Expansion_box	*ebox;
	Expansion_box	*ebox2;
	Caption		*caption;
	const char	**list;

	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
		"Syscall", (Callback_ptr)Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_syscall_dialog);
	ebox = new Expansion_box(dialog, "syscall box", OR_vertical);

	process_caption = new Caption(ebox, "Programs:", CAP_LEFT);
	process_list = new Simple_text(process_caption, "", TRUE);
	process_caption->add_component(process_list);
	ebox->add_component(process_caption);

	ebox2 = new Expansion_box(ebox, "syscall box", OR_horizontal);
	ebox->add_elastic(ebox2);

	order = Alpha;
	list = get_syslist(order);
	syslist = new Selection_list(ebox2, "syscall list", 7, 2, "%s %-s",
		Nsyscalls, list, SM_multiple);
	ebox2->add_component(syslist);

	pbox = new Packed_box(ebox2, "syscall box", OR_vertical);
	ebox2->add_elastic(pbox);
	caption = new Caption(pbox, "Order list by", CAP_TOP_CENTER);
	ordering = new Radio_list(caption, "order", OR_vertical, order_buttons,
		sizeof(order_buttons)/sizeof(char *), 0,
		(Callback_ptr)Syscall_dialog::set_order, this);
	caption->add_component(ordering);
	pbox->add_component(caption);

	caption = new Caption(ebox, "Stop on:     ", CAP_LEFT);
	entry_exit = new Toggle_button(caption, "entry or exit", toggles,
		sizeof(toggles)/sizeof(Toggle_data), OR_horizontal, this);
	caption->add_component(entry_exit, FALSE);
	ebox->add_component(caption);

	caption = new Caption(ebox, "Commands:", CAP_LEFT);
	commands = new Text_line(caption, "commands", "", 20, TRUE);
	caption->add_component(commands);
	ebox->add_component(caption);

	caption = new Caption(ebox, "Count:      ", CAP_LEFT);
	count = new Text_line(caption, "count", "1", 4, TRUE);
	caption->add_component(count, FALSE);
	ebox->add_component(caption);

	dialog->add_component(ebox);

	doChange = 0;
	eventId  = 0;
	save_count = save_cmd = 0;
	nselections = 0;
	selections = 0;
	entry_exit_state = NONE_SET;
}

void
Syscall_dialog::apply(Component *, int)
{
	char		*cmd;
	char		*cnt;
	int		*syscalls = 0;
	int		nsys;
	int		i;
	const char	*ex;
	int		toggle_state;

	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	nsys = syslist->get_selections(syscalls);
	if (!nsys)
	{
		dialog->error(E_ERROR, GE_no_sys_calls);
		return;
	}

	if (entry_exit->is_set(0) && entry_exit->is_set(1))
	{
		toggle_state = BOTH_SET;
		ex = "-ex";
	}
	else if (entry_exit->is_set(0))
	{
		toggle_state = ENTRY_SET;
		ex = "-e";
	}
	else if (entry_exit->is_set(1))
	{
		toggle_state = EXIT_SET;
		ex = "-x";
	}
	else
	{
		dialog->error(E_ERROR, GE_entry_exit);
		return;
	}

	cmd = commands->get_text();
	cnt = count->get_text();

	buffer1.clear();
	for (i = 0; i < nsys; i++)
	{
		if (i > 0)
			buffer1.add(' ');
		buffer1.add(syslist->get_item(syscalls[i], 0));
	}

	if (doChange)
	{
		if (!cnt || !*cnt)
			cnt = "1";
		if (cmd && *cmd)
			dispatcher.send_msg(this, window_set->current_process()->get_id(),
				"change %d -c%s %s %s {%s}\n", eventId, cnt, ex,
				(char *)buffer1, cmd);
		else
			dispatcher.send_msg(this, window_set->current_process()->get_id(),
				"change %d -c%s %s %s\n", eventId, cnt, ex,
				(char *)buffer1);
	}
	else
	{
		// save state for reset
		nselections = nsys;
		delete selections;
		selections = new int[nsys];
		memcpy(selections, syscalls, sizeof(int) * nsys);
	
		delete save_cmd;
		if (cmd && *cmd)
			save_cmd = makestr(cmd);
		else
			save_cmd = 0;

		delete save_count;
		if (cnt && *cnt)
			save_count = makestr(cnt);
		else
		{
			save_count = 0;
			cnt = "1";
		}
		entry_exit_state = toggle_state;

		if (cmd && *cmd)
			dispatcher.send_msg(this, window_set->current_process()->get_id(),
				"syscall -p %s -c%s %s %s {%s}\n",
				make_plist(total, processes, 0, level),
				cnt, ex, (char *)buffer1, cmd);
		else
			dispatcher.send_msg(this, window_set->current_process()->get_id(),
				"syscall -p %s -c%s %s %s\n",
				make_plist(total, processes, 0, level),
				cnt, ex, (char *)buffer1);
		order = (Order)ordering->which_button();
	}

	dialog->wait_for_response();
}

void
Syscall_dialog::set_order(Component *, Order o)
{
	syslist->set_list(Nsyscalls, get_syslist(o));
	if (doChange)
	{
		Event	*event = event_list.findEvent(eventId);
		if (event)
			set_selections(event->get_condition(), syslist, Nsyscalls);
		else
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}
}

void
Syscall_dialog::reset(Component *, int)
{
	int i;

	if (ordering->which_button() != order)
	{
		ordering->set_button(order);
		syslist->set_list(Nsyscalls, get_syslist(order));
	}

	if (doChange)
	{
		Event	*event = event_list.findEvent(eventId);
		if (event)
			fillContents(event);
		else
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}
	else
	{
		if (ordering->which_button() == order)
		{
			int	*sys;
			int	nsys = syslist->get_selections(sys);
			for (i = 0; i < nsys; i++)
				syslist->deselect(sys[i]);
		}

		for (i = 0; i < nselections; i++)
			syslist->select(selections[i]);
		commands->set_text(save_cmd);
		count->set_text(save_count);
		if (entry_exit_state == BOTH_SET)
		{
			entry_exit->set(0, 1);
			entry_exit->set(1, 1);
		}
		else if (entry_exit_state == ENTRY_SET)
		{
			entry_exit->set(0, 1);
			entry_exit->set(1, 0);
		}
		else
		{
			entry_exit->set(0, 0);
			entry_exit->set(1, 1);
		}
	}
}

void
Syscall_dialog::fillContents(Event *event)
{
	char	s[MAX_INT_DIGITS+1];
	char	*ptr;

	set_selections(event->get_condition(), syslist, Nsyscalls);
	commands->set_text(strip_curlies(event->get_commands()));
	if (((Syscall_event *)event)->get_count())
	{
		sprintf(s, "%d", ((Syscall_event *)event)->get_count());
		count->set_text(s);
	}
	else
		count->set_text("1");
	
	// exit or enter	
	ptr = strchr(event->get_type_string(), ' ');
	if (ptr && *++ptr)
	{
		if (*ptr == 'E')
		{
			entry_exit->set(0, 1);
			ptr++;
		}
		else
			entry_exit->set(0, 0);

		if (*ptr == 'X')
			entry_exit->set(1, 1);
		else
			entry_exit->set(1, 0);
	}
}

Onstop_dialog::Onstop_dialog(Window_set *ws) : PROCESS_DIALOG(ws)
{
	static const Button	buttons[] =
	{
		{ B_ok,	    "On Stop",  'O', (Callback_ptr)Onstop_dialog::apply },
		{ B_apply,   0, 0,  	     (Callback_ptr)Onstop_dialog::apply },
		{ B_cancel,  0,	0, (Callback_ptr)Onstop_dialog::reset },
		{ B_help,    0,	0, 0 },
	};

	Expansion_box	*box;
	Caption		*caption;

	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
		"On Stop", (Callback_ptr)Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_on_stop_dialog);
	box = new Expansion_box(dialog, "onstop box", OR_vertical);
	dialog->add_component(box);

	process_caption = new Caption(box, "Programs:", CAP_LEFT);
	process_list = new Simple_text(process_caption, "", TRUE);
	process_caption->add_component(process_list);
	box->add_component(process_caption);

	caption = new Caption(box, "Commands:", CAP_TOP_LEFT);
	commands = new Text_area(caption, "commands", 4, 40);
	caption->add_component(commands);
	box->add_elastic(caption);
	
	doChange = 0;
	eventId  = 0;
	save_cmds = 0;
}

void
Onstop_dialog::apply(Component *, int)
{
	char	*cmd = commands->get_text();

	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	if (!cmd || !*cmd)
	{
		dialog->error(E_ERROR, GE_no_cmd_list);
		return;
	}

	if (doChange)
		dispatcher.send_msg(this, window_set->current_process()->get_id(),
			"change %d {%s}\n", eventId, cmd);
	else
	{
		// save state for reset
		delete save_cmds;
		save_cmds = makestr(cmd);

		dispatcher.send_msg(this, window_set->current_process()->get_id(),
			"onstop -p %s {%s}\n", make_plist(total, processes, 0, level),
			cmd);
	}
	dialog->wait_for_response();
}

void
Onstop_dialog::reset(Component *, int)
{
	if (doChange)
	{
		Event	*event = event_list.findEvent(eventId);
		if (event)
			fillContents(event);
		else
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
	}
	else
	{
		commands->clear();
		commands->add_text(save_cmds);
	}
}

void
Onstop_dialog::fillContents(Event *event)
{
	commands->clear();
	commands->add_text(strip_curlies(event->get_commands()));
}

Cancel_dialog::Cancel_dialog(Window_set *ws) : PROCESS_DIALOG(ws)
{
	static const Button	buttons[] =
	{
		{ B_apply,	    "Cancel Signal",  'S', (Callback_ptr)Cancel_dialog::apply },
		{ B_cancel,  0,	0, (Callback_ptr)Cancel_dialog::cancel },
		{ B_help,    0,	0, 0 },
	};

	Expansion_box	*box1;
	Expansion_box	*box2;
	Caption		*caption;
	char		*signals[2];

	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
		"Cancel", (Callback_ptr)Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_cancel_dialog);
	box1 = new Expansion_box(dialog, "cancel box", OR_vertical);
	dialog->add_component(box1);

	process_caption = new Caption(box1, "Programs:", CAP_LEFT);
	process_list = new Simple_text(process_caption, "", TRUE);
	process_caption->add_component(process_list);
	box1->add_component(process_caption);

	box2 = new Expansion_box(box1, "cancel box", OR_horizontal);
	box1->add_elastic(box2);

	// Temporarily initialize the list with a blank entry.
	// The list will be set correctly later in set_process
	order = Alpha;
	signals[0] = signals[1] = "";
	siglist = new Selection_list(box2, "signal list", 5, 2, "%s %-s", 1,
		signals, SM_multiple);
	box2->add_component(siglist);

	caption = new Caption(box2, "Order list by", CAP_TOP_CENTER);
	ordering = new Radio_list(caption, "order", OR_vertical, order_buttons,
		sizeof(order_buttons)/sizeof(char *), 1,
		(Callback_ptr)Cancel_dialog::set_order, this);
	caption->add_component(ordering, FALSE);
	box2->add_elastic(caption);
}

void
Cancel_dialog::set_order(Component *, Order o)
{
	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	const char	**signals;
	int		num_sigs;

	signals = processes[0]->get_pending_signals(o, num_sigs);
	siglist->set_list(num_sigs, signals);
}

void
Cancel_dialog::apply(Component *, int)
{
	int		*signos = 0;
	int		nsigs;// # of signals to cancel

	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	nsigs = siglist->get_selections(signos);
	if (!nsigs)
	{
		dialog->error(E_ERROR, GE_no_signals);
		return;
	}
	nselections = nsigs;
	delete selections;
	selections = new int[nsigs];
	memcpy(selections, signos, sizeof(int) *nsigs);

	buffer1.clear();
	for (int i = 0; i < nsigs; i++)
	{
		buffer1.add(siglist->get_item(signos[i], 0));
		buffer1.add(' ');
	}

	dispatcher.send_msg(this, processes[0]->get_id(), "cancel %s\n", (char *)buffer1);
	dialog->wait_for_response();
	order = (Order)ordering->which_button();
}

void
Cancel_dialog::cancel(Component *, int)
{
	if (!dialog->is_pinned())
		return;

	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	const char	**signals;
	int		npending;

	ordering->set_button(order);
	signals = processes[0]->get_pending_signals(order, npending);
	siglist->set_list(npending, signals);
}

void
Cancel_dialog::cmd_complete()
{
	if (dialog->is_pinned())
	{
		const char	**signals;
		int		npending;

		signals = processes[0]->get_pending_signals(order, npending);
		siglist->set_list(npending, signals);
	}
	dialog->cmd_complete();
}

void
Cancel_dialog::set_process(Boolean reset)
{
	const char	**signals;
	int		num_sigs;

	if (reset)
		dialog->set_busy(TRUE);

	if (!processes)
		siglist->set_list(0, 0);
	else
	{
		signals = processes[0]->get_pending_signals(order, num_sigs);
		siglist->set_list(num_sigs, signals);
	}
	if (reset)
		dialog->set_busy(FALSE);
}

Kill_dialog::Kill_dialog(Window_set *ws) : PROCESS_DIALOG(ws)
{
	static const Button	buttons[] =
	{
		{ B_apply,   "Kill",  'K', (Callback_ptr)Kill_dialog::apply },
		{ B_cancel,  0, 0, (Callback_ptr)Kill_dialog::cancel },
		{ B_help,    0,	0, 0 },
	};

	Expansion_box	*box1;
	Expansion_box	*box2;
	Caption		*caption;
	const char	**signals;

	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
		"Kill", (Callback_ptr)Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_kill_dialog);
	box1 = new Expansion_box(dialog, "kill box", OR_vertical);

	dialog->add_component(box1);
	process_caption = new Caption(box1, "Programs:", CAP_LEFT);
	process_list = new Simple_text(process_caption, "", TRUE);
	process_caption->add_component(process_list);
	box1->add_component(process_caption);

	box2 = new Expansion_box(box1, "kill box", OR_horizontal);
	box1->add_elastic(box2);

	order = Numeric;
	signals = get_signals(order);
	siglist = new Selection_list(box2, "signal list", 9, 2, "%s %-s", Nsignals,
		signals, SM_single);
	box2->add_component(siglist);

	// come up with kill selected
	siglist->select(SIGKILL-1);	// -1 since list is 0-based
	selection = makestr("kill");

	caption = new Caption(box2, "Order list by", CAP_TOP_CENTER);
	ordering = new Radio_list(caption, "order", OR_vertical, order_buttons,
		sizeof(order_buttons)/sizeof(char *), 1,
		(Callback_ptr)Kill_dialog::set_order, this);
	caption->add_component(ordering, FALSE);
	box2->add_elastic(caption);

}

void
Kill_dialog::set_order(Component *, Order o)
{
	const char	**signals;

	signals = get_signals(o);
	siglist->set_list(Nsignals, signals);
	for (int i = 0; i < Nsignals; i++)
	{
		if (strcmp(siglist->get_item(i, 0), selection) == 0)
		{
			siglist->select(i);
			break;
		}
	}
}

void
Kill_dialog::apply(Component *, int)
{
	int	*signos = 0;
	int	nsigs;//for SM_single case, this should always be 1

	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	nsigs = siglist->get_selections(signos);
	if (!nsigs)
	{
		dialog->error(E_ERROR, GE_no_signals);
		return;
	}
	selection = makestr(siglist->get_item(signos[0], 0));
	
	dispatcher.send_msg(this, processes[0]->get_id(), "kill -p %s %s\n",
		make_plist(total, processes, 0, level), selection);
	order = (Order)ordering->which_button();
	dialog->wait_for_response();
}

void
Kill_dialog::cancel(Component *c, int)
{
	ordering->set_button(order);
       	set_order(c, order);
}

Setup_signals_dialog::Setup_signals_dialog(Window_set *ws) : PROCESS_DIALOG(ws)
{
	static const Button	buttons[] =
	{
		{ B_apply,   "Ignore", 'I', (Callback_ptr)Setup_signals_dialog::apply },
		{ B_exec,  "Catch",	'A', (Callback_ptr)Setup_signals_dialog::apply},
		{ B_cancel,  0, 0, (Callback_ptr)Setup_signals_dialog::cancel},
		{ B_help,    0,	0, 0 },
	};

	Expansion_box	*box1;
	Expansion_box	*box2;
	Caption		*caption;

	// Temporarily initialize list with a blank entry.
	// The list will be set correctly later in set_process
	const char	*initial_string = "";

	dialog = new Dialog_shell(ws->get_context_window()->get_window_shell(),
		"Ignore Signals", (Callback_ptr)Process_dialog::dismiss_cb, this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_ignore_signals_dialog);
	box1 = new Expansion_box(dialog, "signals box", OR_vertical);
	dialog->add_component(box1);

	process_caption = new Caption(box1, "Programs:", CAP_LEFT);
	process_list = new Simple_text(process_caption, "", TRUE);
	process_caption->add_component(process_list);
	box1->add_component(process_caption);

	box2 = new Expansion_box(box1, "signals box", OR_horizontal);
	box1->add_elastic(box2);

	order = Numeric;
	siglist = new Selection_list(box2, "signal list", 5, 3, "%s %-s %s",
		1, &initial_string, SM_multiple);
	box2->add_component(siglist);

	caption = new Caption(box2, "Order list by", CAP_TOP_CENTER);
	ordering = new Radio_list(caption, "order", OR_vertical, order_buttons,
		sizeof(order_buttons)/sizeof(char *), 1,
		(Callback_ptr)Setup_signals_dialog::set_order, this);
	caption->add_component(ordering, FALSE);
	box2->add_elastic(caption);
}

void
Setup_signals_dialog::set_order(Component *, Order o)
{
	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	const char **signals = processes[0]->get_signals_with_status(o);
	siglist->set_list(Nsignals, signals);
}

void
Setup_signals_dialog::apply(Component *, int what)
{
	int	*signos = 0;
	int	nsigs;

	dialog->clear_msg();
	if (!processes)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	nsigs = siglist->get_selections(signos);
	if (!nsigs)
	{
		dialog->error(E_ERROR, GE_no_signals);
		return;
	}

	buffer1.clear();
	for (int i = 0; i < nsigs; i++)
	{
		if (i > 0)
			buffer1.add(' ');
		buffer1.add(siglist->get_item(signos[i], 0));
	}

	dispatcher.send_msg(this, processes[0]->get_id(), "signal %s %s\n",
		(what == 'I') ? "-i" : "", (char *)buffer1);
	dialog->wait_for_response();
	order = (Order)ordering->which_button();
}

void
Setup_signals_dialog::set_process(Boolean reset)
{
	if (reset)
		dialog->set_busy(TRUE);
	if (processes)
	{
		const char **signals = processes[0]->get_signals_with_status(
			(Order)ordering->which_button());
		siglist->set_list(Nsignals, signals);
	}
	else
		siglist->set_list(0, 0);
	if (reset)
		dialog->set_busy(FALSE);
}

// If the dialog is pinned, the signal list should be updated to reflect the
// current state.  If the dialog is not pinned, this isn't necessary, since
// the list will be updated anyway the next time the dialog is popped up
void
Setup_signals_dialog::cmd_complete()
{
	if (dialog->is_pinned())
	{
		const char **signals = processes[0]->get_signals_with_status(order);
		siglist->set_list(Nsignals, signals);
	}
	dialog->cmd_complete();
}

void
Setup_signals_dialog::cancel(Component *, int)
{
	ordering->set_button(order);
	if (dialog->is_pinned())
	{
		const char **signals = processes[0]->get_signals_with_status(order);
		siglist->set_list(Nsignals, signals);
	}
}
