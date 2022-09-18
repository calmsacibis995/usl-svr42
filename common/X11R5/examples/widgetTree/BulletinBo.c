/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:BulletinBo.c	1.1"
#endif

/*  Example of the Bulletin Board widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <Xol/OpenLook.h>
#include <Xol/Caption.h>
#include <Xol/StaticText.h>
#include <Xol/FButtons.h>
#include <Xol/BulletinBo.h>

#include "WidgetTree.h"

static item ok_items[] = {
	{ "OK", (XtPointer) PopdownCB},
};

/*ARGSUSED*/
void
BulletinBoardCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, caption, bulletin;

        popup = XtVaCreatePopupShell("transientShell",
                transientShellWidgetClass, XtParent(w), 
		XtNtitle, "WidgetTree: BulletinBoard",
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        caption = XtVaCreateManagedWidget("caption", captionWidgetClass, popup,
                XtNlabel, "BulletinBoard:",
                XtNfont, (XFontStruct *)_OlGetDefaultFont(w, OlDefaultBoldFont),
		XtNposition, OL_TOP,
		XtNalignment, OL_LEFT,
                (String) 0);

        bulletin = XtVaCreateManagedWidget("bulletinBoard",
		bulletinBoardWidgetClass, caption,
		XtNwidth, 200,
		XtNheight, 150,
		XtNborderWidth, 1,
                (String) 0);

        XtVaCreateManagedWidget("0x0", staticTextWidgetClass, bulletin,
                XtNstring, "(0,0)",
		(String) 0);

        XtVaCreateManagedWidget("100x100", staticTextWidgetClass, bulletin,
                XtNstring, "(150,100)",
                XtNx, 100,
                XtNy, 100,
		(String) 0);

        XtVaCreateManagedWidget("80x0", staticTextWidgetClass, bulletin,
                XtNstring, "(80,0)",
                XtNx, 80,
                XtNy, 0,
		(String) 0);

        XtVaCreateManagedWidget("ok",
		flatButtonsWidgetClass, bulletin,
                XtNx, 90,
                XtNy, 130,
		XtNitems, ok_items,
		XtNnumItems, XtNumber(ok_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
		XtNclientData, popup,
                (String) 0);

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
                        OL_DISK_SOURCE, "BulletinBo.c");

        XtPopup(popup, XtGrabNone);
}  /* end of BulletinBoardCB() */
