/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_MENU_H
#define	_MENU_H
#ident	"@(#)debugger:gui.d/common/Menu.h	1.7"

#include "Component.h"
#include "List.h"

class Window_set;
class Base_window;

// The flags field in Menu_table describes what type of framework object
// the button's callback references, or if the button brings up another menu
// Callbacks are invoked as
// Window_cb:	window->function((Menu *)this, table->cdata)
// Set_cb:	window_set->function((Menu *)this, window)

#define	Window_cb	1	// individual window
#define	Set_cb		2	// window set
#define	Menu_button	4	// cascade, brings up another menu

// The sensitivity field in Menu_table is a 32 bit mask, with the upper
// 16 bits reserved for global settings, and the lower 16 bits for window
// specific settings.
#define SEN_invalid		0		// not implemented - never sensitive
#define	SEN_always		0xffffffff	// all bits set == always available
#define SEN_sel_required	0x80000000

#define SEN_PROC_STATE		0x7fff0000	// sensitivity depends on process state
#define SEN_process		0x00010000	// program, process, or thread
#define SEN_use_selection	0x00020000	// selected process overrides current
#define SEN_proc_running	0x00040000	// process must be running
#define SEN_proc_stopped	0x00080000	// process must be stopped
#define	SEN_proc_live		0x00100000	// live process (non-core) required
						// live includes both running and
						// stopped
#define SEN_proc_single		0x00400000	// single process only
#define SEN_proc_stopped_core	0x00800000	// process must be stopped or a core file
#define SEN_proc_io_redirected	0x01000000	// process IO is through a pseudo-terminal

// the following used by the event window
#define	SEN_event_only		0x000000ff	// allocate 8 bits to events
#define	SEN_event_dis_sel	0x00000001	// a disabled event selected
#define	SEN_event_able_sel	0x00000002	// an abled event selected
#define	SEN_event_sel		0x00000003	// an event selected
#define	SEN_event_change	0x00000004	// an event selected
#define	SEN_event_no_sel	0x00000008	// no event selected

// the following used by source window
#define SEN_file_required       0x00000001	// The source window must have a current file
#define SEN_breakpt_required    0x00000002	// The selected line must have a breakpoint set


// the following used by the disassembly window
#define	SEN_line_breakpt_required 0x00000002	//a line with break point
#define	SEN_ins_sel_required	0x00000004 	//a line without break point
#define	SEN_line_sel_required	0x00000008 	//any line, even in reg pane
#define	SEN_disp_dis_required	0x00000010 	//display of instruction 

// the following used by the context window
#define	SEN_frame		0x00000002	// selection from the stack pane
#define SEN_symbol		0x00000004	// selection from the symbols pane
#define SEN_proc_created	0x00000008	// for Recreate, must have had a create
#define SEN_user_symbol		0x00000010	// user symbol from the symbols pane
#define SEN_single_symbol	0x00000020	// single selection from the symbols pane
#define SEN_program_symbol	0x00000040	// program symbol for watchpoints

// descriptor for the buttons in the individual menus
struct Menu_table
{
	const char	*label;		// button label
	unsigned char	mnemonic; 	// for mouseless operations - should be unique
				   	// within the menu
	unsigned char	flags;		// callback type
	unsigned int	sensitivity;	// determines button's availability
	Callback_ptr	callback;	// function called when this button is selected,
					// or secondary menu
	Help_id		help_msg;	// context sensitive help, may be 0
	int		cdata;		// client data used in callbacks, or
					// number of buttons for cascading menus
	const char	*accelerator;	// for mouseless operations, if specified,
					// must be unique across all debugger's
					// accelerators
};

// descriptor for each entry in the menu bar
struct Menu_bar_table
{
	const char		*label;		// button label
	const Menu_table	*table;		// menu's descriptor
	unsigned char		mnemonic;	// must be unique within menu bar
	int			nbuttons;	// number of buttons in the menu
	Help_id			help_msg;	// context sensitive help, may be 0
};

class Menu : public Component
{
	#include		"MenuP.h"

	Base_window		*window;
	Window_set		*window_set;
	int			nbuttons;
	List			children;
	const Menu_table	*table;

public:
				Menu(Component *parent, const char *name,
					Boolean title, Widget pwidget,
					Base_window *window, Window_set *window_set,
					const Menu_table *, int num_buttons,
					Help_id help_msg = HELP_none);
				~Menu();

				// access functions
	Base_window		*get_window()		{ return window; }
	Window_set		*get_window_set()	{ return window_set; }
	int			get_nbuttons()		{ return nbuttons; }
	const Menu_table	*get_table()		{ return table; }
	Menu			*find_item(char *);
	void			add_item(Menu_table *);
	void			delete_item(char *);

	Menu			*first_child()	{ return (Menu *)children.first(); }
	Menu			*next_child()	{ return (Menu *)children.next(); }

				// set the sensitivity of an individual button
	void			set_sensitive(int button, Boolean);
};

class Menu_bar : public Component
{
	#include	"Menu_barP.h"

	const Menu_bar_table *bar_table;
	int		nbuttons;
	Menu		**children;

public:
			Menu_bar(Component *parent, Base_window *window,
				Window_set *window_set, const Menu_bar_table *,
				int nbuttons, Help_id help_msg = HELP_none);
			~Menu_bar();

			// access functions
	Menu		**get_menus()	{ return children; }
	int		get_nbuttons()	{ return nbuttons; }
	Menu		*find_item(char *label);
};

#endif	// _MENU_H
