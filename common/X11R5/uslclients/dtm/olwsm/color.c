/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:olwsm/color.c	1.72"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/CompositeP.h>

#include <Xol/OpenLookP.h>
#include <Xol/Form.h>
#include <Xol/Category.h>
#include <Xol/ChangeBar.h>
#include <Xol/ControlAre.h>
#include <Xol/PrimitiveP.h>
#include <Xol/ScrolledWi.h>
#include <Xol/StubP.h>
#include <Xol/TextEditP.h>
#include <Xol/TextEPos.h>
#include <Xol/TextWrap.h>
#include <Xol/FButtons.h>

#include <FSelector.h>

#include <misc.h>
#include <node.h>
#include <list.h>
#include <menu.h>
#include <exclusive.h>
#include <colormenu.h>
#include <property.h>
#include <resource.h>
#include <wsm.h>
#include <WM.h>
#include "error.h"

#include "../Dtm.h"

#if	defined(FACTORY_LIST)
#include <Xol/ConvertersI.h>
#include <Xol/AbbrevButt.h>
#include <Xol/MenuShell.h>
#include <Xol/StaticText.h>
#endif

/*
 * Special types:
 */

typedef enum SelectType {
	SELECT_WORKSPACE,
	SELECT_BACKGROUND,
	SELECT_TEXT
}			SelectType;

typedef enum BorderState {
	VISIBLE,
	INVISIBLE
}			BorderState;

typedef struct ColorDescription {
	int			follows;
	unsigned int		color_flags;
	unsigned int		bw_flags;
	String			color_name;
	XColor			color;
}			ColorDescription;

typedef struct ColorElement {
	XtArgVal		label;
	XtArgVal		set;
	XtArgVal		chip_color;
}			ColorElement;

/*
 * Local functions:
 */

static void		Import OL_ARGS((
	XtPointer		closure
));
static void		Export OL_ARGS((
	XtPointer		closure
));
static void		PopdownCB OL_ARGS((
	Widget			w,
	XtPointer		closure
));
static void		ResetCB OL_ARGS((
	Widget			w,
	XtPointer		closure
));
static void		FactoryCB OL_ARGS((
	Widget			w,
	XtPointer		closure
));
static ApplyReturn *	ApplyCB OL_ARGS((
	Widget			w,
	XtPointer		closure
));
static void		Create OL_ARGS((
	Widget			work,
	XtPointer		closure
));
static void		MakeSample OL_ARGS((
	Widget			parent
));
static void		ColorChoiceCB OL_ARGS((
	Pixel			pixel
));
static void		PseudoColorChoiceCB OL_ARGS((
	XColor			*color,
	int			more
));
static void		BWChoiceCB OL_ARGS((
	Pixel			fg,
	Pixel			bg
));
static void		UpdateElement OL_ARGS((
	Cardinal		element,
	Pixel			pixel,
	Boolean			followersOnly
));
static void		ChangeElementByName OL_ARGS((
	Widget			w,
	Cardinal		element,
	String			name,
	OlDefine		state
));
static void		ChangeElementByPixel OL_ARGS((
	Cardinal		element,
	Pixel			pixel,
	OlDefine		state
));
static void		ChangeElementByColor OL_ARGS((
	Cardinal		element,
	XColor			*color,
	OlDefine		state
));
static void		PropagateResource OL_ARGS((
	String			name,
	XtArgVal		value,
	unsigned int		type
));
static void		PropagateResourceHold OL_ARGS((
	void
));
static void		PropagateResourceResume OL_ARGS((
	void
));
static void		SyncColorMenuCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
static void		SelectElement OL_ARGS((
	Cardinal		element
));
static void		SyncColorMenu OL_ARGS((
	void
));
static void		TouchEH OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XEvent *		pe,
	Boolean *		continue_to_dispatch
));
static void		WindowSelectCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
static void		ButtonSelectCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
static void		WhichTextPart OL_ARGS((
	Widget			w,
	XEvent *		pe
));

#if	defined(FACTORY_LIST)
static void		CreateFactoryList OL_ARGS((
	Widget			parent
));
static void		SwitchToCustomItem OL_ARGS((
	void
));
static void		FactorySelectCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
static void		NextChoiceCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
static Boolean		IsColorLists OL_ARGS((
	String			_input,
	ColorLists **		p_list
));
static Boolean		ParseColorLists OL_ARGS((
	String			_input,
	Cardinal *		p_n,
	ColorList *		store
));
static String		strbal OL_ARGS((
	String			str,
	char			left,
	char			right
));
#endif

/*
 * Global data:
 */

static Arg        	color_args[] = {
      	{XtNvPad,       0},     /* See note below */
      	{XtNvSpace,     0},	/* See note below */
      	{XtNlayoutType, OL_FIXEDCOLS},
      	{XtNsameSize,   OL_NONE},
      	{XtNcenter,     True}
};

static OlDtHelpInfo ColorHelpInfo = {
	NULL, NULL, "DesktopMgr/clrpref.hlp", NULL, NULL
};

Boolean			ColorAllocError;


/* Note: if you change the location of this element you must also */
/* change the index value in CreatePropertyPopup(). */

Property			colorProperty = {
	"Color",
      	color_args,
      	XtNumber (color_args),
	&ColorHelpInfo,
	'\0',
	Import,
	Export,
	Create,
	ApplyCB,
	ResetCB,
	FactoryCB,
	PopdownCB,
	0,
	"Click on the part of the sample window you want to change",
	0,
};

char Msg[BUFSIZ];

/*
 * Convenient macros:
 */

#define ITEM(colormenu, name)	GetColorMenuItem(colormenu, name)
#define PIXEL(x)		colors[x].color.pixel
#define COLOR(x)		colors[x].color_name
#define NAME(x)			(String)items[x].label
#define SELECTED(x)		*((Boolean *)&(items[x].set))
#define RWCOLOR(x)		colors[x].color

#define FontColorWithFocus(FOCUS) \
	((FOCUS == PIXEL(FONTCOLOR))? PIXEL(WINDOWS) : PIXEL(FONTCOLOR))

#define COLORDIFF(a,b)	(((unsigned long)(a.red-b.red)*(a.red-b.red) >> 2) + \
			 ((unsigned long)(a.green-b.green)*(a.green-b.green) >> 2) + \
			 ((unsigned long)(a.blue-b.blue)*(a.blue-b.blue) >> 2))
#define MINCONTRAST	0x300

#define WORKSPACE		0
#define WINDOWS			1
#define INPUT_WINDOW		2
#define INPUT_FOCUS		3
#define TEXT_FOREGROUND		4
#define TEXT_BACKGROUND		5
#define HELP_KEYCOLOR		6
#define NELEMENTS_NAMED		7

#define FOREGROUND		7
#define BORDERCOLOR		8
#define FONTCOLOR		9
#define NELEMENTS	 	10

#define COLORTUPLELIST		10	/* NOT IN ALL LISTS !! */

#define NF			0x0FF	/* None of the above.      */

#define NRWCOLORS	NELEMENTS_NAMED + 3
#define BG0		NELEMENTS_NAMED
#define BG2		NELEMENTS_NAMED + 1
#define BG3		NELEMENTS_NAMED + 2

#define PROPAGATE_CACHE_LIMIT	12

#define PROPAGATE_OTHER	0x001
#define PROPAGATE_TEXT	0x002
#define PROPAGATE_FOCUS	0x004
#define PROPAGATE_ALL	(PROPAGATE_OTHER|PROPAGATE_TEXT|PROPAGATE_FOCUS)

/*
 * Local data:
 */

static Widget		elements;
static Widget		workspace;
static Widget		window;
static Widget		example_text;
static Widget		focus;
static Widget		color_menu;
static void		(*ColorChoice)();
static Pixel		Pixels [NRWCOLORS];
static XColor		BG0Color, BG2Color, BG3Color;
static OlColorTupleList	*RestoreList;
static Boolean		RestoreUseList;

static char		RN_workspace         [] = "dtm.workspace";
static char		RN_Background        [] = "*Background";
static char		RN_inputFocusColor   [] = "*inputFocusColor";
static char		RN_inputWindowHeader [] = "*inputWindowHeader";
static char		RN_TextFontColor     [] = "*TextFontColor";
static char		RN_TextBackground    [] = "*TextBackground";
static char		RN_helpKeyColor      [] = "dtm.helpKeyColor";
static char		RN_foreground        [] = "*foreground";
static char		RN_borderColor       [] = "*borderColor";
static char		RN_FontColor         [] = "*FontColor";
static char		RN_colorTupleList    [] = "*colorTupleList";

static Resource		_current[] = {
	{ RN_workspace         },
	{ RN_Background        },
	{ RN_inputWindowHeader },
	{ RN_inputFocusColor   },
	{ RN_TextFontColor     },
	{ RN_TextBackground    },
	{ RN_helpKeyColor      },
	{ RN_foreground        },
	{ RN_borderColor       },
	{ RN_FontColor         },
};
static List		current = LIST(Resource, _current);

static Resource		_factory[] = {
	{ RN_workspace         },
	{ RN_Background        },
	{ RN_inputWindowHeader },
	{ RN_inputFocusColor   },
	{ RN_TextFontColor     },
	{ RN_TextBackground    },
	{ RN_helpKeyColor      },
	{ RN_foreground        },
	{ RN_borderColor       },
	{ RN_FontColor         },
	{ RN_colorTupleList    },
};
static List		factory = LIST(Resource, _factory);

static ColorDescription	colors[] = {
{ /* WORKSPACE       */               NF, COLOR_WORKSPACE,        BW_BG },
{ /* WINDOWS         */               NF, COLOR_BG1,              BW_BG },
{ /* INPUT_WINDOW    */               NF, COLOR_INPUT_WINDOW,     BW_FG },
{ /* INPUT_FOCUS     */               NF, COLOR_INPUT_FOCUS,      BW_FG },
{ /* TEXT_FOREGROUND */               NF, COLOR_TEXT_BG|CONTRAST, BW_FG },
{ /* TEXT_BACKGROUND */               NF, COLOR_TEXT_BG,          BW_BG },
{ /* HELP_KEYCOLOR   */               NF, COLOR_HELP_FG|CONTRAST, BW_BG },
{ /* FOREGROUND      */ CONTRAST|WINDOWS, COLOR_BG1    |CONTRAST, BW_FG },
{ /* BORDERCOLOR     */ CONTRAST|WINDOWS, COLOR_BG1    |CONTRAST, BW_FG },
{ /* FONTCOLOR       */ CONTRAST|WINDOWS, COLOR_BG1    |CONTRAST, BW_FG },
};

static ColorElement	items[] = {
	{ (XtArgVal)"Workspace"         },
	{ (XtArgVal)"Window Background" },
	{ (XtArgVal)"Input Window"      },
	{ (XtArgVal)"Input Focus"       },
	{ (XtArgVal)"Text Foreground"   },
	{ (XtArgVal)"Text Background"   },
	{ (XtArgVal)"Help Highlighting"   },
};

static String		fields[] = {
	XtNlabel, XtNset, XtNchipColor
};

#define FACTORY(x)		*(ADDR *)&(_factory[x].value)
#define CURRENT(x)		*(ADDR *)&(_current[x].value)
#define GLOBAL(x) \
	resource_value(&global_resources, _factory[x].name)

static struct {
	Cardinal		a;
	Cardinal		b;
}			IllMatch[] = {
	{ INPUT_WINDOW,    WINDOWS         },
	{ TEXT_BACKGROUND, INPUT_FOCUS     },
	{ TEXT_BACKGROUND, TEXT_FOREGROUND },
	{ TEXT_BACKGROUND, TEXT_FOREGROUND },
	{ WINDOWS,	   TEXT_FOREGROUND },
	/* These two items MUST be the last two items
	 * so that they won't be checked for conflict
	 * in a monochrome setting.
     */
	{ HELP_KEYCOLOR, TEXT_FOREGROUND },
	{ HELP_KEYCOLOR, TEXT_BACKGROUND },
};

static Boolean		GoneFullCycle	= False;

/**
 ** Import()
 **/

static void
#if	OlNeedFunctionPrototypes
Import (
	XtPointer		closure
)
#else
Import (closure)
	XtPointer		closure;
#endif
{
	Cardinal		i;


	for (i = 0; i < NELEMENTS; i++)
		_factory[i].value = DefaultColor(
			colors[i].color_flags,
			colors[i].bw_flags
		);
	_factory[COLORTUPLELIST].value = DefaultColorTupleList();
	merge_resources (&global_resources, &factory);
	return;
} /* Import */

/**
 ** Export()
 **/

static void
#if	OlNeedFunctionPrototypes
Export (
	XtPointer		closure
)
#else
Export (closure)
	XtPointer		closure;
#endif
{
	Cardinal		i;
	Cardinal		n;

	Boolean			ill_matched	= False;


	/*
	 * Before accepting the ``colors'' that are now in the
	 * RESOURCE_MANAGER, see if they are reasonable.
	 */
	for (i = 0; i < NELEMENTS; i++)
		PIXEL(i) = StringToPixel(InitShell, GLOBAL(i));

	/* If on monochrome display, don't verify helpKeyColor */
	n = (PlanesOfScreen(DESKTOP_SCREEN(Desktop)) > 1) ?
			XtNumber(IllMatch) : (XtNumber(IllMatch) - 2);

	for (i = 0; i < n; i++)

		if (PIXEL(IllMatch[i].a) == PIXEL(IllMatch[i].b)) {
		        Dm__VaPrintMsg(TXT_footerMsg_sameColors,
				NAME(IllMatch[i].a), NAME(IllMatch[i].b));
			ill_matched = True;
		}

	if (ill_matched) {
		Dm__VaPrintMsg(TXT_warningMsg_poorColors);
		merge_resources (&global_resources, &factory);
	}

	for (i = 0; i < NELEMENTS; i++)
		CURRENT(i) = XtNewString(GLOBAL(i));

	return;
} /* Export */

/**
 ** PopdownCB()
 **/

static void
#if	OlNeedFunctionPrototypes
PopdownCB (
	Widget			w,
	XtPointer		closure
)
#else
PopdownCB (w, closure)
	Widget			w;
	XtPointer		closure;
#endif
{
	Cardinal		i;


	for (i = 0; i < NELEMENTS; i++) {
		if (COLOR(i))
			XtFree (COLOR(i));
		COLOR(i) = 0;
	}

	if (ColorChoice == PseudoColorChoiceCB)
	    XFreeColors (XtDisplay(w), InitShell->core.colormap, Pixels,
			 NRWCOLORS, 0);

	/*
	 * CURRENT(i) and FACTORY(i) are not freed, as they were
	 * permanently allocated in the Import and Export procedures.
	 */
	return;
} /* PopdownCB */

/**
 ** ResetCB()
 **/

static void
#if	OlNeedFunctionPrototypes
ResetCB (
	Widget			w,
	XtPointer		closure
)
#else
ResetCB (w, closure)
	Widget			w;
	XtPointer		closure;
#endif
{
	Cardinal		i;


	/*
	 * Change only the primary elements, the rest follow from
	 * that.
	 */
	PropagateResourceHold ();
	for (i = 0; i < NELEMENTS_NAMED; ++i)
		ChangeElementByName (w, i, CURRENT(i), OL_NONE);
	PropagateResourceResume ();

#if	defined(FACTORY_LIST)
	SwitchToCustomItem ();
#endif

	return;
} /* ResetCB */

/**
 ** FactoryCB()
 **/

static void
#if	OlNeedFunctionPrototypes
FactoryCB (
	Widget			w,
	XtPointer		closure
)
#else
FactoryCB (w, closure)
	Widget			w;
	XtPointer		closure;
#endif
{
	Cardinal		i;


	/*
	 * Change only the primary elements, the rest follow from
	 * that.
	 */
	PropagateResourceHold ();
	for (i = 0; i < NELEMENTS_NAMED; ++i)
		ChangeElementByName (w, i, FACTORY(i), OL_NORMAL);
	PropagateResourceResume ();

#if	defined(FACTORY_LIST)
	/*
	 * MORE: Should coordinate the ``Reset to Factory'' color
	 * with the canned (``factory?'') color combinations. For
	 * now, resetting to factory means trying them out as the
	 * potential custom colors.
	 */
	SwitchToCustomItem ();
#endif

	return;
} /* FactoryCB */

/**
 ** ApplyCB()
 **/

static ApplyReturn *
#if	OlNeedFunctionPrototypes
ApplyCB (
	Widget			w,
	XtPointer		closure
)
#else
ApplyCB(w, closure)
	Widget			w;
	XtPointer		closure;
#endif
{
	Cardinal		i;
	Cardinal		n;
	Display			*dpy = XtDisplay(w);

	String			msg	= 0;

	static char		buf[BUFSIZ];

	static ApplyReturn	ret;


	/*
	 * Check for invalid combinations:
	 */
	n = (PlanesOfScreen(DESKTOP_SCREEN(Desktop)) > 1) ?
			XtNumber(IllMatch) : (XtNumber(IllMatch) - 2);

	for (i = 0; i < NELEMENTS_NAMED; i++)
		XQueryColor (dpy, InitShell->core.colormap, &RWCOLOR(i));

	for (i = 0; i < n; i++)
	{
		/* If two colors are nearly the same, disallow the choice.
		 * The distance measure is the square of the difference, and
		 * the threshold is somewhat arbitrary.
		 */
		if (COLORDIFF(RWCOLOR(IllMatch[i].a),RWCOLOR(IllMatch[i].b)) <
		    MINCONTRAST)
		{
			sprintf (
				(msg = buf),
				Dm__gettxt(TXT_footerMsg_sameColors),
				NAME(IllMatch[i].a),
				NAME(IllMatch[i].b)
			);
			break;
		}
	}

	if (msg) {
		ret.bad_sheet = &colorProperty;
		ret.reason    = APPLY_ERROR;
		ret.u.message = msg;
	} else {
		DmHelpAppPtr	hap;
		Screen		*screen	= XtScreenOfObject(w);
		Pixel		newPixel;

		for (i = 0; i < NELEMENTS; ++i) {
			if (CURRENT(i))
				XtFree (CURRENT(i));
			if (COLOR(i))
			{
				XtFree (COLOR(i));
				COLOR(i) = 0;
			}
			CURRENT(i) = PixelToString(w, PIXEL(i));
			CURRENT(i) = XtNewString(CURRENT(i));

			if (i < NELEMENTS_NAMED)
				_OlFlatSetChangeBarState (
					elements, i, OL_NONE, OL_PROPAGATE
				);
		}
		newPixel = StringToPixel(w, CURRENT(WORKSPACE));
		if (wsm.workspace.pixel != newPixel) {
			XSetWindowBackground (DISPLAY, ROOT, newPixel);
			XClearWindow (DISPLAY, ROOT);
			XSync (DISPLAY, False);
			wsm.workspace.pixel = newPixel;
		}

#if	defined(FACTORY_LIST)
		/*
		 * If the user had picked one of the canned color combos,
		 * and has now applied it, that's the custom color now.
		 */
		SwitchToCustomItem ();
#endif

		merge_resources (&global_resources, &current);
		ret.reason = APPLY_OK;

		/* If on a color display, update highlighting color
		 * for existing help windows.
		 */
		if (PlanesOfScreen(screen) > 1) {
			Pixel	newPixel;

			newPixel = StringToPixel (w, CURRENT(HELP_KEYCOLOR));
			for (hap = DESKTOP_HELP_INFO(Desktop); hap; hap = hap->next) {
				if (hap->hlp_win.shell != NULL) {
					XtVaSetValues(hap->hlp_win.htext, XtNkeyColor,
						      newPixel, NULL);
				}
			}
			hddp->key_color = newPixel;
		}
	}

	return (&ret);
} /* ApplyCB */

/**
 ** Create()
 **/

static void
#if	OlNeedFunctionPrototypes
Create (
	Widget			work,
	XtPointer		closure
)
#else
Create (work, closure)
	Widget			work;
	XtPointer		closure;
#endif
{
	Screen *		screen	= XtScreenOfObject(work);
	Display *		dpy = XtDisplay(work);

	Widget			top;
	Widget			bottom;
	Widget			left;

	Cardinal		i,j;
	int			space_pixel;
	XFontStruct *		fs;

	Colormap		colormap;
	Visual			*visual;



	/* If the colormap has writable pixels, get one cell per item
	 * we can change, plus some for additional ones for bg0, 2, and 3.
	 */
	ColorChoice = ColorChoiceCB;
	ColorAllocError = False;
	colormap = InitShell->core.colormap;
	visual = XDefaultVisualOfScreen(screen);
	if (visual->class & 0x1)
		if (XAllocColorCells(dpy, colormap, False, 0, 0,
				     Pixels, NRWCOLORS) != 0)
		{
			ColorChoice = PseudoColorChoiceCB;
			BG0Color.pixel = Pixels [BG0];
			BG2Color.pixel = Pixels [BG2];
			BG3Color.pixel = Pixels [BG3];
		}
		else
			ColorAllocError = True;

	for (i = 0; i < NELEMENTS; i++) {
		if (COLOR(i))
			XtFree (COLOR(i));
		COLOR(i) = XtNewString(CURRENT(i));

		/*
		 * This has been done in Export().
		 */
/*		PIXEL(i) = StringToPixel(work, CURRENT(i));	*/

		if (i < NELEMENTS_NAMED) {
			SELECTED(i) = False;
			if (ColorChoice == ColorChoiceCB)
				items[i].chip_color = (XtArgVal)PIXEL(i);
			else
			{
				items[i].chip_color = (XtArgVal)Pixels[i];
				XParseColor (dpy, colormap, CURRENT(i),
					     &RWCOLOR(i));
				PIXEL(i) = Pixels[i];
				XStoreColor (dpy, colormap, &RWCOLOR(i));
			}
		}
	}

        /* .15 in */
	space_pixel = OlScreenMMToPixel(OL_VERTICAL, 3.81, screen);
	top = XtVaCreateManagedWidget(
		"top",
		controlAreaWidgetClass,
		work,
		XtNshadowThickness,	(XtArgVal) 0,
		XtNvPad,		(XtArgVal) 0,
		XtNvSpace,		(XtArgVal) space_pixel,
		XtNlayoutType,		(XtArgVal) OL_FIXEDCOLS,
		XtNsameSize,		(XtArgVal) OL_NONE,
		(String)0
	);
#if	defined(FACTORY_LIST)
	if (wsm.factory_color_lists)
		CreateFactoryList (top);
#endif
	color_menu = CreateColorMenu(top, ColorChoice, BWChoiceCB);

        /* .25 horizontal inches */
	space_pixel = OlScreenMMToPixel (OL_HORIZONTAL, 6.35, screen);

	bottom = XtVaCreateManagedWidget(
		"bottom",
		controlAreaWidgetClass,
		work,
		XtNshadowThickness,	(XtArgVal) 0,
		XtNvPad,		(XtArgVal) 0,
		XtNhSpace,		(XtArgVal) space_pixel,
		(String)0
	);

	items[0].label = (XtArgVal)OLG(workspace,fixedString);
	items[1].label = (XtArgVal)OLG(windowBG,fixedString);
	items[2].label = (XtArgVal)OLG(inputWindow,fixedString);
	items[3].label = (XtArgVal)OLG(inputFocus,fixedString);
	items[4].label = (XtArgVal)OLG(textFG,fixedString);
	items[5].label = (XtArgVal)OLG(textBG,fixedString);
	items[6].label = (XtArgVal)OLG(helpKeyClr,fixedString);

        space_pixel = OlScreenPointToPixel (OL_VERTICAL, 20, screen);
	
	XtVaGetValues(XtParent(work),
		      XtNcategoryFont, &fs,
		      (String) 0);

	elements = XtVaCreateManagedWidget(
		"elements",
		flatSelectorWidgetClass,
		bottom,
		XtNfont,		(XtArgVal)fs,			   
		XtNnoneSet,		(XtArgVal)True,
		XtNexclusives,		(XtArgVal)True,
		XtNitems,		(XtArgVal)items,
		XtNnumItems,		(XtArgVal) XtNumber(items),
		XtNitemFields,		(XtArgVal)fields,
		XtNnumItemFields,	(XtArgVal)XtNumber(fields),
		XtNlayoutType,		(XtArgVal)OL_FIXEDCOLS,
		XtNitemMinHeight,	(XtArgVal)space_pixel,
		XtNallowChangeBars,	(XtArgVal)True,
		(String)0
	);
	if (PlanesOfScreen(screen) > 1 && !ColorAllocError)
		XtVaSetValues (
			elements,
			XtNselectProc,   (XtArgVal)SyncColorMenuCB,
			XtNunselectProc, (XtArgVal)SyncColorMenuCB,
			(String)0
		);
	else
		XtSetSensitive (elements, False);

	if (ColorAllocError)
		colorProperty.footer = Dm__gettxt(TXT_footerMsg_colorError);
	else
		colorProperty.footer = Dm__gettxt(TXT_footerMsg_colorStart);

	MakeSample (bottom);

	PropagateResourceHold ();
	for (i = 0; i < NELEMENTS_NAMED; ++i)
		ChangeElementByName (work, i, CURRENT(i), OL_NONE);
	PropagateResourceResume ();

	SyncColorMenu ();

	return;
} /* Create */

static char *button_fields[] = {
	XtNlabel,
	XtNselectProc
};

typedef struct _button_items {
	char *	label;
	void	(*select)();
} _button_items;

static _button_items _button1[] = {
	"File", ButtonSelectCB
};

static _button_items _button234[] = {
	{ "Edit", ButtonSelectCB },
	{ "View", ButtonSelectCB },
	{ "Help", ButtonSelectCB }
};

/**
 ** MakeSample()
 **/

static void
#if	OlNeedFunctionPrototypes
MakeSample (
	Widget			parent
)
#else
MakeSample (parent)
	Widget			parent;
#endif
{
 	int			space_pixel;
	int			h_dist;
	int			v_dist;
	Screen *		screen	= XtScreenOfObject(parent);

	Widget			client;
	Widget			upper;
	Widget			sw;
	Widget			w;

	Cardinal		i;
	Display *		dpy;


	/*
	 * MORE: Is this right?
	 *
	 * Use an event handler, instead of the XtNconsumeEvent callback,
	 * because the composite widgets don't include Motion events
	 * in their event_proc lists.
	 */
#define ADD_TOUCH_EH(W,D) \
	if (PlanesOfScreen(screen) > 1)					\
	    XtAddEventHandler (						\
		W,							\
		EnterWindowMask|ButtonPressMask|ButtonReleaseMask,	\
		False,							\
		TouchEH,						\
		(XtPointer)(D)						\
	    )

        space_pixel = OlScreenPointToPixel (OL_VERTICAL, 1, screen);
	workspace = XtVaCreateManagedWidget(
		"workspace",
		formWidgetClass,
		parent,
		XtNbackground,	(XtArgVal)PIXEL(WORKSPACE),
		XtNborderWidth, space_pixel,
		(String)0
	);
	ADD_TOUCH_EH (workspace, SELECT_WORKSPACE);

	h_dist = OlScreenMMToPixel (OL_HORIZONTAL, 3, screen);
        v_dist = OlScreenMMToPixel (OL_VERTICAL, 3, screen);
	window = XtVaCreateManagedWidget(
		"window",
		wmWidgetClass,
		workspace,
		XtNxOffset,		(XtArgVal)h_dist,
		XtNyOffset,		(XtArgVal)v_dist,
		XtNxAttachOffset,	(XtArgVal)h_dist,
		XtNyAttachOffset,	(XtArgVal)v_dist,
		(String)0
	);
	if (PlanesOfScreen(screen) > 1)
		XtAddCallback (window, XtNselect, WindowSelectCB, (XtPointer)0);

	client = XtVaCreateManagedWidget(
		"client",
		controlAreaWidgetClass,
		window,
		XtNlayoutType,		(XtArgVal)OL_FIXEDCOLS,
		XtNhPad,		(XtArgVal)0,
		XtNvPad,		(XtArgVal)0,
		XtNvSpace,		(XtArgVal)0,
		XtNshadowThickness,	(XtArgVal)0,
		(String)0
	);
	ADD_TOUCH_EH (client, SELECT_BACKGROUND);
	upper = XtVaCreateManagedWidget(
		"upper",
		controlAreaWidgetClass,
		client,
		XtNhPad,		(XtArgVal)0,
		XtNvPad,		(XtArgVal)2,
		XtNshadowThickness,	(XtArgVal)0,
		(String)0
	);
	ADD_TOUCH_EH (upper, SELECT_BACKGROUND);
	/*
         * The first button is special, for we color it with the
	 * input focus color, as a target for the user to choose
	 * that color. To avoid having button clicks on it being
	 * handled by the parent (as background color), we make
	 * it a widget instead of a gadget.
	 */
	dpy = XtDisplay(upper);
	_button1[0].label = OLG(file,fixedString);
	focus = XtVaCreateManagedWidget(
		"button1",
		flatButtonsWidgetClass,
		upper,
		XtNitemFields,		(XtArgVal)button_fields,
		XtNnumItemFields,	XtNumber(button_fields),
		XtNitems,		(XtArgVal)_button1,
		XtNnumItems,		(XtArgVal)XtNumber(_button1),
		XtNfontColor,		(XtArgVal)
                   FontColorWithFocus(PIXEL(INPUT_FOCUS)),
		XtNbackground,		(XtArgVal)PIXEL(INPUT_FOCUS),
		XtNtraversalOn,		(XtArgVal)False,
		XtNmenubarBehavior,	(XtArgVal)True,
		(String)0
	);
	_button234[0].label = OLG(view,fixedString);
	_button234[1].label = OLG(edit,fixedString);
	_button234[2].label = OLG(help,fixedString);
        w = XtVaCreateManagedWidget(
		"button234",
		flatButtonsWidgetClass,
		upper,
		XtNitemFields,		(XtArgVal)button_fields,
		XtNnumItemFields,	XtNumber(button_fields),
		XtNitems,		(XtArgVal)_button234,
		XtNnumItems,		(XtArgVal)XtNumber(_button234),
		XtNtraversalOn,		(XtArgVal)False,
		XtNmenubarBehavior,	(XtArgVal)True,
		(String)0
	);
	sw = XtVaCreateManagedWidget(
		"scroll",
		scrolledWindowWidgetClass,
		client,
		XtNforceVerticalSB,	(XtArgVal)True,
		(String)0
	);
	XtVaGetValues (sw, XtNvScrollbar, &w, (String)0);
	ADD_TOUCH_EH (sw, SELECT_BACKGROUND);
	ADD_TOUCH_EH (w, SELECT_BACKGROUND);
	/* .25 vertical inches */
	v_dist = OlScreenMMToPixel (OL_VERTICAL, 6.35, screen);
	/* .25 horizontal inches */
	h_dist = OlScreenMMToPixel (OL_HORIZONTAL, 6.35, screen);
	example_text = XtVaCreateManagedWidget(
		"exampleText",
		textEditWidgetClass,
		sw,
		XtNfontColor,		(XtArgVal)PIXEL(TEXT_FOREGROUND),
		XtNbackground,		(XtArgVal)PIXEL(TEXT_BACKGROUND),
		XtNinputFocusColor,	(XtArgVal)PIXEL(INPUT_FOCUS),
		XtNtraversalOn,		(XtArgVal)False,
                XtNleftMargin,		(XtArgVal)h_dist,
		XtNrightMargin,		(XtArgVal)h_dist,
		XtNtopMargin,		(XtArgVal)v_dist,
		XtNbottomMargin,	(XtArgVal)v_dist,
		XtNcharsVisible,	(XtArgVal)25,
		XtNlinesVisible,	(XtArgVal)7,
		XtNeditType,		(XtArgVal)OL_TEXT_READ,
                XtNsource,		(XtArgVal)OLG(useThisArea,footerMsg),
		(String)0
	);
	ADD_TOUCH_EH (example_text, SELECT_TEXT);

#undef	ADD_TOUCH_EH
	return;
} /* MakeSample */

/**
 ** ColorChoiceCB()
 **/

static void
#if	OlNeedFunctionPrototypes
ColorChoiceCB (
	Pixel			pixel
)
#else
ColorChoiceCB (pixel)
	Pixel			pixel;
#endif
{
	Cardinal		i;


	PropagateResourceHold ();
	for (i = 0; i < NELEMENTS_NAMED; i++)
		if (SELECTED(i))
			ChangeElementByPixel (i, pixel, OL_NORMAL);
	PropagateResourceResume ();

	FooterMessage (colorProperty.w, 0, OL_LEFT, False);
	GoneFullCycle = True;

	return;
} /* ColorChoiceCB */

/**
 ** PseudoColorChoiceCB()
 **/

static void
#if	OlNeedFunctionPrototypes
PseudoColorChoiceCB (
	XColor			*color,
	int			more
)
#else
PseudoColorChoiceCB (color, more)
	XColor			*color;
	int			more;
#endif
{
	Cardinal		i;

	for (i = 0; i < NELEMENTS_NAMED; i++)
		if (SELECTED(i))
			ChangeElementByColor (i, color, OL_NORMAL);

	if (!more && SELECTED(WINDOWS))
	    UpdateElement (WINDOWS, PIXEL(WINDOWS), True);

	GoneFullCycle = True;

	return;
} /* PseudoColorChoiceCB */

/**
 ** BWChoiceCB()
 **/

static void
#if	OlNeedFunctionPrototypes
BWChoiceCB (
	Pixel			fg,
	Pixel			bg
)
#else
BWChoiceCB (fg, bg)
	Pixel			fg;
	Pixel			bg;
#endif
{
	Cardinal		i;


	PropagateResourceHold ();
	for (i = 0; i < NELEMENTS_NAMED; i++)
		if (colors[i].bw_flags == BW_FG)
			ChangeElementByPixel (i, fg, OL_NORMAL);
		else
			ChangeElementByPixel (i, bg, OL_NORMAL);
	PropagateResourceResume ();

	return;
} /* BWChoiceCB */

/**
 ** ChangeElementByName()
 **/

static void
#if	OlNeedFunctionPrototypes
ChangeElementByName (
	Widget			w,
	Cardinal		element,
	String			name,
	OlDefine		state
)
#else
ChangeElementByName (w, element, name, state)
	Widget			w;
	Cardinal		element;
	String			name;
	OlDefine		state;
#endif
{
	if (ColorChoice == ColorChoiceCB)
	    ChangeElementByPixel (element, StringToPixel(w, name), state);
	else
	{
	    XParseColor (XtDisplay(w), InitShell->core.colormap, name,
			 &RWCOLOR(element));
	    ChangeElementByColor (element, &RWCOLOR(element), state);
	    UpdateElement (element, PIXEL(element), False);
	    SyncColorMenu ();
	}
	COLOR(element) = XtNewString(name);

	return;
} /* ChangeElementByName */

/**
 ** ChangeElementByColor()
 **/

static void
#if	OlNeedFunctionPrototypes
ChangeElementByColor (
	Cardinal		element,
	XColor			*color,
	OlDefine		state
)
#else
ChangeElementByColor (element, color, state)
	Cardinal		element;
	XColor			*color;
	OlDefine		state;
#endif
{
	static Display		*dpy;

	if (!dpy)
		dpy = XtDisplayOfObject(workspace);

	/*
	 * This routine is for visible elements, only.
	 */
	if (element >= NELEMENTS_NAMED)
		return;

	if (COLOR(element)) {
		XtFree (COLOR(element));
		COLOR(element) = 0;
	}

	_OlFlatSetChangeBarState (elements, element, state, OL_PROPAGATE);

	RWCOLOR(element).red = color->red;
	RWCOLOR(element).green = color->green;
	RWCOLOR(element).blue = color->blue;
	XStoreColor(dpy, InitShell->core.colormap, &RWCOLOR(element));

	/* WARNING -- This code assumes that the window background is the
	 * only color that will require a recalculation of bg's 0, 2, and 3.
	 */
	if (element == WINDOWS)
	{

		OlgGetColors (SCREEN, PIXEL(WINDOWS), &BG0Color,
			      &BG2Color, &BG3Color);
		XStoreColor(dpy, InitShell->core.colormap, &BG0Color);
		XStoreColor(dpy, InitShell->core.colormap, &BG2Color);
		XStoreColor(dpy, InitShell->core.colormap, &BG3Color);
	}

	return;
} /* ChangeElementByColor */

/**
 ** ChangeElementByPixel()
 **/

static void
#if	OlNeedFunctionPrototypes
ChangeElementByPixel (
	Cardinal		element,
	Pixel			pixel,
	OlDefine		state
)
#else
ChangeElementByPixel (element, pixel, state)
	Cardinal		element;
	Pixel			pixel;
	OlDefine		state;
#endif
{
	/*
	 * This routine is for visible elements, only.
	 * Also, don't bother continuing if the pixel isn't new.
	 */
	if (element >= NELEMENTS_NAMED || PIXEL(element) == pixel)
		return;

	/*
	 * Don't bother converting the pixel value to a name here,
	 * let the caller do that, if necessary. Do clear the name,
	 * though, otherwise it would be the wrong name.
	 */
	if (COLOR(element)) {
		XtFree (COLOR(element));
		COLOR(element) = 0;
	}
	PIXEL(element) = pixel;

	OlVaFlatSetValues (
		elements,
		element,
		XtNchipColor,	(XtArgVal)pixel,
		(String)0
	);
	_OlFlatSetChangeBarState (elements, element, state, OL_PROPAGATE);

	UpdateElement (element, pixel, False);

	/*
	 * This is redundant whenever a single color element is
	 * selected, but is necessary (e.g. to clear a DIM state)
	 * whenever more than one elements are selected.
	 */
	SyncColorMenu ();

	return;
} /* ChangeElementByPixel */

/**
 ** UpdateElement ()
 **/

static void
#if	OlNeedFunctionPrototypes
UpdateElement (
	Cardinal		element,
	Pixel			pixel,
	Boolean			followersOnly
)
#else
UpdateElement (element, pixel, followersOnly)
	Cardinal		element;
	Pixel			pixel;
	Boolean			followersOnly;
#endif
{
				/* Any widget will do. */
	Widget			w = elements;

	Cardinal		i;


	/*
	 * The color of a named, visible elements may affect some
	 * auxiliary ``followers''. Here we update the unnamed elements
	 * in the sample, and ALSO assign the PIXEL() values.
	 * Do these first, as we may need to check some of the values
	 * when doing the named elements.
	 */
	for (i = NELEMENTS_NAMED; i < NELEMENTS; i++) {
		Pixel		old = PIXEL(i);
		Pixel		new = PIXEL(i);

		if (colors[i].follows == element)
			new = PIXEL(element);
		else if (colors[i].follows == (CONTRAST | element))
		{
			if (ColorChoice == PseudoColorChoiceCB)
			{
				unsigned long	intensity;

				intensity = RWCOLOR(element).red * 39l +
				    RWCOLOR(element).green * 50l +
				    RWCOLOR(element).blue * 11l;

				if (intensity > 65535l*25)
					new = BlackPixelOfScreen (SCREEN);
				else
					new = WhitePixelOfScreen (SCREEN);
			}
			else
				new = _OlContrastingColor(w,PIXEL(element),25);
		}

		if (new != old) {
			if (COLOR(i)) {
				XtFree (COLOR(i));
				COLOR(i) = 0;
			}
			PIXEL(i) = new;

			switch (i) {
			case FOREGROUND:
				PropagateResource (
					XtNforeground, (XtArgVal)new,
					PROPAGATE_ALL
				);
				break;
			case BORDERCOLOR:
				PropagateResource (
					XtNborderColor, (XtArgVal)new,
					PROPAGATE_ALL
				);
				break;
			case FONTCOLOR:
				PropagateResource (
					XtNfontColor, (XtArgVal)new,
					PROPAGATE_OTHER
				);
				break;
			}
		}
	}

	if (followersOnly)
		return;

	/*
	 * Now update the named elements in the sample:
	 */
	switch (element) {

	case WORKSPACE:
		XtVaSetValues (
			workspace,
			XtNbackground,	(XtArgVal)pixel,
			(String)0
		);
		break;

	case WINDOWS:

		/* If the colormap is writable, we need to prevent the toolkit
		 * from allocating colors for the widgets that use the Window
		 * color.  Create a temporary color tuple list that contains only
		 * one item in it.
		 */
		if (ColorChoice == PseudoColorChoiceCB)
		{
			Display			*dpy = XtDisplayOfObject(w);
			static OlColorTupleList	tmpList;
			static OlColorTuple	tuple [1];

			OlgGetColors (SCREEN, PIXEL(WINDOWS), &BG0Color,
				      &BG2Color, &BG3Color);
			XStoreColor(dpy, InitShell->core.colormap, &BG0Color);
			XStoreColor(dpy, InitShell->core.colormap, &BG2Color);
			XStoreColor(dpy, InitShell->core.colormap, &BG3Color);

			tmpList.size = 1;
			tmpList.list = tuple;
			tuple[0].bg0 = BG0Color.pixel;
			tuple[0].bg1 = PIXEL(WINDOWS);
			tuple[0].bg2 = BG2Color.pixel;
			tuple[0].bg3 = BG3Color.pixel;

			OlGetColorTupleList ((Widget)0, &RestoreList, &RestoreUseList);
			OlSetColorTupleList ((Widget)0, &tmpList, True);
		}

		PropagateResource (
			XtNbackground,		(XtArgVal)pixel,
			PROPAGATE_OTHER
		);
		break;

	case INPUT_FOCUS:
		/*
		 * Make sure the foreground is visible in the input focus
		 * elements.
		 */
		PropagateResource (
			XtNinputFocusColor, (XtArgVal)pixel,
			PROPAGATE_ALL
		);
		PropagateResource (
			XtNbackground, (XtArgVal)pixel,
			PROPAGATE_FOCUS
		);
		PropagateResource (
			XtNfontColor, (XtArgVal)FontColorWithFocus(pixel),
			PROPAGATE_FOCUS
		);
		break;

	case INPUT_WINDOW:
		XtVaSetValues (
			window,
			"inputWindowHeader",	(XtArgVal)pixel,
			(String)0
		);
		break;

	case TEXT_FOREGROUND:
		PropagateResource (
			XtNfontColor,		(XtArgVal)pixel,
			PROPAGATE_TEXT
		);
		break;

	case TEXT_BACKGROUND:
		PropagateResource (
			XtNbackground,		(XtArgVal)pixel,
			PROPAGATE_TEXT
		);
		break;

	case HELP_KEYCOLOR:
		break;

	default:
		break;
	}
}

/**
 ** PropagateResource()
 ** PropagateResourceHold()
 ** PropagateResourceResume()
 **/

static enum {
	_HOLD_,
	_RESUME_
}			propagate_state;

static struct propagate_cache {
	String			name;
	XtArgVal		value;
	unsigned int		type;
}			propagate_cache[PROPAGATE_CACHE_LIMIT];

static Cardinal		propagate_cache_count	= 0;

static void		PropagateResourceList OL_ARGS((
	Widget			w
));

static void
#if	OlNeedFunctionPrototypes
PropagateResource (
	String			name,
	XtArgVal		value,
	unsigned int		type
)
#else
PropagateResource (name, value, type)
	String			name;
	XtArgVal		value;
	unsigned int		type;
#endif
{
	if (propagate_cache_count >= PROPAGATE_CACHE_LIMIT)
		Dm__VaPrintMsg(TXT_internal_cacheExceed);

	propagate_cache[propagate_cache_count].name  = name;
	propagate_cache[propagate_cache_count].value = value;
	propagate_cache[propagate_cache_count].type  = type;
	propagate_cache_count++;

	if (propagate_state != _HOLD_) {
		PropagateResourceList (workspace);
		if (RestoreList)
		{
			OlSetColorTupleList ((Widget)0, RestoreList, RestoreUseList);
			RestoreList = 0;
		}
		propagate_cache_count = 0;
	} else

	return;
} /* PropagateResource */

static void
#if	OlNeedFunctionPrototypes
PropagateResourceList (
	Widget			w
)
#else
PropagateResourceList (w)
	Widget			w;
#endif
{
	Cardinal		nargs;
	Cardinal		i;

	Arg			args[PROPAGATE_CACHE_LIMIT];


	if (!propagate_cache_count)
		return;

#define IsText(w)	(XtClass(w) == textEditWidgetClass)
	if (w != workspace) {
		nargs = 0;
		for (i = 0; i < propagate_cache_count; i++) {
			struct propagate_cache * p = &propagate_cache[i];
			unsigned int		type;

			if (w == focus)
				type = PROPAGATE_FOCUS;
			else if (IsText(w))
				type = PROPAGATE_TEXT;
			else
				type = PROPAGATE_OTHER;

			if (p->type & type) {
				args[nargs].name  = p->name;
				args[nargs].value = p->value;
				nargs++;
			}
		}
		XtSetValues (w, args, nargs);
	}
#undef	IsText

	if (XtIsComposite(w)) {
		CompositeWidget	cw           = (CompositeWidget)w;
		Widget *	children     = cw->composite.children;
		Cardinal	num_children = cw->composite.num_children;

		while (num_children--)
			PropagateResourceList (*children++);
	}

	return;
} /* PropagateResourceList */

static void
#if	OlNeedFunctionPrototypes
PropagateResourceHold (
	void
)
#else
PropagateResourceHold ()
#endif
{
	propagate_state = _HOLD_;
	return;
} /* PropagateResourceHold */

static void
#if	OlNeedFunctionPrototypes
PropagateResourceResume (
	void
)
#else
PropagateResourceResume ()
#endif
{
	propagate_state = _RESUME_;
	PropagateResourceList (workspace);
	if (RestoreList)
	{
		OlSetColorTupleList ((Widget)0, RestoreList, RestoreUseList);
		RestoreList = 0;
	}

	/*
	 * Generate a single exposure over the entire sample,
	 * so that the colod changes occur in one shot.
	 */
	if (XtIsRealized (workspace))
		XClearArea (
			XtDisplayOfObject(workspace),
			XtWindowOfObject(workspace),
			0, 0, workspace->core.width, workspace->core.height,
			True
		);

	propagate_cache_count = 0;
	return;
} /* PropagateResourceResume */

/**
 ** SyncColorMenuCB()
 **/

static void
#if	OlNeedFunctionPrototypes
SyncColorMenuCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
SyncColorMenuCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	OlFlatCallData *	cd = (OlFlatCallData *)call_data;

	SyncColorMenu ();
	return;
} /* SyncColorMenuCB */

/**
 ** SelectElement()
 **/

static void
#if	OlNeedFunctionPrototypes
SelectElement (
	Cardinal		element
)
#else
SelectElement (element)
	Cardinal		element;
#endif
{
	Cardinal		i;


#define	SET(I,B) \
	OlVaFlatSetValues(elements, (I), XtNset, (XtArgVal)(B), (String)0)

	for (i = 0; i < NELEMENTS_NAMED; i++)
		if (i == element) {
			if (!SELECTED(i))
				SET (i, True);
		} else {
			if (SELECTED(i))
				SET (i, False);
		}

	SyncColorMenu ();

	if (!GoneFullCycle)
		FooterMessage (
			colorProperty.w,
			Dm__gettxt(TXT_footerMsg_chooseColor),
			OL_LEFT,
			False
		);
#undef	SET
	return;
} /* SelectElement */

/**
 ** SyncColorMenu()
 **/

static void
#if	OlNeedFunctionPrototypes
SyncColorMenu (
	void
)
#else
SyncColorMenu ()
#endif
{
	Cardinal	ndifferent;
	Cardinal	i;
	Pixel		selected_pixel = (Pixel)~0;
	unsigned int	flag;


	/* NOTE:  elements is now exclusive, so ndifferent can only be 0 or
	 * 1.  The additional generality is harmless, and useful if we
	 * decide to go back to non-exclusive.  (It will need to be changed
	 * to accomodate the scheme used for writable colormaps.)
	 */
	ndifferent = 0;
	for (i = 0; i < NELEMENTS_NAMED; i++)
		if (SELECTED(i) && PIXEL(i) != selected_pixel) {
			if (selected_pixel == ~0)
				selected_pixel = PIXEL(i);
			ndifferent++;
		}

	switch (ndifferent) {
	case 0:
		/*
		 * It's a bit of a hack, but....when we're in a
		 * #planes==1 situation, we always come here
		 * (because the color elements can't be selected).
		 * Furthermore, the SetColorMenu() routine needs a
		 * single color from which it can figure out if
		 * we're black-on-white or white-on-black. The
		 * pixel has to be a ``background'' color, so we
		 * send the WINDOWS color.
		 *
		 * Thus, even if we're (apparently) telling the
		 * color menu code that we have no color of interest,
		 * we always pass the WINDOWS color in case we're
		 * in the #planes==1 situation.
		 */
		SetColorMenu (color_menu, PIXEL(WINDOWS), COLOR_SET_NONE);
		break;

	case 1:
		flag = COLOR_SET_ONE;
		goto LoopSet;
	default:
		flag = COLOR_SET_MANY;
LoopSet:	for (i = 0; i < NELEMENTS_NAMED; i++)
			if (SELECTED(i)) {
				SetColorMenu (color_menu, PIXEL(i), flag);
				break;
			}
		break;
	}

	return;
} /* SyncColorMenu */

/**
 ** TouchEH()
 **/

static void
#if	OlNeedFunctionPrototypes
TouchEH (
	Widget			w,
	XtPointer		client_data,
	XEvent *		pe,
	Boolean *		continue_to_dispatch
)
#else
TouchEH (w, client_data, pe)
	Widget			w;
	XtPointer		client_data;
	XEvent *		pe;
	Boolean *		continue_to_dispatch;
#endif
{
	SelectType		type	= (SelectType)client_data;

	OlVirtualEventRec	ve;


	OlLookupInputEvent (w, pe, &ve, OL_DEFAULT_IE);

	switch (ve.virtual_name) {
	case OL_SELECT:
		switch (pe->type) {
		case ButtonPress:
		case EnterNotify:
			switch (type) {
			case SELECT_WORKSPACE:
				SelectElement (WORKSPACE);
				break;
			case SELECT_BACKGROUND:
				SelectElement (WINDOWS);
				break;
			case SELECT_TEXT:
				WhichTextPart (w, pe);
				break;
			}
			break;
		}
		break;
	}
	return;
} /* TouchEH */

/**
 ** WindowSelectCB()
 **/

static void
#if	OlNeedFunctionPrototypes
WindowSelectCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
WindowSelectCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	OlDefine *		p_part = (OlDefine *)call_data;


	switch (*p_part) {
	case WM_PART_NONE:
		break;
	case WM_PART_HEADER:
		SelectElement (INPUT_WINDOW);
		break;
	case WM_PART_BACKGROUND:
		SelectElement (WINDOWS);
		break;
	}
} /* WindowSelectCB */

/**
 ** ButtonSelectCB()
 **/

static void
#if	OlNeedFunctionPrototypes
ButtonSelectCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
ButtonSelectCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	Screen *		screen	= XtScreenOfObject(w);

	Cardinal		button	= (Cardinal)client_data;


	if (button == 1 && PlanesOfScreen(screen) > 1)
		SelectElement (INPUT_FOCUS);

	return;
} /* ButtonSelectCB */

/**
 ** WhichTextPart()
 **/

static void
#if	OlNeedFunctionPrototypes
WhichTextPart (
	Widget			w,
	XEvent *		pe
)
#else
WhichTextPart (w, pe)
	Widget			w;
	XEvent *		pe;
#endif
{
	TextEditWidget		tew	= (TextEditWidget)w;

	/*
	 * This type punning works because the events we handle
	 * here have the same structure up to the x,y values.
	 */
	XButtonEvent *		pbe	= &(pe->xbutton);

	TextBuffer *		text;

	TextPosition		p;
	TextPosition		from;
	TextPosition		to;
	TextLocation		location;
	TextLocation		wrap_location;

	WrapTable *		wrap_table;

	XRectangle		rect;


	/*
	 * So far, pretty simple--except watch for the TextEdit magic
	 * trick: Put a String in, get a TextBuffer* out!
	 */
	XtVaGetValues (w, XtNsource, &text, (String)0);

	/*
	 * ``The mouse coordinates begat the character position in the
	 * TextBuffer, the character position begat the Location (the
	 * *input* line and offset within the line), the Location begat
	 * the WrapLocation (the *display* line and offset).''
	 */
	p             = _PositionFromXY(w, pbe->x, pbe->y);
	location      = LocationOfPosition(text, p);
	wrap_table    = tew->textedit.wrapTable;
	wrap_location = _WrapLocationOfLocation(wrap_table, location);

	/*
	 * Get the range of character positions of the *display* line
	 * under the mouse pointer. Trim trailing whitespace.
	 */
	from = _PositionOfWrapLocation(text, wrap_table, wrap_location);
	(void)_GetNextWrappedLine(text, wrap_table, &wrap_location);
	to   = _PositionOfWrapLocation(text, wrap_table, wrap_location);
	if (to < 0)
		to = LastTextBufferPosition(text);
	while (from < to) {
		register int c
		  = GetTextBufferChar(text, LocationOfPosition(text, to));

		if (!isspace(c))
			break;
		to--;
	}

 	/*
	 * Now get the rectangle bounding the *display* line. If we're
	 * within it, we have text foreground, else background.
	 */
	rect = _RectFromPositions(tew, from, to);
	if (
		rect.x <= pbe->x && pbe->x < rect.x + (int)rect.width
	     && rect.y <= pbe->y && pbe->y < rect.y + (int)rect.height
	)
		SelectElement (TEXT_FOREGROUND);
	else
		SelectElement (TEXT_BACKGROUND);

	/*
	 * Whew!!
	 */
	return;
} /* WhichTextPart */

#if	defined(FACTORY_LIST)

/**
 ** Factory color choices:
 **/

static Widget		preview;
static Widget		choices;

static Cardinal		next_item_index;

/**
 ** CreateFactoryList()
 **/

static void
#if	OlNeedFunctionPrototypes
CreateFactoryList (
	Widget			parent
)
#else
CreateFactoryList (parent)
	Widget			parent;
#endif
{
	Widget			box;
	Widget			abbrev;
	Widget			menupane;
	Widget			next;
	Widget			factoryW;
	int pixels;
	int height, width;
	Display *		dpy = XtDisplay(parent);

	static String		fields[] = {
		XtNlabel, XtNclientData, XtNset
	};

	typedef struct ListItem {
		XtPointer		label;
		XtPointer		client_data;
		XtPointer		set;
	}			ListItem;

	static ListItem *	_items	= 0;
	static ListItem		next_choice[] = {
		"Next choice", (XtPointer)0, (XtPointer) True
	};

	Cardinal		N	= wsm.factory_color_lists->size;
	Cardinal		n;

	ColorList *		list	= wsm.factory_color_lists->list;

	Screen *		screen	= XtScreenOfObject(parent);
	Visual *		visual	= XDefaultVisualOfScreen(screen);


	if (
		PlanesOfScreen(screen) <= 1
	     || visual->class == StaticGray
	     || visual->class == GrayScale
	)
		return;

        factoryW = CreateCaption(
                                "factoryList",
				OLG(colorCombo,fixedString),
                                parent
	);
        /* Convert .1 horizontal inches to pixels */
	pixels = OlScreenMMToPixel(OL_HORIZONTAL, 2.54, screen);
	XtVaSetValues (
		factoryW,
		XtNspace,   (XtArgVal)pixels,
		(String)0
	);

	box = XtVaCreateManagedWidget(
		"box",
		formWidgetClass,
		factoryW,
		XtNshadowThickness,	(XtArgVal)0,
		(String)0
	);
	/* Convert .3 horizontal inches to pixels */
        pixels = OlScreenMMToPixel (OL_HORIZONTAL, 7.62, screen);
	/* 2 inches */
	width = OlScreenMMToPixel (OL_HORIZONTAL, 50.8, screen);
	height = OlScreenPointToPixel (OL_VERTICAL, 18, screen);
	dpy = XtDisplay(box);
	preview = XtVaCreateManagedWidget(
		"preview",
		staticTextWidgetClass,
		box,
		XtNxOffset,		(XtArgVal)pixels,
		XtNstring,		(XtArgVal)OLG(custom,fixedString),
		XtNwidth,		(XtArgVal)width,
		XtNheight,		(XtArgVal)height,
		XtNwrap,		(XtArgVal)False,
		XtNrecomputeSize,	(XtArgVal)False,
		XtNgravity,		(XtArgVal)NorthWestGravity,
		(String)0
	);

	menupane = XtCreatePopupShell("pane", popupMenuShellWidgetClass, box,
                NULL, 0);
	abbrev = XtVaCreateManagedWidget(
		"abbrev",
		abbreviatedButtonWidgetClass,
		box,
		XtNpopupWidget,	(XtArgVal) menupane,
		(String)0
	);
	XtVaGetValues (abbrev, XtNmenuPane, (XtArgVal)&menupane, (String)0);

	if (!_items) {
		String			custom;

		XtVaGetValues (preview, XtNstring, (XtArgVal)&custom, (String)0);
		_items = ARRAY(ListItem, N+1);
		_items[0].label       = (XtPointer)XtNewString(custom);
		_items[0].client_data = (XtPointer)0;
		for (n = 1; n <= N; n++) {
			_items[n].label       = (XtPointer)list[n-1].name;
			_items[n].client_data = (XtPointer)&(list[n-1]);
		}
	}
	_items[0].set = (XtPointer)True;
	for (n = 1; n <= N; n++)
		_items[n].set = (XtPointer)False;
	next_item_index = 1;

	choices = XtVaCreateManagedWidget(
		"factoryChoices",
		flatButtonsWidgetClass,
		menupane,
                XtNbuttonType,	  (XtArgVal)OL_RECT_BTN,
		XtNexclusives,	  (XtArgVal)True,
		XtNselectProc,    (XtArgVal)FactorySelectCB,
		XtNitems,         (XtArgVal)_items,
		XtNnumItems,      (XtArgVal)N+1,
		XtNitemFields,    (XtArgVal)fields,
		XtNnumItemFields, (XtArgVal)XtNumber(fields),
		XtNlayoutType,	  (XtArgVal)OL_FIXEDCOLS,
		(String)0
	);

        next_choice[0].client_data = choices;
	next = XtVaCreateManagedWidget(
		"Next choice",
		flatButtonsWidgetClass,
		menupane,
		XtNselectProc,	  (XtArgVal)NextChoiceCB,
		XtNitems,	  (XtArgVal)next_choice,
		XtNnumItems,	  (XtArgVal)XtNumber(next_choice),
		XtNitemFields,	  (XtArgVal)fields,
		XtNnumItemFields, (XtArgVal)XtNumber(fields),
		XtNdefault,	  (XtArgVal)True,
		XtNlabel, (XtArgVal)Dm__gettxt(TXT_fixedString_nextChoice),
		(String)0
	);

	return;
} /* CreateFactoryList */

/**
 ** SwitchToCustomItem()
 **/

static void
#if	OlNeedFunctionPrototypes
SwitchToCustomItem (
	void
)
#else
SwitchToCustomItem ()
#endif
{
	String			label;


	OlVaFlatSetValues (choices, 0, XtNset, (XtArgVal)True, (String)0);
	OlVaFlatGetValues (choices, 0, XtNlabel, (XtArgVal)&label, (String)0);
	XtVaSetValues (preview, XtNstring, (XtArgVal)label, (String)0);
	next_item_index = 1;

	return;
} /* SwitchToCustomItem */

/**
 ** FactorySelectCB()
 **/

static void
#if	OlNeedFunctionPrototypes
FactorySelectCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
FactorySelectCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	OlFlatCallData *	fd	= (OlFlatCallData *)call_data;

	ColorList *		list	= (ColorList *)client_data;

	String			label;


	if (!list)
		ResetCB (w, (XtPointer)0);
	else {
		if (ColorChoice == ColorChoiceCB)
		{

#define CHANGE(I,F) \
	ChangeElementByPixel((I), list->F, OL_NORMAL);

			PropagateResourceHold ();
			CHANGE (WORKSPACE,       workspace);
			CHANGE (WINDOWS,         background);
			CHANGE (INPUT_FOCUS,     input_focus_color);
			CHANGE (INPUT_WINDOW,    input_window_header);
			CHANGE (TEXT_FOREGROUND, text_font_color);
			CHANGE (TEXT_BACKGROUND, text_background);
			CHANGE (HELP_KEYCOLOR,   help_highlight);
			PropagateResourceResume ();
#undef	CHANGE
		}
		else
		{

#define CHANGE(I,F) \
	ChangeElementByName(w, (I), PixelToString(w, list->F), OL_NORMAL)

			CHANGE (WORKSPACE,       workspace);
			CHANGE (WINDOWS,         background);
			CHANGE (INPUT_FOCUS,     input_focus_color);
			CHANGE (INPUT_WINDOW,    input_window_header);
			CHANGE (TEXT_FOREGROUND, text_font_color);
			CHANGE (TEXT_BACKGROUND, text_background);
			CHANGE (HELP_KEYCOLOR,   help_highlight);
#undef	CHANGE
		}
	}

	OlVaFlatGetValues (
		w, fd->item_index, XtNlabel, (XtArgVal)&label, (String)0
	);
	XtVaSetValues (preview, XtNstring, (XtArgVal)label, (String)0);

	/*
	 * Prepare for the next time someone invokes the ``Next choice''
	 * button.
	 */
	next_item_index = fd->item_index + 1;
	if (next_item_index >= fd->num_items)
		next_item_index = 0;

	return;
} /* FactorySelectCB */

/**
 ** NextChoiceCB()
 **/

static void
#if	OlNeedFunctionPrototypes
NextChoiceCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
NextChoiceCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	Widget			choices	= (Widget)client_data;


	/*
	 * Selecting the ``Next choice'' button is identical to
	 * selecting the choice after the current choice, so use
	 * the same code-path. Note that when activating a sub-item,
	 * the indexes go from 1 to n, not 0 to n-1; so add 1 to
	 * what we think is the next item's index.
	 */
	OlActivateWidget (
		choices,
		OL_SELECTKEY,
		(XtPointer)(next_item_index+1)
	);
	return;
} /* NextChoiceCB */

/**
 ** StringToColorLists()
 **/

/*ARGSUSED5*/
Boolean
#if	OlNeedFunctionPrototypes
StringToColorLists (
	Display *		display,
	XrmValue *		args,
	Cardinal *		num_args,
	XrmValue *		from,
	XrmValue *		to,
	XtPointer *		converter_data
)
#else
StringToColorLists (display, args, num_args, from, to, converter_data)
	Display *		display;
	XrmValue *		args;
	Cardinal *		num_args;
	XrmValue *		from;
	XrmValue *		to;
	XtPointer *		converter_data;
#endif
{
	ColorLists *		list;

	Screen *		screen   = InitShell->core.screen;

	Colormap		colormap = InitShell->core.colormap;


	if (*num_args != 0)
		Dm__VaPrintMsg(TXT_badConversion_tooManyParams,
			"String", "ColorList");
		/*NOTREACHED*/

	if (!IsColorLists((String)from->addr, &list)) {
		Dm__VaPrintMsg(TXT_badConversion_illegalString,
			"String", "ColorList",
			(String)from->addr
		);
		return (False);
	}

	ConversionDone (ColorLists *, list);
} /* StringToColorLists */

/**
 ** IsColorLists()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
IsColorLists (
	String			_input,
	ColorLists **		p_list
)
#else
IsColorLists (_input, p_list)
	String			_input;
	ColorLists **		p_list;
#endif
{
	ColorLists *		p;

	Cardinal		n;


	/*
	 * Two passes over the input, the first to count the
	 * number of tuples, the second to store the tuples.
	 * Sounds inefficient, but it is better than malloc'ing
	 * on the fly.
	 */

#define PARSE(PN,L) ParseColorLists(_input,(PN),(L))

	if (!PARSE(&n, (ColorList *)0) || !n)
		return (False);

	p = *p_list = XtNew(ColorLists);
	p->size = n;
	p->list = ARRAY(ColorList, n);
	if (!PARSE((Cardinal *)0, p->list))
		return (False);

	return (True);
} /* IsColorLists */

/**
 ** ParseColorLists()
 **/
		
static Boolean
#if	OlNeedFunctionPrototypes
ParseColorLists (
	String			_input,
	Cardinal *		p_n,
	ColorList *		store
)
#else
ParseColorLists (_input, p_n, store)
	String			_input;
	Cardinal *		p_n;
	ColorList *		store;
#endif
{
	Display *		display	= XtDisplayOfObject(InitShell);

	Cardinal		n;

	String			input	= XtNewString(_input);
	String			list;
	String			rest;

	static Cardinal		offset[8] = {
		XtOffsetOf(ColorList, name),
		XtOffsetOf(ColorList, workspace),
		XtOffsetOf(ColorList, background),
		XtOffsetOf(ColorList, input_focus_color),
		XtOffsetOf(ColorList, input_window_header),
		XtOffsetOf(ColorList, text_font_color),
		XtOffsetOf(ColorList, text_background),
		XtOffsetOf(ColorList, help_highlight),
	};


/*
 * Easy way to ensure we always free things before leaving:
 */
#define RETURN(R) return ((XtFree(input), (R)))


	/*
	 * Syntax:
	 *
	 *	lists := lists , list
	 *	list  := ( name , color1 , color2 , ... , color6 )
	 *
	 * Each of colorN is a value that can be parsed
	 * with the registered String to Pixel converter, optionally
	 * enclosed in parentheses. (Enclosing in parentheses allows
	 * giving a string that has arbitrary syntax parsed by a
	 * new String to Pixel routine added later.)
	 */

#define LPAR	'('		/* I18N */
#define RPAR	')'		/* I18N */
#define SEP	','		/* I18N */

	for (
		list = strchr(input, LPAR), n = 0;
		list;
		list = strchr(rest, LPAR),  n++
	) {
		String			p;
		String			thing;

		Cardinal		i;

		XrmValue		from;

		static Pixel		pixel;

		static XrmValue	to = { sizeof(Pixel), (XtPointer)&pixel };


		list++;
		if (!(p = strbal(list, LPAR, RPAR)))
			RETURN (False);
		*p++ = 0;
		rest = p;

		for (i = 0; i <= 8 && list; i++) {

			while (isspace(*list))
				list++;
			if (!*list)
				RETURN (False);

			if (*list == LPAR) {
				thing = list+1;
				while (isspace(*thing))
					thing++;
				if (!*thing)
					RETURN (False);

				if (!(list = strbal(thing, LPAR, RPAR)))
					RETURN (False);
				*list = 0;
			} else
				thing = list;

			/*
			 * Prepare for the next pass; this will
			 * also let us terminate this pass' color
			 * value with a null.
			 */
			list = strchr(list+1, SEP);
			if (list)
				*list++ = 0;

			/*
			 * Trim trailing whitespace from the thing value.
			 */
			for (p = strchr(thing, 0); p != thing && isspace(p[-1]); p--)
				;
			if (p == thing)
				RETURN (False);
			*p = 0;

			if (store && i == 0) {
				*((String *)((char *)store + offset[i])) = XtNewString(thing);
			} else if (store) {
				from.addr = thing;
				from.size = strlen(thing) + 1;
				if (!XtConvertAndStore(
					InitShell,
					XtRString,
					&from,
					XtRPixel,
					&to
				))
					RETURN (False);

				*((Pixel *)((char *)store + offset[i])) = pixel;
			}
		}

		if (store)
			store++;
	}

	if (p_n)
		*p_n = n;

	RETURN (True);

} /* ParseColorLists */

/**
 ** strbal()
 **/

static String
#if	OlNeedFunctionPrototypes
strbal (
	String			str,
	char			left,
	char			right
)
#else
strbal (str, left, right)
	String			str;
	char			left;
	char			right;
#endif
{
	/*
	 * Find the character that balances another character,
	 * ignoring nested pairs of such characters. For instance,
	 * given a pointer to the character AFTER a left parenthesis,
	 * this routine will find the balancing right parenthesis.
	 * It will skip enclosed, balanced pairs of parentheses.
	 */
	for (; *str && *str != right; str++)
		if (*str == left) {
			str = strbal(str+1, left, right);
			if (!str)
				return (0);
		}
	return (!*str? 0 : str);
} /* strbal */

#endif
