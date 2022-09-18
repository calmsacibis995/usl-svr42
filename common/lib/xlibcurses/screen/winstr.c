/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/winstr.c	1.4.2.2"
#ident  "$Header: winstr.c 1.2 91/06/27 $"
#include	"curses_inc.h"

winstr(win, str)
register	WINDOW	*win;
register	char	*str;
{
    register	int	counter = 0;
    int			cy = win->_cury;
    register	chtype	*ptr = &(win->_y[cy][win->_curx]),
			*pmax = &(win->_y[cy][win->_maxx]);
    chtype		*p1st = &(win->_y[cy][0]);
    chtype		wc;
    int			ew, sw, s;

    while (ISCBIT(*ptr) && (p1st < ptr))
	ptr--;

    while (ptr < pmax)
    {
	wc = RBYTE(*ptr);
	sw = mbscrw(wc);
	ew = mbeucw(wc);
	for (s = 0; s < sw; s++, ptr++)
	{
	    if ((wc = RBYTE(*ptr)) == MBIT)
		continue;
	    str[counter++] = wc;
	    if ((wc = LBYTE(*ptr) | MBIT) == MBIT)
		continue;
	    str[counter++] = wc;
	}
    }
    str[counter] = '\0';

    return (counter);
}
