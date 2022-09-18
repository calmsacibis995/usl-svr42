/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef	NOIDENT
#ident	"@(#)olexamples:text/unit_test7.c	1.6"
#endif

/*
 *  OLXTK-32.12 - test that the XtNinsertPosition resource works
 *	for a positive number, 0, a number greater that the
 *	length of the string, and a negative number.
 */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/Text.h>

static char string1[] = "This is a text widget. \
 This test is setting XtNinsertPosition to 20.";

static char string2[] = "This is a text widget. \
 This test is setting XtNinsertPosition to 0.";

static char string3[] = "This is a text widget. \
 This test is setting XtNinsertPosition to -1.";

static char string4[] = "This is a text widget. \
 This test is setting XtNinsertPosition to 200.";

void main (argc, argv)
int argc;
char **argv;
{
	Widget toplevel, box, editText;
	Arg arg[20];
	unsigned int n;

	/* <mikez> 21-Jun */
	printf("This unit test used XtNinsertPosition which is no longer in the API\n");
	exit(1);

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
	XtSetArg(arg[n], XtNinsertPosition, 20);		n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	editText = XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string2);			n++;
	XtSetArg(arg[n], XtNinsertPosition, 0);			n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	editText = XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string3);			n++;
	XtSetArg(arg[n], XtNinsertPosition, -1);		n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	editText = XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string4);			n++;
	XtSetArg(arg[n], XtNinsertPosition, 200);		n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	editText = XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);


	XtRealizeWidget(toplevel);
	XtMainLoop();
}
