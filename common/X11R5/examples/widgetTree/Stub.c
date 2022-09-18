/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:Stub.c	1.1"
#endif

/*  Example of the Stub widget that implements a horizontal seperator.  */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <Xol/OpenLook.h>
#include <Xol/RubberTile.h>
#include <Xol/Caption.h>
#include <Xol/FButtons.h>
#include <Xol/Olg.h>
#include <Xol/Stub.h>

#include "WidgetTree.h"

static item ok_items[] = {
	{ (XtPointer) "OK", (XtPointer) PopdownCB},
};

/*ARGSUSED*/
static void
SeperatorExpose OLARGLIST((w, xevent, region))
	OLARG(Widget, w)
	OLARG(XEvent *, xevent)
	OLGRA(Region, region)
{
	Dimension width, height;
	OlgAttrs * pInfo;
	Position half;

	XtVaGetValues(w, XtNwidth, &width,
		XtNheight, &height, XtNuserData, &pInfo, 0);
	
	half = height / 2;
	if ((Dimension)half > height) half = 0;

	(void) OlgDrawLine(XtScreen(w), XtWindow(w), pInfo,
		0,		/* x coordinate */
		half,		/* y is centered in the widget's height */
		width,		/* line length is width of widget */
		2,		/* line thickness */
		False);		/* is vertical ? */

}  /* end of SeperatorExpose()  */

/*ARGSUSED*/
static void
SeperatorInitialize OLARGLIST((request, new, args, num_args))
	OLARG(Widget, request)
	OLARG(Widget, new)
	OLARG(ArgList, args)
	OLGRA(Cardinal *, num_args)
{
	OlgAttrs * pInfo;
	Pixel background;

	/*  When the stub is initialized, create the drawing information
	    and save it in the userData field. */
	XtVaGetValues(request, XtNbackground, &background, 0);

	pInfo = OlgCreateAttrs(XtScreen(new),
		(Pixel) 0,		/* foreground */
		(OlgBG *) background,	/* background */
		False,			/* background is a pixel */
		12);			/* point size */

	XtVaSetValues(new, XtNuserData, pInfo, 0);

}  /* end of SeperatorInitialize() */


/*ARGSUSED*/
static void
SeperatorDestroy OLARGLIST((w))
	OLGRA(Widget, w)
{
	OlgAttrs * pInfo;

	XtVaGetValues(w, XtNuserData, &pInfo, 0);

	/*  Clean up the drawing attributes when the stub is destroyed.  */
	OlgDestroyAttrs(pInfo);

}  /*  end of SeperatorDestroy() */

/*ARGSUSED*/
void
StubCB OLARGLIST((w, client_data,  call_data))
        OLARG(Widget, w)
        OLARG(XtPointer, client_data)
        OLGRA(XtPointer, call_data)
{
        Widget popup, rubbertile, caption;

        popup = XtVaCreatePopupShell("transient",
                transientShellWidgetClass, XtParent(w), 
		XtNtitle, "Stub",
		XtNresizeCorners, True,
                (String)0);
	XtAddCallback(popup, XtNpopdownCallback, DestroyCB, w);

	rubbertile = XtVaCreateManagedWidget("rubbertile",
		rubberTileWidgetClass, popup,
		XtNorientation, OL_VERTICAL,
		(String) 0);

	caption = XtVaCreateManagedWidget("caption", captionWidgetClass,
		rubbertile,
		XtNlabel, "Stub:",
                XtNfont, (XFontStruct *) _OlGetDefaultFont(w, OlDefaultBoldFont),
		XtNposition, OL_TOP,
		XtNalignment, OL_LEFT,
		(String) 0);

        XtVaCreateManagedWidget("stub", stubWidgetClass, caption,
		XtNwidth, 100,
		XtNheight, 25,
		XtNinitialize, SeperatorInitialize,
		XtNexpose, SeperatorExpose,
		XtNdestroy, SeperatorDestroy,
		(String) 0);

        OlRegisterHelp(OL_WIDGET_HELP, (XtPointer) popup, NULL,
                        OL_DISK_SOURCE, "Stub.c");

        XtVaCreateManagedWidget("ok",
		flatButtonsWidgetClass, rubbertile,
		XtNgravity, CenterGravity,
		XtNweight, 0,
		XtNitems, ok_items,
		XtNnumItems, XtNumber(ok_items),
		XtNitemFields, item_fields,
		XtNnumItemFields, num_item_fields,
		XtNclientData, popup,
                (String) 0);

        XtPopup(popup, XtGrabNone);
}  /* end of StubCB() */
