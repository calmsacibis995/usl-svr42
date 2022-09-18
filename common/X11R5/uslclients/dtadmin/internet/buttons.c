/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:internet/buttons.c	1.20"
#endif

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <OpenLook.h>
#include <PopupWindo.h>
#include <FButtons.h>
#include <MenuShell.h>
#include <Gizmos.h>
#include "inet.h"
#include "error.h"

extern void		FindPopupCB();
extern void		GetFindPopup();
extern Widget		AddFileMenu();
extern Widget		AddEditMenu ();
extern Widget		AddViewMenu ();
extern Widget		AddHelpMenu ();

extern char *		ApplicationName;

/* the resource definition for (Menu) Items below */
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

static Items SystemItems[] = {
	{ (XA)0, NULL,},	/* FILE */
	{ (XA)0, NULL,},	/* EDIT */
	{ (XA)0, NULL,},	/* VIEW */
	{ (XA)0, NULL,},	/* HELP */
};
 
static Menu SystemMenu = {
	"system",
	(Items *) 0,
	XtNumber(SystemItems),
	False,
	OL_FIXEDROWS,
	OL_NONE
};

Widget
AddMenu(parent, menu, menubar_behavior)
Widget parent;
Menu *menu;
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
	GetFindPopup(form);
	SET_LABEL (SystemItems,FILE_INDEX,actions);
	SET_LABEL (SystemItems,EDIT_INDEX,msystem);
	SET_LABEL (SystemItems,VIEW_INDEX,search);
	SET_LABEL (SystemItems,HELP_INDEX,help);
	hf->popupMenuItems =
		CopyItems (SystemItems, SystemMenu.numitems);
	hf->popupMenuItems[FILE_INDEX].popup = (XtArgVal) AddFileMenu(form);
	hf->popupMenuItems[EDIT_INDEX].popup =  (XtArgVal)AddEditMenu(form);
	hf->popupMenuItems[EDIT_INDEX].sensitive = hf->update;
	hf->popupMenuItems[VIEW_INDEX].popup =  (XtArgVal)AddViewMenu(form);
	hf->popupMenuItems[HELP_INDEX].popup =  (XtArgVal)AddHelpMenu(form);
	SystemMenu.items = hf->popupMenuItems;
	return AddMenu (form, &SystemMenu, True);
} /* InitButtons */
