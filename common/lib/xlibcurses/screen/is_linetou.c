/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/is_linetou.c	1.3.2.2"
#ident  "$Header: is_linetou.c 1.2 91/06/26 $"
#include	"curses_inc.h"

is_linetouched(win,line)
WINDOW	*win;
int	line;
{
    if (line < 0 || line >= win->_maxy)
        return (ERR);
    if (win->_firstch[line] == _INFINITY)
	return (FALSE);
    else
	return (TRUE);
}
