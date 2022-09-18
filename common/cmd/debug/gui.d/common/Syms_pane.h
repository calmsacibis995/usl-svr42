/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYMS_PANE_H
#define _SYMS_PANE_H
#ident	"@(#)debugger:gui.d/common/Syms_pane.h	1.3"

#include "Component.h"
#include "Dialogs.h"
#include "Windows.h"
#include "Sender.h"

class Divided_box;
class Message;
class Process;
class Table_calldata;

#define SYM_local	001
#define	SYM_file	002
#define	SYM_global	004
#define	SYM_debugger	010
#define	SYM_user	020

class Symbols_pane : public Command_sender
{
	int		symbol_types;
	Table		*pane;
	int		next_row;
	int		total_selections;
	int		reading_symbols;
public:
			Symbols_pane(Window_set *cw, Divided_box *);
			~Symbols_pane();

			// access functions
	Table		*get_table()	{ return pane; }
	int		get_sym_type()	{ return symbol_types; }
	int		get_total()	{ return total_selections; }

			// callbacks
	void		update_cb(void *server, Reason_code, void *, Process *);
	void		select_symbol(Table *, void *);
	void		deselect_symbol(Table *, int);
	void		symbols_dialog_cb(Component *, void *);

			// functions inherited from Command_sender
	void		de_message(Message *);
	void		cmd_complete();

	void		deselect();
	char		**get_selections();
	void		set_sym_type(int);
	void		export_symbols();

	void		popup();
	void		popdown();
};

class Symbol_class_dialog : public Dialog_box
{
	Toggle_button	*program_settings;
	Toggle_button	*debug_settings;
	Symbols_pane	*parent;

public:
			Symbol_class_dialog(Symbols_pane *);
			~Symbol_class_dialog() {}

	void		apply();
	void		reset();
};

#endif	// _SYMS_PANE_H
