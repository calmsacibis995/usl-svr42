/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/waddchnstr.c	1.4.2.2"
#ident  "$Header: waddchnstr.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/*
 * Add ncols worth of data to win, using string as input.
 * Return the number of chtypes copied.
 */
waddchnstr(win, string, ncols)
register	WINDOW	*win;
chtype		*string;
int		ncols;
{
    int		my_x = win->_curx;
    int		my_y = win->_cury;
    int		my_maxx;
    int		counter;
    chtype	*ptr = &(win->_y[my_y][my_x]);
    int		remcols;
    int		b;
    int		sw;
    int		ew;

    if (ncols < 0) {
	remcols = win->_maxx - my_x;
	while (*string && remcols) {
	    sw = mbscrw(_CHAR(*string));
	    ew = mbeucw(_CHAR(*string));
	    if (remcols < sw)
		    break;
	    for (b = 0; b < ew; b++) {
		if (waddch(win, *string++) == ERR)
			goto out;
	    }
	    remcols -= sw;
	}
    }
    else
    {
	remcols = win->_maxx - my_x;
	while ((*string) && (remcols > 0) && (ncols > 0)) {
	    sw = mbscrw(_CHAR(*string));
	    ew = mbeucw(_CHAR(*string));
	    if ((remcols < sw) || (ncols < ew))
		break;
	    for (b = 0; b < ew; b++) {
		if (waddch(win, *string++) == ERR)
			goto out;
	    }
	    remcols -= sw;
	    ncols -= ew;
	}
    }
out:
    /* restore cursor position */
    win->_curx = my_x;
    win->_cury = my_y;

    win->_flags |= _WINCHANGED;

    /* sync with ancestor structures */
    if (win->_sync)
	wsyncup(win);

    return (win->_immed ? wrefresh(win) : OK);
}
