/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/curs_set.c	1.8.2.3"
#ident  "$Header: curs_set.c 1.2 91/06/26 $"

#include	"curses_inc.h"

/* Change the style of cursor in use. */

curs_set(visibility)
register	int	visibility;
{
#ifdef __STDC__
    extern  int     _outch(int);
#else
    extern  int     _outch();
#endif
    int		ret = cur_term->_cursorstate;
    char	**cursor_seq = cur_term->cursor_seq;

    if ((visibility < 0) || (visibility > 2) || (!cursor_seq[visibility]))
	ret = ERR;
    else
	if (visibility != ret)
	    tputs(cursor_seq[cur_term->_cursorstate = visibility], 0, _outch);
    (void) fflush(SP->term_file);
    return (ret);
}
