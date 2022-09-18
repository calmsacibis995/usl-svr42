/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_UI_H
#define	_UI_H
#ident	"@(#)debugger:gui.d/common/UI.h	1.7"

// GUI headers
#include "gui_msg.h"
#include "Component.h"

// Debug headers
#include "Language.h"
#include "Severity.h"
#include "Vector.h"

#include <stdarg.h>
#include <X11/Intrinsic.h>

class	Message;
struct	Menu_table;
class	Process;
class	Window_set;
class	Vector;

// Order is used by the functions that return lists of signals or system calls
enum Order { Alpha, Numeric };

extern	const char	*prompt;
extern	int		in_script;
extern	int		has_assoc_cmd;

// Scratch vectors available for use anywhere.  There is also a scratch
// Buffer, buffer1, which is not declared here because of conflicts with
// the graphics header files
extern	Vector	vscratch1;
extern	Vector	vscratch2;

extern	void	debug(const char * ...);
extern	void	ui_exit(int);

// get_signals and get_syslist set the global variables Nsignals and Nsyscalls,
// respectively, to tell how many entries are in the lists.
extern	int		Nsignals;
extern	int		Nsyscalls;
extern	const char	**get_signals(Order);
extern	const char	**get_syslist(Order);

// comparison function for qsort
extern	int	alpha_comp(const void *, const void *);

// Toolkit specific routines
extern	Widget	init_gui(const char *name, const char *widget_class,
			int *argc, char **argv);
extern  void	toolkit_main_loop();
extern	void	beep();

// interface to sprintf that ensures there is enough room
char *do_vsprintf(const char *fmt, va_list args);

// set_create_args, set_lang, and set_directory are called by the Dispatcher
// to keep the Recreate, Language, and Change Directory dialogs up to date
extern	Language	cur_language;
extern	const char	*create_args;
extern	void		set_create_args(Message *);
extern	void		set_lang(Message *);
extern	void		set_directory();

// handle messages (informational or errors) that aren't associated with
// a particular window, or that need confirmation
// The callback in the last version is invoked as
//	 object->function((void *)0, int ok_or_cancel)
overload	display_msg;
extern	void	display_msg(Message *);
extern	void	display_msg(Severity, Gui_msg_id ...);
extern	void	display_msg(Callback_ptr, void *obj, const char *action,
			const char *no_action, Gui_msg_id ...);

//number of system calls - just a guess
#define	NSYS	256
// get the ps info for the Grab Process dialog
extern int	do_ps(char ***);

// returns TRUE if the message is one that should never be displayed to the user
extern Boolean	gui_only_message(Message *);

#endif	// _UI_H
