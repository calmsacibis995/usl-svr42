/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:text/unit_test3.c	1.4"
#endif

/*
 *  OLXTK-32.11 - This tests the XtNgrow resource.  It tries all
 *	of the valid values: OL_GROW_OFF, OL_GROW_HORIZONTAL,
 *	OL_GROW_VERTICAL, and OL_GROW_BOTH.  Other tests should be
 *	added to check the precedence of XtNgrow, XtNwrap, and
 *	XtNscroll.  XtNgrow should have the highest priority.
 */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/Text.h>

static char string1[] = "This is a XtNgrow OL_GROW_OFF.\n";
static char string2[] = "This is a XtNgrow OL_GROW_HORIZONTAL.\n";
static char string3[] = "This is a XtNgrow OL_GROW_VERTICAL.\n";
static char string4[] = "This is a XtNgrow OL_GROW_BOTH.\n";

void main (argc, argv)
int argc;
char **argv;
{
	Widget toplevel, box;
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
	XtSetArg(arg[n], XtNgrow, OL_GROW_OFF);			n++;
	XtSetArg(arg[n], XtNwidth, 175);			n++;
	XtSetArg(arg[n], XtNheight, 50);			n++;
	XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string2);			n++;
	XtSetArg(arg[n], XtNgrow, OL_GROW_HORIZONTAL);		n++;
	XtSetArg(arg[n], XtNwidth, 175);			n++;
	XtSetArg(arg[n], XtNheight, 50);			n++;
	XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string3);			n++;
	XtSetArg(arg[n], XtNgrow, OL_GROW_VERTICAL);		n++;
	XtSetArg(arg[n], XtNwidth, 175);			n++;
	XtSetArg(arg[n], XtNheight, 50);			n++;
	XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string4);			n++;
	XtSetArg(arg[n], XtNgrow, OL_GROW_BOTH);		n++;
	XtSetArg(arg[n], XtNwidth, 175);			n++;
	XtSetArg(arg[n], XtNheight, 50);			n++;
	XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);

	XtRealizeWidget(toplevel);
	XtMainLoop();
}
