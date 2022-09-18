/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/printw.c	1.10.2.2"
#ident  "$Header: printw.c 1.2 91/06/26 $"
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
 *	This routine implements a printf on the standard screen.
 */
/*VARARGS1*/
#ifdef __STDC__
printw(char *fmt, ...)
#else
printw(va_alist)
va_dcl
#endif
{
#ifndef __STDC__
	register char * fmt;
#endif
	va_list ap;

#ifdef __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
	fmt = va_arg(ap, char *);
#endif
	return vwprintw(stdscr, fmt, ap);
}
