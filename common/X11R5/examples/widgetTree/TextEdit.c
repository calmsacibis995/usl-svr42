/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:TextEdit.c	1.1"
#endif

/*  Example of the TextEdit widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <Xol/Caption.h>
#include <Xol/FButtons.h>
#include <Xol/TextEdit.h>

#include "WidgetTree.h"

static item ok_items[] = {
	{ "OK", (XtPointer) PopdownCB},
};

/*ARGSUSED*/
void
TextEditCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, upper, lower, caption;

        popup = XtVaCreatePopupShell("popupWindowShell",
                popupWindowShellWidgetClass, XtParent(w), 
		XtNtitle, "WidgetTree: TextEdit",
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        XtVaGetValues(popup, XtNupperControlArea, &upper,
                        XtNlowerControlArea, &lower,
                        (String) 0);

        caption = XtVaCreateManagedWidget("caption", captionWidgetClass, upper,
                XtNlabel, "TextEdit:",
                XtNfont, (XFontStruct *) _OlGetDefaultFont(w, OlDefaultBoldFont),
		XtNposition, OL_TOP,
		XtNalignment, OL_LEFT,
                (String) 0);

        XtVaCreateManagedWidget("textEdit", textEditWidgetClass, caption,
		XtNsource, "The TextEdit widget displays multi-line text. It can wrap on white space.",
		XtNwidth, 200,
		XtNheight, 100,
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
                        OL_DISK_SOURCE, "TextEdit.c");

        XtPopup(popup, XtGrabNone);
}  /* end of TextEditCB() */
