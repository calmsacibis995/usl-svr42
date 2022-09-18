/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/setcurscreen.c	1.5.2.2"
#ident  "$Header: setcurscreen.c 1.2 91/06/26 $"
#include "curses_inc.h"

SCREEN	*
setcurscreen(new)
SCREEN	*new;
{
    register	SCREEN	*rv = SP;

    if (new != SP)
    {

#ifdef	DEBUG
	if (outf)
	    fprintf(outf, "setterm: old %x, new %x\n", rv, new);
#endif	/* DEBUG */

	SP = new;
	(void) setcurterm(SP->tcap);
	LINES = SP->lsize;
	COLS = SP->csize;
	TABSIZE = SP->tsize;
	stdscr = SP->std_scr;
	curscr = SP->cur_scr;
	_virtscr = SP->virt_scr;
    }
    return (rv);
}
