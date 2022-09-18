/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:FButtons.c	1.1"
#endif

/*  Example of the Flat Buttons widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Caption.h>
#include <Xol/PopupWindo.h>
#include <Xol/PopupMenu.h>
#include <Xol/FButtons.h>

#include "WidgetTree.h"

/*  To use a flat button, first decide which resource each flat button
    will need a distinct value for.  In this example, only the label
    of the button and the select callback are different.  But, other
    resources can be added to the structure as they are needed.  For
    example, if the font is different for each button, then add a font
    field to the Item structure, and add the XtNfont resource to the
    ItemFields list below. */

typedef struct _Item  {
	XtPointer	label;
	XtPointer	select;
}  Item;

/*  Declare the resource fields used for the flat buttons.  This is a
    list of "item" resources described in the manual page. Add to this
    list the resource name that corresponds to the field in the item
    structure.  */

static char * ItemFields[] = {XtNlabel, XtNselectProc};

static Item lower_items[] = {
	{ "OK", (XtPointer) PopdownCB },
	{ "Apply", NULL},
	{ "Reset", NULL},
	{ "Cancel", NULL},
	{ "Help", NULL},
};

static Item onoff_items[] = {
	{ "On", NULL},
	{ "Off", NULL},
};

static Item color_items[] = {
	{ "Red", NULL},
	{ "Green", NULL},
	{ "Blue", NULL},
};

static Item permission_items[] = {
	{ "Read", NULL},
	{ "Write", NULL},
	{ "Execute", NULL},
};

/*  As an example of how to use a Flat Button menu button, create a new
    item structure that adds a popup menu field to the item stucture and
    field list.  */

typedef struct _MenuItem  {
	XtPointer	label;
	XtPointer	select;
	XtPointer	menu;
}  MenuItem;

static char * MenuItemFields[] = {XtNlabel, XtNselectProc, XtNpopupMenu};

static MenuItem menu_button_items[] = {
	{ "Flat Menu Button", NULL, NULL},
};

static MenuItem menu_items[] = {
	{ "Menu Item 1", NULL, NULL},
	{ "Cascade", NULL, NULL},
	{ "Menu Item 3", NULL, NULL},
};

static Item sub_menu_items[] = {
	{ "Menu Item 1", NULL},
};

/*ARGSUSED*/
void
FButtonsCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, upper, lower, caption;

        popup = XtVaCreatePopupShell("popupWindowShell",
                popupWindowShellWidgetClass, XtParent(w), 
		XtNtitle, "WidgetTree: FlatButtons",
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        XtVaGetValues(popup,
		XtNupperControlArea, &upper,
		XtNlowerControlArea, &lower,
		(String) 0);

	/*  Create a exclusive button set. */
        caption  = XtVaCreateManagedWidget("bell", captionWidgetClass,
		upper,
                XtNlabel, "Bell:",
		(String) 0);

        XtVaCreateManagedWidget("onoff",
		flatButtonsWidgetClass, caption,
		XtNbuttonType, OL_RECT_BTN,
		XtNexclusives, True,
		XtNitems, onoff_items,
		XtNnumItems, XtNumber(onoff_items),
		XtNitemFields, ItemFields,
		XtNnumItemFields, XtNumber(ItemFields),
                (String) 0);

	/*  Create a non-exclusive button set. */
        caption  = XtVaCreateManagedWidget("color", captionWidgetClass,
		upper,
                XtNlabel, "Color:",
		(String) 0);

        XtVaCreateManagedWidget("size",
		flatButtonsWidgetClass, caption,
		XtNbuttonType, OL_RECT_BTN,
		XtNitems, color_items,
		XtNnumItems, XtNumber(color_items),
		XtNitemFields, ItemFields,
		XtNnumItemFields, XtNumber(ItemFields),
                (String) 0);

	/*  Create a checkbox button set. */
        caption  = XtVaCreateManagedWidget("permission", captionWidgetClass,
		upper,
                XtNlabel, "Permissions:",
		(String) 0);

        XtVaCreateManagedWidget("permsission",
		flatButtonsWidgetClass, caption,
		XtNbuttonType, OL_CHECKBOX,
		XtNitems, permission_items,
		XtNnumItems, XtNumber(permission_items),
		XtNitemFields, ItemFields,
		XtNnumItemFields, XtNumber(ItemFields),
                (String) 0);

	/*  Create a menu button set. */
        caption = XtVaCreateManagedWidget("menu_button", captionWidgetClass,
		upper,
                XtNlabel, "Flat Menu Button:",
		(String) 0);

	/*  Create the popup menu shell's in the flat button popup field. */
	menu_button_items[0].menu = (XtPointer) XtVaCreatePopupShell("menu",
		popupMenuShellWidgetClass, popup, 
		XtNpushpin, OL_OUT,
		XtNtitle, "PopupMenuShell",
		(String) 0);

	menu_items[1].menu = (XtPointer) XtCreatePopupShell("cascade",
		popupMenuShellWidgetClass, popup, NULL, 0);

        XtVaCreateManagedWidget("menu_button",
		flatButtonsWidgetClass, caption,
		XtNitems, menu_button_items,
		XtNnumItems, XtNumber(menu_button_items),
		XtNitemFields, MenuItemFields,
		XtNnumItemFields, XtNumber(MenuItemFields),
                (String) 0);

        XtVaCreateManagedWidget("menu_items",
		flatButtonsWidgetClass, menu_button_items[0].menu,
		XtNitems, menu_items,
		XtNnumItems, XtNumber(menu_items),
		XtNitemFields, MenuItemFields,
		XtNnumItemFields, XtNumber(MenuItemFields),
                (String) 0);

        XtVaCreateManagedWidget("cascade_items",
		flatButtonsWidgetClass, menu_items[1].menu,
		XtNitems, sub_menu_items,
		XtNnumItems, XtNumber(sub_menu_items),
		XtNitemFields, ItemFields,
		XtNnumItemFields, XtNumber(ItemFields),
                (String) 0);

        XtVaCreateManagedWidget("ok",
		flatButtonsWidgetClass, lower,
		XtNitems, lower_items,
		XtNnumItems, XtNumber(lower_items),
		XtNitemFields, ItemFields,
		XtNnumItemFields, XtNumber(ItemFields),
		XtNclientData, popup,
                (String) 0);

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
                        OL_DISK_SOURCE, "FButtons.c");

        XtPopup(popup, XtGrabNone);
}  /* end of FButtonsCB() */
