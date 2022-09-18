/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:Help.c	1.1"
#endif

/*  Example of the Help widget.  */
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
HelpCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, upper, lower, caption;

        popup = XtVaCreatePopupShell("popupWindowShell",
                popupWindowShellWidgetClass, XtParent(w), 
		XtNtitle, "WidgetTree: Help",
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        XtVaGetValues(popup, XtNupperControlArea, &upper,
                        XtNlowerControlArea, &lower,
                        (String) 0);

        caption = XtVaCreateManagedWidget("caption", captionWidgetClass, upper,
                XtNlabel, "Help:",
                XtNfont, (XFontStruct *)_OlGetDefaultFont(w, OlDefaultBoldFont),
                (String) 0);

        XtVaCreateManagedWidget("staticText", staticTextWidgetClass, caption,
		XtNstring, "The Help widget is not explicitly created and manipulated by an application.  The OlRegisterHelp function is used in an application to register a help file, a help string, or a help function that is displayed when the user presses the Help key (F1).  The Help widget is a Shell that contains a magnifying glass to indicate the context, and a TextEdit widget to display the help message.\n\nPosition the cursor here and press F1 for help.",
		XtNalignment, OL_LEFT,
		XtNwidth, 350, 
		XtNrecomputeSize, False,
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
                        OL_DISK_SOURCE, "Help.c");

        XtPopup(popup, XtGrabNone);
}  /* end of HelpCB() */
