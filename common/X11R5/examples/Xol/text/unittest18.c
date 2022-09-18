/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef	NOIDENT
#ident	"@(#)olexamples:text/unittest18.c	1.4"
#endif

/*
 *  OLXTK-32.16 - test that the XtNmodifyVerification resource works
 *	for a simple callback when you insert or delete text.
 *  OLXTK-32.17 - test that the XtNmotionVerification resource works
 *	for a simple callback when you move the insertion cursor.
 */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/Text.h>

static char string1[] = "This test sets the \
 XtNmodifyVerification resource to a callback that just prints \
 'Called modify verification callback' when you delete or insert \
text.  It also tests XtNmotionVerification by setting to to \
a callback that prints 'Called motion verificaitoncallback' \
when the insertion cursor is moved to a new position";

void modifyCallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	printf("Called modify verification callback.\n");
}

void motionCallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	printf("Called motion verification callback.\n");
}

void main (argc, argv)
int argc;
char **argv;
{
	Widget toplevel, box, text;
	Arg arg[20];
	unsigned int n;

	toplevel = OlInitialize("text",
		"text",
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
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	text = XtCreateManagedWidget("Text",
		textWidgetClass,
		box,
		arg,
		n);
	XtAddCallback(text, XtNmodifyVerification, modifyCallback, NULL);
	XtAddCallback(text, XtNmotionVerification, motionCallback, NULL);
	

	XtRealizeWidget(toplevel);
	XtMainLoop();
}
