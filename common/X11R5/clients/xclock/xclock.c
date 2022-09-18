/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5xclock:xclock.c	1.1"
/* $XConsortium: xclock.c,v 1.32 91/04/19 13:46:20 converse Exp $ */

/*
 * xclock --  Hacked from Tony Della Fera's much hacked clock program.
 */

/*
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 */

#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Clock.h>
#include <X11/Xaw/Cardinals.h>
#include "clock.bit"
#include "clmask.bit"

extern void exit();
static void quit();

/* Command line options table.  Only resources are entered here...there is a
   pass over the remaining options after XtParseCommand is let loose. */

static XrmOptionDescRec options[] = {
{"-chime",	"*clock.chime",		XrmoptionNoArg,		"TRUE"},
{"-hd",		"*clock.hands",		XrmoptionSepArg,	NULL},
{"-hands",	"*clock.hands",		XrmoptionSepArg,	NULL},
{"-hl",		"*clock.highlight",	XrmoptionSepArg,	NULL},
{"-highlight",	"*clock.highlight",	XrmoptionSepArg,	NULL},
{"-update",	"*clock.update",	XrmoptionSepArg,	NULL},
{"-padding",	"*clock.padding",	XrmoptionSepArg,	NULL},
{"-d",		"*clock.analog",	XrmoptionNoArg,		"FALSE"},
{"-digital",	"*clock.analog",	XrmoptionNoArg,		"FALSE"},
{"-analog",	"*clock.analog",	XrmoptionNoArg,		"TRUE"},
};


static XtActionsRec xclock_actions[] = {
    { "quit",	quit },
};

static Atom wm_delete_window;

/*
 * Report the syntax for calling xclock.
 */
Syntax(call)
	char *call;
{
	(void) printf ("Usage: %s [-analog] [-bw <pixels>] [-digital]\n", call);
	(void) printf ("       [-fg <color>] [-bg <color>] [-hd <color>]\n");
	(void) printf ("       [-hl <color>] [-bd <color>]\n");
	(void) printf ("       [-fn <font_name>] [-help] [-padding <pixels>]\n");
	(void) printf ("       [-rv] [-update <seconds>] [-display displayname]\n");
	(void) printf ("       [-geometry geom]\n\n");
	exit(1);
}

void main(argc, argv)
    int argc;
    char **argv;
{
    Widget toplevel;
    Arg arg;
    Pixmap icon_pixmap = None;
    XtAppContext app_con;

    toplevel = XtAppInitialize (&app_con, "XClock", options, XtNumber(options),
				&argc, argv, NULL, NULL, ZERO);
    if (argc != 1) Syntax(argv[0]);

    XtAppAddActions (app_con, xclock_actions, XtNumber(xclock_actions));

    /*
     * This is a hack so that f.delete will do something useful in this
     * single-window application.
     */
    XtOverrideTranslations(toplevel, 
		    XtParseTranslationTable ("<Message>WM_PROTOCOLS: quit()"));

    XtSetArg(arg, XtNiconPixmap, &icon_pixmap);
    XtGetValues(toplevel, &arg, ONE);
    if (icon_pixmap == None) {
	arg.value = (XtArgVal)XCreateBitmapFromData(XtDisplay(toplevel),
				       XtScreen(toplevel)->root,
				       (char *)clock_bits, clock_width, clock_height);
	XtSetValues (toplevel, &arg, ONE);
    }
    XtSetArg(arg, XtNiconMask, &icon_pixmap);
    XtGetValues(toplevel, &arg, ONE);
    if (icon_pixmap == None) {
	arg.value = (XtArgVal)XCreateBitmapFromData(XtDisplay(toplevel),
				       XtScreen(toplevel)->root,
				       (char *)clock_mask_bits, clock_mask_width, 
				       clock_mask_height);
	XtSetValues (toplevel, &arg, ONE);
    }

    XtCreateManagedWidget ("clock", clockWidgetClass, toplevel, NULL, ZERO);
    XtRealizeWidget (toplevel);
    wm_delete_window = XInternAtom (XtDisplay(toplevel), "WM_DELETE_WINDOW",
				    False);
    (void) XSetWMProtocols (XtDisplay(toplevel), XtWindow(toplevel),
			    &wm_delete_window, 1);
    XtAppMainLoop (app_con);
}


static void quit (w, event, params, num_params)
    Widget w;
    XEvent *event;
    String *params;
    Cardinal *num_params;
{
    if (event->type == ClientMessage &&
	event->xclient.data.l[0] != wm_delete_window) {
	XBell (XtDisplay(w), 0);
	return;
    }
    XCloseDisplay (XtDisplay(w));
    exit (0);
}
