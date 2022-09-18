/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)libDtI:HyperText.c	1.41"

/***************************************************************
**
**      Copyright (c) 1990
**      AT&T Bell Laboratories (Department 51241)
**      All Rights Reserved
**
**      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
**      AT&T BELL LABORATORIES
**
**      The copyright notice above does not evidence any
**      actual or intended publication of source code.
**
**      Author: Hai-Chen Tu
**      File:   HyperText.c
**      Date:   08/06/90
**
**************************************************************************
*/

/*
 *************************************************************************
 *
 * Description:
 *   This file contains the source code for the hypertext widget.
 *
 ******************************file*header********************************
 */

                        /* #includes go here    */

#include <string.h>
#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <Xol/OpenLookP.h>
#include <Xol/BulletinBo.h>
#include <Xol/ScrolledWi.h>

#include "HyperTextP.h"

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define BLACK BlackPixelOfScreen(screen)
#define WHITE WhitePixelOfScreen(screen)

#define IsColor(screen) (DefaultDepthOfScreen(screen)>1)

#define STR_DIFF(a,b) (((a)==NULL)?((int)(b)):((b)==NULL?1:(strcmp((a),(b)))))
#define str_len(s) ((s)?strlen((s)):0)
#define MAX(a,b) ((a)>(b)?(a):(b))

#define BYTE_OFFSET XtOffsetOf(HyperTextRec, hyper_text.dyn_flags)

/* SPACE_ABOVE can be 0, but SPACE_BELOW has to be at least 1 */
#define SPACE_ABOVE  1
#define SPACE_BELOW  1

extern char *strcat();
extern char *strcpy();

/* to be used to display terms in a help window */
static Font italic_fid = NULL;

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *      1. Private Procedures
 *      2. Class   Procedures
 *      3. Action  Procedures
 *      4. Public  Procedures
 *
 **************************forward*declarations***************************
 */

                    /* private procedures       */

static HyperSegment *GetFirstCmdSegment();
static HyperSegment *GetPrevCmdSegment();
static HyperSegment *GetNextCmdSegment();
static HyperLine    *HyperTextFindLine();

static void HyperTextExpose();
static void gc_init();
static void get_italic_font();
static void draw_request();
static void draw_lines();
static void draw_line1();
static void draw_segment();
static void draw_focus_segment();
static void set_line();
static void layout();
static void adjust_size();
static int  layout_line();
static int  check_max_lines();
static XtGeometryResult QueryGeom();
static void ViewSizeChanged(Widget w, OlSWGeometries *geom);

                    /* class procedures     */

static void class_init();
static void init();
static void destroy();
static void resize();
static void expose();
static void hyper_text_select();
static void HTButtonHandler();
static Boolean set_values();

                    /* action procedures        */

static void    HighlightHandler();
static Boolean ActivateWidget();
static Widget  TraversalHandler();

                    /* public procedures        */
/* none */

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

static OlEventHandlerRec ht_event_procs[] =
{
	{ ButtonPress,   HTButtonHandler },
	{ ButtonRelease, HTButtonHandler },
};

static int default_source_type = OL_STRING_SOURCE;

/*
 ***********************************************************************
 * Dynamic resources
 ***********************************************************************
*/

static _OlDynResource dyn_res[] = {
	{ { XtNbackground, XtCTextBackground, XtRPixel, sizeof(Pixel), 0,
		XtRString, XtDefaultBackground }, BYTE_OFFSET, OL_B_HYPERTEXT_BG,
		NULL
	},

	{ { XtNfont, XtCFont, XtRFont, sizeof(XFontStruct *), 0,
		XtRString, OlDefaultFont }, BYTE_OFFSET,
		OL_B_HYPERTEXT_FONT, NULL
	},

	{ { XtNfontColor, XtCTextFontColor, XtRPixel, sizeof(Pixel), 0,
		XtRString, XtDefaultForeground }, BYTE_OFFSET,
		OL_B_HYPERTEXT_FONTCOLOR, NULL
	},
};
#undef BYTE_OFFSET


/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

#define offset(field) XtOffset(HyperTextWidget, field)

static XtResource resources[] = {
	{ XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
		offset(core.border_width), XtRImmediate, (XtPointer) 0 },

	{ XtNstring, XtCString, XtRString, sizeof(String),
		offset(hyper_text.string), XtRString, NULL },

	{ XtNfile, XtCFile, XtRString, sizeof(String),
        offset(hyper_text.file), XtRString, NULL },

	{ XtNsourceType, XtCSourceType, XtRInt, sizeof(int),
		offset(hyper_text.source_type), XtRInt,
		(XtPointer) &default_source_type },

	{ XtNbackground, XtCTextBackground, XtRPixel, sizeof(Pixel),
		offset(core.background_pixel), XtRString, XtDefaultBackground },

	{ XtNfontColor, XtCTextFontColor, XtRPixel, sizeof(Pixel),
		offset(primitive.font_color), XtRString, XtDefaultForeground },

	{ XtNkeyColor, XtCForeground, XtRPixel, sizeof(Pixel),
		offset(hyper_text.key_color), XtRString, "Blue" },

	{ XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
		offset(hyper_text.font), XtRString, OlDefaultFont },

	{ XtNleftMargin, XtCMargin, XtRDimension, sizeof(Dimension),
		offset(hyper_text.left_margin), XtRString, "20" },

	{ XtNtopMargin, XtCMargin, XtRDimension, sizeof(Dimension),
		offset(hyper_text.top_margin), XtRString, "20" },

	{ XtNrightMargin, XtCMargin, XtRDimension, sizeof(Dimension),
		offset(hyper_text.right_margin), XtRString, "20" },

	{ XtNbottomMargin, XtCMargin, XtRDimension, sizeof(Dimension),
		offset(hyper_text.bot_margin), XtRString, "20" },

	{ XtNmaxLines, XtCMaxSize, XtRDimension, sizeof(Dimension),
		offset(hyper_text.max_lines), XtRString, "20" },

	{ XtNselect, XtCCallback, XtRCallback, sizeof(XtPointer),
		offset(hyper_text.callbacks), XtRCallback, (XtPointer) NULL },

	{ XtNrecomputeSize, XtCRecomputeSize, XtRBoolean, sizeof(Boolean),
		offset(hyper_text.resizable), XtRBoolean, (XtPointer)"TRUE" },
};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

/*
 * ...ClassData must be initialized at compile time.  Must initialize all
 * substructures.  (Actually, last two here need not be initialized since not
 * used.)
 */
HyperTextClassRec hyperTextClassRec = {
 {
 (WidgetClass) & primitiveClassRec, /* superclass            */
 "HyperText",                       /* class_name            */
 sizeof(HyperTextRec),              /* size                  */
 NULL,                              /* class_initialize      */
 NULL,                              /* class_part_initialize */
 FALSE,                             /* class_inited          */
 init,                              /* initialize            */
 NULL,                              /* initialize_hook       */
 XtInheritRealize,                  /* realize               */
 NULL,                              /* actions               */
 0,                                 /* num_actions           */
 resources,                         /* resources             */
 XtNumber(resources),               /* resource_count        */
 NULLQUARK,                         /* xrm_class             */
 TRUE,                              /* compress_motion       */
 TRUE,                              /* compress_exposure     */
 TRUE,                              /* compress_enterleave   */
 FALSE,                             /* visible_interest      */
 destroy,                           /* destroy               */
 resize,                            /* resize                */
 expose,                            /* expose                */
 set_values,                        /* set_values            */
 NULL,                              /* set_values_hook       */
 XtInheritSetValuesAlmost,          /* set_values_almost     */
 NULL,                              /* get_values_hook       */
 XtInheritAcceptFocus,              /* accept_focus          */
 XtVersion,                         /* version               */
 NULL,                              /* callback_private      */
 XtInheritTranslations,             /* tm_table              */
 QueryGeom,                         /* query_geometry        */
},     /* End of CoreClass fields initialization */

{
 True,                              /* focus_on_select       */
 (OlHighlightProc)HighlightHandler, /* highlight_handler     */
 (OlTraversalFunc)TraversalHandler, /* traversal_handler     */
 XtInheritRegisterFocus,            /* register_focus        */
 (OlActivateFunc)ActivateWidget,    /* activate              */
 ht_event_procs,                    /* event_procs           */
 XtNumber(ht_event_procs),          /* num_event_procs       */
 OlVersion,                         /* version               */
 NULL,                              /* extension             */
 { dyn_res, XtNumber(dyn_res) },    /* dyn_data              */
 NULL                               /* transparent_proc      */
 },    /* End of Primitive field initializations */

 {
 0,                                 /* field not used        */
 },    /* End of HyperTextClass fields initialization */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass hyperTextWidgetClass = (WidgetClass) & hyperTextClassRec;


/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 ****************************procedure*header*****************************
 * Set segment to inverse video.
 *************************************************************************
*/
void
HyperTextSetSegmentRV(hw, hs, flag)
HyperTextWidget hw;
HyperSegment *hs;
Boolean flag;
{
    if (XtIsSubclass((Widget)hw, hyperTextWidgetClass) == FALSE)
        return;

    if (hs == NULL)
        return;

    hs->reverse_video = flag;

    if (XtIsRealized((Widget)hw))
        draw_segment(hw, XtWindow(hw), hs, TRUE, FALSE);
}

/*
 ****************************procedure*header*****************************
 * Make the given segment as the current selection.
 * Also unhighlight the previous selection.
 * Input: hs may be NULL (will still do HyperTextUnhighlightSegment(hw)).
 *************************************************************************
*/
void

HyperTextHighlightSegment(hw, hs)
HyperTextWidget hw;
HyperSegment * hs;
{
    if (XtIsSubclass((Widget)hw, hyperTextWidgetClass) == FALSE)
	return;

    HyperTextUnhighlightSegment(hw); /* also unset is_busy flag */

    if (hs == NULL)
	return;

    hw->hyper_text.highlight = hs;
    if (XtIsRealized((Widget)hw))
	draw_focus_segment(hw, XtWindow(hw), hs, FALSE);
}


/*
 ****************************procedure*header*****************************
 * Unhighlight the current segment selection.
 *************************************************************************
*/
void
HyperTextUnhighlightSegment(hw)
HyperTextWidget hw;
{
    HyperSegment * hs;

    if (XtIsSubclass((Widget)hw, hyperTextWidgetClass) == FALSE)
	return;

    hs = hw->hyper_text.highlight;
    hw->hyper_text.highlight = NULL;
    hw->hyper_text.is_busy = FALSE;

    if (hs == NULL)
	return;

    if (XtIsRealized((Widget)hw))
	draw_focus_segment(hw, XtWindow(hw), hs, TRUE);
}

/*
 ****************************procedure*header*****************************
 * Return the current highlighted segment.
 *************************************************************************
*/

HyperSegment *
HyperTextGetHighlightedSegment(hw)
HyperTextWidget hw;
{
    if (XtIsSubclass((Widget)hw, hyperTextWidgetClass) == FALSE)
	return(NULL);

    return(hw->hyper_text.highlight);
}

/*
 ****************************procedure*header*****************************
 * Use default color for the segment.
 *************************************************************************
*/

void
HyperTextUnsetSegmentColor(hw, hs)
HyperTextWidget hw;
HyperSegment * hs;
{
    if (XtIsSubclass((Widget)hw, hyperTextWidgetClass) == FALSE)
	return;

    HyperTextUnhighlightSegment(hw);

    if (hs == NULL)
	return;

    hs->use_color = FALSE;
    if (XtIsRealized((Widget)hw))
	draw_segment(hw, XtWindow(hw), hs, TRUE, FALSE);
}


/*
 ****************************procedure*header*****************************
 * Find the segment by key.
 * Returns (HyperSegment*) which may be NULL.
 *************************************************************************
*/
HyperSegment *
HyperTextFindSegmentByKey(hw, key)
HyperTextWidget hw;
char * key;
{
    HyperLine * hl;
    HyperSegment * hs;

    if (XtIsSubclass((Widget)hw, hyperTextWidgetClass) == FALSE)
	return;

    hl = hw->hyper_text.first_line;
    if (hl == NULL || key == NULL)
	return(NULL);

    for (; hl; hl = hl->next) {
	for (hs = hl->first_segment; hs; hs = hs->next) {
	    if (hs->key != NULL && strcmp(key, hs->key) == 0)
		return(hs);
	}
    }
    return(NULL);
}

/*
 ****************************procedure*header*****************************
 * Finds the hypersegment covering the point <x1,y1>.
 * Returns (HyperSegment*) which may be NULL.
 *************************************************************************
*/
HyperSegment *
HyperTextFindSegment(hw, x1, y1)
HyperTextWidget hw;
int x1, y1;
{
    int i;
    HyperLine * hl;
    HyperSegment * hs;

    if (XtIsSubclass((Widget)hw, hyperTextWidgetClass) == FALSE)
	return;

    /* the first line to be drown */
    i = (y1 >= (int)(hw->hyper_text.top_margin)) ?
	 (y1-(int)(hw->hyper_text.top_margin))/(int)(hw->hyper_text.line_height)
	: -1;

    /* skip lines */
    for (hl = hw->hyper_text.first_line; hl && i > 0; hl = hl->next, i--);

    if (hl == NULL)
	return(NULL);

    for (hs = hl->first_segment; hs; hs = hs->next) {
	 if (((int)(hs->x) <= x1) && ((int)(hs->x) + (int)(hs->w) - 1) >= x1)
	    return(hs);
    }

    return(NULL);
}

/*
 ****************************procedure*header*****************************
 *************************************************************************
*/
void
HyperTextScanSegments(hw, func, client)
HyperTextWidget hw;
void (*func)();
XtPointer client;
{
    int i;
    HyperLine * hl;
    HyperSegment * hs;

    if (XtIsSubclass((Widget)hw, hyperTextWidgetClass) == FALSE || func == NULL)
	return;

    for (hl = hw->hyper_text.first_line; hl; hl = hl->next) {
	for (hs = hl->first_segment; hs; hs = hs->next) {
	     if (hs->key != NULL)
		(*func)(hw, hs, client);
	}
    }
    return;
}

/*
 *************************************************************************
 *
 * Private Procedures - actions for translation manager.
 *
 ***************************private*procedures****************************
 */

/*
 ****************************procedure*header*****************************
 * 
 *************************************************************************
*/
static void
hyper_text_select(hw, ev0)
HyperTextWidget	hw;
XButtonPressedEvent	*ev0;
{
	HyperSegment *hs;
	HyperSegment *hs1;
	Window       junk_w;
	unsigned int kb;
	int          x1, y1, x2, y2;
	XEvent       ev;
	int          x, y;
	Display      *dpy = XtDisplay(hw);
	Window       win = XtWindow(hw);


	/* locate the selected segment */
	x = ev0->x; y = ev0->y;
	hs = HyperTextFindSegment(hw, x, y);

	/* highlight the item if it is a keyword */
	if (hs && hs->key) {
		/* NOTE: should check if selected segment is
		 * the currently highlighted segment.
		 */
		HyperTextHighlightSegment(hw, hs);
	} else 
		return;

	/* grab the pointer */
	while (XGrabPointer(dpy, win, 1, ButtonPressMask | ButtonReleaseMask,
		GrabModeAsync, GrabModeAsync, 0, None, CurrentTime) != GrabSuccess)
			;

	/* move the pointer around until mouse pressed or released */
	/* selected item is changed based on new pointer position  */
	while (1) {
		XPending(dpy);
		if (XCheckMaskEvent(dpy,ButtonPressMask | ButtonReleaseMask,&ev) != 0){
				break;
		} else {
			XQueryPointer(dpy, win, &junk_w, &junk_w, &x1, &y1, &x2, &y2, &kb);
			if (x2 == x && y2 == y)
				continue;

			hs1 = HyperTextFindSegment(hw, x2, y2);
			if (hs1 == hs)
				continue;

			hs = hs1; x = x2; y = y2;
			HyperTextUnhighlightSegment(hw);
			if (hs && hs->key) {
				HyperTextHighlightSegment(hw, hs);
				XSync(dpy, 0);
			}
		}
	}

	/* make sure the pointer is released */
	while (ev.type != ButtonRelease) {
		XNextEvent(XtDisplay(hw), &ev);
	}

	/* ungrab button */
	XUngrabPointer(XtDisplay(hw), CurrentTime);
	XSync(XtDisplay(hw), 0);

	/* check if the released button is the same as the original pressed one */
	if (ev0->button != ev.xbutton.button) {
		HyperTextUnhighlightSegment(hw);
		return;
	}

	/* finally, check we need to do call back -- iff keyword segment */
	if (hs && hs->key) {
		HyperSegment hs_rec;

		hs1 = HyperTextGetHighlightedSegment(hw);
		if (hs1 != hs)
			HyperTextUnhighlightSegment(hw);

		/* grey on */
		hw->hyper_text.highlight = hs;
		hw->hyper_text.is_busy = TRUE;
		hs_rec = *hs;
		draw_segment(hw, win, hs, FALSE, TRUE);
		XtCallCallbacks((Widget)hw, XtNselect, (XtPointer) hs);

		/*
		 * If the widget or the segment is destroyed,
		 * hs's value will be different.
		 */
		if ((hw->hyper_text.is_busy == TRUE) &&
			hs_rec.text == hs->text &&
			hs_rec.x == hs->x &&
			hs_rec.y == hs->y &&
			hs_rec.w == hs->w &&
			hs_rec.h == hs->h) {
			hw->hyper_text.highlight = NULL;
			draw_segment(hw, win, hs, TRUE, FALSE);
		}
	}
} /* end of hyper_text_select */


/*************** widget class procedures ***********************/

/*ARGSUSED*/    /* make lint happy */
/*
 ****************************procedure*header*****************************
 *  initialize procedure
 *************************************************************************
*/
static void
init(request, new)
Widget request;
Widget new;
{
	Widget parent = XtParent(new);
	HyperTextWidget hw = (HyperTextWidget) new;

	gc_init(hw);
	get_italic_font(hw);

	hw->hyper_text.first_line = NULL;
	hw->hyper_text.last_line = NULL;
	hw->hyper_text.highlight = NULL;
	hw->hyper_text.line_height = (hw->hyper_text.font->max_bounds.ascent +
		       hw->hyper_text.font->max_bounds.descent) +
		       SPACE_ABOVE + SPACE_BELOW;

	if (hw->primitive.font_list)
		hw->hyper_text.tab_width = OlTextWidth(hw->primitive.font_list,
								(unsigned char *)"nnnnnnnn", 8);
	else
		hw->hyper_text.tab_width = XTextWidth(hw->hyper_text.font,
								"nnnnnnnn", 8);

	hw->hyper_text.window_gravity_set = FALSE;
	hw->hyper_text.is_busy = FALSE;
	hw->hyper_text.src_copy = NULL;

	set_line(hw);
	layout(hw, /* do adjust size */ FALSE);

	/* reset width and height if necessary */
	if (hw->core.width == 0) {
		hw->core.width = hw->hyper_text.w0;
	}

	if (hw->core.height == 0) {
		hw->core.height = hw->hyper_text.h0;
	}

	/*
	 * Check if widget is created inside a scrolled window.  If so,
	 * set the computeGeometries callback for the scrolled window.
	 */
	if (XtIsSubclass(parent, scrolledWindowWidgetClass) == True)
		XtVaSetValues(parent, XtNcomputeGeometries, ViewSizeChanged, NULL);

} /* end of init */

/*
 ****************************procedure*header*****************************
 * 
 *************************************************************************
*/
static void
destroy(w)
Widget w;
{
	HyperTextWidget hw = (HyperTextWidget) w;

	XtRemoveAllCallbacks((Widget)hw, (String)XtNselect);

	HyperLineFree(hw, hw->hyper_text.first_line);
	hw->hyper_text.first_line = NULL;
	hw->hyper_text.last_line = NULL;
	hw->hyper_text.is_busy = FALSE;
	hw->hyper_text.highlight = NULL;

	if (XtIsRealized(w) && hw->hyper_text.gc)
		XFreeGC(XtDisplay(w), hw->hyper_text.gc);

	if (hw->hyper_text.src_copy)
		free(hw->hyper_text.src_copy);
}

/*
 ****************************procedure*header*****************************
 * 
 *************************************************************************
*/
static void
resize(w)
Widget w;
{
}

/*
 ****************************procedure*header*****************************
 * 
 *************************************************************************
*/
static void
expose(w, ev, region)
Widget w;
XExposeEvent *ev;
Region region;
{
    HyperTextWidget hw = (HyperTextWidget) w;
    int x, y, width, height;

    if (ev != NULL) {
	x = ev->x; y = ev->y; width = ev->width; height = ev->height;
    }
    else {
	x = 0; y = 0; width = w->core.width; height = w->core.height;
    }

    HyperTextExpose(hw, x, y, width, height);
    if (w->core.sensitive == FALSE || w->core.ancestor_sensitive == FALSE) {
	x_fill_grids(XtDisplay(w), XtWindow(w), w->core.background_pixel,
		     x, y, width, height);
    }
}

/*
 ****************************procedure*header*****************************
 * 
 *************************************************************************
*/

static Boolean
set_values(old, request, new)
HyperTextWidget	old,
				request,
				new;
{
	Boolean		was_changed = FALSE;
	Boolean		need_layout = FALSE;
	HyperSegment	*hs;

	/*
	 * string, file, source_type,
	 * font, left_margin, top_margin, right_margin, bot_margin,
	 * key_color, font_color
	 */

	HyperTextUnhighlightSegment(new);

	if (new->core.sensitive != old->core.sensitive ||
		new->core.ancestor_sensitive != old->core.ancestor_sensitive)

			was_changed = TRUE;

	/* see if original input data has changed */

	if (new->hyper_text.source_type != old->hyper_text.source_type) {
		/* we have to reconstruct the hyper line field */
		set_line(new);

		new->hyper_text.highlight = NULL;
		new->hyper_text.is_busy = NULL;
		need_layout = TRUE;
		was_changed = TRUE;

	} else if (new->hyper_text.source_type == OL_DISK_SOURCE &&
		STR_DIFF(new->hyper_text.file, new->hyper_text.src_copy)) {

		/* we have to reconstruct the hyper line field */
		set_line(new);

		new->hyper_text.highlight = NULL;
		new->hyper_text.is_busy = NULL;
		need_layout = TRUE;
	 	was_changed = TRUE;

	} else if (new->hyper_text.source_type == OL_STRING_SOURCE &&
		STR_DIFF(new->hyper_text.string, new->hyper_text.src_copy)) {

		set_line(new);
		new->hyper_text.highlight = NULL;
		new->hyper_text.is_busy = NULL;
		need_layout = TRUE;
		was_changed = TRUE;
	}

	/* see if and size related resource has been changed */
	if (new->hyper_text.font          != old->hyper_text.font ||
	    new->primitive.font           != old->primitive.font ||
	    new->primitive.font_list      != old->primitive.font_list ||
	    new->hyper_text.left_margin   != old->hyper_text.left_margin ||
	    new->hyper_text.top_margin    != old->hyper_text.top_margin ||
	    new->hyper_text.right_margin  != old->hyper_text.right_margin ||
	    new->hyper_text.bot_margin    != old->hyper_text.bot_margin) {

		new->hyper_text.line_height =
				(new->hyper_text.font->max_bounds.ascent +
				new->hyper_text.font->max_bounds.descent) +
				SPACE_ABOVE + SPACE_BELOW;

		if (new->primitive.font_list)
			new->hyper_text.tab_width=OlTextWidth(new->primitive.font_list,
				    (unsigned char *)"nnnnnnnn", 8);
		else
			new->hyper_text.tab_width = XTextWidth(new->hyper_text.font,
				    "nnnnnnnn", 8);

		need_layout = TRUE;
		was_changed = TRUE;
	}

	/* see if any color related resource has been changed */
	if (new->primitive.font_color != old->primitive.font_color ||
		new->hyper_text.key_color != old->hyper_text.key_color) {
		was_changed = TRUE;
	}

	if (need_layout)
		layout(new, /* do adjust size */ TRUE);

	/* if widget currently has focus, assign input focus to
	 * the first command segment */
	if (old->hyper_text.highlight) {
		hs = GetFirstCmdSegment(new, True);
		if (hs)
			HyperTextHighlightSegment(new, hs);
	}

	return(was_changed);
} /* end of set_values */

/*
 ****************************procedure*header*****************************
 * 
 *************************************************************************
*/
static XtGeometryResult
QueryGeom(w, intended, reply)
Widget w;
XtWidgetGeometry *intended;
XtWidgetGeometry *reply;
{
	XtGeometryResult result;

	/* printf("wanted w=%d h=%d\n", intended->width, intended->height); */
	result = XtGeometryYes;
	return(result);
}


/****************************************************************
 *
 * Private Procedures - the rest of it.
 *
 ****************************************************************/

/*
 ****************************procedure*header*****************************
 * 
 *************************************************************************
*/
static void
HyperTextExpose(hw, x1, y1, w1, h1)
HyperTextWidget hw;
int x1, y1;
int w1, h1;
{
    int i, j;
    int n;

    /* the first line to be drawn */
    i = (y1 >= (int)(hw->hyper_text.top_margin)) ?
	 (y1-(int)(hw->hyper_text.top_margin))/(int)(hw->hyper_text.line_height)
	 : 0;

    /* the number of lines to be drawn */
    j = ((y1+h1-1) >= (int)(hw->hyper_text.top_margin)) ?
	 (y1+h1-1-(int)(hw->hyper_text.top_margin))/(int)(hw->hyper_text.line_height)
	 : 0;

    n = j - i + 1;

    /* draw line[i] to lines[i+n-1] within the [x1,x1+w1-1] boundary */
    draw_lines(hw, i, n, x1, x1+w1-1);
}


/*
 ****************************procedure*header*****************************
 * 
 *************************************************************************
*/
static HyperLine *
HyperTextFindLine(hw, y1)
HyperTextWidget hw;
int y1;
{
    int i;
    HyperLine * hl;

    i = (y1 >= (int)(hw->hyper_text.top_margin)) ?
	(y1-(int)(hw->hyper_text.top_margin))/(int)(hw->hyper_text.line_height)
	: -1;

    /* skip lines */
    for (hl = hw->hyper_text.first_line; hl && i > 0; hl = hl->next, i--);

    return(hl);
}

/*
 ****************************procedure*header*****************************
 * Initialises hw->hyper_text.gc. 
 *************************************************************************
*/
static void
gc_init(hw)
HyperTextWidget hw;
{
#define stipple_width 16
#define stipple_height 16
static unsigned char stipple_bits[] = {
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa};

	XGCValues values;
	Pixmap    bm = 0;
	Display   *dpy = XtDisplay(hw);

	bm = XCreatePixmapFromBitmapData(dpy,
				DefaultRootWindow(dpy),
				(char *)stipple_bits,
				stipple_width,
				stipple_height,
				0, /* foreground always off */
				1, /* background always on  */
				(unsigned) 1);

	values.foreground = hw->primitive.font_color;
	values.font = hw->hyper_text.font->fid;
	values.stipple = bm;

	hw->hyper_text.gc = XCreateGC(dpy, RootWindowOfScreen(XtScreen(hw)),
			(unsigned) GCForeground | GCFont | GCStipple, &values);
} /* end of gc_init */

/*
 ****************************procedure*header*****************************
 * This routine is called to get the italics font to be used to display
 * terms in a help window.  It initializes italic_fid if it's NULL;
 * otherwise, it simply returns.
 *************************************************************************
*/
static void
get_italic_font(hw)
HyperTextWidget hw;
{
	XFontStruct *italic_font;
	XrmValue    from;
	XrmValue    to;

	if (italic_fid != NULL)
		return;

	from.size = strlen(olDefaultItalicFont);
	from.addr = olDefaultItalicFont;
	to.size   = sizeof(XFontStruct *);
	to.addr   = (XtPointer)&italic_font; 

	if (XtConvertAndStore((Widget)hw, XtRString, &from, XtRFontStruct, &to)) {
		italic_fid = italic_font->fid;
	} else {
		italic_fid = hw->hyper_text.font->fid;
	}
} /* end of get_italic_font */

/*------------ layout hyper text positions -------------*/

/*
 ****************************procedure*header*****************************
 * 
 *************************************************************************
*/
static int
check_max_lines(hw)
HyperTextWidget hw;
{
    register HyperLine * hl;
    register HyperLine * next;
    register unsigned int n;
    register unsigned int max;
    Boolean shrinked;

    /* no need to shrink if no limit set */
    max = hw->hyper_text.max_lines;
    if (max == 0)
	return(0);

    /* compute number of lines */
    n = 0;
    for (hl = hw->hyper_text.first_line; hl; hl = hl->next) n++;

    if (n <= max)
	return(0);

    /* find new head */
    shrinked = n - max;
    hl = hw->hyper_text.first_line;
    while (n > max) {
	next = hl->next;
	hl->next = NULL;
	HyperLineFree(hw, hl);
	hw->hyper_text.last_line = next;
	hl = next;
	n--;
    }
    /* hl != NULL since max > 0 */
    hw->hyper_text.first_line = hl;
    return(shrinked);
}


/*
 ****************************procedure*header*****************************
 * Construct (initialize) the hyper line.
 *************************************************************************
*/
static void
set_line(hw)
HyperTextWidget	hw;
{
	HyperLine	*hl;

	/* free the current hyper line field */
	if (hw->hyper_text.first_line != NULL)
		HyperLineFree(hw, hw->hyper_text.first_line);

	hw->hyper_text.first_line = NULL;
	hw->hyper_text.last_line = NULL;

	/* either init the hyper line from the file or from the string */
	if (hw->hyper_text.source_type == OL_DISK_SOURCE) {

		/* check the string */
		if (hw->hyper_text.file == NULL)
			return;

		if (hw->hyper_text.src_copy)
			free(hw->hyper_text.src_copy);

		hw->hyper_text.src_copy = HtNewString(hw->hyper_text.file);
		hw->hyper_text.first_line =  HyperLineFromFile(hw,
								hw->hyper_text.file);

	} else { /* from string: OL_STRING_SOURCE */

		/* check the string */
		if (hw->hyper_text.string == NULL)
			return;

		if (hw->hyper_text.src_copy)
			free(hw->hyper_text.src_copy);

		hw->hyper_text.src_copy = HtNewString(hw->hyper_text.string);
		hw->hyper_text.first_line =  HyperLineFromString(hw,
								hw->hyper_text.string);
	}

	/* Find the last hyper line and also set the ptr to the
	 * previous hyperline along the way.  Would be more efficient
	 * to set this up as the text is read in but this will do
	 * for now.
	 */
	hw->hyper_text.first_line->prev = NULL;
	hl = hw->hyper_text.first_line->next;

	if (hl == NULL) {
			hw->hyper_text.last_line = hw->hyper_text.first_line;
	} else {
		HyperLine	*save = hl;

		hl->prev = hw->hyper_text.first_line;
		hl = hl->next;
		while (hl) {
			hl->prev = save;
			save = hl;
			hl = hl->next;
			if (hl == NULL)
				hw->hyper_text.last_line = save;
		}
	}

	/* we don't care whether or not the ...From... function failed */
	return;

} /* end of set_line */

/*
 ****************************procedure*header*****************************
 *
 *************************************************************************
*/
/*
	y      _________________________
	y_text _________________________<- space above text
			XXX
			X X             <- ascent
	       ________ XXX ____________
		 ------   X ----------- <- underline
			X X             <- descent
	       ________ XXX ____________
	       _________________________<- space below text

*/
static void
layout(hw, adjust_size_flag)
HyperTextWidget hw;
Boolean adjust_size_flag;
{
    int y;
    int w, w1;
    HyperLine * hl;
    XFontStruct * font = hw->hyper_text.font;
    int line_height = hw->hyper_text.line_height;
    int left_margin = hw->hyper_text.left_margin;
    int tab_width = hw->hyper_text.tab_width;

    w = left_margin;
    hw->hyper_text.h0 = y = hw->hyper_text.top_margin;

    for (hl = hw->hyper_text.first_line; hl ; hl = hl->next) {
	w1 = layout_line(hw, hl, font, line_height, left_margin, tab_width, y);
	w = MAX(w, w1);
	y += hw->hyper_text.line_height;
    }

    hw->hyper_text.w0 = w + hw->hyper_text.right_margin;
    hw->hyper_text.h0 = y + hw->hyper_text.bot_margin;

    if (hw->hyper_text.w0 <= 0) hw->hyper_text.w0 = 1;
    if (hw->hyper_text.h0 <= 0) hw->hyper_text.h0 = 1;

    if (adjust_size_flag)
	adjust_size(hw);
}

/*
 ****************************procedure*header*****************************
 * returns the width required for displaying the line.
 *************************************************************************
*/
static int
layout_line(hw, hl, font, line_height, left_margin, tab_width, y)
HyperTextWidget hw;
HyperLine * hl;
XFontStruct * font;
int line_height;
int left_margin;
int tab_width;
int y;
{
    register HyperSegment * hs;
    int x;

    x = left_margin;

    for (hs = hl->first_segment; hs; hs = hs->next) {
	if (hs->tabs > 0)
	    x  = ((x-left_margin+((int)(hs->tabs) * tab_width))/tab_width)
		 * tab_width + left_margin;
	hs->x = x;
	hs->y = y;
	hs->y_text = y + font->max_bounds.ascent + SPACE_ABOVE;

	if (hw->primitive.font_list)
		hs->w = OlTextWidth(hw->primitive.font_list,
					(unsigned char *)hs->text, hs->len);
	else
		hs->w = XTextWidth(font, hs->text, hs->len);

	hs->h = line_height;
	x += hs->w;
    }
    return(x);
}

/*
 ****************************procedure*header*****************************
 *************************************************************************
*/
static void
adjust_size(hw)
HyperTextWidget hw;
{
    int w, h;
    int w1, h1;
    Widget parent;
    XtGeometryResult requestReturn;

    if (hw->hyper_text.resizable == FALSE)
	return;

    w = hw->hyper_text.w0;
    h = hw->hyper_text.h0;

	/* use core.width and core.height of bulletin board parent,
	 * not scrolled window.
	 */
    parent = XtParent(hw);
    if (XtParent(parent) &&
        XtIsSubclass(XtParent(parent), scrolledWindowWidgetClass)) {
	if (w < (int)(parent->core.width)) w = parent->core.width;
	if (h < (int)(parent->core.height)) h = parent->core.height;
    }

    if (XtIsRealized((Widget)hw) && hw->hyper_text.window_gravity_set == FALSE) {
	XSetWindowAttributes ws;
	unsigned int valuemask;

	/* prevent the window being cleared */
	valuemask = CWBitGravity;
	ws.bit_gravity = NorthWestGravity;
	XChangeWindowAttributes(XtDisplay(hw), XtWindow(hw), valuemask, &ws);

	hw->hyper_text.window_gravity_set = TRUE;
    }

    requestReturn = XtMakeResizeRequest((Widget)hw, w, h,
					(Dimension *)&w1, (Dimension *)&h1);

    switch (requestReturn) {
	case XtGeometryYes: /* Request accepted. */
	    break;
	case XtGeometryNo:  /* Request denied. */
	    break;
	case XtGeometryAlmost: /* Request denied, but willing to take replyBox. */
	    XtMakeResizeRequest((Widget)hw, w1, h1, NULL, NULL);
	    break;
	case XtGeometryDone:    /* Request accepted and done. */
	    break;
    }
}

/*------------ draw hyper text -------------*/

/*
 ****************************procedure*header*****************************
 *************************************************************************
*/
static void
draw_lines(hw, i, n, x1, x2)
HyperTextWidget hw;
int i, n;
int x1, x2;
{
    HyperLine *hl;

    /* skip lines */
    for (hl = hw->hyper_text.first_line; hl && i > 0; hl = hl->next, i--);

    /* draw lines */
    for (; hl && n > 0; hl = hl->next, n--)
	draw_line1(hw, XtWindow(hw), hl, x1, x2);
}


/*
 ****************************procedure*header*****************************
 *************************************************************************
*/
static void
draw_line1(hw, win, hl, x1, x2)
HyperTextWidget hw;
Window          win;
HyperLine       *hl;
int             x1, x2;
{
    register HyperSegment *hs;

    /* skip segments that are at left of x1 */
    for (hs = hl->first_segment;
	     hs && (((int)(hs->x) + (int)(hs->w) - 1) < x1);
	     hs = hs->next)
			;

    /* draw segments that are at left of x2 */
    for (; hs && (hs->x < x2); hs = hs->next)
		draw_segment(hw, win, hs, FALSE, FALSE);
}

/*
 ****************************procedure*header*****************************
 *************************************************************************
*/

static void
draw_segment(hw, win, hs, clear_flag, busy_flag)
HyperTextWidget hw;
Window          win;
HyperSegment    *hs;
Boolean         clear_flag;
Boolean         busy_flag;
{
	Pixel   color;
	Screen  *screen = XtScreen(hw);
	Display *dpy = XtDisplay(hw);

	if (hs->len == 0)
		return;

	/* select the right color */
	if (IsColor(screen) == FALSE) {
		if (hw->core.background_pixel == BLACK) {
			color = WHITE;
		} else {
			color = BLACK;
		}

	} else if (hs->use_color == TRUE) { /* override what ever has been set */
		color = hs->color;

	} else if (hs->key != NULL) {  /* use default key_color */
		color = hw->hyper_text.key_color;

	} else {                       /* use default foreground color */
		color = hw->primitive.font_color;
	}

	if (clear_flag == TRUE) {
		XClearArea(dpy, win, hs->x, hs->y, hs->w, hs->h, FALSE);
	}

	if (busy_flag) {
		XSetBackground(dpy, hw->hyper_text.gc,
			hw->core.background_pixel);
		XSetFillStyle(dpy, hw->hyper_text.gc, FillOpaqueStippled);
		XFillRectangle(dpy, win, hw->hyper_text.gc,
			hs->x, hs->y, hs->w, hs->h);
		XSetFillStyle(dpy, hw->hyper_text.gc, FillSolid);

	} else if (hs->reverse_video == TRUE) { /* display terms in italics */

		/* set hw->hyper_text.gc.fid to italic_fid */ 
		XSetForeground(dpy, hw->hyper_text.gc, hw->primitive.font_color);
		XSetFont(dpy, hw->hyper_text.gc, italic_fid);

		if (hw->primitive.font_list)
			OlDrawString(dpy, win, hw->primitive.font_list,
				hw->hyper_text.gc, hs->x, hs->y_text,
				(unsigned char *)(hs->text), hs->len);
		else
			XDrawString(dpy, win, hw->hyper_text.gc, hs->x, hs->y_text,
				hs->text, hs->len);

		/* set hw->hyper_text.gc.fid back to hw->hyper_text.font->fid */ 
		XSetFont(dpy, hw->hyper_text.gc, hw->hyper_text.font->fid);

	} else if (hs->key) { /* draw links */

		/* display links in reverse video on monochrome display */
		if (IsColor(screen) ==  FALSE) {
			XSetForeground(dpy,hw->hyper_text.gc, hw->primitive.font_color);
			XFillRectangle(dpy, win, hw->hyper_text.gc, hs->x, hs->y+1,
					hs->w, hs->h-3);

			XSetForeground(dpy,hw->hyper_text.gc,hw->core.background_pixel);

			if (hw->primitive.font_list)
				OlDrawString(dpy, win, hw->primitive.font_list,
					hw->hyper_text.gc, hs->x, hs->y_text,
					(unsigned char *)(hs->text), hs->len);
			else
				XDrawString(dpy, win, hw->hyper_text.gc, hs->x, hs->y_text,
					hs->text, hs->len);
		} else {
			/* display link in hw->hyper_text.key_color */
			XSetForeground(dpy, hw->hyper_text.gc,hw->hyper_text.key_color);
			if (hw->primitive.font_list)
				OlDrawString(dpy, win, hw->primitive.font_list,
					hw->hyper_text.gc, hs->x, hs->y_text,
					(unsigned char *)(hs->text), hs->len);
			else
				XDrawString(dpy, win, hw->hyper_text.gc, hs->x, hs->y_text,
					hs->text, hs->len);
		}
	} else {
		/* draw non-command segments */
		XSetForeground(dpy, hw->hyper_text.gc, hw->primitive.font_color);
		if (hw->primitive.font_list)
			OlDrawString(dpy, win, hw->primitive.font_list,
				hw->hyper_text.gc, hs->x, hs->y_text,
				(unsigned char *)(hs->text), hs->len);
		else
			XDrawString(dpy, win, hw->hyper_text.gc, hs->x, hs->y_text,
				hs->text, hs->len);
	}

	/* draw current focus segment */
	if (hs == hw->hyper_text.highlight)
		draw_focus_segment(hw, XtWindow(hw), hs, FALSE);

	if (busy_flag)
		XSync(dpy, 0);

} /* end of draw_segment */

/*
 ****************************procedure*header*****************************
 * Underlines with input focus color an item which has just gained focus
 * and un-underline an item which has just lost focus.
 *
 * Called from HyperTextHighlightSegment() and HyperTextUnhighlightSegment().
 *************************************************************************
*/
static void
draw_focus_segment(hw, win, hs, clear_flag, busy_flag)
HyperTextWidget hw;
Window          win;
HyperSegment    *hs;
Boolean         clear_flag; /* False for highlight, True for unhighlight */
{
	Screen  *screen = XtScreen(hw);
	Display *dpy = XtDisplay(hw);

	if (hs->len == 0)
		return;

	if (clear_flag == TRUE) {
		XSetForeground(dpy, hw->hyper_text.gc, hw->core.background_pixel);
	} else {
		/* If on a color display, use current input focus color;
		 * otherwise, use black or white.
		 */
		if (IsColor(screen) == FALSE) {
			if (hw->core.background_pixel == BLACK) {
				XSetForeground(dpy, hw->hyper_text.gc, WHITE);
			} else {
				XSetForeground(dpy, hw->hyper_text.gc, BLACK);
			}

		} else {
			XSetForeground(dpy, hw->hyper_text.gc,
					hw->primitive.input_focus_color);
		}
	}

	XDrawLine(dpy, win, hw->hyper_text.gc, hs->x,
			hs->y_text+2,hs->x+hs->w-1,hs->y_text+2);

} /* end of draw_focus_segment */

/*
 ****************************procedure*header*****************************
 *
 *************************************************************************
*/
static void
HTButtonHandler(w, ve)
Widget w;
OlVirtualEvent ve;
{
	switch(ve->virtual_name) {
	case OL_SELECT:
		ve->consumed = True;
		if (ve->xevent->type == ButtonPress)
			hyper_text_select((HyperTextWidget)w, ve->xevent);
		else
			; /* button up */
	}
}

/*
 ****************************procedure*header*****************************
 *
 *************************************************************************
*/
static Boolean
ActivateWidget(w, type, call_data)
Widget		w;
OlVirtualName	type;
XtPointer	call_data;
{
	HyperTextWidget	hw = (HyperTextWidget)w;

	switch(type) {
	case OL_SELECTKEY:
	{
		HyperSegment	*hs;
		HyperSegment hs_rec;

		hs = HyperTextGetHighlightedSegment(hw);

		/* no need to check if segment is a command segment */
		/* - it has to be.  Show busy visual.               */
		hw->hyper_text.is_busy = TRUE;
		hs_rec = *hs;
		draw_segment(hw, XtWindow(hw), hs, FALSE, TRUE);

		/* invoke callback */
		XtCallCallbacks((Widget)hw, XtNselect, (XtPointer) hs);

		/*
		 * If the widget or the segment is destroyed,
		 * hs's value will be different.
		 */
		if ((hw->hyper_text.is_busy == TRUE) &&
			hs_rec.text == hs->text &&
			hs_rec.x == hs->x &&
			hs_rec.y == hs->y &&
			hs_rec.w == hs->w &&
			hs_rec.h == hs->h) {
			draw_segment(hw, XtWindow(hw), hs, TRUE, FALSE);
		}
	}
		break;
	default:
		/* Ignore other key presses */
		break;
	}
}

/*
 ****************************procedure*header*****************************
 *
 *************************************************************************
*/
static Widget
TraversalHandler(shell, w, direction, time)
Widget	shell;
Widget	w;
OlDefine	direction;
Time		time;
{
	HyperTextWidget	hw = (HyperTextWidget)w;
	HyperSegment		*hs;

	switch(direction) {

	case OL_MOVELEFT:
	case OL_MULTILEFT:
	case OL_MULTIUP:
	case OL_MOVEUP:
		/* Using current focus item, find and highlight
		 * previous cmd segment.
		 */
		hs = GetPrevCmdSegment(hw);
		if (hs)
			HyperTextHighlightSegment(hw, hs);
		break;

	case OL_MOVERIGHT:
	case OL_MULTIRIGHT:
	case OL_MULTIDOWN:
	case OL_MOVEDOWN:
		/* Using current focus item, find and highlight
		 * next cmd segment.
		 */
		hs = GetNextCmdSegment(hw);
		if (hs)
			HyperTextHighlightSegment(hw, hs);
		break;

	default:
		break;
	}
} /* end of TraversalHandler */

/*
 ****************************procedure*header*****************************
 *
 *************************************************************************
*/
static void
HighlightHandler(w, highlight_type)
Widget w;
OlDefine highlight_type;
{
	HyperTextWidget	hw = (HyperTextWidget)w;
	HyperSegment		*hs;

	if (highlight_type == OL_IN) {
		/* highlight the first segment with hs->key != NULL */
		hs = GetFirstCmdSegment(hw, True);
		HyperTextHighlightSegment(hw, hs);
	} else {
		/* find current segment with focus */
		hs = HyperTextGetHighlightedSegment(hw);
		if (hs)
			HyperTextUnhighlightSegment(hw);
	}

} /* end of HighlightHandler */

/*
 ****************************procedure*header*****************************
 * Returns the ptr to the first cmd segment.
 *************************************************************************
*/
static HyperSegment *
GetFirstCmdSegment(hw)
HyperTextWidget	hw;
{
	int i;
	HyperLine		*hl;
	HyperSegment	*hs;

	if (XtIsSubclass((Widget)hw, hyperTextWidgetClass) == FALSE)
		return(NULL);

	for (hl = hw->hyper_text.first_line; hl; hl = hl->next) {
		for (hs = hl->first_segment; hs; hs = hs->next) {
			if (hs->key != NULL)
				return(hs);
		}
	}
	return(NULL);
} /* end of GetFirstCmdSegment */

/*
 ****************************procedure*header*****************************
 * Returns the ptr to the next cmd segment in the list following the
 * currently highlighted segment.
 *************************************************************************
 */ 
static HyperSegment *
GetNextCmdSegment(hw)
HyperTextWidget	hw;
{
	int i;
	HyperLine		*hl;
	HyperSegment	*hs;
	HyperSegment	*next_hs;
	Boolean		past = False; /* past current focus segment */

	/* Scan each segment in each line for the highlighted segment.
	 * Then look for the next command segment, which could be either
	 * in the same line as the current segment with focus or on another
	 * line.  The flag "past" is used to indicate whether we've scanned
	 * past the current segment with focus. 
	 */
	for (hl = hw->hyper_text.first_line; hl; hl = hl->next) {
		for (hs = hl->first_segment; hs; hs = hs->next) {
			if (hs == hw->hyper_text.highlight) {
				past = True;
				continue;
			} else if (past) {
				if (hs->key != NULL)
					return(hs);
			}
		}
	}
	return(NULL);
} /* end of GetNextCmdSegment */

/*
 ****************************procedure*header*****************************
 * Returns the ptr to the previous cmd segment in the list preceeding the
 * currently highlighted segment.
 *************************************************************************
 */ 
static HyperSegment *
GetPrevCmdSegment(hw)
HyperTextWidget	hw;
{
	int i;
	HyperLine		*hl;
	HyperSegment	*hs;
	HyperSegment	*prev_hs;
	Boolean		hit = False; /* hit current focus segment */

	/* Scan each segment in each line for the highlighted segment
	 * starting from the last segment in a hyper line.  Once the
	 * current segment with focus is found, turn the flag "hit"
	 * on and start looking for the previous command segment.
	 * Keep backtracking until the previous command segment is
	 * found or until the beginning of the text is reached.
	 */
	for (hl = hw->hyper_text.last_line; hl; hl = hl->prev) {
		for (hs = hl->last_segment; hs; hs = hs->prev) {
			if (hit) {
				if (hs->key != NULL)
					return(hs);

			} else if (hs == hw->hyper_text.highlight) {
				hit = True;
				continue;
			}
		}
	}
	return(NULL);
} /* end of GetPrevCmdSegment */

/*
 * ComputeGeometries callback from scrolled window parent of
 * hypertext window.  The purpose of this callback is to resize
 * view vertically only.  The width of the help window shell has
 * to be adjusted by the application to accomodate the presence of
 * a vertical scrollbar.
 */
static void
ViewSizeChanged(w, geom)
Widget w;
OlSWGeometries *geom;
{
	Boolean need_vsb;
	Dimension height;
	Dimension width;

	/* get height of hypertext widget */
	XtVaGetValues(w, XtNwidth, &width, XtNheight, &height, NULL);

	if (height > geom->sw_view_height)
		need_vsb = True;
	else
		need_vsb = False;

	if (height < geom->sw_view_height)
		geom->bbc_height = geom->bbc_real_height = geom->sw_view_height;
	else
		geom->bbc_height = geom->bbc_real_height = height;

	if (width > geom->sw_view_width) {
		geom->bbc_width = geom->bbc_real_width = width;
		/* need to exclude height of horizontal scrollbar to avoid 
		 * an unnecessary vertical scrollbar but only if height is
		 * less than geom->sw_view_height.
		 */
		if (geom->bbc_height == geom->sw_view_height)
			geom->bbc_height = geom->bbc_real_height =
				geom->sw_view_height - geom->hsb_height;
	} else {
		/* need to exclude width of vertical scrollbar to avoid
		 * an unnecessary horizontal scrollbar.
		 */
		if (need_vsb)
			geom->bbc_width = geom->bbc_real_width =
				geom->sw_view_width - geom->vsb_width;
		else
			geom->bbc_width = geom->bbc_real_width = geom->sw_view_width;
	}
} /* end of ViewSizeChanged */
