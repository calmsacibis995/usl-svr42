/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/mvwin.c	1.9.2.2"
#ident  "$Header: mvwin.c 1.2 91/06/26 $"
#include	"curses_inc.h"

/* relocate the starting position of a _window */

mvwin(win, by, bx)
register	WINDOW	*win;
register	int	by, bx;
{
    if ((by + win->_maxy) > LINES || (bx + win->_maxx) > COLS ||
	 by < 0 || bx < 0)
         return ERR;
    win->_begy = by;
    win->_begx = bx;
    (void) wtouchln(win, 0, win->_maxy, -1);
    return (win->_immed ? wrefresh(win) : OK);
}
