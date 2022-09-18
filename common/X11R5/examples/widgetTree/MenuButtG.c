/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:MenuButtG.c	1.1"
#endif

/*  Example of the MenuButton gadget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <Xol/OblongButt.h>
#include <Xol/MenuButton.h>

#include "WidgetTree.h"

/*ARGSUSED*/
void
MenuButtonGadgetCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, upper, lower, menubutton, pane;

        popup = XtVaCreatePopupShell("popupWindowShell",
                popupWindowShellWidgetClass, XtParent(w), 
		XtNtitle, "Menu Button Gadget",
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        XtVaGetValues(popup,
		XtNupperControlArea, &upper,
		XtNlowerControlArea, &lower,
		(String) 0);

        menubutton = XtVaCreateManagedWidget("menugadget",
		menuButtonGadgetClass, upper,
		XtNlabel, "Menu Button Gadget",
		(String)0);

        XtVaGetValues(menubutton, XtNmenuPane, &pane, (String) 0);

        XtCreateManagedWidget("Open...", oblongButtonGadgetClass,
		pane, NULL, 0);

        XtCreateManagedWidget("Save...", oblongButtonGadgetClass,
		pane, NULL, 0);

        XtCreateManagedWidget("Exit", oblongButtonGadgetClass,
		pane, NULL, 0);

        XtCreateManagedWidget("OK", oblongButtonGadgetClass,
		lower, NULL, 0);

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
                        OL_DISK_SOURCE, "MenuButtG.c");

        XtPopup(popup, XtGrabNone);
}  /* end of MenuButtonGadgetCB() */
