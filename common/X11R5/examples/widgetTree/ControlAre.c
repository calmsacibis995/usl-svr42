/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:ControlAre.c	1.1"
#endif

/*  Example of the Control Area widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <Xol/OpenLook.h>
#include <Xol/Caption.h>
#include <Xol/FButtons.h>
#include <Xol/ControlAre.h>

#include "WidgetTree.h"

static item ok_items[] = {
	{ "OK", (XtPointer) PopdownCB},
};

static item color_items[] = {
	{ "Red", NULL},
	{ "Orange", NULL},
	{ "Yellow", NULL},
	{ "Green", NULL},
	{ "Blue", NULL},
	{ "Indigo", NULL},
	{ "Violet", NULL},
	{ "Black", NULL},
};

static item fruit_items[] = {
	{ "Apples", NULL},
	{ "Oranges", NULL},
	{ "Banannas", NULL},
	{ "Grapes", NULL},
	{ "Watermelon", NULL},
	{ "Kiwi", NULL},
};

static item animal_items[] = {
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

/*ARGSUSED*/
void
ControlAreaCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, caption, controlarea;

        popup = XtVaCreatePopupShell("transientShell",
                transientShellWidgetClass, XtParent(w), 
		XtNtitle, "WidgetTree: ControlArea",
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        caption = XtVaCreateManagedWidget("caption", captionWidgetClass, popup,
                XtNlabel, "ControlArea:",
                XtNfont, (XFontStruct *) _OlGetDefaultFont(w, OlDefaultBoldFont),
		XtNposition, OL_TOP,
		XtNalignment, OL_LEFT,
                (String) 0);

        controlarea = XtVaCreateManagedWidget("controlarea",
		controlAreaWidgetClass, caption,
		XtNalignCaptions, True,
		XtNlayoutType, OL_FIXEDCOLS,
		XtNcenter, True,
		XtNhSpace, 10,
                (String) 0);

        caption  = XtVaCreateManagedWidget("fruit", captionWidgetClass,
		controlarea,
                XtNlabel, "Fruits:",
		(String) 0);

        XtVaCreateManagedWidget("fruitbuttons",
		flatButtonsWidgetClass, caption,
		XtNlayoutType, OL_FIXEDCOLS,
		XtNbuttonType, OL_RECT_BTN,
		XtNexclusives, True,
		XtNmeasure, 3,
		XtNitems, fruit_items,
		XtNnumItems, XtNumber(fruit_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
                (String) 0);

        caption  = XtVaCreateManagedWidget("colors", captionWidgetClass,
		controlarea,
                XtNlabel, "Colors:",
		(String) 0);

        XtVaCreateManagedWidget("colorbuttons",
		flatButtonsWidgetClass, caption,
		XtNlayoutType, OL_FIXEDCOLS,
		XtNbuttonType, OL_RECT_BTN,
		XtNexclusives, True,
		XtNmeasure, 4,
		XtNitems, color_items,
		XtNnumItems, XtNumber(color_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
                (String) 0);

        caption  = XtVaCreateManagedWidget("animals", captionWidgetClass,
		controlarea,
                XtNlabel, "Animals:",
		(String) 0);

        XtVaCreateManagedWidget("animalbuttons",
		flatButtonsWidgetClass, caption,
		XtNlayoutType, OL_FIXEDCOLS,
		XtNbuttonType, OL_RECT_BTN,
		XtNexclusives, True,
		XtNmeasure, 3,
		XtNitems, animal_items,
		XtNnumItems, XtNumber(animal_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
                (String) 0);

        XtVaCreateManagedWidget("ok",
		flatButtonsWidgetClass, controlarea,
		XtNitems, ok_items,
		XtNnumItems, XtNumber(ok_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
		XtNclientData, popup,
                (String) 0);

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
                        OL_DISK_SOURCE, "ControlAre.c");

        XtPopup(popup, XtGrabNone);
}  /* end of ControlAreaCB() */
