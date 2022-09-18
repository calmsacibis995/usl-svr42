/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:scrollbar.c	1.2.1.28"
#endif
/*
 scrollbar.c (C source file)
	Acc: 601052400 Tue Jan 17 10:00:00 1989
	Mod: 601054126 Tue Jan 17 10:28:46 1989
	Sta: 601054126 Tue Jan 17 10:28:46 1989
	Owner: 7007
	Group: 1985
	Permissions: 644
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/

/* Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts. */

#include <X11/copyright.h>

#include <stdio.h>
#include <setjmp.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/StringDefs.h>		/* ehr3 - From Atoms.h */
#include "ptyx.h"
#include "data.h"
#include <Xol/OpenLook.h>
#include <Xol/Scrollbar.h> 
#include "error.h"

extern void bcopy();

#ifndef lint
static char rcs_id[] = "$Header: scrollbar.c,v 1.3 88/02/20 15:31:26 swick Exp $";
#endif	/* lint */

/* Event handlers */

static void ScrollText();

Bool IsEventType( display, event, type )
	Display *display;
	XEvent *event;
	int type;
{
	return (event->type == type);
}


Widget 
CreateScrollBar OLARGLIST((xw))
	OLGRA(XtermWidget, xw)
{
	Widget scrollWidget;
	TScreen *screen = &xw->screen;
	Arg argList[10];
	register int i=0;

/* FLH dynamic 
 *
 *		The scrollbar will be made a child of the container widget.
 *		The container will take care of placing the scrollbar to the
 *		Right of the xterm text window, with the appropriate height
 *
FLH dynamic */

	XtSetArg(argList[i], XtNorientation,	(XtArgVal) OL_VERTICAL); i++;
	XtSetArg(argList[i], XtNborderWidth,	(XtArgVal) 0); i++;
	XtSetArg(argList[i], XtNgranularity,	(XtArgVal) 1); i++;

	XtSetArg(argList[i], XtNsliderMax, (XtArgVal)
			     (screen->savelines + screen->max_row + 1)); i++;
	XtSetArg(argList[i], XtNsliderMin,	(XtArgVal) 0); i++;
	XtSetArg(argList[i], XtNproportionLength,
			     (XtArgVal) (screen->max_row + 1)); i++;
	XtSetArg(argList[i], XtNtraversalOn, (XtArgVal) False); i++;

/* 
 *		Reverse video applies to text only, not the scrollbar.
 *		The scrollbar should pick up its foreground and background
 *		from its parent shell (see below)
 */

/* FLH dynamic */
	/*
	 *	make scrollbar a child of container so it will 
	 * 1) pick up dynamic resource changes
	 * 2) appear outside of the text window pane
	 */
			/* set refName for scrollbar so container knows where
			 * to place scrollbar
	 		 */
	XtSetArg(argList[i], XtNrefName, ((Widget) footerpane)->core.name); i++;
	   /*
		 *	make scrollbar width unaffected by window resizing
		 */
	XtSetArg(argList[i], XtNweight, 0); i++;
	scrollWidget = XtCreateManagedWidget("scrollbar", scrollbarWidgetClass, 
					     (Widget) container, argList, i);
	XtAddCallback (scrollWidget, XtNsliderMoved, ScrollText, 0);
/* FLH dynamic */

/* FLH mouseless */
		/* associate scrollbar with xterm widget to provide
		 * mouseless scrolling
		 * NOTE: For now, the scrollbar is activated directly from misc.c.
		 * OlAssociateWidget is the proper way to have the scrollbar 
		 * activated, though.
		 *
		 * OlAssociateWidget((Widget) xw,scrollWidget,TRUE);
		 */
/* FLH mouseless-end */
	return(scrollWidget);
}

#ifdef REVERSE_WIDGETS	/* widgets currently don't support reverse video */

ScrollBarReverseVideo(scrollWidget)
	register Widget scrollWidget;
{
	register TScreen *screen = &term->screen;
	Arg argList[2];

	XtSetArg(argList[0], XtNforeground,  (XtArgVal) screen->foreground);
	XtSetArg(argList[1], XtNbackground,  (XtArgVal) screen->background);
	XtSetValues(scrollWidget, argList, XtNumber(argList));
}

#endif /* REVERSE_WIDGETS */


ScrollBarDrawThumb(scrollWidget)
	register Widget scrollWidget;
{
	register TScreen *screen = &term->screen;
	Arg args[1];

	if (screen->savelines > 0)
	{
	    XtSetArg(args[0], XtNsliderValue,
			     (XtArgVal) (screen->savedlines + screen->topline));
	    XtSetValues (scrollWidget, args, 1);
	}
}


WindowScroll(screen, top)
	register TScreen *screen;
	int top;
{
	register int i, lines;
	register int scrolltop, scrollheight, refreshtop;
	register int x = 0;

	if (top < -screen->savedlines)
		top = -screen->savedlines;
	else if (top > 0)
		top = 0;
	if((i = screen->topline - top) == 0) {
		ScrollBarDrawThumb(screen->scrollWidget);
		return;
	}

	ScrollSelection(i);

	if(screen->cursor_state)
		HideCursor();
	lines = i > 0 ? i : -i;
	if(lines > screen->max_row + 1)
		lines = screen->max_row + 1;
	scrollheight = screen->max_row - lines + 1;
	if(i > 0)
		refreshtop = scrolltop = 0;
	else {
		scrolltop = lines;
		refreshtop = scrollheight;
	}
/* SS-scrollbar */
	x = screen->border;
/* SS-scrollbar-end */
	/* x = screen->scrollbar +	screen->border; */
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
		XCopyArea(
		    screen->display, 
		    TextWindow(screen), TextWindow(screen),
		    screen->normalGC[0],
		    (int) x,
		    (int) scrolltop * FontHeight(screen) + screen->border, 
		    (unsigned) Width(screen),
		    (unsigned) scrollheight * FontHeight(screen),
		    (int) x,
		    (int) (scrolltop + i) * FontHeight(screen)
			+ screen->border);
	}
	screen->topline = top;
	XClearArea(
	    screen->display,
	    TextWindow(screen), 
	    (int) x,
	    (int) refreshtop * FontHeight(screen) + screen->border, 
	    (unsigned) Width(screen),
	    (unsigned) lines * FontHeight(screen),
	    FALSE);
/* SS-color : this used to be ScrnRefresh */
	ColorScrnRefresh(screen, refreshtop, 0, lines, screen->max_col + 1);
/* SS-color-end */

	ScrollBarDrawThumb(screen->scrollWidget);
}

ScrollBarOn(screen)
	register TScreen *screen;
{
	register int border = 2 * screen->border;
	register int i;
#ifndef MEMUTIL
	char *realloc(), *calloc();
#endif
/* SS-color */
	register int rows = screen->max_row + 1;
	register int savelines = screen->savelines;
	register int cols = screen->max_col + 1;
/* SS-color-end */

/* SS-color : replaces screen->.. by the local registers declared above */
	if(screen->scrollbar)
		return;

	/* in creating scrollbar, leave 1 pixel between the scrollbar */
	/* and window manager border.				      */

	if(!screen->scrollWidget) {
/* FLH dynamic */
					/*
					 *		Make scrollbar a child of the container widget
					 *		so it is outside of text window
					 */
		if((screen->scrollWidget = CreateScrollBar(term)) == NULL) {
			XtRealizeWidget(screen->scrollWidget);
			Bell();
			return;
		}
/* FLH dynamic */
		if (screen->allbuf) {
		    if((screen->allbuf = (ScrnBuf) realloc((char *) screen->buf,
			(unsigned) 4*(rows + 1 + savelines) * sizeof(char *)))
		            == NULL)
			       Error (ERROR_SBRALLOC);
		    screen->buf = &screen->allbuf[4 * savelines];
		    bcopy ((char *)screen->allbuf, (char *)screen->buf,
			   4 * (rows + 1) * sizeof (char *));
		    for(i = 4 * savelines - 1 ; i >= 0 ; i--)
			if((screen->allbuf[i] = (Char *)
			    calloc((unsigned) cols, sizeof(Char))) == NULL)
				Error (ERROR_SBRALLOC2);
		}
/* SS-color-end */
	}
	screen->scrollbar = screen->scrollWidget->core.width;
	ScrollBarDrawThumb(screen->scrollWidget);
	{
	     XSizeHints sizehints;
	     XGetNormalHints(XtDisplay((Widget) term), 
								XtWindow((Widget) container), 
								&sizehints);
	}
	/* manage afterwards so BitGravity can be used profitably */
	XtManageChild(screen->scrollWidget);
}

ScrollBarOff(screen)
	register TScreen *screen;
{
	register int border = 2 * screen->border;

	if(!screen->scrollbar)
		return;
	screen->scrollbar = 0;
	XtUnmanageChild(screen->scrollWidget);
}

/*ARGSUSED*/
static void ScrollText(scrollbarWidget, closure, call_data)
	Widget scrollbarWidget;
	XtPointer closure;
	OlScrollbarVerify *call_data;
{
	register TScreen *screen = &term->screen;
	register int     savedlines = screen->savedlines;
	register int newTopLine;
/* SS-copy */
	extern Boolean	Have_hilite;
	extern int      TrackText();

	if (Have_hilite) {
	    TrackText (0, 0, 0, 0);
	    Have_hilite = FALSE;
	}
/* SS-copy-end */
 
	if (call_data->new_location > savedlines)
        {
            newTopLine = 0;
            call_data->new_location = savedlines;
        }
        else
            newTopLine = call_data->new_location - savedlines;
	WindowScroll(screen, newTopLine);
}
