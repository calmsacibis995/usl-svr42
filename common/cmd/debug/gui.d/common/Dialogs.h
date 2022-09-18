/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_DIALOGS_H
#define _DIALOGS_H
#ident	"@(#)debugger:gui.d/common/Dialogs.h	1.4"

#include "Component.h"
#include "Sender.h"
#include "UI.h"

// cfront 2.1 requires class name in constructor, 1.2 doesn't accept it
#ifdef __cplusplus
#define DIALOG_BOX	Dialog_box
#define PROCESS_DIALOG	Process_dialog
#else
#define DIALOG_BOX
#define PROCESS_DIALOG
#endif

class	Window_set;
class	Base_window;
class	Process;

class Dialog_box : public Command_sender
{
public:
	Dialog_shell	*dialog;

			Dialog_box(Window_set *ws);
			~Dialog_box();

	virtual void	display();
	virtual void	de_message(Message *);
	virtual void	cmd_complete();
};

class Process_dialog : public Dialog_box
{
protected:
	int		level;
	int		total;
	int		track_current;
	Process		**processes;
	Simple_text	*process_list;
	Caption		*process_caption;

public:
			Process_dialog(Window_set *ws);
			~Process_dialog();

			// callbacks
	void		update_list_cb(void *, int rc, void *, Process *process);
	void		update_current_cb(void *, int rc, void *, Process *process);
	void		dismiss_cb(Component *, void *);

	void		set_plist(Base_window *, int level);
	virtual void	set_process(Boolean reset);
};

class Create_dialog : public Dialog_box
{
	Text_line	*cmd_line;
	Toggle_button	*toggles;
	char		*save_cmd;
	Boolean		follow_state;
	Boolean		io_state;
	Boolean		new_window_state;
	Window_set	*save_ws;
public:
			Create_dialog(Window_set *);
			~Create_dialog() {}

			// display functions and callbacks
	void		do_create(Component *, void *);
	void		drop_cb(Component *, void *);
	void		cancel(Component *, void *);
};

class Recreate_dialog : public Dialog_box
{
	Simple_text	*cmd_line;
	Toggle_button	*toggles;
	Boolean		follow_state;
	Boolean		io_state;
	Boolean		new_window_state;
	Window_set	*save_ws;
public:
			Recreate_dialog(Window_set *);
			~Recreate_dialog() {}

			// display functions and callbacks
	void		do_recreate(Component *, void *);
	void		cancel(Component *, void *);

			// update the display
	void		set_create_args(const char *s);
};

class Grab_process_dialog : public Dialog_box
{
	Text_line	*object_file;
	Toggle_button	*toggles;
	Selection_list	*ps_list;
	char		*save_obj;
	Boolean		follow_state;
	Boolean		new_window_state;
	Window_set	*save_ws;
public:

			Grab_process_dialog(Window_set *);
			~Grab_process_dialog() { delete save_obj; }

			// callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);
	void		drop_cb(Selection_list *, Component *dropped_on);

	void		do_it(Window_set *);	// called by apply & cancel to do the grab
	void		setup();		// initialize process list
};

class Grab_core_dialog : public Dialog_box
{
	Text_line	*object_file;
	Text_line	*core_file;
	Toggle_button	*new_set;
	char		*save_core;
	char		*save_obj;
	Boolean		save_toggle;
	Window_set	*save_ws;
public:

			Grab_core_dialog(Window_set *);
			~Grab_core_dialog() {}

			// display functions and callbacks
	void		apply();
	void		cancel();
	void		drop_cb(Component *, void *);
};

class Set_language_dialog : public Dialog_box
{
	Radio_list	*lang_choices;
public:
			Set_language_dialog(Window_set *);
			~Set_language_dialog() {}

	void		apply();
	void		reset();
};

class Cd_dialog : public Dialog_box
{
	Simple_text	*current_directory;
	Text_line	*new_directory;
	char		*save_text;

public:
			Cd_dialog(Window_set *);
			~Cd_dialog()	{ delete save_text; }

			// button callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);

			// update functions
	void		update_directory(const char *);
};

#endif // _DIALOGS_H
