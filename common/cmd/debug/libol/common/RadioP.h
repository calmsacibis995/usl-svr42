/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_RADIOP_H
#define	_RADIOP_H
#ident	"@(#)debugger:libol/common/RadioP.h	1.2"

// toolkit specific members of Radio class,
// included by ../../gui.d/common/Radio.h

// Radio list is implemented as a FlatButton widget
private:
	void		*buttons; 	// list of entries for creating flatE buttons
        int             nbuttons;       // total number of buttons
        int             current;        // button currently pushed

public:
			// access functions
	int		get_nbuttons()	{ return nbuttons; }
	Callback_ptr	get_callback()	{ return callback; }
	void		set_current(int i)	{ current = i; }

#endif	// _RADIOP_H
