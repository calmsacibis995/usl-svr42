/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#if	!defined(NOIDENT)
#ident	"@(#)dtm:olwsm/FKeys.c	1.31"
#endif

#include "stdio.h"
#include "string.h"

#include "X11/IntrinsicP.h"
#include "X11/StringDefs.h"
#include "X11/keysym.h"

#include "Xol/OpenLookI.h"
#include "Xol/DynamicP.h"
#include "FKeysP.h"
#include "error.h"

/*
 * Macros:
 */

	/*
	 * Parts of widget records:
	 */
#define FLAT_P(w)		((FlatKeysWidget)(w))->flat
#define ROWCOLUMN_P(w)		((FlatKeysWidget)(w))->row_column
#define KEYS_P(w)		((FlatKeysWidget)(w))->keys
#define FLAT_IP(i)		((FlatKeysItem)(i))->flat
#define KEYS_IP(i)		((FlatKeysItem)(i))->keys
#define DFT_KEYS_ITEM(w)	((FlatKeysItem)(OlFlatDefaultItem(w)))->keys

	/*
	 * Convenient macros for accessing modifier masks and names:
	 */
#define NMASKS XtNumber(mask_names)
#define MASKNAME(J) \
	*(String *)((char *)resource_base + mask_names[(J)].p_name_offset)
#define MASKMASK(J) mask_names[(J)].mask

	/*
	 * Convenient macro for determining if an event starts
	 * us ``quoting'' a FlatKey.
	 */
#if	defined(XK_grave)
# define XK_GRAVE XK_grave
#else
# define XK_GRAVE XK_quoteleft
#endif
#define IsQuotingKey(VE) \
	(								\
		((XKeyEvent *)((VE)->xevent))->type == KeyPress		\
	     && (							\
			(VE)->keysym == XK_backslash			\
		     || (VE)->keysym == XK_GRAVE			\
		)							\
	)

	/*
	 * Convenient macro for checking if a new modifier/detail
	 * combination will fit.
	 */
#define SizeFlatKeyCheck(W,ITEM) \
	SizeFlatKey((W), (ITEM), (Dimension *)0,(Dimension *)0,(Dimension *)0,(Dimension *)0)

	/*
	 * Convenient macro for setting the Left to the maximum
	 * of itself and the Right.
	 */
#define SETMAX(L,R)	if ((R) > (L)) L = R

#undef Strlen
#undef Strdup
#define Strlen(S)	(S && *S? strlen((S)) : 0)
#define Strdup(S)	strcpy((String)MALLOC((unsigned)Strlen(S) + 1), S)

/*
 * Local routines:
 */

static void		ClassInitialize OL_NO_ARGS();

static void		ChangeManaged OL_ARGS((
	Widget			w,
	FlatItem *		items,
	Cardinal		num_changed
));
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
static Boolean		Activate OL_ARGS((
	Widget			w,
	OlVirtualName		type,
	XtPointer		data
));
static void		DrawItem OL_ARGS((
	Widget			w,
	FlatItem		item,
	OlFlatDrawInfo *	di
));
static Boolean ItemLocCursorDims OL_ARGS((Widget, FlatItem, OlFlatDrawInfo *));
static void		ItemDimensions OL_ARGS((
	Widget			w,
	FlatItem		item,
	register Dimension *	p_width,
	register Dimension *	p_height
));
static void		ItemHighlight OL_ARGS((
	Widget			w,
	FlatItem		item,
	OlDefine		type
));
static void		ItemInitialize OL_ARGS((
	Widget			w,
	FlatItem		request,
	FlatItem		new,
	ArgList			args,
	Cardinal *		num_args
));
static Boolean		ItemSetValues OL_ARGS((
	Widget			w,
	FlatItem		current,
	FlatItem		request,
	FlatItem		new,
	ArgList			args,
	Cardinal *		num_args
));
static void		Layout OL_ARGS((
	Widget			w
));
static void		FlatKeysLayout OL_ARGS((
	Widget			w
));
static void		MappingHandler OL_ARGS((
	Widget			w,
	OlVirtualEvent		ve
));
static void		KeyHandler OL_ARGS((
	Widget			w,
	OlVirtualEvent		ve
));
static Boolean		GrabStuff OL_ARGS((
	Widget			w,
	Time			time
));
static void		UngrabStuff OL_ARGS((
	Widget			w,
	Time			time
));
static Modifiers	KeycodeToModifier OL_ARGS((
	register XModifierKeymap *map,
	register int		keycode
));
static Boolean		ClientSaysOk OL_ARGS((
	Widget			w,
	Cardinal		indx,
	Modifiers		modifiers,
	KeySym			keysym,
	OlDefine		change
));
static void		ButtonHandler OL_ARGS((
	Widget			w,
	OlVirtualEvent		ve
));
static void		DrawText OL_ARGS((
	Widget			w,
	OlgTextLbl *		text,
	GC			gc,
	int			gravity,
	Position		x,
	Position		y,
	Dimension		width,
	Dimension		height
));
static void		SizeText OL_ARGS((
	Widget			w,
	OlgTextLbl *		text,
	Dimension *		p_width,
	Dimension *		p_height
));
static Boolean		SizeFlatKey OL_ARGS((
	Widget			w,
	FlatItem		item,
	Dimension *		p_caption_width,
	Dimension *		p_modifiers_width,
	Dimension *		p_keysym_width,
	Dimension *		p_height
));
static void		SizeKey OL_ARGS((
	Screen *		screen,
	OlgAttrs *		attrs,
	OlgTextLbl *		lbl,
	Dimension *		p_width,
	Dimension *		p_height
));
static void		DrawKey OL_ARGS((
	Screen *		screen,
	Drawable		drawable,
	OlgAttrs *		attrs,
	Position		x,
	Position		y,
	Dimension		width,
	Dimension		height,
	OlgTextLbl *		lbl
));
static String		KeysymToString OL_ARGS((
	KeySym			keysym
));
static String		KeyToString OL_ARGS((
	Modifiers		modifiers,
	KeySym			keysym
));
static void		StringToKey OL_ARGS((
	Widget			w,
	String			string,
	Modifiers *		p_modifiers,
	KeySym *		p_keysym
));
static void		CheckLabelAndKeysym OL_ARGS((
	Widget			w,
	FlatItem		item
));
static void		GetGCs OL_ARGS((
	Widget		w
));
static void		FreeGCs OL_ARGS((
	Widget		w
));

/*
 * Global data:
 */

char			XtNmodifiers     [] = "modifiers";
char			XtNkeysym        [] = "keysym";
char			XtNcaptionFont   [] = "captionFont";
char			XtNcaptionGap    [] = "captionGap";
char			XtNkeyChanged    [] = "keyChanged";
char			XtNisHeader      [] = "isHeader";

char			XtCModifiers     [] = "Modifiers";
char			XtCKeysym        [] = "Keysym";
char			XtCCaptionFont   [] = "CaptionFont";
char			XtCCaptionGap    [] = "CaptionGap";
char			XtCKeyChanged    [] = "KeyChanged";
char			XtCIsHeader      [] = "IsHeader";

char			msg[BUFSIZ];

/*
 * Local data:
 */

typedef struct MaskName {
	Cardinal		p_name_offset;
	Modifiers		mask;
}			MaskName;

static MaskName		mask_names[] = {

#define offset(F) XtOffsetOf(_OlAppAttributes,F)
	{ offset(shift_name),   (Modifiers)ShiftMask   },
	{ offset(lock_name),    (Modifiers)LockMask    },
	{ offset(control_name), (Modifiers)ControlMask },
	{ offset(mod1_name),    (Modifiers)Mod1Mask    },
	{ offset(mod2_name),    (Modifiers)Mod2Mask    },
	{ offset(mod3_name),    (Modifiers)Mod3Mask    },
	{ offset(mod4_name),    (Modifiers)Mod4Mask    },
	{ offset(mod5_name),    (Modifiers)Mod5Mask    },
#undef offset

};

static _OlAppAttributes *	resource_base;

/*
 * (Stolen from Xol/DynamicI.h)
 */
static CharKeysymMap singlechar_map[] = {
/* { ' ',  XK_space        },	Too confusing to see empty space */
   { '!',  XK_exclam       },
   { '"',  XK_quotedbl     },
   { '#',  XK_numbersign   },
   { '$',  XK_dollar       },
   { '%',  XK_percent      },
   { '&',  XK_ampersand    },
#if	defined(XK_apostrophe)
   { '\'', XK_apostrophe   },
#else
   { '\'', XK_quoteright   },
#endif
   { '(',  XK_parenleft    },
   { ')',  XK_parenright   },
   { '*',  XK_asterisk     },
   { '+',  XK_plus         },
   { ',',  XK_comma        },
   { '-',  XK_minus        },
   { '.',  XK_period       },
   { '/',  XK_slash        },
   { ':',  XK_colon        },
   { ';',  XK_semicolon    },
   { '<',  XK_less         },
   { '=',  XK_equal        },
   { '>',  XK_greater      },
   { '?',  XK_question     },
   { '@',  XK_at           },
   { '[',  XK_bracketleft  },
   { '\\', XK_backslash    },
   { ']',  XK_bracketright },
   { '^',  XK_asciicircum  },
   { '_',  XK_underscore   },
#if	defined(XK_grave)
   { '`',  XK_grave        },
#else
   { '`',  XK_quoteleft    },
#endif
   { '{',  XK_braceleft    },
   { '|',  XK_bar          },
   { '}',  XK_braceright   },
   { '~',  XK_asciitilde   },
   {   0,  0               }
};

static OlKeyOrBtnRec	FlatKeysIEDB[] = {
    { XtNdelCharBakKey, "<BackSpace>",    OL_DELCHARBAK },
    { XtNdelCharFwdKey, "<Delete>",       OL_DELCHARFWD },
    { XtNdelWordBakKey, "c s<BackSpace>", OL_DELWORDBAK },
    { XtNdelWordFwdKey, "c s<Delete>",    OL_DELWORDFWD },
};

/*
 * Translations and Actions:
 */

static char		translations[] = "\
	<FocusIn>:	OlAction()	\n\
	<FocusOut>:	OlAction()	\n\
	<Mapping>:	OlAction()	\n\
	<KeyDown>:	OlAction()	\n\
	<KeyUp>:	OlAction()	\n\
	<BtnDown>:	OlAction()	\n\
	<BtnUp>:	OlAction()	\n\
";

static OlEventHandlerRec event_procs[] = {
	{ MappingNotify, MappingHandler },
	{ KeyPress,      KeyHandler     },
	{ KeyRelease,    KeyHandler     },
	{ ButtonPress,   ButtonHandler  },
	{ ButtonRelease, ButtonHandler  },
};

/*
 * Widget instance resources:
 */

static Modifiers	default_modifiers = 0;
static XtPointer	default_keysym    = NoSymbol;

static XtResource		resources[] = {
#define offset(FIELD)        XtOffsetOf(FlatKeysRec,FIELD)

    /*
     * Some defaults differ from the super-class'.
     */
    {
	XtNlayoutType, XtCLayoutType,
	XtROlDefine, sizeof(OlDefine), offset(row_column.layout_type),
	XtRString, (XtPointer)"fixedcols"
    },
    {
	XtNsameHeight, XtCSameHeight,
	XtROlDefine, sizeof(OlDefine), offset(row_column.same_height),
	XtRString, (XtPointer)"none"
    },
    {
	XtNvSpace, XtCVSpace,
	XtRDimension, sizeof(Dimension), offset(row_column.v_space),
	XtRString, (XtPointer)".05 vertical inch"
    },

    /*
     * New resources:
     */
    {	/* SGI */
	XtNallowChangeBars, XtCAllowChangeBars,
	XtRBoolean, sizeof(Boolean), offset(keys.allow_change_bars),
	XtRString, (XtPointer)"false"
    },
    {	/* SGI */
	XtNcaptionFont, XtCCaptionFont,
	XtRFontStruct, sizeof(XFontStruct *), offset(keys.caption_font),
	XtRString, (XtPointer)OlDefaultBoldFont
    },
    {	/* SGI */
	XtNcaptionGap, XtCCaptionGap,
	XtRDimension, sizeof(Dimension), offset(keys.caption_gap),
	XtRString, (XtPointer)".2 horizontal inches"
    },
    {	/* SI */
	XtNkeyChanged, XtCCallback,
	XtRCallback, sizeof(XtPointer), offset(keys.key_changed),
	XtRCallback, (XtPointer)0
    }

#undef	offset
};

/*
 * Widget sub-object resource list:
 */

static OlFlatReqRsc		required_resources[] = {
	{ XtNtraversalOn },
	{ XtNmodifiers	 },
	{ XtNkeysym	 },
};

static XtResource		item_resources[] = {
#define offset(FIELD) XtOffsetOf(FlatKeysItemRec,keys.FIELD)

    {	/* SGI */
	XtNchangeBar, XtCChangeBar,
	XtROlDefine, sizeof(OlDefine), offset(change_bar),
	XtRString, (XtPointer)"none"
    },
	/*
	 * This is needed here because we put it in
	 * the required_list:
	 */
    {	/* SGI */
	XtNtraversalOn, XtCTraversalOn,
	XtRBoolean, sizeof(Boolean), XtOffsetOf(FlatKeysItemRec, flat.traversal_on),
	XtRString, (XtPointer)"true"
    },
	/*
	 * These are new to the FlatKeys widget:
	 */
    {	/* SGI */
	XtNisHeader, XtCIsHeader,
	XtRBoolean, sizeof(Boolean), offset(is_header),
	XtRImmediate, (XtPointer)False
    },
    {	/* SGI */
	XtNcaption, XtCCaption,
	XtRString, sizeof(String), offset(caption),
	XtRString, (XtPointer)0
    },
    {	/* SGI */
	XtNmodifiers, XtCModifiers,
	XtRModifiers, sizeof(Modifiers), offset(modifiers),
	XtRModifiers, (XtPointer)&default_modifiers
    },
    {	/* SGI */
	XtNkeysym, XtCKeysym,
	XtRPointer, sizeof(XtPointer), offset(keysym),
	XtRPointer, (XtPointer)&default_keysym
    }

#undef	offset
};

/*
 * Class record structure:
 */

FlatKeysClassRec	flatKeysClassRec = {
    /*
     * Core class:
     */
    {
    /* superclass          */	(WidgetClass)&flatRowColumnClassRec,
    /* class_name          */	"FlatKeys",
    /* widget_size         */	sizeof(FlatKeysRec),
    /* class_initialize    */	ClassInitialize,
    /* class_part_initialize*/	NULL,
    /* class_inited        */	FALSE,
    /* initialize          */	NULL,
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
    /* tm_table            */	translations,
    /* query_geometry      */	XtInheritQueryGeometry,
    /* display_accelerator */	NULL,
    /* extension           */	NULL
    },
    /*
     * Primitive class:
     */
    {
    /* focus_on_select	   */	True,
    /* highlight_handler   */	XtInheritHighlightHandler,
    /* traversal_handler   */	XtInheritTraversalHandler,
    /* register_focus      */	NULL,
    /* activate            */	Activate,
    /* event_procs         */	event_procs,
    /* num_event_procs     */	XtNumber(event_procs),
    /* version             */	OlVersion,
    /* extension           */	NULL
    },
    /*
     * Flat class:
     */
    {
    /* extension	   */	(XtPointer) NULL,
    /* transparent_bg      */	True,
    /* default_offset	   */	XtOffsetOf(FlatKeysRec, default_item),
    /* rec_size		   */	sizeof(FlatKeysItemRec),
    /* item_resources      */	item_resources,
    /* num_item_resources  */	XtNumber(item_resources),
    /* required_resources  */	required_resources,
    /* num_required_resources*/	XtNumber(required_resources),

		/*
		 *  See ClassInitialize for procedures
		 */

    },
    {
    /* unused              */	0
    }
};

WidgetClass	flatKeysWidgetClass = (WidgetClass)&flatKeysClassRec;

/**
 ** ClassInitialize()
 **/
static void
ClassInitialize OL_NO_ARGS()
{
		/* Inherit all superclass procedures, but	*/
		/* override a few as well provide some chained	*/
		/* procedures. This scheme saves us from	*/
		/* worrying about putting function pointers	*/
		/* in the wrong class slot if they were		*/
		/* statically declared. It also allows us to	*/
		/* inherit new functions simply be recompiling,	*/
		/* i.e., we don't have to stick XtInheritFoo	*/
		/* into the class slot.				*/
	OlFlatInheritAll(flatKeysWidgetClass);

#undef F
#define F	flatKeysClassRec.flat_class

	F.initialize		= Initialize;
	F.set_values		= SetValues;
	F.change_managed	= ChangeManaged;
	F.draw_item		= DrawItem;
	F.item_dimensions	= ItemDimensions;
	F.item_highlight	= ItemHighlight;
	F.item_initialize	= ItemInitialize;
	F.item_set_values	= ItemSetValues;

	if (OlGetGui() == OL_MOTIF_GUI)
	{
		F.item_location_cursor_dimensions = ItemLocCursorDims;
	}

#undef F
} /* end of ClassInitialize */

/**
 ** ChangeManaged()
 **/
/*ARGSUSED*/
static void
ChangeManaged OLARGLIST((w, items, num_changed))
	OLARG( Widget,		w)
	OLARG( FlatItem *,	items)		/* unused */
	OLGRA( Cardinal,	num_changed)	/* unused */
{
	Layout(w);
} /* end of ChangedManaged */

/**
 ** Initialize()
 **/
/*ARGSUSED*/
static void
Initialize OLARGLIST((request, new, args, num_args))
	OLARG( Widget,			request)	/* unused */
	OLARG( Widget,			new)
	OLARG( ArgList,			args)		/* unused */
	OLGRA( Cardinal *,		num_args)	/* unused */
{
	static Boolean		class_part_initialized = False;
	XrmValue		from, to;


	/*
	 * The following would be in the ClassPartInitialize routine,
	 * but for want of a widget ID....
	 */
	if (!class_part_initialized) {
		OlVirtualEventTable	flat_keys_iedb;


		flat_keys_iedb = OlCreateInputEventDB(
			new,
			FlatKeysIEDB, XtNumber(FlatKeysIEDB),
			(OlKeyOrBtnRec *)0, 0
		);
		OlClassSearchIEDB (flatKeysWidgetClass, flat_keys_iedb);
		class_part_initialized = True;
	}


#define WARN(R,V) \
	Dm__VaPrintMsg(TXT_invalidResource_setToSomething, XtName(new), \
			XtClass(new)->core_class.class_name, (R), (V))

	/*
	 * We can't handle but a single column layout.
	 */
	if (ROWCOLUMN_P(new).layout_type != OL_FIXEDCOLS) {
		WARN (XtNlayoutType, "OL_FIXEDCOLS");
		ROWCOLUMN_P(new).layout_type = OL_FIXEDCOLS;
	}
	if (ROWCOLUMN_P(new).measure != 1) {
		WARN (XtNmeasure, "1");
		ROWCOLUMN_P(new).measure = 1;
	}
	if (ROWCOLUMN_P(new).same_width != OL_COLUMNS) {
		WARN (XtNsameWidth, "OL_COLUMNS");
		ROWCOLUMN_P(new).same_width = OL_COLUMNS;
	}
	if (ROWCOLUMN_P(new).same_height != OL_NONE) {
		WARN (XtNsameHeight, "OL_NONE");
		ROWCOLUMN_P(new).same_height = OL_NONE;
	}

	/*
	 * Get the modifier names from the resource database,
	 * if we haven't yet.
	 */
	if (!resource_base)
		resource_base = _OlGetAppAttributesRef(new);

	/*
	 * Fetch the modifiers map for this display. This will be
	 * used in the "KeyHandler()" routine to figure out if a
	 * KeyPress is a modifier.
	 */
	KEYS_P(new).modifiers_map = XGetModifierMapping(XtDisplayOfObject(new));

	/*
	 * Initialize things.
	 */
	ROWCOLUMN_P(new).overlap = 0;
	if (DFT_KEYS_ITEM(new).caption)
		DFT_KEYS_ITEM(new).caption =
				Strdup(DFT_KEYS_ITEM(new).caption);
	KEYS_P(new).consume_keycode = 0;
	KEYS_P(new).cb              = OlCreateChangeBar(
	    new, OlContrastingColor(new, CORE_P(new).background_pixel, 25)
	);
	KEYS_P(new).quoting         = OL_NO_ITEM;
	KEYS_P(new).old_quoting     = OL_NO_ITEM;
	KEYS_P(new).old_modifiers   = 0;
	KEYS_P(new).old_keysym      = NoSymbol;
	KEYS_P(new).flags           = 0;
	GetGCs (new);

	if (KEYS_P(new).caption_font == NULL) {
	  from.size = strlen (OL_DEFAULT_FONT_NAME);
	  from.addr = OL_DEFAULT_FONT_NAME;
	  to.size = 0;
	  to.addr = 0;
	  if (XtConvertAndStore (
		new, XtRString, &from, XtRFontStruct, &to
		) == False)
	    Dm__VaPrintMsg(TXT_badFont_defaultOLFont, XtName(new),
				OlWidgetToClassName(new));
	  KEYS_P(new).caption_font = (XFontStruct *) to.addr;
	}

#undef WARN
	return;
} /* Initialize */

/**
 ** Destroy()
 **/
static void
Destroy OLARGLIST((w))
	OLGRA( Widget,			w)
{
	FreeGCs (w);
	if (KEYS_P(w).cb)
		OlDestroyChangeBar (w, KEYS_P(w).cb);
	if (DFT_KEYS_ITEM(w).caption)
		FREE (DFT_KEYS_ITEM(w).caption);
	if (KEYS_P(w).modifiers_map)
		XFreeModifiermap (KEYS_P(w).modifiers_map);

	return;
} /* Destroy */

/**
 ** SetValues()
 **/
/*ARGSUSED*/
static Boolean
SetValues OLARGLIST((current, request, new, args, num_args))
	OLARG( Widget,			current)
	OLARG( Widget,			request)	/* unused */
	OLARG( Widget,			new)
	OLARG( ArgList,			args)		/* unused */
	OLGRA( Cardinal *,		num_args)	/* unused */
{
	Boolean			redisplay	= False;
	Boolean			get_GCs		= False;


#define DIFFERENT(FIELD) (KEYS_P(new).FIELD != KEYS_P(current).FIELD)

	/*
	 * If the XtNallowChangeBars resource has changed, just effect
	 * a redisplay.
	 */
	if (DIFFERENT(allow_change_bars))
		redisplay = True;

	if (new->core.background_pixel != current->core.background_pixel){
		if (KEYS_P(new).cb)
			OlChangeBarSetValues (
				new,
				OlContrastingColor(new, CORE_P(new).background_pixel, 25),
				KEYS_P(new).cb
			);
		get_GCs = True;
	}
	if (
		DIFFERENT(caption_font)
	     || DIFFERENT(caption_font->fid)
	     || PRIMITIVE_P(new).foreground != PRIMITIVE_P(current).foreground
	)
		get_GCs = True;

	if (get_GCs) {
		FreeGCs (new);
		GetGCs (new);
		redisplay = True;
	}

#undef DIFFERENT
	return (redisplay);
} /* SetValues */

/**
 ** Activate()
 **/
/*ARGSUSED*/
static Boolean
Activate OLARGLIST((w, type, data))
	OLARG( Widget,			w)
	OLARG( OlVirtualName,		type)
	OLGRA( XtPointer,		data)	/* unused */
{
	FlatKeysItemRec		item;
	FlatKeysItemPart *	fkip;

	Cardinal		indx;

	Modifiers		modifiers;

	KeySym			keysym;

	Boolean			ret;


	switch (type) {

	case OL_UNDO:
		indx      = KEYS_P(w).old_quoting;
		modifiers = KEYS_P(w).old_modifiers;
		keysym    = KEYS_P(w).old_keysym;
		goto ChangeKey;

	case OL_DELCHARBAK:
	case OL_DELCHARFWD:
	case OL_DELWORDBAK:
	case OL_DELWORDFWD:
		/*
		 * Generally this sort of thing should be implemented
		 * in the ItemActivate method, but the super-class is the
		 * one who calls ItemActivate from its Activate method....
		 * but we've usurped the default Activate method with
		 * this one, so we have to do this here.
		 */
		indx      = FLAT_P(w).focus_item;
		modifiers = 0;
		keysym    = 0;
ChangeKey:
		if (
			indx != OL_NO_ITEM
		     && ClientSaysOk (
				w,
				indx,
				modifiers,
				keysym,
				(type == OL_UNDO?
					OL_FLATKEY_UNDONE
				      : OL_FLATKEY_DELETED)
			)
		) {
			OlFlatExpandItem (w, indx, (FlatItem)&item);
			fkip = &KEYS_IP(&item);

			OlVaFlatSetValues (
				w,
				indx,
				XtNmodifiers, (XtArgVal)modifiers,
				XtNkeysym,    (XtArgVal)keysym,
				(String)0
			);

			/*
			 * Note: The set-values above does not affect the
			 * values referenced by "fkip", as the latter
			 * values are copies. Thus the following works.
			 */
			KEYS_P(w).old_quoting   = indx;
			KEYS_P(w).old_modifiers = fkip->modifiers;
			KEYS_P(w).old_keysym    = fkip->keysym;
		}
		ret = True;
		break;

	default:
		ret = False;
		break;
	}

	return (ret);
} /* Activate */

/**
 ** DrawItem()
 **/
static void
DrawItem OLARGLIST((w, item, dii))
	OLARG( Widget,			w)
	OLARG( FlatItem,		item)
	OLGRA( OlFlatDrawInfo *,	dii)
{
	int			j;	/* not Cardinal! */

	Modifiers		modifiers;

	KeySym			keysym;

	OlgLabelProc		drawProc;

	OlgAttrs *		item_attrs;
	OlgTextLbl *		lbl;
	OlgTextLbl		lbl2;

	Position		x;

	Dimension		width;
	Dimension		height;

	OlFlatDrawInfo		DI;
	OlFlatDrawInfo *	di = &DI;


	if (!FLAT_IP(item).mapped_when_managed)
		return;

	DI = *dii;

	if ( OlGetGui() == OL_MOTIF_GUI )
	{
#define THICKNESS ((PrimitiveWidget)w)->primitive.highlight_thickness
#define SUPERCLASS \
	  ((FlatKeysClassRec *)flatKeysClassRec.core_class.superclass)

		Dimension	h2 = 2 * THICKNESS;

		if ( FLAT_P(w).focus_item == item->flat.item_index)
			(*SUPERCLASS->flat_class.draw_item)(w, item, di);

		di->x += THICKNESS;
		di->y += THICKNESS;
		di->width  -= h2;
		di->height -= h2;

#undef THICKNESS
#undef SUPERCLASS
	}

	lbl2.mnemonic    = 0;
	lbl2.accelerator = 0;
	lbl2.flags       = 0;
	lbl2.font        = KEYS_P(w).caption_font;
	lbl2.font_list   = PRIMITIVE_P(w).font_list;
	
	/*
	 * We like nicely shaped ``keys''. For instance,
	 * we don't like keys that are narrower then they
	 * are tall. See SizeKey() for details.
	 */
#define SIZEIT(LBL,WIDTH) \
	SizeKey (di->screen, item_attrs, (LBL), &WIDTH, &height)

#define DRAWIT(LBL,X,WIDTH) \
	DrawKey (di->screen, di->drawable, item_attrs,			\
			   (X), di->y, (WIDTH), di->height, (LBL))


	/*
	 * Maybe draw the change bar, maybe a caption.
	 */
	if (KEYS_P(w).allow_change_bars)
		OlDrawChangeBar (
			w,
			KEYS_P(w).cb,
			KEYS_IP(item).change_bar,
			False,
			di->x,
			di->y,
			(Region)0
		);
	if (KEYS_IP(item).caption && *KEYS_IP(item).caption)
	{
		lbl2.label	 = KEYS_IP(item).caption;
		
		DrawText (
			w,
			&lbl2,
			KEYS_P(w).caption_GC,
			(KEYS_IP(item).is_header? SouthEastGravity : EastGravity),
			di->x,
			di->y,
			KEYS_P(w).caption_width - KEYS_P(w).caption_gap,
			di->height
		);
	}

	if (KEYS_IP(item).is_header) {
		if (FLAT_IP(item).label && *FLAT_IP(item).label)
		{
			lbl2.label	= FLAT_IP(item).label;

			DrawText (
				w,
				&lbl2,
				KEYS_P(w).caption_GC,
				SouthGravity,
				di->x + KEYS_P(w).caption_width,
				di->y,
				KEYS_P(w).modifiers_width + KEYS_P(w).detail_width,
				di->height
			);
		}

	} else {
		OlFlatSetupAttributes (
			w,
			item,
			di,
			&item_attrs,
			(XtPointer *)&lbl,
			&drawProc
		);

		lbl->mnemonic    = 0;
		lbl->accelerator = 0;
		lbl->flags	 = 0;

		modifiers = KEYS_IP(item).modifiers;
		keysym	  = KEYS_IP(item).keysym;

		x = di->x + KEYS_P(w).caption_width + KEYS_P(w).modifiers_width;

		/*
		 * Draw a ``key'' for the detail. Yes, this is done before
		 * drawing the modifiers...doing it now makes it easier
		 * to do the bookkeeping of "x".
		 */
		lbl->label = KeysymToString(keysym);
		SIZEIT (lbl, width);
		DRAWIT (lbl, x, width);

		/*
		 * Draw a ``key'' for each modifier (mask). Do this
		 * right-to-left, so that we can start at the alignment
		 * position.
		 */
		for (j = NMASKS-1; j >= 0; j--) {
			if (modifiers & MASKMASK(j)) {
				lbl->label = MASKNAME(j);
				SIZEIT (lbl, width);
				x -= width;
				DRAWIT (lbl, x, width);
			}
		}
	}

	return;
} /* DrawItem */

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
	Dimension		height;
	Dimension		caption_width;
	Dimension		modifiers_width;
	Dimension		detail_width;


	SizeFlatKey (
		w,
		item,
		&caption_width,
		&modifiers_width,
		&detail_width,
		&height
	);

	if (KEYS_P(w).flags & _FLATKEYS_SET_MAX_WIDTHS) {
		if (KEYS_IP(item).is_header) {
			int			delta;
			int			tmp;

			/*
			 * Center the header over the combined modifiers
			 * and details of the real (non-header) flat keys.
			 *
			 * Our caller should have made two passes, the
			 * first for non-header sub-objects, the second
			 * for just headers. Thus, the total sub-part
			 * widths so far reflect optimum layout for
			 * real FlatKeys.
			 *
			 * Note: The "SizeFlatKey()" routine put the
			 * entire header width in "modifiers_width".
			 */
			delta = (
				  (int)modifiers_width
				- (int)KEYS_P(w).modifiers_width 
				- (int)KEYS_P(w).detail_width
			) / 2;

#define SETWIDTH(W) \
	tmp = (int)KEYS_P(w).W + delta, W = (Dimension)(tmp < 0? 0 : tmp)

			SETWIDTH (modifiers_width);
			SETWIDTH (detail_width);

#undef	SETWIDTH
		}

		SETMAX (KEYS_P(w).caption_width, caption_width);
		SETMAX (KEYS_P(w).detail_width, detail_width);

		/*
		 * While ``quoting'' a FlatKey, we let the modifiers
		 * overlap the caption (but not the change bar).
		 * This has the effect of needing less room for the
		 * modifiers--thus the subtraction.
		 */
		if (
			KEYS_P(w).quoting == FLAT_IP(item).item_index
		     && !KEYS_IP(item).is_header
		) {
			int			wid;

			wid = modifiers_width - KEYS_P(w).caption_width;
			if (KEYS_P(w).allow_change_bars)
				wid += OlChangeBarSpan(KEYS_P(w).cb);

			modifiers_width = (Cardinal)(wid < 0? 0 : wid);
		}
		SETMAX (KEYS_P(w).modifiers_width, modifiers_width);
	} else {
		if (p_width)
			*p_width = KEYS_P(w).caption_width
				 + KEYS_P(w).modifiers_width
				 + KEYS_P(w).detail_width;
		if (p_height)
			*p_height = height;
	}

		/* Add space for Location Cursor */
	if (OlGetGui() == OL_MOTIF_GUI)
	{
#define THICKNESS ((PrimitiveWidget)w)->primitive.highlight_thickness

		Dimension	h2 = 2 * THICKNESS;

		*p_width += h2;
		*p_height += h2;

#undef THICKNESS
	}

	return;
} /* ItemDimensions */

/**
 ** ItemHighlight()
 **/
static void
ItemHighlight OLARGLIST((w, item, type))
	OLARG( Widget,			w)
	OLARG( FlatItem,		item)
	OLGRA( OlDefine,		type)
{
	Cardinal		indx      = KEYS_P(w).quoting;


	/*
	 * If we're in the process of quoting a key and we lose
	 * focus, terminate (undo) the quoting.
	 */
	if (
		type == OL_OUT
	     && indx != OL_NO_ITEM
	     && indx == FLAT_IP(item).item_index
	) {
		Modifiers	modifiers = KEYS_P(w).old_modifiers;
		KeySym		keysym    = KEYS_P(w).old_keysym;

		ClientSaysOk (w, indx, modifiers, keysym, OL_FLATKEY_ABORTED);
		UngrabStuff (w, CurrentTime);
		OlVaFlatSetValues (
			w,
			indx,
			XtNmodifiers, (XtArgVal)modifiers,
			XtNkeysym,    (XtArgVal)keysym,
			(String)0
		);
		KEYS_P(w).quoting = KEYS_P(w).old_quoting = OL_NO_ITEM;
	}

	/*
	 * Wrap our superclass method around us.
	 */

#define SUPERCLASS \
	((FlatClassRec *)flatKeysClassRec.core_class.superclass)

	(*SUPERCLASS->flat_class.item_highlight) (w, item, type);

	return;
} /* ItemHighlight */

/**
 ** ItemInitialize()
 **/
/*ARGSUSED*/
static void
ItemInitialize OLARGLIST((w, request, new, args, num_args))
	OLARG( Widget,			w)
	OLARG( FlatItem,		request)	/* unused */
	OLARG( FlatItem,		new)
	OLARG( ArgList,			args)		/* unused */
	OLGRA( Cardinal *,		num_args)	/* unused */
{
	if (KEYS_IP(new).is_header)
		FLAT_IP(new).traversal_on = False;
	else
		CheckLabelAndKeysym (w, new);

	return;
} /* ItemInitialize */

/**
 ** ItemSetValues()
 **/
/*ARGSUSED*/
static Boolean
ItemSetValues OLARGLIST((w, current, request, new, args, num_args))
	OLARG( Widget,			w)
	OLARG( FlatItem,		current)
	OLARG( FlatItem,		request)	/* unused */
	OLARG( FlatItem,		new)
	OLARG( ArgList,			args)		/* unused */
	OLGRA( Cardinal *,		num_args)	/* unused */
{
	Cardinal		item_index	= new->flat.item_index;

	Boolean			redisplay	= False;


#define DIFFERENT(FIELD) (KEYS_IP(new).FIELD != KEYS_IP(current).FIELD)

	/*
	 * If this item used to be a header, but now is not
	 * (or vice versa), then the traversal is the opposite
	 * of what it used to be.
	 */
	if (DIFFERENT(is_header))
		FLAT_IP(new).traversal_on = !FLAT_IP(current).traversal_on;

	if (!KEYS_IP(new).is_header) {
		if (DIFFERENT(keysym) || DIFFERENT(modifiers)) {
			if (FLAT_IP(current).label)
				FREE (FLAT_IP(current).label);
			FLAT_IP(new).label     = 0;
			FLAT_IP(current).label = 0;
		} else if (FLAT_IP(new).label != FLAT_IP(current).label) {
			if (FLAT_IP(current).label)
				FREE (FLAT_IP(current).label);
			KEYS_IP(new).keysym    = NoSymbol;
			KEYS_IP(new).modifiers = 0;
		}
		CheckLabelAndKeysym (w, new);

		OlFlatItemDimensions(
			w, new, &new->flat.width, &new->flat.height);
	}

	if (
		DIFFERENT(is_header)
	     || DIFFERENT(change_bar)
	     || DIFFERENT(caption)
	     || DIFFERENT(keysym)
	     || DIFFERENT(modifiers)
	) {
		/*
		 * The change in this item's parts may require
		 * realignment of all items in the same column,
		 * even though the overall width doesn't change.
		 * The normal "Layout()" routine won't catch this,
		 * so we have to check it here. If we decide to
		 * realign, we just ping each item in the column
		 * and have the Flat super-class take care of the
		 * rest.
		 *
		 * MORE: Currently we assume single-column layout.
		 */

		Dimension	caption_width   = KEYS_P(w).caption_width;
		Dimension	detail_width    = KEYS_P(w).detail_width;
		Dimension	modifiers_width = KEYS_P(w).modifiers_width;
		Dimension	old_total	= caption_width
						+ detail_width
						+ modifiers_width;
		Dimension	new_total;
		Boolean		concerned;

		FlatKeysLayout (w);
		new_total = KEYS_P(w).caption_width
			  + KEYS_P(w).detail_width
			  + KEYS_P(w).modifiers_width;

		/*
		 * The normal "Layout()" routine will pick up the
		 * need to realign, depending on the layout type.
		 */
		switch (ROWCOLUMN_P(w).layout_width) {
		case (int)OL_IGNORE:
		case OL_MAXIMIZE:
			concerned = (new_total <= old_total);
			break;
		case OL_MINIMIZE:
			concerned = (new_total == old_total);
			break;
		}
#define REALIGN(F)	(F > KEYS_P(w).F)
		if (
			concerned
		     &&	(
				REALIGN(caption_width)
			     || REALIGN(detail_width)
			     || REALIGN(modifiers_width)
			)
		) {
			Cardinal		i;
			Cardinal		n = FLAT_P(w).num_items;

			/*
			 * Ping all items in this column save the one
			 * at hand--it's pinged when we return "True".
			 */
			for (i = 0; i < n; i++)
				if (i != item_index)
					OlFlatRefreshItem(w, i, True);
		}
		redisplay = True;
#undef	REALIGN
	}

#undef	DIFFERENT

	return (redisplay);
} /* ItemSetValues */

/**
 ** Layout()
 **/
static void
Layout OLARGLIST((w))
	OLGRA( Widget,			w)
{
	FlatKeysItemRec		item;

	Cardinal		i;


	/*
	 * While ``quoting'' a FlatKey, don't do any relayout
	 * unless necessary. Since we allow modifiers to overlap the
	 * caption, a small increase in the modifiers width may
	 * not require more room.
	 */
	if (KEYS_P(w).quoting != OL_NO_ITEM) {
		Boolean			relayout = False;

		/*
		 * Note: While ``quoting'' we assume the headers
		 * (if any) already fit.
		 */
	 	for (i = 0; i < FLAT_P(w).num_items; ++i) {
			OlFlatExpandItem (w, i, (FlatItem)&item);
			if (!SizeFlatKeyCheck(w, (FlatItem)&item)) {
				relayout = True;
				break;
			}
		}
		if (!relayout)
			return;
	}

	/*
	 * Let the super-class do most of the work here. But first,
	 * we step through each of the items, calling the "ItemDimensions"
	 * routine with the _FLATKEYS_SET_MAX_WIDTHS flag set. This tells
	 * the "ItemDimensions" routine to update the maximum sub-part
	 * widths. Then we call the super-class' layout routine; it will
	 * likewise call "ItemDimensions" for each item, but we'll have
	 * cleared the _FLATKEYS_SET_MAX_WIDTHS flag, so the
	 * "ItemDimensions" routine will give the same size for each item:
	 * the sum of the maximum sub-part widths. This will fool the
	 * super-class into thinking each item has the same size.
	 *
	 * WARNING: This is presently a pretty dumb calculation,
	 * that relies on a single column layout!
	 *
	 * Note: This large comment refers mainly to code inside
	 * "FlatKeysLayout()".
	 */

	FlatKeysLayout (w);
	(*((FlatWidgetClass)XtSuperclass(w))->flat_class.change_managed) (
		w,
		(FlatItem *) NULL,
		(Cardinal) 0
	);

	return;
} /* Layout */

/**
 ** FlatKeysLayout()
 **/
static void
FlatKeysLayout OLARGLIST((w))
	OLGRA( Widget,			w)
{
	FlatKeysItemRec		item;

	Cardinal		i;


	KEYS_P(w).flags |= _FLATKEYS_SET_MAX_WIDTHS;

	KEYS_P(w).caption_width   =
	KEYS_P(w).detail_width    =
	KEYS_P(w).modifiers_width = 0;

 	for (i = 0; i < FLAT_P(w).num_items; ++i) {
		Dimension		width;
		Dimension		height;

		/*
		 * First get the maximum of the sub-parts for non-headers.
		 */
		OlFlatExpandItem (w, i, (FlatItem)&item);
		if (!KEYS_IP(&item).is_header)
			OlFlatItemDimensions (w, (FlatItem)&item, &width, &height);
	}
 	for (i = 0; i < FLAT_P(w).num_items; ++i) {
		Dimension		width;
		Dimension		height;

		/*
		 * Now get the maximum of the sub-parts for the headers.
		 * We have to do this in a second pass because the
		 * width of ``sub-parts'' for headers depends on the
		 * space available so far.
		 */
		OlFlatExpandItem (w, i, (FlatItem)&item);
		if (KEYS_IP(&item).is_header)
			OlFlatItemDimensions (w, (FlatItem)&item, &width, &height);
	}

	KEYS_P(w).flags &= ~_FLATKEYS_SET_MAX_WIDTHS;
	return;
} /* FlatKeysLayout */

/**
 ** MappingHandler()
 **/
static void
MappingHandler OLARGLIST((w, ve))
	OLARG( Widget,			w)
	OLGRA( OlVirtualEvent,		ve)
{
	if (KEYS_P(w).modifiers_map)
		XFreeModifiermap (KEYS_P(w).modifiers_map);
	KEYS_P(w).modifiers_map = XGetModifierMapping(XtDisplayOfObject(w));
	ve->consumed = True;
	return;
} /* MappingHandler */

/**
 ** KeyHandler()
 **/
static void
KeyHandler OLARGLIST((w, ve))
	OLARG( Widget,			w)
	OLGRA( OlVirtualEvent,		ve)
{
	FlatKeysItemRec		item;
	FlatKeysItemPart *	fkip;

	XKeyEvent *		xkey = (XKeyEvent *)ve->xevent;

	Cardinal		indx = FLAT_P(w).focus_item;


	if (indx == OL_NO_ITEM)
		return;


	OlFlatExpandItem (w, indx, (FlatItem)&item);
	fkip = &KEYS_IP(&item);


	/*
	 * The "keysym" field in the OlVirtualEvent structure isn't
	 * set on a KeyRelease.
	 * MORE: Is this a bug?
	 */
	if (xkey->type == KeyRelease) {
		XComposeStatus		s;
		char			buf[1];
		XLookupString (xkey, buf, 1, &(ve->keysym), &s);
	}

	/*
	 * The KeyPress that tells us to start or end ``quoting''
	 * has a corresponding KeyRelease that we have to ignore.
	 * To be safe, we also ignore any other KeyPresses or KeyReleases
	 * until we see the matching KeyRelease.
	 */
	if (KEYS_P(w).consume_keycode) {
		if (KEYS_P(w).consume_keycode == xkey->keycode)
			KEYS_P(w).consume_keycode = 0;
		ve->consumed = True;

	/*
	 * If we're not currently ``quoting'' this FlatKey, then
	 * the only events of interest are those that get us into
	 * ``quoting'' mode.
	 */
	} else if (indx != KEYS_P(w).quoting) {

		if (IsQuotingKey(ve) && GrabStuff(w, xkey->time)) {
			if (ClientSaysOk(
				w,
				indx,
				(Modifiers)0,
				(KeySym)NoSymbol,
				OL_FLATKEY_CHANGING
			)) {

				/*
				 * Start quoting. We ``collect'' the
				 * modifier bits and the detail right
				 * in the sub-object. We save the original
				 * value for a single-level ``undo''.
				 */
				KEYS_P(w).old_modifiers = fkip->modifiers;
				KEYS_P(w).old_keysym    = fkip->keysym;
				KEYS_P(w).quoting       =
				KEYS_P(w).old_quoting   = indx;
				OlVaFlatSetValues (
					w,
					indx,
					XtNmodifiers, (XtArgVal)0,
					XtNkeysym,    (XtArgVal)NoSymbol,
					(String)0
				);

				/*
				 * We have to absorb the KeyRelease
				 * corresponding to this KeyPress, and
				 * not consider it be the detail to be
				 * quoted for this FlatKey.
				 */
				KEYS_P(w).consume_keycode = xkey->keycode;
			}
			ve->consumed = True;
		}

	/*
	 * If we're quoting and only a modifier key was pressed
	 * or released, update the visual feedback.
	 */
	} else if (IsModifierKey(ve->keysym)) {
		Modifiers		modifiers;

		modifiers = KeycodeToModifier(
			KEYS_P(w).modifiers_map, xkey->keycode
		);
		if (xkey->type == KeyPress)
			modifiers = fkip->modifiers | modifiers;
		else
			modifiers = fkip->modifiers & ~modifiers;
		OlVaFlatSetValues (
			w,
			indx,
			XtNmodifiers, (XtArgVal)modifiers,
			(String)0
		);
		ve->consumed = True;

	/*
	 * If the detail came in, update the visual immediately
	 * but wait for the KeyRelease before ending the ``quoting''
	 * mode.
	 */
	} else if (xkey->type == KeyPress) {
		KeySym			keysym;
		Modifiers		modifiers = fkip->modifiers;
		KeyCode			k;


		keysym = _OlCanonicalKeysym(
			XtDisplayOfObject(w),
			ve->keysym,
			&k,
			&modifiers
		);

		if (ClientSaysOk(w, indx, modifiers, keysym, OL_FLATKEY_CHANGED)) {
			/*
			 * We have to clear the ``quoting'' flag before
			 * calling the set-values routine, otherwise the
			 * layout procedure will allow the modifiers to
			 * overlap the caption.
			 */
			KEYS_P(w).quoting = OL_NO_ITEM;

			UngrabStuff (w, xkey->time);

			OlVaFlatSetValues (
				w,
				indx,
				XtNkeysym,    (XtArgVal)keysym,
				XtNmodifiers, (XtArgVal)modifiers,
				(String)0
			);

			/*
			 * Since we've cleared the ``quoting'' flag, we
			 * need some other way to tell this routine to
			 * ignore the KeyRelease corresponding to this
			 * KeyPress. We can use the ".consume_keycode"
			 * field to remember this.
			 */
			KEYS_P(w).consume_keycode = xkey->keycode;
		}
		ve->consumed = True;
	}

	return;
} /* KeyHandler */

/**
 ** GrabStuff()
 **/
static Boolean
GrabStuff OLARGLIST((w, time))
	OLARG( Widget,			w)
	OLGRA( Time,			time)
{
	Display *		display = XtDisplayOfObject(w);

	Window			window	= XtWindowOfObject(w);


	/*
	 * We have to grab the keyboard (sorry!), because some
	 * clients (like us!) have passive grabs on some of the keys.
	 * We also grab the pointer, to prevent the user from fooling
	 * around with our window or other controls within it.
	 */

	switch (XGrabKeyboard(
		display, window, False, GrabModeAsync, GrabModeAsync, time
	)) {
	case AlreadyGrabbed:
	case GrabInvalidTime:
	case GrabNotViewable:
	case GrabFrozen:
		/*
		 * MORE: Do something interesting here?
		 */
		return (False);
	default:
		break;
	}

	switch (XGrabPointer(
		display, window, False, (unsigned int)0,
		GrabModeAsync, GrabModeAsync, None, None, time
	)) {
	case AlreadyGrabbed:
	case GrabInvalidTime:
	case GrabNotViewable:
	case GrabFrozen:
		/*
		 * MORE: Do something interesting here?
		 */
		XUngrabKeyboard (display, time);
		return (False);
	default:
		break;
	}

	return (True);
} /* GrabStuff */

/**
 ** UngrabStuff()
 **/
static void
UngrabStuff OLARGLIST((w, time))
	OLARG( Widget,			w)
	OLGRA( Time,			time)
{
	Display *		display = XtDisplayOfObject(w);

	XUngrabKeyboard (display, time);
	XUngrabPointer  (display, time);
	return;
} /* UngrabStuff */

/**
 ** KeycodeToModifier()
 **/
static Modifiers
KeycodeToModifier OLARGLIST((map, keycode))
	OLARG( register XModifierKeymap *,	map)
	OLGRA( register int,			keycode)
{
	register Cardinal	j;
	register Cardinal	k;


	for (j = 0; j < NMASKS; j++) {
		register Cardinal	jm = j * map->max_keypermod;

		for (k = 0; k < map->max_keypermod; k++)
			if (map->modifiermap[jm + k] == keycode)
				return (MASKMASK(j));
	}
	return (0);
} /* KeycodeToModifier */

/**
 ** ClientSaysOk()
 **/
static Boolean
ClientSaysOk OLARGLIST((w, indx, modifiers, keysym, change))
	OLARG( Widget,			w)
	OLARG( Cardinal,		indx)
	OLARG( Modifiers,		modifiers)
	OLARG( KeySym,			keysym)
	OLGRA( OlDefine,		change)
{
	OlFlatKeyChanged	cd;


	cd.ok = True;
	if (XtHasCallbacks(w, XtNkeyChanged) == XtCallbackHasSome) {
		cd.index     = indx;
		cd.modifiers = modifiers;
		cd.keysym    = keysym;
		cd.change    = change;
		XtCallCallbacks (w, XtNkeyChanged, &cd);
	}
	return (cd.ok);
} /* ClientSaysOk */

/**
 ** ButtonHandler()
 **/
static void
ButtonHandler OLARGLIST((w, ve))
	OLARG( Widget,			w)
	OLGRA( OlVirtualEvent,		ve)
{
	FlatKeysItemRec		item;

	XButtonEvent *		xbutton	= (XButtonEvent *)ve->xevent;

	Cardinal		indx;


	indx = ve->item_index;

	if (indx != OL_NO_ITEM && ve->virtual_name == OL_SELECT) {
		ve->consumed = True;

		/*
		 * Can't move focus if in the middle of quoting a FlatKey;
		 * move it on button down, to give immediate feedback;
		 * and don't bother doing anything if the focus is already
		 * there (avoids unnecessary screen update).
		 */
		if (
			KEYS_P(w).quoting == OL_NO_ITEM
		     && ve->xevent->type == ButtonPress
		     && FLAT_P(w).focus_item != indx
		) {
			OlFlatExpandItem(w, indx, (FlatItem)&item);
			if (item.flat.traversal_on == True)
				OlFlatCallAcceptFocus (
					w, indx, xbutton->time
				);
		}
	}

	return;
} /* ButtonHandler */

/**
 ** SizeText()
 **/
static void
SizeText OLARGLIST((w, text, p_width, p_height))
	OLARG( Widget,			w)
	OLARG( OlgTextLbl *,		text)
	OLARG( Dimension *,		p_width)
	OLGRA( Dimension *,		p_height)
{
	String			p;
	String			newl;

	Cardinal		len	= Strlen(text->label);

	Dimension		width;
	Dimension		height;

	int			dir;
	int			ascent;
	int			descent;

	XCharStruct		overall;


	*p_width = *p_height = 0;

	/* I18N */
	height = OlFontHeight(text->font, text->font_list);

	for (p = text->label; p; p = newl) {
		if ((newl = strchr(p, '\n'))) {
			len = newl - p;
			newl++;
		} else
			len = Strlen(p);

		/* I18N */
		width = (text->font_list) ?
		    OlTextWidth(text->font_list, (unsigned char *)p, len) :
			XTextWidth(text->font, p, len);

		_OlAssignMax(*p_width, width);
		*p_height += height;
	}

	return;
} /* SizeText */

/**
 ** DrawText()
 **/
static void
DrawText OLARGLIST((w, text, gc, gravity, bb_x, bb_y, bb_width, bb_height))
	OLARG( Widget,			w)
	OLARG( OlgTextLbl *,		text)
	OLARG( GC,			gc)
	OLARG( int,			gravity)
	OLARG( Position,		bb_x)
	OLARG( Position,		bb_y)
	OLARG( Dimension,		bb_width)
	OLGRA( Dimension,		bb_height)
{
	String			p;
	String			newl;
	Cardinal		len	= Strlen(text->label);
	Position		y;
	Dimension		width;
	Dimension		height;
	Dimension		total_height;
	int			delta_x;

	/* I18N */
	/* can't use OlgSizeTextLabel because we need the ascent. */

	height = OlFontHeight(text->font, text->font_list);

	total_height = 0;
	for (p = text->label; p; p = newl) {
		if ((newl = strchr(p, '\n')))
			newl++;
		total_height += height;
	}

	switch (gravity) {
	case NorthEastGravity:
	case NorthGravity:
	case NorthWestGravity:
		y = bb_y;
		break;
	case EastGravity:
	case CenterGravity:
	case WestGravity:
		y = bb_y + ((int)bb_height - (int)total_height) / 2;
		break;
	case SouthEastGravity:
	case SouthGravity:
	case SouthWestGravity:
	default:
		y = bb_y + ((int)bb_height - (int)total_height);
		break;
	}

	for (p = text->label; p; p = newl) {
		if ((newl = strchr(p, '\n'))) {
			len = newl - p;
			newl++;
		} else
			len = Strlen(p);

		/* I18N */

		width = (text->font_list) ?
		    OlTextWidth(text->font_list, (unsigned char *)p, len) :
			XTextWidth(text->font, p, len);

		switch (gravity) {
		case NorthEastGravity:
		case EastGravity:
		case SouthEastGravity:
			delta_x = (int)bb_width - (int)width;
			break;
		case NorthGravity:
		case CenterGravity:
		case SouthGravity:
			delta_x = ((int)bb_width - (int)width) / 2;
			break;
		case NorthWestGravity:
		case WestGravity:
		case SouthWestGravity:
		default:
			delta_x = 0;
			break;
		}
		if (delta_x < 0)
			delta_x = 0;

		/* I18N */
		
		if (text->font_list)
		{
		    if (len)
			OlDrawImageString(XtDisplayOfObject(w),
					  XtWindowOfObject(w),
					  text->font_list,
					  gc,
					  bb_x + delta_x,
					  y + text->font_list->max_bounds.ascent,
					  (unsigned char *)p,
					  len
					  );
		} else
		{
		    XDrawImageString (XtDisplayOfObject(w),
				      XtWindowOfObject(w),
				      gc,
				      bb_x + delta_x,
				      y + text->font->ascent,
				      p,
				      len
				      );
		}

		y += height;
	}

	return;
} /* DrawText */

/**
 ** SizeFlatKey()
 **/
static Boolean
SizeFlatKey OLARGLIST((w, item, p_caption_width, p_modifiers_width, p_detail_width, p_height))
	OLARG( Widget,			w)
	OLARG( FlatItem,		item)
	OLARG( Dimension *,		p_caption_width)
	OLARG( Dimension *,		p_modifiers_width)
	OLARG( Dimension *,		p_detail_width)
	OLGRA( Dimension *,		p_height)
{
	Screen *		screen	  = XtScreenOfObject(w);

	Modifiers		modifiers;

	KeySym			keysym;

	OlgTextLbl		lbl;

	Dimension		caption_width	= 0;
	Dimension		modifiers_width	= 0;
	Dimension		detail_width	= 0;
	Dimension		overall_height	= 0;
	Dimension		width;
	Dimension		height;

	Cardinal		j;

	lbl.mnemonic    = 0;
	lbl.accelerator = 0;
	lbl.flags       = 0;
	/* I18N */
	lbl.font_list	= PRIMITIVE_P(w).font_list;

	/*
	 * Include the height of the change bar, even if we never
	 * show them in this widget-instance, so that this widget
	 * aligns vertically with a neighboring FlatKeys widget
	 * that is showing change bars.
	 */
	if (KEYS_P(w).allow_change_bars)
		caption_width += OlChangeBarSpan(KEYS_P(w).cb);
	SETMAX (overall_height, OlChangeBarHeight(KEYS_P(w).cb));

	if (KEYS_IP(item).caption && *KEYS_IP(item).caption)
	{
	        lbl.font        = KEYS_P(w).caption_font;
	        lbl.label 	= KEYS_IP(item).caption;

		SizeText (
			w,
			&lbl,
			&width,
			&height
		);
		caption_width += width + KEYS_P(w).caption_gap;
		SETMAX (overall_height, height);
	}
		
	if (KEYS_IP(item).is_header) {
		/*
		 * Just put all of the space for a header in the
		 * ``modifiers'' sub-part for now; our caller will
		 * know what to do with it.
		 */
		modifiers_width = detail_width = 0;
		if (FLAT_IP(item).label && *FLAT_IP(item).label)
		{
		        lbl.font        = KEYS_P(w).caption_font;
		        lbl.label	= FLAT_IP(item).label;

			SizeText (
				w,
				&lbl,
				&modifiers_width,
				&height
			);
			SETMAX (overall_height, height);
		}

	} else {
		int		overlap = 0;

		/*
		 * Calculate the space needed to display ``keys'' for
		 * the current modifiers and the details.
		 */
		lbl.mnemonic    = 0;
		lbl.accelerator = 0;
		lbl.flags	= 0;
		lbl.font        = FLAT_IP(item).font;
		lbl.font_list   = PRIMITIVE_P(w).font_list;

		modifiers = KEYS_IP(item).modifiers;
		keysym	  = KEYS_IP(item).keysym;
		for (j = 0; j < NMASKS; j++)
			if (modifiers & MASKMASK(j)) {
				lbl.label = MASKNAME(j);
				SizeKey (screen, FLAT_P(w).pAttrs,
					 &lbl, &width, &height);
				modifiers_width += width - overlap;
				SETMAX (overall_height, height);

				/*
				 * Fence-post problem solved; We don't
				 * subtract the overlap for the first
				 * modifier (overlap is zero the first
				 * time here).
				 */
				overlap = ROWCOLUMN_P(w).overlap;
			}

		lbl.label = KeysymToString(keysym);
		SizeKey (screen, FLAT_P(w).pAttrs, &lbl, &width, &height);
		detail_width += width - overlap; /*see fence-post above*/
		SETMAX (overall_height, height);
	}

	if (p_caption_width)
		*p_caption_width = caption_width;
	if (p_modifiers_width)
		*p_modifiers_width = modifiers_width;
	if (p_detail_width)
		*p_detail_width = detail_width;
	if (p_height)
		*p_height = overall_height;

	/*
	 * See if there is room for the modifiers and detail inside
	 * the flat widget, using the existing alignment. We may
	 * allow covering up the caption, but not the change bar.
	 */
	width = KEYS_P(w).modifiers_width;
	if (
		KEYS_P(w).quoting == FLAT_IP(item).item_index
	    && !KEYS_IP(item).is_header
	) {
		width += KEYS_P(w).caption_width;
		if (KEYS_P(w).allow_change_bars)
			width -= OlChangeBarSpan(KEYS_P(w).cb);
	}
	if (
		caption_width <= KEYS_P(w).caption_width
	     && modifiers_width <= width
	     && detail_width <= KEYS_P(w).detail_width
	)
		return (True);
	else
		return (False);
} /* SizeFlatKey */

/**
 ** SizeKey()
 **/
static void
SizeKey OLARGLIST((screen, attrs, lbl, p_width, p_height))
	OLARG( Screen *,		screen)
	OLARG( OlgAttrs *,		attrs)
	OLARG( OlgTextLbl *,		lbl)
	OLARG( Dimension *,		p_width)
	OLGRA( Dimension *,		p_height)
{
	OlgSizeTextLabel (screen, attrs, lbl, p_width, p_height);

	/*
	 * Here's a picture to help understand what's going on here
	 * and in DrawKey below. The dots (.) represent ``padding''
	 * around the basic text.
	 *
	 *	/-----------\ .
	 *	|+---------+| .
	 *	||	   || .
	 *	||  Label  ||
	 *	||	   || .
	 *	|+---------+| .
	 *	\-----------/ .
	 *	....	 ....
	 *
	 */
	*p_width  += OlgGetHorizontalStroke(attrs) * 8;
	*p_height += OlgGetVerticalStroke(attrs) * 6;

	/*
	 * We don't want keys that are narrower than their height.
	 * (Real keyboard don't...)
	 */
	if (*p_width < *p_height)
		*p_width = *p_height;

	return;
} /* SizeKey */

/**
 ** DrawKey()
 **/
static void
DrawKey OLARGLIST((screen, drawable, attrs, x, y, width, height, lbl))
	OLARG( Screen *,		screen)
	OLARG( Drawable,		drawable)
	OLARG( OlgAttrs *, 		attrs)
	OLARG( Position,		x)
	OLARG( Position,		y)
	OLARG( Dimension,		width)
	OLARG( Dimension,		height)
	OLGRA( OlgTextLbl *,		lbl)
{
	register _OlgDevice *	pDev = attrs->pDev;

	_OlgDesc *		ul;
	_OlgDesc *		ur;
	_OlgDesc *		ll;
	_OlgDesc *		lr;


	/*
	 * See SizeKey() above for a picture.
	 */

	/*
	 * Draw a 1-stroke thick, rounded-corner box.
	 */
	if (OlgIs3d()) {
		ul = &pDev->rect3UL;
		ur = &pDev->rect3UR;
		ll = &pDev->rect3LL;
		lr = &pDev->rect3LR;
	} else {
		ul = &pDev->rect2UL;
		ur = &pDev->rect2UR;
		ll = &pDev->rect2LL;
		lr = &pDev->rect2LR;
	}
	OlgDrawFilledRBox (screen, drawable, attrs,
			x, y, width, height, ul, ur, ll, lr, FB_UP);
	OlgDrawRBox (screen, drawable, attrs,
			x, y, width, height, ul, ur, ll, lr, FB_UP);

	/*
	 * Inside it, draw a 1-stroke thick, rectangular box.
	 */
	x      += OlgGetHorizontalStroke(attrs);
	y      += OlgGetVerticalStroke(attrs);
	width  -= OlgGetHorizontalStroke(attrs) * 2;
	height -= OlgGetVerticalStroke(attrs) * 2;
	OlgDrawBox (screen, drawable, attrs, x, y, width, height, False);

	if (!lbl)
		return;

	/*
	 * Inside that, draw the label, leaving a 2-stroke space
	 * on either side and a 1-stroke spave above and below.
	 */
	x      += OlgGetHorizontalStroke(attrs) * 3;
	y      += OlgGetVerticalStroke(attrs) * 2;
	width  -= OlgGetHorizontalStroke(attrs) * 6;
	height -= OlgGetVerticalStroke(attrs) * 4;
	OlgDrawTextLabel (screen, drawable, attrs, x, y, width, height, lbl);

	return;
} /* DrawKey */

/**
 ** KeysymToString()
 **/
static String
KeysymToString OLARGLIST((keysym))
	OLGRA( KeySym,			keysym)
{
	static char		ret[] = { 0 , 0 };

	register Cardinal	j;


	if (keysym == NoSymbol)
		return("");

	for (j = 0; j < XtNumber(singlechar_map); j++)
		if (singlechar_map[j].keysym == keysym) {
			ret[0] = singlechar_map[j].single;
			return (ret);
		}
	return (XKeysymToString(keysym));
} /* KeysymToString */

/**
 ** KeyToString()
 **/
static String
KeyToString OLARGLIST((modifiers, keysym))
	OLARG( Modifiers,		modifiers)
	OLGRA( KeySym,			keysym)
{
	Cardinal		len	= 0;
	Cardinal		len_sep;
	Cardinal		i;

	String			detail	= 0;
	String			sep;
	String			ret;


	if (keysym != NoSymbol)
		detail = XKeysymToString(keysym);

	/*
	 * Count the number of bytes needed to form the string,
	 * then allocate that much space (plus one for our friend, the
	 * Terminating Null).
	 */

	len_sep = 0;
	for (i = 0; i < NMASKS; i++)
		if (modifiers & MASKMASK(i)) {
			len += len_sep + Strlen(MASKNAME(i));
			len_sep = 1;
		}
	if (detail)
		len += 1 + Strlen(detail) + 1;

	ret = (String)MALLOC(len + 1);
	*ret = 0;

	/*
	 * Now run through similar code, but copy the pieces this time.
	 */

	sep = 0;
	for (i = 0; i < NMASKS; i++)
		if (modifiers & MASKMASK(i)) {
			if (sep)
				strcat (ret, sep);
			strcat (ret, MASKNAME(i));
			sep = " ";
		}

	if (detail) {
		strcat (ret, "<");
		strcat (ret, detail);
		strcat (ret, ">");
	}

	return (ret);
} /* KeyToString */

/**
 ** StringToKey()
 **/
static void
StringToKey OLARGLIST((w, string, p_modifiers, p_keysym))
	OLARG( Widget,			w)
	OLARG( String,			string)
	OLARG( Modifiers *,		p_modifiers)
	OLGRA( KeySym *,		p_keysym)
{
	XrmValue		from;
	XrmValue		to;


	from.addr = (XtPointer)string;
	from.size = Strlen(string) + 1;
	to.addr   = 0;

	if (!XtCallConverter(
		XtDisplayOfObject(w),
		_OlStringToOlKeyDef,
		(XrmValuePtr)0,
		(Cardinal)0,
		&from,
		&to,
		(XtCacheRef)0
	)) {
		*p_modifiers = 0;
		*p_keysym    = NoSymbol;
	} else {
		*p_modifiers = ((OlKeyDef *)to.addr)->modifier[0];
		*p_keysym    = ((OlKeyDef *)to.addr)->keysym[0];
	}

	return;
} /* StringToKey */

/**
 ** CheckLabelAndKeysym()
 **/
static void
CheckLabelAndKeysym OLARGLIST((w, item))
	OLARG( Widget,			w)
	OLGRA( FlatItem,		item)
{
	Display *		display	   = XtDisplayOfObject(w);

	Cardinal		item_index = item->flat.item_index;


#define WARN(M) \
	Dm__VaPrintMsg(TXT_invalidResource_flatKeys,			\
		XtName(w),						\
		XtClass(w)->core_class.class_name,			\
		item_index,						\
		(M)							\
	)

	/*
	 * Client can give modifiers and detail individually
	 * or as string label that we have to parse and convert.
	 * If both given, they had better match.
	 */
	if (!FLAT_IP(item).label && KEYS_IP(item).keysym == NoSymbol)
		/*EMPTY*/;

	else if (FLAT_IP(item).label) {
		Modifiers		modifiers;
		KeySym			keysym;
		KeyCode			k;

		StringToKey (w, FLAT_IP(item).label, &modifiers, &keysym);
		keysym = _OlCanonicalKeysym(display, keysym, &k, &modifiers);

		if (
			KEYS_IP(item).modifiers
		     && KEYS_IP(item).modifiers != modifiers
		)
			Dm__VaPrintMsg(TXT_warningMsg_badModMatch);
		if (
			KEYS_IP(item).keysym != NoSymbol
		     && KEYS_IP(item).keysym != keysym
		)
			Dm__VaPrintMsg(TXT_warningMsg_badKeyMatch);

		KEYS_IP(item).modifiers = modifiers;
		KEYS_IP(item).keysym    = keysym;

	} else
		FLAT_IP(item).label =
			KeyToString(KEYS_IP(item).modifiers, KEYS_IP(item).keysym);

	return;
} /* CheckLabelAndKeysym */

/**
 ** GetGCs()
 **/
static void
GetGCs OLARGLIST((w))
	OLGRA( Widget,		w)
{
	XGCValues		v;


	/*
	 * WARNING:
	 * Never call this routine if the GC is already in the widget
	 * structure. Call FreeGCss() first if a new GC is needed,
	 * so that the old GC is released.
	 */

	v.foreground = PRIMITIVE_P(w).foreground;
	v.background = w->core.background_pixel;
	v.font       = KEYS_P(w).caption_font->fid;
	KEYS_P(w).caption_GC =
		XtGetGC(w, GCForeground|GCBackground|GCFont, &v);

	return;
} /* GetGCs */

/**
 ** FreeGCs()
 **/
static void
FreeGCs OLARGLIST((w))
	OLGRA( Widget,		w)
{
	if (KEYS_P(w).caption_GC) {
		XtReleaseGC (w, KEYS_P(w).caption_GC);
		KEYS_P(w).caption_GC = 0;
	}
	return;
} /* FreeGCs */

/*
 * ItemLocCursorDims - figure out the place to draw location cursor.
 */
static Boolean
ItemLocCursorDims OLARGLIST((w, item, di))
	OLARG( Widget,			w)
	OLARG( FlatItem,		item)
	OLGRA( OlFlatDrawInfo *,	di)
{
	if (KEYS_P(w).allow_change_bars)
	{
		di->x     += OlChangeBarSpan(KEYS_P(w).cb);
		di->width -= OlChangeBarSpan(KEYS_P(w).cb);
	}

	return(True);
} /* end of ItemLocCursorDims */
