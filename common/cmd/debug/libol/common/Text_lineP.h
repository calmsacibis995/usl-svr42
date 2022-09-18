/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _TEXT_LINEP_H
#define	_TEXT_LINEP_H
#ident	"@(#)debugger:libol/common/Text_lineP.h	1.1"

// toolkit specific members of the Text_line class
// included by ../../gui.d/common/Text_line.h

private:
        Callback_ptr    return_cb;
        Callback_ptr    get_sel_cb;
        Callback_ptr    lost_sel_cb;
        Boolean         editable;
        char            *string;        // string most recently retrieved by get_text

public:
			// access functions
	Callback_ptr	get_return_cb()	{ return return_cb; }

#endif	// _TEXT_LINEP_H
