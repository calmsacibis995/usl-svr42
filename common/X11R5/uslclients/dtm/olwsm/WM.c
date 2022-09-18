/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtm:olwsm/WM.c	1.13"
#endif

#include <stdio.h>
#include "X11/IntrinsicP.h"
#include "X11/StringDefs.h"

#include "Xol/OpenLookP.h"
#include "Xol/OlgP.h"

#include "WMP.h"
#include "error.h"

/*
 * This is a simple composite that takes exactly one child,
 * and wraps it in an OPEN LOOK window decoration.
 *
 * This code draws the sample window in the color property sheet.
 * It has been stolen from the code for the window manager, but has
 * been reduced considerably to remove capabilities not needed for
 * the sample. It's also been turned into a composite.
 */

/*
 * Macros:
 */

#define NWRECT(X,Y,W,H)	\
	NWrect[num_NW].x = X;						\
	NWrect[num_NW].y = Y;						\
	NWrect[num_NW].width = W;					\
	NWrect[num_NW].height = H;					\
	num_NW++

#define SERECT(X,Y,W,H)	\
	SErect[num_SE].x = X;						\
	SErect[num_SE].y = Y;						\
	SErect[num_SE].width = W;					\
	SErect[num_SE].height = H;					\
	num_SE++

#define PixelWidth(screen,points) \
	OlScreenPointToPixel(OL_HORIZONTAL, points, screen)
#define PixelHeight(screen,points) \
	OlScreenPointToPixel(OL_VERTICAL, points, screen)

#define ConvertX(s,v)  ((v = PixelWidth(s, v) / 10) > 0? v : (v=1,v))
#define ConvertY(s,v)  ((v = PixelHeight(s, v) / 10) > 0? v : (v=1,v))

#define CornerX(w,m)		(m->cornerX)
#define CornerY(w,m)		(m->cornerY)
#define Cornerx(w,m)		(m->cornerx)
#define Cornery(w,m)		(m->cornery)
#define LineWidX(w,m)		(m->linewidX)
#define LineWidY(w,m)		(m->linewidY)
#define LineWidx(w,m)		(m->linewidx)
#define LineWidy(w,m)		(m->linewidy)
#define GapX(w,m)		(m->gapx)
#define GapY(w,m)		(m->gapy)
#define Offset(w,m)		(m->offset)
#define MarkWid(w,m)		(m->markwid)
#define BorderX(w,m)		(LineWidX(w,m) + GapX(w,m))
#define BorderY(w,m)		(LineWidY(w,m) + GapY(w,m))
#define BannerHt(w,m)		(m->bannerht)
#define BannerWid(w,m)		(w->core.width - BorderX(w,m) * 2)
#define Baseline(w,m)		(BorderY(w,m) + BannerHt(w,m) - m->baseline)
#define LineHt(w,m)		(m->linewid + m->hgap1 + m->hgap2)

#define LN		0
#define NW		1
#define NE		2
#define SE		3
#define SW		4
#define BD		5
#define BN		6
#define PP		7
#define MM		8

char msg[BUFSIZ];

/*
 * Local data:
 */

#define _12PT_BASE_WINDOW	3

static WMmetrics	wmmetrics[] = {
  /*                          ALL MEASUREMENTS IN TENTHS OF POINTS
   *                                              ====== == ======
   * scale
   * CornerX
   * CornerY
   * Cornerx
   * Cornery
   * LineWidX
   * LineWidY
   * linewidx
   * Linewidy
   * Gapx
   * Gapy
   * Bannerht
   * hgap1
   * linewid
   * hgap2
   * offset (pushpin/menu mark)
   * baseline
   * markwid
   * selectwidth (horizontal only)
   *
   * In sets of:
   *
   * base window 
   * icon window
   * nohd window
   *
   */

{10,100,100, 40, 40, 20, 20,10,10, 30, 30,160, 10, 10, 10,100, 70,320, 8},
{10,  0,  0,  0,  0, 20, 20, 0, 0, 30, 30,160, 10, 10, 10,100, 70,320,10},
{10,  0,  0,  0,  0, 20, 20, 0, 0, 30, 30,  0, 10, 10, 10,100, 70,320, 8},

{12,110,110, 40, 40, 20, 20,10,10, 30, 30,180, 10, 10, 10,120, 70,320,10},
{12,  0,  0,  0,  0, 20, 20, 0, 0, 30, 30,180, 10, 10, 10,120, 70,320,10},
{12,  0,  0,  0,  0, 20, 20, 0, 0, 30, 30,  0, 10, 10, 10,120, 70,320,10},

{14,120,120, 40, 40, 20, 20,10,10, 30, 30,220, 10, 10, 10,140, 80,320,12},
{14,  0,  0,  0,  0, 20, 20, 0, 0, 30, 30,220, 10, 10, 10,140, 80,320,10},
{14,  0,  0,  0,  0, 20, 20, 0, 0, 30, 30,  0, 10, 10, 10,140, 80,320,12},

{19,140,140, 40, 40, 30, 30,20,20, 30, 30,260, 10, 20, 10,190,110,320,16},
{19,  0,  0,  0,  0, 30, 30, 0, 0, 30, 30,260, 10, 20, 10,190,110,320,10},
{19,  0,  0,  0,  0, 30, 30, 0, 0, 30, 30,  0, 10, 20, 10,190,110,320,16},

};

/*
 * Resources:
 */

static XtResource	resources[] = {
#define offset(F) XtOffsetOf(WMRec,wm.F)

    {	/* SGI */
	XtNtitle, XtCTitle,
	XtRString, sizeof(String), offset(title),
	XtRString, (XtPointer)"Color Sample"
    },
    {	/* SGI */
	XtNfont, XtCFont,
	XtRFontStruct, sizeof(XFontStruct *), offset(font),
	XtRString, (XtPointer)NlucidaBold
    },
    {	/* SGI */
	XtNforeground, XtCForeground,
	XtRPixel, sizeof(Pixel), offset(foreground),
	XtRString, XtDefaultForeground
    },
    {	/* SGI */
	"inputWindowHeader", "InputWindowHeader",
	XtRPixel, sizeof(Pixel), offset(input_window_header),
	XtRString, (XtPointer)XtDefaultBackground
    },
    {	/* SGI */
	"pointerFocus", "PointerFocus",
	XtRBoolean, sizeof(Boolean), offset(pointer_focus),
	XtRString, (XtPointer)"false"
    },
    {	/* SI */
	XtNselect, XtCCallback,
	XtRCallback, sizeof(XtPointer), offset(select),
	XtRCallback, (XtPointer)0
    },

#undef	offset
};

/*
 * Function declarations:
 */

static Boolean		Layout OL_ARGS((
	WMWidget		w,
	Boolean			resizable
));
static void		CompileMetrics OL_ARGS((
	WMWidget		w,
	WMmetrics *		m
));
static void		GetGCs OL_ARGS((
	WMWidget		w
));
static void		FreeGCs OL_ARGS((
	WMWidget		w
));
static void		DisplayWM OL_ARGS((
	WMWidget		w
));
static void		CalcRects OL_ARGS((
	WMWidget		w,
	WMmetrics *		m,
	int			compass_point,
	XRectangle *		NWrect,
	int *			num_NWrect,
	XRectangle *		SErect,
	int *			num_SErect
));
static void		CallPartSelectCallback OL_ARGS((
	WMWidget		w,
	XEvent *		pe
));
static void		_Initialize OL_ARGS((
	WMWidget		request,
	WMWidget		new,
	ArgList			args,
	Cardinal *		num_args
));
static void		_Destroy OL_ARGS((
	WMWidget		w
));
static void 		_Resize OL_ARGS((
	WMWidget		w
));
static void		_Expose OL_ARGS((
	WMWidget		w,
	XEvent *		event,	/* not used */
	Region			region	/* not used */
));
static Boolean		_SetValues OL_ARGS((
	WMWidget		current,
	WMWidget		request,
	WMWidget		new,
	ArgList			args,
	Cardinal *		num_args
));
static XtGeometryResult	_QueryGeometry OL_ARGS((
	WMWidget		widget,
	XtWidgetGeometry *	constraint,
	XtWidgetGeometry *	preferred
));
static XtGeometryResult	_GeometryManager OL_ARGS((
	 Widget			w,
	 XtWidgetGeometry *	request,
	 XtWidgetGeometry *	reply
));
static void		_ChangeManaged OL_ARGS((
	WMWidget		w
));
static void		TouchMeHandler OL_ARGS((
	WMWidget		w,
	OlVirtualEvent		ve
));

/*
 * Translations and Actions:
 */

static char		translations[] = "\
	<FocusIn>:	OlAction()	\n\
	<FocusOut>:	OlAction()	\n\
	<KeyDown>:	OlAction()	\n\
	<KeyUp>:	OlAction()	\n\
	<BtnDown>:	OlAction()	\n\
	<BtnUp>:	OlAction()	\n\
	<Enter>:	OlAction()	\n\
	<Leave>:	OlAction()	\n\
	<BtnMotion>:	OlAction()	\n\
";

static OlEventHandlerRec event_procs[] = {
	{ EnterNotify,   (OlEventHandlerProc)TouchMeHandler },
	{ LeaveNotify,   (OlEventHandlerProc)TouchMeHandler },
	{ MotionNotify,  (OlEventHandlerProc)TouchMeHandler },
	{ ButtonPress,   (OlEventHandlerProc)TouchMeHandler },
	{ ButtonRelease, (OlEventHandlerProc)TouchMeHandler },
};

/*
 * Class definition:
 */

WMClassRec		wmClassRec = {
    /*
     * Core class:
     */
    {
    /* superclass          */	(WidgetClass)&managerClassRec,
    /* class_name          */	"WM",
    /* widget_size         */	sizeof(WMRec),
    /* class_initialize    */	NULL,
    /* class_part_init     */	NULL,
    /* class_inited        */	FALSE,
    /* initialize          */	_Initialize,
    /* initialize_hook     */	NULL,
    /* realize             */	XtInheritRealize,
    /* actions             */	NULL,
    /* num_actions         */	0,
    /* resources           */	resources,
    /* num_resources       */	XtNumber(resources),
    /* xrm_class           */	NULLQUARK,
    /* compress_motion     */	TRUE,
    /* compress_exposure   */	TRUE,
    /* compress_enterleave */	TRUE,
    /* visible_interest    */	FALSE,
    /* destroy             */	_Destroy,
    /* resize              */	_Resize,
    /* expose              */	_Expose,
    /* set_values          */	_SetValues,
    /* set_values_hook     */	NULL,
    /* set_values_almost   */	XtInheritSetValuesAlmost,
    /* get_values_hook     */	NULL,
    /* accept_focus        */	XtInheritAcceptFocus,
    /* version             */	XtVersion,
    /* callback_private    */	NULL,
    /* tm_table            */	translations /*XtInheritTranslations*/,
    /* query_geometry      */	_QueryGeometry,
    },

    /*
     * Composite class:
     */
    {
    /* geometry_manager    */	_GeometryManager,
    /* change_managed      */	_ChangeManaged,
    /* insert_child        */	XtInheritInsertChild,
    /* delete_child        */	XtInheritDeleteChild,
    /* extension           */	NULL
    },

    /*
     * Constraint class:
     */
    {
    /* resources           */	NULL,
    /* num_resources       */	(Cardinal)0,
    /* constraint_size     */	(Cardinal)0,
    /* initialize          */	NULL,
    /* destroy             */	NULL,
    /* set_values          */	NULL,
    /* extension           */	NULL
    },

    /*
     * Manager class:
     */
    {
    /* focus_on_select	   */	True,
    /* highlight_handler   */	NULL,
    /* traversal_handler   */	NULL,
    /* activate            */	NULL,
    /* event_procs         */	event_procs,
    /* num_event_procs     */	XtNumber(event_procs),
    /* register_focus      */	NULL,
    /* version             */	OlVersion,
    /* extension           */	NULL
    },

    /*
     * WM class:
     */
    {
    /* empty               */	0,
    }
};

WidgetClass		wmWidgetClass = (WidgetClass)&wmClassRec;

/**
 ** _OlWMFrameDimensions()
 **/

void
#if	OlNeedFunctionPrototypes
_OlWMFrameDimensions (
	Widget			w,
	Dimension *		p_header,
	Dimension *		p_side,
	Dimension *		p_bottom
)
#else
_OlWMFrameDimensions (w, p_header, p_side, p_bottom)
	Widget			w;
	Dimension *		p_header;
	Dimension *		p_side;
	Dimension *		p_bottom;
#endif
{
	WMmetrics *		m = XtNew(WMmetrics);


	*m = wmmetrics[_12PT_BASE_WINDOW];
	CompileMetrics ((WMWidget)w, m);

	if (p_header)
		*p_header = BorderY(w, m)	/* top border	     */
			  + BannerHt(w, m)	/* header	     */
			  + LineHt(w, m);	/* raised line	     */
	if (p_bottom)
		*p_bottom = BorderY(w, m);	/* bottom border     */

	if (p_side)
		*p_side   = BorderX(w, m);	/* left/right border */

	XtFree (m);
	return;
} /* _OlWMFrameDimensions */

/**
 ** Layout()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
Layout (
	WMWidget		w,
	Boolean			resizable
)
#else
Layout (w, resizable)
	WMWidget		w;
	Boolean			resizable;
#endif
{
	Widget			child;

	Dimension		width;
	Dimension		height;

	WMmetrics *		m;

	Boolean			ret	= True;


	if (w->composite.num_children < 1)
		return (False);
	child = w->composite.children[0];

	m = w->wm.metrics;

	width	= BorderX(w, m)			/* left border	*/
		+ child->core.width
		+ child->core.border_width * 2
		+ BorderX(w, m);		/* right border	*/

	height	= BorderY(w, m)			/* top border	*/
		+ BannerHt(w, m)		/* header	*/
		+ LineHt(w, m)			/* raised line	*/
		+ child->core.height
		+ child->core.border_width * 2
		+ BannerHt(w, m)		/* ``footer''	*/
		+ BorderY(w, m);		/* bottom border*/

	/*
	 * MORE: Handle query from below.
	 * MORE: Handle query from above.
	 */
	if (resizable) {
#define RR(W,WIDTH,HEIGHT) \
	XtMakeResizeRequest((Widget)(W), WIDTH, HEIGHT, &WIDTH, &HEIGHT)

		switch (RR(w, width, height)) {
		case XtGeometryYes:
			break;

		case XtGeometryAlmost:
			RR (w, width, height);
			ret = False;
			break;

		case XtGeometryNo:
			ret = False;
			break;
		}

#undef	RR
	}

	XtMoveWidget (
		child,
		BorderX(w, m),
		BorderY(w, m) + BannerHt(w, m) + LineHt(w, m)
	);

	return (ret);
} /* Layout */

/**
 ** CompileMetrics()
 **/

static void
#if	OlNeedFunctionPrototypes
CompileMetrics (
	WMWidget		w,
	WMmetrics *		m
)
#else
CompileMetrics (w, m)
	WMWidget		w;
	WMmetrics *		m;
#endif
{
	Screen *		screen	 = XtScreenOfObject(w);
	

	ConvertX (screen, m->cornerX);
	ConvertX (screen, m->cornerx);
	ConvertX (screen, m->linewidX);
	ConvertX (screen, m->linewidx);
	ConvertX (screen, m->gapx);
	ConvertX (screen, m->offset);
	ConvertX (screen, m->markwid);
	ConvertY (screen, m->cornerY);
	ConvertY (screen, m->cornery);
	ConvertY (screen, m->linewidY);
	ConvertY (screen, m->linewidy);
	ConvertY (screen, m->gapy);
	ConvertY (screen, m->bannerht);
	ConvertX (screen, m->hgap1);
	ConvertX (screen, m->hgap2);
	ConvertX (screen, m->linewid);
	ConvertX (screen, m->baseline);
	ConvertX (screen, m->selectwidth);

	return;
} /* CompileMetrics */

/**
 ** GetGCs()
 **/

static void
#if	OlNeedFunctionPrototypes
GetGCs (
	WMWidget		w
)
#else
GetGCs (w)
	WMWidget		w;
#endif
{
	Screen *		screen	= XtScreenOfObject((Widget)w);


	w->wm.attrs = OlgCreateAttrs(
		screen,
		w->wm.foreground,
		&(w->core.background_pixel),
		FALSE,
		12
	);
	OlgSizeAbbrevMenuB (
		screen,
		w->wm.attrs,
		&(w->wm.mmWidth),
		&(w->wm.mmHeight)
	);

	return;
} /* GetGCs */

/**
 ** FreeGCs
 **/

static void
#if	OlNeedFunctionPrototypes
FreeGCs (
	WMWidget		w
)
#else
FreeGCs (w)
	WMWidget		w;
#endif
{
	OlgDestroyAttrs (w->wm.attrs);
	w->wm.attrs = 0;
	return;
} /* FreeGCs */

/**
 ** DisplayWM()
 **/

static void
#if	OlNeedFunctionPrototypes
DisplayWM (
	WMWidget		w
)
#else
DisplayWM (w)
	WMWidget		w;
#endif
{
	Display *		display	= XtDisplayOfObject(w);

	Window			window	= XtWindowOfObject(w);

	WMmetrics *		m	= w->wm.metrics;

	XRectangle		nw_corners[32];
	XRectangle		se_corners[32];

	int			nw_i	= 0;
	int			se_i	= 0;

	GC			nw_gc;
	GC			se_gc;
	GC			bg1_gc;

	char *			title;

	int			titlelen;
	int			titlewid;
	int			titlearea;
	int			centered;

	OlgTextLbl		lbl;

	XRectangle		rect;


	if (!XtIsRealized(w))
		return;

/*	title = OlGetMessage(display, msg, BUFSIZ,
			 OleNfixedString,
			 OleTcolorSample,
			 OleCOlClientOlwsmMsgs,
			 OleMfixedString_colorSample,
			 (XrmDatabase)NULL);*/

/*
	title     = w->wm.title;
*/
	title	  = OLG(colorSample,fixedString);
	titlelen  = strlen(title);
	titlewid  = XTextWidth(w->wm.font, title, titlelen);
	titlearea = BannerWid(w,m) - (Offset(w,m) + MarkWid(w,m));
	centered  = (titlewid < (titlearea - (Offset(w,m) + MarkWid(w,m))));

	while (titlewid > titlearea && titlelen > 0) {
		titlelen--;
		titlewid = XTextWidth(w->wm.font, title, titlelen);
	}

	rect.x      = BorderX(w,m);
	rect.y      = BorderY(w,m);
	rect.width  = BannerWid(w,m);
	rect.height = BannerHt(w,m);

	bg1_gc = OlgGetScratchGC(w->wm.attrs);
	XCopyGC (
		display,
		OlgGetBg1GC(w->wm.attrs),
		GCForeground|GCBackground|GCSubwindowMode,
		bg1_gc
	);
	XSetFont (display, bg1_gc, w->wm.font->fid);
	XSetFillStyle (display, bg1_gc, FillSolid);

	if (OlgIs3d()) {
		nw_gc = OlgGetBrightGC(w->wm.attrs);
		se_gc = OlgGetBg3GC(w->wm.attrs);
	} else {
		nw_gc = OlgGetDarkGC(w->wm.attrs);
		se_gc = OlgGetDarkGC(w->wm.attrs);
	}

	CalcRects (w, m, NW, nw_corners, &nw_i, se_corners, &se_i);
	CalcRects (w, m, NE, nw_corners, &nw_i, se_corners, &se_i);
	CalcRects (w, m, SE, nw_corners, &nw_i, se_corners, &se_i);
	CalcRects (w, m, SW, nw_corners, &nw_i, se_corners, &se_i);
	CalcRects (w, m, BD, nw_corners, &nw_i, se_corners, &se_i);
	CalcRects (w, m, LN, nw_corners, &nw_i, se_corners, &se_i);

	if (w->wm.pointer_focus)
		CalcRects (w, m, BN, se_corners, &se_i, nw_corners, &nw_i);
	else {
		XSetForeground (display, bg1_gc, w->wm.input_window_header);
		XFillRectangles (display, window, bg1_gc, &rect, 1);

		/*
		 * The user may accidently pick a window background that
		 * results in a foreground == window header. If so, switch
		 * to background for drawing the window title.
		 */
		if (w->wm.foreground == w->wm.input_window_header)
			XSetForeground (display, bg1_gc, w->core.background_pixel);
		else
			XSetForeground (display, bg1_gc, w->wm.foreground);
	}

	lbl.normalGC      = lbl.inverseGC = bg1_gc;
	lbl.font          = w->wm.font;
	lbl.font_list     = NULL;
	lbl.accelerator   = 0;
	lbl.mnemonic      = 0;
	lbl.flags         = 0;
/*
	lbl.label         = w->wm.title;
*/
	lbl.label         = title;
	lbl.justification = TL_CENTER_JUSTIFY;

	OlgDrawTextLabel (
		XtScreenOfObject(w),
		window,
		w->wm.attrs,
		(centered? rect.x : Offset(w,m) + MarkWid(w,m)),
		rect.y,
		(centered? rect.width : rect.width - Offset(w,m) - MarkWid(w,m)),
		rect.height,
		&lbl
	);

	OlgDrawAbbrevMenuB (
		XtScreenOfObject(w),
		window,
		w->wm.attrs,
		Offset(w,m),
		BorderY(w,m) + (BannerHt(w,m) - (int)w->wm.mmHeight) / 2,
		AM_NORMAL
	);

	XFillRectangles (display, window, nw_gc, nw_corners, nw_i);
	XFillRectangles (display, window, se_gc, se_corners, se_i);

	return;
} /* DisplayWM */

/**
 ** CalcRects()
 **/

static void
#if	OlNeedFunctionPrototypes
CalcRects (
	WMWidget		w,
	WMmetrics *		m,
	int			compass_point,
	XRectangle *		NWrect,
	int *			num_NWrect,
	XRectangle *		SErect,
	int *			num_SErect
)
#else
CalcRects (w, m, compass_point, NWrect, num_NWrect, SErect, num_SErect)
	WMWidget		w;
	WMmetrics *		m;
	int			compass_point;
	XRectangle *		NWrect;
	int *			num_NWrect;
	XRectangle *		SErect;
	int *			num_SErect;
#endif
{
	int			rx;
	int			ry;
	int			XCorner  = CornerX(w,m);
	int			YCorner  = CornerY(w,m);
	int			xCorner  = Cornerx(w,m);
	int			yCorner  = Cornery(w,m);
	int			XLineWid = LineWidX(w,m);
	int			YLineWid = LineWidY(w,m);
	int			xLineWid = LineWidx(w,m);
	int			yLineWid = LineWidy(w,m);
	int			num_NW   = *num_NWrect;
	int			num_SE   = *num_SErect;
	int			width    = w->core.width;
	int			height   = w->core.height;
	int			addwidth = 0;

	switch (compass_point) {

	case BN:
		NWRECT (
			XCorner + xLineWid,
			BorderY(w,m) - YLineWid,
			width - 2 * (XCorner + xLineWid),
			YLineWid
		);
		NWRECT (
			BorderX(w,m),
			BorderY(w,m) + BannerHt(w,m) - YLineWid,
			BannerWid(w,m),
			YLineWid
		);
		break;
	case BD:
		if (!OlgIs3d())
			addwidth = m->selectwidth;
		NWRECT (
			XCorner,
			0,
			width - 2 * XCorner,
			YLineWid + addwidth
		);
		NWRECT (
			0,
			YCorner,
			XLineWid + addwidth,
			height - 2 * YCorner
		);
		SERECT (
			XCorner,
			height - (YLineWid + addwidth),
			width - 2 * XCorner,
			YLineWid + addwidth
		);
		SERECT (
			width - (XLineWid + addwidth),
			YCorner,
			XLineWid + addwidth,
			height - 2 * YCorner
		);
		break;
	case LN:
		NWRECT (
			BorderX(w,m),
			BorderY(w,m) + BannerHt(w,m) + m->hgap1,
			BannerWid(w,m),
			m->linewid
		);
		SERECT (
			BorderX(w,m),
			BorderY(w,m) + BannerHt(w,m) + m->hgap1 + m->linewid, 
			BannerWid(w,m),
			m->linewid
		);
		break;
	case NW:
		rx = 0;
		ry = 0;
		NWRECT (rx, ry, XCorner, yLineWid);
		NWRECT (rx, ry, xLineWid, YCorner);
		SERECT (
			rx + xLineWid,
			ry + YCorner - yLineWid,
			xCorner,
			yLineWid
		);
		SERECT (
			rx + xCorner,
			ry + yCorner,
			xLineWid,
			YCorner - yCorner
		);
		SERECT (
			rx + xCorner,
			ry + yCorner,
			XCorner - xCorner,
			yLineWid
		);
		SERECT (
			rx + XCorner - xLineWid,
			ry + yLineWid,
			xLineWid,
			yCorner
		);
		break;
	case NE:
		rx = width - XCorner;
		ry = 0;
		NWRECT (rx, ry, XCorner, yLineWid);
		NWRECT (rx, ry + yLineWid, xLineWid, yCorner);
		SERECT (
			rx + xLineWid,
			ry + yCorner,
			XCorner - xCorner - xLineWid,
			yLineWid
		);
		NWRECT (
			rx + XCorner - xCorner - xLineWid,
			ry + yCorner,
			xLineWid,
			YCorner - yCorner
		);
		SERECT (
			rx + XCorner - xCorner - xLineWid,
			ry + YCorner - yLineWid,
			xCorner,
			yLineWid
		);
		SERECT (rx + XCorner - xLineWid, ry, xLineWid, XCorner);
		break;
	case SW:
		rx = 0;
		ry = height;
		NWRECT (rx + xLineWid, ry - YCorner, xCorner, yLineWid);
		NWRECT (rx, ry - YCorner, xLineWid, YCorner);
		SERECT (rx, ry - yLineWid, XCorner, yLineWid);
		SERECT (
			rx + XCorner - xLineWid,
			ry - yLineWid - yCorner,
			xLineWid,
			yCorner
		);
		NWRECT (
			rx + xCorner,
			ry - yCorner - yLineWid,
			XCorner - xCorner - xLineWid,
			yLineWid
		);
		SERECT (
			rx + xCorner,
			ry - YCorner + yLineWid,
			xLineWid,
			YCorner - yCorner - yLineWid
		);
		break;
	case SE:
		rx = width;
		ry = height;
		NWRECT (
			rx - xLineWid - xCorner,
			ry - YCorner,
			xCorner,
			yLineWid
		);
		NWRECT (
			rx - xLineWid - xCorner,
			ry - YCorner + yLineWid,
			xLineWid,
			YCorner - yCorner - yLineWid
		);
		NWRECT (
			rx - XCorner + xLineWid,
			ry - yLineWid - yCorner,
			XCorner - xCorner - xLineWid,
			yLineWid
		);
		NWRECT (
			rx - XCorner,
			ry - yLineWid - yCorner,
			xLineWid,
			yCorner
		);
		SERECT (
			rx - XCorner,
			ry - yLineWid,
			XCorner,
			yLineWid
		);
		SERECT (
			rx - xLineWid,
			ry - YCorner,
			xLineWid,
			YCorner
		);
		break;
	}

	*num_SErect = num_SE;
	*num_NWrect = num_NW;

	return;
} /* CalcRects */

/**
 ** CallPartSelectCallback()
 **/

static void
#if	OlNeedFunctionPrototypes
CallPartSelectCallback (
	WMWidget		w,
	XEvent *		pe
)
#else
CallPartSelectCallback (w, pe)
	WMWidget		w;
	XEvent *		pe;
#endif
{
	/*
	 * This type punning works because the events we handle
	 * here have the same structure up to the x,y values.
	 */
	XButtonEvent *		pbe	= &(pe->xbutton);

	Position		x	= (Position)pbe->x;
	Position		y	= (Position)pbe->y;

	Dimension		width	= w->core.width;

	WMmetrics *		m	= w->wm.metrics;

	OlDefine		part;


	if (XtHasCallbacks(w, XtNselect) != XtCallbackHasSome)
		return;

	if (
		Cornerx(w,m) < x && x < (int)width - Cornerx(w,m)
	     && Cornery(w,m) < y && y < Cornery(w,m) + BannerHt(w,m)
	)
		part = WM_PART_HEADER;
	else
		part = WM_PART_BACKGROUND;

	XtCallCallbacks (w, XtNselect, &part);

	return;
} /* CallPartSelectCallback */

/**
 ** _Initialize()
 **/

static void
#if	OlNeedFunctionPrototypes
_Initialize (
	WMWidget		request,
	WMWidget		new,
	ArgList			args,
	Cardinal *		num_args
)
#else
_Initialize (request, new, args, num_args)
	WMWidget		request;
	WMWidget		new;
	ArgList			args;
	Cardinal *		num_args;
#endif
{
	/*
	 * For now....
	 */
	if (!new->core.width)
		new->core.width = 1;
	if (!new->core.height)
		new->core.height = 1;

	if (!new->wm.title)
		new->wm.title = XtName(new);
	new->wm.title = XtNewString(new->wm.title);

	new->wm.metrics    = XtNew(WMmetrics);
	*(new->wm.metrics) = wmmetrics[_12PT_BASE_WINDOW];
	CompileMetrics (new, new->wm.metrics);

	GetGCs (new);

	return;
} /* _Initialize */

/**
 ** _Destroy()
 **/

static void
#if	OlNeedFunctionPrototypes
_Destroy (
	WMWidget		w
)
#else
_Destroy (w)
	WMWidget		w;
#endif
{
	XtFree (w->wm.title);
	XtFree (w->wm.metrics);

	FreeGCs (w);

	return;
} /* _Destroy */

/**
 ** _Resize()
 **/

static void 
#if	OlNeedFunctionPrototypes
_Resize (
	WMWidget		w
)
#else
_Resize (w)
	WMWidget		w;
#endif
{
	Layout (w, False);
	return;
} /* _Resize */

/**
 ** _Expose()
 **/

static void
#if	OlNeedFunctionPrototypes
_Expose (
	WMWidget		w,
	XEvent *		event,	/* not used */
	Region			region	/* not used */
)
#else
_Expose (w, event, region)
	WMWidget		w;
	XEvent *		event;
	Region			region;
#endif
{
	DisplayWM (w);
	return;
} /* _Expose */

/**
 ** _SetValues()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
_SetValues (
	WMWidget		current,
	WMWidget		request,
	WMWidget		new,
	ArgList			args,
	Cardinal *		num_args
)
#else
_SetValues (current, request, new, args, num_args)
	WMWidget		current;
	WMWidget		request;
	WMWidget		new;
	ArgList			args;
	Cardinal *		num_args;
#endif
{
	Boolean			redisplay	= False;


#define DIFFERENT(FIELD) (new->FIELD != current->FIELD)

	/*
	 * MORE: Handle client directed resize.
	 */
	new->core.width  = current->core.width;
	new->core.height = current->core.height;

	if (
		DIFFERENT(wm.title)
	     || DIFFERENT(wm.pointer_focus)
	)
		redisplay = True;

	if (
		DIFFERENT(wm.font)
	     || DIFFERENT(wm.font->fid)
	     || DIFFERENT(wm.foreground)
	     || DIFFERENT(wm.input_window_header)
	     || DIFFERENT(core.background_pixel)
	) {
		FreeGCs (new);
		GetGCs (new);
		redisplay = True;
	}

	return (redisplay);

#undef	DIFFERENT
} /* _SetValues */

/**
 ** _QueryGeometry()
 **/

static XtGeometryResult
#if	OlNeedFunctionPrototypes
_QueryGeometry (
	WMWidget		widget,
	XtWidgetGeometry *	constraint,
	XtWidgetGeometry *	preferred
)
#else
_QueryGeometry (widget, constraint, preferred)
	WMWidget		widget;
	XtWidgetGeometry *	constraint;
	XtWidgetGeometry *	preferred;
#endif
{
	/*
	 * MORE: SHOULD QUERY OUR CHILDREN ABOUT THIS.
	 */
	return (XtGeometryNo);
} /* _QueryGeometry */

/**
 ** _GeometryManager()
 **/

static XtGeometryResult
#if	OlNeedFunctionPrototypes
_GeometryManager (
	 Widget			w,
	 XtWidgetGeometry *	request,
	 XtWidgetGeometry *	reply
)
#else
_GeometryManager (w, request, reply)
	 Widget			w;
	 XtWidgetGeometry *	request;
	 XtWidgetGeometry *	reply;
#endif
{
	struct save {
		Position		x;
		Position		y;
		Dimension		width;
		Dimension		height;
		Dimension		border_width;
	}			save;


	if (
		(request->request_mode & CWX)
	     || (request->request_mode & CWY)
	)
		return (XtGeometryNo);

	/*
	 * For our convenience, make all fields in the request valid.
	 */
	if (!(request->request_mode & CWWidth))
		request->width = w->core.width;
	if (!(request->request_mode & CWHeight))
		request->height = w->core.height;
	if (!(request->request_mode & CWBorderWidth))
		request->border_width = w->core.border_width;

#define SAVE(A,B) \
	(A).width        = (B).width;					\
	(A).height       = (B).height;				\
	(A).border_width = (B).border_width

	SAVE (save, w->core);
	SAVE (w->core, (*request));

	if (Layout((WMWidget)w->core.parent, True))
		return (XtGeometryYes);
	else {
		SAVE (w->core, save);
		return (XtGeometryNo);
	}
} /* _GeometryManager */

/**
 ** _ChangeManaged()
 **/

static void
#if	OlNeedFunctionPrototypes
_ChangeManaged (
	WMWidget		w
)
#else
_ChangeManaged (w)
	WMWidget		w;
#endif
{
	Layout (w, True);
} /* _ChangeManaged */

/**
 ** TouchMeHandler()
 **/

static void
#if	OlNeedFunctionPrototypes
TouchMeHandler (
	WMWidget		w,
	OlVirtualEvent		ve
)
#else
TouchMeHandler (w, ve)
	WMWidget		w;
	OlVirtualEvent		ve;
#endif
{
	switch (ve->virtual_name) {
	case OL_SELECT:
		ve->consumed = True;
		switch (ve->xevent->type) {
		case ButtonPress:
		case EnterNotify:
			CallPartSelectCallback (w, ve->xevent);
		}
		break;
	}
	return;
} /* ButtonHandler */
