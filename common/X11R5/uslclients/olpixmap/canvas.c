/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olpixmap:canvas.c	1.8"
#endif

#include "pixmap.h"


extern void	InitializeMagnifier();


Pixel		CurrentForeground;
Pixel		CurrentBackground;
int		CurrentFunction = GXcopy;
Bool		DrawFilled = True;
int		CurrentLineWidth = 0;
int		CurrentLineStyle = LineSolid;
Bool		ShowGrid = True;

Dimension	CanvasPixelWidth;
Dimension	CanvasPixelHeight;
Pixmap		RealPixmap = (Pixmap) 0;
GC		DrawGC = (GC) 0;


static GC	GridGC = (GC) 0;
static GC	GridLineGC = (GC) 0;



void
MakeGrid(canvas)
Widget canvas;
{
	Pixmap		pixmap;
	XSegment	segments[2];

	if (GridGC == (GC) 0) {
		XGCValues	gcv;

		gcv.graphics_exposures = False;
		gcv.line_width = 0;
		GridGC = XCreateGC(DISPLAY, XtWindow(canvas),
				GCLineWidth | GCGraphicsExposures, &gcv);
	}

	pixmap = XCreatePixmap(DISPLAY, XtWindow(canvas),
				(unsigned int)CanvasPixelWidth,
				(unsigned int)CanvasPixelHeight, 1);
	if (GridLineGC == (GC) 0)
	{
		XGCValues	gcv;

		gcv.graphics_exposures = False;
		gcv.line_width = 0;
		GridLineGC = XCreateGC(DISPLAY, pixmap,
				GCLineWidth | GCGraphicsExposures, &gcv);
	}

	XSetFunction(DISPLAY, GridLineGC, GXclear);
	XFillRectangle(DISPLAY, pixmap, GridLineGC, 0, 0,
				(unsigned int)CanvasPixelWidth,
				(unsigned int)CanvasPixelHeight);
	XSetFunction(DISPLAY, GridLineGC, GXset);

	segments[0].x1 = 0;
	segments[0].y1 = 0;
	segments[0].x2 = CanvasPixelWidth - 1;
	segments[0].y2 = 0;

	segments[1].x1 = 0;
	segments[1].y1 = 0;
	segments[1].x2 = 0;
	segments[1].y2 = CanvasPixelHeight - 1;

	XDrawSegments(DISPLAY, pixmap, GridLineGC, segments, 2);
	XFlush(DISPLAY);

	XSetStipple(DISPLAY, GridGC, pixmap);

	INIT_ARGS();
	SET_ARGS(XtNhStepSize, (int)CanvasPixelWidth);
	SET_ARGS(XtNvStepSize, (int)CanvasPixelHeight);
	SET_VALUES(ScrolledWindow);
	END_ARGS();

	XFreePixmap(DISPLAY, pixmap);
}


void
DrawGrid(dpy, win, x, y, width, height)
Display *dpy;
Window win;
int x;
int y;
unsigned int width;
unsigned int height;
{
	if (ShowGrid != True)
		return;

	XSetForeground(dpy, GridGC, CurrentForeground);
	XSetFunction(dpy, GridGC, GXinvert);
	XSetFillStyle(dpy, GridGC, FillStippled);
	XFillRectangle(dpy, win, GridGC, x, y, width, height);
}


void
ShrinkPixel(x, y, p)
int x;
int y;
XPoint *p;
{
	p->x = x / (int)CanvasPixelWidth;
	p->y = y / (int)CanvasPixelHeight;
}


void
ExpandPixel(x, y, p)
int x;
int y;
XPoint *p;
{
	p->x = x * CanvasPixelWidth + CanvasPixelWidth / 2;
	p->y = y * CanvasPixelHeight + CanvasPixelHeight / 2;
}


void
FillPixel(canvas, p)
Widget canvas;
XPoint *p;
{
	XPoint	ep;

	ep.x = p->x * CanvasPixelWidth;
	ep.y = p->y * CanvasPixelHeight;
	Draw(canvas, PIXEL, DrawFilled, p, &ep);
}


void
InitializePixmap(canvas, new_pixmap, small_width, small_height,
						big_width, big_height, depth)
Widget canvas;
Pixmap new_pixmap;
unsigned int small_width;
unsigned int small_height;
unsigned int big_width;
unsigned int big_height;
unsigned int depth;
{
	Pixmap	old_pixmap = RealPixmap;

	if (DrawGC == (GC) 0) {
		XGCValues	gcv;

		gcv.graphics_exposures = False;
		DrawGC = XCreateGC(DISPLAY, XtWindow(canvas),
				GCLineWidth | GCGraphicsExposures, &gcv);
	}

	if (new_pixmap != (Pixmap) 0) {
		RealPixmap = new_pixmap;
	} else {
		RealPixmap = XCreatePixmap(DISPLAY, ROOT,
					small_width, small_height, depth);
		XSetForeground(DISPLAY, DrawGC, CurrentBackground);
		XFillRectangle(DISPLAY, RealPixmap, DrawGC, 0, 0,
						small_width, small_height);
		ResetDrawGC();
	}
	if (old_pixmap != (Pixmap) 0)
		XFreePixmap(DISPLAY, old_pixmap);

	InitializeMagnifier(canvas, small_width, small_height, big_width,
							big_height, depth);
}
