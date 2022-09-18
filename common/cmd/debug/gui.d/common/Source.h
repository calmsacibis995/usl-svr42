/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SOURCE_H
#define _SOURCE_H
#ident	"@(#)debugger:gui.d/common/Source.h	1.5"

#include "Component.h"
#include "Eventlist.h"
#include "Sender.h"
#include "Windows.h"

class Process;
class Status_pane;
class Show_function_dialog;
class Show_line_dialog;
class Open_dialog;

class Source_window : public Base_window
{
	Status_pane		*status_pane;
	Caption			*caption;
	Text_display		*pane;
	File_list		*file_ptr;	// ptr to breakpoint info for current file
	Flink			*flink;
	const char		*current_file;
	char			*current_path;
	int			current_line;
	int			selected_line;
	Boolean			primary;	// primary source window for
						// the window set
	Boolean			registered;

	Show_function_dialog	*show_function;
	Show_line_dialog	*show_line;
	Open_dialog		*open_box;

	Boolean			set_path(Boolean must_set);
	void			clear();
public:
				Source_window(Window_set *, Boolean primary);
				~Source_window();

				// access functions
	const char		*get_current_file()	{ return current_file; }
	int			get_selected_line()	{ return selected_line; }
	Text_display		*get_pane()		{ return pane; }
	void			set_current_line(int l)	{ current_line = l; }

				// framework callbacks
	void			update_cb(void *, Reason_code, void *, Process *);
	void			break_list_cb(void *, Reason_code_break, void *, Stop_event *);

				// component callbacks
	void			set_break_cb(Component *, void *);
	void			delete_break_cb(Component *, void *);
	void			ok_to_delete(Component *, int);
	void			show_function_cb(Component *, void *);
	void			show_line_cb(Component *, void *);
	void			open_dialog_cb(Component *, void *);
	void			new_source_cb(Component *, void *);
	void			copy_cb(Component *, void *);
	void			select_cb(Text_display *, int line);


	void			popup();
	void			popdown();
	void			set_file(const char *file, const char *path = 0);

				// functions inherited from Base_window
	Selection_type		selection_type();
	int			check_sensitivity(int sense);

	char			*get_selection();
};

#endif	// _SOURCE_H
