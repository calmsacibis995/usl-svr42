/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:PopupWindo.c	1.1"
#endif

/*  Example of the Popup Window widget.  */
#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Caption.h>
#include <Xol/FButtons.h>
#include <Xol/Footer.h>
#include <Xol/PopupWindo.h>

#include "WidgetTree.h"

static item onoff_items[] = {
	{ "On", NULL},
	{ "Off", NULL},
};

static item size_items[] = {
	{ "6", NULL},
	{ "8", NULL},
	{ "10", NULL},
	{ "12", NULL},
	{ "14", NULL},
	{ "18", NULL},
	{ "24", NULL},
};

/*ARGSUSED*/
static void
ApplyCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
	printf("Apply\n");
}  /*  end of ApplyCB() */

/*ARGSUSED*/
static void
ResetCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
	printf("Reset\n");
}  /*  end of ResetCB() */

/*ARGSUSED*/
static void
ResetFactoryCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
	printf("ResetFactory\n");
}  /*  end of ResetFactoryCB() */

/*ARGSUSED*/
static void
SetDefaultsCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
	printf("SetDefaults\n");
}  /*  end of SetDefaultsCB() */

static XtCallbackRec apply[] = {
	{ ApplyCB, NULL },
	{ NULL, NULL},
};

static XtCallbackRec reset[] = {
	{ ResetCB, NULL },
	{ NULL, NULL},
};

static XtCallbackRec resetFactory[] = {
	{ ResetFactoryCB, NULL },
	{ NULL, NULL},
};

static XtCallbackRec setDefaults[] = {
	{ SetDefaultsCB, NULL },
	{ NULL, NULL},
};

/*ARGSUSED*/
void
PopupWindowCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, upper, lower, footer, caption;

        popup = XtVaCreatePopupShell("popupWindowShell",
                popupWindowShellWidgetClass, XtParent(w), 
		XtNtitle, "PopupWindowShell",
		XtNapply, apply,
		XtNreset, reset,
		XtNresetFactory, resetFactory,
		XtNsetDefaults, setDefaults,
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        XtVaGetValues(popup,
		XtNupperControlArea, &upper,
		XtNlowerControlArea, &lower,
		XtNfooterPanel, &footer,
		(String) 0);

        caption  = XtVaCreateManagedWidget("bell", captionWidgetClass,
		upper,
                XtNlabel, "Bell:",
		(String) 0);

        XtVaCreateManagedWidget("onoff",
		flatButtonsWidgetClass, caption,
		XtNbuttonType, OL_RECT_BTN,
		XtNexclusives, True,
		XtNitems, onoff_items,
		XtNnumItems, XtNumber(onoff_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
                (String) 0);

        caption  = XtVaCreateManagedWidget("size", captionWidgetClass,
		upper,
                XtNlabel, "Point Size:",
		(String) 0);

        XtVaCreateManagedWidget("size",
		flatButtonsWidgetClass, caption,
		XtNbuttonType, OL_RECT_BTN,
		XtNexclusives, True,
		XtNitems, size_items,
		XtNnumItems, XtNumber(size_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
                (String) 0);

	XtVaCreateManagedWidget("footer", footerWidgetClass, footer, 
		XtNleftFoot, "Left",
		XtNrightFoot, "Right",
		(String)0);

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
                        OL_DISK_SOURCE, "PopupWindo.c");

        XtPopup(popup, XtGrabNone);
}  /* end of PopupWindowCB() */
