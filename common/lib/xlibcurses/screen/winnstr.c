/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/winnstr.c	1.4.2.2"
#ident  "$Header: winnstr.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/*
 * Copy n chars in window win from current cursor position to end
 * of window into char buffer str.  Return the number of chars copied.
 */

winnstr(win, str, ncols)
register	WINDOW	*win;
register	char	*str;
register	int	ncols;
{
    register	int	counter = 0;
    int			cy = win->_cury;
    register	chtype	*ptr = &(win->_y[cy][win->_curx]),
			*pmax = &(win->_y[cy][win->_maxx]);
    chtype		wc;
    int			eucw, scrw, s;


    while (ISCBIT(*ptr))
	ptr--;

    if (ncols < -1)
	ncols = MAXINT;

    while (counter < ncols)
    {
	scrw = mbscrw(RBYTE(*ptr));
	eucw = mbeucw(RBYTE(*ptr));
	if (counter + eucw > ncols)
	    break;

	for (s = 0; s < scrw; s++, ptr++)
	{
	    if ((wc = RBYTE(*ptr)) == MBIT)
		continue;
	    *str++ = wc;
	    counter++;
	    if ((wc = LBYTE(*ptr) | MBIT) == MBIT)
		continue;
	    *str++ = wc;
	    counter++;
	}

	if (ptr >= pmax)
	{
	    if (++cy == win->_maxy)
		break;

	    ptr = &(win->_y[cy][0]);
	    pmax = ptr + win->_maxx;
	}
    }
    if (counter < ncols)
	*str = '\0';

    return (counter);
}
