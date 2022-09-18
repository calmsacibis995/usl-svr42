/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olpixmap:utils.c	1.28"
#endif

#include "pixmap.h"
#include <X11/Xos.h>
#include <FButtons.h>
#include <MenuShell.h>
#include <OlCursors.h>

#include <ctype.h>


extern Widget		PixmapPopup;
extern Widget		FilePopup;
extern Widget		FileNotice;
extern Widget		PropertiesPopup;

extern void		Magnify();
extern void		InitializePixmap();
extern void		SetDimensionFields();


static void		ConstrainPointer();


char * MenuFields[] = {
        XtNlabel,
	XtNselectProc,
	XtNclientData,
	XtNsensitive,
	XtNmappedWhenManaged,	/* False and item won't display */
	XtNpopupMenu,		/* Popup menu widget */
	XtNmnemonic,		/* mnemonic */
};


Widget
AddMenu(parent, menu)
Widget parent;
Menu *menu;
{
 	Widget	flatmenu;
	Widget	shell;

        shell = parent;
	if (menu->use_popup == True) {
		INIT_ARGS();
		SET_ARGS(XtNpushpin, menu->pushpin);
		shell = CREATE_POPUP (
			menu->label,
			popupMenuShellWidgetClass,
			parent
		);
		END_ARGS();
	}

	INIT_ARGS();
	SET_ARGS(XtNlabelJustify, OL_LEFT);
	SET_ARGS(XtNrecomputeSize, True);
	SET_ARGS(XtNlayoutType, menu->orientation);
	SET_ARGS(XtNitemFields, MenuFields);
	SET_ARGS(XtNnumItemFields, XtNumber(MenuFields));
	SET_ARGS(XtNitems, menu->items);
	SET_ARGS(XtNnumItems, menu->numitems);

	/* only the menubar will have no popup to use and
	   more than 4 items */
	
	if (menu->use_popup == False && menu->numitems > 4) {
	  SET_ARGS(XtNmenubarBehavior, True);
	}
	
	flatmenu = CREATE_MANAGED("_menu_", flatButtonsWidgetClass, shell);
	END_ARGS();

	return menu->use_popup == True ? shell : flatmenu;
}


void
RefreshCanvas(r_origin, r_corner)
XPoint *r_origin;
XPoint *r_corner;
{
	XPoint	scale;

	scale.x = CanvasPixelWidth;
	scale.y = CanvasPixelHeight;

	Magnify(DISPLAY, RealPixmap, XtWindow(Canvas), r_origin,
							r_corner, &scale);
	XFlush(DISPLAY);
}


void
ResetAllVisuals(new_pixmap, new_width, new_height)
Pixmap new_pixmap;
Dimension new_width;
Dimension new_height;
{
	Dimension	view_width,
			view_height,
			viewport_w,
			viewport_h,
			old_canvas_w = PixmapWidth * CanvasPixelWidth,
			old_canvas_h = PixmapHeight * CanvasPixelHeight;

	view_width = PixmapWidth = new_width;
	view_height = PixmapHeight = new_height;

#ifdef USE_SQUARE_PIXELS
	CanvasPixelWidth = MIN_PIXELWIDTH;
	CanvasPixelHeight = MIN_PIXELHEIGHT;
#else
	CanvasPixelWidth = CanvasPixelHeight =
				MAX(MIN_PIXELWIDTH, MIN_PIXELHEIGHT);
#endif

	viewport_w = view_width * CanvasPixelWidth;
	viewport_h = view_height * CanvasPixelHeight;

	if ((int) viewport_w > WidthOfScreen(SCREEN) ||
	    (int) viewport_h > HeightOfScreen(SCREEN)) {
		view_width = view_height = MIN (
		    WidthOfScreen(SCREEN) / ((int) CanvasPixelWidth) * 6/10,
		    HeightOfScreen(SCREEN) / ((int) CanvasPixelHeight) * 6/10 );
		viewport_w = view_width * CanvasPixelWidth;
		viewport_h = view_height * CanvasPixelHeight;
	}

	if ((int) viewport_w < PREFERRED_WIDTH &&
	    (int) viewport_h < PREFERRED_HEIGHT) {
		float	mag_factor = MIN(PREFERRED_WIDTH / (float) viewport_w,
					PREFERRED_HEIGHT / (float) viewport_h);

		CanvasPixelWidth =
			((float) CanvasPixelWidth) * mag_factor + 0.5;
		CanvasPixelHeight =
			((float) CanvasPixelHeight) * mag_factor + 0.5;
		viewport_w = view_width * CanvasPixelWidth;
		viewport_h = view_height * CanvasPixelHeight;
	}

	INIT_ARGS();
	SET_ARGS(XtNwidth, PixmapWidth * CanvasPixelWidth);
	SET_ARGS(XtNheight, PixmapHeight * CanvasPixelHeight);
	SET_VALUES(Canvas);
	END_ARGS();

	INIT_ARGS();
	SET_ARGS(XtNviewWidth, viewport_w);
	SET_ARGS(XtNviewHeight, viewport_h);
	SET_VALUES(ScrolledWindow);
	END_ARGS();

	InitializePixmap(Canvas, new_pixmap,
			(unsigned int)PixmapWidth, (unsigned int)PixmapHeight,
			(unsigned int)viewport_w, (unsigned int)viewport_h,
			(unsigned int)PixmapDepth);

	if (PixmapDisplay != (Widget) 0) {
		INIT_ARGS();
		SET_ARGS(XtNwidth, PixmapWidth);
		SET_ARGS(XtNheight, PixmapHeight);
		SET_VALUES(PixmapDisplay);
		END_ARGS();
	}
	SetDimensionFields(PixmapWidth, PixmapHeight);

	MakeGrid(Canvas);
	if (PixmapWidth * CanvasPixelWidth == old_canvas_w &&
	    PixmapHeight * CanvasPixelHeight == old_canvas_h)
		RefreshAllVisuals();
}


void
RefreshAllVisuals()
{
	XPoint start, end;

	start.x = 0;
	start.y = 0;
	end.x = PixmapWidth - 1;
	end.y = PixmapHeight - 1;
	RefreshCanvas(&start, &end);

	if (PixmapIsDisplayed)
		RefreshPixmapDisplay(0, 0, PixmapWidth, PixmapHeight);
}


void
ResetDrawGC()
{
	XSetFunction(DISPLAY, DrawGC, CurrentFunction);
	XSetForeground(DISPLAY, DrawGC, CurrentForeground);
	XSetBackground(DISPLAY, DrawGC, CurrentBackground);
	XSetFillStyle(DISPLAY, DrawGC, FillSolid);
	XSetLineAttributes(DISPLAY, DrawGC, CurrentLineWidth, CurrentLineStyle,
							CapButt, JoinMiter);
}


void
RubberBandLine(canvas, start, end)
Widget canvas;
XPoint *start;
XPoint *end;
{
	XPoint		point[2],
			current_point,
			last_point;
	Window		root,
			jc;
	XEvent		ev;
	Bool		EraseLine = False;
	unsigned int	mask;
	int		root_x,
			root_y,
			x,
			y;

	ShrinkPixel((int)start->x, (int)start->y, &last_point);
	ExpandPixel((int)last_point.x, (int)last_point.y, &(point[0]));

	XSetFunction(DISPLAY, DrawGC, GXinvert);
	XSetLineAttributes(DISPLAY, DrawGC, 0, LineSolid, CapButt, JoinMiter);
	while (XCheckWindowEvent(DISPLAY, XtWindow(canvas),
		ButtonPressMask | ButtonReleaseMask, &ev) != True)
	{
		XQueryPointer(DISPLAY, XtWindow(canvas),
				&root, &jc, &root_x, &root_y, &x, &y, &mask);

		ShrinkPixel(x, y, &current_point);
		if (current_point.x == last_point.x &&
						current_point.y == last_point.y)
			continue;

		last_point = current_point;

		if (EraseLine != True)
			EraseLine = True;
		else
			XDrawLine(DISPLAY, XtWindow(canvas), DrawGC,
					(int)point[0].x, (int)point[0].y,
					(int)point[1].x, (int)point[1].y);

		ExpandPixel((int)current_point.x, (int)current_point.y,
								&(point[1]));
		XDrawLine(DISPLAY, XtWindow(canvas), DrawGC,
					(int)point[0].x, (int)point[0].y,
					(int)point[1].x, (int)point[1].y);
	}
	if (EraseLine)
		XDrawLine(DISPLAY, XtWindow(canvas), DrawGC,
					(int)point[0].x, (int)point[0].y,
					(int)point[1].x, (int)point[1].y);
	ResetDrawGC();

	ShrinkPixel(ev.xbutton.x, ev.xbutton.y, end);
}


void
RubberBandRectangle(canvas, start, end, constrain)
Widget canvas;
XPoint *start;
XPoint *end;
Bool constrain;
{
	XPoint		point[5],
			current_point,
			last_point;
	Window		root,
			jc;
	XEvent		ev;
	Bool		EraseLines = False;
	unsigned int	mask;
	int		root_x,
			root_y,
			x,
			y,
			i;

	XSetFunction(DISPLAY, DrawGC, GXinvert);
	XSetLineAttributes(DISPLAY, DrawGC, 0, LineSolid, CapButt, JoinMiter);
	ShrinkPixel((int)start->x, (int)start->y, &last_point);
	ExpandPixel((int)last_point.x, (int)last_point.y, &(point[0]));

	while (XCheckWindowEvent(DISPLAY, XtWindow(canvas),
		ButtonPressMask | ButtonReleaseMask, &ev) != True)
	{
		XQueryPointer(DISPLAY, XtWindow(canvas),
				&root, &jc, &root_x, &root_y, &x, &y, &mask);

		if (constrain)
			ConstrainPointer(SCREEN, XtWindow(canvas),
				(int)point[0].x, (int)point[0].y, &x, &y);

		ShrinkPixel(x, y, &current_point);
		if (current_point.x == last_point.x &&
						current_point.y == last_point.y)
			continue;

		last_point = current_point;

		if (EraseLines != True)
			EraseLines = True;
		else
			XDrawLines(DISPLAY, XtWindow(canvas), DrawGC,
						point, 5, CoordModeOrigin);

		ExpandPixel((int)current_point.x, (int)current_point.y,
								&(point[2]));
		point[1].x = point[2].x;
		point[1].y = point[0].y;
		point[3].x = point[0].x;
		point[3].y = point[2].y;
		point[4] = point[0];

		XDrawLines(DISPLAY, XtWindow(canvas), DrawGC,
						point, 5, CoordModeOrigin);
	}
	if (EraseLines)
		XDrawLines(DISPLAY, XtWindow(canvas), DrawGC,
						point, 5, CoordModeOrigin);
	ResetDrawGC();

	x = ev.xbutton.x;
	y = ev.xbutton.y;
	if (constrain)
		ConstrainPointer(SCREEN, XtWindow(canvas),
			(int)point[0].x, (int)point[0].y, &x, &y);
	ShrinkPixel(x, y, end);
}


void
RubberBandOval(canvas, start, end, constrain)
Widget canvas;
XPoint *start;
XPoint *end;
Bool constrain;
{
	XPoint		point[5],
			current_point,
			last_point;
	Window		root,
			jc;
	XEvent		ev;
	unsigned int	mask;
	Bool		EraseLines = False;
	int		root_x,
			root_y,
			x,
			y,
			i;

	XSetFunction(DISPLAY, DrawGC, GXinvert);
	XSetLineAttributes(DISPLAY, DrawGC, 0, LineSolid, CapButt, JoinMiter);
	ShrinkPixel((int)start->x, (int)start->y, &last_point);
	ExpandPixel((int)last_point.x, (int)last_point.y, start);

	while (XCheckWindowEvent(DISPLAY, XtWindow(canvas),
		ButtonPressMask | ButtonReleaseMask, &ev) != True)
	{
		XQueryPointer(DISPLAY, XtWindow(canvas),
				&root, &jc, &root_x, &root_y, &x, &y, &mask);

		if (constrain)
			ConstrainPointer(SCREEN, XtWindow(canvas),
				(int)start->x, (int)start->y, &x, &y);

		ShrinkPixel(x, y, &current_point);
		if (current_point.x == last_point.x &&
						current_point.y == last_point.y)
			continue;

		last_point = current_point;

		if (EraseLines != True)
			EraseLines = True;
		else
			XDrawLines(DISPLAY, XtWindow(canvas), DrawGC,
						point, 5, CoordModeOrigin);

		ExpandPixel((int)current_point.x, (int)current_point.y,
								&(point[0]));
		point[1].x = start->x - (point[0].x - start->x);
		point[1].y = point[0].y;
		point[2].x = start->x - (point[0].x - start->x);
		point[2].y = start->y - (point[0].y - start->y);
		point[3].x = point[0].x;
		point[3].y = start->y - (point[0].y - start->y);
		point[4] = point[0];

		XDrawLines(DISPLAY, XtWindow(canvas), DrawGC,
						point, 5, CoordModeOrigin);
	}
	if (EraseLines)
		XDrawLines(DISPLAY, XtWindow(canvas), DrawGC,
						point, 5, CoordModeOrigin);
	ResetDrawGC();

	x = ev.xbutton.x;
	y = ev.xbutton.y;
	if (constrain)
		ConstrainPointer(SCREEN, XtWindow(canvas),
			(int)start->x, (int)start->y, &x, &y);
	ShrinkPixel((int)(start->x - (x - start->x)),
			(int)(start->y - (y - start->y)), start);
	ShrinkPixel(x, y, end);
}


static void
ConstrainPointer(screen, window, anchor_x, anchor_y, x, y)
Screen *screen;
Window window;
int anchor_x;
int anchor_y;
int *x;
int *y;
{
	int	delta_x,
		delta_y;
	Bool	same_sign;

	/*
	 *	Correcting for aspect ratio here...
	 */
	delta_x = OlPixelToPoint(OL_HORIZONTAL, *x - anchor_x);
	delta_y = OlPixelToPoint(OL_VERTICAL, *y - anchor_y);
	same_sign = ((delta_y >= 0) == (delta_x >= 0));

	if (ABS(delta_x) > ABS(delta_y)) {
		if (same_sign)
			*y = anchor_y + OlPointToPixel(OL_VERTICAL, delta_x);
		else
			*y = anchor_y - OlPointToPixel(OL_VERTICAL, delta_x);
	} else {
		if (same_sign)
			*x = anchor_x + OlPointToPixel(OL_HORIZONTAL, delta_y);
		else
			*x = anchor_x - OlPointToPixel(OL_HORIZONTAL, delta_y);
	}

/*
	XWarpPointer(DisplayOfScreen(screen), None, window, 0, 0, 0, 0, *x, *y);
 */
}


/*
 *	The toolkit currently doesn't provide a routine for us to
 *	do this, so we've got to do it ourselves...
 */
void
BringDownPopup(wid)
Widget wid;
{
	if (XtIsRealized(wid)) {
		long	pushpin_state = WMPushpinIsOut;

		GetWMPushpinState(XtDisplay(wid), XtWindow(wid),
							&pushpin_state);
		switch (pushpin_state) {

			case WMPushpinIsIn:
				break;

			case WMPushpinIsOut:
			default:
				XtPopdown(wid);
				break;
		}
	}
}


int
power(x, n)		/* raise x to the n-th power; n > 0 */
int x, n;
{
	int p;

	for (p = 1; n > 0; --n)
		p *= x;
	return(p);
}


unsigned int
ConvertToPosInt(string)
char * string;
{
 	int		i,
			len;
	unsigned int	retval = 0;

	if (string && (len = strlen(string)) != 0) {
		for (i = 0; i < len; i++)
			if (!isdigit(string[i]) && string[i] != ' ')
				break;

		if (i == len)
			retval = (unsigned int) atoi(string);
	}
	return (retval);
}


void
SetStatus(newstate)
StatusType newstate;
{

	Cursor	c;
	long	wm_state;

	if (newstate == Busy) {
		c = OlGetBusyCursor(Toplevel);
		wm_state = WMWindowIsBusy;
	} else {
		c = OlGetStandardCursor(Toplevel);
		wm_state = WMWindowNotBusy;
	}

	XDefineCursor(DISPLAY, XtWindow(Toplevel), c);
	SetWMWindowBusy(DISPLAY, XtWindow(Toplevel), wm_state);

	if (PixmapIsDisplayed) {
		XDefineCursor(DISPLAY, XtWindow(PixmapPopup), c);
		SetWMWindowBusy(DISPLAY, XtWindow(PixmapPopup), wm_state);
	}
	if (XtIsRealized(FilePopup)) {
		XDefineCursor(DISPLAY, XtWindow(FilePopup), c);
		SetWMWindowBusy(DISPLAY, XtWindow(FilePopup), wm_state);
	}
	if (FileNotice != (Widget) NULL && XtIsRealized(FileNotice)) {
		XDefineCursor(DISPLAY, XtWindow(FileNotice), c);
		SetWMWindowBusy(DISPLAY, XtWindow(FileNotice), wm_state);
	}
	if (XtIsRealized(PropertiesPopup)) {
		XDefineCursor(DISPLAY, XtWindow(PropertiesPopup), c);
		SetWMWindowBusy(DISPLAY, XtWindow(PropertiesPopup), wm_state);
	}
}


void
ResetCursorColors()
{
	XColor	fgbg[2];

	fgbg[0].pixel = CurrentForeground;
#ifdef out
	if (CurrentBackground != CurrentForeground)
		fgbg[1].pixel = CurrentBackground;
	else if (CurrentBackground != WhitePixelOfScreen(SCREEN))
		fgbg[1].pixel = WhitePixelOfScreen(SCREEN);
	else
		fgbg[1].pixel = BlackPixelOfScreen(SCREEN);
#else
	/*
	 *	Currently the background color is fixed, so we can use a
	 *	heuristic borrowed from mfbResolveColor() to do better...
	 */

	XQueryColors(DISPLAY, PixmapColormap, fgbg, 1);
	/* 
	 * Gets intensity from RGB.  If intensity is <= half, pick white, else
	 * pick black.  This may well be more trouble than it's worth.
	 */
	fgbg[1].pixel = (((30L * (long)fgbg[0].red +
			   59L * (long)fgbg[0].green +
			   11L * (long)fgbg[0].blue) >> 8) <=
				(((1<<8)-1)*50)) ? WhitePixelOfScreen(SCREEN) :
						   BlackPixelOfScreen(SCREEN);
#endif

	XQueryColors(DISPLAY, PixmapColormap, fgbg, 2);
	XRecolorCursor(DISPLAY, OlGetStandardCursor(Toplevel),
						&fgbg[0], &fgbg[1]);
}


void
PopupMenu (shell, parent, popdown, x, y)
Widget shell;
Widget parent;
void (*popdown)();
int x;
int y;
{
		/* SAMC, (x, y) is (root_x, root_y) from ButtonPress.   */
		/*      should also pass in (x, y) from ButtonPress and */
		/*      use them as (init_x, init_y)...			*/
	OlPostPopupMenu (
		parent,
		shell,
		OL_MENU,	/* edit.c and evernts.c are only	*/
		 		/* interested in ButtonPress...		*/
		(OlPopupMenuCallbackProc)popdown,
		(Position)x, (Position)y,	/* root_x,root_y	*/
		(Position)x, (Position)y	/* init_x,init_y	*/
	);
}
