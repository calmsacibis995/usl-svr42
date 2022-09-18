/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:util.c	1.2.1.22"
#endif
/*
 util.c (C source file)
	Acc: 601052456 Tue Jan 17 10:00:56 1989
	Mod: 601054171 Tue Jan 17 10:29:31 1989
	Sta: 601054171 Tue Jan 17 10:29:31 1989
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

#include <X11/copyright.h>

#ifndef lint
static char rcs_id[] = "$Header: util.c,v 1.4 88/02/18 17:54:48 jim Exp $";
#endif	/* lint */

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <signal.h>
#include <setjmp.h>
typedef int *jmp_ptr;

#include "ptyx.h"
#include "data.h"
#include "error.h"

/* SS-color */
#include "xterm.h"
/* SS-color-end */

/*
 * These routines are used for the jump scroll feature
 */
FlushScroll(screen)
register TScreen *screen;
{
	register int i;
	register int shift = -screen->topline;
	register int bot = screen->max_row - shift;
	register int refreshtop;
	register int refreshheight;
	register int scrolltop;
	register int scrollheight;

	if(screen->cursor_state)
		HideCursor();
	if(screen->scroll_amt > 0) {
		refreshheight = screen->refresh_amt;
		scrollheight = screen->bot_marg - screen->top_marg -
		 refreshheight + 1;
		if((refreshtop = screen->bot_marg - refreshheight + 1 + shift) >
		 (i = screen->max_row - screen->scroll_amt + 1))
			refreshtop = i;
		if(screen->scrollWidget && !screen->alternate
		 && screen->top_marg == 0) {
			scrolltop = 0;
			if((scrollheight += shift) > i)
				scrollheight = i;
			if((i = screen->bot_marg - bot) > 0 &&
			 (refreshheight -= i) < screen->scroll_amt)
				refreshheight = screen->scroll_amt;
			if((i = screen->savedlines) < screen->savelines) {
				if((i += screen->scroll_amt) >
				  screen->savelines)
					i = screen->savelines;
				screen->savedlines = i;
				ScrollBarDrawThumb(screen->scrollWidget);
			}
		} else {
			scrolltop = screen->top_marg + shift;
			if((i = bot - (screen->bot_marg - screen->refresh_amt +
			 screen->scroll_amt)) > 0) {
				if(bot < screen->bot_marg)
					refreshheight = screen->scroll_amt + i;
			} else {
				scrollheight += i;
				refreshheight = screen->scroll_amt;
				if((i = screen->top_marg + screen->scroll_amt -
				 1 - bot) > 0) {
					refreshtop += i;
					refreshheight -= i;
				}
			}
		}
	} else {
		refreshheight = -screen->refresh_amt;
		scrollheight = screen->bot_marg - screen->top_marg -
		 refreshheight + 1;
		refreshtop = screen->top_marg + shift;
		scrolltop = refreshtop + refreshheight;
		if((i = screen->bot_marg - bot) > 0)
			scrollheight -= i;
		if((i = screen->top_marg + refreshheight - 1 - bot) > 0)
			refreshheight -= i;
	}
	if(scrollheight > 0) {
		if (screen->multiscroll && scrollheight == 1 &&
		 screen->topline == 0 && screen->top_marg == 0 &&
		 screen->bot_marg == screen->max_row) {
			if (screen->incopy < 0 && screen->scrolls == 0)
				CopyWait (screen);
			screen->scrolls++;
		} else {
			if (screen->incopy)
				CopyWait (screen);
			screen->incopy = -1;
		}

		/* here, and in all other places, we can use any GC	  */
		/* since the font information is not used by the fucntion */

		XCopyArea (
		    screen->display, 
		    TextWindow(screen),
		    TextWindow(screen),
		    screen->normalGC[0],
/* SS-scrollbar */
		    /* (int) screen->border + screen->scrollbar, */
		    (int) screen->border,
/* SS-scrollbar-end */
		    (int) (scrolltop + screen->scroll_amt) * FontHeight(screen)
			+ screen->border, 
		    (unsigned) Width(screen),
		    (unsigned) scrollheight * FontHeight(screen),
/* SS-scrollbar */
		    /* (int) screen->border + screen->scrollbar, */
		    (int) screen->border,
/* SS-scrollbar-end */
		    (int) scrolltop*FontHeight(screen) + screen->border);
	}
	screen->scroll_amt = 0;
	screen->refresh_amt = 0;
	if(refreshheight > 0) {
		XClearArea (
		    screen->display,
		    TextWindow(screen),
/* SS-scrollbar */
		    /* (int) screen->border + screen->scrollbar, */
		    (int) screen->border,
/* SS-scrollbar-end */
		    (int) refreshtop * FontHeight(screen) + screen->border,
		    (unsigned) Width(screen),
		    (unsigned) refreshheight * FontHeight(screen),
		    FALSE);
/* SS-color : added last argument */
		ScrnRefresh(screen, refreshtop, 0, refreshheight,
				     screen->max_col + 1, FALSE);
/* SS-color-end */
	}
}

AddToRefresh(screen)
register TScreen *screen;
{
	register int amount = screen->refresh_amt;
	register int row = screen->cur_row;

	if(amount == 0)
		return(0);
	if(amount > 0) {
		register int bottom;

		if(row == (bottom = screen->bot_marg) - amount) {
			screen->refresh_amt++;
			return(1);
		}
		return(row >= bottom - amount + 1 && row <= bottom);
	} else {
		register int top;

		amount = -amount;
		if(row == (top = screen->top_marg) + amount) {
			screen->refresh_amt--;
			return(1);
		}
		return(row <= top + amount - 1 && row >= top);
	}
}

/* 
 * scrolls the screen by amount lines, erases bottom, doesn't alter 
 * cursor position (i.e. cursor moves down amount relative to text).
 * All done within the scrolling region, of course. 
 * requires: amount > 0
 */
Scroll(screen, amount)
register TScreen *screen;
register int amount;
{
	register int i = screen->bot_marg - screen->top_marg + 1;
	register int shift;
	register int bot;
	register int refreshtop;
	register int refreshheight;
	register int scrolltop;
	register int scrollheight;
/* SS-copy */
	extern Boolean	Have_hilite;
	extern int      TrackText();

	if (Have_hilite) {
	    TrackText (0, 0, 0, 0);
	    Have_hilite = FALSE;
	}
/* SS-copy-end */
 
	if(screen->cursor_state)
		HideCursor();
	if (amount > i)
		amount = i;
    if(screen->jumpscroll) {
	if(screen->scroll_amt > 0) {
		if(screen->refresh_amt + amount > i)
			FlushScroll(screen);
		screen->scroll_amt += amount;
		screen->refresh_amt += amount;
	} else {
		if(screen->scroll_amt < 0)
			FlushScroll(screen);
		screen->scroll_amt = amount;
		screen->refresh_amt = amount;
	}
	refreshheight = 0;
    } else {

	if (amount == i) {
		ClearScreen(screen);
		return;
	}
	shift = -screen->topline;
	bot = screen->max_row - shift;
	scrollheight = i - amount;
	refreshheight = amount;
	if((refreshtop = screen->bot_marg - refreshheight + 1 + shift) >
	 (i = screen->max_row - refreshheight + 1))
		refreshtop = i;
	if(screen->scrollWidget && !screen->alternate
	 && screen->top_marg == 0) {
		scrolltop = 0;
		if((scrollheight += shift) > i)
			scrollheight = i;
		if((i = screen->savedlines) < screen->savelines) {
			if((i += amount) > screen->savelines)
				i = screen->savelines;
			screen->savedlines = i;
			ScrollBarDrawThumb(screen->scrollWidget);
		}
	} else {
		scrolltop = screen->top_marg + shift;
		if((i = screen->bot_marg - bot) > 0) {
			scrollheight -= i;
			if((i = screen->top_marg + amount - 1 - bot) >= 0) {
				refreshtop += i;
				refreshheight -= i;
			}
		}
	}
	if(scrollheight > 0) {
		if (screen->multiscroll
		&& amount==1 && screen->topline == 0
		&& screen->top_marg==0
		&& screen->bot_marg==screen->max_row) {
			if (screen->incopy<0 && screen->scrolls==0)
				CopyWait(screen);
			screen->scrolls++;
		} else {
			if (screen->incopy)
				CopyWait(screen);
			screen->incopy = -1;
		}

		XCopyArea(
		    screen->display, 
		    TextWindow(screen),
		    TextWindow(screen),
		    screen->normalGC[0],
/* SS-scrollbar */
		    /* (int) screen->border + screen->scrollbar, */
		    (int) screen->border,
/* SS-scrollbar-end */
		    (int) (scrolltop+amount) * FontHeight(screen) + screen->border, 
		    (unsigned) Width(screen),
		    (unsigned) scrollheight * FontHeight(screen),
/* SS-scrollbar */
		    /* (int) screen->border + screen->scrollbar, */
		    (int) screen->border,
/* SS-scrollbar-end */
		    (int) scrolltop * FontHeight(screen) + screen->border);
	}
	if(refreshheight > 0) {
		XClearArea (
		   screen->display,
		   TextWindow(screen),
/* SS-scrollbar */
		    /* (int) screen->border + screen->scrollbar, */
		    (int) screen->border,
/* SS-scrollbar-end */
		   (int) refreshtop * FontHeight(screen) + screen->border,
		   (unsigned) Width(screen),
		   (unsigned) refreshheight * FontHeight(screen),
		   FALSE);
		if(refreshheight > shift)
			refreshheight = shift;
	}
    }
	if(screen->scrollWidget && !screen->alternate && screen->top_marg == 0)
/* SS-color : changed the first argument to screen and added 6th argument  */
/*	      indicating whether to use allbuf ot buf.			   */
/* SS-cut:    added the last argument indicating that we are NOT deleting  */
/* 	      lines (we are scrolling)					   */
		ScrnDeleteLine(screen, screen->bot_marg + screen->savelines,
			       0, amount, screen->max_col + 1, TRUE, FALSE);
	else
		ScrnDeleteLine(screen, screen->bot_marg, screen->top_marg,
		 	       amount, screen->max_col + 1, FALSE, FALSE);
	if(refreshheight > 0)
	   ScrnRefresh(screen, refreshtop, 0, refreshheight,
		 			screen->max_col + 1, FALSE);
/* SS-color-end */
}


/*
 * Reverse scrolls the screen by amount lines, erases top, doesn't alter
 * cursor position (i.e. cursor moves up amount relative to text).
 * All done within the scrolling region, of course.
 * Requires: amount > 0
 */
RevScroll(screen, amount)
register TScreen *screen;
register int amount;
{
	register int i = screen->bot_marg - screen->top_marg + 1;
	register int shift;
	register int bot;
	register int refreshtop;
	register int refreshheight;
	register int scrolltop;
	register int scrollheight;
/* SS-copy */
	extern Boolean	Have_hilite;
	extern int      TrackText();

	if (Have_hilite) {
	    TrackText (0, 0, 0, 0);
	    Have_hilite = FALSE;
	}
/* SS-copy-end */
 
	if(screen->cursor_state)
		HideCursor();
	if (amount > i)
		amount = i;
    if(screen->jumpscroll) {
	if(screen->scroll_amt < 0) {
		if(-screen->refresh_amt + amount > i)
			FlushScroll(screen);
		screen->scroll_amt -= amount;
		screen->refresh_amt -= amount;
	} else {
		if(screen->scroll_amt > 0)
			FlushScroll(screen);
		screen->scroll_amt = -amount;
		screen->refresh_amt = -amount;
	}
    } else {
	shift = -screen->topline;
	bot = screen->max_row - shift;
	refreshheight = amount;
	scrollheight = screen->bot_marg - screen->top_marg -
	 refreshheight + 1;
	refreshtop = screen->top_marg + shift;
	scrolltop = refreshtop + refreshheight;
	if((i = screen->bot_marg - bot) > 0)
		scrollheight -= i;
	if((i = screen->top_marg + refreshheight - 1 - bot) > 0)
		refreshheight -= i;
	if(scrollheight > 0) {
		if (screen->multiscroll
		&& amount==1 && screen->topline == 0
		&& screen->top_marg==0
		&& screen->bot_marg==screen->max_row) {
			if (screen->incopy<0 && screen->scrolls==0)
				CopyWait(screen);
			screen->scrolls++;
		} else {
			if (screen->incopy)
				CopyWait(screen);
			screen->incopy = -1;
		}

		XCopyArea (
		    screen->display,
		    TextWindow(screen),
		    TextWindow(screen),
		    screen->normalGC[0],
/* SS-scrollbar */
		    /* (int) screen->border + screen->scrollbar, */
		    (int) screen->border,
/* SS-scrollbar-end */
		    (int) (scrolltop-amount) * FontHeight(screen) + screen->border, 
		    (unsigned) Width(screen),
		    (unsigned) scrollheight * FontHeight(screen),
/* SS-scrollbar */
		    /* (int) screen->border + screen->scrollbar, */
		    (int) screen->border,
/* SS-scrollbar-end */
		    (int) scrolltop * FontHeight(screen) + screen->border);
	}
	if(refreshheight > 0)
		XClearArea (
		    screen->display,
		    TextWindow(screen),
/* SS-scrollbar */
		    /* (int) screen->border + screen->scrollbar, */
		    (int) screen->border,
/* SS-scrollbar-end */
		    (int) refreshtop * FontHeight(screen) + screen->border,
		    (unsigned) Width(screen),
		    (unsigned) refreshheight * FontHeight(screen),
		    FALSE);
    }
/* SS-color : changed the first argument to screen */
	ScrnInsertLine (screen, screen->bot_marg, screen->top_marg,
			amount, screen->max_col + 1);
/* SS-color-end */
}

/*
 * If cursor not in scrolling region, returns.  Else,
 * inserts n blank lines at the cursor's position.  Lines above the
 * bottom margin are lost.
 */
InsertLine (screen, n)
register TScreen *screen;
register int n;
{
	register int i;
	register int shift;
	register int bot;
	register int refreshtop;
	register int refreshheight;
	register int scrolltop;
	register int scrollheight;

	if (screen->cur_row < screen->top_marg ||
	 screen->cur_row > screen->bot_marg)
		return;
	if(screen->cursor_state)
		HideCursor();
	screen->do_wrap = 0;
	if (n > (i = screen->bot_marg - screen->cur_row + 1))
		n = i;
    if(screen->jumpscroll) {
	if(screen->scroll_amt <= 0 &&
	 screen->cur_row <= -screen->refresh_amt) {
		if(-screen->refresh_amt + n > screen->max_row + 1)
			FlushScroll(screen);
		screen->scroll_amt -= n;
		screen->refresh_amt -= n;
	} else if(screen->scroll_amt)
		FlushScroll(screen);
    }
    if(!screen->scroll_amt) {
	shift = -screen->topline;
	bot = screen->max_row - shift;
	refreshheight = n;
	scrollheight = screen->bot_marg - screen->cur_row - refreshheight + 1;
	refreshtop = screen->cur_row + shift;
	scrolltop = refreshtop + refreshheight;
	if((i = screen->bot_marg - bot) > 0)
		scrollheight -= i;
	if((i = screen->cur_row + refreshheight - 1 - bot) > 0)
		refreshheight -= i;
	if(scrollheight > 0) {
		if (screen->incopy)
			CopyWait (screen);
		screen->incopy = -1;
		XCopyArea (
		    screen->display, 
		    TextWindow(screen),
		    TextWindow(screen),
		    screen->normalGC[0],
/* SS-scrollbar */
		    /* (int) screen->border + screen->scrollbar, */
		    (int) screen->border,
/* SS-scrollbar-end */
		    (int) (scrolltop - n) * FontHeight(screen) + screen->border, 
		    (unsigned) Width(screen),
		    (unsigned) scrollheight * FontHeight(screen),
/* SS-scrollbar */
		    /* (int) screen->border + screen->scrollbar, */
		    (int) screen->border,
/* SS-scrollbar-end */
		    (int) scrolltop * FontHeight(screen) + screen->border);
	}
	if(refreshheight > 0)
		XClearArea (
		    screen->display,
		    TextWindow(screen),
/* SS-scrollbar */
		    /* (int) screen->border + screen->scrollbar, */
		    (int) screen->border,
/* SS-scrollbar-end */
		    (int) refreshtop * FontHeight(screen) + screen->border,
		    (unsigned) Width(screen),
		    (unsigned) refreshheight * FontHeight(screen),
		    FALSE);
    }
	/* adjust screen->buf */
/* SS-color : changed the first argument to screen */
/*
	if (screen->cur_row + (2 * (n+1)) < screen->bot_marg)
		ScrnInsertLine(screen, screen->bot_marg, screen->cur_row, n,
			screen->max_col + 1);
	else 
*/
	{
		int index;

		for (index = n; index; index--)
			ScrnInsertLine(screen, screen->bot_marg, screen->cur_row, 1,
				screen->max_col + 1);
	}
/* SS-color-end */
}

/*
 * If cursor not in scrolling region, returns.  Else, deletes n lines
 * at the cursor's position, lines added at bottom margin are blank.
 */
DeleteLine(screen, n)
register TScreen *screen;
register int n;
{
	register int i;
	register int shift;
	register int bot;
	register int refreshtop;
	register int refreshheight;
	register int scrolltop;
	register int scrollheight;

	if (screen->cur_row < screen->top_marg ||
	 screen->cur_row > screen->bot_marg)
		return;
	if(screen->cursor_state)
		HideCursor();
	screen->do_wrap = 0;
	if (n > (i = screen->bot_marg - screen->cur_row + 1))
		n = i;
    if(screen->jumpscroll) {
	if(screen->scroll_amt >= 0 && screen->cur_row == screen->top_marg) {
		if(screen->refresh_amt + n > screen->max_row + 1)
			FlushScroll(screen);
		screen->scroll_amt += n;
		screen->refresh_amt += n;
	} else if(screen->scroll_amt)
		FlushScroll(screen);
    }
    if(!screen->scroll_amt) {

	shift = -screen->topline;
	bot = screen->max_row - shift;
	scrollheight = i - n;
	refreshheight = n;
	if((refreshtop = screen->bot_marg - refreshheight + 1 + shift) >
	 (i = screen->max_row - refreshheight + 1))
		refreshtop = i;
	if(screen->scrollWidget && !screen->alternate && screen->cur_row == 0) {
		scrolltop = 0;
		if((scrollheight += shift) > i)
			scrollheight = i;
		if((i = screen->savedlines) < screen->savelines) {
			if((i += n) > screen->savelines)
				i = screen->savelines;
			screen->savedlines = i;
			ScrollBarDrawThumb(screen->scrollWidget);
		}
	} else {
		scrolltop = screen->cur_row + shift;
		if((i = screen->bot_marg - bot) > 0) {
			scrollheight -= i;
			if((i = screen->cur_row + n - 1 - bot) >= 0) {
				refreshheight -= i;
			}
		}
	}
	if(scrollheight > 0) {
		if (screen->incopy)
			CopyWait(screen);
		screen->incopy = -1;

		XCopyArea (
		    screen->display,
		    TextWindow(screen),
		    TextWindow(screen),
		    screen->normalGC[0],
/* SS-scrollbar */
		    /* (int) screen->border + screen->scrollbar, */
		    (int) screen->border,
/* SS-scrollbar-end */
		    (int) (scrolltop + n) * FontHeight(screen) + screen->border, 
		    (unsigned) Width(screen),
		    (unsigned) scrollheight * FontHeight(screen),
/* SS-scrollbar */
		    /* (int) screen->border + screen->scrollbar, */
		    (int) screen->border,
/* SS-scrollbar-end */
		    (int) scrolltop * FontHeight(screen) + screen->border);
	}
	if(refreshheight > 0)
		XClearArea (
		    screen->display,
		    TextWindow(screen),
/* SS-scrollbar */
		    /* (int) screen->border + screen->scrollbar, */
		    (int) screen->border,
/* SS-scrollbar-end */
		    (int) refreshtop * FontHeight(screen) + screen->border,
		    (unsigned) Width(screen),
		    (unsigned) refreshheight * FontHeight(screen),
		    FALSE);
    }
	/* adjust screen->buf */
	if(screen->scrollWidget && !screen->alternate && screen->cur_row == 0)
/* SS-color : changed the first argument to screen and added 6th argument  */
/*	      indicating whether to use allbuf ot buf.			   */
/* SS-cut:    added the last argument indicating that we are deleting line */
/*	      (as oppsed to scrolling)					   */
  	     
		ScrnDeleteLine(screen, screen->bot_marg + screen->savelines,
			       0, n, screen->max_col + 1, TRUE, TRUE);
	else
		ScrnDeleteLine(screen, screen->bot_marg, screen->cur_row,
		 	       n, screen->max_col + 1, FALSE, TRUE);
/* SS-color-end */
}

/*
 * Insert n blanks at the cursor's position, no wraparound
 */
InsertChar (screen, n)
register TScreen *screen;
register int n;
{
	register int width = n * FontWidth(screen), cx, cy;

	if(screen->cursor_state)
		HideCursor();
	screen->do_wrap = 0;
	if(screen->cur_row - screen->topline <= screen->max_row) {
	    if(!AddToRefresh(screen)) {
		if(screen->scroll_amt)
			FlushScroll(screen);
	
		if (screen->incopy)
			CopyWait (screen);
		screen->incopy = -1;
	
		cx = CursorX (screen, screen->cur_col);
		cy = CursorY (screen, screen->cur_row);
		XCopyArea(
		    screen->display,
		    TextWindow(screen), TextWindow(screen),
		    screen->normalGC[0],
		    cx, cy,
		    (unsigned) Width(screen)
		        - (screen->cur_col + n) * FontWidth(screen),
		    (unsigned) FontHeight(screen), 
		    cx + width, cy);
		XFillRectangle(
		    screen->display,
		    TextWindow(screen), 
		    screen->reverseGC[0],
		    cx, cy,
		    (unsigned) width, (unsigned) FontHeight(screen));
	    }
	}
	/* adjust screen->buf */
/* SS-color : changed the first argument to screen */
	ScrnInsertChar(screen, screen->cur_row, screen->cur_col, n,
			screen->max_col + 1);
/* SS-color-end */
}

/*
 * Deletes n chars at the cursor's position, no wraparound.
 */
DeleteChar (screen, n)
register TScreen *screen;
register int	n;
{
	register int width, cx, cy;

	if(screen->cursor_state)
		HideCursor();
	screen->do_wrap = 0;
	if (n > (width = screen->max_col + 1 - screen->cur_col))
	  	n = width;
		
	if(screen->cur_row - screen->topline <= screen->max_row) {
	    if(!AddToRefresh(screen)) {
		if(screen->scroll_amt)
			FlushScroll(screen);
	
		width = n * FontWidth(screen);
	
		if (screen->incopy)
			CopyWait (screen);
		screen->incopy = -1;
	
		cx = CursorX (screen, screen->cur_col);
		cy = CursorY (screen, screen->cur_row);
		XCopyArea(screen->display,
		     TextWindow(screen), TextWindow(screen),
		     screen->normalGC[0], 
		     cx + width, cy,
		     Width(screen) - (screen->cur_col + n) * FontWidth(screen),
		     FontHeight(screen), 
		     cx, cy);
		XFillRectangle (screen->display, TextWindow(screen),
		     screen->reverseGC[0],
/* SS-scrollbar-end */
		     /* screen->border + screen->scrollbar + Width(screen) - width, */
		     screen->border + Width(screen) - width,
/* SS-scrollbar-end */
		     cy, width, FontHeight(screen));
	    }
	}
	/* adjust screen->buf */
/* SS-color : changed first argument to screen */
	ScrnDeleteChar (screen, screen->cur_row, screen->cur_col, n,
			screen->max_col + 1);
/* SS-color-end */

}

/*
 * Clear from cursor position to beginning of display, inclusive.
 */
ClearAbove (screen)
register TScreen *screen;
{
	register top, height;

	if(screen->cursor_state)
		HideCursor();
	if((top = -screen->topline) <= screen->max_row) {
		if(screen->scroll_amt)
			FlushScroll(screen);
		if((height = screen->cur_row + top) > screen->max_row)
			height = screen->max_row;
		if((height -= top) > 0)
			XClearArea(screen->display, TextWindow(screen),
/* SS-scrollbar */
		    /* (int) screen->border + screen->scrollbar, */
		    (int) screen->border,
/* SS-scrollbar-end */
			 top * FontHeight(screen) + screen->border,
			 Width(screen), height * FontHeight(screen), FALSE);

		if(screen->cur_row - screen->topline <= screen->max_row)
			ClearLeft(screen);
	}
	ClearBufRows(screen, 0, screen->cur_row - 1);
}

/*
 * Clear from cursor position to end of display, inclusive.
 */
ClearBelow (screen)
register TScreen *screen;
{
	register top;

	ClearRight(screen);
	if((top = screen->cur_row - screen->topline) <= screen->max_row) {
		if(screen->scroll_amt)
			FlushScroll(screen);
		if(++top <= screen->max_row)
			XClearArea(screen->display, TextWindow(screen),
/* SS-scrollbar */
		    /* (int) screen->border + screen->scrollbar, */
		    (int) screen->border,
/* SS-scrollbar-end */
			 top * FontHeight(screen) + screen->border,
			 Width(screen), (screen->max_row - top + 1) *
			 FontHeight(screen), FALSE);
	}
	ClearBufRows(screen, screen->cur_row + 1, screen->max_row);
}

/* 
 * Clear last part of cursor's line, inclusive.
 */
ClearRight (screen)
register TScreen *screen;
{
	int i, j;

	if(screen->cursor_state)
		HideCursor();
	screen->do_wrap = 0;
	if(screen->cur_row - screen->topline <= screen->max_row) {
	    if(!AddToRefresh(screen)) {
	if(screen->scroll_amt)
		FlushScroll(screen);
		XFillRectangle(screen->display, TextWindow(screen),
		  screen->reverseGC[0],
		 CursorX(screen, screen->cur_col),
		 CursorY(screen, screen->cur_row),
		 Width(screen) - screen->cur_col * FontWidth(screen),
		 FontHeight(screen));
	    }
	}
	bzero(screen->buf [i = 4 * screen->cur_row] + screen->cur_col,
	       (j = screen->max_col - screen->cur_col + 1));
	bzero(screen->buf [i + 1] + screen->cur_col, j);
	bzero(screen->buf [i + 3] + screen->cur_col, j);
}

/*
 * Clear first part of cursor's line, inclusive.
 */
ClearLeft (screen)
register TScreen *screen;
{
	int i, j;

	if(screen->cursor_state)
		HideCursor();
	screen->do_wrap = 0;
	if(screen->cur_row - screen->topline <= screen->max_row) {
	    if(!AddToRefresh(screen)) {
		if(screen->scroll_amt)
			FlushScroll(screen);
		XFillRectangle (screen->display, TextWindow(screen),
		     screen->reverseGC[0],
/* SS-scrollbar */
		     /* screen->border + screen->scrollbar, */
		     screen->border,
/* SS-scrollbar-end */
		      CursorY (screen, screen->cur_row),
		     (screen->cur_col + 1) * FontWidth(screen),
		     FontHeight(screen));
	    }
	}
	bzero (screen->buf [i = 4 * screen->cur_row], (j=screen->cur_col + 1));
	bzero (screen->buf [i + 1], j);
	bzero (screen->buf [i + 3], j);
}

/* 
 * Erase the cursor's line.
 */
ClearLine(screen)
register TScreen *screen;
{
	int i, j;

	if(screen->cursor_state)
		HideCursor();
	screen->do_wrap = 0;
	if(screen->cur_row - screen->topline <= screen->max_row) {
	    if(!AddToRefresh(screen)) {
		if(screen->scroll_amt)
			FlushScroll(screen);
		XFillRectangle (screen->display, TextWindow(screen), 
		     screen->reverseGC[0],
/* SS-scrollbar */
		     /* screen->border + screen->scrollbar, */
		     screen->border,
/* SS-scrollbar-end */
		      CursorY (screen, screen->cur_row),
		     Width(screen), FontHeight(screen));
	    }
	}
	bzero (screen->buf [i = 4 * screen->cur_row], (j=screen->max_col + 1));
	bzero (screen->buf [i + 1], j);
	bzero (screen->buf [i + 3], j);
}

ClearScreen(screen)
register TScreen *screen;
{
	register int top;

	if(screen->cursor_state)
		HideCursor();
	screen->do_wrap = 0;
	if((top = -screen->topline) <= screen->max_row) {
		if(screen->scroll_amt)
			FlushScroll(screen);
		if(top == 0)
			XClearWindow(screen->display, TextWindow(screen));
		else
			XClearArea(screen->display, TextWindow(screen),
/* SS-scrollbar */
		    /* (int) screen->border + screen->scrollbar, */
		    (int) screen->border,
/* SS-scrollbar-end */
			 top * FontHeight(screen) + screen->border,	
		 	 Width(screen), (screen->max_row - top + 1) *
			 FontHeight(screen), FALSE);
	}
	ClearBufRows (screen, 0, screen->max_row);
}

CopyWait(screen)
register TScreen *screen;
{
	XEvent reply;
	XEvent *rep = &reply;

	while (1) {
		XWindowEvent (screen->display, VWindow(screen), 
		  ExposureMask, &reply);
		switch (reply.type) {
		case Expose:
			HandleExposure (screen, (XExposeEvent *) &reply);
			break;
		case NoExpose:
		case GraphicsExpose:
			if (screen->incopy <= 0) {
				screen->incopy = 1;
				if (screen->scrolls > 0)
					screen->scrolls--;
			}
			if (reply.type == GraphicsExpose)
				HandleExposure (screen, (XExposeEvent *) &reply);

			if ((reply.type == NoExpose) ||
			    ((XExposeEvent *)rep)->count == 0) {
			    if (screen->incopy <= 0 && screen->scrolls > 0)
				screen->scrolls--;
			    if (screen->scrolls == 0) {
				screen->incopy = 0;
				return;
			    }
			    screen->incopy = -1;
			}
			break;
		}
	}
}
/*
 * This routine handles exposure events
 */
HandleExposure (screen, reply)
register TScreen *screen;
register XExposeEvent *reply;
{
	register int toprow, leftcol, nrows, ncols;

	if((toprow = (reply->y - screen->border) /
	 FontHeight(screen)) < 0)
		toprow = 0;
/* SS-scrollbar */
	/* if((leftcol = (reply->x - screen->border - screen->scrollbar) */
	if((leftcol = (reply->x - screen->border)
/* SS-scrollbar-end */
	 / FontWidth(screen)) < 0)
		leftcol = 0;
	nrows = (reply->y + reply->height - 1 - screen->border) / 
		FontHeight(screen) - toprow + 1;
	ncols =
/* SS-scrollbar */
 /* (reply->x + reply->width - 1 - screen->border - screen->scrollbar) / */
	 (reply->x + reply->width - 1 - screen->border) /
/* SS-scrollbar-end */
			FontWidth(screen) - leftcol + 1;
	toprow -= screen->scrolls;
	if (toprow < 0) {
		nrows += toprow;
		toprow = 0;
	}
	if (toprow + nrows - 1 > screen->max_row)
		nrows = screen->max_row - toprow + 1;
	if (leftcol + ncols - 1 > screen->max_col)
		ncols = screen->max_col - leftcol + 1;

	if (nrows > 0 && ncols > 0) {
/* SS-color */
		ScrnRefresh (screen, toprow, leftcol, nrows, ncols, FALSE);
/* SS-color-end */
		if (screen->cur_row >= toprow &&
		    screen->cur_row < toprow + nrows &&
		    screen->cur_col >= leftcol &&
		    screen->cur_col < leftcol + ncols)
			return (1);
	}
	return (0);
}

ReverseVideo (term)
	XtermWidget term;
{
	register TScreen *screen = &term->screen;
	register GC tmpGC;
	register int tmp;
#ifdef TEK
	register Window tek = TWindow(screen);
#endif
	extern Pixel	textFG;
	extern Pixel	textBG;

	tmp = screen->background;
	if(screen->cursorcolor == screen->foreground)
		screen->cursorcolor = tmp;
	if(screen->mousecolor == screen->foreground)
		screen->mousecolor = tmp;
/* SS-color */
	screen->background = screen->foreground;
/* SS-color-end */
	screen->foreground = tmp;

	tmpGC = screen->normalGC[0];
	screen->normalGC[0] = screen->reverseGC[0];
	screen->reverseGC[0] = tmpGC;

	tmpGC = screen->normalboldGC[0];
	screen->normalboldGC[0] = screen->reverseboldGC[0];
	screen->reverseboldGC[0] = tmpGC;

/* SS-color */
	if (!(term->flags & USE_FG_COLOR))
	    textFG = screen->foreground;
	if (!(term->flags & USE_BG_COLOR))
	    textBG = screen->background;
/* SS-color-end */

	XFreeCursor(screen->display, screen->curs);
	XFreeCursor(screen->display, screen->arrow);
	{
	    unsigned long fg, bg;
	    bg = screen->background;
	    if (screen->mousecolor == screen->background) {
		fg = screen->foreground;
	    } else {
		fg = screen->mousecolor;
	    }
	    if (XStrCmp(term->misc.curs_shape, "arrow") == 0) {
		screen->curs = make_arrow (term, fg, bg);
	    } else {
		screen->curs = make_xterm (term, fg, bg);
	    }
	    screen->arrow = make_arrow (term, fg, bg);
	}

	XDefineCursor(screen->display, TextWindow(screen), screen->curs);

#ifdef TEK
	if(tek)
                XDefineCursor(screen->display, tek, screen->arrow);
#endif

	if (term) {
/* FLH dynamic
 *
 * term is now 2 levels below the shell 
 */
	    if (term->core.border_pixel == screen->background) {
		term->core.border_pixel = screen->foreground;
		toplevel->core.border_pixel = screen->foreground;
		if (VShellWindow)
		  XSetWindowBorder (screen->display,
					 VShellWindow,
				    term->core.border_pixel);
/* FLH dynamic */
	    }
	}

	XSetWindowBackground(screen->display,
			     TextWindow(screen), screen->background);

#ifdef TEK
	if(tek)
            TekReverseVideo(screen);
#endif /* TEK */

	XClearWindow(screen->display, TextWindow(screen));
/* SS-color */
	ScrnRefresh (screen, 0, 0, screen->max_row + 1,
	 	       		   screen->max_col + 1, FALSE);
/* SS-color-end */

#ifdef TEK
	if(screen->Tshow) {
            XClearWindow(screen->display, tek);
            TekExpose((XExposeEvent *) NULL);
        }
#endif /* TEK */
}




/* SS-mouse */
int
make_myx_format (buff, y, x)
register char *buff;
register int  y, x;
{
	register int i = 0;

	if (x < 0 || y < 0)
	    buff[i++] = CTRL('^'); 	/* pointer outside the window */
	else
	{
	    /* encode X and Y coordinates in the myx format	*/

	    int x_div = x / 96;
	    int x_rem = x % 96;
	    int y_div = y / 96;
	    int y_rem = y % 96;

	    if (x_div > 0)
		buff [i++] = CTRL ('A' + x_div - 1);
	    buff [i++] = ' ' + x_rem;

	    if (y_div > 0)
		buff [i++] = CTRL ('A' + y_div - 1);
	    buff [i++] = ' ' + y_rem;
	}
	return (i);
}
/* SS-mouse-end */
