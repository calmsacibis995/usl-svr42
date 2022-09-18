/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _DIS_H
#define _DIS_H
#ident	"@(#)debugger:gui.d/common/Dis.h	1.8"

#include "Component.h"
#include "Eventlist.h"
#include "Sender.h"
#include "Windows.h"
#include "Iaddr.h"

class Process;
class Status_pane;
class Function_dialog;
class Show_loc_dialog;

class Disasm_window : public Base_window
{
	Status_pane		*status_pane;
	Caption			*caption;
	Text_display		*pane;
	Text_area		*reg_pane;
	Text_area		*sel_pane;
	char			*current_func;
	char			*current_loc;
	int			current_line;
	int			selected_line;
	Vector			locations;

	Show_loc_dialog		*show_loc;
	Function_dialog		*show_function;

	void			clear();
	int			has_inst(const char *);
	int			has_inst(Iaddr address);
	void			disasm_func(const char *);
	void			set_display_line();
public:
				Disasm_window(Window_set *);
				~Disasm_window();

				// access functions
	int			get_selected_line()	{ return selected_line; }
	Text_display		*get_pane()		{ return pane; }

				// framework callbacks
	void			update_cb(void *, Reason_code, void *, Process *);
	void			break_list_cb(void *, Reason_code_break, void *, Stop_event *);

				// component callbacks
	void			select_cb(Text_display *, int line);
	void			set_break_cb(Component *, void *);
	void			delete_break_cb(Component *, void *);
	void			ok_to_delete(Component *, int);
	void			show_function_cb(Component *, void *);
	void			show_loc_cb(Component *, void *);
	void			copy_cb(Component *, void *);

	void			popup();
	void			popdown();

				// functions inherited from Base_window
	Selection_type		selection_type();
	int			check_sensitivity(int sense);

	char			*get_selection();
	void			disasm_set_func(const char *);
	void			disasm_addr(Iaddr, char *);
	void			disp_regs();
	const char		*get_selected_addr();
};

#endif	// _DIS_H
