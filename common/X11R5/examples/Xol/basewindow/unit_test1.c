/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:basewindow/unit_test1.c	1.3"
#endif
/*
 unit_test1.c (C source file)
	Acc: 602549679 Fri Feb  3 17:54:39 1989
	Mod: 602546759 Fri Feb  3 17:05:59 1989
	Sta: 602546759 Fri Feb  3 17:05:59 1989
	Owner: 4777
	Group: 1985
	Permissions: 664
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
#include <stdio.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include <Xol/BaseWindow.h>
#include <Xol/ControlAre.h>
#include <Xol/FooterPane.h>
#include <Xol/OblongButt.h>
#include <Xol/StaticText.h>
#include <Xol/Text.h>

static void Button();
static void Msg();
static void Usage();

static Arg	args[10];
static Cardinal	cnt;

main(argc, argv)
	int	argc;
	char	*argv[];
{
	extern char *	optarg;
	extern int	optind;
	int		opterr = 0;
	int		opt;

	Widget		toplevel, popup;
	Widget		base, panel, control;
	Widget		footer, msg, button, popdown;
	Boolean		stext_footer = FALSE;
	Boolean		text_footer = FALSE;
	XtPopdownIDRec	popdown_widgets;

	/* Initialize the toolkit.  do this before 'getopt' (below) so we
	 * have a shot at using toolkit flags (ie. -display)
	 */
	toplevel = OlInitialize(NULL, "PAnel", NULL, 0, &argc, argv);

	while ( (opt = getopt(argc, argv, "df:?")) != -1)
		switch (opt) {
		case 'd' :
#ifdef PANEL_DEBUG
		_panelDebug = TRUE;
#else
		fprintf(stderr,
			"%s: not compiled with -DLIST_DEBUG \n",
			argv[0]);
#endif /* PANEL_DEBUG */
			break;
		case 'f' :
			if (optarg[0] == 't')
				text_footer = TRUE;
			else if (optarg[0] == 's')
				stext_footer = TRUE;
			else
				Usage(argv[0]);
			break;
		case '?' :
			opterr++;
			break;
		}
	if (opterr)
		Usage(argv[0]);

	base = XtCreatePopupShell("base", baseWindowShellWidgetClass,
			toplevel, NULL, 0);

	cnt = 0;
	popup = XtCreateManagedWidget("popup", oblongButtonWidgetClass,
			toplevel, args, cnt);
	XtAddCallback(popup, XtNselect, XtCallbackNone, base);

	panel = XtCreateManagedWidget("box", footerPanelWidgetClass,
			base, NULL, 0);
	cnt = 0;
	XtSetArg(args[cnt], XtNcenter, TRUE); cnt++;
	XtSetArg(args[cnt], XtNlayoutType, OL_FIXEDCOLS); cnt++;
	XtSetArg(args[cnt], XtNsameSize, OL_NONE); cnt++;
        control = XtCreateManagedWidget("control", controlAreaWidgetClass,
			panel, args, cnt);

	if (stext_footer || text_footer) {
		WidgetClass footerClass;

		cnt = 0;
		XtSetArg(args[cnt], XtNborderWidth, 0); cnt++;
		XtSetArg(args[cnt], XtNstring, "footer"); cnt++;

		if (stext_footer)
			footerClass = staticTextWidgetClass;
		else {
			footerClass = textWidgetClass;

			XtSetArg(args[cnt], XtNeditType, OL_TEXT_READ); cnt++;
			XtSetArg(args[cnt], XtNsourceType, OL_STRING_SOURCE); cnt++;
			XtSetArg(args[cnt], XtNverticalSB, TRUE); cnt++;
		}

		footer = XtCreateManagedWidget("footer", footerClass,
				panel, args, cnt);

		/* create button to alter footer message */
		cnt = 0;
		msg = XtCreateManagedWidget("message", oblongButtonWidgetClass, 
				control, args, cnt);
		XtAddCallback(msg, XtNselect, Msg, footer);
	}

	cnt = 0;
	button = XtCreateManagedWidget("button", oblongButtonWidgetClass,
			control, args, cnt);
	XtAddCallback(button, XtNselect, Button, NULL);

	cnt = 0;
	popdown = XtCreateManagedWidget("popdown", oblongButtonWidgetClass,
			control, args, cnt);
	popdown_widgets.shell_widget	= base;
	popdown_widgets.enable_widget	= popup;
	XtAddCallback(popdown, XtNselect, XtCallbackPopdown, &popdown_widgets);

	XtRealizeWidget(toplevel);
	XtRealizeWidget(base);
	XtMainLoop();
}

static void
Button(w, closure, call_data)
	Widget w;
	caddr_t closure, call_data;
{
 	static Boolean long_button = TRUE;
	String string = (long_button) ? "long button" : "button";

	cnt = 0;
	XtSetArg(args[cnt], XtNlabel, string); cnt++;
	XtSetValues(w, args, cnt);

	long_button = !long_button;
}

static void
Msg(w, closure, call_data)
	Widget w;
	caddr_t closure, call_data;
{
 	static Boolean long_msg = TRUE;
	String string = (long_msg) ? "longer footer message" : "footer";

	cnt = 0;
	XtSetArg(args[cnt], XtNstring, string); cnt++;
	XtSetValues((Widget)closure, args, cnt);

	long_msg = !long_msg;
}

static void
Usage(name)
	char *	name;
{
 	fprintf(stderr, "usage: %s [-f s|t] [-d] \n", name);
	fprintf(stderr, "\twhere: %s %s %s %s",
                        "\n\t\t-f s	create static text footer",
                        "\n\t\t-f t	create text footer",
                        "\n\t\t-d	debug",
                        "\n"
		);
	exit(0);
}
