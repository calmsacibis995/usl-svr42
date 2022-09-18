/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:Nonexclusi.c	1.1"
#endif

/*  Example of the Nonexclusives widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <Xol/Caption.h>
#include <Xol/OblongButt.h>
#include <Xol/RectButton.h>
#include <Xol/Nonexclusi.h>

#include "WidgetTree.h"

/*ARGSUSED*/
void
NonexclusivesCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, upper, lower, caption, nonexclusives;

        popup = XtVaCreatePopupShell("popupWindowShell",
                popupWindowShellWidgetClass, XtParent(w), 
		XtNtitle, "FlatNonexclusives",
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        XtVaGetValues(popup,
		XtNupperControlArea, &upper,
		XtNlowerControlArea, &lower,
		(String) 0);

        caption = XtVaCreateManagedWidget("caption", captionWidgetClass, upper, 
			XtNlabel, "Nonexclusives:",
			XtNfont, (XFontStruct *)_OlGetDefaultFont(upper, OlDefaultBoldFont),
			(String) 0);

        nonexclusives = XtCreateManagedWidget("Nonexclusives Widget",
		nonexclusivesWidgetClass, caption, NULL, 0);

        XtCreateManagedWidget("Red", rectButtonWidgetClass,
		nonexclusives, NULL, 0);

        XtCreateManagedWidget("Green", rectButtonWidgetClass,
		nonexclusives, NULL, 0);

        XtCreateManagedWidget("Blue", rectButtonWidgetClass,
		nonexclusives, NULL, 0);

        XtCreateManagedWidget("OK", oblongButtonWidgetClass,
		lower, NULL, 0);

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
                        OL_DISK_SOURCE, "Nonexclusi.c");

        XtPopup(popup, XtGrabNone);
}  /* end of NonexclusivesCB() */
