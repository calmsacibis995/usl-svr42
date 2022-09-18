/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/nodelay.c	1.6.2.2"
#ident  "$Header: nodelay.c 1.2 91/06/26 $"
/*
 * Routines to deal with setting and resetting modes in the tty driver.
 * See also setupterm.c in the termlib part.
 */
#include "curses_inc.h"

/*
 * TRUE => don't wait for input, but return -1 instead.
 */

nodelay(win,bf)
WINDOW *win;
bool   bf;
{
    win->_delay = (bf) ? 0 : -1;
    return (OK);
}
