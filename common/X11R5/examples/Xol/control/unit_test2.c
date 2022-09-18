/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olexamples:control/unit_test2.c	1.7"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Button.h>
#include <Xol/RectButton.h>
#include <Xol/OblongButt.h>
#include <Xol/ControlAre.h>
#include <Xol/BulletinBo.h>

Widget control;
int num_children = 0;
int measure = 4;

static void	QuitCallback();
static void	AddButtonCallback();
static void	DeleteButtonCallback();
static void	IncrementCallback();
static void	DecrementCallback();

static	/*SC, added*/
void 
IncrementCallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	Arg	arg[1];
	measure++;
	(void) printf("Measure is now %d\n",measure);

	XtSetArg(arg[0], XtNmeasure, measure);
	XtSetValues (control, arg, 1);

}


static	/*SC, added*/
void 
DecrementCallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	Arg	arg[1];
	measure--;
	(void) printf("Measure is now %d\n",measure);

	XtSetArg(arg[0], XtNmeasure, measure);
	XtSetValues (control, arg, 1);
}


static	/*SC, added*/
void 
QuitCallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	(void) printf("Called quit button callback.\n");
	exit(0);
}


static	/*SC, added*/
void 
AddButtonCallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	Arg	arg[10];
	unsigned int 	n;
	Widget	w;

	printf ("Called AddButtonCallback\n");

	num_children++;

	n = 0;
	XtSetArg(arg[n], XtNlabel, "Go Away");		n++;

	w = XtCreateManagedWidget(	"button0", 
					oblongButtonWidgetClass, 
					control, arg, n);

	XtAddCallback(w, XtNselect, DeleteButtonCallback, NULL);	
}

static	/*SC, added*/
void
DeleteButtonCallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	printf ("Deleting widget %x\n",widget);

	XtDestroyWidget(widget);
}

int main(argc, argv)
unsigned int argc;
char **argv;
{

	Widget	toplevel, quitButton, moreButton, box;
	Arg	arg[20];
	unsigned int	n;
	int	layout;

	toplevel = OlInitialize("top", "Top", NULL, 0, &argc, argv);

	n = 0;
	box = XtCreateManagedWidget(	"box", 
					controlAreaWidgetClass, 
					toplevel, arg, n);

	n = 0;
	XtSetArg(arg[n], XtNlayoutType, OL_FIXEDROWS);	n++;
	XtSetArg(arg[n], XtNmeasure, measure);		n++;
	control = XtCreateManagedWidget(	"control", 
						controlAreaWidgetClass, 
						box, arg, n);

	n = 0;
	XtSetArg(arg[n], XtNlabel, "Quit");		n++;
	quitButton = XtCreateManagedWidget(	"button0", 
						oblongButtonWidgetClass, 
						control, arg, n);

	XtAddCallback(quitButton, XtNselect, QuitCallback, NULL);	

	n = 0;
	XtSetArg(arg[n], XtNlabel, "Make More");		n++;
	moreButton = XtCreateManagedWidget(	"button1", 
						oblongButtonWidgetClass, 
						control, arg, n);

	XtAddCallback(moreButton, XtNselect, AddButtonCallback, NULL);	

	n = 0;
	XtSetArg(arg[n], XtNlabel, "Increment measure");	n++;
	moreButton = XtCreateManagedWidget(	"button2", 
						oblongButtonWidgetClass, 
						control, arg, n);

	XtAddCallback(moreButton, XtNselect, IncrementCallback, NULL);	

	n = 0;
	XtSetArg(arg[n], XtNlabel, "Decrement measure");	n++;
	moreButton = XtCreateManagedWidget(	"button3", 
						oblongButtonWidgetClass, 
						control, arg, n);

	XtAddCallback(moreButton, XtNselect, DecrementCallback, NULL);	

	XtRealizeWidget(toplevel);
	XtMainLoop();
}
