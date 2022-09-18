/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:text/unit_test2.c	1.4"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/Text.h>

static char string1[] = "This is a XtNeditType OL_TEXT_READ.\n";
static char string2[] = "This is a XtNeditType OL_TEXT_APPEND.\n";
static char string3[] = "This is a XtNeditType OL_TEXT_EDIT.\n";
static char string4[] = "This is an invalid XtNeditType 99.\n";

void main (argc, argv)
int argc;
char **argv;
{
	Widget toplevel, box, editText;
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

/*
 *  OLXTK-32.5 - Test that the XtNeditType resource works for the
 *	valid values: OL_TEXT_READ, OL_TEXT_APPEND, OL_TEXT_EDIT.
 *	Also try an invalid value: 99.
 */
	n = 0;
	XtSetArg(arg[n], XtNstring, string1);			n++;
	XtSetArg(arg[n], XtNeditType, OL_TEXT_READ);		n++;
	XtSetArg(arg[n], XtNwidth, 175);			n++;
	XtSetArg(arg[n], XtNheight, 100);			n++;
	editText = XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string2);			n++;
	XtSetArg(arg[n], XtNeditType, OL_TEXT_APPEND);		n++;
	XtSetArg(arg[n], XtNwidth, 175);			n++;
	XtSetArg(arg[n], XtNheight, 100);			n++;
	editText = XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string3);			n++;
	XtSetArg(arg[n], XtNeditType, OL_TEXT_EDIT);		n++;
	XtSetArg(arg[n], XtNwidth, 175);			n++;
	XtSetArg(arg[n], XtNheight, 100);			n++;
	editText = XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string4);			n++;
	XtSetArg(arg[n], XtNeditType, 99);			n++;
	XtSetArg(arg[n], XtNwidth, 175);			n++;
	XtSetArg(arg[n], XtNheight, 100);			n++;
	editText = XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);

	XtRealizeWidget(toplevel);
	XtMainLoop();
}
