/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_TOGGLEP_H
#define	_TOGGLEP_H
#ident	"@(#)debugger:libol/common/ToggleP.h	1.1"

// toolkit specific members of Toggle_button class
// included by ../../gui.d/common/Toggle.h

// OpenLook version of Toggle is implemented as a checkbox Widget
// One of the friend functions is called when the user selects the checkbox

private:
	void			*toggles;
        const Toggle_data       *buttons;
        int                     nbuttons;

public:
				// access functions
	void			*get_toggles()	{ return toggles; }
	const Toggle_data	*get_buttons()	{ return buttons; }
	int			get_nbuttons()	{ return nbuttons; }

#endif	// _TOGGLEP_H
