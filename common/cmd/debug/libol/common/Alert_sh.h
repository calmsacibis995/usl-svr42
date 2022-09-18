/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_ALERT_SH_H
#define	_ALERT_SH_H
#ident	"@(#)debugger:libol/common/Alert_sh.h	1.2"

#include "Component.h"
#include "gui_msg.h"
#include "Severity.h"
#include <stdarg.h>

// An Alert_shell is a notice box used to display a message to the user
// and ask for confirmation.  An alert_shell may have one (action) or two
// (action, no_action) buttons.  The component is destroyed when the
// widget is popped down - the widget is not reused because if the
// message or button strings are replaced the widget may not be resized
// properly

// Callbacks are invoked as
// 	object->function((Alert_shell *)this, int yes_or_no)

class Alert_shell : public Component
{
	Callback_ptr	handler;
	void		*object;

public:
			Alert_shell(const char *message_string,
				const char *action,	// first button label
				const char *no_action = 0, // 2nd button label
				Callback_ptr handler = 0, void *object = 0);
			~Alert_shell();

			// access functions
	Callback_ptr	get_handler()	{ return handler; }
	void		*get_object()	{ return object; }
};

#endif	// _ALERT_SH_H
