/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_WINDOW_SHELL
#define _WINDOW_SHELL
#ident	"@(#)debugger:gui.d/common/Window_sh.h	1.1"

#include "Component.h"
#include "gui_msg.h"
#include "Severity.h"

class Base_window;

// The interface to the top level widget for a base window.
// A Window_shell accepts one and only one child component
// (usually a box of some kind, that contains other components)

// In a window with several sub-components that accept input/selection/etc,
// the one that is created first (and that has the input focus by
// default) is not necessarily the one where you would expect to type
// (specifically, the Command line in the Command window should have
// the focus, not the transcript pane).  set_focus() allows the
// specification of any component in the tree as the focus widget

class Window_shell : public Component
{
	#include	"Window_shP.h"

private:
	Component	*child;
	Callback_ptr	dismiss;	// function called when user tries to
					// get rid of the window - can't dismiss
					// debug's last open window

	void		display_msg(Severity, const char *);

public:
			Window_shell(const char *name,
				Callback_ptr dismiss, Base_window *creator,
				Help_id help_msg);
			~Window_shell();

	void		add_component(Component *);	// accepts one child
	void		set_busy(Boolean);	// turn busy indicator on or off
	void		clear_msg();		// blank out an earlier message
	void		display_msg(Message *);	// display a message from debug
	void		display_msg(Severity, Gui_msg_id, ...); // error caught by
								// the gui
	void		set_focus(Component *);	// change default focus widget
	void		popup();		// display the window
	void		popdown();		// dismiss the window
	void		raise();		// raise the window
};

#endif // WINDOW_SHELL
