/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olpixmap:draw.c	1.19"
#endif

#include "pixmap.h"
#include "error.h"
#include <stdio.h>  
#include <FButtons.h>
#include <MenuShell.h>
#include <Olg.h>
#include <Xol/Error.h>  

#include "pixels.xpm"
#ifdef TEXT_SUPPORTED
#include "text.xpm"
#endif
#include "lines.xpm"
#include "segments.xpm"
#include "ovals.xpm"
#include "circles.xpm"
#include "rectangles.xpm"
#include "squares.xpm"


#define SETUP_MNEM(i,a,b) \
	exc_items_l[i].mnemonic = (XtArgVal) *(OlGetMessage(DISPLAY, \
					NULL, 0, \
					OleNmnemonic, \
					a, \
					OleCOlClientOlpixmapMsgs, \
					b, \
					(XrmDatabase)NULL))

#define SETUP_XPM(index, name) \
	exc_items_i[index].bg_pixmap = XCreatePixmapFromData(DISPLAY, \
					ROOT, PixmapColormap, \
					concat(name,_width), \
					concat(name,_height), \
					DefaultDepthOfScreen(SCREEN), \
					concat(name,_ncolors), \
					concat(name,_chars_per_pixel), \
					concat(name,_colors), \
					concat(name,_pixels))


extern Bool	Editing;

extern void	PixelEventHandler();
extern void	TextEventHandler();
extern void	LinesEventHandler();
extern void	SegmentsEventHandler();
extern void	OvalsEventHandler();
extern void	RectanglesEventHandler();


void		(*CurrentDrawOp)() = PixelEventHandler;
XtPointer	CurrentData = (XtPointer) 0;


static void	Pixels();
static void	Text();
static void	Lines();
static void	Segments();
static void	Ovals();
static void	Rectangles();


typedef struct {
	XtArgVal	selectProc;
	XtArgVal	clientData;
	XtArgVal	label;
	XtArgVal	mnemonic;
} FlatExclusivesL;

static String	exc_fields_l[] = {
	XtNselectProc, XtNclientData, XtNlabel, XtNmnemonic };

typedef struct {
	XtArgVal	selectProc;
	XtArgVal	clientData;
	XtArgVal	mapped;
	XtArgVal	bg_pixmap;
} FlatExclusivesI;

static String	exc_fields_i[] = { XtNselectProc, XtNclientData,
				   XtNmappedWhenManaged, XtNbackgroundPixmap };


static char	*Labels[8];


#define GETMESS(a,b)	 OlGetMessage(dsp, NULL, 0, \
		     OleNfixedString, \
		     a,  \
		     OleCOlClientOlpixmapMsgs, \
		     b, \
		     (XrmDatabase)NULL)

/*
#define GETMNEM(a,b)	 OlGetMessage(dsp, NULL, 0, \
		     OleNmnemonic, \
		     a,  \
		     OleCOlClientOlpixmapMsgs, \
		     b, \
		     (XrmDatabase)NULL)
 */
void
SetUpLabels(dsp)
  Display *dsp;
{
  Cardinal n = 0;

#ifdef OLDSTUFF
  OLGM(pixels);
#ifdef TEXT_SUPPORTED
  OLGM(text);
#endif
  OLGM(lines);
  OLGM(segments);
  OLGM(ovals);
  OLGM(circles);
  OLGM(rectangles);
  OLGM(squares);

#else

Labels[n++] =
	GETMESS(OleTpixels,OleMfixedString_pixels);
#ifdef TEXT_SUPPORTED
Labels[n++] =
	GETMESS(OleTtext,OleMfixedString_text);
#endif
Labels[n++] =
	GETMESS(OleTlines,OleMfixedString_lines);
Labels[n++] =
	GETMESS(OleTsegments,OleMfixedString_segments);
Labels[n++] =
	GETMESS(OleTovals,OleMfixedString_ovals);
Labels[n++] =
	GETMESS(OleTcircles,OleMfixedString_circles);
Labels[n++] =
	GETMESS(OleTrectangles,OleMfixedString_rectangles);
Labels[n++] =
	GETMESS(OleTsquares,OleMfixedString_squares);

#endif
}


Widget
SetDraw(button_stack)
Widget button_stack;
{
	Widget			menu_pane;
	Cardinal		i;
	static FlatExclusivesL	exc_items_l[] = {
		{ (XtArgVal)Pixels, (XtArgVal)0, (XtArgVal)"Pixels" },
#ifdef TEXT_SUPPORTED
		{ (XtArgVal)Text, (XtArgVal)0, (XtArgVal)"Text" },
#endif
		{ (XtArgVal)Lines, (XtArgVal)0, (XtArgVal)"Lines" },
		{ (XtArgVal)Segments, (XtArgVal)0, (XtArgVal)"Segments" },
		{ (XtArgVal)Ovals, (XtArgVal)False, (XtArgVal)"Ovals" },
		{ (XtArgVal)Ovals, (XtArgVal)True, (XtArgVal)"Circles" },
		{ (XtArgVal)Rectangles, (XtArgVal)False,(XtArgVal)"Rectangles"},
		{ (XtArgVal)Rectangles, (XtArgVal)True, (XtArgVal)"Squares" },
	};
	static FlatExclusivesI	exc_items_i[] = {
		{ (XtArgVal)Pixels, (XtArgVal)0, (XtArgVal)True },
#ifdef TEXT_SUPPORTED
		/*
		 *	Don't need the XtNmappedWhenManaged field
		 *	in this case.
		 */
		{ (XtArgVal)Text, (XtArgVal)0, (XtArgVal)True },
#else
		{ (XtArgVal)Pixels, (XtArgVal)0, (XtArgVal)False },
#endif
		{ (XtArgVal)Lines, (XtArgVal)0, (XtArgVal)True },
		{ (XtArgVal)Segments, (XtArgVal)0, (XtArgVal)True },
		{ (XtArgVal)Ovals, (XtArgVal)False, (XtArgVal)True },
		{ (XtArgVal)Ovals, (XtArgVal)True, (XtArgVal)True },
		{ (XtArgVal)Rectangles, (XtArgVal)False, (XtArgVal)True },
		{ (XtArgVal)Rectangles, (XtArgVal)True, (XtArgVal)True },
	};

	if (AppResources.icon_menu) {
		SETUP_XPM(0, pixels);
#ifdef TEXT_SUPPORTED
		SETUP_XPM(1, text);
#else
		SETUP_XPM(1, pixels);
#endif
		SETUP_XPM(2, lines);
		SETUP_XPM(3, segments);
		SETUP_XPM(4, ovals);
		SETUP_XPM(5, circles);
		SETUP_XPM(6, rectangles);
		SETUP_XPM(7, squares);
	} else {
		SetUpLabels(DISPLAY);
#ifdef TEXT_SUPPORTED	
		for (i = 0; i < 8; i++)
#else	  
		for (i = 0; i < 7; i++)
#endif	  
			exc_items_l[i].label = (XtArgVal)Labels[i];

		i = 0;
		SETUP_MNEM(i, OleTpixels,     OleMmnemonic_pixels);     i++;

#ifdef TEXT_SUPPORTED
		SETUP_MNEM(i, OleTtext,       OleMmnemonic_text);	i++;
#endif
		SETUP_MNEM(i, OleTlines,      OleMmnemonic_lines);      i++;
		SETUP_MNEM(i, OleTsegments,   OleMmnemonic_segments);   i++;
		SETUP_MNEM(i, OleTovals,      OleMmnemonic_ovals);      i++;
		SETUP_MNEM(i, OleTcircles,    OleMmnemonic_circles);    i++;
		SETUP_MNEM(i, OleTrectangles, OleMmnemonic_rectangles); i++;
		SETUP_MNEM(i, OleTsquares,    OleMmnemonic_squares);
	}

	INIT_ARGS();
	SET_ARGS(XtNpushpin, OL_OUT);
	menu_pane = CREATE_POPUP (
		"Draw", popupMenuShellWidgetClass, button_stack
	);
	END_ARGS();

	INIT_ARGS();
	SET_ARGS(XtNbuttonType, OL_RECT_BTN);
	SET_ARGS(XtNexclusives, True);
	SET_ARGS(XtNlayoutType, OL_FIXEDCOLS);
	if (AppResources.icon_menu) {
		SET_ARGS(XtNmeasure, 2);
		SET_ARGS(XtNitems, exc_items_i);
		SET_ARGS(XtNnumItems, XtNumber(exc_items_i));
		SET_ARGS(XtNitemFields, exc_fields_i);
		SET_ARGS(XtNnumItemFields, XtNumber(exc_fields_i));
	} else {
		SET_ARGS(XtNitems, exc_items_l);
		SET_ARGS(XtNnumItems, XtNumber(exc_items_l));
		SET_ARGS(XtNitemFields, exc_fields_l);
		SET_ARGS(XtNnumItemFields, XtNumber(exc_fields_l));
	}
	CREATE_MANAGED("_menu_", flatButtonsWidgetClass, menu_pane);
	END_ARGS();

        return menu_pane;
}


static void
Pixels(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	XtRemoveEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask, False,
						CurrentDrawOp, CurrentData);
	CurrentDrawOp = PixelEventHandler;
	CurrentData = (XtPointer) 0;
	if (!Editing)
		XtAddEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask,
					False, CurrentDrawOp, CurrentData);
}


static void
Text(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	XtRemoveEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask, False,
						CurrentDrawOp, CurrentData);
	CurrentDrawOp = TextEventHandler;
	CurrentData = (XtPointer) 0;
	if (!Editing)
		XtAddEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask,
					False, CurrentDrawOp, CurrentData);
}


static void
Lines(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	XtRemoveEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask, False,
						CurrentDrawOp, CurrentData);
	CurrentDrawOp = LinesEventHandler;
	CurrentData = (XtPointer) 0;
	if (!Editing)
		XtAddEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask,
					False, CurrentDrawOp, CurrentData);
}


static void
Segments(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	extern Bool	StartFreshSegment;

	XtRemoveEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask, False,
						CurrentDrawOp, CurrentData);
	CurrentDrawOp = SegmentsEventHandler;
	CurrentData = (XtPointer) 0;
	if (!Editing)
		XtAddEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask,
					False, CurrentDrawOp, CurrentData);
	StartFreshSegment = True;
}


static void
Ovals(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	XtRemoveEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask, False,
						CurrentDrawOp, CurrentData);
	CurrentDrawOp = OvalsEventHandler;
	CurrentData = client_data;
	if (!Editing)
		XtAddEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask,
					False, CurrentDrawOp, CurrentData);
}


static void
Rectangles(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	XtRemoveEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask, False,
						CurrentDrawOp, CurrentData);
	CurrentDrawOp = RectanglesEventHandler;
	CurrentData = client_data;
	if (!Editing)
		XtAddEventHandler(Canvas, ButtonPressMask | ButtonReleaseMask,
					False, CurrentDrawOp, CurrentData);
}


void
Draw(canvas, function, draw_filled, p1, p2)
Widget canvas;
int function;
Bool draw_filled;
XPoint *p1;
XPoint *p2;
{
	int		x,
			y;
	unsigned int	width,
			height;
	unsigned int	dimension_adjustment;
	Bool		do_refresh = True;

	if (function == PIXEL) {
		x = p1->x;
		y = p1->y;
		width = height = 0;
	} else {
		x = MIN(p1->x, p2->x);
		y = MIN(p1->y, p2->y);
		width = MAX(p1->x, p2->x) - x;
		height = MAX(p1->y, p2->y) - y;
		dimension_adjustment = (CurrentLineWidth == 0) ?
					0 : (CurrentLineWidth - 1);
	}

	switch (function) {

	case PIXEL:
		XDrawPoint(DISPLAY, RealPixmap, DrawGC,
					(int)p1->x, (int)p1->y);
		XFillRectangle(DISPLAY, XtWindow(canvas), DrawGC,
					(int)p2->x, (int)p2->y,
					(unsigned int)CanvasPixelWidth,
					(unsigned int)CanvasPixelHeight);
		DrawGrid(DISPLAY, XtWindow(canvas),
					(int)p2->x, (int)p2->y,
					(unsigned int)CanvasPixelWidth,
					(unsigned int)CanvasPixelHeight);
		do_refresh = False;
		break;

	case LINE:
	case SEGMENT:
		XDrawLine(DISPLAY, RealPixmap, DrawGC,
					(int)p1->x, (int)p1->y,
					(int)p2->x, (int)p2->y);
		x -= CurrentLineWidth / 2;
		y -= CurrentLineWidth / 2;
		width += dimension_adjustment;
		height += dimension_adjustment;
		break;

	case OVAL:
	case CIRCLE:
		if (draw_filled) {
			XFillArc(DISPLAY, RealPixmap, DrawGC, x, y,
					width + 1, height + 1, 0, 360 * 64);
		} else {
			XDrawArc(DISPLAY, RealPixmap, DrawGC, x, y,
						width, height, 0, 360 * 64);
			x -= CurrentLineWidth / 2;
			y -= CurrentLineWidth / 2;
			width += dimension_adjustment;
			height += dimension_adjustment;
		}
		break;

	case RECTANGLE:
	case SQUARE:
		if (draw_filled) {
			XFillRectangle(DISPLAY, RealPixmap, DrawGC, x, y,
							width + 1, height + 1);
		} else {
			XDrawRectangle(DISPLAY, RealPixmap, DrawGC, x, y,
							width, height);
			x -= CurrentLineWidth / 2;
			y -= CurrentLineWidth / 2;
			width += dimension_adjustment;
			height += dimension_adjustment;
		}
		break;

	case TEXT:
		break;

	default:
		break;
	}

	Changed = True;
	if (do_refresh)
	{
		XPoint	start,
			end;

		start.x = x;
		start.y = y;
		end.x = x + width;
		end.y = y + height;

		RefreshCanvas(&start, &end);
	}
	if (PixmapIsDisplayed)
		RefreshPixmapDisplay(x, y, width+1, height+1);
}
