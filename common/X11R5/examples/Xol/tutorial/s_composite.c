/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:tutorial/s_composite.c	1.5"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/OblongButt.h>
#include <Xol/ControlAre.h>


void 
ToggleCallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	Arg	args[1];

	static int value = 1;
	int n;

	n = 0;

	switch (value) {
	case 1:
		XtSetArg(args[n], XtNlabel, "Two");	n++;
		value++;
		break;
	case 2:
		XtSetArg(args[n], XtNlabel, "One");	n++;
		value--;
	}
	XtSetValues (widget, args, n);
}

void 
QuitCallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	exit(0);
}


int main(argc, argv)
int argc;
char **argv;
{

	Widget	toplevel, control, toggleButton, quitButton;
	Arg	args[10];
	int	n;

	toplevel = OlInitialize("top", "Top", NULL, 0, &argc, argv);

	n = 0;
	XtSetArg(args[n], XtNlayoutType, OL_FIXEDCOLS);	n++;
	XtSetArg(args[n], XtNmeasure, 1);		n++;
	control = XtCreateManagedWidget(	"control", 
						controlAreaWidgetClass, 
						toplevel, args, n);

	n = 0;
	XtSetArg(args[n], XtNlabel, "Quit");		n++;
	quitButton = XtCreateManagedWidget(	"qbutton", 
						oblongButtonWidgetClass, 
						control, args, n);

	XtAddCallback(quitButton, XtNselect, QuitCallback, NULL);

	n = 0;
	XtSetArg(args[n], XtNlabel, "One");		n++;
	toggleButton = XtCreateManagedWidget(	"tbutton", 
						oblongButtonWidgetClass, 
						control, args, n);

	XtAddCallback(toggleButton, XtNselect, ToggleCallback, NULL);

	XtRealizeWidget(toplevel);
	XtMainLoop();
}
