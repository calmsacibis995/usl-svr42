/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/_insnstr.c	1.1.2.2"
#ident  "$Header: _insnstr.c 1.2 91/06/26 $"

#define		NOMACROS
#include	"curses_inc.h"

insnstr(s, n)
char	*s;
int	n;
{
    return (winsnstr(stdscr, s, n));
}
