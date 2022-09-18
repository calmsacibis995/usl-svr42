/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/vwprintw.c	1.7.2.2"
#ident  "$Header: vwprintw.c 1.2 91/06/27 $"
/*
 * printw and friends
 *
 */

# include	"curses_inc.h"

#ifdef __STDC__
#include	<stdarg.h>
#else
#include <varargs.h>
#endif

/*
 *	This routine actually executes the printf and adds it to the window
 *
 *	This code now uses the vsprintf routine, which portably digs
 *	into stdio.  We provide a vsprintf for older systems that don't
 *	have one.
 */

/*VARARGS2*/
vwprintw(win, fmt, ap)
register WINDOW	*win;
register char * fmt;
va_list ap;
{
	char	buf[BUFSIZ];
	register int n;

	n = vsprintf(buf, fmt, ap);
	va_end(ap);
	if (n == ERR)
		return ERR;
	return waddstr(win, buf);
}
