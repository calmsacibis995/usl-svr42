/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olpixmap:initialize.c	1.36"
#endif

#include "pixmap.h"
#include "error.h"
#include <Xol/Error.h>  
#include <DnD/OlDnDVCX.h>
#include <X11/Xatom.h>
#include <Form.h>
#include <FooterPane.h>
#include <ControlAre.h>
#include <StaticText.h>
#include <Scrollbar.h>
#include <ScrolledWi.h>
#include <Stub.h>
#include <Shell.h>

#include "olpixmap.xpm"
#include "olpixmask.xbm"


extern Widget	AddMenu ();
extern Widget	SetFile();
extern Widget	SetView();
extern Widget	SetEdit();
extern Widget	SetDraw();
extern Widget	SetPalette();
extern void	SetProperties();

extern void	InitializePixmapPopup();
extern void	WindowManagerEventHandler();
extern void	CanvasExpose();
extern void	PixelEventHandler();
extern void	FilePopupCallback();
extern void	PropertiesPopupCallback();

extern Widget	FilePopup;
Widget		Canvas;
Widget		ScrolledWindow;
Widget		ButtonArea;


static void	InitializeContext();
static Widget	InitializeFooterPanel();
static Widget	InitializeControlArea();
static Widget	InitializeFooter();
static Widget	InitializeScrolledWindow();
static Widget	InitializeCanvas();
static Widget	InitializeControls();
static void	InitializeIcon();

static Boolean	PIXTriggerNotify OL_ARGS((Widget, Window, Position,
					  Position, Atom, Time,
					  OlDnDDropSiteID,
					  OlDnDTriggerOperation, Boolean,
					  Boolean, XtPointer));
static void	PIXSelectionCB OL_ARGS((Widget, XtPointer, Atom *, Atom *,
					XtPointer, unsigned long *, int *));

Widget
InitializeAllVisuals(toplevel)
Widget toplevel;
{
	Widget	control_area,
		footer_panel,
		footer;

	InitializeContext();
	footer_panel = InitializeFooterPanel(toplevel);
	control_area = InitializeControlArea(footer_panel);
	footer = InitializeFooter(footer_panel);
	ButtonArea = InitializeControls(toplevel, control_area);
	ScrolledWindow = InitializeScrolledWindow(control_area);
	Canvas = InitializeCanvas(ScrolledWindow);

	InitializePixmapPopup(toplevel);
	InitializeIcon(toplevel);
#if 0
		/* don't know why this, WindowManagerEventHandler
		 * actually is a CB. Maybe a problem if we don't
		 * do this...
		 */
	XtAddEventHandler(toplevel, NoEventMask, True,
				WindowManagerEventHandler, (XtPointer) 0);
#endif

 	INIT_ARGS();
	SET_ARGS(XtNresizeCorners, False);
	SET_ARGS(XtNwmProtocolInterested,
				(OL_WM_DELETE_WINDOW | OL_WM_SAVE_YOURSELF));
	SET_VALUES(toplevel);
	END_ARGS();
	OlAddCallback(toplevel, XtNwmProtocol,
				WindowManagerEventHandler, (XtPointer) 0);

	XtRealizeWidget(toplevel);

	XStoreName(DISPLAY, XtWindow(toplevel), ApplicationName);

	return footer;
}


static void
InitializeContext()
{
	PixmapWidth = DEFAULT_WIDTH;
	PixmapHeight = DEFAULT_HEIGHT;
	/*
	 *	Make the on-screen and internal representation of a
	 *	pixmap the same.  Presumably, the default depth of
	 *	the display is the maximal depth that it can support,
	 *	thus giving us the most colors to work with.  The
	 *	depth is significant for only the inner workings of
	 *	this application; the external interface (i.e., the
	 *	file format) is essentially depth independent.
	 */
	PixmapDepth = DefaultDepthOfScreen(SCREEN);
	/*
	 *	For now, just use the default colormap of the screen.
	 */
	PixmapColormap = DefaultColormapOfScreen(SCREEN);

	CurrentForeground = BlackPixelOfScreen(SCREEN);
	CurrentBackground = WhitePixelOfScreen(SCREEN);
	ResetCursorColors();
}


static Widget
InitializeFooterPanel(parent)
Widget parent;
{
	Widget		footer_panel;

	INIT_ARGS();
	footer_panel = CREATE_MANAGED("footer_panel", footerPanelWidgetClass,
								parent);
	END_ARGS();

	return footer_panel;
}


static Widget
InitializeControlArea(parent)
Widget parent;
{
	Widget	control_area;		/* it's actually a Form Widget */

	INIT_ARGS();
	SET_ARGS(XtNborderWidth, 0);
	SET_ARGS(XtNshadowThickness, 0);
	control_area = CREATE_MANAGED("control_area", formWidgetClass,
								parent);
	END_ARGS();

	return control_area;
}


static Widget
InitializeFooter(parent)
Widget parent;
{
	Widget	footer;

	INIT_ARGS();
	SET_ARGS(XtNborderWidth, 0);
	SET_ARGS(XtNgravity, WestGravity);
	footer = CREATE_MANAGED("footer", staticTextWidgetClass, parent);
	END_ARGS();

	return footer;
}


static Widget
InitializeScrolledWindow(parent)
Widget parent;
{
	Widget		scrolled_window;
	Widget		hscrollbar;
	Widget		vscrollbar;

	INIT_ARGS();
	scrolled_window = CREATE_MANAGED("scrolled_window",
					scrolledWindowWidgetClass, parent);
	END_ARGS();

	INIT_ARGS();
	SET_ARGS(XtNhScrollbar, &hscrollbar);
	SET_ARGS(XtNvScrollbar, &vscrollbar);
	GET_VALUES(scrolled_window);
	END_ARGS();

	INIT_ARGS();
	SET_ARGS(XtNstopPosition, OL_GRANULARITY);
	SET_VALUES(hscrollbar);
	SET_VALUES(vscrollbar);
	END_ARGS();
	
	return scrolled_window;
}


static Widget
InitializeCanvas(parent)
Widget parent;
{
	Widget		canvas;

	INIT_ARGS();
	SET_ARGS(XtNwidth, 1);
	SET_ARGS(XtNheight, 1);
	SET_ARGS(XtNexpose, CanvasExpose);
	SET_ARGS(XtNborderWidth, 0);
	canvas = CREATE_MANAGED("canvas", stubWidgetClass, parent);
	END_ARGS();

	OlDnDRegisterDDI(canvas, OlDnDSitePreviewNone, PIXTriggerNotify,
			 (OlDnDPMNotifyProc)NULL, True, NULL);

	XtAddEventHandler(canvas, ButtonPressMask | ButtonReleaseMask, False,
					PixelEventHandler, (XtPointer) 0);

	return canvas;
}

#define NUL (XtPointer)0
#define FILE	0
#define VIEW	1+offset
#define EDIT	2-offset
#define DRAW	3
#define PALETTE 4
#define PROP	5
static MenuItem items[] = {
    {(XtArgVal)NULL, (XtArgVal)0, (XtArgVal)NUL,
	(XtArgVal)True, (XtArgVal)True, (XtArgVal)NUL, (XtArgVal)'F'},
    {(XtArgVal)NULL, (XtArgVal)0, (XtArgVal)NUL, (XtArgVal)True,
		 (XtArgVal)True, (XtArgVal)NUL, (XtArgVal)'V'},
    {(XtArgVal)NULL, (XtArgVal)0, (XtArgVal)NUL, (XtArgVal)True,
		(XtArgVal)True, (XtArgVal)NUL, (XtArgVal)'E'},
    {(XtArgVal)NULL, (XtArgVal)0, (XtArgVal)NUL, (XtArgVal)True,
		 (XtArgVal)True, (XtArgVal)NUL, (XtArgVal)'D'},
    {(XtArgVal)NULL, (XtArgVal)0, (XtArgVal)NUL, (XtArgVal)True,
		 (XtArgVal)True, (XtArgVal)NUL, (XtArgVal)'P'},
    {(XtArgVal)NULL,(XtArgVal)PropertiesPopupCallback, (XtArgVal)NUL,
		(XtArgVal)True, (XtArgVal)True, (XtArgVal)NUL, (XtArgVal)'O'},
};
static Menu menu = {
	"properties",
	items,
	XtNumber(items),
	False,
	OL_FIXEDROWS,
	OL_NONE
};

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


static Widget
InitializeControls(toplevel, parent)
Widget toplevel;
Widget parent;
{
	Widget	button_area,
		button,
		button_stack;
	Display *dsp = XtDisplay(toplevel);
	OlDefine layout;
	short offset = (OlGetGui() == OL_OPENLOOK_GUI) ? 0 : 1;

	INIT_ARGS();
	SET_ARGS(XtNborderWidth, 0);
	SET_ARGS(XtNmeasure, 1);
	SET_ARGS(XtNxAttachRight, True);
	SET_ARGS(XtNxResizable, True);
	button_area = CREATE_MANAGED("button_area", controlAreaWidgetClass,
								parent);
	END_ARGS();


	items[FILE].popup = (XtArgVal)SetFile(Toplevel);
        items[VIEW].popup = (XtArgVal)SetView(Toplevel);
	items[EDIT].popup = (XtArgVal)SetEdit(Toplevel);
	items[DRAW].popup = (XtArgVal)SetDraw(Toplevel);
	items[PALETTE].popup = (XtArgVal)SetPalette(Toplevel);

	items[FILE].label = 
			(XtArgVal)GETMESS(OleTfile,OleMfixedString_file);
	items[VIEW].label = 
			(XtArgVal)GETMESS(OleTview,OleMfixedString_view);
	items[EDIT].label = 
			(XtArgVal)GETMESS(OleTedit,OleMfixedString_edit);
	items[DRAW].label = 
			(XtArgVal)GETMESS(OleTdraw,OleMfixedString_draw);
	items[PALETTE].label = 
			(XtArgVal)GETMESS(OleTpalette,OleMfixedString_palette);
	items[PROP].label = (OlGetGui() == OL_MOTIF_GUI ?
			(XtArgVal)GETMESS(OleToptions,OleMfixedString_options) :
			(XtArgVal)GETMESS(OleTproperties,OleMfixedString_properties));

	items[FILE].mnemonic =
			 (XtArgVal)*(GETMNEM(OleTfile,OleMmnemonic_file));
	items[VIEW].mnemonic =
			 (XtArgVal)*(GETMNEM(OleTview,OleMmnemonic_view));
	items[EDIT].mnemonic =
			 (XtArgVal)*(GETMNEM(OleTedit,OleMmnemonic_edit));
	items[DRAW].mnemonic =
			 (XtArgVal)*(GETMNEM(OleTdraw,OleMmnemonic_draw));
	items[PALETTE].mnemonic =
			 (XtArgVal)*(GETMNEM(OleTpalette,OleMmnemonic_palette));
	items[PROP].mnemonic = (OlGetGui() == OL_MOTIF_GUI ?
			 (XtArgVal)*(GETMNEM(OleToptions,OleMmnemonic_options)) :
			 (XtArgVal)*(GETMNEM(OleTproperties,OleMmnemonic_properties)));
	
	SetProperties(Toplevel, button);

	XtVaGetValues(button_area,
		      XtNlayoutType, &layout,
		      (String) 0);
	menu.orientation = layout;
	AddMenu (button_area, &menu);

	return button_area;
}


static void
InitializeIcon(toplevel)
Widget toplevel;
{
	Pixmap	icon,
		iconmask;

	icon = XCreatePixmapFromData(DISPLAY, ROOT, PixmapColormap,
			olpixmap_width, olpixmap_height, PixmapDepth,
			olpixmap_ncolors, olpixmap_chars_per_pixel,
			olpixmap_colors, olpixmap_pixels);
	iconmask = XCreateBitmapFromData(DISPLAY, ROOT, (char *)olpixmask_bits,
			olpixmask_width, olpixmask_height);

	INIT_ARGS();
	SET_ARGS(XtNiconPixmap, icon);
	SET_ARGS(XtNiconMask, iconmask);
  	SET_ARGS(XtNiconName, "");
   	SET_VALUES(toplevel);
	END_ARGS();
}

static Boolean
PIXTriggerNotify OLARGLIST((w, win, x, y, selection, timestamp,
			    drop_site_id, op, send_done, forwarded, closure))
  OLARG( Widget,		w)
  OLARG( Window,		win)
  OLARG( Position,		x)
  OLARG( Position,		y)
  OLARG( Atom,			selection)
  OLARG( Time,			timestamp)
  OLARG( OlDnDDropSiteID,	drop_site_id)
  OLARG( OlDnDTriggerOperation,	op)
  OLARG( Boolean,		send_done)
  OLARG( Boolean,		forwarded)	/* not used */
  OLGRA( XtPointer,		closure)
{
	XtGetSelectionValue(
		w, selection, OL_XA_FILE_NAME(XtDisplay(w)),
		PIXSelectionCB, (XtPointer)send_done, timestamp
	);

	return(True);
} /* end of PIXTriggerNotify */

static void
PIXSelectionCB OLARGLIST ((w, client_data, selection, type, value, length,
			   format))
  OLARG( Widget,		w)
  OLARG( XtPointer,		client_data)
  OLARG( Atom *,		selection)
  OLARG( Atom *,		type)
  OLARG( XtPointer,		value)
  OLARG( unsigned long *,	length)
  OLGRA( int *,			format)
{
  Boolean send_done = (Boolean)client_data;
  String fullname;

  /* Since only OL_XA_FILE_NAME(dpy) is passed in, we know we have a
     valid type.
   */

  fullname = (String) value;

  if (OpenFile(fullname))
    BringDownPopup(FilePopup);

  /* Errors handled by OpenFile.  We don't care if there was an error.  */
  /* The transaction is done regardless. */

  XtFree(value);

  if (send_done == True) {
    OlDnDDragNDropDone(w, *selection, CurrentTime, NULL, NULL);
  }
} /* end of PIXSelectionCB */


