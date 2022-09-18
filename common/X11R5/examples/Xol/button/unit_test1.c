/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:button/unit_test1.c	1.12"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/ControlAre.h>
#include <Xol/OpenLookP.h>
#include <Xol/RectButton.h>
#include <Xol/OblongButt.h>
#include <Xol/CheckBox.h>


Widget box;
Widget setButton;
Widget dimButton;
Widget defaultButton;
Widget imageButton;
Widget oblongButton;
Widget stayupButton;
Widget halfButton;
Widget popupButton;
Widget sensitiveButton;
Widget odefButton;
Widget osetButton;
XColor red_visual_return;
XColor red_exact_return;
XColor yellow_visual_return;
XColor yellow_exact_return;
XColor blue_visual_return;
XColor blue_exact_return;
XColor green_visual_return;
XColor green_exact_return;

#define pushpin_width 32
#define pushpin_height 12
static char pushpin_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x80, 0x43, 0x00,
   0x00, 0x40, 0x81, 0x00, 0x00, 0x20, 0x81, 0x00, 0x00, 0x20, 0x43, 0x00,
   0x00, 0x20, 0x7e, 0x00, 0x00, 0x60, 0x40, 0x00, 0x00, 0xc0, 0x30, 0x00,
   0x00, 0x60, 0x0f, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static char defCascadeData[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x03, 0x00, 0x06, 0x00, 0x0c, 0x00, 0x18,
   0xff, 0x3f, 0x00, 0x18, 0x00, 0x0c, 0x00, 0x06,
   0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void core_to_labelCB(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	static int count;
	Arg arg;
	char *label="corename->label";

	if(count==2) count=0;			/* recycle */
	if(count==0) {
		XtSetArg(arg,XtNlabel,(XtArgVal) label);
	}
	else {
		XtSetArg(arg,XtNlabel,(XtArgVal) NULL);
	}

	XtSetValues(widget,&arg,1);
	count++;
}

void popchangeCB(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	static int count;
	Arg arg;
	char *label0="popup-new *";
	char *label1="popup-new ***";
	char *label2="popup-new *****";

	if(count==3) count=0;			/* recycle */
	switch(count) {
		case 0:
		XtSetArg(arg,XtNlabel,(XtArgVal) label0);
		break;
		case 1:
		XtSetArg(arg,XtNlabel,(XtArgVal) label1);
		break;
		case 2:
		XtSetArg(arg,XtNlabel,(XtArgVal) label2);
		break;
	}
	XtSetValues(widget,&arg,1);
	count++;
}

void popcorechCB(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	static int count;
	Arg arg;
	char *label0="core-new *";
	char *label1="core-new ***";
	char *label2="core-new *****";

	if(count==3) count=0;			/* recycle */
	switch(count) {
		case 0:
		XtSetArg(arg,XtNname,(XtArgVal) label0);
		break;
		case 1:
		XtSetArg(arg,XtNname,(XtArgVal) label1);
		break;
		case 2:
		XtSetArg(arg,XtNname,(XtArgVal) label2);
		break;
	}
	XtSetValues(widget,&arg,1);
	count++;
}
void Quitcallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	(void) printf("Called quit button callback.\n");
	exit(0);
}


void Setcallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	Arg arg[20];
	unsigned int n;

	n = 0;
	XtSetArg(arg[n], XtNset, TRUE);		n++;
	XtSetArg(arg[n], XtNlabel, "Set Set");		n++;
	XtSetArg(arg[n], XtNbackground, blue_exact_return.pixel); n++;
	XtSetArg(arg[n], XtNforeground, green_exact_return.pixel); n++;
	XtSetArg(arg[n], XtNcornerColor, yellow_exact_return.pixel); n++;
	XtSetArg(arg[n], XtNwidth, 150); n++;
	XtSetArg(arg[n], XtNlabelJustify, OL_CENTER); n++;
	XtSetValues(odefButton, arg, n);

	n = 0;
	XtSetArg(arg[n], XtNset, TRUE);		n++;
	XtSetArg(arg[n], XtNlabel, "Set Set Set Set Set");		n++;
	XtCallCallbacks(dimButton, XtNselect, NULL);
	XtSetValues(dimButton, arg, n);
	
}


void RDimcallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	Arg arg[20];
	unsigned int n;
	int foo;

	n = 0;

	XtSetArg(arg[n], XtNbackground, red_exact_return.pixel); n++;
	XtSetArg(arg[n], XtNforeground, blue_exact_return.pixel); n++;
/*
	XtSetArg(arg[n], XtNlabel, "trigger label"); n++;
*/
	XtSetValues(widget, arg, n);

}

void Dimcallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	Arg arg[20];
	unsigned int n;
	int foo;

	n = 0;
	XtSetArg(arg[n], XtNbackground, red_exact_return.pixel); n++;
	XtSetArg(arg[n], XtNforeground, blue_exact_return.pixel); n++;
	XtSetValues(sensitiveButton, arg, n);

}


void Defaultcallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	Arg arg[20];
	unsigned int n;

	n = 0;
	XtSetArg(arg[n], XtNset, FALSE);		n++;
	XtSetArg(arg[n], XtNbackground, green_exact_return.pixel); n++;
	XtSetArg(arg[n], XtNforeground, red_exact_return.pixel); n++;
	XtSetArg(arg[n], XtNlabelJustify, OL_LEFT); n++;
	XtSetArg(arg[n], XtNwidth, 70); n++;
/*
	XtSetArg(arg[n], XtNscale, 14); n++;
*/
	XtSetArg(arg[n], XtNtrigger, TRUE); n++;
	XtSetValues(popupButton, arg, n);
	
}

void Previewcallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	Arg arg[20];
	unsigned int n;

	/*
	 *  This setvalues will cause the rectangular defaultButton
	 *  to display itself in the oblong half stack.  It is the
	 *  menu managers job to redisplay the half stack with a
	 *  full Redisplay call - this code does not and leaves an
	 *  ugly image when the preview is pressed.
	 */
	n = 0;
	XtSetArg(arg[n], XtNpreview, halfButton);		n++;
	XtSetValues(defaultButton, arg, n);
	
	XtSetArg(arg[n], XtNbackground, yellow_exact_return.pixel); n++;
	XtSetValues(box, arg, n);
}

int test_case = 0;

void testcallback(widget, clientData, callData)
Widget widget;
caddr_t clientData, callData;
{
	Arg arg[20];
	unsigned int n;

	n = 0;

	test_case++;

	switch (test_case)  {
	case 1:
		printf("testing setvalues of SMALL_SCALE\n");
		break;
	case 2:
		printf("testing setvalues of MEDIUM_SCALE\n");
		break;
	default:
		test_case = 0;
		break;
	}
}

#define NONMENUMODE (XtArgVal) BaseWindow 
#define MENUMODE (XtArgVal)    PressDragReleaseMenu


int main(argc, argv)
unsigned int argc;
char **argv;
{

	Widget toplevel, quitButton, button2, button;
	Arg arg[20];
	unsigned int n;
	XImage *my_image;
	Display *display;

	toplevel = OlInitialize("quitButton",
		"QuitButton",
		NULL,
		0,
		&argc,
		argv);

	XAllocNamedColor(XtDisplay(toplevel),
		DefaultColormap(XtDisplay(toplevel), 0),
		"red",
		&red_visual_return,
		&red_exact_return);
	XAllocNamedColor(XtDisplay(toplevel),
		 DefaultColormap(XtDisplay(toplevel), 0),
		"yellow",
		&yellow_visual_return,
		&yellow_exact_return);
	XAllocNamedColor(XtDisplay(toplevel),
		 DefaultColormap(XtDisplay(toplevel), 0),
		"blue",
		&blue_visual_return,
		&blue_exact_return);
	XAllocNamedColor(XtDisplay(toplevel),
		DefaultColormap(XtDisplay(toplevel), 0),
		"green",
		&green_visual_return,
		&green_exact_return);

	n = 0;
	XtSetArg(arg[n], XtNbackground, green_exact_return.pixel); n++;
	XtSetArg(arg[n], XtNlayoutType, OL_FIXEDCOLS); n++;
	XtSetArg(arg[n], XtNmeasure, 3); n++;
	box = XtCreateManagedWidget("box",
		controlAreaWidgetClass,
		toplevel,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNlabel, "Quit Button");		n++;
	quitButton = XtCreateManagedWidget("button",
		rectButtonWidgetClass,
		box,
		arg,
		n);
	XtAddCallback(quitButton, XtNselect, Quitcallback, NULL);	

	n = 0;
	XtSetArg(arg[n], XtNlabel, "No callbacks");		n++;
	button=XtCreateManagedWidget("button",
		rectButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNlabel, "Set Button");		n++;
	XtSetArg(arg[n], XtNset, TRUE);		n++;
/*
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
*/
	setButton = XtCreateManagedWidget("button",
		rectButtonWidgetClass,
		box,
		arg,
		n);
	XtAddCallback(setButton, XtNselect, Setcallback, NULL);


	n = 0;
	XtSetArg(arg[n], XtNlabel, "Dim Button");		n++;
	XtSetArg(arg[n], XtNset, TRUE);		n++;
	XtSetArg(arg[n], XtNdim, TRUE);		n++;
	dimButton = XtCreateManagedWidget("button",
		rectButtonWidgetClass,
		box,
		arg,
		n);
	XtAddCallback(dimButton, XtNselect, RDimcallback, NULL);

/*
	n = 0;
	XtSetArg(arg[n], XtNlabel, "Image Button");		n++;
	XtSetArg(arg[n], XtNlabelType, OL_IMAGE);		n++;
	XtSetArg(arg[n], XtNlabelImage, t_colorimg(XtDisplay(toplevel), 32, 32));		n++;
	imageButton = XtCreateManagedWidget("button",
		rectButtonWidgetClass,
		box,
		arg,
		n);
*/

	n = 0;
	XtSetArg(arg[n], XtNlabel, "Default Button");		n++;
	XtSetArg(arg[n], XtNdefault, TRUE);		n++;
/*
	XtSetArg(arg[n], XtNrecomputeSize, FALSE);		n++;
*/
	defaultButton = XtCreateManagedWidget("button",
		rectButtonWidgetClass,
		box,
		arg,
		n);
	XtAddCallback(defaultButton, XtNselect, Defaultcallback, NULL);

	n = 0;
	XtSetArg(arg[n], XtNlabel, "Quit");		n++;
	oblongButton = XtCreateManagedWidget("button",
		oblongButtonWidgetClass,
		box,
		arg,
		n);
	XtAddCallback(oblongButton, XtNselect, Quitcallback, NULL);	

	n = 0;
	XtSetArg(arg[n], XtNshellBehavior, MENUMODE);	n++;
	XtSetArg(arg[n], XtNlabel, "Cut - Gadget");		n++;
	XtSetArg(arg[n], XtNset, TRUE);		n++;
	XtSetArg(arg[n], XtNlabelJustify, OL_LEFT);		n++;
	osetButton = XtCreateManagedWidget("button",
		oblongButtonGadgetClass,
		box,
		arg,
		n);
	XtAddCallback(osetButton, XtNselect, Setcallback, NULL);

	n = 0;
	XtSetArg(arg[n], XtNshellBehavior, NONMENUMODE);	n++;
	XtSetArg(arg[n], XtNlabel, "Cut");		n++;
	XtSetArg(arg[n], XtNlabelJustify, OL_CENTER);		n++;
	XtSetArg(arg[n], XtNset, TRUE);		n++;
	osetButton = XtCreateManagedWidget("button",
		oblongButtonWidgetClass,
		box,
		arg,
		n);
	XtAddCallback(osetButton, XtNselect, Setcallback, NULL);

	n = 0;
	XtSetArg(arg[n], XtNshellBehavior, NONMENUMODE);	n++;
	XtSetArg(arg[n], XtNbuttonType, OL_BUTTONSTACK);	n++;
	XtSetArg(arg[n], XtNlabel, "Edit"); n++;
	XtSetArg(arg[n], XtNlabelJustify, OL_CENTER);		n++;
	XtSetArg(arg[n], XtNset, TRUE);		n++;
	stayupButton = XtCreateManagedWidget("button",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	XtSetArg(arg[n], XtNshellBehavior, MENUMODE);	n++;
	XtSetArg(arg[n], XtNbuttonType, OL_BUTTONSTACK);	n++;
	XtSetArg(arg[n], XtNlabel, "Edit");		n++;
	XtSetArg(arg[n], XtNlabelJustify, OL_LEFT);		n++;
	XtSetArg(arg[n], XtNset, TRUE);		n++;
	stayupButton = XtCreateManagedWidget("button",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNshellBehavior, MENUMODE);	n++;
	XtSetArg(arg[n], XtNbuttonType, OL_BUTTONSTACK);	n++;
	XtSetArg(arg[n], XtNlabelJustify, OL_LEFT); n++;
	XtSetArg(arg[n], XtNlabel, "Button Stack");		n++;
	halfButton = XtCreateManagedWidget("button",
		oblongButtonWidgetClass,
		box,
		arg,
		n);
	XtAddCallback(halfButton, XtNselect, Defaultcallback, NULL);

	n = 0;
	XtSetArg(arg[n], XtNlabel, "Cut");		n++;
	XtSetArg(arg[n], XtNdefault, TRUE);		n++;
	odefButton = XtCreateManagedWidget("button",
		oblongButtonWidgetClass,
		box,
		arg,
		n);
	XtAddCallback(odefButton, XtNselect, Dimcallback, NULL);

	n = 0;
	XtSetArg(arg[n], XtNlabel, "Cut");		n++;
	XtSetArg(arg[n], XtNsensitive, FALSE);		n++;
	sensitiveButton = XtCreateManagedWidget("button",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNlabel, "Cut");		n++;
	XtSetArg(arg[n], XtNbusy, TRUE);		n++;
	stayupButton = XtCreateManagedWidget("button",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	n = 0;
	XtSetArg(arg[n], XtNlabelType, OL_POPUP);	n++;
	XtSetArg(arg[n], XtNlabelJustify, OL_CENTER); n++;
	XtSetArg(arg[n], XtNlabel, "nonmenuC: popup");		n++;
	XtSetArg(arg[n], XtNcornerColor, blue_exact_return.pixel);  n++;
	popupButton = XtCreateManagedWidget("button",
		oblongButtonWidgetClass,
		box,
		arg,
		n);
	XtAddCallback(popupButton, XtNselect, Previewcallback, NULL);	

	n = 0;
	XtSetArg(arg[n], XtNlabelType, OL_POPUP);	n++;
	XtSetArg(arg[n], XtNlabelJustify, OL_LEFT); n++;
	XtSetArg(arg[n], XtNlabel, "nonmenuL: popup");		n++;
	XtSetArg(arg[n], XtNcornerColor, blue_exact_return.pixel);  n++;
	popupButton = XtCreateManagedWidget("button",
		oblongButtonWidgetClass,
		box,
		arg,
		n);
	XtAddCallback(popupButton, XtNselect, Previewcallback, NULL);	

	n = 0;
	XtSetArg(arg[n], XtNshellBehavior, MENUMODE);	n++;
	XtSetArg(arg[n], XtNlabelType, OL_POPUP);	n++;
	XtSetArg(arg[n], XtNlabelJustify, OL_CENTER); n++;
	XtSetArg(arg[n], XtNlabel, "menuC: popup");		n++;
	XtSetArg(arg[n], XtNcornerColor, blue_exact_return.pixel);  n++;
	popupButton = XtCreateManagedWidget("button",
		oblongButtonWidgetClass,
		box,
		arg,
		n);
	XtAddCallback(popupButton, XtNselect, Previewcallback, NULL);	
	
	n = 0;
	XtSetArg(arg[n], XtNshellBehavior, MENUMODE);	n++;
	XtSetArg(arg[n], XtNlabelType, OL_POPUP);	n++;
	XtSetArg(arg[n], XtNlabelJustify, OL_LEFT); n++;
	XtSetArg(arg[n], XtNlabel, "menuL: popup");		n++;
	XtSetArg(arg[n], XtNcornerColor, blue_exact_return.pixel);  n++;
	popupButton = XtCreateManagedWidget("button",
		oblongButtonWidgetClass,
		box,
		arg,
		n);
	XtAddCallback(popupButton, XtNselect, Previewcallback, NULL);	
	
	n = 0;
	XtSetArg(arg[n], XtNlabelType, OL_POPUP);	n++;
	popupButton = XtCreateManagedWidget("core_popup",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	XtAddCallback(popupButton, XtNselect, core_to_labelCB, NULL);	

	n = 0;
	XtSetArg(arg[n], XtNlabelType, OL_POPUP);	n++;
	popupButton = XtCreateManagedWidget("core_popup",
		oblongButtonWidgetClass,
		box,
		arg,
		n);

	XtAddCallback(popupButton, XtNselect, popchangeCB, NULL);	

/*
	display = XtDisplay(toplevel);
	my_image = XCreateImage(display,
		CopyFromParent,
/*		XDefaultVisual(display, XDefaultScreen(display)), 
		1,
		ZPixmap,
		0,
		defCascadeData,
		16,
		16,
		8,
		2);
	my_image->byte_order = MSBFirst;
  	my_image->bitmap_bit_order = LSBFirst;
  	my_image->bitmap_unit = 8;
*/

	n = 0;
	XtSetArg(arg[n], XtNlabelType, OL_POPUP); n++;
	XtSetArg(arg[n], XtNlabel, "popup");	n++;
	popupButton = XtCreateManagedWidget("button",
		oblongButtonWidgetClass,
		box,
		arg,
		n);
	XtAddCallback(popupButton, XtNselect, popchangeCB, NULL);	

	XtRealizeWidget(toplevel);
	XtMainLoop();
}

