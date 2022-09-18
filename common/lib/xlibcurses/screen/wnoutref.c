/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/wnoutref.c	1.14.2.2"
#ident  "$Header: wnoutref.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/* Like refresh but does not output */

wnoutrefresh(win)
register	WINDOW	*win;
{
    register	short	*bch, *ech, *sbch, *sech;
    register	chtype	**wcp, **scp, *wc, *sc;
    register	int	*hash;
    int		y, x, xorg, yorg, scrli, scrco,
		boty, sminy, smaxy, minx, maxx, lo, hi;
    bool	doall;

    if (win->_parent)
	wsyncdown(win);

    doall = win->_clear;

    sminy = SP->Yabove;
    smaxy = sminy + LINES;
    scrli = curscr->_maxy;
    scrco = curscr->_maxx;

    yorg = win->_begy + win->_yoffset;
    xorg = win->_begx;

    /* save flags, cursor positions */
    SP->virt_scr->_leave = win->_leave;
    if ((!win->_leave || (win->_flags & (_WINCHANGED | _WINMOVED))) &&
	((y = win->_cury + yorg) >= 0) && (y < scrli) &&
	((x = win->_curx + xorg) >= 0) && (x < scrco))
    {
	_virtscr->_cury = y;
	_virtscr->_curx = x;
    }
    if (!(win->_use_idc))
	_virtscr->_use_idc = FALSE;
    if (win->_use_idl)
	_virtscr->_use_idl = TRUE;
    if (win->_clear)
    {
	_virtscr->_clear = TRUE;
	win->_clear = FALSE;
	win->_flags |= _WINCHANGED;
    }

    if (!(win->_flags & _WINCHANGED))
	goto done;

    /* region to update */
    boty = win->_maxy+yorg;
    if (yorg >= sminy && yorg < smaxy && boty >= smaxy)
	boty = smaxy;
    else
	if (boty > scrli)
	    boty = scrli;
    boty -= yorg;

    minx = 0;
    if ((maxx = win->_maxx+xorg) > scrco)
	maxx = scrco;
    maxx -= xorg + 1;

    /* update structure */
    bch = win->_firstch;
    ech = win->_lastch;
    wcp = win->_y;

    hash = _VIRTHASH + yorg;
    sbch = _virtscr->_firstch + yorg;
    sech = _virtscr->_lastch + yorg;
    scp  = _virtscr->_y + yorg;

    /* first time around, set proper top/bottom changed lines */
    if (curscr->_sync)
    {
	_VIRTTOP = scrli;
	_VIRTBOT = -1;
    }

    /* update each line */
    for (y = 0; y < boty; ++y, ++hash, ++bch, ++ech, ++sbch, ++sech, ++wcp, ++scp)
    {
	if (!doall && *bch == _INFINITY)
	    continue;

	lo = (doall || *bch == _REDRAW || *bch < minx) ? minx : *bch;
	hi = (doall || *bch == _REDRAW || *ech > maxx) ? maxx : *ech;

	wc = *wcp;
	sc = *scp;
	/* adjust lo and hi so they contain whole characters */
	if(_scrmax > 1)
		{
		if(ISCBIT(wc[lo]))
			{
			for(x = lo-1; x >= minx; --x)
				if(!ISCBIT(wc[x]))
					break;
			if(x < minx)
				{
				for(x = lo+1; x <= maxx; ++x)
					if(!ISCBIT(wc[x]))
						break;
				if(x > maxx)
					goto nextline;
				}
			lo = x;
			}
		if(ISMBIT(wc[hi]))
			{
			register int	w;
			register char	rb;
			for(x = hi; x >= lo; --x)
				if(!ISCBIT(wc[x]))
					break;
			rb = RBYTE(wc[x]);
			w = _scrwidth[TYPE(rb)];
			hi = (x+w) <= maxx+1 ? x+w-1 : x;
			}
		}

	if (hi < lo)
	    continue;

	/* clear partial multi-chars about to be overwritten */
	if(_scrmax > 1)
		{
		if(ISMBIT(sc[lo+xorg]))
			_mbclrch(_virtscr,y+yorg,lo+xorg);
		if(ISMBIT(sc[hi+xorg]))
			_mbclrch(_virtscr,y+yorg,hi+xorg);
		}

	/* update the change structure */
	if (*bch == _REDRAW || *sbch == _REDRAW)
	    *sbch =  _REDRAW;
	else
	{
	    if (*sbch > lo+xorg)
		*sbch = lo+xorg;
	    if (*sech < hi+xorg)
		*sech = hi+xorg;
	}
	if ((y + yorg) < _VIRTTOP)
	    _VIRTTOP = y+yorg;
	if ((y + yorg) > _VIRTBOT)
	    _VIRTBOT = y + yorg;

	/* update the image */
	wc = *wcp + lo;
	sc = *scp + lo + xorg;
	(void) memcpy((char *) sc, (char *) wc, (int) (((hi - lo) + 1) * sizeof(chtype)));

	/* the hash value of the line */
	*hash = _NOHASH;

nextline:
	*bch = _INFINITY;
	*ech = -1;
    }

done:
    _virtscr->_flags |= _WINCHANGED;
    win->_flags &= ~(_WINCHANGED | _WINMOVED | _WINSDEL);
    return (OK);
}
