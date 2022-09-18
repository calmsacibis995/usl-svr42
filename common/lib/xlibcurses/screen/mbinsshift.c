/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/mbinsshift.c	1.2.2.2"
#ident  "$Header: mbinsshift.c 1.2 91/06/26 $"
#include	"curses_inc.h"

/*
**	Shift right an interval of characters
*/
_mbinsshift(win,len)
WINDOW	*win;
int	len;
	{
	reg int		x, y, maxx, mv;
	reg chtype	*wcp, *wp, *ep;

	y = win->_cury;
	x = win->_curx;
	maxx = win->_maxx;
	wcp  = win->_y[y];

	/* ASSERT(!ISCBIT(wcp[x])); */

	/* shift up to a whole character */
	if(_scrmax > 1)
		{
		wp = wcp+maxx-1;
		if(ISMBIT(*wp))
			{
			reg chtype	rb;

			for(; wp >= wcp; --wp)
				if(!ISCBIT(*wp))
					break;
			if(wp < wcp)
				return ERR;
			rb = RBYTE(*wp);
			if((wp+_scrwidth[TYPE(rb)]) > (wcp+maxx))
				maxx = wp - wcp;
			}
		}

	/* see if any data need to move */
	if((mv = maxx - (x+len)) <= 0)
		return OK;

	/* the end of the moved interval must be whole */
	if(ISCBIT(wcp[x+mv]))
		_mbclrch(win,y,x+mv-1);

	/* move data */
	ep = wcp+x+len;
	for(wp = wcp+maxx-1; wp >= ep; --wp)
		*wp = *(wp-len);

	/* clear a possible partial multibyte character */
	if(ISMBIT(*wp))
		for(ep = wp; ep >= wcp; --ep)
		{
		mv = ISCBIT(*ep);
		*ep = win->_bkgd;
		if(!mv)
			break;
		}

	/* update the change structure */
	if(x < win->_firstch[y])
		win->_firstch[y] = x;
	win->_lastch[y] = maxx-1;

	return OK;
	}
