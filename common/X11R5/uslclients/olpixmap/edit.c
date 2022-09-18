/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olpixmap:edit.c	1.26"
#endif

#include "pixmap.h"
#include <MenuShell.h>
#include "error.h"
#include "Xol/Error.h"  


extern void		(*CurrentDrawOp)();
extern XtPointer	CurrentData;
extern Widget		ColorMenuShell;

extern void		RectanglesEventHandler();
extern void		RubberBandRectangle();
extern Widget		AddMenu();
extern void		PopupMenu();


Bool		Editing = False;


static Widget	EditMenuPane;
static Widget	EditMenuShell;
static Widget	PreviousFocusWidget = NULL;
static Bool	Filledness;
static Pixel	Foreground;

#ifdef out
static void	SetRotate();
static void	SetReflect();
#endif
static void	Fill();
static void	Recolor();
static void	Clear();
static void	Mopy();
static void	Roll();
static void	FillEventHandler();
static void	RecolorEventHandler();
static void	ClearEventHandler();
static void	MopyEventHandler1();
static void	MopyEventHandler2();
static void	RollEventHandler();


#define NUL (XtPointer)0
static MenuItem items[] = {
    {(XtArgVal)NULL,  (XtArgVal)Fill,	(XtArgVal)NUL,          
		(XtArgVal)True, (XtArgVal)True, (XtArgVal)NUL, (XtArgVal)'F'},
    {(XtArgVal)NULL,  (XtArgVal)Recolor,	(XtArgVal)NUL,
		(XtArgVal)True, (XtArgVal)True, (XtArgVal)NUL, (XtArgVal)'R'},
    {(XtArgVal)NULL,  (XtArgVal)Clear,	(XtArgVal)NUL,
		(XtArgVal)True, (XtArgVal)True, (XtArgVal)NUL, (XtArgVal)'e'},
    {(XtArgVal)NULL,  (XtArgVal)Mopy,	(XtArgVal)False, (XtArgVal)True,
			 (XtArgVal)True, (XtArgVal)NUL, (XtArgVal)'C'},
    {(XtArgVal)NULL,  (XtArgVal)Mopy,	(XtArgVal)True,  (XtArgVal)True,
			 (XtArgVal)True, (XtArgVal)NUL, (XtArgVal)'M'},
    {(XtArgVal)NULL,  (XtArgVal)Roll,	(XtArgVal)NUL,
		 (XtArgVal)True, (XtArgVal)True, (XtArgVal)NUL, (XtArgVal)'o'},
};

static Menu menu = {
	"Edit",
	items,
	XtNumber(items),
	True,
	OL_FIXEDCOLS,
	OL_OUT
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

Widget
SetEdit(parent)
Widget parent;
{
	Display *dsp = XtDisplay(parent);

	/* Fill in labels and mnemonics  in items[] */
	items[0].label = 
			(XtArgVal)GETMESS(OleTfill,OleMfixedString_fill);
	items[0].mnemonic = 
			 (XtArgVal)*(GETMNEM(OleTfill,OleMmnemonic_fill));
	items[1].label = 
			(XtArgVal)GETMESS(OleTrecolor,OleMfixedString_recolor);
	items[1].mnemonic = 
			 (XtArgVal)*(GETMNEM(OleTrecolor,OleMmnemonic_recolor));
	items[2].label = 
			(XtArgVal)GETMESS(OleTclear,OleMfixedString_clear);
	items[2].mnemonic = 
			 (XtArgVal)*(GETMNEM(OleTclear,OleMmnemonic_clear));
	items[3].label = 
			(XtArgVal)GETMESS(OleTcopy,OleMfixedString_copy);
	items[3].mnemonic = 
			 (XtArgVal)*(GETMNEM(OleTcopy,OleMmnemonic_copy));
	items[4].label = 
			(XtArgVal)GETMESS(OleTmove,OleMfixedString_move);
	items[4].mnemonic = 
			 (XtArgVal)*(GETMNEM(OleTmove,OleMmnemonic_move));
	items[5].label = 
			(XtArgVal)GETMESS(OleTroll,OleMfixedString_roll);
	items[5].mnemonic = 
		(XtArgVal)	 *(GETMNEM(OleTroll,OleMmnemonic_roll));

 	return EditMenuShell = AddMenu (parent, &menu);
}

extern Widget ButtonArea;

static void
PreEdit(wid)
Widget wid;
{
	Editing = True;
	PreviousFocusWidget = wid;
	XtSetSensitive(EditMenuShell, False);
	XtSetSensitive(ButtonArea, False);
	XtRemoveEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask,
					False, CurrentDrawOp, CurrentData);
}


static void
PostEdit()
{
	FooterMessage(NULL, False);
	XtSetSensitive(EditMenuShell, True);
	XtSetSensitive(ButtonArea, True);
	if (PreviousFocusWidget &&
	    OlCanAcceptFocus(EditMenuShell, CurrentTime))
		OlSetInputFocus(PreviousFocusWidget, RevertToParent,
								CurrentTime);
	Editing = False;
}


static void
Fill(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	char *msg;

	PreEdit(wid);
	XtAddEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask, False,
					FillEventHandler, (XtPointer) 0);

	msg = OlGetMessage(XtDisplay(wid), NULL, 0,
		     OleNfooterMsg,
		     OleTfill, 
		     OleCOlClientOlpixmapMsgs,
		     OleMfooterMsg_fill,
		     (XrmDatabase)NULL);
	
	FooterMessage(msg, False);
}


static XImage *	image;
static XPoint	min_bound,
		max_bound;
static Pixel	pixel_value;

static void
FloodOne(x, y)
Position x;
Position y;
{
	if (x >= 0 && x < (int) PixmapWidth &&
	    y >= 0 && y < (int) PixmapHeight &&
	    XGetPixel(image, (int)x, (int)y) == pixel_value) {
		if (x < min_bound.x)
			min_bound.x = x;
		else if (x > max_bound.x)
			max_bound.x = x;
		if (y < min_bound.y)
			min_bound.y = y;
		else if (y > max_bound.y)
			max_bound.y = y;
		XPutPixel(image, (int)x, (int)y, CurrentForeground);
		FloodOne( (Position)(x + 1), y );
		FloodOne( x, (Position)(y + 1) );
		FloodOne( (Position)(x - 1), y );
		FloodOne( x, (Position)(y - 1) );
	}
}


static void
FillEventHandler(canvas, data, ev, cont_to_dispatch)
Widget canvas;
XtPointer data;
XEvent *ev;
Boolean *cont_to_dispatch;
{
	OlVirtualEventRec	ve;

	if (ev->type != ButtonPress)
		return;

	OlLookupInputEvent(canvas, ev, &ve, OL_DEFAULT_IE);
	switch (ve.virtual_name) {

	case OL_SELECT:
		SetStatus(Busy);
		XtRemoveEventHandler(Canvas,
				ButtonPressMask | ButtonReleaseMask, False,
				FillEventHandler, (XtPointer) 0);
		XtAddEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask,
				False, CurrentDrawOp, CurrentData);

		image = XGetImage(DISPLAY, RealPixmap, 0, 0,
				PixmapWidth, PixmapHeight, AllPlanes, ZPixmap);
		ShrinkPixel(ev->xbutton.x, ev->xbutton.y, &min_bound);
		max_bound = min_bound;
		pixel_value = XGetPixel(image,
					(int)min_bound.x, (int)min_bound.y);

		if (pixel_value != CurrentForeground) {
			FloodOne((Position)min_bound.x, (Position)min_bound.y);
			XPutImage(DISPLAY, RealPixmap, DrawGC, image, 0, 0,
					0, 0, PixmapWidth, PixmapHeight);
			Changed = True;
			RefreshCanvas(&min_bound, &max_bound);
			if (PixmapIsDisplayed)
			    RefreshPixmapDisplay(
				(int)min_bound.x, (int)min_bound.y,
				(unsigned int)(max_bound.x - min_bound.x + 1),
				(unsigned int)(max_bound.y - min_bound.y + 1));
		}
		XDestroyImage(image);

		PostEdit();
		SetStatus(Normal);
		break;

	case OL_MENU:
		PopupMenu (
                        ColorMenuShell, Canvas, NULL,
                        ev->xbutton.x_root, ev->xbutton.y_root
		);
		break;

	default:
		break;
	}
}


static void
Recolor(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	char *msg;

	PreEdit(wid);
	XtAddEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask, False,
					RecolorEventHandler, (XtPointer) 0);

	msg = OlGetMessage(XtDisplay(wid), NULL, 0,
		     OleNfooterMsg,
		     OleTrecolor,
		     OleCOlClientOlpixmapMsgs,
		     OleMfooterMsg_recolor,
		     (XrmDatabase)NULL);

	FooterMessage(msg, False);
}


static void
RecolorEventHandler(canvas, data, ev, cont_to_dispatch)
Widget canvas;
XtPointer data;
XEvent *ev;
Boolean *cont_to_dispatch;
{
	OlVirtualEventRec	ve;
	int			x, y;

	if (ev->type != ButtonPress)
		return;

	OlLookupInputEvent(canvas, ev, &ve, OL_DEFAULT_IE);
	switch (ve.virtual_name) {

	case OL_SELECT:
		SetStatus(Busy);
		XtRemoveEventHandler(Canvas,
				ButtonPressMask | ButtonReleaseMask, False,
				RecolorEventHandler, (XtPointer) 0);
		XtAddEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask,
				False, CurrentDrawOp, CurrentData);

		image = XGetImage(DISPLAY, RealPixmap, 0, 0,
				PixmapWidth, PixmapHeight, AllPlanes, ZPixmap);
		ShrinkPixel(ev->xbutton.x, ev->xbutton.y, &min_bound);
		max_bound = min_bound;
		pixel_value = XGetPixel(image,
					(int)min_bound.x, (int)min_bound.y);

		if (pixel_value != CurrentForeground) {
			for (x = 0; x < (int) PixmapWidth; x++) {
			    for (y = 0; y < (int) PixmapHeight; y++) {
				if (XGetPixel(image, x, y) == pixel_value) {
				    XPutPixel(image, x, y, CurrentForeground);
				    if (x < min_bound.x)
					min_bound.x = x;
				    else if (x > max_bound.x)
					max_bound.x = x;
				    if (y < min_bound.y)
					min_bound.y = y;
				    else if (y > max_bound.y)
					max_bound.y = y;
				}
			    }
			}
			XPutImage(DISPLAY, RealPixmap, DrawGC, image, 0, 0,
					0, 0, PixmapWidth, PixmapHeight);
			Changed = True;
			RefreshCanvas(&min_bound, &max_bound);
			if (PixmapIsDisplayed)
			    RefreshPixmapDisplay(
				(int)min_bound.x, (int)min_bound.y,
				(unsigned int)(max_bound.x - min_bound.x + 1),
				(unsigned int)(max_bound.y - min_bound.y + 1));
		}
		XDestroyImage(image);

		PostEdit();
		SetStatus(Normal);
		break;

	case OL_MENU:
		PopupMenu (
                        ColorMenuShell, Canvas, NULL,
                        ev->xbutton.x_root, ev->xbutton.y_root
		);
		break;

	default:
		break;
	}
}


static void
Clear(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	char *msg;

	PreEdit(wid);
	XtAddEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask, False,
					ClearEventHandler, (XtPointer) 0);

	msg = OlGetMessage(XtDisplay(wid), NULL, 0,
		     OleNfooterMsg,
		     OleTclear, 
		     OleCOlClientOlpixmapMsgs,
		     OleMfooterMsg_clear,
		     (XrmDatabase)NULL);
	
	FooterMessage(msg, False);
}


static void
ClearEventHandler(canvas, data, ev, cont_to_dispatch)
Widget canvas;
XtPointer data;
XEvent *ev;
Boolean *cont_to_dispatch;
{
	OlVirtualEventRec	ve;

	Foreground = CurrentForeground;
	CurrentForeground = CurrentBackground;
	XSetForeground(DISPLAY, DrawGC, CurrentBackground);
	Filledness = DrawFilled;
	DrawFilled = True;
	RectanglesEventHandler(canvas, (XtPointer) False, ev);

	OlLookupInputEvent(canvas, ev, &ve, OL_DEFAULT_IE);
	if (ev->type == ButtonPress && ve.virtual_name == OL_SELECT) {
		XtRemoveEventHandler(Canvas,
				ButtonPressMask | ButtonReleaseMask, False,
				ClearEventHandler, (XtPointer) 0);
		XtAddEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask,
				False, CurrentDrawOp, CurrentData);
		Changed = True;
		/*
		 *	Restore original stuff.
		 */
		CurrentForeground = Foreground;
		XSetForeground(DISPLAY, DrawGC, CurrentForeground);
		DrawFilled = Filledness;

		PostEdit();
	}
}


static void
Mopy(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	char *msg;
	Bool	move = (Bool) client_data;

	PreEdit(wid);
	XtAddEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask, False,
					MopyEventHandler1, client_data);

	if (move) {
	  msg = OlGetMessage(XtDisplay(wid), NULL, 0,
		       OleNfooterMsg,
		       OleTmove, 
		       OleCOlClientOlpixmapMsgs,
		       OleMfooterMsg_move,
		       (XrmDatabase)NULL);
	  
	  FooterMessage(msg, False);
	}
	else {
	  msg = OlGetMessage(XtDisplay(wid), NULL, 0,
		       OleNfooterMsg,
		       OleTcopy, 
		       OleCOlClientOlpixmapMsgs,
		       OleMfooterMsg_copy,
		       (XrmDatabase)NULL);
	
	  FooterMessage(msg, False);
	}
}


static XPoint		source;
static int		width,
			height;

static void
MopyEventHandler1(canvas, data, ev, cont_to_dispatch)
Widget canvas;
XtPointer data;
XEvent *ev;
Boolean *cont_to_dispatch;
{
	char			*msg;
	Bool			move = (Bool) data;
	XPoint			start_point,
				end_point;
	OlVirtualEventRec	ve;

	if (ev->type != ButtonPress)
		return;

	OlLookupInputEvent(canvas, ev, &ve, OL_DEFAULT_IE);
	switch (ve.virtual_name) {

	case OL_SELECT:
		start_point.x = ev->xbutton.x;
		start_point.y = ev->xbutton.y;

		RubberBandRectangle(canvas, &start_point, &end_point, False);
		ShrinkPixel(ev->xbutton.x, ev->xbutton.y, &start_point);

		XtRemoveEventHandler(Canvas,
				ButtonPressMask | ButtonReleaseMask, False,
				MopyEventHandler1, data);
		XtAddEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask,
				False, MopyEventHandler2, data);

		source.x = MIN(start_point.x, end_point.x);
		source.y = MIN(start_point.y, end_point.y);
		width = MAX(start_point.x, end_point.x) - source.x + 1;
		height = MAX(start_point.y, end_point.y) - source.y + 1;

		if (move) {
		  msg = OlGetMessage(XtDisplay(canvas), NULL, 0,
			       OleNfooterMsg,
			       OleTmove2, 
			       OleCOlClientOlpixmapMsgs,
			       OleMfooterMsg_move2,
			       (XrmDatabase)NULL);
		  
		  FooterMessage(msg, False);
		}
		else {
		  msg = OlGetMessage(XtDisplay(canvas), NULL, 0,
			       OleNfooterMsg,
			       OleTcopy2,
			       OleCOlClientOlpixmapMsgs,
			       OleMfooterMsg_copy2,
			       (XrmDatabase)NULL);
		  
		  FooterMessage(msg, False);
		}
		break;

	case OL_MENU:
		PopupMenu (
                        ColorMenuShell, Canvas, NULL,
                        ev->xbutton.x_root, ev->xbutton.y_root
		);
		break;

	default:
		break;
	}
}


static void
MopyEventHandler2(canvas, data, ev, cont_to_dispatch)
Widget canvas;
XtPointer data;
XEvent *ev;
Boolean *cont_to_dispatch;
{
	Bool			move = (Bool) data;
	XPoint			dest,
				r_origin,
				r_corner;
	OlVirtualEventRec	ve;

	if (ev->type != ButtonPress)
		return;

	OlLookupInputEvent(canvas, ev, &ve, OL_DEFAULT_IE);
	switch (ve.virtual_name) {

	case OL_SELECT:
		XtRemoveEventHandler(Canvas,
				ButtonPressMask | ButtonReleaseMask, False,
				MopyEventHandler2, data);
		XtAddEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask,
				False, CurrentDrawOp, CurrentData);

		ShrinkPixel(ev->xbutton.x, ev->xbutton.y, &dest);

		if (!move) {
			XCopyArea(DISPLAY, RealPixmap, RealPixmap, DrawGC,
					source.x, source.y, width, height,
					dest.x, dest.y);
			r_origin = dest;
			r_corner.x = r_origin.x + width - 1;
			r_corner.y = r_origin.y + height - 1;
		} else {
			Pixmap	tmp_pixmap;

			tmp_pixmap = XCreatePixmap(DISPLAY, ROOT,
						width, height, PixmapDepth);
			XSetForeground(DISPLAY, DrawGC, CurrentBackground);
			XFillRectangle(DISPLAY, tmp_pixmap, DrawGC,
							0, 0, width, height);
			XCopyArea(DISPLAY, RealPixmap, tmp_pixmap, DrawGC,
				source.x, source.y, width, height, 0, 0);
			XFillRectangle(DISPLAY, RealPixmap, DrawGC,
					source.x, source.y, width, height);
			XSetForeground(DISPLAY, DrawGC, CurrentForeground);
			XCopyArea(DISPLAY, tmp_pixmap, RealPixmap, DrawGC,
					0, 0, width, height, dest.x, dest.y);
			XFreePixmap(DISPLAY, tmp_pixmap);

			r_origin.x = MIN(source.x, dest.x);
			r_origin.y = MIN(source.y, dest.y);
			width += ABS(dest.x - source.x);
			height += ABS(dest.y - source.y);
			r_corner.x = r_origin.x + width - 1;
			r_corner.y = r_origin.y + height - 1;
		}
		Changed = True;
		RefreshCanvas(&r_origin, &r_corner);
		if (PixmapIsDisplayed)
			RefreshPixmapDisplay((int)r_origin.x, (int)r_origin.y,
				(unsigned int)width, (unsigned int)height);

		PostEdit();
		break;

	case OL_MENU:
		PopupMenu (
                        ColorMenuShell, Canvas, NULL,
                        ev->xbutton.x_root, ev->xbutton.y_root
		);
		break;

	default:
		break;
	}
}


static void
Roll(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	char	*msg;

	PreEdit(wid);
	XtAddEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask, False,
					RollEventHandler, (XtPointer) 0);

	msg = OlGetMessage(XtDisplay(wid), NULL, 0,
		     OleNfooterMsg,
		     OleTroll,
		     OleCOlClientOlpixmapMsgs,
		     OleMfooterMsg_roll,
		     (XrmDatabase)NULL);
	
	FooterMessage(msg, False);
}


static void
RollEventHandler(canvas, data, ev, cont_to_dispatch)
Widget canvas;
XtPointer data;
XEvent *ev;
Boolean *cont_to_dispatch;
{
	XPoint			new;
	Pixmap			tmp_pixmap;
	OlVirtualEventRec	ve;

	if (ev->type != ButtonPress)
		return;

	OlLookupInputEvent(canvas, ev, &ve, OL_DEFAULT_IE);
	switch (ve.virtual_name) {

	case OL_SELECT:
		XtRemoveEventHandler(Canvas,
				ButtonPressMask | ButtonReleaseMask, False,
				RollEventHandler, (XtPointer) 0);
		XtAddEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask,
				False, CurrentDrawOp, CurrentData);

		ShrinkPixel(ev->xbutton.x, ev->xbutton.y, &new);

		tmp_pixmap = XCreatePixmap(DISPLAY, ROOT,
				PixmapWidth, PixmapHeight, PixmapDepth);
		XCopyArea(DISPLAY, RealPixmap, tmp_pixmap, DrawGC,
				new.x, 0, PixmapWidth - new.x, PixmapHeight,
				0, 0);
		XCopyArea(DISPLAY, RealPixmap, tmp_pixmap, DrawGC,
				0, 0, new.x, PixmapHeight,
				PixmapWidth - new.x, 0);
		XCopyArea(DISPLAY, tmp_pixmap, RealPixmap, DrawGC,
				0, new.y, PixmapWidth, PixmapHeight - new.y,
				0, 0);
		XCopyArea(DISPLAY, tmp_pixmap, RealPixmap, DrawGC,
				0, 0, PixmapWidth, new.y,
				0, PixmapHeight - new.y);
		XFreePixmap(DISPLAY, tmp_pixmap);
		Changed = True;
		RefreshAllVisuals();

		PostEdit();
		break;

	case OL_MENU:
      		PopupMenu (
      			ColorMenuShell, Canvas, NULL,
      			ev->xbutton.x_root, ev->xbutton.y_root
      		);
		break;

	default:
		break;
	}
}
