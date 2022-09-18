/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:button/unit_test6.c	1.7"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/ControlAre.h>
#include <Xol/OpenLookP.h>
#include <Xol/RectButton.h>
#include <Xol/OblongButt.h>


Widget quitButton;
Widget testButton;
Widget stackButton;

void Quitcallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	exit(0);
}


int test_case = 0;

void testCallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	Arg arg[20];
	unsigned int n;

	n = 0;

	test_case++;

	switch (test_case)  {
	case 1:
		printf("SetValues XtNshellBehavior PressDragReleaseMenu\n");
		n = 0;
		XtSetArg(arg[n], XtNshellBehavior, PressDragReleaseMenu);	n++;
		XtSetValues(stackButton, arg, n);
		break;
	case 2:
		printf("SetValues XtNshellBehavior StayUpMenu\n");
		n = 0;
		XtSetArg(arg[n], XtNshellBehavior, StayUpMenu);	n++;
		XtSetValues(stackButton, arg, n);
		break;
	case 3:
		printf("SetValues XtNshellBehavior PressDragReleaseMenu\n");
		n = 0;
		XtSetArg(arg[n], XtNshellBehavior, PressDragReleaseMenu);	n++;
		XtSetValues(stackButton, arg, n);
		break;
	case 4:
		printf("SetValues XtNshellBehavior PinnedMenu\n");
		n = 0;
		XtSetArg(arg[n], XtNshellBehavior, PinnedMenu);	n++;
		XtSetValues(stackButton, arg, n);
		break;
	case 5:
		printf("SetValues XtNshellBehavior PressDragReleaseMenu\n");
		n = 0;
		XtSetArg(arg[n], XtNshellBehavior, PressDragReleaseMenu);	n++;
		XtSetValues(stackButton, arg, n);
		break;
	case 6:
		printf("SetValues XtNshellBehavior UnpinnedMenu\n");
		n = 0;
		XtSetArg(arg[n], XtNshellBehavior, UnpinnedMenu);	n++;
		XtSetValues(stackButton, arg, n);
		break;
	default:
		test_case = 0;
		printf("That's all the tests\n");
		break;
	}
}


int main(argc, argv)
unsigned int argc;
char **argv;
{

	Widget toplevel, box, quitButton, button2;
	Arg arg[20];
	unsigned int n;
	XImage *my_image;
	Display *display;

	toplevel = OlInitialize("quitButton", "QuitButton", NULL, 0, &argc, argv);

	n=0;
	XtSetArg(arg[n], XtNlayoutType, OL_FIXEDCOLS); n++;
	XtSetArg(arg[n], XtNmeasure, 1); n++;
	box = XtCreateManagedWidget("box", 
		controlAreaWidgetClass, toplevel, arg, 2);

	n = 0;
	quitButton = XtCreateManagedWidget("Quit",
		oblongButtonWidgetClass,
		box,
		arg,
		n);
	XtAddCallback(quitButton, XtNselect, Quitcallback, NULL);	

	n = 0;
	testButton = XtCreateManagedWidget("Test",
		oblongButtonWidgetClass,
		box,
		arg,
		n);
	XtAddCallback(testButton, XtNselect, testCallback, NULL);	

	n = 0;
	XtSetArg(arg[n], XtNbuttonType, OL_BUTTONSTACK);	n++;
	stackButton = XtCreateManagedWidget("Button Stack",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	
	XtRealizeWidget(toplevel);
	XtMainLoop();
}

