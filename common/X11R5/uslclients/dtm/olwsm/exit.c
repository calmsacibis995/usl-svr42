/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:olwsm/exit.c	1.18"
#endif

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>

#include <misc.h>
#include <list.h>
#include <notice.h>
#include <wsm.h>
#include "error.h"

/*
 * Local routines:
 */

static void		ExitNoticeCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));

/*
 * Convenient macros:
 */

#define EXIT_CAPTION \
	"Do you want to exit all running programs and the workspace?"

/*
 * Local data:
 */

static NoticeItem	exitNoticeItems[] = {
	{ (XtArgVal)False, (XtArgVal)ExitNoticeCB, (XtArgVal)"Yes", 'Y' },
	{ (XtArgVal)True,  (XtArgVal)NULL,         (XtArgVal)"No", 'N'  },
};

static Notice	notice  = {
	"exitQuery", EXIT_CAPTION, exitNoticeItems, 2
};

/**
 ** CreateExitNotice()
 **/

Widget
#if	OlNeedFunctionPrototypes
CreateExitNotice (
	Widget			parent
)
#else
CreateExitNotice (parent)
	Widget			parent;
#endif
{
	Cardinal n = 0;
	Display *dpy = XtDisplay(parent);
	String temp;
	static Bool firsttime = TRUE;

	if (firsttime) {
	  exitNoticeItems[0].string = (XtArgVal)OLG(yes,fixedString);
	  temp = OLG(yes,mnemonic);
	  exitNoticeItems[0].mnemonic = (XtArgVal)temp[0];
	  exitNoticeItems[1].string = (XtArgVal)OLG(no,fixedString);
	  temp = OLG(no,mnemonic);
	  exitNoticeItems[1].mnemonic = (XtArgVal)temp[0];
	  notice.string = OLG(wantExit,footerMsg);
	  firsttime = FALSE;
	}
	CreateNoticeBox (parent, &notice);
	return (notice.w);
} /* CreateExitNotice */

/**
 ** SetWSMBang()
 **/

void
#if	OlNeedFunctionPrototypes
SetWSMBang (
	Display *		display,
	Window			send_to,
	unsigned long		mask
)
#else
SetWSMBang (display, send_to, mask)
	Display *		display;
	Window			send_to;
	unsigned long		mask;
#endif
{
	XEvent			sev;

	sev.xclient.type = ClientMessage;
	sev.xclient.display = display;
	sev.xclient.window = send_to;
	sev.xclient.message_type = XA_BANG(display);
	sev.xclient.format = 8;
	XSendEvent (display, send_to, False, mask, &sev);

	return;
} /* SetWSMBang */

/**
 ** ExitNoticeCB()
 **/

static void
#if	OlNeedFunctionPrototypes
ExitNoticeCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
ExitNoticeCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	static Boolean		exit_pending = FALSE;

	Display *		display	= XtDisplayOfObject(w);

	Window			root	= RootWindowOfScreen(XtScreenOfObject(w));


	if (exit_pending) {
		/*
		 * If the user chooses 'Exit'-'Yes' a second time,
		 * the workspace manager will give up on whatever
		 * termination negotiations it's in the middle of
		 * and really exit.  This is so things don't hang
		 * forever when anomalous client conditions arise.
		 */
		WSMExit (False);
	} else {
		exit_pending = TRUE;

		/*
		 * Make all of our windows go away, so that the
		 * window manager doesn't mistake us for a recalcitrant
		 * client.
		 */
		DestroyPropertyPopup ();
		_OlPopdownHelpTree(workspaceMenu);
		XtPopdown (exitNotice);
		OlUpdateDisplay (exitNotice);
		OlUpdateDisplay (workspaceMenu);
	}

	SetWSMBang (display, root, SubstructureRedirectMask);
	return;
} /* ExitNoticeCB */
