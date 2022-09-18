/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ifndef	NOIDENT
#ident	"@(#)olexamples:text/unittest15.c	1.4"
#endif

/*
 *  OLXTK-32.7 - test that the XtNfile resource works only when
 *	the sourceType is OL_DISK_SOURCE.  The third widget should
 *	create a temporary file for editing because XtNfile was
 *	not set.
 */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/Text.h>

static char string1[] = "This is a string that you should see. \
 This test is setting XtNfile when XtNsourceType to OL_STRING_SOURCE.";

static char string2[] = "This is a string that you should NOT see. \
 This test is setting XtNfile when XtNsourceType to OL_DISK_SOURCE.";

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
	XtSetArg(arg[n], XtNfile, "disksrc");			n++;
	XtSetArg(arg[n], XtNsourceType, OL_STRING_SOURCE);	n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	editText = XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string2);			n++;
	XtSetArg(arg[n], XtNfile, "disksrc");			n++;
	XtSetArg(arg[n], XtNsourceType, OL_DISK_SOURCE);	n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);

/*
	n = 0;
	XtSetArg(arg[n], XtNsourceType, OL_DISK_SOURCE);	n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);
*/

	n = 0;
	XtSetArg(arg[n], XtNsourceType, OL_DISK_SOURCE);	n++;
	XtSetArg(arg[n], XtNeditType, OL_TEXT_READ);		n++;
	XtSetArg(arg[n], XtNfile, "disksrc");			n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);

	XtRealizeWidget(toplevel);
	XtMainLoop();
}
