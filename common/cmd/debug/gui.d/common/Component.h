/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_COMPONENT_H
#define _COMPONENT_H
#ident	"@(#)debugger:gui.d/common/Component.h	1.1"

#include "Toolkit.h"
#include "Help.h"

// cfront 2.1 requires class name in constructor, 1.2 doesn't accept it
#ifdef __cplusplus
#define COMPONENT	Component
#else
#define COMPONENT
#endif

class Message;
class Base_window;
class Window_set;

// Components - all are derived from Component base class
class Alert_shell;
class Caption;
class Component;
class Dialog_shell;
class Divided_box;
class Expansion_box;
class Menu;
class Menu_bar;
class Packed_box;
class Radio_list;
class Selection_list;
class Simple_text;
class Table;
class Text_area;
class Text_display;
class Text_line;
class Toggle_button;
class Window_shell;

enum Component_type
{
	WINDOW_SHELL,
	DIALOG_SHELL,
	OTHER
};

enum Select_mode
{
	SM_single,
	SM_multiple,
};

enum Orientation
{
	OR_horizontal,
	OR_vertical
};

// Framework callbacks all have the same form,
// 1st argument - pointer the framework object that created the Component
// 2nd argument - pointer to the component
// 3rd argument - Component/callback specific - for most, simply zero
typedef void (*Callback_ptr)(void *, Component *, void *);

class Component
{
protected:
	Widget		widget;
	const char	*label;
	Component	*parent;
	void		*creator;	// framework object, used by callbacks
	Help_id		help_msg;
	Component_type	type;

public:
		Component(Component *p, const char *l, void *c, Help_id h,
				Component_type t = OTHER) 
			{ parent = p; label = l; creator = c; help_msg = h; type = t; }
	virtual	~Component();

			// access functions
	Widget		get_widget()		{ return widget; }
	Component	*get_parent()		{ return parent; }
	void		*get_creator()		{ return creator; }
	Help_id		get_help_msg()		{ return help_msg; }
	Component_type	get_type()		{ return type; }
	const char	*get_label()		{ return label; }

			// walk up the tree to find the base window
	Base_window	*get_base();
};

#endif // _COMPONENT_H
