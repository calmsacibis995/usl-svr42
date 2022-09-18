/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtcalc:Icons.c	1.2"
#endif

#include "X11/IntrinsicP.h"
#include "X11/StringDefs.h"
#include "X11/Shell.h"

#include "Xol/OpenLookP.h"
#include "Xol/StubP.h"		/* for access to CORE_P macro */

/*
 * Public routines:
 */

void		OlBestWMIconSize OL_ARGS((
	Widget			w,
	unsigned int		width,
	unsigned int		height,
	unsigned int *		p_width,
	unsigned int *		p_height
));

/*
 * Local types:
 */

typedef struct PixMix {
	Widget			w;
	XRectangle		geometry;
	Pixmap			pixmap;
	GC			gc;
}			PixMix;

/*
 * Local routines:
 */

static void		ExposeProc OL_ARGS((
	Widget			w,
	XEvent *		xevent,
	Region			region
));
static void		DestroyCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
static void		DynamicCB OL_ARGS((
	XtPointer		client_data
));
static int		BestIconDimension OL_ARGS((
	XIconSize *		size_list,
	int			count,
	unsigned int		size,
	Cardinal		min_offset,
	Cardinal		max_offset,
	Cardinal		inc_offset
));
static void		GetValue OL_ARGS((
	Widget			w,
	String			name,
	XtArgVal		value
));

/**
 ** OlCreateIconWidget()
 **/

Widget
#if	OlNeedFunctionPrototypes
OlCreateIconWidget (
	Widget			shell,
	Pixmap			pixmap
)
#else
OlCreateIconWidget (shell, pixmap)
	Widget			shell;
	Pixmap			pixmap;
#endif
{
	int			pixmap_x;		/*SET,NOTUSED*/
	int			pixmap_y;		/*SET,NOTUSED*/
	unsigned int		pixmap_width;
	unsigned int		pixmap_height;
	unsigned int		pixmap_border_width;	/*SET,NOTUSED*/
	unsigned int		pixmap_depth;
	unsigned int		width;
	unsigned int		height;
	unsigned int		depth;

	Display *		display	= XtDisplayOfObject(shell);

	Screen *		screen	= XtScreenOfObject(shell);

	Window			window;
	Window			root;			/*SET,NOTUSED*/

	Widget			troll;

	Pixmap			mask;

	Pixel			background;

	PixMix *		pixmix;

	XGCValues		v;

	static XtTranslations	null_translations = 0;


	/*
	 * We need to create a Stub widget, but we don't want to
	 * confuse any composite by creating the widget as its child.
	 * We'll use the Troll widget, if we can find it.
	 */
	troll = XtWindowToWidget(display, RootWindowOfScreen(screen));
	if (!troll)
		return (0); /* uh-oh */

	XGetGeometry (
		display, pixmap,
		&root,
		&pixmap_x, &pixmap_y, &pixmap_width, &pixmap_height,
		&pixmap_border_width,
		&pixmap_depth
	);

	/*
	 * If the pixmap is a bitmap, Shell has already handled the icon.
	 */
	if (pixmap_depth == 1)
		return (None);

	GetValue (shell, XtNdepth, (XtArgVal)&depth);
	if (pixmap_depth != depth)
		return (None);

	/*
	 * Find a icon_window size that is allowed by the window manager
	 * and fits the pixmap.
	 */
	OlBestWMIconSize (
		shell, pixmap_width, pixmap_height, &width, &height
	);
	if (!width || !height)
		return (None);
	GetValue (shell, XtNiconMask, (XtArgVal)&mask);

	pixmix = XtNew(PixMix);
	pixmix->pixmap = pixmap;
	pixmix->geometry.x = (width - pixmap_width) / 2;
	pixmix->geometry.y = (height - pixmap_height) / 2;
	pixmix->geometry.width = pixmap_width;
	pixmix->geometry.height = pixmap_height;

	v.graphics_exposures = False;
	v.clip_mask = mask;
	v.clip_x_origin = pixmix->geometry.x;
	v.clip_y_origin = pixmix->geometry.y;
	pixmix->gc = XtGetGC(
	      shell,
	      GCGraphicsExposures|GCClipMask|GCClipXOrigin|GCClipYOrigin,
	      &v
	);

	window = XCreateSimpleWindow(
		display, RootWindowOfScreen(screen),
		0, 0, width, height, 0, (Pixel)0, (Pixel)0
	);
	if (!null_translations)
		null_translations = XtParseTranslationTable("");
	pixmix->w = XtVaCreateWidget(
		"iconWindow", stubWidgetClass, troll,
		XtNwindow,       (XtArgVal)window,
		XtNexpose,       (XtArgVal)ExposeProc,
		XtNuserData,     (XtArgVal)pixmix,
		XtNtranslations, (XtArgVal)null_translations,
		(String)0
	);

	/*
	 * Setting the XtNwindow resource isn't enough to realize the
	 * stub, it still needs to be "realized". Even so, realizing it
	 * doesn't set the background pixel to match the widget's value,
	 * so we have to do this "manually".
	 */
	XtRealizeWidget (pixmix->w);
	GetValue (pixmix->w, XtNbackground, (XtArgVal)&background);
	XSetWindowBackground (display, window, background);

	XtVaSetValues (shell, XtNiconWindow, (XtArgVal)window, (String)0);
	XtAddCallback (shell, XtNdestroyCallback, DestroyCB, (XtPointer)pixmix);
	OlRegisterDynamicCallback (DynamicCB, (XtPointer)pixmix);

	return (pixmix->w);
} /* OlCreateIconWidget */

/**
 ** OlBestWMIconSize()
 **/

void
#if	OlNeedFunctionPrototypes
OlBestWMIconSize (
	Widget			w,
	unsigned int		width,
	unsigned int		height,
	unsigned int *		p_width,
	unsigned int *		p_height
)
#else
OlBestWMIconSize (w, width, height, p_width, p_height)
	Widget			w;
	unsigned int		width;
	unsigned int		height;
	unsigned int *		p_width;
	unsigned int *		p_height;
#endif
{
	int			count;

	XIconSize *		size_list;


	XGetIconSizes (
		XtDisplayOfObject(w),
		RootWindowOfScreen(XtScreenOfObject(w)),
		&size_list, &count
	);
	if (size_list) {
		*p_width = BestIconDimension(
			size_list, count,
			width,
			XtOffset(XIconSize *, min_width),
			XtOffset(XIconSize *, max_width),
			XtOffset(XIconSize *, width_inc)
		);
		*p_height = BestIconDimension(
			size_list, count,
			(int)height,
			XtOffset(XIconSize *, min_height),
			XtOffset(XIconSize *, max_height),
			XtOffset(XIconSize *, height_inc)
		);
		XFree ((XtPointer)size_list);
	}
	return;
} /* OlBestWMIconSize */

/**
 ** ExposeProc
 **/

static void
#if	OlNeedFunctionPrototypes
ExposeProc (
	Widget			w,
	XEvent *		xevent,
	Region			region
)
#else
ExposeProc (w, xevent, region)
	Widget			w;
	XEvent *		xevent;
	Region			region;
#endif
{
	XtPointer		user_data;

	PixMix *		pixmix;

	XRectangle		clip;


	GetValue (w, XtNuserData, (XtArgVal)&user_data);
	pixmix = (PixMix *)user_data;

	/*
	 * If the rectangle is in the exposed region, draw the pixmap.
	 * Note: The clip rectangle returned has coordinates relative to
	 * the widget; subtracting the pixmap's corner coordinates
	 * makes them relative to the pixmap.
	 */
	if (OlRectInRegion(region, &pixmix->geometry, &clip))
		XCopyArea (
			XtDisplay(w), pixmix->pixmap, XtWindow(w),
			pixmix->gc,
			clip.x - pixmix->geometry.x,
			clip.y - pixmix->geometry.y,
			clip.width, clip.height,
			clip.x, clip.y
		);

	return;
} /* ExposeProc */

/**
 ** DestroyCB()
 **/

static void
#if	OlNeedFunctionPrototypes
DestroyCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
DestroyCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	PixMix *		pixmix = (PixMix *)client_data;

	XtReleaseGC (pixmix->w, pixmix->gc);
	XtDestroyWidget (pixmix->w);
	OlUnregisterDynamicCallback (DynamicCB, pixmix);
	XtFree ((char *)pixmix);
	return;
} /* DestroyCB */

/**
 ** DynamicCB()
 **/

static void
#if	OlNeedFunctionPrototypes
DynamicCB (
	XtPointer		client_data
)
#else
DynamicCB (client_data)
	XtPointer		client_data;
#endif
{
	PixMix *		pixmix = (PixMix *)client_data;

	Display *		display = XtDisplay(pixmix->w);

	Window			window = XtWindow(pixmix->w);

	Pixel			background;

	Arg			arg;


	/*
	 * We need this dynamic callback because the icon widget is not
	 * a regular child and thus won't be traversed in the dynamic
	 * updating done by the toolkit. Drat.
	 */
	XtSetArg (arg, XtNbackground, &background);
	OlGetApplicationValues (pixmix->w, &arg, 1);
	XSetWindowBackground (display, window, background);
	XClearArea (
		display, window,
		CORE_P(pixmix->w).x, CORE_P(pixmix->w).y,
		CORE_P(pixmix->w).width, CORE_P(pixmix->w).height,
		True
	);

	return;
} /* DynamicCB */

/**
 ** BestIconDimension()
 **/

static int
#if	OlNeedFunctionPrototypes
BestIconDimension (
	XIconSize *		size_list,
	int			count,
	unsigned int		size,
	Cardinal		min_offset,
	Cardinal		max_offset,
	Cardinal		inc_offset
)
#else
BestIconDimension (size_list, count, size, min_offset, max_offset, inc_offset)
	XIconSize *		size_list;
	int			count;
	unsigned int		size;
	Cardinal		min_offset;
	Cardinal		max_offset;
	Cardinal		inc_offset;
#endif
{
	int			delta;
	int			best_delta;
	int			best_size;
	int			min;
	int			max;
	int			inc;
	int			i;

	Boolean			found = False;

#define ADDR(i)	(char *)&(size_list[i])


	/*
	 * Try for an exact fit:
	 */
	for (i = 0; i < count && !found; i++) {
		min = (int)*(ADDR(i) + min_offset);
		max = (int)*(ADDR(i) + max_offset);
		inc = (int)*(ADDR(i) + inc_offset);

		if (size < min || max < size)
			;
		else if (size == max || size == min)
			found = True;
		else if (inc && !((size - min) % inc))
			found = True;
	}

	if (!found) {
		/*
		 * Try for best fit in larger dimension:
		 */
		best_size = 0;
		best_delta = 32767;
		for (i = 0; i < count; i++) {
			min = (int)*(ADDR(i) + min_offset);
			max = (int)*(ADDR(i) + max_offset);
			inc = (int)*(ADDR(i) + inc_offset);

			if (max < size)
				continue;

#define ISBEST(DELTA,SIZE) \
			if (DELTA < best_delta) {			\
				best_size = SIZE;			\
				best_delta = DELTA;			\
			}

			delta = max - size;
			ISBEST (delta, max)
			if (size < min) {
				delta = min - size;
				ISBEST (delta, min)
			} else if (inc) {
				delta = inc - ((size - min) % inc);
				if (max < size - delta)
					continue;
				ISBEST (delta, size + delta)
			}
#undef	ISBEST
		}
		if (best_size) {
			size = best_size;
			found = True;
		}
	}

	if (!found) {
		/*
		 * Try for best fit in smaller dimension:
		 * (Note: We get here only if "size" exceeds "max" for all
		 * "size_list[]" entries.)
		 */
		best_size = 0;
		for (i = 0; i < count; i++) {
			max = (int)*(ADDR(i) + max_offset);
			if (max > best_size)
				best_size = max;
		}
		size = best_size;
	}

#undef	ADDR
	return (size);
} /* BestIconDimension */

/**
 ** GetValue()
 **/

static void
#if	OlNeedFunctionPrototypes
GetValue (
	Widget			w,
	String			name,
	XtArgVal		value
)
#else
GetValue (w, name, value)
	Widget			w;
	String			name;
	XtArgVal		value;
#endif
{
	Arg arg;
	XtSetArg (arg, name, value);
	XtGetValues (w, &arg, 1);
	return;
} /* GetValue */
