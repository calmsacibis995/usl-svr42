/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:PopupMenu.c	1.1"
#endif

/*  Example of the PopupMenuShell widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <Xol/Caption.h>
#include <Xol/StaticText.h>
#include <Xol/FButtons.h>
#include <Xol/PopupMenu.h>
#include <Xol/OblongButt.h>

#include "WidgetTree.h"

static item ok_items[] = {
	{ "OK", (XtPointer) PopdownCB},
};

static item menu_items[] = {
	{ "Save...", NULL},
	{ "Open...", NULL},
	{ "Properties...", NULL},
	{ "Exit", NULL},
};

/*ARGSUSED*/
void
PopupMenuCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, upper, lower, caption, stext, menu, items;

        popup = XtVaCreatePopupShell("popupWindowShell",
                popupWindowShellWidgetClass, XtParent(w), 
		XtNtitle, "PopupMenu",
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        XtVaGetValues(popup, XtNupperControlArea, &upper,
                        XtNlowerControlArea, &lower,
                        (String) 0);

        caption = XtVaCreateManagedWidget("caption", captionWidgetClass, upper,
                XtNlabel, "PopupMenuShell:",
                XtNfont, (XFontStruct *)_OlGetDefaultFont(w, OlDefaultBoldFont),
                (String) 0);

        stext = XtVaCreateManagedWidget("staticText", staticTextWidgetClass,
		caption,
		XtNstring, "Press MenuButton for menu",
		XtNalignment, OL_CENTER,
                (String) 0);

        menu = XtVaCreatePopupShell("popupMenu",
                popupMenuShellWidgetClass, popup,
		XtNtitle, "PopupMenu",
		XtNpushpin, OL_OUT,
                (String)0);

        items = XtVaCreateManagedWidget("menuitems",
		flatButtonsWidgetClass, menu,
		XtNitems, menu_items,
		XtNnumItems, XtNumber(menu_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
                (String) 0);

        OlVaFlatSetValues(items, 2, XtNdefault, True, 0);

	OlAddDefaultPopupMenuEH(stext, menu);

        XtVaCreateManagedWidget("ok",
		flatButtonsWidgetClass, lower,
		XtNitems, ok_items,
		XtNnumItems, XtNumber(ok_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
		XtNclientData, popup,
                (String) 0);

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
                        OL_DISK_SOURCE, "PopupMenu.c");

        XtPopup(popup, XtGrabNone);
}  /* end of PopupMenuCB() */
