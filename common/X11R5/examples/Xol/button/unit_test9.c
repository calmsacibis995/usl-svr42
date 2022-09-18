/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:button/unit_test9.c	1.5"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/ControlAre.h>
#include <Xol/OpenLook.h>
#include <Xol/RectButton.h>
#include <Xol/OblongButt.h>

Widget r_normalButton;
Widget r_setButton;
Widget r_dimButton;
Widget r_defaultButton;

Widget o_normalButton;
Widget o_setButton;
Widget o_defButton;
Widget o_inactiveButton;
Widget o_busyButton;

Widget os_normalButton;
Widget os_setButton;
Widget os_defButton;
Widget os_inactiveButton;
Widget os_busyButton;

Widget ohs_normalButton;
Widget ohs_setButton;
Widget ohs_defButton;
Widget ohs_inactiveButton;
Widget ohs_busyButton;

Widget quitButton;
Widget testButton;



void Quitcallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	exit(0);
}

int test_case = 0;

void Testcallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	Arg arg[20];
	unsigned int n;

	n = 0;

	test_case++;

	switch (test_case)  {
	case 1:
		printf("testing setvalues of justify OL_LEFT\n");
		n = 0;
		XtSetArg(arg[n], XtNlabelJustify, OL_LEFT);	n++;

		XtSetValues(r_normalButton, arg, n);
		XtSetValues(r_setButton, arg, n);
		XtSetValues(r_dimButton, arg, n);
		XtSetValues(r_defaultButton, arg, n);
	
		XtSetValues(o_normalButton, arg, n);
		XtSetValues(o_setButton, arg, n);
		XtSetValues(o_defButton, arg, n);
		XtSetValues(o_inactiveButton, arg, n);
		XtSetValues(o_busyButton, arg, n);
	
		XtSetValues(os_normalButton, arg, n);
		XtSetValues(os_setButton, arg, n);
		XtSetValues(os_defButton, arg, n);
		XtSetValues(os_inactiveButton, arg, n);
		XtSetValues(os_busyButton, arg, n);
	
		XtSetValues(ohs_normalButton, arg, n);
		XtSetValues(ohs_setButton, arg, n);
		XtSetValues(ohs_defButton, arg, n);
		XtSetValues(ohs_inactiveButton, arg, n);
		XtSetValues(ohs_busyButton, arg, n);
		break;
	case 2:
		printf("testing setvalues of justify OL_CENTER\n");
		n = 0;
		XtSetArg(arg[n], XtNlabelJustify, OL_CENTER);	n++;

		XtSetValues(r_normalButton, arg, n);
		XtSetValues(r_setButton, arg, n);
		XtSetValues(r_dimButton, arg, n);
		XtSetValues(r_defaultButton, arg, n);
	
		XtSetValues(o_normalButton, arg, n);
		XtSetValues(o_setButton, arg, n);
		XtSetValues(o_defButton, arg, n);
		XtSetValues(o_inactiveButton, arg, n);
		XtSetValues(o_busyButton, arg, n);
	
		XtSetValues(os_normalButton, arg, n);
		XtSetValues(os_setButton, arg, n);
		XtSetValues(os_defButton, arg, n);
		XtSetValues(os_inactiveButton, arg, n);
		XtSetValues(os_busyButton, arg, n);
	
		XtSetValues(ohs_normalButton, arg, n);
		XtSetValues(ohs_setButton, arg, n);
		XtSetValues(ohs_defButton, arg, n);
		XtSetValues(ohs_inactiveButton, arg, n);
		XtSetValues(ohs_busyButton, arg, n);
		break;
	default:
		printf("That's all the tests.\n");
		test_case = 0;
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

	toplevel = OlInitialize("toplevel",
		"QuitButton",
		NULL,
		0,
		&argc,
		argv);

	box = XtCreateManagedWidget("box",
		controlAreaWidgetClass,
		toplevel,
		NULL,
		0);

	/*
	 *  Create the Quit button and add the callback
	 */
	n = 0;
	quitButton = XtCreateManagedWidget("Quit",
		oblongButtonWidgetClass,
		box,
		arg,
		n);
	XtAddCallback(quitButton, XtNselect, Quitcallback, NULL);	

	/*
	 *  Create the Test button and add the callback
	 */
	n = 0;
	testButton = XtCreateManagedWidget("Test",
		oblongButtonWidgetClass,
		box,
		arg,
		n);
	XtAddCallback(testButton, XtNselect, Testcallback, NULL);	


	/*
	 *  Create the rectangular buttons
	 */
	n = 0;
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	r_normalButton = XtCreateManagedWidget("Normal Rectangle",
		rectButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNset, TRUE);		n++;
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	r_setButton = XtCreateManagedWidget("Set Rectangle",
		rectButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNdim, TRUE);		n++;
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	r_dimButton = XtCreateManagedWidget("Dim Rectangle",
		rectButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNdefault, TRUE);		n++;
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	r_defaultButton = XtCreateManagedWidget("Default Rectangle",
		rectButtonWidgetClass,
		box,
		arg,
		n);

	/*
	 *  Create the oblong buttons
	 */
	n = 0;
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	o_normalButton = XtCreateManagedWidget("Normal Oblong",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNset, TRUE);		n++;
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	o_setButton = XtCreateManagedWidget("Set Oblong",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNdefault, TRUE);		n++;
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	o_defButton = XtCreateManagedWidget("Default Oblong",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNsensitive, FALSE);		n++;
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	o_inactiveButton = XtCreateManagedWidget("Inactive Oblong",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNbusy, TRUE);		n++;
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	o_busyButton = XtCreateManagedWidget("Busy Oblong",
		oblongButtonWidgetClass,
		box,
		arg,
		n);


	/*
	 *  Create the oblong stack buttons
	 */
	n = 0;
	XtSetArg(arg[n], XtNbuttonType, OL_BUTTONSTACK);	n++;
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	os_normalButton = XtCreateManagedWidget("Normal Stack",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNbuttonType, OL_BUTTONSTACK);	n++;
	XtSetArg(arg[n], XtNset, TRUE);		n++;
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	os_setButton = XtCreateManagedWidget("Set Stack",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNbuttonType, OL_BUTTONSTACK);	n++;
	XtSetArg(arg[n], XtNdefault, TRUE);		n++;
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	os_defButton = XtCreateManagedWidget("Default Stack",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNbuttonType, OL_BUTTONSTACK);	n++;
	XtSetArg(arg[n], XtNsensitive, FALSE);		n++;
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	os_inactiveButton = XtCreateManagedWidget("Inactive Stack",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNbuttonType, OL_BUTTONSTACK);	n++;
	XtSetArg(arg[n], XtNbusy, TRUE);		n++;
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	os_busyButton = XtCreateManagedWidget("Busy Stack",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	/*
	 *  Create the half stack buttons
	 */
	n = 0;
	XtSetArg(arg[n], XtNbuttonType, OL_HALFSTACK);	n++;
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	ohs_normalButton = XtCreateManagedWidget("Normal Halfstack",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNbuttonType, OL_HALFSTACK);	n++;
	XtSetArg(arg[n], XtNset, TRUE);		n++;
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	ohs_setButton = XtCreateManagedWidget("Set Halfstack",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNbuttonType, OL_HALFSTACK);	n++;
	XtSetArg(arg[n], XtNdefault, TRUE);		n++;
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	ohs_defButton = XtCreateManagedWidget("Default Halfstack",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNbuttonType, OL_HALFSTACK);	n++;
	XtSetArg(arg[n], XtNsensitive, FALSE);		n++;
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	ohs_inactiveButton = XtCreateManagedWidget("Inactive Halfstack",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNbuttonType, OL_HALFSTACK);	n++;
	XtSetArg(arg[n], XtNbusy, TRUE);		n++;
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
	XtSetArg(arg[n], XtNwidth, 200);			n++;
	ohs_busyButton = XtCreateManagedWidget("Busy Halfstack",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	XtRealizeWidget(toplevel);
	XtMainLoop();
}

