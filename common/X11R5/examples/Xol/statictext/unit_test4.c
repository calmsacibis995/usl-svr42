/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:statictext/unit_test4.c	1.6"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/StaticText.h>
#include <Xol/ControlAre.h>

static char string1[] = "     Five leading blanks";
static char string2[] = "Five trailing blanks     ";
static char string3[] = "     Five blanks on both sides     ";

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
	box = XtCreateManagedWidget("box",
		controlAreaWidgetClass,
		toplevel,
		arg,
		n);
/*
 *  OLXTK-31.5
 */

	n = 0;
	XtSetArg(arg[n], XtNstring, string1);			n++;
	XtSetArg(arg[n], XtNstrip, FALSE);			n++;
	XtSetArg(arg[n], XtNalignment, OL_CENTER);	n++;
	XtCreateManagedWidget("Static Text",
		staticTextWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string2);			n++;
	XtSetArg(arg[n], XtNstrip, FALSE);			n++;
	XtSetArg(arg[n], XtNalignment, OL_RIGHT);		n++;
	XtCreateManagedWidget("Static Text",
		staticTextWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string3);			n++;
	XtSetArg(arg[n], XtNstrip, FALSE);			n++;
	XtSetArg(arg[n], XtNalignment, OL_LEFT);		n++;
	XtCreateManagedWidget("Static Text",
		staticTextWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string1);			n++;
	XtSetArg(arg[n], XtNstrip, TRUE);			n++;
	XtSetArg(arg[n], XtNalignment, OL_LEFT);		n++;
	XtCreateManagedWidget("Static Text",
		staticTextWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string2);			n++;
	XtSetArg(arg[n], XtNstrip, TRUE);			n++;
	XtSetArg(arg[n], XtNalignment, OL_RIGHT);		n++;
	XtCreateManagedWidget("Static Text",
		staticTextWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNstring, string3);			n++;
	XtSetArg(arg[n], XtNstrip, TRUE);			n++;
	XtSetArg(arg[n], XtNalignment, OL_CENTER);	n++;
	XtCreateManagedWidget("Static Text",
		staticTextWidgetClass,
		box,
		arg,
		n);

	XtRealizeWidget(toplevel);
	XtMainLoop();
}
