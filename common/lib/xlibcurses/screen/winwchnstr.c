/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/winwchnstr.c	1.3.2.2"
#ident  "$Header: winwchnstr.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/*
 * Read in ncols worth of data from window win and assign the
 * chars to string. NULL terminate string upon completion.
 * Return the number of chtypes copied.
 */

winwchnstr(win,string,ncols)
register	WINDOW	*win;
chtype		*string;
int		ncols;
{
    register	chtype	*ptr = &(win->_y[win->_cury][win->_curx]);
    register	int	counter = 0;
    register	int	maxcols = win->_maxx - win->_curx;
    int			eucw, scrw, s, wc;
    char		*mp, mbbuf[CSMAX+1];
    wchar_t		wch;
    chtype		rawc;
#ifdef	_WCHAR16
    chtype		attr;
#endif	/*_WCHAR16*/

    if (ncols < 0)
	ncols = MAXINT;

    while (ISCBIT(*ptr))
    {
	ptr--;
	maxcols++;
    }

    while ((counter < ncols) && maxcols > 0)
    {
#ifdef	_WCHAR16
	attr = _ATTR(*ptr);
	rawc = _CHAR(*ptr);
#else	/*_WCHAR16*/
	rawc = *ptr;
#endif	/*_WCHAR16*/
	eucw = mbeucw(RBYTE(rawc));
	scrw = mbscrw(RBYTE(rawc));
	for (mp = mbbuf, s = 0; s < scrw; s++, maxcols--, ptr++)
	{
	    if ((wc = RBYTE(rawc)) == MBIT)
		continue;
	    *mp++ = wc;
	    if ((wc = LBYTE(rawc) | MBIT) == MBIT)
		continue;
	    *mp++ = wc;
	}
	*mp = '\0';
	if (_curs_mbtowc(&wch, mbbuf, CSMAX) <= 0)
	    break;
#ifdef	_WCHAR16
	*string++ = wch | attr;
#else	/*_WCHAR16*/
	*string++ = wch;
#endif	/*_WCHAR16*/
	counter++;
    }
    if (counter < ncols)
	*string = (chtype) 0;
    return (counter);
}
