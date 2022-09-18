/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/wscanw.c	1.10.2.2"
#ident  "$Header: wscanw.c 1.2 91/06/27 $"
/*
 * scanw and friends
 *
 */

# include	"curses_inc.h"

#ifdef __STDC__
#include	<stdarg.h>
#else
#include <varargs.h>
#endif

/*
 *	This routine implements a scanf on the given window.
 */
/*VARARGS*/
#ifdef __STDC__
wscanw(WINDOW *win, ...)
#else
wscanw(va_alist)
va_dcl
#endif
{
#ifndef __STDC__
	register WINDOW	*win;
#endif
	register char	*fmt;
	va_list	ap;

#ifdef __STDC__
	va_start(ap, win);
#else
	va_start(ap);
	win = va_arg(ap, WINDOW *);
#endif
	fmt = va_arg(ap, char *);
	return vwscanw(win, fmt, ap);
}
