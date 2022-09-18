/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:text/unit_test4.c	1.4"
#endif

/*
 *  OLXTK-32.21 - This tests the XtNscroll resource of the Text
 *	widget.  It tries all of the valid values:  OL_AUTO_SCROLL_OFF,
 *	OL_AUTO_SCROLL_VERTICAL, OL_AUTO_SCROLL_HORIZONTAL,
 *	OL_AUTO_SCROLL_BOTH.
 */
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/Text.h>

static char string1[] = "This is a XtNscroll OL_AUTO_SCROLL_OFF.\n";
static char string2[] = "This is a XtNscroll OL_AUTO_SCROLL_VERTICAL.\n";
static char string3[] = "This is a XtNscroll OL_AUTO_SCROLL_HORIZONTAL.\n";
static char string4[] = "This is a XtNscroll OL_AUTO_SCROLL_BOTH.\n";

void main (argc, argv)
int argc;
char **argv;
{
	Widget toplevel, box;
	TextWidget textWidget;
	Arg arg[20];
	unsigned int n;

	toplevel = OlInitialize("quitButton",
		"QuitButton",
		NULL,
		0,
		&argc,
		argv);

	n = 0;
	XtSetArg(arg[n], XtNlayoutType, OL_FIXEDCOLS);		n++;
	box = XtCreateManagedWidget("ControlArea",
		controlAreaWidgetClass,
		toplevel,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string1);			n++;
	XtSetArg(arg[n], XtNwrap, FALSE);			n++;
	XtSetArg(arg[n], XtNscroll, OL_AUTO_SCROLL_OFF);	n++;
	XtSetArg(arg[n], XtNheight, 20);			n++;
	XtSetArg(arg[n], XtNwidth, 300);			n++;
	XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string2);			n++;
	XtSetArg(arg[n], XtNwrap, FALSE);			n++;
	XtSetArg(arg[n], XtNscroll, OL_AUTO_SCROLL_VERTICAL);	n++;
	XtSetArg(arg[n], XtNheight, 20);			n++;
	XtSetArg(arg[n], XtNwidth, 300);			n++;
	XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string3);			n++;
	XtSetArg(arg[n], XtNwrap, FALSE);			n++;
	XtSetArg(arg[n], XtNscroll, OL_AUTO_SCROLL_HORIZONTAL);	n++;
	XtSetArg(arg[n], XtNheight, 20);			n++;
	XtSetArg(arg[n], XtNwidth, 300);			n++;
	XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string4);			n++;
	XtSetArg(arg[n], XtNwrap, FALSE);			n++;
	XtSetArg(arg[n], XtNscroll, OL_AUTO_SCROLL_BOTH);	n++;
	XtSetArg(arg[n], XtNheight, 20);			n++;
	XtSetArg(arg[n], XtNwidth, 300);			n++;
	XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);

	XtRealizeWidget(toplevel);
	XtMainLoop();
}
