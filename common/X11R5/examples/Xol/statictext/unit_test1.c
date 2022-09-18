/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:statictext/unit_test1.c	1.9"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/StaticText.h>
#include <Xol/ControlAre.h>
#include <Xol/OblongButt.h>

/*
** unit_test1.  Basic sanity checking and dynamically-changing resources.
*/

char string1[] = "'Twas brillig, and the slithy toves\nDid gyre and gymbal in \
the wabe\nAll mimsey were the borogoves\n\
And the mome raths outgrabe.\n-Lewis Carroll";

Widget	statictext;
static int height = 75;
static int width = 250;
static int line_space = 0;
static int alignment = 0;
static int gravity = 0;
static int resizable = 1;

static void
HeightCallback(w,client_data,call_data)
Widget w;
caddr_t client_data, call_data;
{
	Arg arg[5];
	unsigned int n;
	
	height += (int) client_data;
	printf ("height is %d\n",height);

	n = 0;
	XtSetArg(arg[n], XtNheight, height);		n++;
	XtSetValues (statictext, arg, n);
}

static void
ResizableCallback(w,client_data,call_data)
Widget w;
caddr_t client_data, call_data;
{
	Arg arg[5];
	unsigned int n;

	resizable = (resizable ? 0 : 1);
	
	printf ("RecomputeSize is %s\n",resizable ? "on" : "off");

	n = 0;
	XtSetArg(arg[n], XtNrecomputeSize, resizable);		n++;
	XtSetValues (statictext, arg, n);
}


static void
WidthCallback(w,client_data,call_data)
Widget w;
caddr_t client_data, call_data;
{
	Arg arg[5];
	unsigned int n;
	
	width += (int) client_data;
	printf ("Width is %d\n",width);
	n = 0;
	XtSetArg(arg[n], XtNwidth, width);		n++;
	XtSetValues (statictext, arg, n);
}

static void
AlignmentCallback(w,client_data,call_data)
Widget w;
caddr_t client_data, call_data;
{
	Arg arg[5];
	unsigned int n;
	
	alignment++;
	alignment %= 3;
	n = 0;

	switch (alignment) {
	case 0:
		printf ("Alignment is OL_LEFT\n");
		XtSetArg(arg[n], XtNalignment, OL_LEFT);	n++;
		break;
	case 1:
		printf ("Alignment is OL_CENTER\n");
		XtSetArg(arg[n], XtNalignment, OL_CENTER);	n++;
		break;
	case 2:
		printf ("Alignment is OL_RIGHT\n");
		XtSetArg(arg[n], XtNalignment, OL_RIGHT);	n++;
		break;
	}
		
	XtSetValues (statictext, arg, n);
}

static void
GravityCallback(w,client_data,call_data)
Widget w;
caddr_t client_data, call_data;
{
	Arg arg[5];
	unsigned int n;
	
	gravity++;
	gravity %= 3;
	n = 0;

	switch (gravity) {
	case 0:
		printf ("Gravity is CenterGravity\n");
		XtSetArg(arg[n], XtNgravity, CenterGravity);	n++;
		break;
	case 1:
		printf ("Gravity is SouthGravity\n");
		XtSetArg(arg[n], XtNgravity, SouthGravity);	n++;
		break;
	case 2:
		printf ("Gravity is NorthWestGravity\n");
		XtSetArg(arg[n], XtNgravity, NorthWestGravity);	n++;
		break;
	}
		
	XtSetValues (statictext, arg, n);
}

static void
LineSpaceCallback(w,client_data,call_data)
Widget w;
caddr_t client_data, call_data;
{
	Arg arg[5];
	unsigned int n;
	
	line_space += (int) client_data;
	printf ("line_space is %d\n",line_space);
	n = 0;
	XtSetArg(arg[n], XtNlineSpace, line_space);		n++;
	XtSetValues (statictext, arg, n);
}

static void
StringCallback(w,client_data,call_data)
Widget w;
caddr_t client_data, call_data;
{
	Arg arg[5];
	unsigned int n;
	
	n = 0;
	if ((int) client_data == 1) {
		XtSetArg(arg[n], XtNstring, string1);		n++;
	}
	else {
		XtSetArg(arg[n], XtNstring, "");		n++;
	}
	XtSetValues (statictext, arg, n);
}


void 
main (argc, argv)
int argc;
char **argv;
{
	Widget toplevel, box, box1, button;
	Arg arg[20];
	unsigned int n;

	XColor	blue_visual_return;
	XColor	blue_exact_return;
	XColor	yellow_visual_return;
	XColor	yellow_exact_return;
	XColor	green_visual_return;
	XColor	green_exact_return;

	toplevel = OlInitialize("quitButton",
		"QuitButton",
		NULL,
		0,
		&argc,
		argv);

	n = 0;
	XtSetArg(arg[n], XtNlayoutType, OL_FIXEDROWS);	n++;
	XtSetArg(arg[n], XtNmeasure, 1);	n++;
	XtSetArg(arg[n], XtNhSpace, 10);			n++;

	box = XtCreateManagedWidget("box",
		controlAreaWidgetClass,
		toplevel,
		arg,
		n);

	XAllocNamedColor(	XtDisplay(box),
				DefaultColormap(XtDisplay(box), 0),
				"blue",
				&blue_visual_return,
				&blue_exact_return);

	XAllocNamedColor(	XtDisplay(box),
				DefaultColormap(XtDisplay(box), 0),
				"yellow",
				&yellow_visual_return,
				&yellow_exact_return);

	XAllocNamedColor(	XtDisplay(box),
				DefaultColormap(XtDisplay(box), 0),
				"green",
				&green_visual_return,
				&green_exact_return);
	n = 0;
	XtSetArg(arg[n], XtNlayoutType, OL_FIXEDCOLS);	n++;
	XtSetArg(arg[n], XtNmeasure, 1);	n++;

	box1 = XtCreateManagedWidget("box",
		controlAreaWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNborderWidth, 0);		n++;
	XtSetArg(arg[n], XtNheight, height);		n++;
	XtSetArg(arg[n], XtNwidth, width);		n++;
	XtSetArg(arg[n], XtNlineSpace, line_space);	n++;

	XtSetArg(arg[n], XtNforeground, yellow_exact_return.pixel);	n++;
	XtSetArg(arg[n], XtNbackground, green_exact_return.pixel);	n++;
	XtSetArg(arg[n], XtNfontColor, blue_exact_return.pixel);	n++;

	XtSetArg(arg[n], XtNalignment, OL_LEFT);	n++;
	XtSetArg(arg[n],
		XtNfont,
		XLoadQueryFont(XtDisplay(toplevel), "ger-s35"));	n++;

	XtSetArg(arg[n], XtNstring, string1);			n++;
	XtSetArg(arg[n], XtNgravity, WestGravity);	n++;

	statictext = XtCreateManagedWidget("Static Text",
		staticTextWidgetClass,
		box1,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNlayoutType, OL_FIXEDCOLS);	n++;
	XtSetArg(arg[n], XtNmeasure, 2);	n++;

	box = XtCreateManagedWidget("box",
		controlAreaWidgetClass,
		box,
		arg,
		n);

	XtSetArg(arg[0], XtNlabel, "grow width");
	button = XtCreateManagedWidget(	"button1", 
					oblongButtonWidgetClass, 
					box, arg, 1);
	XtAddCallback(button, XtNselect, WidthCallback, 25);

	XtSetArg(arg[0], XtNlabel, "shrink width");
	button = XtCreateManagedWidget(	"button0", 
					oblongButtonWidgetClass, 
					box, arg, 1);
	XtAddCallback(button, XtNselect, WidthCallback, -25);

	XtSetArg(arg[0], XtNlabel, "grow height");
	button = XtCreateManagedWidget(	"button3", 
					oblongButtonWidgetClass, 
					box, arg, 1);
	XtAddCallback(button, XtNselect, HeightCallback, 25);

	XtSetArg(arg[0], XtNlabel, "shrink height");
	button = XtCreateManagedWidget(	"button2", 
					oblongButtonWidgetClass, 
					box, arg, 1);
	XtAddCallback(button, XtNselect, HeightCallback, -25);

	XtSetArg(arg[0], XtNlabel, "grow linespace");
	button = XtCreateManagedWidget(	"button8", 
					oblongButtonWidgetClass, 
					box, arg, 1);
	XtAddCallback(button, XtNselect, LineSpaceCallback, 25);

	XtSetArg(arg[0], XtNlabel, "shrink linespace");
	button = XtCreateManagedWidget(	"button7", 
					oblongButtonWidgetClass, 
					box, arg, 1);
	XtAddCallback(button, XtNselect, LineSpaceCallback, -25);

	XtSetArg(arg[0], XtNlabel, "supply string");
	button = XtCreateManagedWidget(	"button5", 
					oblongButtonWidgetClass, 
					box, arg, 1);
	XtAddCallback(button, XtNselect, StringCallback, 1);

	XtSetArg(arg[0], XtNlabel, "delete string");
	button = XtCreateManagedWidget(	"button6",
					oblongButtonWidgetClass, 
					box, arg, 1);
	XtAddCallback(button, XtNselect, StringCallback, 0);

	XtSetArg(arg[0], XtNlabel, "recompute size");
	button = XtCreateManagedWidget(	"button7",
					oblongButtonWidgetClass, 
					box, arg, 1);
	XtAddCallback(button, XtNselect, ResizableCallback, 0);

	XtSetArg(arg[0], XtNlabel, "alignment");
	button = XtCreateManagedWidget(	"button9",
					oblongButtonWidgetClass, 
					box, arg, 1);
	XtAddCallback(button, XtNselect, AlignmentCallback, 0);

	XtSetArg(arg[0], XtNlabel, "Gravity");
	button = XtCreateManagedWidget(	"button6",
					oblongButtonWidgetClass, 
					box, arg, 1);
	XtAddCallback(button, XtNselect, GravityCallback, 0);

	XtRealizeWidget(toplevel);
	XtMainLoop();
}
