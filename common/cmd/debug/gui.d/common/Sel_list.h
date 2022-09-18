/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_SEL_LIST_H
#define	_SEL_LIST_H
#ident	"@(#)debugger:gui.d/common/Sel_list.h	1.2"

#include "Component.h"

// A Selection list is a scrolling list of selectable items.
// Items are numbered from 0 to n-1

// Framework callbacks:
// The select callback is called whenever an item in the list is selected.
// The deselect callback is called whenever a selected item is unselected.
// Both callbacks are invoked as
//	creator->callback((Selection_list *)this, int item_index);
//
// The drop callback is called whenever an item in the list is dragged to
// and dropped on another window.  The callback is invoked as
//	creator->callback((Selection_list *)this, Component *dropped_on);

class Selection_list : public Component
{
	#include	"Sel_listP.h"
	Callback_ptr	select_cb;
	Callback_ptr	deselect_cb;
	Callback_ptr	drop_cb;

public:
			Selection_list(Component *parent, const char *name,
				int rows, int columns, const char *format,
				int items, const char **ival, Select_mode,
				void *creator = 0,
				Callback_ptr select_cb = 0,
				Callback_ptr deselect_cb = 0,
				Callback_ptr drop_cb = 0,
				Help_id help_msg = HELP_none);
			~Selection_list();

			// update list
	void		set_list(int rows, const char **values);
	void		select(int row);
	void		deselect(int row);

			// overflow handling
	void		set_overflow(int new_size);

			// selection handling
	int		get_selections(int *&selections);
	const char	*get_item(int row, int column);

			// access functions
	Callback_ptr	get_drop_cb()		{ return drop_cb; }
	Callback_ptr	get_select_cb()		{ return select_cb; }
	Callback_ptr	get_deselect_cb()	{ return deselect_cb; }
};

#endif	// _SEL_LIST_H
