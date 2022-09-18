/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/winwch.c	1.2.2.2"
#ident  "$Header: winwch.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/*
**	Get a process code at (curx,cury).
*/
chtype winwch(win)
WINDOW	*win;
{
	wchar_t	wchar;
	int	length;
#ifdef	_WCHAR16
	chtype	a;

	a = _ATTR(win->_y[win->_cury][win->_curx]);
#endif	/* _WCHAR16 */

	length = _curs_mbtowc(&wchar,wmbinch(win,win->_cury,win->_curx),
							sizeof(wchar_t));
#ifdef	_WCHAR16
	return(a | wchar);
#else	/* _WCHAR16 */
	return (wchar);
#endif	/* _WCHAR16 */
}
