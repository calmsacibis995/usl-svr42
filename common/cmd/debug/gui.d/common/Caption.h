/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_CAPTION_H
#define	_CAPTION_H
#ident	"@(#)debugger:gui.d/common/Caption.h	1.1"

#include "Component.h"

// The string passed to the Caption constructor is used to label the caption's child
enum Caption_position
{
	CAP_LEFT,
	CAP_TOP_LEFT,
	CAP_TOP_CENTER
};

class Caption : public Component
{
	Component       *child;

	#include	"CaptionP.h"

public:
			Caption(Component *parent, const char *caption,
				Caption_position, Help_id help_msg = HELP_none);
			~Caption();

			// accepts one child only
	void		add_component(Component *, Boolean resizable = TRUE);
	void		set_label(const char *label);	// changes the label
};

#endif	// _CAPTION_H
