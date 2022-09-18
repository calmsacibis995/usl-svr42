/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_BOXES_H
#define	_BOXES_H
#ident	"@(#)debugger:gui.d/common/Boxes.h	1.3"

#include "Component.h"
#include "List.h"

// Components in a packed box are placed one after another in the major dimension
// (horizontal or vertical).  Components do not change size or position if the
// box is resized in the major dimension; they are resized in minor dimension
// in all box types.
class Packed_box : public Component
{
	List            children;

	#include "Pack_boxP.h"

public:
			Packed_box(Component *parent, const char *name,
				Orientation, Help_id help_msg = HELP_none);
			~Packed_box();

	void		add_component(Component *);
};

// One or more children of an Expansion_box are elastic components.
// When the box is resized in the major dimension, the elastic components
// are resized.  All others are unchanged
class Expansion_box : public Component
{
	List            children;

	#include "Expn_boxP.h"
public:
			Expansion_box(Component *parent, const char *name,
				Orientation, Help_id help_msg = HELP_none);
			~Expansion_box();

	void		add_component(Component *child);
	void		add_elastic(Component *child);
};

// A Divided_box is vertical only.  The user may re-apportion the panes
// within the box by clicking and dragging sashes
class Divided_box : public Component
{
	List            children;

	#include "Divd_boxP.h"
public:
			Divided_box(Component *parent, const char *name,
				Help_id help_msg = HELP_none);
			~Divided_box();

	void		add_component(Component *child);
};

#endif	// _BOXES_H
