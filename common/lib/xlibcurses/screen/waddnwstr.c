/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/waddnwstr.c	1.2.2.2"
#ident  "$Header: waddnwstr.c 1.2 91/06/27 $"
#include	"curses_inc.h"

/*
**	Add to 'win' at most n 'characters' of code starting at (cury,curx)
*/
waddnwstr(win,code,n)
WINDOW	*win;
wchar_t	*code;
int	n;
{
	register char	*sp;
	extern char 	*_strcode2byte();

	/* translate the process code to character code */
	if((sp = _strcode2byte(code,NULL,n)) == NULL)
		return ERR;

	/* now call waddnstr to do the real work */
	return waddnstr(win,sp,-1);
}
