/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:tutorial/s_menu.c	1.4"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ButtonStac.h>
#include <Xol/Menu.h>
#include <Xol/Exclusives.h>
#include <Xol/RectButton.h>

void 
QuitCallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	exit(0);
}

void 
SelectCallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	printf("Button %d selected\n", *clientData);
}

void 
UnselectCallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	printf("Button %d unselected\n", *clientData);
}

void 
main(argc, argv) 
int argc;
char **argv;
{
	Widget	toplevel,buttonStack;
	Widget	menupane,exclusives,button1,button2,button3;

	Arg args[10];
	int n;
	int n1, n2;

	toplevel = OlInitialize("top", "Top", NULL, 0, &argc, argv);


	buttonStack = XtCreateManagedWidget(	"bstack", 
						buttonStackWidgetClass,
						toplevel, NULL, 0);

	n = 0;
	XtSetArg(args[n], XtNmenuPane, &menupane);	n++;
	XtGetValues(buttonStack, args, n);


	n = 0;
	XtSetArg(args[n],XtNlayout, OL_FIXEDCOLS);	n++;
	XtSetArg(args[n],XtNmeasure, 1);		n++;
	XtSetArg(args[n],XtNrecomputeSize,(XtArgVal) TRUE);	n++;

	exclusives= XtCreateManagedWidget(	"exclusives",
						exclusivesWidgetClass,
						menupane,args,n);


	n1 = 1;
	button1 = XtCreateManagedWidget(	"ONE",
						rectButtonWidgetClass,
						exclusives, NULL, 0);
	XtAddCallback(button1, XtNselect, SelectCallback, &n1);
	XtAddCallback(button1, XtNunselect, UnselectCallback, &n1);

	n2 = 2;
	button2 = XtCreateManagedWidget(	"TWO", 
						rectButtonWidgetClass,
						exclusives, NULL, 0);
	XtAddCallback(button2, XtNselect, SelectCallback, &n2);
	XtAddCallback(button2, XtNunselect, UnselectCallback, &n2);

	button3 = XtCreateManagedWidget(	"EXIT", 
						rectButtonWidgetClass,
						exclusives, NULL, 0);
	XtAddCallback(button3, XtNselect, QuitCallback, NULL);

	XtRealizeWidget(toplevel);
	XtMainLoop();

}
