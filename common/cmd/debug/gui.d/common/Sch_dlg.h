/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_SCH_DLG_H
#define _SCH_DLG_H
#ident	"@(#)debugger:gui.d/common/Sch_dlg.h	1.3"

#include "Component.h"
#include "Dialogs.h"

class Base_window;
class Window_set;

class Search_dialog : public Dialog_box
{
	Base_window	*base_window; // the last window which brought this up
	Text_line	*string;
	char		*save_string;
public:
			Search_dialog(Window_set *);
			~Search_dialog()	{ delete save_string; }

			// callbacks
	void		apply(Component *, int mnemonic);
	void		cancel(Component *, void *);

			// initialization routines
	void		set_string(const char *);
	void		set_base_window(Base_window *b);
};

#endif	// _SCH_DLG_H
