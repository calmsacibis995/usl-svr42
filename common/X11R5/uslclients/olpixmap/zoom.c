/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olpixmap:zoom.c	1.8"
#endif

#include "pixmap.h"



void
ZoomIn(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	Position	x,
			y;
	Dimension	width,
			height;

	INIT_ARGS();
	SET_ARGS(XtNviewWidth, &width);
	SET_ARGS(XtNviewHeight, &height);
	GET_VALUES(ScrolledWindow);
	END_ARGS();

	if ((int) CanvasPixelWidth * 2 <= (int) width &&
	    (int) CanvasPixelHeight * 2 <= (int) height) {
		CanvasPixelWidth *= 2;
		CanvasPixelHeight *= 2;

		width = PixmapWidth * CanvasPixelWidth;
		height = PixmapHeight * CanvasPixelHeight;

		INIT_ARGS();
		SET_ARGS(XtNx, &x);
		SET_ARGS(XtNy, &y);
		GET_VALUES(Canvas);
		END_ARGS();

		x *= 2;
		y *= 2;

		/*
		 *	Should cause exposure which will draw new grid...
		 */
		INIT_ARGS();
		SET_ARGS(XtNx, x);
		SET_ARGS(XtNy, y);
		SET_ARGS(XtNwidth, width);
		SET_ARGS(XtNheight, height);
		SET_VALUES(Canvas);
		END_ARGS();

		MakeGrid(Canvas);
	}
}


void
ZoomOut(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	Position	x,
			y;
	Dimension	min_pixel_width,
			min_pixel_height,
			width,
			height;

#ifdef USE_SQUARE_PIXELS
	min_pixel_width = MIN_PIXELWIDTH;
	min_pixel_height = MIN_PIXELHEIGHT;
#else
	min_pixel_width = min_pixel_height =
				MAX(MIN_PIXELWIDTH, MIN_PIXELHEIGHT);
#endif

	if ((int) CanvasPixelWidth / 2 >= (int) min_pixel_width &&
	    (int) CanvasPixelHeight / 2 >= (int) min_pixel_height) {
		CanvasPixelWidth /= 2;
		CanvasPixelHeight /= 2;

		width = PixmapWidth * CanvasPixelWidth;
		height = PixmapHeight * CanvasPixelHeight;

		INIT_ARGS();
		SET_ARGS(XtNx, &x);
		SET_ARGS(XtNy, &y);
		GET_VALUES(Canvas);
		END_ARGS();

		x /= 2;
		y /= 2;

		/*
		 *	Should cause exposure which will draw new grid...
		 */
		INIT_ARGS();
		SET_ARGS(XtNx, x);
		SET_ARGS(XtNy, y);
		SET_ARGS(XtNwidth, width);
		SET_ARGS(XtNheight, height);
		SET_VALUES(Canvas);
		END_ARGS();

		MakeGrid(Canvas);
	}
}
