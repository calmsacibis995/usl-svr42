/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef	NOIDENT
#ident	"@(#)olexamples:text/unittest16.c	1.6"
#endif

/*
 *  OLXTK-32.10 - test that the XtNfontColor resource works
 *	for color.
 */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/Text.h>

static char string1[] = "This is a text widget. \
 This test is setting XtNfontColor to Green. \
The selection color is green and the inputfocus \
color is blue.  background is red.";

static char string2[] = "This is a text widget. \
 This test is setting XtNbackground to Red.";


void main (argc, argv)
int argc;
char **argv;
{
	Widget toplevel, box, text;
	Arg arg[20];
	unsigned int n;
	XColor red_visual_return;
	XColor red_exact_return;
	XColor green_visual_return;
	XColor green_exact_return;
	XColor blue_visual_return;
	XColor blue_exact_return;
	XColor yellow_visual_return;
	XColor yellow_exact_return;


	toplevel = OlInitialize("text",
		"Text",
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

	XAllocNamedColor(XtDisplay(box),
		DefaultColormap(XtDisplay(box), 0),
		"red",
		&red_visual_return,
		&red_exact_return);

	XAllocNamedColor(XtDisplay(box),
		DefaultColormap(XtDisplay(box), 0),
		"green",
		&green_visual_return,
		&green_exact_return);

	XAllocNamedColor(XtDisplay(box),
		DefaultColormap(XtDisplay(box), 0),
		"blue",
		&blue_visual_return,
		&blue_exact_return);

	XAllocNamedColor(XtDisplay(box),
		DefaultColormap(XtDisplay(box), 0),
		"yellow",
		&yellow_visual_return,
		&yellow_exact_return);

	n = 0;
	XtSetArg(arg[n], XtNstring, string1);			n++;
	XtSetArg(arg[n], XtNfontColor, green_exact_return.pixel); n++;
	XtSetArg(arg[n], XtNbackground, red_exact_return.pixel); n++;
	XtSetArg(arg[n], XtNselectionColor, green_exact_return.pixel); n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	text = XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNinputFocusColor, blue_exact_return.pixel); n++;
	XtSetValues(text, arg, n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string2);			n++;
	XtSetArg(arg[n], XtNbackground, red_exact_return.pixel); n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);

	XtRealizeWidget(toplevel);
	XtMainLoop();
}
