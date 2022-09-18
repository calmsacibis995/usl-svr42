/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/_mvwinwstr.c	1.1.2.2"
#ident  "$Header: _mvwinwstr.c 1.2 91/06/26 $"
#define		NOMACROS
#include	"curses_inc.h"

int
mvwinwstr(win,y,x,ws)
WINDOW *win; 
int y; 
int x; 
wchar_t *ws;
{ 
	return((wmove(win,y,x)==ERR?ERR:winwstr(win,ws))); 
}
