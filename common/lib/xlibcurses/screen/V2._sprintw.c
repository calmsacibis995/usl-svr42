/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/V2._sprintw.c	1.7.2.2"
#ident  "$Header: V2._sprintw.c 1.2 91/06/26 $"

# include	"curses_inc.h"
#ifdef __STDC__
#include	<stdarg.h>
#else
# include	<varargs.h>
#endif

#ifdef _VR2_COMPAT_CODE
/*
	This is only here for compatibility with SVR2 curses.
	It will go away someday. Programs should reference
	vwprintw() instead.
 */

_sprintw(win, fmt, ap)
register WINDOW	*win;
register char * fmt;
va_list ap;
{
	return vwprintw(win, fmt, ap);
}
#endif
