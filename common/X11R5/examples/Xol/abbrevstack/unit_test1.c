/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:abbrevstack/unit_test1.c	1.9"
#endif

/*
 *************************************************************************
 *
 * Description:
 *	This is an Abbreviated Menu Button unit test
 *
 ******************************file*header********************************
 */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/BaseWindow.h>
#include <Xol/AbbrevMenu.h>
#include <Xol/OblongButt.h>
#include <Xol/Exclusives.h>
#include <Xol/RectButton.h>
#include <Xol/ControlAre.h>

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *
 **************************forward*declarations***************************
 */

static void	AddChildren();		/* Adds children to a menu pane	*/
static void	AddExclusives();	/* Adds set of exclusives	*/
static void	AddOblongs();		/* Adds set of oblongs		*/
static void	ChangeColor();		/* Changes color of abbrev.	*/

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define ARG_SIZE 9

static Widget abbrevTopLevel = (Widget) NULL;


/*
 *************************************************************************
 * AddChildren - This routine adds children to the menu pane.
 ****************************procedure*header*****************************
 */
static void
AddChildren(pane)
	Widget pane;				/* The menu pane	*/
{
	AddExclusives(pane);
	AddOblongs(pane);
} /* END OF AddChildren() */

/*
 *************************************************************************
 * AddExclusives - This routine adds a set of exclusives to a menu pane
 ****************************procedure*header*****************************
 */
static void
AddExclusives(pane)
	Widget pane;		/* Exclusives widget	*/
{
	Widget		child;
	static char *	color_names[] = {
			"red",
			"cyan",
			"yellow",
			"orange"
	};
	int		num = sizeof(color_names)/sizeof(color_names[0]);
	int		i;
	Display *	dpy = XtDisplay(pane);
	Colormap	cmap = DefaultColormapOfScreen(XtScreen(pane));
	XColor		color;
	Arg		args[1];
	static Arg	e_args[] = {
			{ XtNmeasure,	2 },
			{ XtNlayoutType, OL_FIXEDROWS }
	};
	Widget		exc = XtCreateManagedWidget("exc",
				exclusivesWidgetClass, pane,
				e_args, XtNumber(e_args));

							/* Add a Child */

	for (i=0; i < num; ++i) {
		if (!XParseColor(dpy, cmap, color_names[i], &color))
			OlError("Parse color failed");
		if (!XAllocColor(dpy, cmap, &color))
			OlError("Color allocation failed");
		else
			XtSetArg(args[0], XtNbackground, color.pixel);

		child = XtCreateManagedWidget(color_names[i],
			rectButtonWidgetClass, exc, args, 1);
	}

} /* END OF AddExclusives() */

/*
 *************************************************************************
 * AddOblongs - this routine adds a set of oblong buttons to a pane
 ****************************procedure*header*****************************
 */
static void
AddOblongs(pane)
	Widget pane;
{
	Widget button;

	button = XtCreateManagedWidget("Change Color", oblongButtonWidgetClass,
			pane, NULL, NULL);
	XtAddCallback(button, XtNselect, ChangeColor, pane);

	button = XtCreateManagedWidget("Oblong 2", oblongButtonWidgetClass,
			pane, NULL, NULL);
} /* END OF AddChildren() */

/*
 *************************************************************************
 * ChangeColor - this routine changes the background color of an
 * abbreviated menu button.
 ****************************procedure*header*****************************
 */
static void
ChangeColor(w, client_data, call_data)
	Widget		w;
	XtPointer	client_data;
	XtPointer	call_data;
{
	Widget		abbrev;		/* Abbreviated menu button	*/
	static char *	colors[] = {"Blue","red","yellow","orange","green"};
	static int	current_index = 0;
	int		color_index;
	XrmValue	fromVal;
	XrmValue	toVal;
	Arg		args[1];
	extern Widget	_OlGetShellOfWidget();

	abbrev = XtParent(_OlGetShellOfWidget(w));

	color_index	= current_index++ % XtNumber(colors);
	fromVal.addr	= colors[color_index];
	fromVal.size	= sizeof(String);

	XtConvert(w, XtRString, &fromVal, XtRPixel, &toVal);

	XtSetArg(args[0], XtNforeground, *((Pixel *)toVal.addr));
	XtSetValues(abbrev, args, 1);

} /* END OF ChangeColor() */

/*
 *************************************************************************
 * Create - creates an AbbrevMenuButton widget
 ****************************procedure*header*****************************
 */
void
Create()
{
	Widget menu_button, control, button;
	static Widget menupane;
	Arg args[20];
	static Arg queryMenuPane[] = {
		{ XtNmenuPane,	(XtArgVal) &menupane}
	};
	int num;

	abbrevTopLevel = XtCreateApplicationShell("toplevel",
			baseWindowShellWidgetClass, NULL, NULL);

	control = XtCreateManagedWidget("control", controlAreaWidgetClass,
			abbrevTopLevel, NULL, NULL);

	num = 0;
	XtSetArg(args[num], XtNpushpin, OL_OUT);
	++num;
	menu_button = XtCreateManagedWidget("AbbrevMenuButton",
			abbrevMenuButtonWidgetClass, control, args, num);

	button = XtCreateManagedWidget("Preview Widget",
			oblongButtonWidgetClass, control, NULL, NULL);

					/* Give the preview Widget to the 
					 * AbbreviatedMenuButton	*/
	num=0;
	XtSetArg(args[num], XtNpreviewWidget, button);
	++num;
	XtSetValues(menu_button, args, num);


				/* Since the menu is automatically added to
				 * the AbbreviatedMenuButton, get the
				 * widget id of the pane that is to be
				 * populated.				*/

	XtGetValues(menu_button, queryMenuPane, XtNumber(queryMenuPane));

			/* Now, we are ready to populate the pane.	*/

	AddChildren(menupane);

	XtRealizeWidget(abbrevTopLevel);

} /* END OF Create() */

/*
 *************************************************************************
 * Destroy
 ****************************procedure*header*****************************
 */
void
Destroy()
{
	if (abbrevTopLevel != (Widget) NULL) {
		XtDestroyWidget(abbrevTopLevel);
		abbrevTopLevel = (Widget) NULL;
	}
	else
		Create();
} /* END OF Destroy() */

/*
 *************************************************************************
 * main - driver function of this demo
 ****************************procedure*header*****************************
 */
main(argc, argv) 
	int argc;
	char ** argv;
{
	Widget toplevel, button;
	Widget quit_button;
	extern void exit ();

	toplevel = OlInitialize("quit_shell", "Demo", NULL, NULL,
				&argc, argv);

	button = XtCreateManagedWidget("quit", oblongButtonWidgetClass,
			toplevel, NULL, NULL);
	XtAddCallback(button, XtNselect, (XtCallbackProc)exit, NULL);

	XtRealizeWidget(toplevel);

					/* Add button to destroy/create
					 * AbbreviatedMenuButton	*/

	toplevel = XtCreateApplicationShell("destroyShell", 
		baseWindowShellWidgetClass, NULL, 0);

	button = XtCreateManagedWidget("Destroy/Create",
			oblongButtonWidgetClass, toplevel, NULL, NULL);
	XtAddCallback(button, XtNselect, (XtCallbackProc)Destroy, NULL);

	XtRealizeWidget(toplevel);

	(void) Create();

	XtMainLoop();
} /* END OF main() */

