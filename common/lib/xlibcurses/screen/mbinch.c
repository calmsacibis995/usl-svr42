/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/mbinch.c	1.2.2.2"
#ident  "$Header: mbinch.c 1.2 91/06/26 $"
#include	"curses_inc.h"

/*
**	Get the (y,x) character of a window and
**	return it in a 0-terminated string.
*/
char	*wmbinch(win,y,x)
WINDOW	*win;
int	y, x;
{
	reg int		k, savx, savy;
	reg chtype	*wp, *ep, wc;
	static char	rs[CSMAX+1];

	k = 0;
	savx = win->_curx;
	savy = win->_cury;

	if(wmbmove(win,y,x) == ERR)
		goto done;
	wp = win->_y[win->_cury] + win->_curx;
	wc = RBYTE(*wp);
	ep = wp + _scrwidth[TYPE(wc & 0377)];

	for(; wp < ep; ++wp)
	{
		if((wc = RBYTE(*wp)) == MBIT)
			break;
		rs[k++] = wc;
		if((wc = LBYTE(*wp)|MBIT) == MBIT)
			break;
		rs[k++] = wc;
	}
done :
	win->_curx = savx;
	win->_cury = savy;
	rs[k] = '\0';
	return rs;
}
