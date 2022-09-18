/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _CONTEXT_H
#define _CONTEXT_H
#ident	"@(#)debugger:gui.d/common/Context.h	1.3"

#include "Component.h"
#include "Window_sh.h"
#include "Windows.h"

class Context_dialog;
class Ps_pane;
class Stack_pane;
class Symbols_pane;
class Symbol_class_dialog;
class Stop_on_function_dialog;
class Dump_dialog;
class Map_dialog;
class Expand_dialog;

class Context_window : public Base_window
{
	Ps_pane			*ps_pane;
	Stack_pane		*stack_pane;
	Symbols_pane		*syms_pane;
	Context_dialog		*setup_panes_dialog;
	Symbol_class_dialog	*sym_class_dialog;
	Stop_on_function_dialog	*stop_on_function_dialog;
	Dump_dialog		*dump_dialog;
	Map_dialog		*map_dialog;
	Expand_dialog		*expand_dialog;
	Selection_type		stype;

public:
				Context_window(Window_set *);
				~Context_window();

				// access_functions
	Ps_pane			*get_ps_pane()		{ return ps_pane; }
	Stack_pane		*get_stack_pane()	{ return stack_pane; }
	Symbols_pane		*get_syms_pane()	{ return syms_pane; }

				// display functions
	void			popup();
	void			popdown();

				//callbacks
	void			set_current(Component *, void *);
	void			setup_panes_cb(Component *, void *);
	void			setup_syms_cb(Component *, void *);
	void			new_window_set_cb(Component *, void *);
	void			stop_on_function_cb(Component *, void *);
	void			export_syms_cb(Component *, void *);
	void			dump_dialog_cb(Component *, void *);
	void			map_dialog_cb(Component *, void *);
	void			set_watchpoint_cb(Component *, void *);
	void			expand_dialog_cb(Component *, void *);

	void			inc_busy() { window->set_busy(TRUE); }
	void			dec_busy() { window->set_busy(FALSE); }

				// functions inherited from Base_window
	Selection_type		selection_type();
	int			check_sensitivity(int sense);

	void			set_selection(Selection_type);
	void			add_ws(int id);
	void			popup_ws(Component *, void *);
	void			delete_ws(int id);
};

#endif	// _CONTEXT_H
