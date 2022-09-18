/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_TEXT_AREAP_H
#define	_TEXT_AREAP_H
#ident	"@(#)debugger:libol/common/Text_areaP.h	1.1"

// toolkit specific members of the Text_area class
// included by ../../gui.d/common/Text_area.h

protected:
	Callback_ptr	select_cb;
	Widget		text_area;
	XFontStruct	*font;
	Position	*tab_table;
	void		*textbuf;
	char		*string;
	Boolean		editable;

	void		setup_tab_table();

public:
			Text_area(Component *, const char *name, void *creator,
				Callback_ptr select_cb, Help_id help_msg);

	virtual void	set_selection(int start, int end);

#endif	// _TEXT_AREAP_H
