/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olpixmap:magnify.c	1.8"
#endif

#include "pixmap.h"


static Pixmap		MagBuf = (Pixmap) 0;
#ifndef SMALLER_SIZE
static Pixmap		MagBuf2 = (Pixmap) 0;
#endif
static GC		MagnifyGC = (GC) 0;

static void	SpreadWide();
static void	SpreadHigh();
static void	SmearWide();
static void	SmearHigh();



void
InitializeMagnifier(canvas, small_w, small_h, big_w, big_h, depth)
Widget canvas;
unsigned int small_w;
unsigned int small_h;
unsigned int big_w;
unsigned int big_h;
unsigned int depth;
{
	if (MagBuf != (Pixmap) 0)
		XFreePixmap(DISPLAY, MagBuf);
	MagBuf = XCreatePixmap(DISPLAY, XtWindow(canvas),
						big_w, small_h, depth);
#ifndef SMALLER_SIZE
	if (MagBuf2 != (Pixmap) 0)
		XFreePixmap(DISPLAY, MagBuf2);
	MagBuf2 = XCreatePixmap(DISPLAY, XtWindow(canvas),
						big_w, big_h, depth);
#endif

 	if (MagnifyGC == (GC) 0) {
		XGCValues	gcv;

		gcv.graphics_exposures = False;
		MagnifyGC = XCreateGC(DISPLAY, XtWindow(canvas),
						GCGraphicsExposures, &gcv);
	}
}


void
Magnify(dpy, small_pixmap, big_window, origin, corner, scale)
Display *dpy;
Pixmap small_pixmap;
Window big_window;
XPoint *origin;
XPoint *corner;
XPoint *scale;
{
	unsigned int	width,
			height;
 	Position	x,
			y;
	XPoint		r_origin,
			r_corner,
			offset;

	/*
	 *	Work off of copies of what was passed in because
	 *	we plan to change what lives at those addresses.
	 */
	r_origin = *origin,
	r_corner = *corner,

	/*
	 *	Move the corner point to be non-inclusive of the affected area.
	 */
	r_corner.x++;
	r_corner.y++;

	/*
	 *	Determine the offset of the canvas so that we magnify
	 *	its visible portion onto our off-screen pixmap(s).
	 */
	INIT_ARGS();
	SET_ARGS(XtNx, &x);
	SET_ARGS(XtNy, &y);
	GET_VALUES(Canvas);
	END_ARGS();
	offset.x = x;
	offset.y = y;

	/*
	 *	This magnification algorithm smears the pixel value
	 *	first across the top of the magnification cell, then
	 *	down to fill the entire cell.  (Thus any cell whose
	 *	upper-left corner is off screen won't get magnified,
	 *	since the initial color seed is getting lost in space.)
	 *	The second smear (the smear *down* the pixel) can be
	 *	done to an off-screen pixmap and then copyarea'd onto
	 *	the window for visual crispness, or done directly
	 *	to the window for memory savings.
	 *
	 *	All pixels in the specified area are magnified at once
	 *	via the two spreading operations.
	 */

	SpreadWide(dpy, small_pixmap, MagBuf, &r_origin, &r_corner, &offset,
								(int)scale->x);

	r_origin.x *= scale->x;
	r_corner.x *= scale->x;

	SmearWide(dpy, MagBuf, &r_origin, &r_corner, &offset, (int)scale->x);

#ifdef SMALLER_SIZE
	SpreadHigh(dpy, MagBuf, big_window, &r_origin, &r_corner, &offset,
								(int)scale->y);
#else
	SpreadHigh(dpy, MagBuf, MagBuf2, &r_origin, &r_corner, &offset,
								(int)scale->y);
#endif

	r_origin.y *= scale->y;
	r_corner.y *= scale->y;
	width = r_corner.x - r_origin.x;
	height = r_corner.y - r_origin.y;

#ifdef SMALLER_SIZE
	SmearHigh(dpy, big_window, &r_origin, &r_corner, &offset,
								(int)scale->y);
#else
	SmearHigh(dpy, MagBuf2, &r_origin, &r_corner, &offset, (int)scale->y);
	XSetFunction(dpy, MagnifyGC, GXcopy);
	XCopyArea(dpy, MagBuf2, big_window, MagnifyGC,
				r_origin.x + offset.x, r_origin.y + offset.y,
				width, height, r_origin.x, r_origin.y);
#endif
	DrawGrid(dpy, big_window, (int)r_origin.x, (int)r_origin.y,
							width, height);
}


static void
SpreadWide(dpy, from, to, r_origin, r_corner, offset, scale)
Display *dpy;
Pixmap from;
Pixmap to;
XPoint *r_origin;
XPoint *r_corner;
XPoint *offset;
int scale;
{
	int	pt = r_origin->x,
		slice_origin_x = pt * scale + offset->x;

	XSetFunction(dpy, MagnifyGC, GXclear);
	XFillRectangle(dpy, to, MagnifyGC, slice_origin_x, r_origin->y,
				(r_corner->x - r_origin->x) * scale,
				r_corner->y - r_origin->y);
	XSetFunction(dpy, MagnifyGC, GXcopy);
	XSetForeground(dpy, MagnifyGC, CurrentForeground);

	while (pt < r_corner->x)
	{
		XCopyArea(dpy, from, to, MagnifyGC, pt, r_origin->y,
					1, r_corner->y - r_origin->y,
					slice_origin_x, r_origin->y);
		pt++;
		slice_origin_x += scale;
	}
}


static void
SpreadHigh(dpy, from, to, r_origin, r_corner, offset, scale)
Display *dpy;
Pixmap from;
#ifdef SMALLER_SIZE
Window to;
#else
Pixmap to;
#endif
XPoint *r_origin;
XPoint *r_corner;
XPoint *offset;
int scale;
{
	int	pt = r_origin->y,
#ifdef SMALLER_SIZE
		slice_origin_x = r_origin->x,
		slice_origin_y = pt * scale;
#else
		slice_origin_x = r_origin->x + offset->x,
		slice_origin_y = pt * scale + offset->y;
#endif

	XSetFunction(dpy, MagnifyGC, GXclear);
	XFillRectangle(dpy, to, MagnifyGC, slice_origin_x, slice_origin_y,
				r_corner->x - r_origin->x,
				(r_corner->y - r_origin->y) * scale);
	XSetFunction(dpy, MagnifyGC, GXcopy);
	XSetForeground(dpy, MagnifyGC, CurrentForeground);

	while (pt < r_corner->y)
	{
		XCopyArea(dpy, from, to, MagnifyGC, r_origin->x + offset->x, pt,
					r_corner->x - r_origin->x, 1,
					slice_origin_x, slice_origin_y);
		pt++;
		slice_origin_y += scale;
	}
}


static void
SmearWide(dpy, to, r_origin, r_corner, offset, scale)
Display *dpy;
Pixmap to;
XPoint *r_origin;
XPoint *r_corner;
XPoint *offset;
int scale;
{
	int	i,
		r,
		slice_origin_x = r_origin->x + offset->x;

	XSetFunction(dpy, MagnifyGC, GXor);
	XSetForeground(dpy, MagnifyGC, CurrentForeground);

	for (i = 1; i <= scale / 2; i *= 2)
		XCopyArea(dpy, to, to, MagnifyGC,
			slice_origin_x, r_origin->y,
			r_corner->x - r_origin->x, r_corner->y - r_origin->y,
			slice_origin_x + i, r_origin->y);
	r = scale - i;
	XCopyArea(dpy, to, to, MagnifyGC,
		slice_origin_x + i - r, r_origin->y,
		r_corner->x - r_origin->x, r_corner->y - r_origin->y,
		slice_origin_x + i, r_origin->y);
}


static void
SmearHigh(dpy, to, r_origin, r_corner, offset, scale)
Display *dpy;
#ifdef SMALLER_SIZE
Window to;
#else
Pixmap to;
#endif
XPoint *r_origin;
XPoint *r_corner;
XPoint *offset;
int scale;
{
	int	i,
		r,
#ifdef SMALLER_SIZE
		slice_origin_x = r_origin->x,
		slice_origin_y = r_origin->y;
#else
		slice_origin_x = r_origin->x + offset->x,
		slice_origin_y = r_origin->y + offset->y;
#endif

	XSetFunction(dpy, MagnifyGC, GXor);
	XSetForeground(dpy, MagnifyGC, CurrentForeground);

	for (i = 1; i <= scale / 2; i *= 2)
		XCopyArea(dpy, to, to, MagnifyGC,
			slice_origin_x, slice_origin_y,
			r_corner->x - r_origin->x, r_corner->y - r_origin->y,
			slice_origin_x, slice_origin_y + i);
	r = scale - i;
	XCopyArea(dpy, to, to, MagnifyGC,
		slice_origin_x, slice_origin_y + i - r,
		r_corner->x - r_origin->x, r_corner->y - r_origin->y,
		slice_origin_x, slice_origin_y + i);
}
