/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:button/unit_test8.c	1.8"
#endif

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Menu.h>
#include <Xol/ButtonStac.h>
#include <Xol/OblongButt.h>

#define ARG_SIZE 9

Widget AddChild();

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
static WidgetList
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
void
main(argc, argv) 
	int argc;
	char ** argv;
{
	Widget toplevel, buttonStack, shell, popup, self;
	Widget quit_button;
	static Widget menupane;
	static WidgetList paneChildren = NULL;
	extern void exit();
	static Arg make_default[] = {
		{ XtNdefault,	(XtArgVal) True	}
	};
	static XtCallbackRec selectcallbacks[] = {
		{ (XtCallbackProc)exit,	(caddr_t)NULL },
		{ (XtCallbackProc)NULL, (caddr_t)NULL }
	};
	static Arg queryMenuPane[] = {
		{ XtNmenuPane,	(XtArgVal) &menupane}
	};
	Arg bsarglist[ARG_SIZE];
	int bsc;
	Arg arglist[ARG_SIZE];
	int ac;

	OlInitialize("toplevelshell", "Demo", NULL, NULL, &argc, argv);

					/* Create a shell for a form
					 * widget.  Then , create a form
					 * widget and add a buttonStack
					 * to it.			*/

	shell = XtCreateApplicationShell("bstackShell", 
		topLevelShellWidgetClass, NULL, 0);

					/* Initialize the Buttonstack's
					 * arg list and create the 
					 * widget.			*/

	bsc = 0;
	XtSetArg(bsarglist[bsc], XtNpushpin, True);		++bsc;
	XtSetArg(bsarglist[bsc], XtNwidth, OlPointToPixel(OL_HORIZONTAL, 120)); ++bsc;
	XtSetArg(bsarglist[bsc], XtNheight, OlPointToPixel(OL_VERTICAL, 32));	++bsc;
	XtSetArg(bsarglist[bsc], XtNlabelType, OL_STRING);		++bsc;
	XtSetArg(bsarglist[bsc], XtNrecomputeSize, True);		++bsc;
	XtSetArg(bsarglist[bsc], XtNpaneName,	"Test Pane");		++bsc;
	XtSetArg(bsarglist[bsc], XtNtitle, "Test Menu");		++bsc;

	if (bsc > ARG_SIZE) 
		OlError("Buttonstack Arg count greater than allowable");

	buttonStack = XtCreateManagedWidget("bstack", buttonStackWidgetClass,
			shell, bsarglist, bsc);

				/* Since the menu is automatically added to
				 * the buttonStack, get the Widget id of
				 * the pane that is to be populated.	*/

	XtGetValues(buttonStack, queryMenuPane, XtNumber(queryMenuPane));

			/* Now, we are ready to populate the pane.	*/

	AddChild(menupane);

			/* Add an Oblong button that beeps the display	*/

	XtAddCallback(XtCreateManagedWidget("Display Beeper",
				oblongButtonWidgetClass, menupane, NULL, NULL),
		      XtNselect, (XtCallbackProc) DBeep, NULL);

			/* First, query the original Buttonstack to
			 * get the list of children on its menu pane.
			 * Next, since we know we added only two
			 * children (both buttonStacks), just use
			 * the first two elements in the returned
			 * array.					*/

	paneChildren = GetPaneChildren(menupane);

			/* Reset the current buttonStack		*/

	buttonStack = paneChildren[0];

			/* Now add children to this buttonStack.	*/

	XtGetValues(buttonStack, queryMenuPane, XtNumber(queryMenuPane));
	XtSetValues(AddChild(menupane), make_default, 1);

			/* Get the second buttonStack, get its menu
			 * pane, and add a child to it.			*/

	buttonStack = paneChildren[1];
	
	XtGetValues(buttonStack, queryMenuPane, XtNumber(queryMenuPane));
	XtSetValues(AddChild(menupane), make_default, 1);

			/* Lets be fancy and add a Menu to an oblong
			 * button.					*/

	toplevel = XtCreateApplicationShell("quitShell", 
		topLevelShellWidgetClass, NULL, 0);

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
	XtSetArg(arglist[ac], XtNtitle, "Appl. Menu");		ac++;

	popup = XtCreatePopupShell("Quit Menu", menuShellWidgetClass,
			quit_button, arglist, ac);

			/* Notice that we can query the Menu popup
			 * directly for the menu pane.			*/

	XtGetValues(popup, queryMenuPane, XtNumber(queryMenuPane));
	XtSetValues(AddChild(menupane), make_default, 1);

	XtRealizeWidget(toplevel);

	XtRealizeWidget(shell);
	XtMainLoop();
} /* END OF MAIN() */

/*
 *************************************************************************
 * AddChild - This routine adds a Buttonstack child to another widget.
 *	Note: This routine is simply a convenience routine !!!!  The 
 * routine is not meant to imply that this is the only way to add
 * children to a menu.
 ****************************procedure*header*****************************
 */
Widget AddChild(w)
	Widget w;
{
	Widget		bstack;
	static int	id = 0;
	char		name[100];
	char		oblongName[100];
	static Arg	notify[] = {
		{ XtNpushpin,	True }
	};

					/* Initialize Arg list		*/

	sprintf(name, "bstackMenuChild_%d", ++id);
	sprintf(oblongName, "oblongMenuChild_%d", id);

			/* Add a Child ButtonstackWidget		*/

	bstack = XtCreateManagedWidget(name, buttonStackWidgetClass, w, 
			notify, XtNumber(notify));

	XtCreateManagedWidget(oblongName, oblongButtonWidgetClass, w, 
			NULL, 0);

	return(bstack);
} /* END OF AddChild() */

