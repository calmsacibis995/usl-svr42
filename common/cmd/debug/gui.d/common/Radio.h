/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_RADIO_H
#define	_RADIO_H
#ident	"@(#)debugger:gui.d/common/Radio.h	1.1"

#include "Component.h"

// A radio list is a vertical array of buttons
// one and only one button may be selected

// Framework callbacks:
// there is one callback function associated with the entire list
// the callback is invoked as
//	creator->function((Radio_list *)this, (int)selection_button)

class Radio_list : public Component
{
	#include	"RadioP.h"

private:
        Callback_ptr    callback;

public:
			Radio_list(Component *parent, const char *name, Orientation,
				const char **buttons, int cnt, int initial_button,
				Callback_ptr fn = 0, void *creator = 0,
				Help_id help_msg = HELP_none);
			~Radio_list();

	int		which_button();		// 0 to nbuttons-1
	void		set_button(int button);	// 0 to nbuttons-1
};

#endif	// _RADIO_H
