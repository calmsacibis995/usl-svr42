/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ifndef	NOIDENT
#ident	"@(#)olexamples:text/unittest14.c	1.4"
#endif

/*
 *  OLXTK-32.26 - This test sets the XtNwrapBreak resource to the
 *	valid values: OL_WRAP_ANY and OL_WRAP_WHITESPACE.  It also
 *	sets one to something else: 99.
 */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/Text.h>

static char string1[] = "This is a XtNwrap TRUE and XtNwrapBreak \
OL_WRAP_ANY.  It should break on any character \
the right margin.\n";

static char string2[] = "This is a XtNwrap TRUE and XtNwrapBreak \
OL_WRAP_WHITE_SPACE.  This line should break on white space.\n";

static char string3[] = "This_is_a_XtNwrap_TRUE_and_XtNwrapBreak_\
OL_WRAP_WHITE_SPACE.__This_line_should_break_on_any_character.\n";

static char string4[] = "This is an invalid XtNwrapBreak 99.  \
 It should default to OL_WRAP_ANY.\n";

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

	n = 0;
	XtSetArg(arg[n], XtNstring, string1);			n++;
	XtSetArg(arg[n], XtNwrap, TRUE);			n++;
	XtSetArg(arg[n], XtNwrapBreak, OL_WRAP_ANY);		n++;
	XtSetArg(arg[n], XtNwidth, 175);			n++;
	editText = XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string2);			n++;
	XtSetArg(arg[n], XtNwrap, TRUE);			n++;
	XtSetArg(arg[n], XtNwrapBreak, OL_WRAP_WHITE_SPACE);	n++;
	XtSetArg(arg[n], XtNwidth, 175);			n++;
	editText = XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string3);			n++;
	XtSetArg(arg[n], XtNwrap, TRUE);			n++;
	XtSetArg(arg[n], XtNwrapBreak, 99);			n++;
	XtSetArg(arg[n], XtNwidth, 175);			n++;
	editText = XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);

	XtRealizeWidget(toplevel);
	XtMainLoop();
}
