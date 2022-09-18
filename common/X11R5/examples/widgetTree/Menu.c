/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:Menu.c	1.2"
#endif

/*  Example of the MenuShell widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <Xol/Caption.h>
#include <Xol/StaticText.h>
#include <Xol/OblongButt.h>
#include <Xol/Menu.h>

#include "WidgetTree.h"

/*ARGSUSED*/
void
MenuCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, upper, lower, caption, stext, menu, menupane;

        popup = XtVaCreatePopupShell("popupWindowShell",
                popupWindowShellWidgetClass, XtParent(w), 
		XtNtitle, "WidgetTree: Menu",
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        XtVaGetValues(popup, XtNupperControlArea, &upper,
                        XtNlowerControlArea, &lower,
                        (String) 0);

        caption = XtVaCreateManagedWidget("caption", captionWidgetClass, upper,
                XtNlabel, "MenuShell:",
                (String) 0);

        stext = XtVaCreateManagedWidget("staticText", staticTextWidgetClass,
		caption,
		XtNstring, "Press MenuButton for menu",
		XtNalignment, OL_CENTER,
                (String) 0);

        menu = XtVaCreatePopupShell("menu",
                menuShellWidgetClass, stext,
		XtNtitle, "Menu",
		XtNpushpin, OL_OUT,
                (String)0);

	XtVaGetValues(menu, XtNmenuPane, &menupane, (String)0);

        XtCreateManagedWidget("Save...", oblongButtonGadgetClass, menupane,
                NULL, 0);

        XtCreateManagedWidget("Open...", oblongButtonGadgetClass, menupane,
                NULL, 0);

        XtCreateManagedWidget("Properties...", oblongButtonGadgetClass, menupane,
                NULL, 0);

        XtCreateManagedWidget("Exit", oblongButtonGadgetClass, menupane,
                NULL, 0);

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
                        OL_DISK_SOURCE, "Menu.c");

        XtCreateManagedWidget("OK", oblongButtonWidgetClass, lower,
                NULL, 0);

        XtPopup(popup, XtGrabNone);
}  /* end of MenuCB() */
