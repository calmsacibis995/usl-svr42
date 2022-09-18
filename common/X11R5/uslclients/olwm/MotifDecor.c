/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olwm:MotifDecor.c	1.13"
#endif

#include <stdio.h>

#include <X11/keysym.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/ShellP.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>

#include <Xol/OpenLookP.h>
#include <Xol/DynamicP.h>
#include <Xol/Stub.h>
#include <Xol/Olg.h>
#include <Xol/WSMcomm.h>
#include <Xol/Flat.h>

#include <wm.h>
#include <Xol/OlCursors.h>
#include <Xol/VendorI.h>
#include <WMStepP.h>

#include <Extern.h>
#include <limits.h>

/* Motif drawing functions */

extern void	DrawMotifButtonShadow OL_ARGS((Widget, Dimension, 
						Dimension, int));
extern void	DrawMotifMaximizeButton OL_ARGS((Widget, int));
extern void	DrawMotifMinimizeButton OL_ARGS((Widget, int));
extern void	DrawMotifMenuButton OL_ARGS((Widget, int ));
extern void	DrawMotifResizeCorners OL_ARGS((WMStepWidget));
extern void	DrawMotifTitleShadow OL_ARGS((Widget, int ));
static void	InitMotifComponentDecor OL_ARGS((Widget, int));
extern void	InitMotifDecorGCs OL_ARGS((Widget));
extern void	MotifFillFocusBackground OL_ARGS((WMStepWidget, Boolean,
					Dimension, Dimension));
extern void	SetMotifMins OL_NO_ARGS();
extern Pixel	GetLighterColor OL_ARGS((Screen *, Pixel));
extern Pixel 	GetDarkerColor OL_ARGS((Screen *, Pixel));

static Pixmap GetMonoPixmap OL_ARGS((Widget, int));

/*
 * "color" for decoration window that has focus
 */

#define mono_focus_color_width 16
#define mono_focus_color_height 16
static OLconst unsigned char mono_focus_color_bits[] = {
	0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 
	0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 
	0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
	0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa
};

/*
 * "color" for decoration windows that do not have focus
 */

#define mono_normal_color_width 16
#define mono_normal_color_height 16
static OLconst unsigned char mono_normal_color_bits[] = {
	0x11, 0x11, 0x44, 0x44, 0x11, 0x11, 0x44, 0x44, 
	0x11, 0x11, 0x44, 0x44, 0x11, 0x11, 0x44, 0x44, 
	0x11, 0x11, 0x44, 0x44, 0x11, 0x11, 0x44, 0x44,
	0x11, 0x11, 0x44, 0x44, 0x11, 0x11, 0x44, 0x44
};

static Pixmap ActiveMonoPixmap, MonoPixmap;


/*
 *************************************************************************
 * InitMotifDecorGCs.
 *	Argument 'w' is probably the Frame (root window) widget.
 * Basically, fill in the GC information in the motCompRes[] array-
 * separate information is needed for each component (client, icon,
 * title, menu, feedback.
 * Here is a potiential problem: we are assuming that the w argument
 * is a step widget - our initial and only call is from Display.c
 * (CreateGCs).  There may be a case where we will need GCs initialized
 * before a step widget exists - then this must be adjusted.
 * For example, if we get feedback, then the win mgr may be killed
 * before a window is ever mapped.  Then we wouldn't have a step widget.
 *
 ****************************procedure*header*****************************
 */
extern void
InitMotifDecorGCs OLARGLIST((w))
OLGRA(Widget, w)
{
int i;

	/* We need decoration GCs for each of the components:
	 * window, icon, title, menu, and feedback.
	 */
	for (i=0; i < NUM_MOTIF_COMPONENTS; i++)
		InitMotifComponentDecor(w, i);

} /* InitMotifDecorGCs */




/*
 *************************************************************************
 *
 * InitMotifComponentDecor.  For index i in the motCompRes array, set up
 * the necessary GCs to get us through - i represents a component, either:
 *	icon
 *	client (window frame)
 *	menu
 *	feedback
 *	title bar
 * But not all GCs are necessary for each component - for example, only
 * the icon and window frame components need active GCs.

 * Addendum by mlp - the title bar drawings, which can be affected by
 * mwm*client*title*resource_name, were not applied to the code for
 * generating active GCs, until recently - just to satisfy calls for
 * drawing buttons.  This can be taken out by simply changing the
 *  3 to a 2 in two if statements below (if ?? < 3), because TITLE
 * component is index 2.
 ****************************procedure*header*****************************
 */
static void
InitMotifComponentDecor OLARGLIST((w, i))
OLARG(Widget, w)
OLGRA(int, i)
{
	/* ASSUME FOR NOW, because it's the only call (from CreateGCs)
	 * that w is a step widget.
	 */
	WMStepWidget	wm = (WMStepWidget)w;
	WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
	XGCValues gcvals, gcvalstop, gcvalsbot;
	Display *display = XtDisplay(w);
	unsigned int width, height;
	Pixmap pix;
	int	status;
	unsigned long mask, topmask, botmask;
	GC	brightGC, darkGC;
	Pixel	brightfgcolor, darkfgcolor;
	MotifCompResourceInfo	*mcri = (MotifCompResourceInfo *)
							&(motCompRes[i]);
	MotCompAppearanceInfo	*mcai = (MotCompAppearanceInfo *)
							&(mcri->compai);
	Screen	*screen = XtScreen(w);
	Pixel	white = WhitePixelOfScreen(screen);
	Pixel	black = BlackPixelOfScreen(screen);

	/* just temporary */

	brightGC = OlgGetBrightGC(wmstep->mdm);
	darkGC = OlgGetBg3GC(wmstep->mdm);
	mask = GCForeground;
	/* Ignore status for now */
	status = XGetGCValues(display, brightGC, mask, &gcvals);
	brightfgcolor = gcvals.foreground;
	/* Ignore status for now */
	status = XGetGCValues(display, darkGC, mask, &gcvals);
	darkfgcolor = gcvals.foreground;

	if (brightfgcolor == darkfgcolor) {
		/* Could be monochrome, make colors distinct - black, white
		 * are fine.
		 */
		brightfgcolor = white;
		darkfgcolor = black;
	}

	/* Now let's go to work */

	/*  Make pixmaps for shadows */
	
	/* Get bright, dark GCs from wmstep->mdm */

	/* Non-iconic windows only */

	if (mcai->topShadowColor == mcai->background) {
		mcai->topShadowColor = GetLighterColor(XtScreen(w), mcai->topShadowColor);
	}
	if (mcai->bottomShadowColor == mcai->background) {
        	mcai->bottomShadowColor = GetDarkerColor(XtScreen(w), mcai->bottomShadowColor);
	}


	topmask = botmask = GCForeground;
	if (mcai->topShadowColor == ULONG_MAX ||
			(mcai->bottomShadowColor == ULONG_MAX) ) {
		gcvalstop.foreground = brightfgcolor;
		gcvalsbot.foreground = darkfgcolor;
	}
	else {
		gcvalstop.foreground = mcai->topShadowColor;
		gcvalsbot.foreground = mcai->bottomShadowColor;
	}
	if (mcai->topShadowPixmap) {
		gcvalstop.stipple = mcai->topShadowPixmap;
		gcvalstop.fill_style = FillStippled;
		topmask |= (GCStipple|GCFillStyle);
	}
	if (mcai->bottomShadowPixmap) {
		gcvalsbot.stipple = mcai->bottomShadowPixmap;
		gcvalsbot.fill_style = FillStippled;
		botmask |= (GCStipple|GCFillStyle);
	}
	mcri->topGC = XtGetGC((Widget)w, topmask, &gcvalstop);
	mcri->botGC = XtGetGC((Widget)w, botmask, &gcvalsbot);

	/* all active windows and icons */

	/* Active foreground, background Top, Bottom shadow color GCs */
	/*  For now, make the default active top and bottom the same
	 * as the non-active top and bottom, for lack of imminent
	 * intuition.
	 */
	
	if (i < 3) {
		if (mcai->activeTopShadowColor == mcai->activeBackground) {
			mcai->activeTopShadowColor = 
				GetLighterColor(XtScreen(w), mcai->activeTopShadowColor);
		}
		if (mcai->activeBottomShadowColor == mcai->activeBackground) {
			mcai->activeBottomShadowColor =
				GetDarkerColor(XtScreen(w), mcai->activeBottomShadowColor);
		}

		topmask = botmask = GCForeground;
		if (mcai->activeTopShadowColor == ULONG_MAX ||
				(mcai->activeBottomShadowColor == ULONG_MAX) ) {
			gcvalstop.foreground = brightfgcolor;
			gcvalsbot.foreground = darkfgcolor;
		}
		else {
			gcvalstop.foreground = mcai->activeTopShadowColor;
			gcvalsbot.foreground = mcai->activeBottomShadowColor;
		}
		/* Any pixmaps on these active ones?? */
		if (mcai->activeTopShadowPixmap) {
			gcvalstop.stipple = mcai->activeTopShadowPixmap;
			gcvalstop.fill_style = FillStippled;
			topmask |= (GCStipple|GCFillStyle);
		}
		if (mcai->activeBottomShadowPixmap) {
			gcvalsbot.stipple = mcai->activeBottomShadowPixmap;
			gcvalsbot.fill_style = FillStippled;
			botmask |= (GCStipple|GCFillStyle);
		}
		mcri->activetopGC = XtGetGC((Widget)w, topmask, &gcvalstop);
		mcri->activebotGC = XtGetGC((Widget)w, botmask, &gcvalsbot);

		/* Active foreground, background GCs */
		gcvals.foreground = mcai->activeForeground;
		mcri->activefgGC = XtGetGC((Widget)w, GCForeground, &gcvals);

		/* For activeBackground, if set to ULONG_MAX, then use
	 	 * wmrcs->inputFocusColor.
		 */
		if (mcai->activeBackground == ULONG_MAX)
			mcai->activeBackground = wmrcs->inputFocusColor;
		/* The way we are using the backgound GC: if there is
 		 * an active background pixmap suppiled, then
		 * set the GC foreground to the activeForeground resource;
		 * we can set the gc background to activeBackground,
	 	 * but it's not important, because we use
	 	 * XSetWindowBackground for this background color.
	 	 * Now, if there is an activeBackgroundPixmap, we can
	 	 * fill in the appropriate GC fields and we will get
	 	 * a bitmap (pixmap) colored in the foreground.
	 	 */
		gcvals.foreground = mcai->activeBackground;
		mask = GCForeground;
		if (DefaultDepthOfScreen(screen) == 1
			&& !mcai->activeBackgroundPixmap) {
			mcai->activeBackgroundPixmap = GetMonoPixmap(w, 1);
			gcvals.foreground = mcai->activeForeground;
			gcvals.background = mcai->activeBackground;
			mask |= GCBackground;
		}
		if (mcai->activeBackgroundPixmap) {
			gcvals.stipple = mcai->activeBackgroundPixmap;
			gcvals.fill_style = FillStippled;
			mask |= (GCFillStyle|GCStipple);
		}
		mcri->activebgGC = XtGetGC((Widget)w, mask, &gcvals);
	} /* i < 3 */

	/* For background, nothing is necessary unless there is a
	 * backgroundpixmap.  If no backgroundPixmap, just use
	 * XSetWindowBackground().
	 */
	mask = 0;
	if (mcai->backgroundPixmap == NULL) {
		if (DefaultDepthOfScreen(screen) == 1) {
			mcai->backgroundPixmap = GetMonoPixmap(w, 0);
			mask = GCBackground;
			gcvals.background = mcai->background;
			gcvals.foreground = mcai->foreground;
		}
		else mcri->backgroundGC = (GC)NULL;
	}
	else gcvals.foreground = mcai->background;
	if (mcai->backgroundPixmap != NULL) {
		gcvals.stipple = mcai->backgroundPixmap;
		gcvals.fill_style = FillStippled;
		mcri->backgroundGC = XtGetGC((Widget)w,
			GCForeground | GCStipple | GCFillStyle | mask, &gcvals);
	} 
} /* InitMotifComponentDecor */


/*
 *************************************************************************
 * DrawMotifMenuButton.
 *
 ****************************procedure*header*****************************
 */
extern void
DrawMotifMenuButton OLARGLIST((w, mode))
OLARG(Widget, w)
OLGRA(int, mode)
{
	WMStepWidget wm = (WMStepWidget)w;
	WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
	Dimension x, y, width, ht;
	int vwidth, hwidth;
	XRectangle rect[4];
	Boolean active = wmstep->is_current;
	MotifCompResourceInfo	*mcri = (MotifCompResourceInfo *)
						&(motCompRes[TITLE_COMP]);

	if (Resizable(wm)) {
		vwidth = wmstep->metrics->motifVResizeBorderWidth;
		hwidth = wmstep->metrics->motifHResizeBorderWidth;
	}
	else {
		if (wmstep->decorations & WMBorder) {
		  vwidth = wmstep->metrics->motifVNResizeBorderWidth;
		  hwidth = wmstep->metrics->motifHNResizeBorderWidth;
		}
		else /* No border */
			vwidth = hwidth = 1;
	} /* Not resizable */

	x = hwidth;
	y = vwidth;
	DrawMotifButtonShadow(w, x, y, mode);
	width = wm->wmstep.metrics->motifButtonWidth / 2;
	ht = width/2;
	x = x + width - ht;
	y = y + wm->wmstep.metrics->motifButtonHeight / 2 - ht/2;
        _OlgDrawBorderShadow(XtScreen(w), XtWindow(w), 
	  wm->wmstep.mdm, OL_SHADOW_OUT, 
	  1, x, y, width, ht,
	  active ? mcri->activetopGC : mcri->topGC,
	  active ? mcri->activebotGC : mcri->botGC);
} /* DrawMotifMenuButton */

/*
 *************************************************************************
 * DrawMotifMaximizeButton().
 * The internal part of the maximize button width == height == 1/2 the 
 * width (ht) of the motifButtonWidth(Height).
 ****************************procedure*header*****************************
 */
extern void
DrawMotifMaximizeButton OLARGLIST((w, mode))
OLARG(Widget, w)
OLGRA(int, mode)
{
	Dimension x, y, width, ht;
	WMStepWidget wm = (WMStepWidget)w;
	WMStepPart *wmstep = (WMStepPart *)&wm->wmstep;
	int vwidth, hwidth;
	XRectangle rect[4];
	Boolean active = wmstep->is_current;
	MotifCompResourceInfo	*mcri = (MotifCompResourceInfo *)
						&(motCompRes[TITLE_COMP]);

	if (Resizable(wm)) {
		vwidth = wmstep->metrics->motifVResizeBorderWidth;
		hwidth = wmstep->metrics->motifHResizeBorderWidth;
	}
	else {
		if (wmstep->decorations & WMBorder) {
		  vwidth = wmstep->metrics->motifVNResizeBorderWidth;
		  hwidth = wmstep->metrics->motifHNResizeBorderWidth;
		}
		else {
			hwidth = vwidth = 1;
		}
	}

	x = w->core.width - hwidth - wmstep->metrics->motifButtonWidth;
	y = vwidth;
	DrawMotifButtonShadow(w, x, y, mode);
	width = wm->wmstep.metrics->motifButtonWidth/2;
	ht = wm->wmstep.metrics->motifButtonHeight/2;
	x = x + width/2;
	y = y + ht/2;
        _OlgDrawBorderShadow(XtScreen(w), XtWindow(w), 
	  wm->wmstep.mdm, wmstep->size == WMFULLSIZE ?
			OL_SHADOW_IN : OL_SHADOW_OUT, 
	  1, x, y, width, ht,
	  active ? mcri->activetopGC : mcri->topGC,
	  active ? mcri->activebotGC : mcri->botGC);
} /* DrawMotifMaximizeButton */

/*
 *************************************************************************
 * DrawMotifMinimizeButton.
 *
 ****************************procedure*header*****************************
 */
extern void
DrawMotifMinimizeButton OLARGLIST((w, mode))
OLARG(Widget, w)
OLGRA(int, mode)
{
	Dimension x, y;
	WMStepWidget wm = (WMStepWidget)w;
	WMStepPart *wmstep = (WMStepPart *)&wm->wmstep;
	int vwidth, hwidth;
	XRectangle rect[4];
	Boolean active = wmstep->is_current;
	MotifCompResourceInfo	*mcri = (MotifCompResourceInfo *)
						&(motCompRes[TITLE_COMP]);

	if (Resizable(wm)) {
		vwidth = wmstep->metrics->motifVResizeBorderWidth;
		hwidth = wmstep->metrics->motifHResizeBorderWidth;
	}
	else {
		if (wmstep->decorations & WMBorder) {
		  vwidth = wmstep->metrics->motifVNResizeBorderWidth;
		  hwidth = wmstep->metrics->motifHNResizeBorderWidth;
		}
		else {
			hwidth = vwidth = 1;
		}
	} /* Not resizable */

/* Make the minimize button 4 x 4 (pixels, with the shadow being 1,
 * and the inside being 2 pixels deep.
 */

	x = w->core.width - hwidth;
	if (wmstep->decorations & WMMaximize)
		x -= 2 * wm->wmstep.metrics->motifButtonWidth;
	else
		x -= wm->wmstep.metrics->motifButtonWidth;
	y = vwidth;
	DrawMotifButtonShadow(w, x, y, mode);
	x = x + wm->wmstep.metrics->motifButtonWidth /2  - 2;
	y = y + wm->wmstep.metrics->motifButtonHeight / 2 - 2;
        _OlgDrawBorderShadow(XtScreen(w), XtWindow(w), 
	  wm->wmstep.mdm, OL_SHADOW_OUT, 
/*
	  1, x, y, 4, 4);
 */
	  1, x, y, 4, 4,
	  active ? mcri->activetopGC : mcri->topGC,
	  active ? mcri->activebotGC : mcri->botGC);
} /* DrawMotifMinimizeButton */

/*
 *************************************************************************
 * DrawMotifTitleShadow.
 *
 ****************************procedure*header*****************************
 */
extern void
DrawMotifTitleShadow OLARGLIST((w, mode))
OLARG(Widget, w)
OLGRA(int, mode)
{
	WMStepWidget wm = (WMStepWidget)w;
	WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
	Screen *scr = XtScreen(w);
	Window win = XtWindow(w);
	Dimension x, y, width, height;
	int vwidth, hwidth;
	XRectangle rect[4];
	Boolean active = wmstep->is_current;
	MotifCompResourceInfo	*mcri = (MotifCompResourceInfo *)
						&(motCompRes[TITLE_COMP]);

	if (Resizable(wm)) {
		vwidth = wmstep->metrics->motifVResizeBorderWidth;
		hwidth = wmstep->metrics->motifHResizeBorderWidth;
	}
	else {
		if (wmstep->decorations & WMBorder) {
		  vwidth = wmstep->metrics->motifVNResizeBorderWidth;
		  hwidth = wmstep->metrics->motifHNResizeBorderWidth;
		}
		else
			hwidth = vwidth = 1;
	}

	/* start: title can use all of header */
	width = wm->core.width - 2*hwidth;

	if (wmstep->decorations & WMMenuButton) {
		x = hwidth + wmstep->metrics->motifButtonWidth;
		/* menu button - reduce space for title */
		width -= wmstep->metrics->motifButtonWidth;
	}
	else {
		x = hwidth;
	}
	y = vwidth;

	/* Minimize button - more space reduction for title */
	if (wmstep->decorations & WMMinimize)
		width -= wmstep->metrics->motifButtonWidth;

	/* Maximize button - even more space reduction for title */
	if (wmstep->decorations & WMMaximize)
		width -= wmstep->metrics->motifButtonWidth;

	height = wmstep->metrics->motifButtonHeight;

	if (mode == AM_NORMAL)
		_OlgDrawBorderShadow(scr, win,
			wmstep->mdm, OL_SHADOW_OUT, 1, x, y,
			width, height, 
	  active ? mcri->activetopGC : mcri->topGC,
	  active ? mcri->activebotGC : mcri->botGC);
	else
		_OlgDrawBorderShadow(scr, win,
			wmstep->mdm, OL_SHADOW_IN, 1, x, y,
			width, height,
	  active ? mcri->activetopGC : mcri->topGC,
	  active ? mcri->activebotGC : mcri->botGC);

	/* In mwms internal border, the inner border below
	 * the title bar is considered the title - it gets the
	 *  color of the title component, but don't use it here yet
	 */
} /* DrawMotifTitleShadow */


/*
 *************************************************************************
 * DrawMotifButtonShadow.
 *  - Request to draw a border shadow at position (x, y),
 *    If mode = AM_NORMAL, draw shadow out,
 *	else draw Shadow In.
 *    Always draw shadow width == 1.
 * Addendum, 2/24/92:  This code will assume that we are drawing
 * ONLY the buttons in the title bar - therefore, we will use the GCs
 * for the title component.
 *
 ****************************procedure*header*****************************
 */
extern void
DrawMotifButtonShadow OLARGLIST((w, x, y, mode))
OLARG(Widget, w)
OLARG(Dimension, x)
OLARG(Dimension, y)
OLGRA(int, mode)
{
Screen *scr = XtScreen(w);
Window win = XtWindow(w);
WMStepWidget wm = (WMStepWidget)w;
WMStepPart *wmstep = (WMStepPart *) &(wm->wmstep);
Boolean active = wmstep->is_current;
MotifCompResourceInfo	*mcri = (MotifCompResourceInfo *)
						&(motCompRes[TITLE_COMP]);


	if (mode == AM_NORMAL)
		_OlgDrawBorderShadow(scr, win,
			wmstep->mdm, OL_SHADOW_OUT, 1, x, y,
			wmstep->metrics->motifButtonWidth,
			wmstep->metrics->motifButtonHeight,
	  active ? mcri->activetopGC : mcri->topGC,
	  active ? mcri->activebotGC : mcri->botGC);
	else
		_OlgDrawBorderShadow(scr, win,
			wmstep->mdm, OL_SHADOW_IN, 1, x, y,
			wmstep->metrics->motifButtonWidth,
			wmstep->metrics->motifButtonHeight,
	  active ? mcri->activetopGC : mcri->topGC,
	  active ? mcri->activebotGC : mcri->botGC);
} /* DrawMotifButtonShadow */

/*
 *************************************************************************
 * DrawMotifResizeCorners.
 * In 3D, of course.
 *
 ****************************procedure*header*****************************
 */
extern void
DrawMotifResizeCorners OLARGLIST((w))
OLGRA(WMStepWidget, w)
{
WMStepPart *wmstep = &(w->wmstep);
int vwidth, hwidth;
XRectangle lightrect[8], darkrect[8];
GC lightGC = OlgGetBrightGC(wmstep->mdm);
GC darkGC = OlgGetBg3GC(wmstep->mdm);
Display *display = XtDisplay(w);
Window window = XtWindow(w);
Boolean active = wmstep->is_current;
MotifCompResourceInfo	*mcri = (MotifCompResourceInfo *)
						&(motCompRes[CLIENT_COMP]);

	/* vwidth = the outer border width (vertically - y axis).
	 * hwidth = outer border width (horizontally - x axis).
	 */
	vwidth = wmstep->metrics->motifVResizeBorderWidth;
	hwidth = wmstep->metrics->motifHResizeBorderWidth;

	/* Top left - part of upper left / top grabber */
	darkrect[0].x = hwidth + wmstep->metrics->motifButtonWidth - 1;
	darkrect[0].y = darkrect[0].width = 1;
	darkrect[0].height = vwidth - 1;

	lightrect[0].x = darkrect[0].x + 1;
	lightrect[0].y = darkrect[0].y + 1;
	lightrect[0].width = 1;
	lightrect[0].height = darkrect[0].height;

	/* Top right - part of upper right/top grabber */
	lightrect[1].x = w->core.width - hwidth -
				 wmstep->metrics->motifButtonWidth;
	lightrect[1].y = 1;
	lightrect[1].width = 1;
	lightrect[1].height = lightrect[0].height;

	darkrect[1].x = lightrect[1].x - 1;
	darkrect[1].y = darkrect[1].width = 1;
	darkrect[1].height = darkrect[0].height;

	/* Bottom left - part of lower left / lower grabber - dark, light */
	darkrect[2].x = darkrect[0].x;
	darkrect[2].y = w->core.height - vwidth + 1;
	darkrect[2].width = 1;
	darkrect[2].height = vwidth - 2;

	lightrect[2].x = lightrect[0].x;
	lightrect[2].y = darkrect[2].y;
	lightrect[2].width = 1;
	lightrect[2].height = darkrect[2].height;

	/* Bottom right - part of lower right/ lower grabber */
	lightrect[3].x = lightrect[1].x;
	lightrect[3].y = lightrect[2].y;
	lightrect[3].width = 1;
	lightrect[3].height = lightrect[2].height;

	darkrect[3].x = darkrect[1].x;
	/* Fix from earlier date for darkect[3].y */
	darkrect[3].y = darkrect[2].y;
	darkrect[3].width = 1;
	darkrect[3].height = darkrect[1].height;

	/* now 4 through 7 - clockwise from the upper right / right */
 
	/*  Right side, top grabber (upper right / right)-dark on top of light*/
	darkrect[4].x = w->core.width - hwidth + 1;
	darkrect[4].y = vwidth + wmstep->metrics->motifButtonHeight;
	darkrect[4].width = hwidth - 2; /* -2, because you start just to
					 * rt. of inner border and go to
					 * 1 less than right border */
	darkrect[4].height = 1;

	lightrect[4].x = darkrect[4].x;
	lightrect[4].y = darkrect[4].y + 1;
	lightrect[4].width = darkrect[4].width;
	lightrect[4].height = 1;

	/* right side, lower grabber (lower right / right) - dark over light */
	lightrect[5].x = lightrect[4].x;
	lightrect[5].y = w->core.height - darkrect[4].y;
	lightrect[5].width = lightrect[4].width;
	lightrect[5].height = 1;

	darkrect[5].x = lightrect[5].x;
	darkrect[5].y = lightrect[5].y - 1;
	darkrect[5].width = darkrect[4].width;
	darkrect[5].height = 1;

	/* left side, lower grabber (lower left/left) - dark over light */
	darkrect[6].x = darkrect[6].height = 1;
	darkrect[6].y = darkrect[5].y;
	darkrect[6].width = hwidth - 1;

	lightrect[6].x = lightrect[6].height = 1;
	lightrect[6].y = lightrect[5].y;
	lightrect[6].width = darkrect[5].width;

	/* left side, upper grabber (upper left/left) - dark over light */
	lightrect[7].x = lightrect[7].height = 1;
	lightrect[7].y = lightrect[4].y;
	lightrect[7].width = lightrect[6].width;

	darkrect[7].x = darkrect[7].height = 1;
	darkrect[7].y = darkrect[4].y;
	darkrect[7].width = lightrect[6].width;

/*
	XFillRectangles(display, window, darkGC, darkrect, 8);
	XFillRectangles(display, window, lightGC, lightrect, 8);
 */
	XFillRectangles(display, window,
	  active ? mcri->activebotGC : mcri->botGC,
	  darkrect, 8);
	XFillRectangles(display, window, active ? mcri->activetopGC :
			mcri->topGC, lightrect, 8);
} /* DrawMotifResizeCorners */


/*
extern void
SetMotifMins()
{
int i;
	for (i=0; i < 12; i+= 3){
	  if (wmmetrics[i].scale == PermScale &&
			wmmetrics[i].resolution == ScreenResolution) {
		if (wmmetrics[i].compiled == 'N') {
			CompileMetrics(XtScreen(Frame), i);
			break;
		}
	  }
	}
	if (i >= 12 && wmmetrics[3].compiled == 'N') {
		i = 3;
		CompileMetrics(XtScreen(Frame), 3);
	}
	Mresminwidth = 3 * wmmetrics[i].motifButtonWidth +
			2 * wmmetrics[i].motifHResizeBorderWidth;
	Mnoresminwidth = 3 * wmmetrics[i].motifButtonWidth +
			2 * wmmetrics[i].motifHNResizeBorderWidth;
	Mresminheight =  wmmetrics[i].motifButtonHeight +
			2 * wmmetrics[i].motifVResizeBorderWidth + 5;
	Mnoresminheight =  wmmetrics[i].motifButtonHeight +
			2 * wmmetrics[i].motifVNResizeBorderWidth + 5;
} /* SetMotifMins */

/*
 *************************************************************************
 * MotifFillFocusBackground.
 * Set the color of the window background to either the non-active window
 * color or the active window background color.
 *
 ****************************procedure*header*****************************
 */
extern void
MotifFillFocusBackground OLARGLIST((w, flag, hwidth, vwidth))
OLARG(WMStepWidget, w)
OLARG(Boolean, flag)
OLARG(Dimension, hwidth)
OLGRA(Dimension, vwidth)
{
XRectangle	rect[4];
Display		*display = XtDisplay((Widget)w);
Window		window = XtWindow((Widget)w);
WMStepPart	*wmstep = (WMStepPart *)(&w->wmstep);
GC		use_gc, tmp;
XGCValues	val;
MotifCompResourceInfo	*mcri = (MotifCompResourceInfo *)
				&(motCompRes[CLIENT_COMP]);
MotCompAppearanceInfo	*mcai = (MotCompAppearanceInfo *) &(mcri->compai);

   if (flag && mcai->activeBackgroundPixmap ||
				!flag && mcai->backgroundPixmap) {
	/* backgroundPixmap forces us to use rectangles.  For the background
	 * color, just use SetValues.
	 */
	if (flag) {
		use_gc = mcri->activebgGC;
		XtVaSetValues((Widget)w, XtNbackground, flag?
			mcai->activeBackground:
			/* wmstep->saveBackgroundPixel, (char *)0); */
			mcai->background, (char *)0);
	}
	else {
		use_gc = mcri->backgroundGC;
		XtVaSetValues((Widget)w, XtNbackground, flag?
			mcai->activeBackground:
			/*wmstep->saveBackgroundPixel, (char *)0);*/
			mcai->background, (char *)0);
	}

	/* The top strip */
	rect[0].x = rect[0].y = 0;
	rect[0].width = w->core.width;
	rect[0].height = wmstep->metrics->motifButtonHeight + vwidth;

	/* The left strip */
	rect[1].x = rect[1].y = 0;
	rect[1].width = hwidth;
	rect[1].height = w->core.height;

	/* The right strip */
	rect[2].x = w->core.width - hwidth;
	rect[2].y = 0;
	rect[2].width = hwidth;
	rect[2].height = w->core.height;

	/* The bottom strip */
	rect[3].x = 0;
	rect[3].y = w->core.height - vwidth;
	rect[3].width = w->core.width;
	rect[3].height = vwidth;

	XGetGCValues(display, use_gc, GCBackground, &val);
	val.foreground = val.background;
	tmp = XCreateGC(display, window, GCForeground, &val);
	XFillRectangles(display, window, tmp, rect, 4);
	XFreeGC(display, tmp);
	XFillRectangles(display, window, use_gc, rect, 4);

   } /* flag && pixmap */
   else {
	/*  No background pixmaps - this is easy!!!
	 */
	XtVaSetValues((Widget)w, XtNbackground, flag?
		mcai->activeBackground :
		/*wmstep->saveBackgroundPixel, (char *)0);*/
		mcai->background, (char *)0);
   } /* else */
} /* MotifFillFocusBackground */

extern void
SetShadowGCS OLARGLIST((w, i, flag))
OLARG(Widget, w)
OLARG(int, i)
OLGRA(int, flag)
{
	WMStepWidget	wm = (WMStepWidget)w;
	WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
	XGCValues gcvals, gcvalstop, gcvalsbot;
	Display *display = XtDisplay(w);
	unsigned int width, height;
	Pixmap pix;
	int	status;
	unsigned long mask, topmask, botmask;
	GC	brightGC, darkGC;
	Pixel	brightfgcolor, darkfgcolor;
	MotifCompResourceInfo	*mcri = (MotifCompResourceInfo *)
							&(motCompRes[i]);
	MotCompAppearanceInfo	*mcai = (MotCompAppearanceInfo *)
							&(mcri->compai);
	Screen	*screen = XtScreen(w);
	Pixel	white = WhitePixelOfScreen(screen);

	if (flag) {
		topmask = botmask = GCForeground;
		gcvalstop.foreground = mcai->activeTopShadowColor;
		gcvalsbot.foreground = mcai->activeBottomShadowColor;
 
		/* Any pixmaps on these active ones?? */
		if (mcai->activeTopShadowPixmap) {
			gcvalstop.stipple = mcai->activeTopShadowPixmap;
			gcvalstop.fill_style = FillStippled;
			topmask |= (GCStipple|GCFillStyle);
		}
		if (mcai->activeBottomShadowPixmap) {
			gcvalsbot.stipple = mcai->activeBottomShadowPixmap;
			gcvalsbot.fill_style = FillStippled;
			botmask |= (GCStipple|GCFillStyle);
		}
		if (mcri->activetopGC) {
			XtReleaseGC((Widget) w, mcri->activetopGC);
			mcri->activetopGC=NULL;
		}
		mcri->activetopGC = XtGetGC((Widget)w, topmask, &gcvalstop);
		if (mcri->activebotGC) {
			XtReleaseGC((Widget) w, mcri->activebotGC);
			 mcri->activebotGC=NULL;
		}
		mcri->activebotGC = XtGetGC((Widget)w, botmask, &gcvalsbot);

		/* Active foreground, background GCs */
		gcvals.foreground = mcai->activeForeground;
		if (mcri->activefgGC)	{
			 XtReleaseGC((Widget) w, mcri->activefgGC);
			mcri->activefgGC=NULL;
		}
		mcri->activefgGC = XtGetGC((Widget)w, GCForeground, &gcvals);

		/* For activeBackground, if set to ULONG_MAX, then use
	 	 * wmrcs->inputFocusColor.
		 */
		if (mcai->activeBackground == ULONG_MAX)
			mcai->activeBackground = wmrcs->inputFocusColor;
		gcvals.foreground = mcai->activeBackground;
		mask = GCForeground;
		if (mcai->activeBackgroundPixmap) {
			gcvals.stipple = mcai->activeBackgroundPixmap;
			gcvals.fill_style = FillStippled;
			mask |= (GCFillStyle|GCStipple);
		}
		if (mcri->activebgGC) 	{
			XtReleaseGC((Widget) w, mcri->activebgGC);
			mcri->activebgGC=NULL;
		}
		mcri->activebgGC = XtGetGC((Widget)w, mask, &gcvals);
	} 
	else {	
		topmask = botmask = GCForeground;
		gcvalstop.foreground = mcai->topShadowColor;
		gcvalsbot.foreground = mcai->bottomShadowColor;
		if (mcai->topShadowPixmap) {
			gcvalstop.stipple = mcai->topShadowPixmap;
			gcvalstop.fill_style = FillStippled;
			topmask |= (GCStipple|GCFillStyle);
		}
		if (mcai->bottomShadowPixmap) {
			gcvalsbot.stipple = mcai->bottomShadowPixmap;
			gcvalsbot.fill_style = FillStippled;
			botmask |= (GCStipple|GCFillStyle);
		}
		if (mcri->topGC) {
			XtReleaseGC((Widget) w, mcri->topGC);
			mcri->topGC=NULL;
		}
		mcri->topGC = XtGetGC((Widget)w, topmask, &gcvalstop);
		if (mcri->botGC) 	{
			XtReleaseGC((Widget) w, mcri->botGC);
			mcri->botGC=NULL;
		}
		mcri->botGC = XtGetGC((Widget)w, botmask, &gcvalsbot);
		/* For background, nothing is necessary unless there is a
		 * backgroundpixmap.  If no backgroundPixmap, just use
		 * XSetWindowBackground().
		 */
		if (mcai->backgroundPixmap == NULL)
			mcri->backgroundGC = (GC)NULL;
		else {
			gcvals.foreground = mcai->background;
			gcvals.stipple = mcai->backgroundPixmap;
			gcvals.fill_style = FillStippled;
			mcri->backgroundGC = XtGetGC((Widget)w,
				GCForeground | GCStipple | GCFillStyle, &gcvals);
		} 
	} 

} /* SetShadowGCS */

extern Pixel 
GetLighterColor OLARGLIST((scr, color))
OLARG(Screen *, scr)
OLGRA(Pixel, color)
{
	Visual	*visual;
	Colormap	cmap;
	XColor	newColor;
	Pixel	white = WhitePixelOfScreen(scr);
	Pixel	black = BlackPixelOfScreen(scr);

	visual = DefaultVisualOfScreen (scr);
	cmap = DefaultColormapOfScreen (scr);

	if (color == white) return black;

	newColor.pixel = color;
	XQueryColor (DisplayOfScreen (scr), cmap, &newColor);
	if (visual->class != StaticColor && visual->map_entries > 16) {
		newColor.red = (unsigned) (newColor.red * 9l) / 100;
		newColor.green = (unsigned) (newColor.green * 9l) / 100;
		newColor.blue = (unsigned) (newColor.blue * 9l) / 100;
		newColor.flags = DoRed | DoGreen | DoBlue;
		if (XAllocColor (DisplayOfScreen (scr), cmap, &newColor))
			    return newColor.pixel;
	}
	return white;
}

extern Pixel 
GetDarkerColor OLARGLIST((scr, color))
OLARG(Screen *, scr)
OLGRA(Pixel, color)
{
	Visual	*visual;
	Colormap	cmap;
	XColor	newColor;
	Pixel	white = WhitePixelOfScreen(scr);
	Pixel	black = BlackPixelOfScreen(scr);

	visual = DefaultVisualOfScreen (scr);
	cmap = DefaultColormapOfScreen (scr);

	if (color == black) return white;

	newColor.pixel = color;
	XQueryColor (DisplayOfScreen (scr), cmap, &newColor);
	if (visual->class != StaticColor && visual->map_entries > 16) {
		newColor.red = (unsigned) (newColor.red / 9l) * 100;
		newColor.green = (unsigned) (newColor.green / 9l) * 100;
		newColor.blue = (unsigned) (newColor.blue / 9l) * 100;
		newColor.flags = DoRed | DoGreen | DoBlue;
		if (XAllocColor (DisplayOfScreen (scr), cmap, &newColor))
			    return newColor.pixel;
	}
	return black;
}

static Pixmap
GetMonoPixmap OLARGLIST((w, flag))
OLARG(Widget, w) 
OLGRA(int, flag)
{

	if (flag) {
		if (!ActiveMonoPixmap) ActiveMonoPixmap 
			= XCreateBitmapFromData(XtDisplay(w),
				XtWindow(Frame),
				(OLconst char *)mono_focus_color_bits,
				mono_focus_color_width,
				mono_focus_color_height);
		return ActiveMonoPixmap;
	}
	else if (!MonoPixmap) MonoPixmap
			= XCreateBitmapFromData(XtDisplay(w),
                        	XtWindow(Frame),
				(OLconst char *)mono_normal_color_bits,
				mono_normal_color_width,
                        	mono_normal_color_height); 
		return MonoPixmap;
}
