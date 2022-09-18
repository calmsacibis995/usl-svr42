/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:Panes.c	1.1"
#endif

/*  Example of the Panes widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/RectObj.h>
#include <Xol/OpenLook.h>
#include <Xol/Caption.h>
#include <Xol/OblongButt.h>
#include <Xol/FButtons.h>
#include <Xol/Panes.h>

#include "WidgetTree.h"

static item ok_items[] = {
	{ "OK", (XtPointer) PopdownCB},
};

/*ARGSUSED*/
void
PanesCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, caption, panes;

        popup = XtVaCreatePopupShell("transientShell",
                transientShellWidgetClass, XtParent(w), 
		XtNtitle, "WidgetTree: Panes",
		XtNresizeCorners, True, 
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        caption = XtVaCreateManagedWidget("caption", captionWidgetClass, popup,
                XtNlabel, "Panes:",
                XtNfont, (XFontStruct *) _OlGetDefaultFont(w, OlDefaultBoldFont),
		XtNposition, OL_TOP,
		XtNalignment, OL_LEFT,
                (String) 0);

        panes = XtVaCreateManagedWidget("panes",
		panesWidgetClass, caption,
		XtNorientation, OL_VERTICAL,
                (String) 0);

        XtVaCreateManagedWidget("Pane_0", oblongButtonWidgetClass, panes,
                XtNweight, 0,
		(String) 0);

        XtVaCreateManagedWidget("Pane_1", oblongButtonWidgetClass, panes,
                XtNweight, 1,
		XtNrefName, "Pane_0",
		XtNrefPosition, OL_BOTTOM,
		(String) 0);

        XtVaCreateManagedWidget("Pane_2", oblongButtonWidgetClass, panes,
                XtNweight, 2,
		XtNrefName, "Pane_1",
		XtNrefPosition, OL_RIGHT,
		(String) 0);

        XtVaCreateManagedWidget("Pane_3", oblongButtonWidgetClass, panes,
                XtNweight, 2,
		XtNrefName, "Pane_2",
		XtNrefPosition, OL_BOTTOM,
		(String) 0);

        XtVaCreateManagedWidget("ok",
		flatButtonsWidgetClass, panes,
		XtNitems, ok_items,
		XtNnumItems, XtNumber(ok_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
		XtNclientData, popup,
		XtNgravity, CenterGravity,
                (String) 0);

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
                        OL_DISK_SOURCE, "Panes.c");

        XtPopup(popup, XtGrabNone);
}  /* end of PanesCB() */
