/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/buttons.c	1.26"
#endif

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <OpenLook.h>
#include <PopupWindo.h>
#include <FButtons.h>
#include <MenuShell.h>
#include <Gizmos.h>
#include "uucp.h"
#include "error.h"

extern Boolean		IsSystemFile();
extern void		FindPopupCB();
extern void		GetFindPopup();
extern void		HelpCB();
extern Widget		AddFileMenu();
extern Widget		AddEditMenu ();
extern Widget		AddViewMenu ();
extern Widget		AddHelpMenu ();
extern Widget		AddMenu ();

extern char *		ApplicationName;

static String fields[] = {
	XtNselectProc,
	XtNpopupMenu,
	XtNsensitive,
	XtNlabel,
	XtNmnemonic,
	XtNclientData
};

#define FILE_INDEX 0
#define EDIT_INDEX 1
#define VIEW_INDEX 2
#define HELP_INDEX 3
#define DHELP_INDEX 2

static Items SystemItems[] = {
	{ (XA)0, NULL,},
	{ (XA)0, NULL,},
	{ (XA)0, NULL,},
	{ (XA)0, NULL,},
};
 
static Menus SystemMenu = {
	"system",
	(Items *) 0,
	XtNumber(SystemItems),
	False,
	OL_FIXEDROWS,
	OL_NONE
};

static Items DeviceTopItems[] = {
	{ (void (*)())0, NULL,},
	{ (void (*)())0, NULL,},
	{ (void (*)())0, NULL,},
};

static Menus DeviceMenu = {
	"device",
	(Items *) 0,
	XtNumber(DeviceTopItems),
	False,
	OL_FIXEDROWS,
	OL_NONE
};

static HelpText AppHelp = {
    title_setup, HELP_FILE, help_setup,
};

static HelpText TOCHelp = {
    title_toc, HELP_FILE, 0,
};

static Items helpItems[] = {
	{HelpCB, NULL, (XA)TRUE, NULL, NULL, (XA)&AppHelp},
	{HelpCB, NULL, (XA)TRUE, NULL, NULL, (XA)&TOCHelp},
	{HelpCB, NULL, (XA)TRUE, NULL, NULL, NULL },
};

static Menus helpMenu = {
	"help",
	helpItems,
	XtNumber (helpItems),
	True,
	OL_FIXEDCOLS,
	OL_NONE
};

Widget
AddHelpMenu(wid)
Widget wid;
{
	static Boolean first_time = True;
	if (first_time) {
		first_time = False;
		SET_HELP(AppHelp);
		SET_HELP(TOCHelp);
	}
	SET_LABEL(helpItems,0,setup);
	SET_LABEL(helpItems,1,toc);
	SET_LABEL(helpItems,2,desk);
	return AddMenu (wid, &helpMenu, False);
} /* AddHelpMenu */


Widget
AddMenu(parent, menu, menubar_behavior)
Widget parent;
Menus *menu;
Boolean menubar_behavior;
{
	Widget	flatmenu;
	Widget  shell;

	shell = parent;
	if (menu->use_popup == True) {
		shell = XtVaCreatePopupShell (
			menu->label,
			popupMenuShellWidgetClass,
			parent,
			XtNpushpin, menu->pushpin,
			(String)0
		);
	}

	flatmenu = XtVaCreateManagedWidget(
		"_menu_",
		flatButtonsWidgetClass,
		shell,
		XtNvPad,		5,
		XtNlabelJustify,	OL_LEFT,
		XtNrecomputeSize,	True,
		XtNlayoutType,		menu->orientation,
		XtNitemFields,		fields,
		XtNnumItemFields,	XtNumber(fields),
		XtNitems,		menu->items,
		XtNnumItems,		menu->numitems,
		XtNdefault,		True,
		XtNmenubarBehavior,	menubar_behavior,
		(String)0
	);

	return menu->use_popup == True ? shell : flatmenu;
} /* AddMenu */

static Items *
CopyItems(items, n)
Items *items;
int n;
{
	Items *ip;
	Items *newitems;
	register i;

	newitems = ip = (Items *) XtMalloc (sizeof (Items) * n);
	for (i=0; i<n; i++) {
		ip->p		= items->p;
		ip->popup	= (XtArgVal) NULL;
		ip->sensitive	= (XtArgVal) TRUE;
		ip->label	= (XtArgVal) items->label;
		ip->mnemonic	= (XtArgVal) items->mnemonic;
		ip->client	= (XtArgVal) items->client;
		ip++;
		items++;
	}
	return newitems;
} /* CopyItems */

Widget
InitButtons(form)
Widget form;
{
	Boolean sys = IsSystemFile(form);
	if (sys) {
		GetFindPopup(form);
		SET_LABEL (SystemItems,FILE_INDEX,actions);
		SET_LABEL (SystemItems,EDIT_INDEX,msystem);
		SET_LABEL (SystemItems,VIEW_INDEX,search);
		SET_LABEL (SystemItems,HELP_INDEX,help);
		sf->popupMenuItems =
			CopyItems (SystemItems, SystemMenu.numitems);
		sf->popupMenuItems[FILE_INDEX].popup = (XtArgVal)AddFileMenu(form);
		sf->popupMenuItems[EDIT_INDEX].popup = (XtArgVal)AddEditMenu(form);
		sf->popupMenuItems[EDIT_INDEX].sensitive = sf->update;
		sf->popupMenuItems[VIEW_INDEX].popup = (XtArgVal)AddViewMenu(form);
		sf->popupMenuItems[HELP_INDEX].popup = (XtArgVal)AddHelpMenu(form);
		SystemMenu.items = sf->popupMenuItems;
		return AddMenu (form, &SystemMenu, True);
	} else {
		SET_LABEL (DeviceTopItems,FILE_INDEX,actions);
		SET_LABEL (DeviceTopItems,EDIT_INDEX,mdevice);
		SET_LABEL (DeviceTopItems,DHELP_INDEX,help);
		df->popupMenuItems =
			CopyItems (DeviceTopItems, DeviceMenu.numitems);
		df->popupMenuItems[FILE_INDEX].popup = (XtArgVal)AddFileMenu(form);
		df->popupMenuItems[EDIT_INDEX].popup = (XtArgVal)AddEditMenu(form);
		df->popupMenuItems[EDIT_INDEX].sensitive = sf->update;
		df->popupMenuItems[DHELP_INDEX].popup = (XtArgVal)AddHelpMenu(form);
		DeviceMenu.items = df->popupMenuItems;
		return AddMenu (form, &DeviceMenu, True);
	}
} /* InitButtons */
