/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ifndef	_TABLE_H
#define	_TABLE_H
#ident	"@(#)debugger:gui.d/common/Table.h	1.6"

#include "Component.h"
#include "TableP.h"
#include <stdarg.h>

enum Column_type
{
	Col_text,
	Col_wrap_text,
	Col_numeric,
	Col_glyph
};

struct Column
{
	const char	*heading;
	int		size;
	Column_type	column_type;
};

enum Glyph_type
{
	Gly_hand,
	Gly_blank,
	Gly_none
};

union Cell_value
{
	Glyph_type glyph;
	const char *string;
};

// Framework callbacks:
// The select callback is called whenever an item in the list is selected
// The drop callback is called when an item in the list is dragged to another
// window.  These two callbacks are both invoked as
//	creator->callback((Table *)this, Table_calldata *)
// The deselect callback is called whenever an item is deselected.  That
// callback is invoked as
//	creator->callback((Table *)this, int item_index)
// Items in the list are numbered from 0 to n-1

struct Table_calldata
{
	int			index;
	Component		*dropped_on;
};

// The update functions - delete_rows, insert_row, set_row, & set_cell -
// update the display immediately unless bracketed by delay_updates
// and finish_updates.  Delaying the updating is done to minimize the
// number of times the table is redrawn while the stack and symbols
// panes are being updated.  There are several restrictions on what can
// be done between the calls to delay_updates and finish_updates:
//	1) delete_rows or insert_rows, but not both
//	2) only one call to delete_rows,
//	3) if more than one call to insert_rows, they must all be
//		appending to the end of the table

class Table : public Component
{
	TABLE_TOOLKIT_SPECIFICS

private:
	Callback_ptr    select_cb;
	Callback_ptr	deselect_cb;
	Callback_ptr	drop_cb;
	Select_mode	mode;
	int		rows;
	int		overflow;
	int		columns;

public:
			Table(Component *parent, const char *name, Select_mode,
				const Column *cspec,
				int columns, int visible_rows,
				Boolean wrap_cols,
				Callback_ptr select_cb = 0,
				Callback_ptr deselect_cb = 0,
				Callback_ptr drop_cb = 0,
				void *creator = 0,
				Help_id help_msg = HELP_none);
			~Table();

			// bracketing functions
	void		delay_updates();
	void		finish_updates();
	Boolean		is_delayed();

			// update functions
	void		delete_rows(int start_row, int total);
	void		set_row(int row ...);
	void		insert_row(int row ...);
	void		set_cell(int row, int column, Glyph_type type);
	void		set_cell(int row, int column, const char *s);
	Cell_value 	get_cell(int row, int column);
	void		clear();

			// graphics functions
	void		set_sensitive(Boolean);
	void		deselect(int row);
	void		deselect_all();
	void		wrap_columns(Boolean);
	void		show_border();
        void            update_glyphs(int row, Boolean highlight);
	void		set_overflow(int new_size);

	int		get_selections(int *&selections); // get the list of
						// selected items (mode == SM_multiple)

			// access functions
	Callback_ptr	get_select_cb()		{ return select_cb; }
	Callback_ptr	get_deselect_cb()	{ return deselect_cb; }
	Callback_ptr	get_drop_cb()		{ return drop_cb; }
	Select_mode	get_mode()		{ return mode; }
	int		get_rows()		{ return rows; }
};

#endif	// _TABLE_H
