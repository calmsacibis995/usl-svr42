/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:ScrollingL.c	1.1"
#endif

/*  Example of the Scrolling List widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Caption.h>
#include <Xol/PopupWindo.h>
#include <Xol/FButtons.h>
#include <Xol/ScrollingL.h>

#include "WidgetTree.h"

static item lower_items[] = {
	{ "OK", (XtPointer) PopdownCB },
};

static OlListToken (*ScrollingListAddItem)();
static void (*ScrollingListTouchItem)();

static OlListItem color_items[] = {
	{ OL_STRING, "Red", NULL, NULL, NULL, 'r'},
	{ OL_STRING, "Green", NULL, NULL, NULL, 'g'},
	{ OL_STRING, "Blue", NULL, NULL, NULL, 'b'},
	{ OL_STRING, "Orange", NULL, NULL, NULL, 'o'},
	{ OL_STRING, "Purple", NULL, NULL, NULL, 'p'},
	{ OL_STRING, "Pink", NULL, NULL, NULL, 'i'},
	{ OL_STRING, "Black", NULL, NULL, NULL, 'l'},
	{ OL_STRING, "White", NULL, NULL, NULL, 'w'},
};

/*ARGSUSED*/
void
ScrollingListCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, upper, lower, caption, slist;
	int i;

        popup = XtVaCreatePopupShell("popupWindowShell",
                popupWindowShellWidgetClass, XtParent(w), 
		XtNtitle, "WidgetTree: ScrollingList",
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        XtVaGetValues(popup,
		XtNupperControlArea, &upper,
		XtNlowerControlArea, &lower,
		(String) 0);

        caption  = XtVaCreateManagedWidget("SListCaption",
		captionWidgetClass, upper,
                XtNlabel, "Scrolling List:",
		(String) 0);

        slist = XtVaCreateManagedWidget("scrollingList",
		scrollingListWidgetClass, caption,
		XtNviewHeight, 3,
                (String) 0);

	XtVaGetValues(slist, XtNapplAddItem, &ScrollingListAddItem,
		XtNapplTouchItem, &ScrollingListTouchItem,
		(String) 0);

	/*  Add the items to the list */
	for (i=0; i < XtNumber(color_items); i++)
		(*ScrollingListAddItem)(slist, NULL, 0, color_items[i]);

        XtVaCreateManagedWidget("ok",
		flatButtonsWidgetClass, lower,
		XtNitems, lower_items,
		XtNnumItems, XtNumber(lower_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
		XtNclientData, popup,
                (String) 0);

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
                        OL_DISK_SOURCE, "ScrollingL.c");

        XtPopup(popup, XtGrabNone);
}  /* end of ScrollingListCB() */
