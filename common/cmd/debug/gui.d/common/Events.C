#ident	"@(#)debugger:gui.d/common/Events.C	1.19"

// GUI headers
#include "Dispatcher.h"
#include "Dialogs.h"
#include "Dialog_sh.h"
#include "Events.h"
#include "Eventlist.h"
#include "Window_sh.h"
#include "Windows.h"
#include "Context.h"
#include "Menu.h"
#include "Table.h"
#include "Boxes.h"
#include "Proclist.h"
#include "Status.h"
#include "Caption.h"
#include "Radio.h"

// debug headers
#include "Buffer.h"
#include "Machine.h"
#include "Message.h"
#include "str.h"

#include <stdio.h>

extern Buffer	buffer1;

class EventPaneDialog : public Dialog_box
{
	Radio_list	*eventPane;
	Radio_list	*onstopPane;
	int		eventPaneState;
	int		onstopPaneState;
public:
			EventPaneDialog(Event_window *);
			~EventPaneDialog() {};

			// callbacks
	void		apply(Component *, int);
	void		reset(Component *, int);
};
		
EventPaneDialog::EventPaneDialog(Event_window *cw) :
	 DIALOG_BOX(cw->get_window_set())
{
	static const Button	buttons[] =
	{
		{ B_apply,   0, 0, (Callback_ptr)EventPaneDialog::apply },
		{ B_reset,   0, 0, (Callback_ptr)EventPaneDialog::reset },
		{ B_cancel,  0,	0, (Callback_ptr)EventPaneDialog::reset },
		{ B_help,    0,	0, 0 },
	};

	static const char *choices[] =
	{
		"Truncate",
		"Wrap",
	};

	Packed_box	*box;
	Caption		*caption;

	eventPaneState = onstopPaneState = 0;
	dialog = new Dialog_shell(cw->get_window_shell(), "Panes", 0, this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_panes2_dialog);
	box = new Packed_box(dialog, "properties", OR_vertical);

	caption = new Caption(box, "Main Event Pane:", CAP_LEFT);
	eventPane = new Radio_list(caption, "main event pane", OR_horizontal, choices,
		sizeof(choices)/sizeof(char *), 0);
	caption->add_component(eventPane);
	box->add_component(caption);

	caption = new Caption(box, "Onstop Pane:   ", CAP_LEFT);
	onstopPane = new Radio_list(caption, "onstop pane", OR_horizontal, choices,
		sizeof(choices)/sizeof(char *), 0);
	caption->add_component(onstopPane);
	box->add_component(caption);

	dialog->add_component(box);
}

void
EventPaneDialog::apply(Component *, int)
{
	Event_window	*cw = window_set->get_event_window();

	eventPaneState = eventPane->which_button();
	cw->getEventPane()->wrap_columns(eventPaneState);
	onstopPaneState = onstopPane->which_button();
	cw->getOnstopPane()->wrap_columns(onstopPaneState);
}

void
EventPaneDialog::reset(Component *, int)
{
	eventPane->set_button(eventPaneState);
	onstopPane->set_button(onstopPaneState);
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
    { "Windows",	'W', Menu_button, SEN_always,	(Callback_ptr)windows_pane,
					HELP_event_windows_menu,
					sizeof(windows_pane)/sizeof(Menu_table) },
    { DISMISS,	  DISMISS_MNE, Set_cb,  SEN_always,	
					(Callback_ptr)(&Window_set::dismiss),
					HELP_event_dismiss_cmd },
    { "Quit",		  'Q', Set_cb,  SEN_always,	
					(Callback_ptr)(&Window_set::ok_to_quit),
					HELP_event_quit_cmd },
};

static const Menu_table edit_pane[] =
{
	{ "Disable",	's',	Window_cb,	SEN_event_able_sel,
				(Callback_ptr)(&Event_window::disableEventCb),
				HELP_event_disable_cmd },
	{ "Enable",  	'E',	Window_cb,	SEN_event_dis_sel,
				(Callback_ptr)(&Event_window::enableEventCb),
				HELP_event_enable_cmd },
	{ "Delete",	'D',	Window_cb,	SEN_event_sel,
				(Callback_ptr)(&Event_window::deleteEventCb),
				HELP_event_delete_cmd },
};

static const Menu_table prop_pane[] =
{
	{ "Panes...",	'P',	Window_cb,	SEN_always,
					(Callback_ptr)(&Event_window::setupPaneCb),
					HELP_event_pane_dialog },
	{ "Granularity...",'G',	Set_cb,		SEN_always,	
				(Callback_ptr)(&Window_set::set_granularity_cb),
				HELP_event_granularity_dialog  },
};

static const Menu_table event_pane[] =
{
    { "Change...",    	'C',  Window_cb,	SEN_event_change, 
				(Callback_ptr)(&Event_window::changeEventCb),
				HELP_event_change_dialog },
    { "Stop...",	'S',  Set_cb, (SEN_process|SEN_proc_stopped|SEN_event_no_sel),
				(Callback_ptr)(&Window_set::stop_dialog_cb),
				HELP_event_stop_dialog },
    { "Signal...",	'i',  Set_cb, (SEN_process|SEN_proc_stopped|SEN_event_no_sel),
				(Callback_ptr)(&Window_set::signal_dialog_cb),
				HELP_event_signal_dialog },
    { "Syscall...",	'y',  Set_cb, (SEN_process|SEN_proc_stopped|SEN_event_no_sel),
				(Callback_ptr)(&Window_set::syscall_dialog_cb),
				HELP_event_syscall_dialog},
    { "On Stop...",	'O',  Set_cb, (SEN_process|SEN_proc_stopped|SEN_event_no_sel),
				(Callback_ptr)(&Window_set::onstop_dialog_cb),
				HELP_event_onstop_dialog },
    { "Ignore Signals...", 'g',Set_cb,	(SEN_process|SEN_proc_stopped|SEN_proc_single), 
				(Callback_ptr)(&Window_set::setup_signals_dialog_cb),
				HELP_event_ignore_signals },
};

// The number of entries in the help menu is hard-coded because of a
// bug in cfront 2.1
static const Menu_table help_pane[3] =
{
	{ "Event...", 'E',	Set_cb, SEN_always,
				(Callback_ptr)(&Window_set::help_sect_cb)  },
	{ "Table of Contents...", 'T', Set_cb, SEN_always,
				(Callback_ptr)(&Window_set::help_toc_cb) },
	{ "Version",	'V',	Set_cb,	SEN_always,
				(Callback_ptr)(&Window_set::version_cb) },
};

static Menu_bar_table menu_table[] =
{
    { "File",     file_pane,  'F', sizeof(file_pane)/sizeof(Menu_table),
				HELP_event_file_menu },
    { "Edit",     edit_pane,  'E', sizeof(edit_pane)/sizeof(Menu_table),
				HELP_event_edit_menu },
    { "Event",    event_pane, 'n', sizeof(event_pane)/sizeof(Menu_table),
				HELP_event_event_menu },
    { "Properties", prop_pane,'P', sizeof(prop_pane)/sizeof(Menu_table),
				HELP_event_properties_menu },
    { "Help",	  help_pane,  'H', sizeof(help_pane)/sizeof(Menu_table) },
};

enum TableCol { COL_ID, COL_STATE, COL_TYPE, COL_PROC, COL_COND, COL_COUNT,
		COL_COMMAND };

static const Column	event_spec[] =
{
	{ "  ID",		4,	Col_numeric },
	{ "",			1,	Col_text },
	{ "Type",		11,	Col_text },
	{ "Processes",		8,	Col_wrap_text },
	{ "Condition",		24,	Col_wrap_text },
	{ "Count",		5,	Col_numeric },
	{ "Command List",	15,	Col_wrap_text },
};

static const Column	onstop_spec[] =
{ 
	{ "  ID",		4,	Col_numeric },
	{ "",			1,	Col_text },
	{ "Processes",		8,	Col_wrap_text },
	{ "Command List",	45,	Col_wrap_text },
};

Event_window::Event_window(Window_set *ws): BASE_WINDOW(ws, BW_event)
{
	Expansion_box	*box1;
	Divided_box 	*box2;
	int		table_size = sizeof(menu_table)/sizeof(Menu_bar_table);

	max_events = max_onstop = 0;
	window = new Window_shell("Event", 0, this, HELP_event_window);

	box1 = new Expansion_box(window, "box", OR_vertical);
	window->add_component(box1);

	menu_bar= new Menu_bar(box1, this, ws, menu_table, table_size);
	box1->add_component(menu_bar);

	status_pane = new Status_pane(box1);

	box2 = new Divided_box(box1, "box");
	box1->add_elastic(box2);

	events = new Table(box2, "events", SM_multiple, event_spec,
		sizeof(event_spec)/sizeof(Column), 8, FALSE,
		(Callback_ptr)(&Event_window::selectEventCb),
		(Callback_ptr)(&Event_window::deselectEventCb), 0, this);
	box2->add_component(events);
	onstop = new Table(box2, "on stop", SM_multiple, onstop_spec,
		sizeof(onstop_spec)/sizeof(Column), 3, FALSE,
		(Callback_ptr)(&Event_window::selectEventCb),
		(Callback_ptr)(&Event_window::deselectEventCb), 0, this);
	box2->add_component(onstop);

	setupPaneDialog = 0;

	ableEventSel = 0;
	disableEventSel = 0;
}

Event_window::~Event_window()
{
	delete window;
	delete status_pane;
	delete setupPaneDialog;
}

void
Event_window::popup()
{
	if (_is_open)
	{
		window->raise();
		return;
	}

	_is_open = 1;
	window->popup();
	update_cb(0, RC_set_current, 0, window_set->current_process());
	window_set->change_current.add(this, (Notify_func)Event_window::update_cb, 0);
}

void
Event_window::popdown()
{
	_is_open = 0;
	window->popdown();
	window_set->change_current.remove(this, (Notify_func)Event_window::update_cb, 0);
}

int
Event_window::contains(int e)
{
	int i;

	for (i = 0; i < max_events; i++)
	{
		const char	*p = events->get_cell(i, COL_ID).string;

		if (atoi(p) == e) return 1; 
	}

	for (i = 0; i < max_onstop; i++)
	{
		const char	*p = onstop->get_cell(i, COL_ID).string;

		if (atoi(p) == e) return 1; 
	}

	return 0;
}

void
Event_window::setupPaneCb(Component *, Base_window *)
{
	if (!setupPaneDialog)
		setupPaneDialog = new EventPaneDialog(this);
	setupPaneDialog->display();
}

void
Event_window::selectEventCb(Table *table, Table_calldata *row)
{
	const char *state = table->get_cell(row->index, COL_STATE).string;

	if (state && *state == 'D')
		disableEventSel++;
	else
		ableEventSel++;

	set_sensitivity();
}

void
Event_window::deselectEventCb(Table *table, int row)
{
	const char *state = table->get_cell(row, COL_STATE).string;

	if (state && *state == 'D')
		disableEventSel--;
	else
		ableEventSel--;

	set_sensitivity();
}

int
Event_window::check_sensitivity(int sense)
{
	if (sense & SEN_process)
	{
		if (!window_set->current_process() || 
			window_set->current_process()->check_sensitivity(sense)==0)
				return 0;
	}

	switch (sense & SEN_event_only)
	{	
		case SEN_event_change:
			 return ((ableEventSel + disableEventSel) == 1);

		case SEN_event_able_sel: 
			return((ableEventSel > 0) && (disableEventSel == 0)); 
		case SEN_event_dis_sel:
			return((disableEventSel > 0) && (ableEventSel == 0)); 
		
		case SEN_event_sel:
			return ((disableEventSel > 0) || ( ableEventSel > 0)); 

		case SEN_event_no_sel:
			return ((disableEventSel == 0) && ( ableEventSel == 0)); 
	}

	return 1;
}

void
Event_window::add_event(Event *event)
{
	char	id[MAX_INT_DIGITS + 1];
	char	buf[MAX_INT_DIGITS + 1];
	char	state[2];
	int	i;

	sprintf(id, "%d", event->get_id());
	state[0] = state[1] = 0;
	buf[0] = 0;

	if (event->get_state() == ES_disabled)
		state[0] = 'D';

	if (event->get_type() == ET_onstop)
	{
		for (i = 0; i < max_onstop; i++)
		{
			int eid = atoi(onstop->get_cell(i, COL_ID).string);
			if (event->get_id() < eid)
				break;
		}
		onstop->deselect_all();
		onstop->insert_row(i, id, state, event->get_plist(),
			event->get_commands());
		max_onstop++;
	}
	else
	{
		switch(event->get_type())
		{
			case ET_stop:
				if (((Stop_event *)event)->get_count() > 1)
					sprintf(buf,"%d",((Stop_event *)event)->get_count());
				break;
			case ET_syscall:
				if (((Syscall_event *)event)->get_count() > 1)
					sprintf(buf,"%d",((Syscall_event *)event)->get_count());
				break;
		}

		for (i = 0; i < max_events; i++)
		{
			int eid = atoi(events->get_cell(i, COL_ID).string);
			if (event->get_id() < eid)
				break;
		}
		events->deselect_all();
		events->insert_row(i, id, state, 
			event->get_type_string(),
			event->get_plist(),
 			event->get_condition(), 
			buf,
			event->get_commands());
		max_events++;
	}
}

void
Event_window::change_event(Event *p)
{
	Table	*t	= 0;
	int	max	= 0;
	char	buf[MAX_INT_DIGITS + 1];
	char	state[2];

	state[0] = state[1] = 0;
	buf[0] = 0;

	if (p->get_state() == ES_disabled)
		state[0] = 'D';

	switch(p->get_type())
	{
		case ET_stop:
		case ET_signal:
		case ET_syscall:
			t = events;
			max = max_events;
			break;

		case ET_onstop:
			t = onstop;
			max = max_onstop;
			break;

		case ET_none:
		default:
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}
	}

	for (int i = 0; i < max; i++)
	{
		Cell_value item = t->get_cell(i, COL_ID);
		int idInt = atoi(item.string);
	
		if (p->get_id() == idInt)
		{
			const char	*id = item.string;

			if (p->get_type() == ET_onstop)
			{
				t->deselect(i);
				t->set_row(i, id, state, p->get_plist(),
						p->get_commands());
			}
			else
			{
				switch(p->get_type())
				{
					case ET_stop:
						if (((Stop_event *)p)->get_count() > 1)
							sprintf(buf, "%d",((Stop_event *)p)->get_count());
						break;
					case ET_syscall:
						if (((Syscall_event *)p)->get_count() > 1)
							sprintf(buf, "%d",((Syscall_event *)p)->get_count());
						break;
				}

				t->deselect(i);
				t->set_row(i, id, state, p->get_type_string(),
					p->get_plist(),
		 			p->get_condition(), buf,
					p->get_commands());
			}
			break;
		}
	}
}

// find the table 
// find the row
// delete it 
void
Event_window::delete_event(Event *p)
{
	Table	*t	= 0;
	int	*max	= 0;

	switch(p->get_type())
	{
		case ET_stop:
		case ET_signal:
		case ET_syscall:
			t = events;
			max = &max_events;
			break;

		case ET_onstop:
			t = onstop;
			max = &max_onstop;
			break;

		case ET_none:
		default:
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}
	}

	for (int i = 0; i < *max; i++)
	{
		Cell_value item = t->get_cell(i, 0);
		int id = atoi(item.string);
	
		if (p->get_id() == id)
		{
			t->deselect(i);
			t->delete_rows(i, 1);
			*max = (*max) - 1;
			break;
		}
	}
}

void
Event_window::update_cb(void *, Reason_code rc, void *, Process *proc)
{
	int	next_event = 0;
	int	next_onstop = 0;

	if (!_is_open)
		return;

	status_pane->update(proc);

	if (!proc)
	{
		events->clear();
		onstop->clear();
		max_onstop = max_events = 0;
		ableEventSel = disableEventSel = 0;
		set_sensitivity();
		return;
	}

	//only RC_set_current will refresh the display
	if (rc != RC_set_current)
	{
		set_sensitivity();
		return;
	}

	onstop->deselect_all();
	onstop->delay_updates();
	events->deselect_all();
	events->delay_updates();
	for (Elink *link = proc->get_events(); link; link = link->next())
	{
		Event	*e = link->event();
		char	id[MAX_INT_DIGITS + 1];
		char	buf[MAX_INT_DIGITS + 1];
		char	state[2];

		sprintf(id, "%-d", e->get_id());
		state[0] = state[1] = 0;
		buf[0] = 0;

		if (e->get_state() == ES_disabled)
			state[0] = 'D';

		if (e->get_type() == ET_onstop)
		{
			if (next_onstop >= max_onstop)
				onstop->insert_row(next_onstop, id, state, e->get_plist(),
					e->get_commands());
			else
				onstop->set_row(next_onstop, id, state, e->get_plist(),
					e->get_commands());
			next_onstop++;
		}
		else
		{
			switch(e->get_type())
			{
				case ET_stop:
					if (((Stop_event *)e)->get_count() > 1)
						sprintf(buf, "%d",((Stop_event *)e)->get_count());
					break;
				case ET_syscall:
					if (((Syscall_event *)e)->get_count() > 1)
						sprintf(buf, "%d",((Syscall_event *)e)->get_count());
					break;
			}

			if (next_event >= max_events)
				events->insert_row(next_event, id, state,
					e->get_type_string(), e->get_plist(),
		 			e->get_condition(), buf, e->get_commands());
			else
				events->set_row(next_event, id, state,
					e->get_type_string(), e->get_plist(),
		 			e->get_condition(), buf, e->get_commands());

			next_event++;
		}
	}

	if (max_events > next_event)
		events->delete_rows(next_event, max_events - next_event);
	events->finish_updates();
	max_events = next_event;

	if (max_onstop > next_onstop)
		onstop->delete_rows(next_onstop, max_onstop - next_onstop);
	onstop->finish_updates();
	max_onstop = next_onstop;
	set_sensitivity();
}

// Put data in buffer1
char *
Event_window::get_selected_ids()
{
	int	*sel;
	int	*selOnstop;
	int	selected 	= events->get_selections(sel);
	int	selectedOnstop 	= onstop->get_selections(selOnstop);

	Cell_value item;

	buffer1.clear();

	for (int i = 0; i < selected; i++)
	{
		item = events->get_cell(sel[i], COL_ID);

		buffer1.add(item.string);
		buffer1.add(' ');
	}

	for (i = 0; i < selectedOnstop; i++)
	{
		item = onstop->get_cell(selOnstop[i], COL_ID);
		buffer1.add(item.string);
		buffer1.add(' ');
	}
	return (char *)buffer1;
}

void
Event_window::disableEventCb(Component *, Base_window *)
{
	dispatcher.send_msg(this, get_window_set()->current_process()->get_id(),
		"disable %s\n", get_selected_ids());
}

void
Event_window::enableEventCb(Component *, Base_window *)
{
	dispatcher.send_msg(this, get_window_set()->current_process()->get_id(),
		"enable %s\n", get_selected_ids());
}

void
Event_window::deleteEventCb(Component *, Base_window *)
{
	dispatcher.send_msg(this, get_window_set()->current_process()->get_id(),
		"delete %s\n", get_selected_ids());
}

// Find the event id to process
// Bring up the appropriate dialog according to the type of event
void
Event_window::changeEventCb(Component *, Base_window *bw)
{
	int	*sel;
	int	*selOnstop;
	int	selected 	= events->get_selections(sel);
	int	selectedOnstop 	= onstop->get_selections(selOnstop);
	int 	eventId = 0;


	if (selected != 0)
	{
		eventId = atoi(events->get_cell(sel[0], 0).string);
	}
	else if (selectedOnstop != 0)
	{
		eventId = atoi(onstop->get_cell(sel[0], 0).string);
	}

	Event		*p 	= event_list.findEvent(eventId);
	Event_type	type = p->get_type();
	Process_dialog	*pToShareCode = 0;

	switch(type)
	{
	case ET_stop:
	{
		Stop_dialog	*stop_box = window_set->get_stop_box();

		// fill data in each interface object
		if (!stop_box)
		{
			stop_box = new Stop_dialog(window_set);
			window_set->set_stop_box(stop_box);
		}

		pToShareCode = stop_box;

		stop_box->setEventId(eventId);
		stop_box->fillContents(p);
		stop_box->setChange(1);
		break;
	}
	case ET_signal:
	{
		Signal_dialog	*signal_box = window_set->get_signal_box();
		pToShareCode = signal_box;

		if (!signal_box)
		{
			signal_box = new Signal_dialog(window_set);
			window_set->set_signal_box(signal_box);
		}

		signal_box->setEventId(eventId);
		signal_box->fillContents(p);
		signal_box->setChange(1);
		break;
	}
	case ET_syscall:
	{
		Syscall_dialog	*syscall_box = window_set->get_syscall_box();
		pToShareCode = syscall_box;

		if (!syscall_box)
		{
			syscall_box = new Syscall_dialog(window_set);
			window_set->set_syscall_box(syscall_box);
		}

		syscall_box->setEventId(eventId);
		syscall_box->fillContents(p);
		syscall_box->setChange(1);
		break;
	}
	case ET_onstop:
	{
		Onstop_dialog	*onstop_box = window_set->get_onstop_box();
		pToShareCode = onstop_box;

		if (!onstop_box)
		{
			onstop_box = new Onstop_dialog(window_set);
			window_set->set_onstop_box(onstop_box);
		}

		onstop_box->setEventId(eventId);
		onstop_box->fillContents(p);
		onstop_box->setChange(1);
		break;
	}
	case ET_none:
	default:
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	}

	pToShareCode->dialog->set_label("Change", 'A');
	pToShareCode->set_plist(bw, window_set->get_event_level());
	pToShareCode->display();
}
