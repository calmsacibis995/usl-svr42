/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:RubberTile.c	1.1"
#endif

/*  Example of the RubberTile widget.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/RectObj.h>
#include <Xol/OpenLook.h>
#include <Xol/Caption.h>
#include <Xol/OblongButt.h>
#include <Xol/FButtons.h>
#include <Xol/RubberTile.h>

#include "WidgetTree.h"

static item ok_items[] = {
	{ "OK", (XtPointer) PopdownCB},
};

/*ARGSUSED*/
void
RubberTileCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, caption, rubbertile, rubbertile2;

        popup = XtVaCreatePopupShell("transientShell",
                transientShellWidgetClass, XtParent(w), 
		XtNtitle, "WidgetTree: RuberTile",
		XtNresizeCorners, True, 
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

        caption = XtVaCreateManagedWidget("caption", captionWidgetClass, popup,
                XtNlabel, "RubberTile:",
                XtNfont, (XFontStruct *) _OlGetDefaultFont(w, OlDefaultBoldFont),
		XtNposition, OL_TOP,
		XtNalignment, OL_LEFT,
                (String) 0);

        rubbertile = XtVaCreateManagedWidget("rubberTile",
		rubberTileWidgetClass, caption,
		XtNorientation, OL_VERTICAL,
                (String) 0);

        XtVaCreateManagedWidget("Weight 0", oblongButtonWidgetClass, rubbertile,
                XtNweight, 0, (String) 0);

        XtVaCreateManagedWidget("Weight 1", oblongButtonWidgetClass, rubbertile,
                XtNweight, 1, (String) 0);

        XtVaCreateManagedWidget("Weight 2", oblongButtonWidgetClass, rubbertile,
                XtNweight, 2, (String) 0);

	/*  Use another RubberTile to keep the OK button centered. */
        rubbertile2 = XtVaCreateManagedWidget("rubberTile2",
		rubberTileWidgetClass, rubbertile,
		XtNweight, 0,
		XtNorientation, OL_HORIZONTAL,
		XtNshadowThickness, 0,
		(String) 0);

	XtVaCreateManagedWidget("spacer", rectObjClass, rubbertile2,
		XtNweight, 1,
		XtNwidth, 30,
		XtNheight, 5,
		(String) 0);

        XtVaCreateManagedWidget("ok",
		flatButtonsWidgetClass, rubbertile2,
		XtNitems, ok_items,
		XtNnumItems, XtNumber(ok_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
		XtNclientData, popup,
		XtNweight, 0,
                (String) 0);

	XtVaCreateManagedWidget("spacer", rectObjClass, rubbertile2,
		XtNweight, 1,
		XtNwidth, 30,
		XtNheight, 5,
		(String) 0);

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
                        OL_DISK_SOURCE, "RubberTile.c");

        XtPopup(popup, XtGrabNone);
}  /* end of RubberTileCB() */
