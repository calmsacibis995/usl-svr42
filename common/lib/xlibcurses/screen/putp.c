/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/putp.c	1.10.2.3"
#ident  "$Header: putp.c 1.2 91/06/26 $"
/*
 * Handy functions to put out a string with padding.
 * These make two assumptions:
 *	(1) Output is via stdio to stdout through putchar.
 *	(2) There is no count of affected lines.  Thus, this
 *	    routine is only valid for certain capabilities,
 *	    i.e. those that don't have *'s in the documentation.
 */
#include	"curses_inc.h"

/*
 * Routine to act like putchar for passing to tputs.
 * _outchar should really be a void since it's used by tputs
 * and tputs doesn't look at return code.  However, tputs also has the function
 * pointer declared as returning an int so we didn't change it.
 */
#ifdef __STDC__
_outchar(int ch)
#else
_outchar(ch)
char	ch;
#endif
{
    (void) putchar(ch);
}

/* Handy way to output a string. */

putp(str)
char	*str;
{
    return (tputs(str, 1, _outchar));
}

/* Handy way to output video attributes. */

vidattr(newmode)
chtype	newmode;
{
    return (vidputs(newmode, _outchar));
}
