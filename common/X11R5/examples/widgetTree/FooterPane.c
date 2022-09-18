/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:FooterPane.c	1.2"
#endif

/*  Example of the FooterPanel widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/Footer.h>
#include <Xol/FButtons.h>
#include <Xol/FooterPane.h>

#include "WidgetTree.h"

static item ok_items[] = {
	{ "OK", (XtPointer) PopdownCB},
};

/*ARGSUSED*/
void
FooterPanelCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, footerpanel;

        popup = XtVaCreatePopupShell("transientShell",
                transientShellWidgetClass, XtParent(w), 
		XtNtitle, "WidgetTree: FooterPanel",
		XtNresizeCorners, True,
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        footerpanel = XtCreateManagedWidget("footerpanel", footerPanelWidgetClass,
		popup, NULL, 0);

        XtVaCreateManagedWidget("ok",
		flatButtonsWidgetClass, footerpanel,
		XtNgravity, CenterGravity,
		XtNitems, ok_items,
		XtNnumItems, XtNumber(ok_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
		XtNclientData, popup,
                (String) 0);

        XtVaCreateManagedWidget("footer",
		footerWidgetClass, footerpanel,
		XtNleftFoot, "Left Message", 
		XtNrightFoot, "Right Message",
                (String) 0);

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
                        OL_DISK_SOURCE, "FooterPane.c");

        XtPopup(popup, XtGrabNone);
}  /* end of FooterPanelCB() */
