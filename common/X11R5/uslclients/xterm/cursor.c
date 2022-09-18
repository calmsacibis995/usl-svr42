/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:cursor.c	1.16"
#endif
/*
 cursor.c (C source file)
	Acc: 601052231 Tue Jan 17 09:57:11 1989
	Mod: 601054031 Tue Jan 17 10:27:11 1989
	Sta: 601054031 Tue Jan 17 10:27:11 1989
	Owner: 7007
	Group: 1985
	Permissions: 666
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/

/* Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts. */

#ifndef lint
static char *rcsid_cursor_c = "$Header: cursor.c,v 1.1 88/02/10 13:08:04 jim Exp $";
#endif	/* lint */

#include <X11/copyright.h>

#ifndef lint
static char rcs_id[] = "$Header: cursor.c,v 1.1 88/02/10 13:08:04 jim Exp $";
#endif	/* lint */

#include <X11/Xlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include "ptyx.h"

extern void bcopy();

/*
 * Moves the cursor to the specified position, checking for bounds.
 * (this includes scrolling regions)
 * The origin is considered to be 0, 0 for this procedure.
 */
CursorSet(screen, row, col, flags)
register TScreen	*screen;
register int	row, col;
unsigned	flags;
{
	register int maxr;

	col = (col < 0 ? 0 : col);
	screen->cur_col = (col <= screen->max_col ? col : screen->max_col);
	maxr = screen->max_row;
	if (flags & ORIGIN) {
		row += screen->top_marg;
		maxr = screen->bot_marg;
	}
	row = (row < 0 ? 0 : row);
	screen->cur_row = (row <= maxr ? row : maxr);
	screen->do_wrap = 0;
}

/*
 * moves the cursor left n, no wrap around
 */
CursorBack(screen, n)
register TScreen	*screen;
int		n;
{
	register int i, j, k;
	extern XtermWidget term;
	register int old_col = screen->cur_col;

	if (screen->do_wrap)
		n--;

	if ((screen->cur_col -= n) < 0) {
		if((term->flags & (REVERSEWRAP | WRAPAROUND)) ==
         	   (REVERSEWRAP | WRAPAROUND)) {
			if((i = (j = screen->max_col + 1) * screen->cur_row +
			 screen->cur_col) < 0) {
				k = j * (screen->max_row + 1);
				i += ((-i) / k + 1) * k;
			}
			screen->cur_row = i / j;
			screen->cur_col = i % j;
		} else
			screen->cur_col = 0;
	}
	screen->do_wrap = 0;

	/* reset bellarmed if necessary */

	if (screen->marginbell && screen->bellarmed == -1) {
	    i = screen->max_col - screen->nmarginbell;
	    if (old_col >= i && screen->cur_col < i)
	        screen->bellarmed = screen->cur_row;
	}
}

/*
 * moves the cursor forward n, no wraparound
 */
CursorForward(screen, n)
register TScreen	*screen;
int		n;
{
	screen->cur_col += n;
	if (screen->cur_col > screen->max_col)
		screen->cur_col = screen->max_col;

	screen->do_wrap = 0;
}

/* 
 * moves the cursor down n, no scrolling.
 * Won't pass bottom margin or bottom of screen.
 */
CursorDown(screen, n)
register TScreen	*screen;
int		n;
{
	register int max;

	max = (screen->cur_row > screen->bot_marg ?
		screen->max_row : screen->bot_marg);

	screen->cur_row += n;
	if (screen->cur_row > max)
		screen->cur_row = max;
	screen->do_wrap = 0;
}

/* 
 * moves the cursor up n, no linestarving.
 * Won't pass top margin or top of screen.
 */
CursorUp(screen, n)
register TScreen	*screen;
int		n;
{
	register int min;

	min = (screen->cur_row < screen->top_marg ?
		0 : screen->top_marg);

	screen->cur_row -= n;
	if (screen->cur_row < min)
		screen->cur_row = min;
	screen->do_wrap = 0;
}

/* 
 * Moves cursor down amount lines, scrolls if necessary.
 * Won't leave scrolling region. No carriage return.
 */
Index(screen, amount)
register TScreen	*screen;
register int	amount;
{
	register int j;

	/* 
	 * indexing when below scrolling region is cursor down.
	 * if cursor high enough, no scrolling necessary.
	 */
	if (screen->cur_row > screen->bot_marg
	 || screen->cur_row + amount <= screen->bot_marg) {
		CursorDown(screen, amount);
		return;
	}

	CursorDown(screen, j = screen->bot_marg - screen->cur_row);
	Scroll(screen, amount - j);
}

/*
 * Moves cursor up amount lines, reverse scrolls if necessary.
 * Won't leave scrolling region. No carriage return.
 */
RevIndex(screen, amount)
register TScreen	*screen;
register int	amount;
{
	/*
	 * reverse indexing when above scrolling region is cursor up.
	 * if cursor low enough, no reverse indexing needed
	 */
	if (screen->cur_row < screen->top_marg
	 || screen->cur_row-amount >= screen->top_marg) {
		CursorUp(screen, amount);
		return;
	}

	RevScroll(screen, amount - (screen->cur_row - screen->top_marg));
	CursorUp(screen, screen->cur_row - screen->top_marg);
}

/*
 * Moves Cursor To First Column In Line
 */
CarriageReturn(screen)
register TScreen *screen;
{
	screen->cur_col = 0;
	screen->do_wrap = 0;
}

/*
 * Save Cursor and Attributes
 */
CursorSave(term, sc)
register XtermWidget term;
register SavedCursor *sc;
{
	register TScreen *screen = &term->screen;

	sc->row = screen->cur_row;
	sc->col = screen->cur_col;
	sc->flags = term->flags;
	sc->curgl = screen->curgl;
	sc->curgr = screen->curgr;
	bcopy(screen->gsets, sc->gsets, sizeof(screen->gsets));
}

/*
 * Restore Cursor and Attributes
 */
CursorRestore(term, sc)
register XtermWidget term;
register SavedCursor *sc;
{
	register TScreen *screen = &term->screen;

	bcopy(sc->gsets, screen->gsets, sizeof(screen->gsets));
	screen->curgl = sc->curgl;
	screen->curgr = sc->curgr;
	term->flags &= ~(BOLD|INVERSE|UNDERLINE);
	term->flags |= sc->flags & (BOLD|INVERSE|UNDERLINE);
	CursorSet(screen, sc->row, sc->col, term->flags);
}
