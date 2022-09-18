/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:help/unit_test2.c	1.2"
#endif

/*
 *************************************************************************
 *
 * Description:
 *	This is a unit test that tests the help facility on FlatWidgets
 * Gadgets and regular widgets.
 *
 ******************************file*header********************************
 */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/BaseWindow.h>
#include <Xol/Menu.h>
#include <Xol/MenuButton.h>
#include <Xol/OblongButt.h>
#include <Xol/FExclusive.h>

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *
 **************************forward*declarations***************************
 */

static Widget		AddChild();	/* adds a menubutton		*/
static void		AddButton();	/* adds button to menu		*/
static void		Create();	/* creats menubutton system	*/
static void		DeleteButton();	/* deletes button from menu	*/
static void		Destroy();	/* destroys menubutton system	*/
static void		DBeep();	/* beeps displays		*/
static WidgetList	GetPaneChildren(); /* Get children of menu pane	*/

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define ARG_SIZE 9

static Widget quit_button = (Widget) NULL;
static Widget menubutton_system = (Widget) NULL;

/*
 *************************************************************************
 * AddChild - This routine adds a MenuButton child to another widget.
 *	Note: This routine is simply a convenience routine !!!!  The 
 * routine is not meant to imply that this is the only way to add
 * children to a menu.
 ****************************procedure*header*****************************
 */
static Widget
AddChild(w)
	Widget w;
{
	Widget		menubutton;
	static int	id = 0;
	char		name[100];
	static String	button_help = " MenuButton Child Help";
	static Arg	notify[] = {
		{ XtNpushpin,	OL_OUT }
	};

					/* Initialize Arg list		*/

	sprintf(name, "menubuttonMenuChild_%d", ++id);

			/* Add a Child MenuButtonWidget		*/

	menubutton = XtCreateManagedWidget(name, menuButtonGadgetClass, w, 
			notify, XtNumber(notify));

	OlRegisterHelp(OL_WIDGET_HELP, menubutton, NULL,
				OL_STRING_SOURCE, button_help);
	return(menubutton);
} /* END OF AddChild() */

/*
 *************************************************************************
 * AddButton - this adds an Oblong Button to the main menu pane. When
 * Adding Buttons, it alternates adding wide-labeled and short-labeled
 * buttons.
 ****************************procedure*header*****************************
 */
static void
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

	new = XtCreateManagedWidget(label, oblongButtonGadgetClass, parent,0,0);
	XtRealizeWidget(new);
} /* END OF AddButton() */

/*
 *************************************************************************
 * DeleteButton - This deletes the 5th child off of a menu pane
 ****************************procedure*header*****************************
 */
static void
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
 * Destroy
 ****************************procedure*header*****************************
 */
static	/*SC, added*/
void
Destroy()
{
	if (menubutton_system != (Widget) NULL) {
		XtDestroyWidget(menubutton_system);

				/* Destroy the quti button's menu	*/

		XtDestroyWidget(quit_button->core.popup_list[0]);
		menubutton_system = (Widget) NULL;
	}
	else
		Create();
} /* END OF Destroy() */

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
 * Create - this routine adds a menu to the quit button and creates
 * an application shell which has menubutton/menu system on it.
 ****************************procedure*header*****************************
 */
static void
Create()
{
	extern Widget	menubutton_system;
	Widget		widget;
	Widget		menubutton;
	static Widget	menu_pane;
	WidgetList	paneChildren = NULL;
	Arg		args[ARG_SIZE];
	static String	button_help = "This is a menuButton";
	int		ac;
	static Arg	make_default[] = {
		{ XtNdefault,	(XtArgVal) True	}
	};
	static Arg	queryMenuPane[] = {
		{ XtNmenuPane,	(XtArgVal) &menu_pane }
	};

	menubutton_system = XtCreateApplicationShell("menubutton_system",
			baseWindowShellWidgetClass, NULL, NULL);

					/* Initialize the MenuButton's
					 * arg list and create the 
					 * widget.			*/

	ac = 0;
	XtSetArg(args[ac], XtNpushpin, OL_OUT);		++ac;
	XtSetArg(args[ac], XtNlabelType, OL_STRING);	++ac;
	XtSetArg(args[ac], XtNrecomputeSize, True);	++ac;
	XtSetArg(args[ac], XtNpaneName,	"Test Pane");	++ac;
	XtSetArg(args[ac], XtNtitle, "Test Menu");	++ac;

	if (ac > ARG_SIZE) 
		OlError("MenuButton Arg count greater than allowable");

	menubutton = XtCreateManagedWidget("menubutton", menuButtonGadgetClass,
			menubutton_system, args, ac);

	OlRegisterHelp(OL_WIDGET_HELP, menubutton, NULL,
				OL_STRING_SOURCE, button_help);

				/* Since the menu is automatically added to
				 * the MenuButton, get the Widget id of
				 * the pane that is to be populated.	*/

	XtGetValues(menubutton, queryMenuPane, XtNumber(queryMenuPane));

			/* Now, we are ready to populate the pane.	*/

	(void) AddChild(menu_pane);
	(void) AddChild(menu_pane);

	{
		Widget			flat;
		OlFlatHelpId		help_id;
		static Boolean		first_time = True;
		static String		fields[] = { XtNlabel };
		static String		item0_help = "Item 0";
		static String		container_help = "container";
		typedef XtArgVal	(FlatExclusives[XtNumber(fields)]);
		static FlatExclusives	items[] = {
			(XtArgVal) "Choice 1",
			(XtArgVal) "Choice 2",
			(XtArgVal) "Choice 3",
			(XtArgVal) "Choice 4"
		};
		XtSetArg(args[0], XtNitems,	items);
		XtSetArg(args[1], XtNnumItems,	XtNumber(items));
		XtSetArg(args[2], XtNitemFields, fields);
		XtSetArg(args[3], XtNnumItemFields, XtNumber(fields));

		flat = XtCreateManagedWidget("flat", flatExclusivesWidgetClass,
					menu_pane, args, 4);

		printf("There is help on Exclusives Choice #1\n");
		help_id.widget = flat;
		help_id.item_index = 0;

		OlRegisterHelp(OL_FLAT_HELP, &help_id, NULL,
				OL_STRING_SOURCE, item0_help);

		if (first_time == True)
		{
			first_time = False;

			printf("There is Flat Exclusives container Help\n");
			OlRegisterHelp(OL_WIDGET_HELP, flat, NULL,
					OL_STRING_SOURCE, container_help);
		}
	}

			/* Add an oblong button that adds a child to this
			 * menu pane					*/

	widget = XtCreateManagedWidget("Add Button", oblongButtonGadgetClass,
					menu_pane, 0,0);
	XtAddCallback(widget, XtNselect, (XtCallbackProc) AddButton,
			(caddr_t) menu_pane);

			/* Add an oblong button that deletes a child of
			 * the pane					*/

	widget = XtCreateManagedWidget("Destroy", oblongButtonGadgetClass,
					menu_pane, 0,0);
	XtAddCallback(widget, XtNselect, (XtCallbackProc) DeleteButton,
			(caddr_t) menu_pane);

			/* Add an Oblong button that beeps the display	*/

	XtAddCallback(XtCreateManagedWidget("Display Beeper",
				oblongButtonGadgetClass, menu_pane, NULL, NULL),
		      XtNselect, (XtCallbackProc) DBeep, NULL);

			/* First, query the original MenuButton to
			 * get the list of children on its menu pane.
			 * Next, since we know we added only two
			 * children (both MenuButtons), just use
			 * the first two elements in the returned
			 * array.					*/

	paneChildren = GetPaneChildren(menu_pane);

			/* Reset the current MenuButton		*/

	menubutton = paneChildren[0];

			/* Now add children to this MenuButton.	*/

	XtGetValues(menubutton, queryMenuPane, XtNumber(queryMenuPane));
	XtSetValues(AddChild(menu_pane), make_default, 1);

			/* Add an Oblong button that beeps the display	*/

	XtAddCallback(XtCreateManagedWidget("Display Beeper",
				oblongButtonGadgetClass, menu_pane, NULL, NULL),
		      XtNselect, (XtCallbackProc) DBeep, NULL);

			/* Get the second MenuButton, get its menu
			 * pane, and add a child to it.			*/

	menubutton = paneChildren[1];
	
	XtGetValues(menubutton, queryMenuPane, XtNumber(queryMenuPane));
	XtSetValues(AddChild(menu_pane), make_default, 1);

			/* Add an Oblong button that beeps the display	*/

	XtAddCallback(XtCreateManagedWidget("Display Beeper",
				oblongButtonGadgetClass, menu_pane, NULL, NULL),
		      XtNselect, (XtCallbackProc) DBeep, NULL);

	XtRealizeWidget(menubutton_system);

} /* END OF Create() */

/*
 *************************************************************************
 * main - driver function of this demo
 ****************************procedure*header*****************************
 */
main(argc, argv) 
	int argc;
	char ** argv;
{
	Widget		shell;
	Widget		button;
	extern Widget	quit_button;
	extern		exit();

	shell = OlInitialize("quit_shell", "Demo", NULL, NULL, &argc, argv);

	quit_button = XtCreateManagedWidget("quit", oblongButtonWidgetClass,
			shell, NULL, NULL);
	XtAddCallback(quit_button, XtNselect, (XtCallbackProc)exit, NULL);

	XtRealizeWidget(shell);

					/* Add button to destroy/create
					 * MenuButton system		*/

	shell = XtCreateApplicationShell("destroyShell", 
		baseWindowShellWidgetClass, NULL, 0);

	button = XtCreateManagedWidget("Destroy/Create",
			oblongButtonGadgetClass, shell, NULL, NULL);
	XtAddCallback(button, XtNselect, (XtCallbackProc)Destroy, NULL);

	XtRealizeWidget(shell);

	(void) Create();

	XtMainLoop();
} /* END OF main() */

