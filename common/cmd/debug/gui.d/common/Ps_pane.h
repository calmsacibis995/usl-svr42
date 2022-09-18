/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PS_PANE_H
#define _PS_PANE_H
#ident	"@(#)debugger:gui.d/common/Ps_pane.h	1.1"

#include "Component.h"
#include "Dialogs.h"
#include "Sender.h"
#include "Windows.h"

class Message;
class Process;
class Program;
class Table_calldata;

class Ps_pane
{
	Table		*pane;
	Window_set	*window_set;
	int		total_selections;
public:
			Ps_pane(Window_set *, Divided_box *box);
			~Ps_pane();

			// access functions
	Table		*get_table()	{ return pane; }
	int		get_total()	{ return total_selections; }

			// functions to update the displayed information,
			// called by Window_set
	void		add_process(int index, Process *);
	void		delete_process(int index);
	void		update_process(Reason_code, int index, Process *);
	void		set_current(Process *);

			// callbacks
	void		select_cb(Table *, void *);
	void		deselect_cb(Table *, void *);
	void		drop_proc(Table *, const Table_calldata *);

	Process		**get_selections();
	void		deselect();
};

#endif	// _PS_PANE_H
