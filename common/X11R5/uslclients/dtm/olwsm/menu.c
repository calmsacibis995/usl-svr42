/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:olwsm/menu.c	1.20"
#endif

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <OpenLook.h>
#include <MenuShell.h>
#include <FButtons.h>

#include <misc.h>
#include <list.h>
#include "menu.h"
#include <wsm.h>

static char * fields[] = {
    XtNlabel, XtNselectProc, XtNclientData, XtNpopupMenu, XtNmnemonic
};

Widget
CreateMenu(parent, menu, horizontal)
	Widget			parent;
	Menu			*menu;
	Boolean			horizontal;
{
	Widget	newMenu;
	char	**pHelp;
	int	i;

	newMenu = XtVaCreateManagedWidget (
		menu->name,
		flatButtonsWidgetClass,
		parent,
		XtNitems,	  (XtArgVal)menu->items,
		XtNnumItems,	  (XtArgVal)menu->itemCnt,
		XtNitemFields,	  (XtArgVal)fields,
		XtNnumItemFields, (XtArgVal)XtNumber(fields),
		XtNlayoutType,
			horizontal ? OL_FIXEDROWS : OL_FIXEDCOLS,
		XtNmeasure,	  (XtArgVal)1,
		(String)0
	);

	if (menu->help)
	{
	    OlFlatHelpId	id;

	    id.widget = newMenu;
	    pHelp = menu->help->items;
	    for (i=0; i<menu->help->helpCnt; i++)
	    {
		if (*pHelp)
		{
		    id.item_index = i;
		    OlRegisterHelp (OL_FLAT_HELP, &id, *(pHelp+1),
				    OL_DESKTOP_SOURCE, HELP(*pHelp));
		}
		pHelp += 2;
	    }
	}

	for (i=0; i<menu->itemCnt; i++)
	{
	    if (menu->items[i].popupMenu)
		menu->items[i].popupMenu =
		    (XtArgVal) CreatePopup (newMenu,
					    menu->items[i].popupMenu);
	}

	return (newMenu);
}

Widget
CreatePopup(parent, menu)
	Widget			parent;
	Menu			*menu;
{
	Widget	shell;
	Widget	newMenu;
	int	i;

	if (menu->title)
	{
	    shell = XtVaCreatePopupShell(
		menu->name,
		popupMenuShellWidgetClass,
		parent,
	    	XtNtitle,	(XtArgVal)menu->title,
	    	XtNhasTitle,	(XtArgVal)True,
	    	XtNpushpin,	(XtArgVal)OL_OUT,
		(String)0
	    );
	}
	else
	{
	    shell = XtVaCreatePopupShell(
		menu->name,
		popupMenuShellWidgetClass,
		parent,
	    	XtNhasTitle,	(XtArgVal)False,
	    	XtNpushpin,	(XtArgVal)OL_NONE,
		(String)0
	    );
	}
	newMenu = CreateMenu (shell, menu, False);
	XtVaSetValues (shell, XtNuserData, (XtArgVal)newMenu, (String)0);

	return (shell);
}

Widget
MenuPane(w)
	Widget			w;
{
	Widget			menu;

	XtVaGetValues (w, XtNmenuPane, (XtArgVal)&menu, (String)0);
	return menu;
}
