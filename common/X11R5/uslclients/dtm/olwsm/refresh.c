/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:olwsm/refresh.c	1.6"
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <OpenLook.h>

#include <misc.h>
#include <list.h>
#include <wsm.h>

static Window		CreateCover OL_ARGS((
	Screen *		scr
));

/**
 ** RefreshCB()
 **/

void
#if	OlNeedFunctionPrototypes
RefreshCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
RefreshCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	static Window		cover = (Window)0;


	if (!cover)
		cover = CreateCover(SCREEN);

	XMapRaised (DISPLAY, cover);
	XUnmapWindow (DISPLAY, cover);

	return;
} /* RefreshCB */

/**
 ** CreateCover()
 **/

static Window
#if	OlNeedFunctionPrototypes
CreateCover (
	Screen *		scr
)
#else
CreateCover (scr)
	Screen *		scr;
#endif
{
	XSetWindowAttributes	xswa;

	xswa.background_pixmap = None;
	xswa.override_redirect = True;

	return (XCreateWindow(
		DisplayOfScreen(scr),
		RootWindowOfScreen(scr),
		0, 0,
		WidthOfScreen(scr), HeightOfScreen(scr),
		0,
		PlanesOfScreen(scr),
		InputOutput,
		CopyFromParent,
		CWBackPixmap | CWOverrideRedirect,
		&xswa
	));
} /* CreateCover */
