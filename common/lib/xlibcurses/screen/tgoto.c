/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/tgoto.c	1.5.2.2"
#ident  "$Header: tgoto.c 1.2 91/06/27 $"
/*
 * tgoto: function included only for upward compatibility with old termcap
 * library.  Assumes exactly two parameters in the wrong order.
 */
extern	char	*tparm();

char	*
tgoto(cap, col, row)
char	*cap;
int	col, row;
{
    char	*cp;

    cp = tparm(cap, row, col);
    return (cp);
}
