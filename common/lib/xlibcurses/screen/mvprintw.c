/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/mvprintw.c	1.8.2.2"
#ident  "$Header: mvprintw.c 1.2 91/06/26 $"
# include	"curses_inc.h"

#ifdef __STDC__
#include	<stdarg.h>
#else
#include <varargs.h>
#endif

/*
 * implement the mvprintw commands.  Due to the variable number of
 * arguments, they cannot be macros.  Sigh....
 *
 */

/*VARARGS*/
#ifdef __STDC__
mvprintw(int y, int x, ...)
#else
mvprintw(va_alist)
va_dcl
#endif
{
#ifndef __STDC__
	register int	y, x;
#endif
	register char * fmt;
	va_list ap;

#ifdef __STDC__
	va_start(ap, x);
#else
	va_start(ap);
	y = va_arg(ap, int);
	x = va_arg(ap, int);
#endif
	fmt = va_arg(ap, char *);
	return move(y, x) == OK ? vwprintw(stdscr, fmt, ap) : ERR;
}
