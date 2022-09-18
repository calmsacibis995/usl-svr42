/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olpixmap:popup.c	1.18"
#endif

#include "pixmap.h"
#include "error.h"
#include <Xol/Error.h>  
#include <PopupWindo.h>
#include <Stub.h>
#include <OlCursors.h>


Widget	PixmapPopup;
Widget	PixmapDisplay = (Widget) NULL;
Bool	PixmapIsDisplayed = False;


static GC	ExposeGC = (GC) 0;

static Widget	InitializeUpperControl();
static void	ExposePixmapDisplay();
static void	PixmapPopupCallback();
static void	PixmapPopdownCallback();
static void	PixmapDoPopdown();

#define NUL	(XtPointer)0

void
ShowPixmap(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	if (PixmapIsDisplayed) {
		XRaiseWindow(DISPLAY, XtWindow(PixmapPopup));
	} else {
		INIT_ARGS();
		SET_ARGS(XtNpushpin, OL_IN);
		SET_VALUES(PixmapPopup);
		END_ARGS();

		XtPopup(PixmapPopup, XtGrabNone);
		XDefineCursor(DISPLAY, XtWindow(PixmapPopup),
			      OlGetStandardCursor(PixmapPopup));
	}
}

static char *Labels[1];

#define GETMESS(a,b)	 OlGetMessage(dsp, NULL, 0, \
		     OleNfixedString, \
		     a,  \
		     OleCOlClientOlpixmapMsgs, \
		     b, \
		     (XrmDatabase)NULL)

#define GETMNEM(a,b)	 OlGetMessage(dsp, NULL, 0, \
		     OleNmnemonic, \
		     a,  \
		     OleCOlClientOlpixmapMsgs, \
		     b, \
		     (XrmDatabase)NULL)

static MenuItem pixmap_items[] = {
  {(XtArgVal)NULL, (XtArgVal)PixmapDoPopdown, (XtArgVal)NUL, (XtArgVal)True,
     (XtArgVal)True, (XtArgVal)NUL}
};

static Menu pixmap_menu = {
  "pixmap",
  pixmap_items,
  XtNumber(pixmap_items),
  False,
  OL_FIXEDROWS,
  OL_NONE,
};

void
InitializePixmapPopup(parent)
Widget parent;
{
	Widget	upper_control, lower_control;
	Display *dsp = XtDisplay(parent);
	Cardinal n = 0;

	Labels[0] = GETMESS(OleTpixmap,OleMfixedString_pixmap);

	INIT_ARGS();
	SET_ARGS(XtNtitle, Labels[0]);
	PixmapPopup = CREATE_POPUP("PixmapPopup",
					popupWindowShellWidgetClass, parent);
	END_ARGS();

	INIT_ARGS();
	SET_ARGS(XtNlowerControlArea, &lower_control);
	GET_VALUES(PixmapPopup);
	END_ARGS();

	pixmap_items[0].label =
	  (XtArgVal)GETMESS(OleTcancel,OleMfixedString_cancel);
	pixmap_items[0].mnemonic =
	  (XtArgVal)*(GETMNEM(OleTcancel,OleMmnemonic_cancel));
	
	(void)AddMenu(lower_control, &pixmap_menu);
	
	XtAddCallback(PixmapPopup, XtNpopupCallback,
					PixmapPopupCallback, (XtPointer) 0);
	XtAddCallback(PixmapPopup, XtNpopdownCallback,
					PixmapPopdownCallback, (XtPointer) 0);

	INIT_ARGS();
	SET_ARGS(XtNupperControlArea, &upper_control);
	GET_VALUES(PixmapPopup);
	END_ARGS();

	PixmapDisplay = InitializeUpperControl(upper_control);
}


static Widget
InitializeUpperControl(parent)
Widget parent;
{
	Widget	pixmap_display;

	INIT_ARGS();
	SET_ARGS(XtNborderWidth, 1);
	SET_ARGS(XtNwidth, PixmapWidth);
	SET_ARGS(XtNheight, PixmapHeight);
	SET_ARGS(XtNexpose, ExposePixmapDisplay);
	pixmap_display = CREATE_MANAGED("pixmap_display",
						stubWidgetClass, parent);
	END_ARGS();

	return pixmap_display;
}


static void
ExposePixmapDisplay(wid, ev, region)
Widget wid;
XEvent *ev;
Region region;
{
	RefreshPixmapDisplay(ev->xexpose.x, ev->xexpose.y,
				ev->xexpose.width, ev->xexpose.height);
}


void
RefreshPixmapDisplay(x, y, width, height)
int x;
int y;
unsigned int width;
unsigned int height;
{
	if (ExposeGC == (GC) 0)
	{
		XGCValues	gcv;

		gcv.graphics_exposures = False;
		ExposeGC = XtGetGC(PixmapDisplay, GCGraphicsExposures, &gcv);
	}

	XCopyArea(DISPLAY, RealPixmap, XtWindow(PixmapDisplay), ExposeGC,
					x, y, width, height, x, y);
	XFlush(DISPLAY);
}


static void
PixmapPopupCallback(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	PixmapIsDisplayed = True;
}


static void
PixmapPopdownCallback(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	PixmapIsDisplayed = False;
}


static void
PixmapDoPopdown(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
  XtPopdown((Widget)_OlGetShellOfWidget(wid));
}
