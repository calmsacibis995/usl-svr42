/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olexamples:popupwindo/unit_test1.c	1.6"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/OblongButt.h>
#include <Xol/RectButton.h>
#include <Xol/Caption.h>
#include <Xol/PopupWindo.h>
#include <Xol/Exclusives.h>
#include <Xol/ControlAre.h>
#include <Xol/StaticText.h>

static char *names[] = {
	"One",			/*SC*/
	"Two",			/*SC*/
	"Three"			/*SC*/
};

static void Go(), Apply(), Reset(), SetDefaults(), ResetFactory();

void 
ExclusivesCallback(widget,client_data,call_data)
Widget widget;
caddr_t client_data, call_data;
{
	printf("button %d is set\n",client_data);
}

QuitCallback(w,client_data,call_data)
Widget w;
caddr_t client_data, call_data;
{
	exit(0);
}

PopupCallback(w,client_data,call_data)
Widget w;
caddr_t client_data, call_data;
{
	XtPopup(client_data,XtGrabNone);
}

static void
Apply(w,client_data,call_data)
Widget w;
caddr_t client_data, call_data;
{
	printf ("Apply callback!\n");
}

static void
SetDefaults(w,client_data,call_data)
Widget w;
caddr_t client_data, call_data;
{
	printf ("SetDefaults callback!\n");
}

static void
ResetFactory(w,client_data,call_data)
Widget w;
caddr_t client_data, call_data;
{
	printf ("ResetFactory callback!\n");
}

static void
Reset(w,client_data,call_data)
Widget w;
caddr_t client_data, call_data;
{
	printf ("Reset callback!\n");
}

main(argc, argv)
int argc;
char **argv;
{
	Arg	args[10];
	int	i;
	Widget	toplevel, button, popupshell, controlparent;
	Widget	uppercontrol, lowercontrol, footer;

	static XtCallbackRec resetcalls[] = {
		{ Reset, NULL },
		{ NULL, NULL },
	};

	static XtCallbackRec applycalls[] = {
		{ Apply, NULL },
		{ NULL, NULL },
	};

	static XtCallbackRec factorycalls[] = {
		{ ResetFactory, NULL },
		{ NULL, NULL },
	};

	static XtCallbackRec defaultcalls[] = {
		{ SetDefaults, NULL },
		{ NULL, NULL },
	};


	toplevel = OlInitialize("test", "test", NULL, NULL, &argc, argv);
/*
** Create control area widget to manage two buttons.
*/
	i = 0;
	XtSetArg(args[i], XtNlayoutType, OL_FIXEDCOLS);		i++;
	XtSetArg(args[i], XtNcenter, True);			i++;
	XtSetArg(args[i], XtNsameSize, OL_NONE);		i++;
	controlparent = XtCreateManagedWidget(	"control", 
						controlAreaWidgetClass,
						toplevel, args, i);

/*
** We set the callbacks for apply and reset.  The Popup window will
** make two buttons appropriately labelled.
**
** YOU MUST SET CALLBACKS BEFORE CREATING THE WIDGET, OR THE
** AUTO-BUTTONS WON'T BE CREATED.
*/
	i = 0;
	XtSetArg(args[i], XtNreset, (XtArgVal) resetcalls);		i++;
	XtSetArg(args[i], XtNapply, (XtArgVal) applycalls);		i++;
	XtSetArg(args[i], XtNresetFactory, (XtArgVal) factorycalls);	i++;
	XtSetArg(args[i], XtNsetDefaults, (XtArgVal) defaultcalls);	i++;
	/*
	XtSetArg(args[i], XtNlayoutType, OL_FIXEDROWS);		i++;
	XtSetArg(args[i], XtNalignCaptions, True);		i++;
	XtSetArg(args[i], XtNmeasure, 3);			i++;
	XtSetArg(args[i], XtNvPad, 20);			i++;
	XtSetArg(args[i], XtNhPad, 20);			i++;
	XtSetArg(args[i], XtNsameSize, OL_NONE);		i++;
	*/

/*
** Create popup popup window.
*/
	popupshell = XtCreatePopupShell(	"PopupShell", 
						popupWindowShellWidgetClass,
						controlparent, args, i);

/*
** The popup window will automatically make three children, for upper and
** lower control areas and footer.  We arrange to be given pointers to them.
*/
	i = 0;
	XtSetArg(args[i], XtNupperControlArea, &uppercontrol);	i++;
	XtSetArg(args[i], XtNlowerControlArea, &lowercontrol);	i++;
	XtSetArg(args[i], XtNfooterPanel, &footer);		i++;

	XtGetValues (popupshell, args, i);

	PopulateControlPanel(uppercontrol);
	PopulateLowerControlPanel(lowercontrol);

/*
** test the footer.
*/
	i = 0;
	XtSetArg(args[i], XtNborderWidth, 0); i++;
	XtSetArg(args[i], XtNstring, "Here's some footer text"); i++;
	XtCreateManagedWidget(	"footer", staticTextWidgetClass,
				footer, args, i);


/*
** Create the two button children of the control area.  This could have been
** done earlier, except that we needed popupshell to be initialized so we
** could use it in the client_data of the first button's callback.
*/

	XtSetArg(args[0], XtNlabel, "Popup the window");
	button = XtCreateManagedWidget(	"button0", 
					oblongButtonWidgetClass, 
					controlparent, args, 1);
	XtAddCallback(button, XtNselect, PopupCallback, popupshell);

	XtSetArg(args[0], XtNlabel, "Quit");
	button = XtCreateManagedWidget(	"button1", 
					oblongButtonWidgetClass, 
					controlparent, args, 1);
	XtAddCallback(button, XtNselect, QuitCallback, NULL);

/*
** Start things up and give control to the intrinsics.
*/

	InitializeOpenLook(XtDisplay(toplevel));
	XtRealizeWidget(toplevel);
	XtMainLoop();
}


/*
** This routine just adds a child to the parent it has been passed.  
** The child is a caption widget, containing an exclusive settings 
** widget as its child, which in turn contains three rectangular buttons.
*/

PopulateControlPanel(parent)
Widget	parent;
{
	Widget	caption, exclusives, button;
	Arg args[2];

	XtSetArg(args[0], XtNlabel, "The world is a:");
	XtSetArg(args[1], XtNborderWidth, 0);
	caption = XtCreateManagedWidget(	"caption", 
						captionWidgetClass, 
						parent, args, 2);

	exclusives = XtCreateManagedWidget(	"exclusives",
						exclusivesWidgetClass,
						caption,NULL,0);

	XtSetArg(args[0],XtNlabel,(XtArgVal) "sphere");
	button = XtCreateManagedWidget(	"rect1",
					rectButtonWidgetClass,
					exclusives,args,1);
	XtAddCallback(button,XtNselect,ExclusivesCallback,1);

	XtSetArg(args[0],XtNlabel,(XtArgVal) "cube");
	button = XtCreateManagedWidget(	"rect2",
					rectButtonWidgetClass,
					exclusives,args,1);
	XtAddCallback(button,XtNselect,ExclusivesCallback,2);

	XtSetArg(args[0],XtNlabel,(XtArgVal) "tetrahedron");
	button = XtCreateManagedWidget(	"rect3",
					rectButtonWidgetClass,
					exclusives,args,1);
	XtAddCallback(button,XtNselect,ExclusivesCallback,3);

	XtSetArg(args[0], XtNlabel, "Childless caption:");
	caption = XtCreateManagedWidget(	"caption", 
						captionWidgetClass, 
						parent, args, 2);

	XtSetArg(args[0], XtNlabel, "Another:");
	caption = XtCreateManagedWidget(	"caption", 
						captionWidgetClass, 
						parent, args, 2);

}

PopulateLowerControlPanel(parent)
Widget	parent;
{
	int j = 0, i;
	Widget	w;
	Arg args[5];

	for (j = 0; j < 3; j++) {
		i = 0;
		XtSetArg(args[i], XtNlabel, names[j]);		i++;
		if (j == 2)
			XtSetArg(args[i], XtNdefault, True);	i++;

		w = XtCreateManagedWidget(	names[j],
						oblongButtonWidgetClass, 
						parent,
						args,
						i);
		XtAddCallback(w,XtNselect,Go,names[j]);
	}
}

static void
Go(w,client_data,call_data)
Widget w;
caddr_t client_data;
caddr_t call_data;
{
	if (client_data && *client_data) {
		printf ("Button hit: %s\n",client_data);
	}
}
