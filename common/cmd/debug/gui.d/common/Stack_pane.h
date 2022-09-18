/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _STACK_PANE_H
#define _STACK_PANE_H
#ident	"@(#)debugger:gui.d/common/Stack_pane.h	1.2"

#include "Component.h"
#include "Dialogs.h"
#include "Windows.h"

enum Stack_cell { ST_current, ST_frame, ST_func, ST_params, ST_loc };

class Message;
class Process;
class Table_calldata;
class Divided_box;

class Stack_pane : public Command_sender
{
	Table		*pane;
	int		top_frame;
	int		cur_frame;
	int		next_row;
	int		has_selection;
	char		*func;
public:
			Stack_pane(Window_set *, Divided_box *);
			~Stack_pane();

			// access functions
	Table		*get_table()	{ return pane; }
	int		get_frame(int frame, const char *&func, const char *&loc);

			// callbacks
	void		update_cb(void *server, Reason_code, void *, Process *);
	void		select_frame(Table *, void *);
	void		deselect_frame(Table *, int);
	void		set_current();

			// functions inherited from Command_sender
	void		de_message(Message *);
	void		cmd_complete();

	void		deselect();

	void		popup();
	void		popdown();
};

#endif	// _STACK_PANE_H
