/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_WINDOWS_H
#define	_WINDOWS_H
#ident	"@(#)debugger:gui.d/common/Windows.h	1.11"

#include "Component.h"
#include "Dialogs.h"
#include "Notifier.h"
#include "Sender.h"
#include "List.h"

// cfront 2.1 requires class name in constructor, 1.2 doesn't accept it
#ifdef __cplusplus
#define BASE_WINDOW	Base_window
#else
#define BASE_WINDOW
#endif

// The reason code is set by the Window_set or Event_list before
// notifying clients to indicate what changed in the given process
enum Reason_code
{
	RC_change_state,	// hit a breakpoint, started running, etc.
	RC_set_current,		// context change within a window set
	RC_set_frame,		// context change within the process
	RC_rename,		// program has been renamed
	RC_delete,		// process died or was released
};

// Identifies button that was pressed in the Control Menu
enum Control_type
{
	CT_run,
	CT_return,
	CT_step_statement,
	CT_step_instruction,
	CT_next_statement,
	CT_next_instruction,
	CT_halt,
	CT_release_running,
	CT_release_suspended,
};

enum Base_window_type
{
	BW_command,
	BW_context,
	BW_event,
	BW_source,
	BW_disasm,
};

// Identifies type of the current selection - only one type
// is allowed at a time per window
enum Selection_type
{
	SEL_none,
	SEL_text,		// text in the Command window, transcript pane
	SEL_process,		// Context window, process pane
	SEL_event,		// Event window, either pane
	SEL_src_line,		// Source window
	SEL_frame,		// Context window, stack pane
	SEL_symbol,		// Context window, symbol pane
	SEL_regs,		// Disassembly window, register pane
	SEL_instruction,	// Disassembly window, dis pane
};

// Identifies action to be taken when an event notification is
// received or a running process generates output
enum Action
{
	A_raise,
	A_beep,
	A_message,
	A_none,
};

class Command_window;
class Context_window;
class Source_window;
class Disasm_window;
class Event_window;
class Window_shell;
class Window_set;
class Message;
class Process;
class Plink;
class Stop_dialog;
class Signal_dialog;
class Syscall_dialog;
class Onstop_dialog;
class Cancel_dialog;
class Kill_dialog;
class Setup_signals_dialog;
class Search_dialog;

// These dialogs are shared - used by more than one window - so they
// are with the Window_set instead of a specific window

class Set_value_dialog : public Process_dialog
{
	Text_line	*expression;
	char		*save_expr;
public:
			Set_value_dialog(Window_set *);
			~Set_value_dialog() { delete save_expr; };

			// callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);

			// initialization
	void		set_expression(const char *);

			// functions inherited from Dialog_box
	void		cmd_complete();
};

class Show_value_dialog : public Process_dialog
{
	Text_line	*expression;
	Text_line	*format_line;
	Text_area	*result;
	Radio_list	*formats;
public:
			Show_value_dialog(Window_set *);
			~Show_value_dialog() {};

			// initialization routines
	void		set_expression(const char *);
	void		clear_result();
	const char	*get_format();

			// callbacks
	void		do_it(Component *, void *);
	void		set_format_type(Component *, void *);

			// functions inherited from Dialog_box
	void		de_message(Message *);
	void		cmd_complete();
};

class Run_dialog : public Process_dialog
{
	Text_line	*location;
	char		*save_string;
public:

			Run_dialog(Window_set *);
			~Run_dialog();

			// display functions and callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);

			// initialization
	void		set_location(const char *);
};

class Step_dialog : public Process_dialog
{
	Text_line	*count;
	Radio_list	*type;
	Radio_list	*times;
	Toggle_button	*over;
	char		*save_count;
	int		type_state;
	int		times_state;
	Boolean		step_over;
public:

			Step_dialog(Window_set *);
			~Step_dialog() { delete save_count; }

			// callbacks
	void		apply(Component *, void *);
	void		reset(Component *, void *);

	void		set_times(Component *, void *);
};

class Jump_dialog : public Process_dialog
{
	Text_line	*location;
	char		*save_string;
public:

			Jump_dialog(Window_set *);
			~Jump_dialog();

			// display functions and callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);

			// initialization
	void		set_location(const char *);
};

class Path_dialog : public Process_dialog
{
	Text_area	*path_area;
	Radio_list	*choice;
	int		state;
public:
			Path_dialog(Window_set *);
			~Path_dialog() {}

			// callbacks
	void		apply(Component *, void *);
	void		reset(Component *, void *);
	void		set_path_type(Radio_list *, int);

			// inherited from Process_dialog
	void		set_process(Boolean);
};

class Granularity_dialog : public Dialog_box
{
	Radio_list	*event_choice;
	Radio_list	*command_choice;
	int		event_state;
	int		command_state;
public:
			Granularity_dialog(Window_set *);
			~Granularity_dialog() {}

			// callbacks
	void		apply(Component *, void *);
	void		reset(Component *, void *);
};

// The Action dialog allows the user to specify how the GUI will behave
// when it gets either an event notification or output from a process
// (the two actions are separately controllable).  The choices are:
// beeping, raising the command window, bringing up an alert box,
// or doing nothing.  In any case, the event messages and output are
// always displayed in the transcript pane.

class Action_dialog : public Dialog_box
{
	Radio_list	*event_action;
	Radio_list	*output_action;
public:
			Action_dialog(Window_set *);
			~Action_dialog() {};

			// Button callbacls
	void		apply(Component *, void *);
	void		reset(Component *, void *);
};

class Base_window : public Command_sender
{
protected:
	Window_shell		*window;
	Menu_bar		*menu_bar;
	Boolean			_is_open;
	Base_window_type	type;

	virtual int		check_sensitivity(int sense);
public:
				Base_window(Window_set *ws, Base_window_type t)
					: COMMAND_SENDER(ws)
					{ window = 0; _is_open = FALSE; type = t;
						menu_bar = 0; }
				~Base_window() {}

				// access functions
	Window_shell		*get_window_shell()	{ return window; }
	Boolean			is_open()		{ return _is_open; }
	Base_window_type	get_type()		{ return type; }

				// functions inherited from Command_sender
	void			de_message(Message *);

				// display functions
	virtual void		popup();
	virtual void		popdown();

	void			set_sensitivity();
	int			is_sensitive(int sense);

	virtual Selection_type	selection_type();
};

class Window_set : public Command_sender
{
	Command_window		*commandwin;
	Context_window		*contextwin;
	Source_window		*sourcewin;
	Event_window		*eventwin;
	Disasm_window		*disasmwin;

				// shared dialogs - only allocated as needed
	Create_dialog		*create_box;
	Recreate_dialog		*recreate_box;
	Grab_process_dialog	*grab_process;
	Grab_core_dialog	*grab_core;
	Set_language_dialog	*set_language;
	Set_value_dialog	*set_value;
	Show_value_dialog	*show_value;
	Run_dialog		*run_box;
	Step_dialog		*step_box;
	Jump_dialog		*jump_box;
	Stop_dialog		*stop_box;
	Signal_dialog		*signal_box;
	Syscall_dialog		*syscall_box;
	Onstop_dialog		*onstop_box;
	Cancel_dialog		*cancel_box;
	Kill_dialog		*kill_box;
	Setup_signals_dialog	*setup_signals;
	Path_dialog		*path_box;
	Granularity_dialog	*granularity_box;
	Cd_dialog		*cd_box;
	Search_dialog		*search_box;
	Action_dialog		*action_box;

				// process list - subset of global proclist
	Plink			*head;
	Plink			*tail;
	Process			*cur_process;

	Action			event_action;
	Action			output_action;
	int			open_windows;
	int			id;		// window set number
	int			event_level;	// granularity
	int			command_level;

	Plink			*find_process(Process *, int &index);
	void			apply_to_process(Control_type, Base_window *);

public:
	Notifier	change_current;
	Notifier	change_any;
	List		source_windows;

			Window_set();
			~Window_set();

			// access functions
	Context_window	*get_context_window()	{ return contextwin; }
	Command_window	*get_command_window()	{ return commandwin; }
	Source_window	*get_source_window()	{ return sourcewin; }
	Event_window	*get_event_window()	{ return eventwin; }
	Disasm_window	*get_disasm_window()	{ return disasmwin; }
	Process		*current_process()	{ return cur_process; }
	Plink		*get_process_list()	{ return head; }
	int		get_id()		{ return id; }
	void		inc_open_windows()	{ open_windows++; }
	void		dec_open_windows()	{ open_windows--; }
	void		set_event_action(Action a) { event_action = a; }
	Action		get_event_action()	{ return event_action; }
	void		set_output_action(Action a) { output_action = a; }
	Action		get_output_action()	{ return output_action; }
	int		get_event_level()	{ return event_level; }
	void		set_event_level(int l)	{ event_level = l; }
	int		get_command_level()	{ return command_level; }
	void		set_command_level(int l) { command_level = l; }
	Stop_dialog	*get_stop_box()		{ return stop_box; }
	Signal_dialog	*get_signal_box()	{ return signal_box; }
	Syscall_dialog	*get_syscall_box()	{ return syscall_box; }
	Onstop_dialog	*get_onstop_box()	{ return onstop_box; }
	void		set_stop_box(Stop_dialog *d) { stop_box = d; }
	void		set_signal_box(Signal_dialog *d) { signal_box = d; }
	void		set_syscall_box(Syscall_dialog *d) { syscall_box = d; }
	void		set_onstop_box(Onstop_dialog *d) { onstop_box = d; }

			// functions called by proclist to keep window set's process
			// list up to date
	void		add_process(Process *, int make_current);
	void		delete_process(Process *);
	void		update_process(Process *);
	void		rename_process(Process *);
	void		set_frame(Process *);
	int		get_frame(int frame, const char *&function,
				const char *&location);

			// functions called by the Dispatcher to keep the
			// window set up to date
	void		inform(Message *);
	void		update_recreate_dialog();
	void		update_language_dialog();
	void		update_cd_dialog(const char *s);

			// change the window set's process list
	void		set_current(Process *);

			// display window, initialize if necessary
	void		popup_context();
	void		popup_command();
	void		popup_source();
	void		popup_disasm();
	void		popup_event();

			// callbacks to bring up dialogs
	void		create_dialog_cb(Component *, Base_window *);
	void		recreate_dialog_cb(Component *, Base_window *);
	void		grab_process_dialog_cb(Component *, Base_window *);
	void		grab_core_dialog_cb(Component *, Base_window *);
	void		set_language_dialog_cb(Component *, Base_window *);
	void		set_value_dialog_cb(Component *, Base_window *);
	void		show_value_dialog_cb(Component *, Base_window *);
	void		map_dialog_cb(Component *, Base_window *);
	void		run_dialog_cb(Component *, Base_window *);
	void		step_dialog_cb(Component *, Base_window *);
	void		jump_dialog_cb(Component *, Base_window *);
	void		stop_dialog_cb(Component *, Base_window *);
	void		signal_dialog_cb(Component *, Base_window *);
	void		syscall_dialog_cb(Component *, Base_window *);
	void		onstop_dialog_cb(Component *, Base_window *);
	void		cancel_dialog_cb(Component *, Base_window *);
	void		kill_dialog_cb(Component *, Base_window *);
	void		setup_signals_dialog_cb(Component *, Base_window *);
	void		path_dialog_cb(Component *, Base_window *);
	void		set_granularity_cb(Component *, Base_window *);
	void		cd_dialog_cb(Component *, void *);
	void		search_dialog_cb(Component *, Base_window *);
	void		action_dialog_cb(Component *, Base_window *);

			// callbacks for simple command buttons
	void		run_button_cb(Component *, Base_window *);
	void		run_r_button_cb(Component *, Base_window *);
	void		step_button_cb(Component *, Base_window *);
	void		step_i_button_cb(Component *, Base_window *);
	void		step_o_button_cb(Component *, Base_window *);
	void		step_oi_button_cb(Component *, Base_window *);
	void		halt_button_cb(Component *, Base_window *);
	void		release_running_cb(Component *, Base_window *);
	void		release_suspended_cb(Component *, Base_window *);
	void		version_cb(Component *, Base_window *);
	void		help_sect_cb(Component *, Base_window *);
	void		help_toc_cb(Component *, Base_window *);

			// close and destroy the window set or a single window
	void		dismiss(Component *, Base_window *);
	void		ok_to_quit();
	void		quit_cb(Component *, int);
};

extern	List		windows;	// the list of all window sets

#endif // _WINDOWS_H
