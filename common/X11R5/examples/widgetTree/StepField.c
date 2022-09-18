/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:StepField.c	1.1"
#endif

/*  Example of the StepField widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <Xol/Caption.h>
#include <Xol/FButtons.h>
#include <Xol/StepField.h>

#include "WidgetTree.h"

static item ok_items[] = {
	{ "OK", (XtPointer) PopdownCB},
};

static char * months[] = {
	"January",
	"February",
	"March",
	"April",
	"May",
	"June", 
	"July",
	"August",
	"September",
	"October",
	"November",
	"December",
};

/*ARGSUSED*/
static void
SteppedCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
	OlTextFieldStepped * stepped = (OlTextFieldStepped *) call_data;
	int current_month;

	XtVaGetValues(w, XtNuserData, &current_month, 0);

	if (stepped->reason == OlSteppedIncrement)  {
		current_month = current_month + stepped->count;
		if (current_month >= 12)
			current_month = current_month - 12;
	}
	else {  /* OlSteppedDecrement */
		current_month = current_month - stepped->count;
		if (current_month < 0)
			current_month = 12 + current_month;
	}

	XtVaSetValues(w, XtNstring, months[current_month],
		XtNuserData, current_month,
		(String) 0);
}  /* end of SteppedCB() */

/*ARGSUSED*/
void
StepFieldCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, upper, lower, caption, stepfield;

        popup = XtVaCreatePopupShell("popupWindowShell",
                popupWindowShellWidgetClass, XtParent(w), 
		XtNtitle, "StepField",
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        XtVaGetValues(popup, XtNupperControlArea, &upper,
                        XtNlowerControlArea, &lower,
                        (String) 0);

        caption = XtVaCreateManagedWidget("caption", captionWidgetClass, upper,
                XtNlabel, "StepField:",
                XtNfont, (XFontStruct *)_OlGetDefaultFont(w, OlDefaultBoldFont),
		XtNposition, OL_LEFT,
                (String) 0);

        stepfield = XtVaCreateManagedWidget("stepField", stepFieldWidgetClass,
		caption,
		XtNstring, "January",
		XtNcharsVisible, 8,
		XtNuserData, 0,		/*  maintains the current month  */
                (String) 0);
	XtAddCallback(stepfield, XtNstepped, SteppedCB, 0);

        XtVaCreateManagedWidget("ok",
		flatButtonsWidgetClass, lower,
		XtNitems, ok_items,
		XtNnumItems, XtNumber(ok_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
		XtNclientData, popup,
                (String) 0);

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
                        OL_DISK_SOURCE, "StepField.c");

        XtPopup(popup, XtGrabNone);
}  /* end of StepFieldCB() */
