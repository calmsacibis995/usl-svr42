/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:button/unit_test4.c	1.7"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/ControlAre.h>
#include <Xol/OpenLook.h>
#include <Xol/RectButton.h>
#include <Xol/OblongButt.h>

Widget evenButton;
Widget oddButton;

void Quitcallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	exit(0);
}


int main(argc, argv)
unsigned int argc;
char **argv;
{

	Widget toplevel, box, quitButton;
	Arg arg[20];
	unsigned int n;

	toplevel = OlInitialize("toplevel",
		"Toplevel",
		NULL,
		0,
		&argc,
		argv);

	n=0;
	XtSetArg(arg[n], XtNlayoutType, OL_FIXEDCOLS); n++;
	XtSetArg(arg[n], XtNmeasure, 1); n++;
	box = XtCreateManagedWidget("box",
		controlAreaWidgetClass,
		toplevel,
		arg,
		2);


	n = 0;
	quitButton = XtCreateManagedWidget("Quit",
		oblongButtonWidgetClass,
		box,
		arg,
		n);
	XtAddCallback(quitButton, XtNselect, Quitcallback, NULL);	

	n = 0;
	XtSetArg(arg[n], XtNrecomputeSize, TRUE);		n++;
/*	XtSetArg(arg[n], XtNbuttonType, OL_HALFSTACK);		n++; */
	evenButton = XtCreateManagedWidget("Label Length",
		oblongButtonWidgetClass,
		box,
		arg,
		n);


	n = 0;
	XtSetArg(arg[n], XtNrecomputeSize, TRUE);		n++;
	XtSetArg(arg[n], XtNbuttonType, OL_HALFSTACK);		n++;
	evenButton = XtCreateManagedWidget("Label Length",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNbuttonType, OL_HALFSTACK);		n++;
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
	XtSetArg(arg[n], XtNwidth, 158);			n++;
	XtSetArg(arg[n], XtNheight, 30);			n++;
	oddButton = XtCreateManagedWidget("Odd Length\nNewLine",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNbuttonType, OL_HALFSTACK);		n++;
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
	XtSetArg(arg[n], XtNwidth, 150);			n++;
	XtSetArg(arg[n], XtNheight, 30);			n++;
	oddButton = XtCreateManagedWidget("Odd Length",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	XtRealizeWidget(toplevel);
	XtMainLoop();
}
