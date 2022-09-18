/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_STEXT_H
#define	_STEXT_H
#ident	"@(#)debugger:gui.d/common/Stext.h	1.1"

#include "Component.h"

class Simple_text : public Component
{
	#include	"StextP.h"

public:
			Simple_text(Component *parent, const char *text,
				Boolean resize, Help_id help_msg = HELP_none);
			~Simple_text();

	void		set_text(const char *text);	// changes the display
	void		clear();			// blank out the display
};

#endif	// _STEXT_H
