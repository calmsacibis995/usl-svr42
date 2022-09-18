/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:caption/unit_test2.c	1.6"
#endif

#include	<stdio.h>
#include	<X11/IntrinsicP.h>
#include	<X11/Xlib.h>
#include	<Xol/Caption.h>
#include	<Xol/ControlAre.h>
#include	<Xol/OblongButt.h>
#include 	<Xol/TextField.h>
#include	<Xol/OpenLook.h>
#include	<X11/StringDefs.h>

static void	ChangeLabelCallback();
static void	ChangeCaptionCallback();

Widget	toplevel, control, caption, label;

/*
**	sample driver for caption widget.  This tests dynamically
**	changing child sizes.
*/

main(argc, argv)
unsigned int argc;
char **argv;
{
	static Arg	args[15];
	int	i;
	int	position;
	int	alignment;
	Widget	w;
	if (argc < 2) {
		printf ("usage: %s caption [TBRL] [TBCRL [space]]\n",argv[0]);
		exit(1);
	}

	if (argc > 2)
	switch (*argv[2]) {
		case 'T' :
			position = OL_TOP;
			printf ("Caption is on top\n");
			break;
		case 'B' :
			position = OL_BOTTOM;
			printf ("Caption is on the bottom\n");
			break;
		case 'R' :
			position = OL_RIGHT;
			printf ("Caption is on the right\n");
			break;
		case 'L' :
		default:
			position = OL_LEFT;
			printf ("Caption is on the left\n");
			break;
	}

	if (argc > 3)
	switch (*argv[3]) {
		case 'T' :
			alignment = OL_TOP;
			printf ("Alignment is top\n");
			break;
		case 'B' :
			alignment = OL_BOTTOM;
			printf ("Alignment is bottom\n");
			break;
		case 'R' :
			alignment = OL_RIGHT;
			printf ("Alignment is right\n");
			break;
		case 'L' :
			alignment = OL_LEFT;
			printf ("Alignment is left\n");
			break;
		case 'C' :
		default:
			alignment = OL_CENTER;
			printf ("Alignment is centered\n");
			break;
	}

	toplevel = OlInitialize("TopLevel", "TopLevel", 0, 0, &argc, argv);

	i = 0;
	XtSetArg(args[i], XtNlayout, OL_FIXEDCOLS);		i++;
	control = XtCreateManagedWidget(	"control", 
					controlAreaWidgetClass, 
					toplevel, args, i);

	i = 0;
	XtSetArg(args[i], XtNcaption, argv[1]);		i++;
	if (argc > 2)
		XtSetArg(args[i], XtNposition, position);	i++;
	if (argc > 3)
		XtSetArg(args[i], XtNalignment, alignment);	i++;
	if (argc > 4) {
		XtSetArg(args[i], XtNspace, atoi(argv[4]));	i++;
		printf ("spacing is %d\n",atoi(argv[4]));
	}

	caption = XtCreateManagedWidget(	"caption widget",
						captionWidgetClass,
						control,
						args,	
						i);

	i = 0;
	XtSetArg(args[i], XtNstring, "default");	i++;
	label = XtCreateManagedWidget(	"button", 
					oblongButtonWidgetClass, 
					caption, 
					args,
					i);

	i = 0;
	XtSetArg(args[i], XtNlabel, "Big");	i++;
	w = XtCreateManagedWidget(	"button0", 
					oblongButtonWidgetClass, 
					control, args, i);
	XtAddCallback(w, XtNselect, ChangeLabelCallback, NULL);	

	i = 0;
	XtSetArg(args[i], XtNlabel, "Little");	i++;
	w = XtCreateManagedWidget(	"button1", 
					oblongButtonWidgetClass, 
					control, args, i);
	XtAddCallback(w, XtNselect, ChangeCaptionCallback, NULL);	

	XtRealizeWidget(toplevel);

	XtMainLoop();
}

static void
ChangeLabelCallback (widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	int i;
	Arg	args[5];

	i = 0;
	XtSetArg(args[i], XtNlabel, "Big, long label");	i++;
	XtSetValues(label, args, i);
	XtSetValues(widget, args, i);
}

static void
ChangeCaptionCallback (widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	int i;
	Arg	args[5];

	i = 0;
	XtSetArg(args[i], XtNlabel, "Little one");	i++;
	XtSetValues(label, args, i);
	XtSetValues(widget, args, i);
}
