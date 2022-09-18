/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#if	!defined(NOIDENT)
#ident	"@(#)dtm:olwsm/FSelector.c	1.16"
#endif

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLookP.h>
#include <Xol/Olg.h>
#include <Xol/ChangeBar.h>
#include <Xol/ColorChip.h>

#include <FSelectorP.h>

/*
 * Convenient macros:
 */

#define FLAT_P(w)        ((FlatSelectorWidget)(w))->flat
#define EXCLUSIVES_P(w)  ((FlatSelectorWidget)(w))->exclusives
#define SELECTOR_P(w)    ((FlatSelectorWidget)(w))->selector
#define FLAT_IP(i)       ((FlatSelectorItem)(i))->flat
#define EXCLUSIVES_IP(i) ((FlatSelectorItem)(i))->exclusives
#define SELECTOR_IP(i)   ((FlatSelectorItem)(i))->selector

/*
 * Global data:
 */

char			XtNchipHeight [] = "chipHeight";
char			XtNchipWidth  [] = "chipWidth";
char			XtNchipColor  [] = "chipColor";
char			XtNchipSpace  [] = "chipSpace";

char			XtCChipHeight [] = "ChipHeight";
char			XtCChipWidth  [] = "ChipWidth";
char			XtCChipSpace  [] = "ChipSpace";
char			XtCChipColor  [] = "ChipColor";

/*
 * Resources:
 */

static XtResource	resources[] = {

#define offset(F)        XtOffsetOf(FlatSelectorRec,F)

	/*
	 * Override some superclass resources:
	 */
    {
	XtNbuttonType, XtCButtonType, XtROlDefine, sizeof(OlDefine),
	offset(exclusives.button_type), XtRImmediate, (XtPointer)OL_RECT_BTN
    },
    {
	XtNexclusives, XtCExclusives, XtRBoolean, sizeof(Boolean),
	offset(exclusives.exclusive_settings), XtRImmediate, (XtPointer)False
    },
    {
	XtNsameWidth, XtCSameWidth, XtROlDefine, sizeof(OlDefine),
	offset(row_column.same_width), XtRImmediate, (XtPointer)OL_ALL
    },
    {
	XtNsameHeight, XtCSameHeight, XtROlDefine, sizeof(OlDefine),
	offset(row_column.same_height), XtRImmediate, (XtPointer)OL_ALL
    },

	/*
	 * New resources:
	 */
    {	/* SGI */
	XtNallowChangeBars, XtCAllowChangeBars, XtRBoolean, sizeof(Boolean),
	offset(selector.allow_change_bars), XtRImmediate, (XtPointer)False
    },

#undef	offset
};

static OlFlatReqRsc		required_resources[] = {
	{ XtNchangeBar },
};

static XtResource		item_resources[] = {

#define offset(F)        XtOffsetOf(FlatSelectorItemRec,selector.F)

    {	/* SGI */
	XtNchangeBar, XtCChangeBar,
	XtROlDefine, sizeof(OlDefine), offset(change_bar),
	XtRString, (XtPointer)"none"
    },
    {
	XtNchipHeight, XtCChipHeight,
	XtRDimension, sizeof(Dimension), offset(chip_height),
	XtRString, (XtPointer)".25 horizontal inches"
    },
    {
	XtNchipWidth, XtCChipWidth,
	XtRDimension, sizeof(Dimension), offset(chip_width),
	XtRString, (XtPointer)".25 horizontal inches"
    },
    {
	XtNchipSpace, XtCChipSpace,
	XtRDimension, sizeof(Dimension), offset(chip_space),
	XtRString, (XtPointer)".125 horizontal inches"
    },
    {
	XtNchipColor, XtCChipColor,
	XtRPixel, sizeof(Pixel), offset(chip_color),
	XtRString, (XtPointer)XtDefaultForeground
    }

#undef	offset
};

/*
 * Local data:
 */

static Dimension	chip_width;
static Dimension	chip_height;
static Dimension	chip_space;

static Pixel		chip_color;

static void		(*size_proc) OL_ARGS((
	Screen *		screen,
	OlgAttrs *		attrs,
	XtPointer *		lbl,
	Dimension *		p_width,
	Dimension *		p_height
));

static void		(*draw_proc) OL_ARGS((
	Screen *		screen,
	Drawable		win,
	OlgAttrs *		attrs,
	Position		x,
	Position		y,
	Dimension		width,
	Dimension		height,
	XtPointer		lbl
));

/*
 * Local routines:
 */

static void		ClassInitialize OL_NO_ARGS();

static void		Initialize OL_ARGS((
	Widget			request,
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
));
static void		Destroy OL_ARGS((
	Widget			w
));
static Boolean		SetValues OL_ARGS((
	Widget			current,
	Widget			request,
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
));
static void		DrawItem OL_ARGS((
	Widget			w,
	FlatItem		item,
	OlFlatDrawInfo *	di
));
static Boolean		ItemActivate OL_ARGS((
	Widget			w,
	FlatItem		item,
	OlVirtualName		type,
	XtPointer		data
));
static void		ItemDimensions OL_ARGS((
	Widget			w,
	FlatItem		item,
	register Dimension *	p_width,
	register Dimension *	p_height
));
static Boolean		ItemSetValues OL_ARGS((
	Widget			w,
	FlatItem		current,
	FlatItem		request,
	FlatItem		new,
	ArgList			args,
	Cardinal *		num_args
));
static void		ButtonHandler OL_ARGS((
	Widget			w,
	OlVirtualEvent		ve
));
static void		_SizeItem OL_ARGS((
	Screen *		screen,
	OlgAttrs *		attrs,
	XtPointer		lbl,
	Dimension *		p_width,
	Dimension *		p_height
));
static void		_DrawItem OL_ARGS((
	Screen *		screen,
	Drawable		win,
	OlgAttrs *		attrs,
	Position		x,
	Position		y,
	Dimension		width,
	Dimension		height,
	XtPointer		lbl
));
static Boolean ItemLocCursorDims OL_ARGS((Widget, FlatItem, OlFlatDrawInfo *));

/*
 * Translations and actions:
 */

/*
 * Note: since the 'augment' and 'override' directives don't work for
 * class translations, we have to copy the generic translations and then
 * append what we need.  See the ClassInitialize Procedure.
 */
#if	Xt_augment_works_right
OLconst static char	translations[] = "#augment\
	<Enter>:	OlAction() \n\
	<Leave>:	OlAction() \n\
	<BtnMotion>:	OlAction() \n\
";
#else
OLconst static char	translations[] = "\
	<Enter>:	OlAction() \n\
	<Leave>:	OlAction() \n\
	<BtnMotion>:	OlAction() \n\
";
#endif

static OlEventHandlerRec event_procs[] = {
	{ ButtonPress,   ButtonHandler },
	{ ButtonRelease, ButtonHandler },
	{ EnterNotify,   ButtonHandler },
	{ LeaveNotify,   ButtonHandler },
	{ MotionNotify,  ButtonHandler }
};

/*
 * Class record structure:
 */

FlatSelectorClassRec	flatSelectorClassRec = {
    /*
     * Core class:
     */
    {
    /* superclass          */	(WidgetClass)&flatButtonsClassRec,
    /* class_name          */	"FlatSelector",
    /* widget_size         */	sizeof(FlatSelectorRec),
    /* class_initialize    */	ClassInitialize,
    /* class_part_initialize*/	NULL,
    /* class_inited        */	False,
    /* initialize          */	NULL,
    /* initialize_hook     */	NULL,
    /* realize             */	XtInheritRealize,
    /* actions             */	NULL,
    /* num_actions         */	0,
    /* resources           */	resources,
    /* num_resources       */	XtNumber(resources),
    /* xrm_class           */	NULLQUARK,
    /* compress_motion     */	True,
    /* compress_exposure   */	True,
    /* compress_enterleave */	True,
    /* visible_interest    */	False,
    /* destroy             */	Destroy,
    /* resize              */	XtInheritResize,
    /* expose              */	XtInheritExpose,
    /* set_values          */	NULL,
    /* set_values_hook     */	NULL,
    /* set_values_almost   */	XtInheritSetValuesAlmost,
    /* get_values_hook     */	NULL,
    /* accept_focus        */	XtInheritAcceptFocus,
    /* version             */	XtVersion,
    /* callback_offsets    */	NULL,
#if	Xt_augment_works_right
    /* tm_table            */	translations,
#else
    /* tm_table            */	NULL,
#endif
    /* query_geometry      */	XtInheritQueryGeometry,
    /* display_accelerator */	XtInheritDisplayAccelerator,
    /* extension           */	NULL
    }, /* End of Core Class Part Initialization */
    {
    /* focus_on_select	   */	True,
    /* highlight_handler   */	XtInheritHighlightHandler,
    /* traversal_handler   */	XtInheritTraversalHandler,
    /* register_focus      */	NULL,
    /* activate            */	XtInheritActivateFunc,
    /* event_procs         */	event_procs,
    /* num_event_procs     */	XtNumber(event_procs),
    /* version             */	OlVersion,
    /* extension           */	NULL
    }, /* End of Primitive Class Part Initialization */
    {
    /* extension	   */	(XtPointer) NULL,
    /* transparent_bg      */	True,
    /* default_offset	   */	XtOffsetOf(FlatSelectorRec, default_item),
    /* rec_size		   */	sizeof(FlatSelectorItemRec),
    /* item_resources      */	item_resources,
    /* num_item_resources  */	XtNumber(item_resources),
    /* required_resources  */	required_resources,
    /* num_required_resources*/	XtNumber(required_resources),

		/*
		 * See ClassInitialize for procedures
		 */

    }, /* End of Flat Class Part Initialization */
    {
    /* unused              */	0,
    }, /* End of FlatRowColumn Class Part Initialization */
    {
    /* unused              */	0,
    }, /* End of FlatButtons (Exclusives) Class Part Initialization */
    {
    /* unused              */	0,
    }, /* End of FlatSelector Class Part Initialization */
};

WidgetClass	flatSelectorWidgetClass = (WidgetClass)&flatSelectorClassRec;

/**
 ** ClassInitialize()
 **/

static void
ClassInitialize OL_NO_ARGS()
{
#ifndef	Xt_augment_works_right
	char *			t;

	t = XtMalloc(
	    strlen(translations) + strlen(_OlGenericTranslationTable) + 1
	);

	sprintf (t, "%s%s", translations, _OlGenericTranslationTable);
	flatSelectorWidgetClass->core_class.tm_table = t;

#endif

		/* Inherit all superclass procedures, but	*/
		/* override a few as well provide some chained	*/
		/* procedures. This scheme saves us from	*/
		/* worrying about putting function pointers	*/
		/* in the wrong class slot if they were		*/
		/* statically declared. It also allows us to	*/
		/* inherit new functions simply be recompiling,	*/
		/* i.e., we don't have to stick XtInheritFoo	*/
		/* into the class slot.				*/
	OlFlatInheritAll(flatSelectorWidgetClass);

#undef F
#define F	flatSelectorClassRec.flat_class

	F.initialize		= Initialize;
	F.set_values		= SetValues;
	F.draw_item		= DrawItem;
	F.item_activate		= ItemActivate;
	F.item_dimensions	= ItemDimensions;
	F.item_set_values	= ItemSetValues;

	if (OlGetGui() == OL_MOTIF_GUI)
	{
		F.item_location_cursor_dimensions = ItemLocCursorDims;
	}

#undef F
} /* ClassInitialize */

/**
 ** Initialize()
 **/

static void
Initialize OLARGLIST((request, new, args, num_args))
	OLARG( Widget,			request)
	OLARG( Widget,			new)
	OLARG( ArgList,			args)
	OLGRA( Cardinal *,		num_args)
{
	SELECTOR_P(new).cb = OlCreateChangeBar(
	    new, OlContrastingColor(new, CORE_P(new).background_pixel, 25)
	);
	_OlCreateColorChip (new);

} /* Initialize */

/**
 ** Destroy()
 **/

static void
Destroy OLARGLIST((w))
	OLGRA( Widget,			w)
{
	if (SELECTOR_P(w).cb)
		OlDestroyChangeBar (w, SELECTOR_P(w).cb);
	_OlDestroyColorChip (w);
	return;
} /* Destroy */

/**
 ** SetValues()
 **/

static Boolean
SetValues OLARGLIST((current, request, new, args, num_args))
	OLARG( Widget,			current)
	OLARG( Widget,			request)
	OLARG( Widget,			new)
	OLARG( ArgList,			args)
	OLGRA( Cardinal *,		num_args)
{
	Boolean			redisplay	= False;


#define DIFFERENT(F) (SELECTOR_P(new).F != SELECTOR_P(current).F)

	if (new->core.background_pixel != current->core.background_pixel){
		if (SELECTOR_P(new).cb)
			OlChangeBarSetValues (
				new,
				OlContrastingColor(new, CORE_P(new).background_pixel, 25),
				SELECTOR_P(new).cb
			);
		redisplay = True;
	}

	/*
	 * If the XtNallowChangeBars resource has changed, just effect
	 * a redisplay.
	 */
	if (DIFFERENT(allow_change_bars))
		redisplay = True;

#undef DIFFERENT

	return (redisplay);
} /* SetValues */

/**
 ** DrawItem()
 **/

static void
DrawItem OLARGLIST((w, item, dii))
	OLARG( Widget,			w)
	OLARG( FlatItem,		item)
	OLGRA( OlFlatDrawInfo, *	dii)
{
	Cardinal		indx	= item->flat.item_index;

	Boolean			draw_selected;

	unsigned int		flags	= 0;

	OlgAttrs *		item_attrs;

	OlFlatDrawInfo		DI;
	OlFlatDrawInfo *	di = &DI;
	Boolean			motif_gui = (OlGetGui() == OL_MOTIF_GUI);

	XtPointer		lbl;

	Position		x;

	Dimension		width;


	if (!item->flat.mapped_when_managed)
		return;

	DI = *dii;

	if ( motif_gui && !EXCLUSIVES_P(w).exclusive_settings )
	{
#define THICKNESS ((PrimitiveWidget)w)->primitive.highlight_thickness
#define SUPERCLASS1 \
	  ((FlatSelectorClassRec *)flatSelectorClassRec.core_class.superclass)
#define SUPERCLASS \
	  ((FlatButtonsClassRec *)SUPERCLASS1->core_class.superclass)

		Dimension	h2 = 2 * THICKNESS;


			/* Use superclass draw_item to draw Location Cursor */
		if ( FLAT_P(w).focus_item == indx )
			(*SUPERCLASS->flat_class.draw_item)(w, item, di);

		di->x += THICKNESS;
		di->y += THICKNESS;
		di->width  -= h2;
		di->height -= h2;

#undef THICKNESS
	}

	x	= di->x;
	width	= di->width;

	/*
	 * Maybe draw the change bar.
	 */
	if (SELECTOR_P(w).allow_change_bars) {
		OlDrawChangeBar (
			w,
			SELECTOR_P(w).cb,
			SELECTOR_IP(item).change_bar,
			False,
			x, di->y,
			(Region)0
		);
		x     += OlChangeBarSpan(SELECTOR_P(w).cb);
		width -= OlChangeBarSpan(SELECTOR_P(w).cb);
	}

	/*
	 * A lot of this is the same as the superclass' draw_item
	 * method, but there are enough differences that we can't just
	 * call the superclass' method.
	 */

	if (EXCLUSIVES_P(w).exclusive_settings) {
		draw_selected = (Boolean)
			((indx == EXCLUSIVES_P(w).current_item &&
			!(EXCLUSIVES_P(w).none_set == True &&
			EXCLUSIVES_P(w).current_item == EXCLUSIVES_P(w).set_item))
					||
			(EXCLUSIVES_P(w).current_item == (Cardinal) OL_NO_ITEM &&
		     	EXCLUSIVES_IP(item).set == True) ? True : False);

	} else {
		draw_selected = (Boolean)
			((indx == EXCLUSIVES_P(w).current_item &&
			  EXCLUSIVES_IP(item).set == False)
					||
	     		(EXCLUSIVES_P(w).current_item != indx &&
			EXCLUSIVES_IP(item).set == True) ? True : False);
	}

	if (EXCLUSIVES_P(w).preview)
		draw_selected = (draw_selected == True ? False : True);

	if (draw_selected)
	        flags |= RB_SELECTED;
	else
		flags |= RB_NOFRAME;

	if (
		EXCLUSIVES_P(w).default_item == indx
	     || EXCLUSIVES_IP(item).is_default
	)
	        flags |= RB_DEFAULT;

	if (EXCLUSIVES_P(w).dim)
	        flags |= RB_DIM;

	if (!item->flat.sensitive || !XtIsSensitive(w))
	        flags |= RB_INSENSITIVE;

	/*
	 * This is gross, but it is efficient: We want to use the
	 * regular drawing routine picked by the OlFlatSetupAttributes()
	 * routine, but we also want to augment the results a bit.
	 * Since we have no way of passing private data through, we
	 * store the private data in a global place where it can
	 * be found in our private routine (ugh.)
	 */
	chip_width  = SELECTOR_IP(item).chip_width;
	chip_height = SELECTOR_IP(item).chip_height;
	chip_space  = SELECTOR_IP(item).chip_space;
	chip_color  = SELECTOR_IP(item).chip_color;
	OlFlatSetupAttributes (w, item, di, &item_attrs, &lbl, &draw_proc);
	OlgDrawRectButton (
		di->screen,
		di->drawable,
		item_attrs,
		x, di->y,
		width, di->height,
		lbl,
		_DrawItem,
		flags
	);

	if ( motif_gui && EXCLUSIVES_P(w).exclusive_settings &&
	     FLAT_P(w).focus_item == indx )
	{
		(*SUPERCLASS->flat_class.draw_item)(w, item, di);

#undef SUPERCLASS1
#undef SUPERCLASS
	}

	return;
} /* DrawItem */

/**
 ** ItemActivate()
 **/

static Boolean
ItemActivate OLARGLIST((w, item, type, data))
	OLARG( Widget,			w)
	OLARG( FlatItem,		item)
	OLARG( OlVirtualName,		type)
	OLGRA( XtPointer,		data)
{
	FlatSelectorItemRec	item_rec;

	Cardinal		indx = item->flat.item_index;
	Cardinal		i;

	Boolean			ret;
	Boolean			set;


#define SUPERCLASS \
    ((FlatButtonsClassRec *)flatSelectorClassRec.core_class.superclass)
#define SELECT(I) \
    (*SUPERCLASS->flat_class.item_activate)(w, (I), OL_SELECTKEY, data)

	switch (type) {

	case OL_SELECTKEY:
		ret = True;

		/*
		 * When the SELECT action is used, we behave like
		 * an exclusives, and when the ADJUST action is used,
		 * we behave like a nonexclusives. We get the
		 * nonexclusives behavior from our superclass, and
		 * simulate the exclusives behavior here.
		 */
		for (i = 0; i < FLAT_P(w).num_items; i++) {
			OlFlatExpandItem (w, i, (FlatItem)&item_rec);
			if (i == indx) {
				if (!item_rec.exclusives.set)
					SELECT ((FlatItem)&item_rec);
			} else {
				if (item_rec.exclusives.set)
					SELECT ((FlatItem)&item_rec);
			}
		}

		break;

	case OL_ADJUSTKEY:
		ret = True;
		SELECT (item);
		break;

	default:
		ret = False;
		break;
	}

#undef	SUPERCLASS
#undef	SELECT
	return (ret);
} /* ItemActivate */

/**
 ** ItemDimensions()
 **/

static void
ItemDimensions OLARGLIST((w, item, p_width, p_height))
	OLARG( Widget,			w)
	OLARG( FlatItem,		item)
	OLARG( register Dimension *,	p_width)
	OLGRA( register Dimension *,	p_height)
{
	XtPointer		lbl;


	/*
	 * This is gross, but it is efficient: We want to use the
	 * regular sizing routine picked by the OlFlatSetupLabelSize()
	 * routine, but we also want to augment the results a bit.
	 * Since we have no way of passing private data through, we
	 * store the private data in a global place where it can
	 * be found in our private routine (ugh.)
	 */
	chip_width  = SELECTOR_IP(item).chip_width;
	chip_height = SELECTOR_IP(item).chip_height;
	chip_space  = SELECTOR_IP(item).chip_space;
	OlFlatSetupLabelSize (w, item, &lbl, &size_proc);
	OlgSizeRectButton (
		XtScreenOfObject(w),
		FLAT_P(w).pAttrs,
		lbl,
		_SizeItem,
		0,
		p_width,
		p_height
	);

	if (SELECTOR_P(w).allow_change_bars) {
		*p_width += OlChangeBarSpan(SELECTOR_P(w).cb);
		if (*p_height < OlChangeBarHeight(SELECTOR_P(w).cb))
			*p_height = OlChangeBarHeight(SELECTOR_P(w).cb);
	}

		/* Add space for Location Cursor */
	if (OlGetGui() == OL_MOTIF_GUI)
	{
#define THICKNESS ((PrimitiveWidget)w)->primitive.highlight_thickness

		Dimension	h2 = 2 * THICKNESS, w_extra, h_extra;

			/* Recover the loss from SizeRectButton in
			 * Motif mode...
			 */
		w_extra = 4 * OlgGetHorizontalStroke(FLAT_P(w).pAttrs);
		h_extra = 4 * OlgGetVerticalStroke(FLAT_P(w).pAttrs);

		*p_width += h2 + w_extra;
		*p_height += h2 + h_extra;

#undef THICKNESS
	}

	return;
} /* ItemDimensions */

/**
 ** ItemSetValues()
 **/

static Boolean
ItemSetValues OLARGLIST((w, current, request, new, args, num_args))
	OLARG( Widget,			w)
	OLARG( FlatItem,		current)
	OLARG( FlatItem,		request)
	OLARG( FlatItem,		new)
	OLARG( ArgList,			args)
	OLGRA( Cardinal *,		num_args)
{
	Cardinal		item_index	= new->flat.item_index;

	Boolean			redisplay	= False;


#define DIFFERENT(F) (SELECTOR_IP(new).F != SELECTOR_IP(current).F)

	if (DIFFERENT(change_bar) || DIFFERENT(chip_color))
		redisplay = True;

#undef	DIFFERENT

	return (redisplay);
} /* ItemSetValues */

/**
 ** ButtonHandler()
 **/

static void
ButtonHandler OLARGLIST((w, ve))
	OLARG( Widget,			w)
	OLGRA( OlVirtualEvent,		ve)
{
	Cardinal		indx;


	switch (ve->xevent->type) {

	case ButtonPress:
	case EnterNotify:
	case LeaveNotify:
	case MotionNotify:
		switch (ve->virtual_name) {
		case OL_SELECT:
		case OL_ADJUST:
			ve->consumed = True;
			break;
		}
		break;

	case ButtonRelease:
		indx = ve->item_index;

#define ACTIVATE(A,I) OlActivateWidget(w, (A), (XtPointer)(I+1))
		switch (ve->virtual_name) {
		case OL_SELECT:
			ve->consumed = ACTIVATE(OL_SELECTKEY, indx);
			break;
		case OL_ADJUST:
			ve->consumed = ACTIVATE(OL_ADJUSTKEY, indx);
			break;
		}
#undef	ACTIVATE
		break;
	}

	return;
} /* ButtonHandler */

/**
 ** _SizeItem()
 **/

static void
_SizeItem OLARGLIST((screen, attrs, lbl, p_width, p_height))
	OLARG( Screen *,		screen)
	OLARG( OlgAttrs *,		attrs)
	OLARG( XtPointer,		lbl)
	OLARG( Dimension *,		p_width)
	OLGRA( Dimension *,		p_height)
{
	(*size_proc) (screen, attrs, lbl, p_width, p_height);

	*p_width += chip_width + chip_space;
	if (*p_height < chip_height)
		*p_height = chip_height;

	return;
} /* _SizeItem */

/**
 ** _DrawItem()
 **/

static void
_DrawItem OLARGLIST((screen, window, attrs, x, y, width, height, lbl))
	OLARG( Screen *,		screen)
	OLARG( Drawable,		window)
	OLARG( OlgAttrs *,		attrs)
	OLARG( Position,		x)
	OLARG( Position,		y)
	OLARG( Dimension,		width)
	OLARG( Dimension,		height)
	OLGRA( XtPointer,		lbl)
{
	XRectangle		rects[4];

	Dimension		h_stroke = attrs->pDev->horizontalStroke;
	Dimension		v_stroke = attrs->pDev->verticalStroke;

	_OlColorChipLabel	color;


	width -= chip_space + chip_width;
	(*draw_proc) (screen, window, attrs, x, y, width, height, lbl);

	x += width + chip_space;
	if (height < chip_height)
		chip_height = height;

	rects[0].x      = x;
	rects[0].y      = y;
	rects[0].width  = chip_width;
	rects[0].height = v_stroke;

	rects[1].x      = rects[0].x;
	rects[1].y      = rects[0].y + v_stroke;
	rects[1].width  = h_stroke;
	rects[1].height = chip_height - 2 * v_stroke;

	rects[2].x      = rects[1].x;
	rects[2].y      = rects[1].y + rects[1].height;
	rects[2].width  = rects[0].width;
	rects[2].height = v_stroke;

	rects[3].x      = rects[0].x + rects[0].width -	h_stroke;
	rects[3].y      = rects[1].y;
	rects[3].width  = h_stroke;
	rects[3].height = rects[1].height;

	/*
	 * Use any convenient GC, as long as it contrasts
	 * with the background.
	 */
	XFillRectangles (
		DisplayOfScreen(screen),
		window,
		((OlgTextLbl *)lbl)->normalGC,
		rects, 4
	);

	color.insensitive = False;
	color.pixel       = chip_color;

	_OlDrawColorChip (
		screen,
		window,
		attrs,
		x + h_stroke, y + v_stroke,
		chip_width - 2 * h_stroke, chip_height - 2 * v_stroke,
		(XtPointer)&color
	);

	return;
} /* _DrawItem */


/*
 * ItemLocCursorDims - figure out the place to draw location cursor.
 */
static Boolean
ItemLocCursorDims OLARGLIST((w, item, di))
	OLARG( Widget,			w)
	OLARG( FlatItem,		item)
	OLGRA( OlFlatDrawInfo *,	di)
{
	OlgAttrs *	attrs = FLAT_P(w).pAttrs;
	Dimension	h_sp = OlgGetHorizontalStroke(attrs),
			v_sp = OlgGetVerticalStroke(attrs);

	if (EXCLUSIVES_P(w).exclusive_settings)
	{
		di->x += h_sp;
		di->y += v_sp;
		di->width -= (h_sp + h_sp);
		di->height -= (v_sp + v_sp);
	}

	if (SELECTOR_P(w).allow_change_bars)
	{
		di->x     += OlChangeBarSpan(SELECTOR_P(w).cb);
		di->width -= OlChangeBarSpan(SELECTOR_P(w).cb);
	}

	return(True);
} /* end of ItemLocCursorDims */
