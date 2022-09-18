/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:statictext/unit_test6.c	1.5"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/ControlAre.h>
#include <Xol/OpenLook.h>
#include <Xol/StaticText.h>
#include <Xol/OblongButt.h>

static char string0[] = "Center";
static char string1[] = "North";
static char string2[] = "South";
static char string3[] = "East";
static char string4[] = "West";
static char string5[] = "NorthWest";
static char string6[] = "NorthEast";
static char string7[] = "SouthWest";
static char string8[] = "SouthEast";

Widget testButton;
Widget staticText;

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
		n = 0;
		XtSetArg(arg[n], XtNgravity, NorthGravity);	n++;
		XtSetArg(arg[n], XtNstring, string1);	n++;

		XtSetValues(staticText, arg, n);
		break;
	case 2:
		n = 0;
		XtSetArg(arg[n], XtNgravity, SouthGravity);	n++;
		XtSetArg(arg[n], XtNstring, string2);	n++;

		XtSetValues(staticText, arg, n);
		break;
	case 3:
		n = 0;
		XtSetArg(arg[n], XtNgravity, EastGravity);	n++;
		XtSetArg(arg[n], XtNstring, string3);	n++;

		XtSetValues(staticText, arg, n);
		break;
	case 4:
		n = 0;
		XtSetArg(arg[n], XtNgravity, WestGravity);	n++;
		XtSetArg(arg[n], XtNstring, string4);	n++;

		XtSetValues(staticText, arg, n);
		break;
	case 5:
		n = 0;
		XtSetArg(arg[n], XtNgravity, NorthWestGravity);	n++;
		XtSetArg(arg[n], XtNstring, string5);	n++;

		XtSetValues(staticText, arg, n);
		break;
	case 6:
		n = 0;
		XtSetArg(arg[n], XtNgravity, NorthEastGravity);	n++;
		XtSetArg(arg[n], XtNstring, string6);	n++;

		XtSetValues(staticText, arg, n);
		break;
	case 7:
		n = 0;
		XtSetArg(arg[n], XtNgravity, SouthWestGravity);	n++;
		XtSetArg(arg[n], XtNstring, string7);	n++;

		XtSetValues(staticText, arg, n);
		break;
	case 8:
		n = 0;
		XtSetArg(arg[n], XtNgravity, SouthEastGravity);	n++;
		XtSetArg(arg[n], XtNstring, string8);	n++;

		XtSetValues(staticText, arg, n);
		break;
	case 9:
		n = 0;
		XtSetArg(arg[n], XtNgravity, CenterGravity);	n++;
		XtSetArg(arg[n], XtNstring, string0);	n++;

		XtSetValues(staticText, arg, n);
		test_case = 0;
		break;
	}

}


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

	testButton = XtCreateManagedWidget("testButton",
		oblongButtonWidgetClass,
		box,
		NULL,
		0);
	XtAddCallback(testButton, XtNselect, testCallback, NULL);	


/*
 *  OLXTK-31.13
 */

	n = 0;
	XtSetArg(arg[n], XtNstring, string0);			n++;
	XtSetArg(arg[n], XtNwidth, 100);			n++;
	XtSetArg(arg[n], XtNheight, 100);			n++;
	XtSetArg(arg[n], XtNgravity, CenterGravity);		n++;
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
	staticText = XtCreateManagedWidget("Static Text",
		staticTextWidgetClass,
		box,
		arg,
		n);

	XtRealizeWidget(toplevel);
	XtMainLoop();
}
