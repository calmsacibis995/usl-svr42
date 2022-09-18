/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_TABLEP_H
#define	_TABLEP_H
#ident	"@(#)debugger:libol/common/TableP.h	1.6"

// toolkit specific members of the Table class
// included by ../../gui.d/common/Table.h

#include "Vector.h"

struct Item_data
{
	char		**strings;
	XtArgVal	is_set;
};

#define TABLE_TOOLKIT_SPECIFICS \
private:						\
	Widget		input;				\
	char		*format;			\
	Item_data	*item_data;			\
	Item_data	*old_data;			\
	Item_data	*blank_data;			\
	Vector		vector;				\
	Column		*column_spec;			\
	Boolean		sensitive;			\
	Boolean		wrapped;			\
	Boolean		delay;				\
	int		font_width;			\
	Pixmap		hand_pm;			\
	Pixmap		solid_pm;			\
	int		delete_start;			\
	int		delete_total;			\
							\
	char		*make_format(Boolean wrap);	\
	void		set_row(Item_data *, va_list);	\
	void		fix_column_spec(XFontStruct *font);	\
	void		make_title(XFontStruct *font, Widget window);	\
	void		make_glyphs(Widget);		\
	void		update_glyph(char **, Boolean, Glyph_type);	\
	void		cleanup(Item_data *, int total);

#endif	// _TABLEP_H
