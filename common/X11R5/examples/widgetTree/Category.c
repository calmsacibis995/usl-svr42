/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:Category.c	1.1"
#endif

/*  Example of the Category widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <Xol/OpenLook.h>
#include <Xol/OblongButt.h>
#include <Xol/FButtons.h>
#include <Xol/Category.h>

#include "WidgetTree.h"

static item page1_items[] = {
	{ "Red", NULL},
	{ "Orange", NULL},
	{ "Yellow", NULL},
	{ "Green", NULL},
	{ "Blue", NULL},
	{ "Indigo", NULL},
	{ "Violet", NULL},
};

static item page2_items[] = {
	{ "Apples", NULL},
	{ "Oranges", NULL},
	{ "Banannas", NULL},
	{ "Grapes", NULL},
	{ "Watermelon", NULL},
};

static item page3_items[] = {
	{ "Lions", NULL},
	{ "Tigers", NULL},
	{ "Bears", NULL},
	{ "Elephants", NULL},
	{ "Zebras", NULL},
	{ "Monkeys", NULL},
	{ "Hippos", NULL},
	{ "Deer", NULL},
	{ "Duck Billed Paltypus", NULL},
};

static item ok_items[] = {
	{ "OK", (XtPointer) PopdownCB},
};

/*ARGSUSED*/
void
CategoryCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, category, lower;

        popup = XtVaCreatePopupShell("transientShell",
                transientShellWidgetClass, XtParent(w), 
		XtNtitle, "WidgetTree: Category",
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

	category = XtVaCreateManagedWidget("category", categoryWidgetClass, popup,
		XtNcategoryLabel, "Category:",
		XtNleftFoot, "LeftFoot",
		XtNrightFoot, "RightFoot",
		XtNshowFooter, True,
		(String) 0);

	XtVaGetValues(category, XtNlowerControlArea, &lower, (String)0);

        XtVaCreateManagedWidget("colors",
		flatButtonsWidgetClass, category,
		XtNpageLabel, "Colors",
		XtNlayoutType, OL_FIXEDCOLS,
		XtNexclusives, True,
		XtNbuttonType, OL_RECT_BTN,
		XtNitems, page1_items,
		XtNnumItems, XtNumber(page1_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
                (String) 0);

        XtVaCreateManagedWidget("fruits",
		flatButtonsWidgetClass, category,
		XtNpageLabel, "Fruits",
		XtNlayoutType, OL_FIXEDCOLS,
		XtNexclusives, True,
		XtNbuttonType, OL_RECT_BTN,
		XtNitems, page2_items,
		XtNnumItems, XtNumber(page2_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
                (String) 0);

        XtVaCreateManagedWidget("animals",
		flatButtonsWidgetClass, category,
		XtNpageLabel, "Animals",
		XtNlayoutType, OL_FIXEDCOLS,
		XtNexclusives, True,
		XtNbuttonType, OL_RECT_BTN,
		XtNitems, page3_items,
		XtNnumItems, XtNumber(page3_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
                (String) 0);

        XtVaCreateManagedWidget("ok", flatButtonsWidgetClass, lower,
		XtNitems, ok_items,
		XtNnumItems, XtNumber(ok_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
		XtNclientData, popup,
                (String) 0);

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
                        OL_DISK_SOURCE, "Category.c");

        XtPopup(popup, XtGrabNone);
}  /* end of CategoryCB() */
