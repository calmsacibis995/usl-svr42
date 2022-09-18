/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _TEXT_LINE_H
#define	_TEXT_LINE_H
#ident	"@(#)debugger:gui.d/common/Text_line.h	1.2"

#include "Component.h"

// Display a single line of text.
// editable specifies whether or not the user may edit it

// Framework callbacks:
// there are three callback functions: one for when the user types return,
// one for getting the selection, and one for losing the selection
// the callbacks are invoked as
//	creator->function((Text_line *)this, (char *)selected_string)

class Text_line : public Component
{

	#include	"Text_lineP.h"

public:
			Text_line(Component *parent, const char *name,
				const char *text, int width, Boolean edit,
				Callback_ptr return_cb = 0,
				Callback_ptr get_sel_cb = 0,
				Callback_ptr lose_sel_cb = 0,
				void *creator = 0, Help_id help_msg = HELP_none);
			~Text_line();

	char		*get_text();		// returns entire contents
	void		set_editable(Boolean);
	void		set_text(const char *);	// change the displayed text
	void		set_cursor(int);	// change cursor position
	void		clear();		// blank out the display
};

#endif	// _TEXT_LINE_H
