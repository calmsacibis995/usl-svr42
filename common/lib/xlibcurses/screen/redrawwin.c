/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/redrawwin.c	1.3.2.2"
#ident  "$Header: redrawwin.c 1.2 91/06/26 $"
#include	"curses_inc.h"

redrawwin(win)
WINDOW	*win;
{
    return (wredrawln(win,0,win->_maxy));
}
