/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtm:olwsm/colormenu.c	1.34"
#endif

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/Caption.h>
#include <Xol/ControlAre.h>
#include <Xol/FColors.h>
#include <Xol/RubberTile.h>
#include <Xol/Stub.h>

#include <misc.h>
#include <list.h>
#include <colormenu.h>
#include <exclusive.h>
#include "error.h"
#include <property.h>
#include <resource.h>
#include <slider.h>
#include <wsm.h>
#include <OlMinStr.h>

/*
 * Convenient macros:
 */

#define BLACK_ON_WHITE		(_video + 0)
#define WHITE_ON_BLACK		(_video + 1)
#define IS_WHITE_ON_BLACK() \
    (MATCH(BlackColor, resource_value(&global_resources, "*Background")))

#define SetColorSlider(S,I) \
	SetSlider (							\
		&S,							\
(int)(S.slider_min + ((long)I * (S.slider_max - S.slider_min)) / 65535L),\
		OL_NONE							\
	)

/*
 * Special types:
 */

typedef void	(*color_cb_type) OL_ARGS(( Pixel ));

typedef struct ColorMenuItem {
	XtArgVal		foreground;
}			ColorMenuItem;

typedef struct VisualItem {
	Widget	(*poor_create) OL_ARGS(( Widget, color_cb_type ));
	void	(*poor_set) OL_ARGS(( Widget , Pixel , unsigned int ));
	String	poor_color_tuple_list;
	Widget	(*rich_create) OL_ARGS(( Widget, color_cb_type ));
	void	(*rich_set) OL_ARGS(( Widget , Pixel , unsigned int ));
	String	rich_color_tuple_list;
}			VisualItem;
	
/*
 * Local functions:
 */

static Widget		DiscreteBW_Create OL_ARGS((
	Widget			parent,
	void			(*bw_cb)( Pixel , Pixel )
));
static void		DiscreteBW_ExclusivesCB OL_ARGS((
	Exclusive *		exclusive
));
static void		DiscreteBW_Set OL_ARGS((
	Widget			w,
	Pixel			pixel
));
static Widget		DiscreteColor_Create OL_ARGS((
	Widget			parent,
	void			(*color_cb)( Pixel )
));
static void		DiscreteColor_DestroyCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
static void		DiscreteColor_SelectCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
static void		DiscreteColor_Set OL_ARGS((
	Widget			w,
	Pixel			pixel,
	unsigned int		flag
));
static Widget		AnalogColor_Create OL_ARGS((
	Widget			parent,
	void			(*color_cb)( Pixel )
));
static void		AnalogColor_SliderCB OL_ARGS((
	Slider *		p,
	XtPointer		closure,
	Boolean			more
));
static void		AnalogColor_Set OL_ARGS((
	Widget			w,
	Pixel			pixel,
	unsigned int		flag
));
static Widget		AnalogGray_Create OL_ARGS((
	Widget			parent,
	void			(*color_cb)( Pixel )
));
static void		AnalogGray_SliderCB OL_ARGS((
	Slider *		p,
	XtPointer		closure,
	Boolean			more
));
static void		AnalogGray_Set OL_ARGS((
	Widget			w,
	Pixel			pixel,
	unsigned int		flag
));
static void		AnalogPseudoColor_SliderCB OL_ARGS((
	Slider *		p,
	XtPointer		closure,
	Boolean			more
));
static void		AnalogGrayScale_SliderCB OL_ARGS((
	Slider *		p,
	XtPointer		closure,
	Boolean			more
));

/*
 * Local data:
 */

static char		BlackColor            [] = "black";
static char		WhiteColor            [] = "white";
static char		DefaultWorkspaceColor [] = "#8A00AE00DB00";
static char		DefaultBG0            [] = "#E7009E007900";
static char		DefaultBG1            [] = "#AA00AA00AA00";
static char		DefaultBG1Contrast    [] = "#000";
static char		DefaultTextFontColor  [] = "#000";
static char		DefaultHelpKeyColor   [] = "#00000000E000";
static char		DefaultTextBackground [] = "#DB00DB00BA00";

static char		BW_ColorTupleList         [] = "";
static char		PoorColor_ColorTupleList  [] = "\
	(#E400E400E400,#AA00AA00AA00,#850085008500,#000000000000)\
";

static VisualItem	visual_table[] = {
    {	/* StaticGray  */
	DiscreteColor_Create, DiscreteColor_Set, "",
	AnalogGray_Create,    AnalogGray_Set,    ""
    },
    {	/* GrayScale   */
	DiscreteColor_Create, DiscreteColor_Set, "",
	AnalogGray_Create,    AnalogGray_Set,    ""
    },
    {	/* StaticColor */
	DiscreteColor_Create, DiscreteColor_Set, PoorColor_ColorTupleList,
	AnalogColor_Create,   AnalogColor_Set,   ""
    },
    {	/* PseudoColor */
	DiscreteColor_Create, DiscreteColor_Set, PoorColor_ColorTupleList,
	AnalogColor_Create,   AnalogColor_Set,   ""
    },
    {	/* TrueColor   */
/*
 * MORE: We can do much better than this.
 */
	AnalogColor_Create,   AnalogColor_Set,   "",
	AnalogColor_Create,   AnalogColor_Set,   ""
    },
    {	/* DirectColor */
/*
 * MORE: We can do much better than this.
 */
	AnalogColor_Create,   AnalogColor_Set,   "",
	AnalogColor_Create,   AnalogColor_Set,   ""
    }
};

	/*
	 * For DiscreteBW:
	 */

static ExclusiveItem	_video[] = {
	{ (XtArgVal)"Black on White" },
	{ (XtArgVal)"White on Black" },
};
static List		video		 = LIST(ExclusiveItem, _video);
static Exclusive	DiscreteBW_Video = EXCLUSIVE("video", "Video Choice", &video);

static void		(*local_bw_cb) OL_ARGS(( Pixel , Pixel ));

	/*
	 * For DiscreteColor:
	 */

static String		color_fields[] = {
	XtNforeground
};

static Cardinal		color_last_set;

static Boolean		color_last_dim	= False;

	/*
	 * For AnalogColor and AnalogGray:
	 */

static void		(*local_color_cb) ();

static Slider		AnalogGray	= COLOR_SLIDER("graySlider");
static Slider		AnalogRed	= COLOR_SLIDER("redSlider");
static Slider		AnalogGreen	= COLOR_SLIDER("greenSlider");
static Slider		AnalogBlue	= COLOR_SLIDER("blueSlider");

static Widget		color_chip;
static XColor		color_of_color_chip;


/**
 ** DefaultColor()
 **/

String
#if	OlNeedFunctionPrototypes
DefaultColor (
	unsigned int		color_flags,
	unsigned int		bw_flags
)
#else
DefaultColor (color_flags, bw_flags)
	unsigned int		color_flags;
	unsigned int		bw_flags;
#endif
{
	Boolean			contrasting	= False;


	if (PlanesOfScreen(SCREEN) < 2) {
		if (bw_flags == BW_BG)
			return (WhiteColor);
		else
			return (BlackColor);
	}

	if (color_flags & CONTRAST) {
		contrasting = True;
		color_flags &= ~CONTRAST;
	}
	switch (color_flags) {
	case COLOR_WORKSPACE:
		return (DefaultWorkspaceColor);
	case COLOR_BG0:
	case COLOR_INPUT_WINDOW:
	case COLOR_INPUT_FOCUS:
		return (DefaultBG0);
	case COLOR_BG1:
		return (contrasting? DefaultBG1Contrast : DefaultBG1);
	case COLOR_TEXT_BG:
		return (contrasting? DefaultTextFontColor : DefaultTextBackground);
	case COLOR_HELP_FG:
		return (contrasting? DefaultHelpKeyColor : DefaultTextFontColor);
	}
	/*NOTREACHED*/

} /* DefaultColor */

/**
 ** DefaultColorTupleList()
 **/

String
#if	OlNeedFunctionPrototypes
DefaultColorTupleList (
	void
)
#else
DefaultColorTupleList ()
#endif
{
	Visual *		visual	= XDefaultVisualOfScreen(SCREEN);

	VisualItem *		p;


	if (PlanesOfScreen(SCREEN) < 2)
		return (BW_ColorTupleList);

	p = &visual_table[visual->class];
	if (PlanesOfScreen(SCREEN) < wsm.depth_threshold)
		return (p->poor_color_tuple_list);
	else
		return (p->rich_color_tuple_list);
} /* DefaultColorTupleList */

/**
 ** PixelToString()
 **/

String
#if	OlNeedFunctionPrototypes
PixelToString (
	Widget			w,
	Pixel			pixel
)
#else
PixelToString (w, pixel)
	Widget			w;
	Pixel			pixel;
#endif
{
	XColor			color;

	Display *		display = XtDisplayOfObject(w);

	Screen *		screen  = XtScreenOfObject(w);

	Colormap		colormap= DefaultColormapOfScreen(screen);

	static String		ret	= "#RRRRGGGGBBBB";


	color.pixel = pixel;
	XQueryColor (display, colormap, &color);
	sprintf (ret, "#%4.4X%4.4X%4.4X", color.red, color.green, color.blue);

	return (ret);
} /* PixelToString */

/**
 ** StringToPixel()
 **/

Pixel
#if	OlNeedFunctionPrototypes
StringToPixel (
	Widget			w,
	String			color
)
#else
StringToPixel (w, color)
	Widget			w;
	String			color;
#endif
{
	XrmValue		from;
	XrmValue		to;

	Pixel			pixel;


	from.addr = (XtPointer)color;
	from.size = strlen(color) + 1;
	to.addr   = (XtPointer)&pixel;
	to.size   = sizeof(Pixel);
	XtConvert (w, XtRString, &from, XtRPixel, &to);

	return (*(Pixel *)to.addr);
} /* StringToPixel */

/**
 ** CreateColorMenu()
 **/

Widget
#if	OlNeedFunctionPrototypes
CreateColorMenu (
	Widget			parent,
	void			(*color_cb)( Pixel ),
	void			(*bw_cb)( Pixel , Pixel )
)
#else
CreateColorMenu (parent, color_cb, bw_cb)
	Widget			parent;
	void			(*color_cb)();
	void			(*bw_cb)();
#endif
{
	Screen *		screen	= XtScreenOfObject(parent);

	Visual *		visual	= XDefaultVisualOfScreen(screen);

	VisualItem *		p;


	if (PlanesOfScreen(screen) < 2)
		return (DiscreteBW_Create(parent, bw_cb));

	p = &visual_table[visual->class];
	if (PlanesOfScreen(screen) < wsm.depth_threshold)
		return ((*p->poor_create)(parent, color_cb));
	else
		return ((*p->rich_create)(parent, color_cb));
} /* CreateColorMenu */

/**
 ** SetColorMenu()
 **/

void
#if	OlNeedFunctionPrototypes
SetColorMenu (
	Widget			w,
	Pixel			pixel,
	unsigned int		flag
)
#else
SetColorMenu (w, pixel, flag)
	Widget			w;
	Pixel			pixel;
	unsigned int		flag;
#endif
{
	Screen *		screen	= XtScreenOfObject(w);

	Visual *		visual	= XDefaultVisualOfScreen(screen);

	VisualItem *		p;


	if (PlanesOfScreen(screen) < 2) {
		DiscreteBW_Set (w, pixel);
		return;
	}

	p = &visual_table[visual->class];
	if (PlanesOfScreen(screen) < wsm.depth_threshold)
		(*p->poor_set) (w, pixel, flag);
	else
		(*p->rich_set) (w, pixel, flag);

	return;
} /* SetColorMenu */

/**
 ** DiscreteBW_Create()
 **/

static Widget
#if	OlNeedFunctionPrototypes
DiscreteBW_Create (
	Widget			parent,
	void			(*bw_cb)()
)
#else
DiscreteBW_Create (parent, bw_cb)
	Widget			parent;
	void			(*bw_cb)();
#endif
{
	Cardinal n = 0;
	Display *dpy = XtDisplay(parent);
	
        _video[0].name = (XtArgVal) OLG(blackOnWhite,fixedString);
        _video[1].name = (XtArgVal) OLG(whiteOnBlack,fixedString);
	video.entry = (ADDR)_video;
	DiscreteBW_Video.string = OLG(video,fixedString);
	DiscreteBW_Video.items = &video;
	DiscreteBW_Video.f = DiscreteBW_ExclusivesCB;
	DiscreteBW_Video.current_item =
		(IS_WHITE_ON_BLACK()? WHITE_ON_BLACK : BLACK_ON_WHITE);
	local_bw_cb = bw_cb;
	CreateExclusive (parent, &DiscreteBW_Video, False);
	return (DiscreteBW_Video.w);
} /* DiscreteBW_Create */

/**
 ** DiscreteBW_ExclusivesCB()
 **/

static void
#if	OlNeedFunctionPrototypes
DiscreteBW_ExclusivesCB (
	Exclusive *		exclusive
)
#else
DiscreteBW_ExclusivesCB (exclusive)
	Exclusive *		exclusive;
#endif
{
	Screen *		screen = XtScreenOfObject(exclusive->w);

#define White	WhitePixelOfScreen(screen)
#define Black	BlackPixelOfScreen(screen)


	if (exclusive->current_item == WHITE_ON_BLACK)
		(*local_bw_cb) (White, Black);
	else
		(*local_bw_cb) (Black, White);

#undef	White
#undef	Black
	return;
} /* DiscreteBW_ExclusivesCB */

/**
 ** DiscreteBW_Set()
 **/

static void
#if	OlNeedFunctionPrototypes
DiscreteBW_Set (
	Widget			w,
	Pixel			background
)
#else
DiscreteBW_Set (w, background)
	Widget			w;
	Pixel			background;
#endif
{
	Screen *		screen = XtScreenOfObject(w);

#define White	WhitePixelOfScreen(screen)
#define Black	BlackPixelOfScreen(screen)

	if (background == Black)
		SetExclusive (&DiscreteBW_Video, WHITE_ON_BLACK, OL_NONE);
	else
		SetExclusive (&DiscreteBW_Video, BLACK_ON_WHITE, OL_NONE);

#undef	White
#undef	Black
	return;
} /* DiscreteBW_Set */

/**
 ** DiscreteColor_Create()
 **/

static Widget
#if	OlNeedFunctionPrototypes
DiscreteColor_Create (
	Widget			parent,
	void			(*color_cb)( Pixel )
)
#else
DiscreteColor_Create (parent, color_cb)
	Widget			parent;
	void			(*color_cb)();
#endif
{
	Widget			w;
	Widget			cw;

	Screen *		screen	= XtScreenOfObject(parent);

	Cardinal		ncolors	= (1 << PlanesOfScreen(screen));
	Cardinal		n;

	ColorMenuItem *		colors;

	char			name[20];
	Display *		dpy = XtDisplay(parent);

	colors = ARRAY(ColorMenuItem, ncolors);
	for (n = 0; n < ncolors; n++)
		colors[n].foreground = (XtArgVal)n;

	/*
	 * Note: The XtNnoneSet state comes and goes. We don't want
	 * the user to be able to unset a button by clicking on it,
	 * but we do want the client to be able to clear all the buttons.
	 */
	sprintf (name, "colormenu%d", PlanesOfScreen(screen));
      	cw = CreateCaption("colorCaption",
			   OLG(colorChoices,fixedString), parent);
      	XtVaSetValues (
      		cw,
      		XtNposition,    (XtArgVal)OL_TOP,
      		XtNalignment,   (XtArgVal)OL_LEFT,
      		(String)0
      	);
	w = XtVaCreateManagedWidget(
		name,
		flatColorsWidgetClass,
		cw,
		XtNnoneSet,       (XtArgVal)True,
		XtNselectProc,    (XtArgVal)DiscreteColor_SelectCB,
		XtNclientData,    (XtArgVal)color_cb,
		XtNitems,         (XtArgVal)colors,
		XtNnumItems,      (XtArgVal)ncolors,
		XtNitemFields,    (XtArgVal)color_fields,
		XtNnumItemFields, (XtArgVal)XtNumber(color_fields),
		XtNlayoutType,	  (XtArgVal)OL_FIXEDCOLS,
                XtNmeasure,	  (XtArgVal)16,
		XtNitemMinWidth,  (XtArgVal)
                    OlScreenPointToPixel (OL_HORIZONTAL, 18, screen),
		XtNitemMinHeight, (XtArgVal)
                    OlScreenPointToPixel (OL_VERTICAL, 23, screen),
		(String)0
	);

	XtAddCallback (
		w,
		XtNdestroyCallback,
		DiscreteColor_DestroyCB,
		(XtPointer)colors
	);

	return (w);
} /* DiscreteColor_Create */

/**
 ** DiscreteColor_DestroyCB
 **/

static void
#if	OlNeedFunctionPrototypes
DiscreteColor_DestroyCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
DiscreteColor_DestroyCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	ColorMenuItem *		colors	= (ColorMenuItem *)client_data;

	XtFree (colors);
	return;
} /* DiscreteColor_DestroyCB */

/**
 ** DiscreteColor_SelectCB()
 **/

static void
#if	OlNeedFunctionPrototypes
DiscreteColor_SelectCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
DiscreteColor_SelectCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	color_cb_type		color_cb = (color_cb_type)client_data;

	OlFlatCallData *	cd       = (OlFlatCallData *)call_data;


	/*
	 * Now that the user has selected a color, he can't unselect it.
	 */
	XtVaSetValues (
		w,
		XtNnoneSet,       (XtArgVal)False,
		(String)0
	);
	(*color_cb) ((Pixel)(color_last_set = cd->item_index));

	return;
} /* DiscreteColor_SelectCB */

/**
 ** DiscreteColor_Set()
 **/

static void
#if	OlNeedFunctionPrototypes
DiscreteColor_Set (
	Widget			w,
	Pixel			pixel,
	unsigned int		flag
)
#else
DiscreteColor_Set (w, pixel, flag)
	Widget			w;
	Pixel			pixel;
	unsigned int		flag;
#endif
{
	Cardinal		indx;

	Boolean			set;
	Boolean			dim;


	switch (flag) {

	case COLOR_SET_NONE:
		/*
		 * No color is selected by the client, so allow this to be
		 * reflected in the exclusives.
		 */
		XtVaSetValues (
			w,
			XtNnoneSet,       (XtArgVal)True,
			(String)0
		);
		if (XtIsSensitive(w))
			XtSetSensitive (w, False);
		indx = color_last_set;
		set = False;
		break;

	case COLOR_SET_ONE:
	case COLOR_SET_MANY:
		if (!XtIsSensitive(w))
			XtSetSensitive (w, True);

		/*
		 * MORE: Fix the FlatExclusives so that the itemsTouched
		 * is not needed. Without it the items don't pick up
		 * the dim state until an exposure.
		 */
		dim = (flag == COLOR_SET_MANY);
		if (color_last_dim != dim)
			XtVaSetValues (
				w,
				XtNdim, (XtArgVal)(color_last_dim = dim),
				XtNitemsTouched, (XtArgVal)True,
				(String)0
			);
		indx = color_last_set = (Cardinal)pixel;
		set = True;
		break;
	}
	OlVaFlatSetValues (w, indx, XtNset, (XtArgVal)set, (String)0);

	return;
} /* DiscreteColor_Set */

/**
 ** AnalogColor_Create()
 **/

static Widget
#if	OlNeedFunctionPrototypes
AnalogColor_Create (
	Widget			parent,
	void			(*color_cb)( Pixel )
)
#else
AnalogColor_Create (parent, color_cb)
	Widget			parent;
	void			(*color_cb)();
#endif
{
 	Screen *		screen	= XtScreenOfObject(parent);
	Widget			box;
	Display *		dpy;
	Visual *		visual;
	extern Boolean		ColorAllocError;
	

	parent = XtVaCreateManagedWidget(
		"rubberBox",
		rubberTileWidgetClass,
		parent,
		XtNorientation, (XtArgVal)OL_HORIZONTAL,
		XtNshadowThickness,	(XtArgVal)0,
		(String)0
	);

	box = XtVaCreateManagedWidget(
		"colorSlidersBox",
		controlAreaWidgetClass,
		parent,
		XtNlayoutType,		(XtArgVal)OL_FIXEDCOLS,
		XtNalignCaptions,	(XtArgVal)True,
		XtNshadowThickness,	(XtArgVal)0,
		(String)0
	);

	dpy = XtDisplayOfObject(box);
	
	visual = DefaultVisualOfScreen (screen);
	AnalogRed.f = AnalogGreen.f = AnalogBlue.f =
	    ((visual->class & 0x1) && !ColorAllocError) ?
		AnalogPseudoColor_SliderCB : AnalogColor_SliderCB;

	AnalogRed.closure   = (XtPointer)DoRed;
	AnalogRed.string    = OLG(red,fixedString);
	AnalogGreen.closure = (XtPointer)DoGreen;
	AnalogGreen.string  = OLG(green,fixedString);
	AnalogBlue.closure  = (XtPointer)DoBlue;
	AnalogBlue.string   = OLG(blue,fixedString);

	local_color_cb = color_cb;

	CreateSlider (box, &AnalogRed, True, True);
	CreateSlider (box, &AnalogGreen, True, True);
	CreateSlider (box, &AnalogBlue, True, True);

	/*
	 * This is a bit expensive, but it is elegant :-)
	 * Our goal is to have the color chip centered vertically
	 * next to the stack of sliders. Here's how we do it:
	 * The outer RubberTile widget ("parent"/"rubberBox") will
	 * ensure that the container for the color chip (reused
	 * "box" variable for this widget named "chipBox")
	 * is sized vertically to span the slider stack. The
	 * color chip container has three children, with the color
	 * chip in the middle. This allows the resource database
	 * to assign weights to each child that can play out
	 * all size adjustments equally to the outer two children
	 * leaving the middle child (the color chip) centered.
	 *
	 * MORE: The ControlArea really should allow vertical centering
	 * of its children, just like the Flats.
	 */
	box = XtVaCreateManagedWidget(
		"chipBox",
		rubberTileWidgetClass,
		parent,
		XtNrefSpace,	(XtArgVal)
                    OlScreenMMToPixel (OL_HORIZONTAL, 3.81, screen),
		XtNorientation, (XtArgVal)OL_VERTICAL,
		XtNshadowThickness,	(XtArgVal)0,
		(String)0
	);
	XtVaCreateManagedWidget(
		"foo",
		stubWidgetClass,
		box,
		XtNweight,	(XtArgVal)1,
		XtNwidth,	(XtArgVal)1,
		XtNheight,	(XtArgVal)1,
		(String)0
	);
	color_chip = XtVaCreateManagedWidget(
		"colorChip",
		stubWidgetClass,
		box,
		XtNweight,	(XtArgVal)0,
		XtNborderWidth, (XtArgVal)
                    OlScreenPointToPixel (OL_HORIZONTAL, 1, screen),
		XtNwidth,	(XtArgVal)
                    OlScreenMMToPixel (OL_HORIZONTAL, 10.16, screen),
		XtNheight,	(XtArgVal)
                    OlScreenMMToPixel (OL_VERTICAL, 10.16, screen),
		(String)0
	);
	XtVaCreateManagedWidget(
		"bar",
		stubWidgetClass,
		box,
		XtNweight,	(XtArgVal)1,
		XtNwidth,	(XtArgVal)1,
		XtNheight,	(XtArgVal)1,
		(String)0
	);

	return (parent);
} /* AnalogColor_Create */

#define INTENSITY(s)	((unsigned short) ((65535L * \
			   (long)((s)->slider_value - (s)->slider_min)) / \
			   (long)((s)->slider_max - (s)->slider_min)))

/**
 ** AnalogColor_SliderCB()
 **/

static void
#if	OlNeedFunctionPrototypes
AnalogColor_SliderCB (
	Slider *		p,
	XtPointer		closure,
	Boolean			more
)
#else
AnalogColor_SliderCB (p, closure, more)
	Slider *		p;
	XtPointer		closure;
	Boolean			more;
#endif
{
	Display *		display	= XtDisplayOfObject(p->w);

	Screen *		screen	= XtScreenOfObject(p->w);

	Colormap		colormap= DefaultColormapOfScreen(screen);
	XColor			tmpColor;


	tmpColor.red = INTENSITY (&AnalogRed);
	tmpColor.green = INTENSITY (&AnalogGreen);
	tmpColor.blue = INTENSITY (&AnalogBlue);

	if (XAllocColor (display, colormap, &tmpColor))
	{
	    XtVaSetValues (color_chip,
			   XtNbackground, (XtArgVal)tmpColor.pixel,
			   (String)0);
	    color_of_color_chip = tmpColor;
	}

	if (!more)
		(*local_color_cb) (color_of_color_chip.pixel);

	return;
} /* AnalogColor_SliderCB */

/**
 ** AnalogColor_Set()
 **/

static void
#if	OlNeedFunctionPrototypes
AnalogColor_Set (
	Widget			w,
	Pixel			pixel,
	unsigned int		flag
)
#else
AnalogColor_Set (w, pixel, flag)
	Widget			w;
	Pixel			pixel;
	unsigned int		flag;
#endif
{
	Display *		display = XtDisplayOfObject(w);

	Screen *		screen  = XtScreenOfObject(w);

	Colormap		colormap= DefaultColormapOfScreen(screen);

	/* If the color chip is already set to the correct color, avoid
	 * having the sliders jump around like a fish on deck.
	 */

	if (color_of_color_chip.pixel == pixel)
	    return;

	color_of_color_chip.pixel = pixel;
	XQueryColor (display, colormap, &color_of_color_chip);

	XtVaSetValues (
		color_chip,
		XtNbackground, (XtArgVal)pixel,
		(String)0
	);
	SetColorSlider (AnalogRed, color_of_color_chip.red);
	SetColorSlider (AnalogGreen, color_of_color_chip.green);
	SetColorSlider (AnalogBlue, color_of_color_chip.blue);

	return;
} /* AnalogColor_Set */

/**
 ** AnalogGray_Create()
 **/

static Widget
#if	OlNeedFunctionPrototypes
AnalogGray_Create (
	Widget			parent,
	void			(*color_cb)( Pixel )
)
#else
AnalogGray_Create (parent, color_cb)
	Widget			parent;
	void			(*color_cb)();
#endif
{
	Screen *		screen	= XtScreenOfObject(parent);
	int			dist;
	Display *		dpy;
	Visual *		visual;
	extern Boolean		ColorAllocError;


	parent = XtVaCreateManagedWidget(
		"rubberBox",
		rubberTileWidgetClass,
		parent,
		XtNorientation, (XtArgVal)OL_HORIZONTAL,
		(String)0
	);
	
	dpy = XtDisplayOfObject(parent);
	
	visual = DefaultVisualOfScreen (screen);
	AnalogGray.f = ((visual->class & 0x1) && !ColorAllocError) ?
	    AnalogGrayScale_SliderCB : AnalogGray_SliderCB;

	AnalogGray.string = OLG(gray,fixedString);
	local_color_cb = color_cb;
	CreateSlider (parent, &AnalogGray, True, True);

	/* .25 horizontal inches */
        dist=OlScreenMMToPixel(OL_HORIZONTAL, 6.35, screen);

	color_chip = XtVaCreateManagedWidget(
		"grayChip",
		stubWidgetClass,
		parent,
		XtNborderWidth, (XtArgVal)
                    OlScreenPointToPixel(OL_HORIZONTAL, 1, screen),
		XtNrefSpace,	(XtArgVal)
                    OlScreenMMToPixel(OL_HORIZONTAL, 3.81, screen),
		XtNwidth,	(XtArgVal) dist,
		XtNheight,	(XtArgVal) dist,
		(String)0
	);

	return (AnalogGray.w);
} /* AnalogGray_Create */

/**
 ** AnalogGray_SliderCB()
 **/

static void
#if	OlNeedFunctionPrototypes
AnalogGray_SliderCB (
	Slider *		p,
	XtPointer		closure,
	Boolean			more
)
#else
AnalogGray_SliderCB (p, closure, more)
	Slider *		p;
	XtPointer		closure;
	Boolean			more;
#endif
{
	Display *		display	= XtDisplayOfObject(p->w);

	Screen *		screen	= XtScreenOfObject(p->w);

	Colormap		colormap= DefaultColormapOfScreen(screen);
	XColor			tmpColor;


	tmpColor.red   =
	tmpColor.green =
	tmpColor.blue  = INTENSITY (p);

	if (XAllocColor (display, colormap, &tmpColor))
	{
	    XtVaSetValues (color_chip,
			   XtNbackground, (XtArgVal)tmpColor.pixel,
			   (String)0);
	    color_of_color_chip = tmpColor;
	}

	if (!more)
		(*local_color_cb) (color_of_color_chip.pixel);

	return;
} /* AnalogGray_SliderCB */

/**
 ** AnalogGray_Set()
 **/

static void
#if	OlNeedFunctionPrototypes
AnalogGray_Set (
	Widget			w,
	Pixel			pixel,
	unsigned int		flag
)
#else
AnalogGray_Set (w, pixel, flag)
	Widget			w;
	Pixel			pixel;
	unsigned int		flag;
#endif
{
	Display *		display = XtDisplayOfObject(w);

	Screen *		screen  = XtScreenOfObject(w);

	Colormap		colormap= DefaultColormapOfScreen(screen);


	color_of_color_chip.pixel = pixel;
	XQueryColor (display, colormap, &color_of_color_chip);

	XtVaSetValues (
		color_chip,
		XtNbackground, (XtArgVal)pixel,
		(String)0
	);
	SetColorSlider (AnalogGray, color_of_color_chip.red);

	return;
} /* AnalogGray_Set */

/**
 ** AnalogPseudoColor_SliderCB()
 **/

static void
#if	OlNeedFunctionPrototypes
AnalogPseudoColor_SliderCB (
	Slider *		p,
	XtPointer		closure,
	Boolean			more
)
#else
AnalogPseudoColor_SliderCB (p, closure, more)
	Slider *		p;
	XtPointer		closure;
	Boolean			more;
#endif
{
	color_of_color_chip.red = INTENSITY (&AnalogRed);
	color_of_color_chip.green = INTENSITY (&AnalogGreen);
	color_of_color_chip.blue = INTENSITY (&AnalogBlue);

	(*local_color_cb) (&color_of_color_chip, more);

	return;
} /* AnalogPseudoColor_SliderCB */

/**
 ** AnalogGrayScale_SliderCB()
 **/

static void
#if	OlNeedFunctionPrototypes
AnalogGrayScale_SliderCB (
	Slider *		p,
	XtPointer		closure,
	Boolean			more
)
#else
AnalogGrayScale_SliderCB (p, closure, more)
	Slider *		p;
	XtPointer		closure;
	Boolean			more;
#endif
{
	color_of_color_chip.red   =
	color_of_color_chip.green =
	color_of_color_chip.blue  = INTENSITY (p);

	(*local_color_cb) (&color_of_color_chip, more);

	return;
} /* AnalogGrayScale_SliderCB */
