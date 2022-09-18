/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:StaticText.c	1.1"
#endif

/*  Example of the StaticText widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <Xol/Caption.h>
#include <Xol/FButtons.h>
#include <Xol/StaticText.h>

#include "WidgetTree.h"

static item ok_items[] = {
	{ "OK", (XtPointer) PopdownCB},
};

/*ARGSUSED*/
void
StaticTextCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, upper, lower, caption;

        popup = XtVaCreatePopupShell("popupWindowShell",
                popupWindowShellWidgetClass, XtParent(w), 
		XtNtitle, "WidgetTree: StaticText",
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        XtVaGetValues(popup, XtNupperControlArea, &upper,
                        XtNlowerControlArea, &lower,
                        (String) 0);

        caption = XtVaCreateManagedWidget("caption", captionWidgetClass, upper,
                XtNlabel, "StaticText:",
                XtNfont, (XFontStruct *) _OlGetDefaultFont(w, OlDefaultBoldFont),
                (String) 0);

        XtVaCreateManagedWidget("staticText", staticTextWidgetClass, caption,
		XtNstring, "StaticText can be selected and copied.\nEach line can be centered.\nIt is not editable.",
		XtNalignment, OL_CENTER,
                (String) 0);

        XtVaCreateManagedWidget("staticText", staticTextWidgetClass, upper,
		XtNstring, "olDefaultFont",
		XtNfont, _OlGetDefaultFont(w, OlDefaultFont),
		XtNalignment, OL_CENTER,
                (String) 0);

        XtVaCreateManagedWidget("staticText", staticTextWidgetClass, upper,
		XtNstring, "olDefaultBoldFont",
		XtNfont, _OlGetDefaultFont(w, OlDefaultBoldFont),
		XtNalignment, OL_CENTER,
                (String) 0);

        XtVaCreateManagedWidget("staticText", staticTextWidgetClass, upper,
		XtNstring, "olDefaultFixedFont",
		XtNfont, _OlGetDefaultFont(w, OlDefaultFixedFont),
		XtNalignment, OL_CENTER,
                (String) 0);

        XtVaCreateManagedWidget("staticText", staticTextWidgetClass, upper,
		XtNstring, "olDefaultItalicFont",
		XtNfont, _OlGetDefaultFont(w, OlDefaultItalicFont),
		XtNalignment, OL_CENTER,
                (String) 0);

        XtVaCreateManagedWidget("staticText", staticTextWidgetClass, upper,
		XtNstring, "olDefaultBoldItalicFont",
		XtNfont, _OlGetDefaultFont(w, OlDefaultBoldItalicFont),
		XtNalignment, OL_CENTER,
                (String) 0);

        XtVaCreateManagedWidget("staticText", staticTextWidgetClass, upper,
		XtNstring, "olDefaultNoticeFont",
		XtNfont, _OlGetDefaultFont(w, OlDefaultNoticeFont),
		XtNalignment, OL_CENTER,
                (String) 0);

        XtVaCreateManagedWidget("staticText", staticTextWidgetClass, upper,
		XtNstring, "xtDefaultFont",
		XtNfont, _OlGetDefaultFont(w, XtDefaultFont),
		XtNalignment, OL_CENTER,
                (String) 0);

        XtVaCreateManagedWidget("ok",
		flatButtonsWidgetClass, lower,
		XtNitems, ok_items,
		XtNnumItems, XtNumber(ok_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
		XtNclientData, popup,
                (String) 0);

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
                        OL_DISK_SOURCE, "StaticText.c");

        XtPopup(popup, XtGrabNone);
}  /* end of StaticTextCB() */
