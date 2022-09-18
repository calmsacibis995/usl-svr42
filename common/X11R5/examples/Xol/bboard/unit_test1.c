/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:bboard/unit_test1.c	1.7"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OlStrings.h>
#include <Xol/OpenLook.h>
#include <Xol/Button.h>
#include <Xol/RectButton.h>
#include <Xol/OblongButt.h>
#include <Xol/BulletinBo.h>

Widget bulletin;
int num_children = 0;

static void	QuitCallback();
static void	AddButtonCallback();
static void	DeleteButtonCallback();

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
	Arg	arg[20];
	unsigned int 	n;
	Widget	w;
	int	xposition;
	int	yposition;

	printf ("Called AddButtonCallback\n");

	num_children++;
	xposition = 50 * num_children;
	yposition = 20 * num_children;

	n = 0;
	XtSetArg(arg[n], XtNlabel, "Go Away");		n++;
	XtSetArg(arg[n], XtNx, xposition);		n++;
	XtSetArg(arg[n], XtNy, yposition);		n++;

	w = XtCreateManagedWidget(	"button0", 
					oblongButtonWidgetClass, 
					bulletin, arg, n);

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

	if (argc != 2) {
		printf ("Usage: %s [123]\n",argv[0]);
		printf ("1=Minimize 2=Maximize 3=Ignore\n");
		exit(0);
	}

	switch (*argv[1]) {
	case '1':	
		layout = OL_MINIMIZE;
		printf ("layout = OL_MINIMIZE\n");
		break;
	case '2':	
		layout = OL_MAXIMIZE;
		printf ("layout = OL_MAXIMIZE\n");
		break;
	case '3':	
		layout = OL_IGNORE;
		printf ("layout = OL_IGNORE\n");
		break;
	default:
		printf ("Usage: %s [123]\n",argv[0]);
		printf ("1=Minimize 2=Maximize 3=Ignore\n");
		exit(0);
	}

	toplevel = OlInitialize("top", "Top", NULL, 0, &argc, argv);

	n = 0;
	XtSetArg(arg[n], XtNheight, 300);		n++;
	XtSetArg(arg[n], XtNwidth, 400);		n++;
	box = XtCreateManagedWidget(	"box", 
					bulletinBoardWidgetClass, 
					toplevel, arg, n);


	n = 0;
	XtSetArg(arg[n], XtNlayout, layout);		n++;
	XtSetArg(arg[n], XtNheight, 100);		n++;
	XtSetArg(arg[n], XtNwidth, 100);		n++;
	bulletin = XtCreateManagedWidget(	"bulletin", 
						bulletinBoardWidgetClass, 
						box, arg, n);

	n = 0;
	XtSetArg(arg[n], XtNlabel, "Quit");		n++;
	XtSetArg(arg[n], XtNx, 0);				n++;
	XtSetArg(arg[n], XtNy, 0);				n++;
	quitButton = XtCreateManagedWidget(	"button0", 
						oblongButtonWidgetClass, 
						bulletin, arg, n);

	XtAddCallback(quitButton, XtNselect, QuitCallback, NULL);	

	n = 0;
	XtSetArg(arg[n], XtNlabel, "Make More");		n++;
	XtSetArg(arg[n], XtNx, 50);				n++;
	XtSetArg(arg[n], XtNy, 0);				n++;
	moreButton = XtCreateManagedWidget(	"button1", 
						oblongButtonWidgetClass, 
						bulletin, arg, n);

	XtAddCallback(moreButton, XtNselect, AddButtonCallback, NULL);	

	XtRealizeWidget(toplevel);
	XtMainLoop();
}
