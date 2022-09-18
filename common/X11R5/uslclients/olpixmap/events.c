/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olpixmap:events.c	1.16"
#endif

#include "pixmap.h"
#include <MenuShell.h>


extern Widget	ColorMenuShell;

extern void	RubberBandLine();
extern void	RubberBandRectangle();
extern void	PopupMenu();


Bool		StartFreshSegment = True;



void
PixelEventHandler(canvas, data, ev, cont_to_dispatch)
Widget canvas;
XtPointer data;
XEvent *ev;
Boolean *cont_to_dispatch;
{
	Window			root,
				jc;
	XEvent			rsev;
	XPoint			last_point,
				this_point;
	unsigned int		mask;
	int			root_x,
				root_y,
				x,
				y;
	Dimension		width,
				height;
	OlVirtualEventRec	ve;

	static Boolean		empty_footer = False;

	if (canvas == (Widget)NULL)
	{
			/* called by FooterMessage(), meaning that
			 * there are status in footer area and we
			 * should clean it up when sees SELECT
			 * Button Press...
			 */
		empty_footer = True;
		return;
	}

	if (ev->type != ButtonPress)
		return;

	OlLookupInputEvent(canvas, ev, &ve, OL_DEFAULT_IE);
	switch (ve.virtual_name) {

	case OL_SELECT:
			/* Canvas is touched, clear the status are now...
			 */
		if (empty_footer)
		{
			empty_footer = False;
			FooterMessage(NULL, False);
		}
		ShrinkPixel(ev->xbutton.x, ev->xbutton.y, &last_point);
		FillPixel(canvas, &last_point);
		INIT_ARGS();
		SET_ARGS(XtNwidth, &width);
		SET_ARGS(XtNheight, &height);
		GET_VALUES(canvas);
		END_ARGS();

		while (XCheckWindowEvent(DISPLAY, XtWindow(canvas),
			ButtonPressMask | ButtonReleaseMask, &rsev) != True)
		{
			XQueryPointer(DISPLAY, XtWindow(canvas),
					&root, &jc, &root_x, &root_y, &x, &y,
					&mask);
			if (x >= 0 && x < (int) width &&
			    y >= 0 && y < (int) height) {
				ShrinkPixel(x, y, &this_point);
				if (last_point.x != this_point.x ||
						last_point.y != this_point.y)
				{
					FillPixel(canvas, &this_point);
					last_point = this_point;
				}
			}
		}
		break;

	case OL_ADJUST:
		/*
		 *	Not sure what this should do yet...
		 */
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


void
LinesEventHandler(canvas, data, ev, cont_to_dispatch)
Widget canvas;
XtPointer data;
XEvent *ev;
Boolean *cont_to_dispatch;
{
	XPoint			start_point,
				end_point,
				tmp_point;
	OlVirtualEventRec	ve;

	if (ev->type != ButtonPress)
		return;

	OlLookupInputEvent(canvas, ev, &ve, OL_DEFAULT_IE);
	switch (ve.virtual_name) {

	case OL_SELECT:
		tmp_point.x = ev->xbutton.x;
		tmp_point.y = ev->xbutton.y;

		ShrinkPixel(ev->xbutton.x, ev->xbutton.y, &start_point);

		RubberBandLine(canvas, &tmp_point, &end_point);

		Draw(canvas, LINE, DrawFilled, &start_point, &end_point);

		tmp_point.x = MIN(start_point.x, end_point.x);
		tmp_point.y = MIN(start_point.y, end_point.y);
		end_point.x = MAX(start_point.x, end_point.x);
		end_point.y = MAX(start_point.y, end_point.y);
		start_point = tmp_point;
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


void
SegmentsEventHandler(canvas, data, ev, cont_to_dispatch)
Widget canvas;
XtPointer data;
XEvent *ev;
Boolean *cont_to_dispatch;
{
	static XPoint		start_point;
	XPoint			end_point,
				tmp_point1,
				tmp_point2;
	OlVirtualEventRec	ve;

	if (ev->type != ButtonPress)
		return;

	OlLookupInputEvent(canvas, ev, &ve, OL_DEFAULT_IE);
	switch (ve.virtual_name) {

	case OL_SELECT:
		if (StartFreshSegment)
		{
			tmp_point1.x = ev->xbutton.x;
			tmp_point1.y = ev->xbutton.y;

			ShrinkPixel(ev->xbutton.x, ev->xbutton.y, &start_point);
		}
		else
			ExpandPixel((int)start_point.x, (int)start_point.y,
								&tmp_point1);

		RubberBandLine(canvas, &tmp_point1, &end_point);

		Draw(canvas, LINE, DrawFilled, &start_point, &end_point);
		start_point = end_point;
		StartFreshSegment = False;
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


void
OvalsEventHandler(canvas, data, ev, cont_to_dispatch)
Widget canvas;
XtPointer data;
XEvent *ev;
Boolean *cont_to_dispatch;
{
	Bool			constrain = (Bool) data;
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

		RubberBandOval(canvas, &start_point, &end_point, constrain);

		Draw(canvas, OVAL, DrawFilled, &start_point, &end_point);
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


void
RectanglesEventHandler(canvas, data, ev, cont_to_dispatch)
Widget canvas;
XtPointer data;
XEvent *ev;
Boolean *cont_to_dispatch;
{
	Bool			constrain = (Bool) data;
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

		RubberBandRectangle(canvas, &start_point, &end_point,constrain);
		ShrinkPixel(ev->xbutton.x, ev->xbutton.y, &start_point);

		Draw(canvas, RECTANGLE, DrawFilled, &start_point, &end_point);
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


void
TextEventHandler(canvas, data, ev, cont_to_dispatch)
Widget canvas;
XtPointer data;
XEvent *ev;
Boolean *cont_to_dispatch;
{
}


void
CanvasExpose(canvas, ev, region)
Widget canvas;
XEvent *ev;
Region region;
{
	XPoint		r_origin,
			r_corner;

	ShrinkPixel(ev->xexpose.x, ev->xexpose.y, &r_origin);
	ShrinkPixel(ev->xexpose.x + ev->xexpose.width - 1,
			ev->xexpose.y + ev->xexpose.height - 1, &r_corner);

	RefreshCanvas(&r_origin, &r_corner);
}
