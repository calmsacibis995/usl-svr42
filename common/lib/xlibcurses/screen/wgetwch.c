/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/wgetwch.c	1.3.2.2"
#ident  "$Header: wgetwch.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/*
**	Get a process code
*/

wgetwch(win)
WINDOW	*win;
	{
	int	c, n, type, width;
	char	buf[CSMAX];
	wchar_t	wchar;
	int	length;

	/* get the first byte */
	if((c = wgetch(win)) == ERR)
		return ERR;

	type = TYPE(c);
	width = cswidth[type] - ((type == 1 || type == 2) ? 0 : 1);
	buf[0] = c;
	for(n = 1; n <= width; ++n)
		{
		if((c = wgetch(win)) == ERR)
			return ERR;
		if(TYPE(c) != 0)
			return ERR;
		buf[n] = c;
		}

	/* translate it to process code */
	if ((length = _curs_mbtowc(&wchar, buf, n)) < 0)
		return ERR;
	return(wchar);
	}
