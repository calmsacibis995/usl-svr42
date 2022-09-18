/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:button/unit_test2.c	1.9"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/ControlAre.h>
#include <Xol/Form.h>
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

XColor red_visual_return;
XColor red_exact_return;
XColor yellow_visual_return;
XColor yellow_exact_return;
XColor blue_visual_return;
XColor blue_exact_return;
XColor green_visual_return;
XColor green_exact_return;


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

printf("DISABLED FOR NOW\n");
goto skip;

	switch (test_case)  {
	case 1:
		printf("testing setvalues of SMALL_SCALE\n");
		n = 0;
		XtSetArg(arg[n], XtNscale, SMALL_SCALE);	n++;

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
		printf("testing setvalues of LARGE_SCALE\n");
		n = 0;
		XtSetArg(arg[n], XtNscale, LARGE_SCALE);	n++;

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
	case 3:
		printf("testing setvalues of EXTRA_LARGE_SCALE\n");
		n = 0;
		XtSetArg(arg[n], XtNscale, EXTRA_LARGE_SCALE);	n++;

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
	case 4:
		printf("testing setvalues of MEDIUM_SCALE\n");
		n = 0;
		XtSetArg(arg[n], XtNscale, MEDIUM_SCALE);	n++;

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
	case 5:
		printf("testing setvalues of label_type = OL_POPUP\n");
		n = 0;
		XtSetArg(arg[n], XtNlabelType, OL_POPUP);	n++;

		/*  causes an XtError  */
/*
		XtSetValues(r_normalButton, arg, n);
		XtSetValues(r_setButton, arg, n);
		XtSetValues(r_dimButton, arg, n);
		XtSetValues(r_defaultButton, arg, n);
*/
	
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
	case 6:
		printf("testing setvalues of label_type = OL_STRING\n");
		n = 0;
		XtSetArg(arg[n], XtNlabelType, OL_STRING);	n++;

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
	case 7:
		printf("testing setvalues of busy = TRUE\n");
		n = 0;
		XtSetArg(arg[n], XtNbusy, TRUE);	n++;

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
	case 8:
		printf("testing setvalues of sensitive = FALSE\n");
		n = 0;
		XtSetArg(arg[n], XtNsensitive, FALSE);	n++;

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
	case 9:
		printf("testing setvalues of sensitive = TRUE (except inactive buttons\n");
		n = 0;
		XtSetArg(arg[n], XtNsensitive, TRUE);	n++;

		XtSetValues(r_normalButton, arg, n);
		XtSetValues(r_setButton, arg, n);
		XtSetValues(r_dimButton, arg, n);
		XtSetValues(r_defaultButton, arg, n);
	
		XtSetValues(o_normalButton, arg, n);
		XtSetValues(o_setButton, arg, n);
		XtSetValues(o_defButton, arg, n);
		XtSetValues(o_busyButton, arg, n);
	
		XtSetValues(os_normalButton, arg, n);
		XtSetValues(os_setButton, arg, n);
		XtSetValues(os_defButton, arg, n);
		XtSetValues(os_busyButton, arg, n);
	
		XtSetValues(ohs_normalButton, arg, n);
		XtSetValues(ohs_setButton, arg, n);
		XtSetValues(ohs_defButton, arg, n);
		XtSetValues(ohs_busyButton, arg, n);
		break;
	case 10:
		printf("testing setvalues of busy = FALSE (except busy buttons)\n");
		n = 0;
		XtSetArg(arg[n], XtNbusy, FALSE);	n++;

		XtSetValues(r_normalButton, arg, n);
		XtSetValues(r_setButton, arg, n);
		XtSetValues(r_dimButton, arg, n);
		XtSetValues(r_defaultButton, arg, n);
	
		XtSetValues(o_normalButton, arg, n);
		XtSetValues(o_setButton, arg, n);
		XtSetValues(o_defButton, arg, n);
		XtSetValues(o_inactiveButton, arg, n);
	
		XtSetValues(os_normalButton, arg, n);
		XtSetValues(os_setButton, arg, n);
		XtSetValues(os_defButton, arg, n);
		XtSetValues(os_inactiveButton, arg, n);
	
		XtSetValues(ohs_normalButton, arg, n);
		XtSetValues(ohs_setButton, arg, n);
		XtSetValues(ohs_defButton, arg, n);
		XtSetValues(ohs_inactiveButton, arg, n);
		break;
	case 11:
		printf("testing setvalues of default = TRUE\n");
		n = 0;
		XtSetArg(arg[n], XtNdefault, TRUE);	n++;

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
	case 12:
		printf("testing setvalues of set = TRUE\n");
		n = 0;
		XtSetArg(arg[n], XtNset, TRUE);	n++;

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
	case 13:
		printf("testing setvalues of dim = TRUE\n");
		n = 0;
		XtSetArg(arg[n], XtNdim, TRUE);	n++;

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
	case 14:
		printf("testing setvalues of set = FALSE(except set buttons)\n");
		n = 0;
		XtSetArg(arg[n], XtNset, FALSE);	n++;

		XtSetValues(r_normalButton, arg, n);
		XtSetValues(r_dimButton, arg, n);
		XtSetValues(r_defaultButton, arg, n);
	
		XtSetValues(o_normalButton, arg, n);
		XtSetValues(o_defButton, arg, n);
		XtSetValues(o_inactiveButton, arg, n);
		XtSetValues(o_busyButton, arg, n);
	
		XtSetValues(os_normalButton, arg, n);
		XtSetValues(os_defButton, arg, n);
		XtSetValues(os_inactiveButton, arg, n);
		XtSetValues(os_busyButton, arg, n);
	
		XtSetValues(ohs_normalButton, arg, n);
		XtSetValues(ohs_defButton, arg, n);
		XtSetValues(ohs_inactiveButton, arg, n);
		XtSetValues(ohs_busyButton, arg, n);
		break;
	case 15:
		printf("testing setvalues of default = FALSE(except default buttons)\n");
		n = 0;
		XtSetArg(arg[n], XtNdefault, FALSE);	n++;

		XtSetValues(r_normalButton, arg, n);
		XtSetValues(r_setButton, arg, n);
		XtSetValues(r_dimButton, arg, n);
	
		XtSetValues(o_normalButton, arg, n);
		XtSetValues(o_setButton, arg, n);
		XtSetValues(o_inactiveButton, arg, n);
		XtSetValues(o_busyButton, arg, n);
	
		XtSetValues(os_normalButton, arg, n);
		XtSetValues(os_setButton, arg, n);
		XtSetValues(os_inactiveButton, arg, n);
		XtSetValues(os_busyButton, arg, n);
	
		XtSetValues(ohs_normalButton, arg, n);
		XtSetValues(ohs_setButton, arg, n);
		XtSetValues(ohs_inactiveButton, arg, n);
		XtSetValues(ohs_busyButton, arg, n);
		break;
	case 16:
		printf("testing setvalues of dim = FALSE (except dim buttons)\n");
		n = 0;
		XtSetArg(arg[n], XtNdim, FALSE);	n++;

		XtSetValues(r_normalButton, arg, n);
		XtSetValues(r_setButton, arg, n);
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
skip:{}
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


	n=0;
	XtSetArg(arg[n], XtNlayoutType, OL_FIXEDCOLS); n++;
	XtSetArg(arg[n], XtNmeasure, 3); n++;
	box = XtCreateManagedWidget("box",
		controlAreaWidgetClass,
		toplevel,
		arg,
		n);


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

	XAllocNamedColor(XtDisplay(box),
		DefaultColormap(XtDisplay(box), 0),
		"red",
		&red_visual_return,
		&red_exact_return);
	XAllocNamedColor(XtDisplay(box),
		DefaultColormap(XtDisplay(box), 0),
		"yellow",
		&yellow_visual_return,
		&yellow_exact_return);
	XAllocNamedColor(XtDisplay(box),
		DefaultColormap(XtDisplay(box), 0),
		"blue",
		&blue_visual_return,
		&blue_exact_return);
	XAllocNamedColor(XtDisplay(box),
		DefaultColormap(XtDisplay(box), 0),
		"green",
		&green_visual_return,
		&green_exact_return);


	/*
	 *  Create the rectangular buttons
	 */
	n = 0;
	r_normalButton = XtCreateManagedWidget("Normal Rectangle",
		rectButtonWidgetClass,
		box,
		arg,
		n);


	n = 0;
	XtSetArg(arg[n], XtNset, TRUE);		n++;
	r_setButton = XtCreateManagedWidget("Set Rectangle",
		rectButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNdim, TRUE);		n++;
	r_dimButton = XtCreateManagedWidget("Dim Rectangle",
		rectButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNdefault, TRUE);		n++;
	r_defaultButton = XtCreateManagedWidget("Default Rectangle",
		rectButtonWidgetClass,
		box,
		arg,
		n);


	/*
	 *  Create the oblong buttons
	 */
	n = 0;
	o_normalButton = XtCreateManagedWidget("Normal Oblong",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNset, TRUE);		n++;
	o_setButton = XtCreateManagedWidget("Set Oblong",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNdefault, TRUE);		n++;
	o_defButton = XtCreateManagedWidget("Default Oblong",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNsensitive, FALSE);		n++;
	o_inactiveButton = XtCreateManagedWidget("Inactive Oblong",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNbusy, TRUE);		n++;
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
	os_normalButton = XtCreateManagedWidget("Normal Stack",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNbuttonType, OL_BUTTONSTACK);	n++;
	XtSetArg(arg[n], XtNset, TRUE);		n++;
	os_setButton = XtCreateManagedWidget("Set Stack",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNbuttonType, OL_BUTTONSTACK);	n++;
	XtSetArg(arg[n], XtNdefault, TRUE);		n++;
	os_defButton = XtCreateManagedWidget("Default Stack",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNbuttonType, OL_BUTTONSTACK);	n++;
	XtSetArg(arg[n], XtNsensitive, FALSE);		n++;
	os_inactiveButton = XtCreateManagedWidget("Inactive Stack",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNbuttonType, OL_BUTTONSTACK);	n++;
	XtSetArg(arg[n], XtNbusy, TRUE);		n++;
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
	ohs_normalButton = XtCreateManagedWidget("Normal Halfstack",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNbuttonType, OL_HALFSTACK);	n++;
	XtSetArg(arg[n], XtNset, TRUE);		n++;
	ohs_setButton = XtCreateManagedWidget("Set Halfstack",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNbuttonType, OL_HALFSTACK);	n++;
	XtSetArg(arg[n], XtNdefault, TRUE);		n++;
	ohs_defButton = XtCreateManagedWidget("Default Halfstack",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNbuttonType, OL_HALFSTACK);	n++;
	XtSetArg(arg[n], XtNsensitive, FALSE);		n++;
	ohs_inactiveButton = XtCreateManagedWidget("Inactive Halfstack",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNbuttonType, OL_HALFSTACK);	n++;
	XtSetArg(arg[n], XtNbusy, TRUE);		n++;
	ohs_busyButton = XtCreateManagedWidget("Busy Halfstack",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	XtRealizeWidget(toplevel);
	XtMainLoop();
}
