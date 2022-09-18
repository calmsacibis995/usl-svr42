/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:tabs.c	1.2.1.9"
#endif
/*
 tabs.c (C source file)
	Acc: 601052442 Tue Jan 17 10:00:42 1989
	Mod: 601054136 Tue Jan 17 10:28:56 1989
	Sta: 601054136 Tue Jan 17 10:28:56 1989
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
static char *rcsid_tabs_c = "$Header: tabs.c,v 1.1 88/02/10 13:08:17 jim Exp $";
#endif	/* lint */

#include <X11/copyright.h>

#ifndef lint
static char rcs_id[] = "$Header: tabs.c,v 1.1 88/02/10 13:08:17 jim Exp $";
#endif	/* lint */

#include <X11/Xlib.h>
#include "ptyx.h"
/*
 * This file presumes 32bits/word.  This is somewhat of a crock, and should
 * be fixed sometime.
 */

/*
 * places tabstops at only every 8 columns
 */
TabReset(tabs)
Tabs	tabs;
{
	register int i;

	for (i=0; i<TAB_ARRAY_SIZE; ++i)
		tabs[i] = 0;

	for (i=0; i<MAX_TABS; i+=8)
		TabSet(tabs, i);
}	


/*
 * places a tabstop at col
 */
TabSet(tabs, col)
Tabs	tabs;
{
	tabs[col >> 5] |= (1 << (col & 31));
}

/*
 * clears a tabstop at col
 */
TabClear(tabs, col)
Tabs	tabs;
{
	tabs[col >> 5] &= ~(1 << (col & 31));
}

/*
 * returns the column of the next tabstop
 * (or MAX_TABS - 1 if there are no more).
 * A tabstop at col is ignored.
 */
TabNext (tabs, col)
Tabs	tabs;
{
	extern XtermWidget term;
	register TScreen *screen = &term->screen;

	if(/*screen->curses &&*/screen->do_wrap && (term->flags & WRAPAROUND)) {
		Index(screen, 1);
		col = screen->cur_col = screen->do_wrap = 0;
	}
	for (++col; col<MAX_TABS; ++col)
		if (tabs[col >> 5] & (1 << (col & 31)))
			return (col);

	return (MAX_TABS - 1);
}

/*
 * clears all tabs
 */
TabZonk (tabs)
Tabs	tabs;
{
	register int i;

	for (i=0; i<TAB_ARRAY_SIZE; ++i)
		tabs[i] = 0;
}
