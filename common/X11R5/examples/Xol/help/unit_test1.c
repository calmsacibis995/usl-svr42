/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:help/unit_test1.c	1.10"
#endif

/*
 *************************************************************************
 *
 * Date:	March 1989
 *
 * Description:
 *	This file contains a trivial unit test that simulates a help
 *	key press to popup a help window
 *
 ******************************file*header********************************
 */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/OblongButt.h>
#include <Xol/BulletinBo.h>

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *
 **************************forward*declarations***************************
 */

static	void HelpPopUp();

/*
 *************************************************************************
 * main - the driver
 ****************************procedure*header*****************************
 */
main(argc, argv)
int argc;
char **argv;
{
	Arg	args[10];
	int	n;
	Widget	button;
	Widget	toplevel;
	Widget	bboard;
	extern void exit ();

	static char string1[] = "This is Help's text area. \
 The text should be aligned to the left, the words should\
 wrap, and the font should be fixed.";

	static char string2[] = "/etc/passwd";

	toplevel = OlInitialize("test", "test", NULL, NULL, &argc, argv);
	bboard = XtCreateManagedWidget("bboard",
		 bulletinBoardWidgetClass,
		 toplevel,
		 NULL,
		 0);
	n = 0;
	XtSetArg(args[n], XtNlabel, "Popup Help");		n++;
	XtSetArg(args[n], XtNx, 20);		n++;
	XtSetArg(args[n], XtNy, 20);		n++;
	button = XtCreateManagedWidget("button", oblongButtonWidgetClass,
		 bboard, args, n);
	XtAddCallback(button, XtNselect, HelpPopUp, NULL);

	OlRegisterHelp (OL_WIDGET_HELP, bboard, "bboard",
			OL_DISK_SOURCE, string2);
	OlRegisterHelp (OL_WIDGET_HELP, button, "button",
			OL_STRING_SOURCE,string1);

	n = 0;
	XtSetArg(args[n], XtNlabel, "Quit");		n++;
	XtSetArg(args[n], XtNx, 20);		n++;
	XtSetArg(args[n], XtNy, 40);		n++;
	button = XtCreateManagedWidget("button", oblongButtonWidgetClass,
					bboard, args, n);
	OlRegisterHelp (OL_WIDGET_HELP, button, "button",
			OL_STRING_SOURCE, "Press SELECT button here to Quit");
	XtAddCallback(button, XtNselect, exit, NULL);

	XtRealizeWidget(toplevel);

	printf("\n\n This unit test has help registered on the popup button,\n\
 the quit button and the composite that these two buttons sit on.\n\
 The help registered for the composite is the \"/etc/passwd\" file.\n");

	XtMainLoop();
} /* END OF main() */

/*
 *************************************************************************
 * HelpPopUp - this routine pops up the help window without the aid of
 * a key press.  Very few applications need to do this.
 ****************************procedure*header*****************************
 */
static void
HelpPopUp(w,client_data,call_data)
	Widget	w;
	caddr_t	client_data;
	caddr_t	call_data;
{
	XEvent		xevent;
	Widget		shell;
	Window		root, child;
	int		root_x, root_y;
	int		window_x, window_y;
	unsigned int	mask;

	shell = _OlGetShellOfWidget(w);

	XQueryPointer(XtDisplay(w), XtWindow(w), &root, &child,
			&root_x, &root_y, &window_x, &window_y, &mask);

	xevent.xclient.type = ClientMessage;
	xevent.xclient.display = XtDisplay(w);
	xevent.xclient.window = XtWindow(shell);
	xevent.xclient.message_type = XA_OL_HELP_KEY(XtDisplay(w));
	xevent.xclient.format = 32;
	xevent.xclient.data.l[0] = XtWindow(w);
	xevent.xclient.data.l[1] = window_x;
	xevent.xclient.data.l[2] = window_y;
	xevent.xclient.data.l[3] = root_x;
	xevent.xclient.data.l[4] = root_y;

	_OlPopupHelpTree(shell, (caddr_t) NULL, &xevent);

} /* END OF HelpPopUp() */
