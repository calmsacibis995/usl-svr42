/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_TEXT_DISP_H
#define	_TEXT_DISP_H
#ident	"@(#)debugger:gui.d/common/Text_disp.h	1.3"

#include "Component.h"
#include "Text_area.h"

// cfront 2.1 requires class name in constructor, 1.2 doesn't accept it
#ifdef __cplusplus
#define TEXT_DISPLAY	Text_display
#else
#define TEXT_DISPLAY
#endif

enum Search_return { SR_bad_expression, SR_found, SR_notfound };

class Text_display : public Text_area
{
	#include	"Text_dispP.h"
public:
			Text_display(Component *, const char *name,
				Callback_ptr select_cb = 0,
				void *creator = 0, Help_id help_msg = HELP_none);
			~Text_display();

			// initialization routines
	void		setup_source_file(const char *path, int rows, int columns);
	void		setup_disassembly(int rows, int columns);
	void		set_file(const char *path);
	void		set_buf(const char *buf);
	void		set_breaklist(int *breaklist);

	void		set_line(int line);
	void		position(int line, Boolean highlight);
	int		has_breakpoint(int line);
	void		set_stop(int line);
	void		clear_stop(int line);
	Search_return	search(const char *expr, int forwards);
	void		clear();

			// access functions
	int		get_last_line();
	int		get_position();
};

#endif	// _TEXT_DISP_H
