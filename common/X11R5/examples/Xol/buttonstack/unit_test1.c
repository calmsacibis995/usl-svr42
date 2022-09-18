/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:buttonstack/unit_test1.c	1.15"
#endif

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/BaseWindow.h>
#include <Xol/Menu.h>
#include <Xol/MenuButton.h>
#include <Xol/OblongButt.h>

#define ARG_SIZE 9

/*
 *************************************************************************
 * AddChild - This routine adds a MenuButton child to another widget.
 *	Note: This routine is simply a convenience routine !!!!  The 
 * routine is not meant to imply that this is the only way to add
 * children to a menu.
 ****************************procedure*header*****************************
 */
Widget AddChild(w)
	Widget w;
{
	Widget		mbutton;
	static int	id = 0;
	char		name[100];
	static Arg	notify[] = {
		{ XtNpushpin,	OL_OUT }
	};

					/* Initialize Arg list		*/

	sprintf(name, "mbuttonMenuChild_%d", ++id);

			/* Add a Child MenuButtonWidget		*/

	mbutton = XtCreateManagedWidget(name, menuButtonWidgetClass, w, 
			notify, XtNumber(notify));

	return(mbutton);
} /* END OF AddChild() */

/*
 *************************************************************************
 * AddButton - this adds an Oblong Button to the main menu pane. When
 * Adding Buttons, it alternates adding wide-labeled and short-labeled
 * buttons.
 ****************************procedure*header*****************************
 */
AddButton(w, client_data, call_data)
	Widget w;
	caddr_t client_data;
	caddr_t call_data;
{
	Widget		parent = (Widget) client_data;
	Widget		new;
	static int	wide_label = 0;
	String		label;

	label = (wide_label ? "This is a very wide label" : "Short");
	wide_label = ++wide_label & 1;

	new = XtCreateManagedWidget(label, oblongButtonWidgetClass, parent,0,0);
	XtRealizeWidget(new);
} /* END OF AddButton() */

/*
 *************************************************************************
 * DeleteButton - This deletes the 5th child off of a menu pane
 ****************************procedure*header*****************************
 */
DeleteButton(w, client_data, call_data)
	Widget w;
	caddr_t client_data;
	caddr_t call_data;
{
	CompositeWidget	parent = (CompositeWidget) client_data;

	if (parent->composite.num_children > 5) {
		XtDestroyWidget(parent->composite.children[5]);
	}
} /* END OF DeleteButton() */

/*
 *************************************************************************
 * DBeep - this routine does a double beep 
 ****************************procedure*header*****************************
 */
static void
DBeep()
{
	XBell(OlDefaultDisplay, 0);
	XBell(OlDefaultDisplay, 0);
} /* END OF DBeep() */

/*
 *************************************************************************
 * GetPaneChildren - this returns a NULL terminated list of widget ids
 * for the children of the menu_pane
 ****************************procedure*header*****************************
 */
WidgetList
GetPaneChildren(menu_pane)
	Widget menu_pane;
{
	CompositeWidget cw = (CompositeWidget) menu_pane;
	int		i;
	static Widget	list[30];

	for (i=0; i < cw->composite.num_children; ++i)
		list[i] = cw->composite.children[i];
	list[cw->composite.num_children] = (Widget) NULL;

	return(list);

} /* END OF GetPaneChildren() */

/*
 *************************************************************************
 * main - driver function of this demo
 ****************************procedure*header*****************************
 */
void main(argc, argv) 
	int argc;
	char ** argv;
{
	extern void exit ();
	Widget toplevel, menuButton, shell, popup, self;
	Widget widget;
	Widget quit_button;
	static Widget menu_pane;
	WidgetList paneChildren = NULL;
	static Arg make_default[] = {
		{ XtNdefault,	(XtArgVal) True	}
	};
	static XtCallbackRec selectcallbacks[] = {
		{ (XtCallbackProc)exit,	(caddr_t)NULL },
		{ (XtCallbackProc)NULL, (caddr_t)NULL }
	};
	static Arg queryMenuPane[] = {
		{ XtNmenuPane,	(XtArgVal) &menu_pane}
	};
	Arg bsarglist[ARG_SIZE];
	int bsc;
	Arg arglist[ARG_SIZE];
	int ac;

	shell = OlInitialize("toplevelshell", "Demo", NULL, NULL, &argc, argv);

					/* Initialize the MenuButton's
					 * arg list and create the 
					 * widget.			*/

	bsc = 0;
	XtSetArg(bsarglist[bsc], XtNpushpin, OL_OUT);		++bsc;
	XtSetArg(bsarglist[bsc], XtNlabelType, OL_STRING);	++bsc;
	XtSetArg(bsarglist[bsc], XtNrecomputeSize, True);	++bsc;
	XtSetArg(bsarglist[bsc], XtNpaneName,	"Test Pane");	++bsc;
	XtSetArg(bsarglist[bsc], XtNtitle, "Test Menu");	++bsc;

	if (bsc > ARG_SIZE) 
		OlError("MenuButton Arg count greater than allowable");

	menuButton = XtCreateManagedWidget("mbutton", menuButtonWidgetClass,
			shell, bsarglist, bsc);

				/* Since the menu is automatically added to
				 * the menuButton, get the Widget id of
				 * the pane that is to be populated.	*/

	XtGetValues(menuButton, queryMenuPane, XtNumber(queryMenuPane));

			/* Now, we are ready to populate the pane.	*/

	AddChild(menu_pane);
	AddChild(menu_pane);

			/* Add an oblong button that adds a child to this
			 * menu pane					*/

	widget = XtCreateManagedWidget("Add Button", oblongButtonWidgetClass,
					menu_pane, 0,0);
	XtAddCallback(widget, XtNselect, (XtCallbackProc) AddButton,
			(caddr_t) menu_pane);

			/* Add an oblong button that deletes a child of
			 * the pane					*/

	widget = XtCreateManagedWidget("Destroy", oblongButtonWidgetClass,
					menu_pane, 0,0);
	XtAddCallback(widget, XtNselect, (XtCallbackProc) DeleteButton,
			(caddr_t) menu_pane);

			/* Add an Oblong button that beeps the display	*/

	XtAddCallback(XtCreateManagedWidget("Display Beeper",
				oblongButtonWidgetClass, menu_pane, NULL, NULL),
		      XtNselect, (XtCallbackProc) DBeep, NULL);

			/* First, query the original MenuButton to
			 * get the list of children on its menu pane.
			 * Next, since we know we added only two
			 * children (both menuButtons), just use
			 * the first two elements in the returned
			 * array.					*/

	paneChildren = GetPaneChildren(menu_pane);

			/* Reset the current menuButton		*/

	menuButton = paneChildren[0];

			/* Now add children to this menuButton.	*/

	XtGetValues(menuButton, queryMenuPane, XtNumber(queryMenuPane));
	XtSetValues(AddChild(menu_pane), make_default, 1);

			/* Get the second menuButton, get its menu
			 * pane, and add a child to it.			*/

	menuButton = paneChildren[1];
	
	XtGetValues(menuButton, queryMenuPane, XtNumber(queryMenuPane));
	XtSetValues(AddChild(menu_pane), make_default, 1);

			/* Lets be fancy and add a Menu to an Athena
			 * CommandWidget.				*/

	toplevel = XtCreateApplicationShell("quitShell", 
		baseWindowShellWidgetClass, NULL, 0);

			/* Set this Arg List at run time so that we
			 * we can use the millimeter to pixel
			 * conversion routines				*/
	ac=0;
	XtSetArg(arglist[ac], XtNlabel, "Quit");		ac++;
	XtSetArg(arglist[ac], XtNselect, selectcallbacks);	ac++;

	if (ac > ARG_SIZE) 
		OlError("Arg count greater than allowable");

	quit_button = XtCreateManagedWidget("quit", oblongButtonWidgetClass,
			toplevel, arglist, ac);

			/* Add the Popup Menu just like creating any
			 * other kind of popup.				*/

	ac=0;
	XtSetArg(arglist[ac], XtNtitle, "Application Menu");	ac++;
	XtSetArg(arglist[ac], XtNpushpin, OL_OUT);		ac++;

	popup = XtCreatePopupShell("Quit Menu", menuShellWidgetClass,
			quit_button, arglist, ac);

			/* Notice that we can query the Menu popup
			 * directly for the menu pane.			*/

	XtGetValues(popup, queryMenuPane, XtNumber(queryMenuPane));
	XtSetValues(AddChild(menu_pane), make_default, 1);

			/* Add an Oblong button that beeps the display	*/

	XtAddCallback(XtCreateManagedWidget("Display Beeper",
				oblongButtonWidgetClass, menu_pane, NULL, NULL),
		      XtNselect, (XtCallbackProc) DBeep, NULL);

	XtRealizeWidget(toplevel);

	XtRealizeWidget(shell);
	XtMainLoop();
} /* END OF MAIN() */

