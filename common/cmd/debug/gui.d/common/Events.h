/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _EVENTS_H
#define _EVENTS_H
#ident	"@(#)debugger:gui.d/common/Events.h	1.6"

#include "Component.h"
#include "Dialogs.h"
#include "Windows.h"

class Message;
class Process;
class Status_pane;
class Event;
class Table_calldata;
class EventPaneDialog;

// The dialog do not directly delete any screen components (Text_line, Radio_list, etc)
// The complete component tree is deleted recursively when the Dialog_shell is
// deleted in the Dialog destructor

class Stop_dialog : public Process_dialog
{
	Text_line	*expr;
	Text_line	*commands;
	Text_line	*count;
	int		eventId;
	int		doChange;

			// state variables for cancel operation
	char		*save_expr;
	char		*save_cmd;
	char		*save_count;
public:
			Stop_dialog(Window_set *);
			~Stop_dialog() { delete save_expr; delete save_cmd;
					 delete save_count; }

			// access functions
	void		setEventId(int id)	{ eventId = id;}
	void		setChange(int s = 0)	{ doChange = s;}

			// initialization routines
	void		fillContents(Event *);
	void		set_expression(const char *);

			// callbacks
	void		apply(Component	*, int);
	void		reset(Component *, int);
};

class Signal_dialog : public Process_dialog
{
	Selection_list	*siglist;
	Text_line	*commands;
	Radio_list	*ordering;

			// state variables for reset operation
	Order		order;
	int		eventId;
	int		doChange;
	char		*save_cmd;
	int		*selections;
	int		nselections;

public:
			Signal_dialog(Window_set *);
			~Signal_dialog() { delete save_cmd; delete selections; };

			// access functions
	void		setEventId(int id)	{ eventId = id;}
	void		setChange(int s = 0)	{ doChange = s;}

			// initialization function
	void		fillContents(Event *);

			// callbacks
	void		set_order(Component *, Order);
	void		apply(Component *, int);
	void		reset(Component *, int);
};

class Syscall_dialog : public Process_dialog
{
	Selection_list	*syslist;
	Text_line	*commands;
	Radio_list	*ordering;
	Text_line	*count;
	Toggle_button	*entry_exit;
	Order		order;
	int		eventId;
	int		doChange;

			// state information saved for reset
	char		*save_cmd;
	char		*save_count;
	int		entry_exit_state;	// which choices are set
	int		*selections;
	int		nselections;
public:
			Syscall_dialog(Window_set *);
			~Syscall_dialog() { delete selections; delete save_cmd;
					    delete save_count; }

			// access routines
	void		setEventId(int id)	{ eventId = id;}
	void		setChange(int s = 0)	{ doChange = s;}

			// initialization function
	void		fillContents(Event *);

			// callbacks
	void		set_order(Component *, Order);
	void		apply(Component *, int);
	void		reset(Component *, int);
};

class Onstop_dialog : public Process_dialog
{
	Text_area	*commands;

			// state variables for reset operation
	char		*save_cmds;
	int		eventId;
	int		doChange;
public:
			Onstop_dialog(Window_set *);
			~Onstop_dialog() { delete save_cmds; };

			// access functions
	void		setEventId(int id)	{ eventId = id;}
	void		setChange(int s = 0)	{ doChange = s;}

			// initialization function
	void		fillContents(Event *);

	void		apply(Component *, int);
	void		reset(Component *, int);
};

class Cancel_dialog : public Process_dialog
{
	Selection_list	*siglist;
	Order		order;
	Radio_list	*ordering;

			// state variables for reset operation
	int		nselections;
	int		*selections;

public:
			Cancel_dialog(Window_set *);
			~Cancel_dialog() { delete selections; };

			// callbacks
	void		apply(Component *, int);
	void		cancel(Component *, int);
	void		set_order(Component *, Order);

			// inherited from Process_dialog
	void		set_process(Boolean reset);

			// functions inherited from Dialog_box 
	void		cmd_complete();
};

class Kill_dialog : public Process_dialog
{
	Selection_list	*siglist;
	Order		order;
	Radio_list	*ordering;

			// state variables for reset operation
	char		*selection;

public:
			Kill_dialog(Window_set *);
			~Kill_dialog() { delete selection; };

			// callbacks
	void		apply(Component *, int);
	void		cancel(Component *, int);
	void		set_order(Component *, Order);
};

class Setup_signals_dialog : public Process_dialog
{
	Selection_list	*siglist;
	Order		order;
	Radio_list	*ordering;
	int		first_time;

public:
			Setup_signals_dialog(Window_set *);
			~Setup_signals_dialog() {};

			// callbacks
	void		apply(Component *, int);
	void		cancel(Component *, int);
	void		set_order(Component *, Order);

			// functions inherited from Dialog_box
	void		set_process(Boolean reset);
	void		cmd_complete();
};

class Event_window : public Base_window
{
	Status_pane		*status_pane;
	Table			*events;
	Table			*onstop;
	int			max_events;
	int			max_onstop;
	EventPaneDialog 	*setupPaneDialog;
						// flag for sensitivity
	int		ableEventSel;		// true if one or more undisabled
						//  event is selected
	int		disableEventSel;

	char		*get_selected_ids();
public:
			Event_window(Window_set *);
			~Event_window();

			// display functions
	void		popup();
	void		popdown();
	void		update_cb(void *server, Reason_code, void *, Process *);
	void		disableEventCb(Component *, Base_window *);
	void		enableEventCb(Component *, Base_window *);
	void		deleteEventCb(Component *, Base_window *);
	void		changeEventCb(Component *, Base_window *);
	void		setupPaneCb(Component *, Base_window *);

			// update the display
	void		add_event(Event *);
	void		delete_event(Event *);
	void		change_event(Event *);

	int		check_sensitivity(int);
	void		selectEventCb(Table *, Table_calldata *);
	void		deselectEventCb(Table *, int);

	int		contains(int eId);	//!0 if contains the event eId
	Table		*getEventPane(){ return events;}
	Table		*getOnstopPane(){ return onstop;}
};

#endif	// _EVENTS_H
