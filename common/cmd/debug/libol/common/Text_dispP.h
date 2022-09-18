/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_TEXT_DISPP_H
#define	_TEXT_DISPP_H
#ident	"@(#)debugger:libol/common/Text_dispP.h	1.2"

// toolkit specific members of the Text_display class
// included by ../../gui.d/common/Text_area.h

private:
	int		current_line;
	int		last_line;
	int		empty;
	const char	*search_expr;
	char		*compiled_expr;
	int		selection_start;
	int		selection_size;
	char		*breaks;
	int		is_source;

	int		top_margin;
	int		x_arrow;
	int		x_stop;
	int		left_margin;
	int		font_height;
	GC		gc_base;
	GC		gc_stop;

	void		draw_number(int line, int top_line);
	void		draw_arrow(int line);
	void		clear_arrow(int line);
	void		draw_stop(int line);
	void		pane_bounds(int &lower, int &upper);

	void		finish_setup();
	void		set_gc();

public:
	void		set_selection(int start, int end);
	void		set_selection_size(int n)	{ selection_size = n; }
	void		fix_margin(XRectangle *);

#endif	// _TEXT_DISPP_H
