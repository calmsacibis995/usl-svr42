/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)abbrevstack:AbbrevFMen.c	1.11"
#endif

/*
 *************************************************************************
 *
 * Description:
 *   This file contains the source code for the AbbreviatedButton widget.
 *
 *************************************************************************
 */

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/AbbrevFMeP.h>
#include <Xol/MenuShell.h>
#include <Xol/FButtons.h>

#define ClassName AbbrevFlatMenu
#include <Xol/NameDefs.h>


/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures 
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/

static void AFMGetNormalGC OL_ARGS ((AbbreviatedButtonWidget));
static void GetDimensions OL_ARGS ((Widget, Boolean, Boolean));
static void StartPreview OL_ARGS ((Widget));
static void StopPreview OL_ARGS ((Widget, Boolean));
static void PutUpMenu OL_ARGS ((Widget, Boolean, int, int));
static void TakeDownMenu OL_ARGS ((Widget, int, int));
static void RevertButton OL_ARGS ((Widget));

					/* class procedures		*/

static void AFMInitialize OL_ARGS ((Widget, Widget, ArgList, Cardinal *));
static void AFMDestroy OL_ARGS ((Widget));
static void AFMRedisplay OL_ARGS ((Widget, XEvent *, Region));
static Boolean AFMSetValues OL_ARGS ((Widget, Widget, Widget, ArgList,
				   Cardinal *));
static void AFMHighlightHandler OL_ARGS ((Widget, OlDefine));
static Boolean AFMActivateWidget OL_ARGS ((Widget, OlVirtualName, XtPointer));

					/* action procedures		*/

static void HandleEvent OL_ARGS ((Widget, OlVirtualEvent));
static void HandleKey OL_ARGS ((Widget, OlVirtualEvent));

					/* public procedures		*/
/* There are no public routines */

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

static OlEventHandlerRec
handlers[] = {
    { ButtonPress,	HandleEvent },
    { ButtonRelease,	HandleEvent },
    { EnterNotify,	HandleEvent },
    { LeaveNotify,	HandleEvent },
    { KeyPress,		HandleKey   },
};

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

static char
translations[] = "\
	<FocusIn>:	OlAction() \n\
	<FocusOut>:	OlAction() \n\
	<Key>:		OlAction() \n\
	<BtnDown>:	OlAction() \n\
	<BtnUp>:	OlAction() \n\
\
	<Enter>:	OlAction() \n\
	<Leave>:	OlAction() \n\
";

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

#define AbOFFSET(field)	XtOffsetOf(AbbreviatedButtonRec,		\
					abbreviated_button.field)

static XtResource
resources[] = {

    { XtNpopupMenu, XtCPopupMenu, XtRWidget, sizeof(Widget),
	  AbOFFSET(popup), XtRWidget, (XtPointer)NULL
    },

    { XtNpreviewWidget, XtCPreviewWidget, XtRWidget, sizeof(Widget),
	  AbOFFSET(preview_widget), XtRWidget, (XtPointer)NULL
    },

    { XtNscale, XtCScale, XtRInt, sizeof(int),
	  AbOFFSET(scale), XtRImmediate, (XtPointer)OL_DEFAULT_POINT_SIZE
    },
};
#undef AbOFFSET

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

AbbreviatedButtonClassRec
abbreviatedButtonClassRec = {
  {
	(WidgetClass) &primitiveClassRec,	/* superclass		*/
	"AbbreviatedButton",			/* class_name		*/
	sizeof(AbbreviatedButtonRec),		/* widget_size		*/
	NULL,					/* class_initialize	*/
	NULL,					/* class_part_initialize*/
	FALSE,					/* class_inited		*/
	AFMInitialize,				/* initialize		*/
	NULL,					/* initialize_hook	*/
	XtInheritRealize,			/* realize		*/
	NULL,					/* actions		*/
	0,					/* num_actions		*/
	resources,				/* resources		*/
	XtNumber(resources),			/* num_resources	*/
	NULLQUARK,				/* xrm_class		*/
	TRUE,					/* compress_motion	*/
	TRUE,					/* compress_exposure	*/
	TRUE,					/* compress_enterleave	*/
	FALSE,					/* visible_interest	*/
	AFMDestroy,				/* destroy		*/
	XtInheritResize,			/* resize		*/
	AFMRedisplay,				/* expose		*/
	AFMSetValues,				/* set_values		*/
	NULL,					/* set_values_hook	*/
	XtInheritSetValuesAlmost,		/* set_values_almost	*/
	NULL,					/* get_values_hook	*/
	XtInheritAcceptFocus,			/* accept_focus		*/
	XtVersion,				/* version		*/
	NULL,					/* callback_private	*/
	translations,				/* tm_table		*/
	XtInheritQueryGeometry			/* query_geometry	*/
  },	/* End of CoreClass field initializations */
  {
        True,					/* focus_on_select	*/
	AFMHighlightHandler,			/* highlight_handler	*/
	NULL,					/* traversal_handler	*/
	NULL,					/* register_focus	*/
	AFMActivateWidget,			/* activate		*/
	handlers,				/* event_procs		*/
	XtNumber(handlers),			/* num_event_procs	*/
	OlVersion,				/* version		*/
	NULL					/* extension		*/
  },	/* End of Primitive field initializations */
  {
	NULL,					/* field not used	*/
  }	/* End of AbbreviatedButtonClass field initializations */
}; 

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass abbreviatedButtonWidgetClass =
		(WidgetClass)&abbreviatedButtonClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * AFMGetNormalGC - this routine gets the normal GC for the Abbreviated
 * Flat Menu Button Widget.
 ****************************procedure*header*****************************
 */
static void
AFMGetNormalGC OLARGLIST((afmbw))
    OLGRA( AbbreviatedButtonWidget,	afmbw)
{
    Pixel	focus_color;
    Boolean	has_focus;

    /* Destroy existing GC	*/

    if (afmbw->abbreviated_button.pAttrs != (OlgAttrs *)NULL) {
	OlgDestroyAttrs (afmbw->abbreviated_button.pAttrs);
    }

    focus_color = afmbw->primitive.input_focus_color;
    has_focus = afmbw->primitive.has_focus;

    if (has_focus)
    {
	if (afmbw->primitive.foreground == focus_color ||
	    afmbw->core.background_pixel == focus_color)
	{
	    /* reverse fg and bg. */
	    if (OlgIs3d ())
	    {
		afmbw->abbreviated_button.pAttrs =
		    OlgCreateAttrs (XtScreenOfObject (afmbw),
				    afmbw->core.background_pixel,
				    &(afmbw->primitive.foreground),
				    False, afmbw->abbreviated_button.scale);
	    }
	    else
	    {
		afmbw->abbreviated_button.pAttrs =
		    OlgCreateAttrs (XtScreenOfObject (afmbw),
				    afmbw->primitive.foreground,
				    &(afmbw->core.background_pixel),
				    False, afmbw->abbreviated_button.scale);
	    }
	}
	else
	    afmbw->abbreviated_button.pAttrs =
		OlgCreateAttrs (XtScreenOfObject (afmbw),
				afmbw->primitive.foreground,
				&(focus_color),
				False, afmbw->abbreviated_button.scale);
    }
    else
	afmbw->abbreviated_button.pAttrs =
	    OlgCreateAttrs (XtScreenOfObject (afmbw),
			    afmbw->primitive.foreground,
			    &(afmbw->core.background_pixel),
			    False, afmbw->abbreviated_button.scale);

} /* End of AFMGetNormalGC() */

/*
 ************************************************************
 *
 *  GetDimensions - this function returns the width and height
 *  of the widget visual as function of point size and screen resolution.
 *
 *********************function*header************************
 */

static void
GetDimensions OLARGLIST ((w, change_width, change_height))
    OLARG (Widget,	w)
    OLARG (Boolean,	change_width)
    OLGRA (Boolean,	change_height)
{
    if (change_width == True || change_height == True)
    {
	AbbreviatedButtonWidget	afmbw = (AbbreviatedButtonWidget) w;
	Dimension	width, height;

	OlgSizeAbbrevMenuB (XtScreen (w), afmbw->abbreviated_button.pAttrs,
			    &width, &height);

	if (change_width == True)
	{
	    afmbw->core.width = width;
	}

	if (change_height == True)
	{
	    afmbw->core.height = height;
	}
    }
} /* End of GetDimensions() */

/*
 *************************************************************************
 * StartPreview - initiates previewing
 * This function is called when the select button is pressed, or when
 * it is pressed and the window is entered and power-user mode is on.
 ****************************procedure*header*****************************
 */

static void
StartPreview OLARGLIST ((w))
    OLGRA (Widget,	w)
{
    AbbreviatedButtonWidget	afmbw = (AbbreviatedButtonWidget) w;

    if (!afmbw->abbreviated_button.set)
    {
	OlSetPreviewMode();
	afmbw->abbreviated_button.set	     =
	afmbw->abbreviated_button.previewing = True;
	AFMRedisplay (w, NULL, NULL);

	if (!afmbw->abbreviated_button.popup ||
	    !OlGetMenuDefault(afmbw->abbreviated_button.popup,
			      &afmbw->abbreviated_button.flat,
			      &afmbw->abbreviated_button.dflt_index,
			      (Boolean)True))
	{
	    XDefineCursor (XtDisplayOfObject (w), XtWindowOfObject (w),
			   GetOlQuestionCursor (XtScreenOfObject(w)));
	    afmbw->abbreviated_button.err_flg = True;
	}
	else
	    if (afmbw->abbreviated_button.preview_widget)
	    {
		OlPreviewMenuDefault(afmbw->abbreviated_button.popup, 
			afmbw->abbreviated_button.preview_widget, OL_NO_ITEM);
	    }
    }

} /* End of StartPreview() */

/*
 *************************************************************************
 * StopPreview - terminates previewing
 * This function is called when the select button is released, or when
 * it is pressed and the window is left and power-user mode is on.
 ****************************procedure*header*****************************
 */

static void
StopPreview OLARGLIST ((w, fire))
    OLARG (Widget,	w)
    OLGRA (Boolean,	fire)
{
    AbbreviatedButtonWidget	afmbw = (AbbreviatedButtonWidget) w;

    if (afmbw->abbreviated_button.set)
    {
	if (afmbw->abbreviated_button.err_flg)
	{
	    XDefineCursor (XtDisplayOfObject (w), XtWindowOfObject (w),
			   GetOlStandardCursor (XtScreenOfObject(w)));
	    afmbw->abbreviated_button.err_flg = False;
	}
	else
	{
	    if (fire)
		OlActivateWidget (afmbw->abbreviated_button.flat,
				  OL_SELECTKEY, (XtPointer)
				  (afmbw->abbreviated_button.dflt_index + 1));

	    if (afmbw->abbreviated_button.preview_widget)
		_OlClearWidget (afmbw->abbreviated_button.preview_widget, True);
	}

	afmbw->abbreviated_button.set	     =
	afmbw->abbreviated_button.previewing = False;
	OlResetPreviewMode();
	AFMRedisplay (w, NULL, NULL);
    }

} /* End of StopPreview() */

/*
 *************************************************************************
 * PutUpMenu - posts menu
 * This function is called when the menu button is pressed, or when
 * it is pressed and the window is entered.  Also for select button when
 * not in power-user mode.
 ****************************procedure*header*****************************
 */

static void
PutUpMenu OLARGLIST ((w, stayup, x, y))
    OLARG (Widget,	w)
    OLARG (Boolean,	stayup)
    OLARG (int,		x)
    OLGRA (int,		y)
{
    AbbreviatedButtonWidget	afmbw = (AbbreviatedButtonWidget) w;
    Position	root_x, root_y;
    XRectangle	rect;

    if (!afmbw->abbreviated_button.set)
    {
	afmbw->abbreviated_button.set = True;
	AFMRedisplay (w, NULL, NULL);
	if (!afmbw->abbreviated_button.popup)
	{
	    XDefineCursor (XtDisplayOfObject (w), XtWindowOfObject (w),
			   GetOlQuestionCursor (XtScreenOfObject(w)));
	    afmbw->abbreviated_button.err_flg = True;
	}
	else
	{
	    XtTranslateCoords (XtParent (w), afmbw->core.x, afmbw->core.y,
			       &root_x, &root_y);
	    rect.x = root_x;
	    rect.y = root_y;
	    rect.width = afmbw-> core.width;
	    rect.height = afmbw-> core.height;
	    OlPopupMenu (afmbw->abbreviated_button.popup, w, RevertButton,
		       &rect, AbbrevDropDownAlignment, True, XtWindow(w),
		       (Position)x, (Position)y);
	    if (stayup)
		OlSetStayupMode ();
	}

	/* This AddGrab must be done after the menu is popped up.  The menu
	 * code will do an XtAddGrab (w, True, True), but the abbreviated
	 * menu button must be in the modal cascade to get enter/leave
	 * events.  The Menu code will remove this grab when the root menu
	 * is brought down.
	 */
	XtAddGrab (w, False, False);
    }
} /* End of PutUpMenu() */

/*
 *************************************************************************
 * TakeDownMenu - unposts menu
 * This function is called when the menu button is pressed and the
 * window is left.  Also for select button when not in power-user mode.
 * The menu is unposted only if the window is exited from the top or left.
 ****************************procedure*header*****************************
 */

static void
TakeDownMenu OLARGLIST ((w, x, y))
    OLARG (Widget,	w)
    OLARG (int,		x)
    OLGRA (int,		y)
{
    AbbreviatedButtonWidget	afmbw = (AbbreviatedButtonWidget) w;

    if (afmbw->abbreviated_button.set)
    {
	if (afmbw->abbreviated_button.err_flg)
	{
	    XDefineCursor (XtDisplayOfObject (w), XtWindowOfObject (w),
			   GetOlStandardCursor (XtScreenOfObject(w)));
	    afmbw->abbreviated_button.err_flg = False;

	    afmbw->abbreviated_button.set = False;
	    AFMRedisplay (w, NULL, NULL);
	    XtRemoveGrab (w);
	}
	else
	{
	    if (x < 0 || y < 0)
	    {
		/* The menu code will remove the grab that we
		 * added and call us to redraw the menu button.
		 * So, just unpost the menu.
		 */
#ifdef DEBUG
		OlPopdownCascade (afmbw->abbreviated_button.popup, False,
				"TakeDownMenu");
#else
		OlPopdownCascade (afmbw->abbreviated_button.popup, False);
#endif
	    }
	}
    }

} /* End of TakeDownMenu() */

/*
 *************************************************************************
 * RevertButton - redraws abbreviated menu button after menu is unposted
 ****************************procedure*header*****************************
 */

static void
RevertButton OLARGLIST ((w))
    OLGRA (Widget,	w)
{
    AbbreviatedButtonWidget	afmbw = (AbbreviatedButtonWidget) w;

    afmbw->abbreviated_button.set = False;
    AFMRedisplay (w, NULL, NULL);
} /* End of RevertButton() */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * Initialize - Initializes the AbbrevMenuButton Instance.  Any conflicts 
 * between the "request" and "new" widgets should be resolved here.
 ****************************procedure*header*****************************
 */

void
AFMInitialize OLARGLIST ((request, new, args, num_args))
    OLARG (Widget,	request)	/* What user wants		*/
    OLARG (Widget,	new)		/* What user gets, so far....	*/
    OLARG (ArgList,	args)
    OLGRA (Cardinal *,	num_args)
{
    AbbreviatedButtonWidget	afmbw = (AbbreviatedButtonWidget) new;

    afmbw->abbreviated_button.pAttrs = (OlgAttrs *) NULL;
    AFMGetNormalGC (afmbw);

    GetDimensions (new, afmbw->core.width == 0, afmbw->core.height == 0);
    afmbw->core.border_width = 0;

    afmbw->abbreviated_button.set = False;
    afmbw->abbreviated_button.err_flg = False;

    if (!afmbw->abbreviated_button.popup ||
	!XtIsSubclass(afmbw->abbreviated_button.popup,
			popupMenuShellWidgetClass))
    {
	OlVaDisplayWarningMsg(XtDisplay(afmbw),
			      OleNfileAbbrevFMen,
			      OleTmsg1,
			      OleCOlToolkitWarning,
			      OleMfileAbbrevFMen_msg1,
			      afmbw->core.name);
	afmbw->abbreviated_button.popup = NULL;
    }

} /* End of AFMInitialize() */

/*
 *************************************************************************
 * Destroy - free the GCs stored in Abbreviated Flat MenuButton widget
 * and remove callbacks.
 ****************************procedure*header*****************************
 */
static void
AFMDestroy OLARGLIST ((w))
    OLGRA (Widget,	w)
{
    OlgDestroyAttrs (((AbbreviatedButtonWidget) w)->abbreviated_button.pAttrs);

} /* End of AFMDestroy() */

/*
 *************************************************************************
 * Redisplay - this routine draws the Abbreviated Menu Button.  The
 * image will be as big as the window.
 ****************************procedure*header*****************************
 */

static void
AFMRedisplay OLARGLIST ((w, xevent, region))
    OLARG (Widget,	w)
    OLARG (XEvent,	*xevent)
    OLGRA (Region,	region)
{
    AbbreviatedButtonWidget	afmbw;
    unsigned			flags;

    if (!XtIsRealized (w))
	return;

    afmbw = (AbbreviatedButtonWidget) w;
    
    /* the button is draw as set if it really is set
     * or if it is 2-D and has input focus and the input
     * focus color conflicts with either the foreground or
     * background color.
     */
    if (afmbw->abbreviated_button.set ||
	(!OlgIs3d () && afmbw->primitive.has_focus &&
	 (afmbw->primitive.input_focus_color ==
	  afmbw->core.background_pixel ||
	  (afmbw->primitive.input_focus_color ==
	   afmbw->primitive.foreground))))
	flags = AM_SELECTED;
    else
	flags = AM_NORMAL;
    
    OlgDrawAbbrevMenuB (XtScreen (w), XtWindow (w),
			afmbw->abbreviated_button.pAttrs, 0, 0, flags);

} /* End of AFMRedisplay() */

/*
 *************************************************************************
 * SetValues - used to set resources associated with the
 * AbbreviatedButtonPart.
 ****************************procedure*header*****************************
 */

static Boolean
AFMSetValues OLARGLIST ((current, request, new, args, num_args))
    OLARG (Widget,	current)
    OLARG (Widget,	request)
    OLARG (Widget,	new)
    OLARG (ArgList,	args)
    OLGRA (Cardinal *,	num_args)
{
    Boolean			redisplay = False;
    AbbreviatedButtonWidget	cafmbw = (AbbreviatedButtonWidget) current;
    AbbreviatedButtonWidget	nafmbw = (AbbreviatedButtonWidget) new;

    if (nafmbw->primitive.foreground != cafmbw->primitive.foreground ||
	nafmbw->core.background_pixel != cafmbw->core.background_pixel ||
	nafmbw->primitive.input_focus_color !=
		cafmbw->primitive.input_focus_color)
    {
	AFMGetNormalGC (nafmbw);
	redisplay = True;
    }

    if (nafmbw->abbreviated_button.popup != cafmbw->abbreviated_button.popup)
    {
	char	msg [100];

	/* There is a problem if the menu is pinned.  But don't worry about
	 * this now.
	 */

	if (!nafmbw->abbreviated_button.popup ||
	    !XtIsSubclass(nafmbw->abbreviated_button.popup,
				popupMenuShellWidgetClass))
	{

	  OlVaDisplayWarningMsg(XtDisplay(nafmbw),
				OleNfileAbbrevFMen,
				OleTmsg1,
				OleCOlToolkitWarning,
				OleMfileAbbrevFMen_msg1,
				nafmbw->core.name);
	  nafmbw->abbreviated_button.popup = NULL;
	}

	if (nafmbw->abbreviated_button.set)
	{
	    if (nafmbw->abbreviated_button.previewing)
	    {
		StopPreview (current, False);
		StartPreview (new);
	    }
	    else
	    {
		TakeDownMenu (current, -1, 0);
		PutUpMenu (new, True, 0, 0);
	    }
	}
    }

    return (redisplay);

} /* End of AFMSetValues() */

/*
 *************************************************************************
 * HighlightHandler - changes the colors when this widget gains or loses
 * focus.
 ****************************procedure*header*****************************
 */

static void
AFMHighlightHandler OLARGLIST ((w, type))
    OLARG (Widget,	w)
    OLGRA (OlDefine,	type)
{
    AFMGetNormalGC ((AbbreviatedButtonWidget) w);
    AFMRedisplay (w, NULL, NULL);
} /* End of AFMHighlightHandler() */

/*
 *************************************************************************
 * ActivateWidget - this routine provides the external interface for
 * others to activate this widget indirectly.
 ****************************procedure*header*****************************
 */

static Boolean
AFMActivateWidget OLARGLIST ((w, type, data))
    OLARG( Widget,		w)
    OLARG( OlVirtualName,	type)
    OLGRA( XtPointer,		data)
{
    Boolean	ret_val = False;

    switch (type) {
    case OL_SELECTKEY:
	if (_OlSelectDoesPreview (w))
	{
	    ret_val = True;
	    StartPreview (w);
	    StopPreview (w, True);
	    break;
	}
	/* Fall Through! */

    case OL_MENUKEY:
	ret_val = True;
	if (((AbbreviatedButtonWidget) w)->abbreviated_button.popup)
	    PutUpMenu (w, True, 0, 0);
	break;
    }

    return (ret_val);

} /* End of AFMActivateWidget() */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/*
 *************************************************************************
 * HandleEvent - handles all button press/release and enter/leave events
 ****************************procedure*header*****************************
 */

static void
HandleEvent OLARGLIST ((w, ve))
    OLARG (Widget,	w)
    OLGRA (OlVirtualEvent,	ve)
{
    if ((ve->xevent->type == EnterNotify || ve->xevent->type == LeaveNotify) &&
	ve->xevent->xcrossing.mode != NotifyNormal)
	return;		/* Ignore enter events generated by grabs */

    switch (ve->virtual_name) {
    case OL_SELECT:
	if (_OlSelectDoesPreview (w))
	{
	    ve->consumed = True;
	    switch (ve->xevent->type) {
	    case ButtonPress:
		XUngrabPointer (XtDisplay (w), CurrentTime);
		/* Fall Through! */

	    case EnterNotify:
		StartPreview (w);
		break;

	    case ButtonRelease:
	    case LeaveNotify:
		StopPreview (w, ve->xevent->type == ButtonRelease);
		break;
	    }
	    break;
	}
	/* Fall Through! */

    case OL_MENU:
	ve->consumed = True;
	switch (ve->xevent->type) {
	case ButtonPress:
	    XUngrabPointer (XtDisplay (w), CurrentTime);
	    PutUpMenu (w, False, ve->xevent->xbutton.x_root,
		       ve->xevent->xbutton.y_root);
	    break;

	case EnterNotify:
	    PutUpMenu (w, False, ve->xevent->xcrossing.x_root,
		       ve->xevent->xcrossing.y_root);
	    break;

	case LeaveNotify:
	    TakeDownMenu (w, ve->xevent->xcrossing.x, ve->xevent->xcrossing.y);
	    break;

	case ButtonRelease:
	    if (((AbbreviatedButtonWidget) w)->abbreviated_button.err_flg)
		TakeDownMenu (w, 0, 0);
	    break;
	}
	break;
    }

} /* End of HandleEvent() */

/*
 *************************************************************************
 * HandleKey - handles all keypresses for this widget
 ****************************procedure*header*****************************
 */

static void
HandleKey OLARGLIST ((w, ve))
    OLARG (Widget,	w)
    OLGRA (OlVirtualEvent,	ve)
{
    if (ve->virtual_name == OL_MOVEDOWN)
    {
	ve->consumed = True;
	OlActivateWidget(w, OL_MENUKEY, (XtPointer)NULL);
    }

} /* End of HandleKey() */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/* There are no public routines */
