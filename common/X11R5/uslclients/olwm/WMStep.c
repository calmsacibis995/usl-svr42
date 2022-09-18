/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olwm:WMStep.c	1.58"
#endif

/*
 *************************************************************************
 *
 * Description:
 *	This file contains the code used to support the WmstepWidget.
 *
 ******************************file*header********************************
 */

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>

#include <Xol/OpenLookP.h>
#include <Xol/Stub.h>
#include <wm.h>
#include <Xol/VendorI.h>
#include <WMStepP.h>
#include <Xol/WSMcomm.h>
#include <Extern.h>

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

static void	AddDeleteWMStep OL_ARGS((Widget,Boolean));

					/* class procedures		*/

static void	Destroy OL_ARGS((Widget));
static void     Initialize OL_ARGS((Widget, Widget, ArgList, Cardinal *));

static void	Realize OL_ARGS((Widget, Mask *, XSetWindowAttributes *));
static void	Redisplay OL_ARGS((Widget, XEvent *, Region));

					/* action procedures		*/

					/* public procedures		*/
extern int	IsWMStepChild OL_ARGS((Window));
extern void	MoveFromWithdrawn OL_ARGS((WMStepWidget));
extern void	MoveToWithdrawn OL_ARGS((WMStepWidget));

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#if !defined(POSTMARK)
#define POSTMARK
#endif


/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

static char defaultTranslations[] = "<BtnDown>:  button()\n\
				     <BtnDown>,<BtnUp>: buttonup()\n";

static XtActionsRec actionsList[] =	{	{"button", Button },
						{"buttonup", ButtonUp}
					};


/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

static XtResource
resources[] = {
   { XtNwindowAttributes, XtCWindowAttributes, XtRPointer, sizeof(XtPointer),
      XtOffset(WMStepWidget, wmstep.windowAttributes), XtRImmediate, (XtPointer)0 },
   { XtNsaveSet, XtCSaveSet, XtRBoolean, sizeof(Boolean),
      XtOffset(WMStepWidget, wmstep.saveSet), XtRImmediate, (XtPointer)1 },
   { XtNwindow, XtCWindow, XtRWindow, sizeof(Window),
      XtOffset(WMStepWidget, wmstep.window), XtRImmediate, (XtPointer)0 },
   { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
      XtOffset(WMStepWidget, wmstep.foreground_pixel), XtRString, "Black" },
   { XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
/*
      XtOffset(WMStepWidget, wmstep.font), XtRString, (XtPointer)Nlucida },
 */
      XtOffset(WMStepWidget, wmstep.font), XtRImmediate, (XtPointer)NULL},
};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

WMStepClassRec
wmstepClassRec = {
  { /* core fields */
    /* superclass               */      (WidgetClass) &widgetClassRec,
    /* class_name               */      "WMStep",
    /* widget_size              */      sizeof(WMStepRec),
    /* class_initialize         */      NULL,
    /* class_part_initialize    */      NULL,
    /* class_inited             */      FALSE,
    /* initialize               */      Initialize,
    /* initialize_hook          */      NULL,
    /* realize                  */      Realize,
    /* actions                  */      actionsList,
    /* num_actions              */      XtNumber(actionsList),
    /* resources                */      resources,
    /* num_resources            */      XtNumber(resources),
    /* xrm_class                */      NULLQUARK,
    /* compress_motion          */      TRUE,
    /* compress_exposure        */      TRUE,
    /* compress_enterleave      */      FALSE,
    /* visible_interest         */      FALSE,
    /* destroy                  */      Destroy,
    /* resize                   */      NULL,
    /* expose                   */      Redisplay,
    /* set_values               */      NULL,
    /* set_values_hook          */      NULL,
    /* set_values_almost        */      XtInheritSetValuesAlmost,
    /* get_values_hook          */      NULL,
    /* accept_focus             */      XtInheritAcceptFocus,
    /* version                  */      XtVersion,
    /* callback_private         */      NULL,
    /* tm_table                 */      defaultTranslations,
    /* query_geometry           */      NULL,
 },
 { /* wmstepWidgetClassFields */
    /* not_used			*/	NULL
 }
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass wmstepWidgetClass = (WidgetClass)&wmstepClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * AddDeleteWMStep - this routine adds and deletes the wmstep widgets
 * from the global array.
 ****************************procedure*header*****************************
 */
static void
AddDeleteWMStep OLARGLIST((w, add_it))
	OLARG(Widget,	w)
	OLGRA(Boolean,	add_it)
{
	/* When adding to the list, keep members of a group together.
	 * A group is defined by the WM_HINTS.window_group window ID-
	 * two windows are in the same group if they both have the
	 * same "highest window group ID" - search "up" the window group
	 * ID's until you find a window that has the same window_group
	 * as the window ID of itself - that is the highest window group
	 * ID.  This function is called at the END of Initialize(), so
	 * the window_group has already been read from the property.
	 * 
	 */
	if (add_it == True) {
		Window window_group;
		int i;
		if (num_wmstep_kids == num_wmstep_slots) {
			num_wmstep_slots += 4;
			wmstep_kids = (WidgetList)
					XtRealloc((XtPointer) wmstep_kids,
					    num_wmstep_slots * sizeof(Widget));
		}
		/* Is anyone in the list with the same "highest window
		 * group ID ?  The following will also work for the first
		 * step widget added.
		 */
		window_group = find_highest_group((WMStepWidget)w,(Window)0);
		for (i=num_wmstep_kids; i > 0; --i)
			if (window_group == find_highest_group(
					(WMStepWidget)wmstep_kids[i-1],
					 (Window) 0) )
				break;
		if (i == num_wmstep_kids)
			wmstep_kids[num_wmstep_kids++] = w;
		else {
			/* i is one index past where the group member was;
			 * add the step widget to index i, move everyone
			 * from i up.  Move them up to and INCLUDING i.
			 */
			int	j;

			num_wmstep_kids++;
			for(j = num_wmstep_kids-1; j > i; j--)
				wmstep_kids[j] = wmstep_kids[j-1];
				/* You wanted to move i, and leave i-1 alone */
			wmstep_kids[i] = w;
			
		}
	} else {
		Cardinal i;

			/* Find the kid in the list, remove it, and
			 * coalesce the list.				*/

		for (i=0; i < num_wmstep_kids; ++i) {
			if (wmstep_kids[i] == w) {
				--num_wmstep_kids;
				while(i < num_wmstep_kids) {
					wmstep_kids[i] = wmstep_kids[i+1];
					++i;
				}
			}
		}
	}
} /* END OF AddDeleteWMStep() */


/*
 *************************************************************************
 * IsWMStepChild - this routine searches the wmstep_kids[] array for a
 *  WMStepWidget that has window as a child.  Return the index if found,
 *  -1 otherwise.
 ****************************procedure*header*****************************
 */
extern int
IsWMStepChild(w)
Window w;
{
	int i;
	WMStepWidget wm;

	for (i=0; i < num_wmstep_kids; ++i) {
		wm = (WMStepWidget)wmstep_kids[i];
		if (wm->wmstep.window == w)
			return(i);
	}
	return(-1);
}

/* The following ifdef is for moving windows to a withdrawn state while
 * still being under olwm control.
 */
#ifdef WITHDRAWNSTATE
/*
 *************************************************************************
 *
 * MoveToWithdrawn - move a stepwidget to Withdrawn from Normal state
 * or iconic.
 *
 ****************************procedure*header*****************************
 */

extern void
MoveToWithdrawn OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
Screen *screen = XtScreen((Widget)wm);
Display *display = XtDisplay((Widget)wm);
Window window = wmstep->window;
int i, k;

	if (wmstep-> classhint.res_name) {
		free(wmstep-> classhint.res_name);
		wmstep->classhint.res_name = NULL;
	}
	
	if (wmstep-> classhint.res_class) {
		free(wmstep-> classhint.res_class);
		wmstep->classhint.res_class = NULL;
	}

	if (wmstep->xwmhints != NULL) {
		free((char *)(wmstep->xwmhints));
		wmstep->xwmhints = NULL;
	}
	if (wmstep->num_cmap_windows > 0) {
		if (wmstep->subwindow_data != NULL) {
			free((char *)(wmstep->subwindow_data));
			wmstep->subwindow_data = NULL;
		}
		if (wmstep->colormap_windows != NULL) {
			free((char *)(wmstep->colormap_windows));
			wmstep->colormap_windows = NULL;
		}
		wmstep->num_cmap_windows = 0;
	}
	if (wmstep->private_db) {
		OlDestroyInputEventDB(wmstep->private_db);
		wmstep->private_db = NULL;
		if (wmstep->private_keys) {
			XtFree((char *)wmstep->private_keys);
			wmstep->private_keys = NULL;
		}
		for (i=0; i < wmstep->private_buttons_used; i++) {
			if (wmstep->private_buttons[i].helpData) {
			   if (wmstep->private_buttons[i].helpData->menuargs &&
		 wmstep->private_buttons[i].selectProc != OlwmSend_Msg &&
		 wmstep->private_buttons[i].selectProc != OlwmCircle_Down &&
		 wmstep->private_buttons[i].selectProc != OlwmCircle_Up &&
		 wmstep->private_buttons[i].selectProc != OlwmNext_Key &&
		 wmstep->private_buttons[i].selectProc != OlwmPrev_Key) {
#ifdef DEBUG
fprintf(stderr,"Free: (Destroy) helpData.args = %x\n", wmstep->private_buttons[i].helpData->menuargs);
fprintf(stderr, "AND the selectProc=%x\n", wmstep->private_buttons[i].selectProc);
#endif

					XtFree((char *)
				wmstep->private_buttons[i].helpData->menuargs);
				wmstep->private_buttons[i].helpData->menuargs=NULL;
			    }
#ifdef DEBUG
fprintf(stderr,"Free: (Destroy) helpData = %x\n", wmstep->private_buttons[i].helpData);
#endif
			   XtFree((char *)wmstep->private_buttons[i].helpData);
			   wmstep->private_buttons[i].helpData=NULL;
			} /* if */
		} /* for */
		/* Now free the private_buttons data structure */
#ifdef DEBUG
fprintf(stderr,"Free (Destroy): private_buttons=%x\n",wmstep->private_buttons);
#endif
		XtFree((char *)wmstep->private_buttons);
		wmstep->private_buttons = NULL;

		/* One more thing: free the menubutton data structures -
		 * I wish I didn't do it this way!
		 */
		if (wmstep->menu_ext) {
			WMMenuButtonData *mbd = wmstep->menu_ext,
					*next = mbd->next;
			for( ; mbd; mbd = next, next = next->next) {
				if (mbd->label) {
#ifdef DEBUG
fprintf(stderr,"Free (Destroy):label=%x its str=%s\n", mbd->label,
				mbd->label);
#endif
					XtFree((char *)mbd->label);
					mbd->label = NULL;
				}
				if (mbd->accelerator) {
#ifdef DEBUG
fprintf(stderr,"Free (Destroy): accel=%x\n",mbd->accelerator);
#endif
					XtFree((char *)mbd->accelerator);
					mbd->accelerator=NULL;
				}
				if (mbd->args &&
					/* Don't free certain args */
		 		(mbd->menucb != OlwmSend_Msg &&
		 			mbd->menucb != OlwmCircle_Down &&
		 			mbd->menucb != OlwmCircle_Up &&
		 			mbd->menucb != OlwmNext_Key &&
		 			mbd->menucb != OlwmPrev_Key) ) {
#ifdef DEBUG
fprintf(stderr,"Free (Destroy):args=%x\n", mbd->args);
#endif
						XtFree((char *)mbd->args);
						mbd->args=NULL;
				}
				XtFree((char *)mbd);
				mbd = NULL;
			} /* for */	
		} /* if wmstep->menu_ext */
	} /* wmstep->private_db */
	if (currentGUI == OL_MOTIF_GUI) {
		if (wmstep->csinfo){
			XtFree((char *)wmstep->csinfo);
			wmstep->csinfo = NULL;
		}
		if (wmstep->iconimagetopGC) {
			XtReleaseGC((Widget)wm, wmstep->iconimagetopGC);
			wmstep->iconimagetopGC = NULL;
		}
		if (wmstep->iconimagebotGC) {
			XtReleaseGC((Widget)wm, wmstep->iconimagebotGC);
			wmstep->iconimagebotGC = NULL;
		}
		if (wmstep->iconimagefgGC) {
			XtReleaseGC((Widget)wm, wmstep->iconimagefgGC);
			wmstep->iconimagefgGC = NULL;
		}
		if (wmstep->iconimagebgGC) {
			XtReleaseGC((Widget)wm, wmstep->iconimagebgGC);
			wmstep->iconimagebgGC = NULL;
		}
	} /* MOTIF */
} /* MoveToWithdrawn */


/*
 *************************************************************************
 *
 * MoveFromWithdrawn - move a stepwidget from Withdrawn to Normal state
 * or iconic
 *
 ****************************procedure*header*****************************
 */

extern void
MoveFromWithdrawn OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
WMStepPart	*wmstep = (WMStepPart *)&(wm->wmstep);
Screen		*screen = XtScreen((Widget)wm);
Display		*display = XtDisplay((Widget)wm);
Window		window = wmstep->window;
unsigned int	depth;
WMGeometry *    p       = &wmstep-> prev,
	   *	icon_geometry = NULL;
int		checkstate = 0;
OlgTextLbl	labeldata;
Dimension	labelwidth, labelheight;
OlgAttrs	*label_mdm = (OlgAttrs *)NULL;
int		minwidth,
		minheight;
MotifCompResourceInfo	*mcri = (MotifCompResourceInfo *)
						&(motCompRes[TITLE_COMP]);
MotCompAppearanceInfo	*mcai = (MotCompAppearanceInfo *)
							&(mcri->compai);
Boolean		SkipStateCheck = False; /* Motif mode - check if any room
					 * for more icons.
					 */

	wmstep->menu_default = 0;
	wmstep->menu_pane_default = 0;
	wmstep->mbd_menu_default = (WMMenuButtonData *)NULL;
	wmstep->default_cb = MenuChooseDefault;
	wmstep->cascade_default = 0;
	wmstep->hints        = 0;
	wmstep->decorations  = 0;
	wmstep->protocols    = 0;
	wmstep->is_current   = False;
	wmstep->is_grabbed   = False;
	wmstep->olwincolors	= 0;
	wmstep->olwa.flags = 0;
	wmstep->wmdh.flags = 0;
	wmstep->transient_parent = (Window) NULL;
	wmstep->menu_ext = (WMMenuButtonData *)NULL;
	wmstep->private_buttons_avail = wmstep->private_buttons_used = (short)0;
	wmstep->private_db = (OlVirtualEventTable)NULL;
	wmstep->private_buttons = (WMMenuDef *)NULL;
	wmstep->num_mwm_msgs = 0;
	wmstep->icon_pixmap = (Pixmap)NULL;
	wmstep->icon_map_pos_row = wmstep->icon_map_pos_col = -1;

	if (currentGUI == OL_OPENLOOK_GUI) {
		minwidth = MINIMUM_WIDTH;
		minheight = MINIMUM_HEIGHT;
	}
	else {
		minwidth = Mresminwidth;
		minheight = Mresminheight;
	}

	XGetWindowAttributes(display, window, &(wmstep->window_attributes));

	/* For later use, if the window_attributes has a colormap
	 * of None, then set it to the DefaultColormap.
	 * Is that right??
	 */
	if (wmstep->window_attributes.colormap == None)
		wmstep->window_attributes.colormap = DefaultColormapOfScreen(screen);

	/* SAVE the original x, y, width, ht, and border width of the window
	 * before the window manager decorates it.
	 */
	wmstep-> prev.x            = wmstep-> window_attributes.x;
	wmstep-> prev.y            = wmstep-> window_attributes.y;
	wmstep-> prev.width        = wmstep-> window_attributes.width;
	wmstep-> prev.height       = wmstep-> window_attributes.height;
	wmstep-> prev.border_width = wmstep-> window_attributes.border_width;
	depth = wmstep-> window_attributes.depth;

	/* Do GetDecorations first, so that in
	 * GetNormalHints(), when we set up the minimum sizes in motif mode,
	 * we know what to set them to based on if it has a header, resize
	 * corners, a border, a minimize button, and a maximize button.
	 */
	GetAllDecorations(wmstep, display, window);

	/* Some of these decorations may be modified after we get
	 * client specific resources - motif mode only.
	 */
	if (!XGetClassHint(display, window, &wmstep-> classhint)) {
		wmstep-> classhint.res_name  = strdup(wmstep->icon_name);
		wmstep-> classhint.res_class = strdup(wmstep->icon_name);
	}

	if (currentGUI == OL_MOTIF_GUI) {
		if ( (wmstep->csinfo = (MotCSInfo *)
			XtMalloc(sizeof(MotCSInfo)) ) != NULL)
		GetCSResources(wm);
	} 

	/* If in motif mode, then if they don't want a border, leave the
	 * original one on.  Keep in mind that without a border, you
	 * can't have resize corners in motif.
	 */
	if (currentGUI==OL_OPENLOOK_GUI || currentGUI == OL_MOTIF_GUI &&
					(wmstep->decorations & WMBorder))
		XSetWindowBorderWidth(display, window, (unsigned long)0);
	if ( (wmstep->menu_ext = GetMWMMenu(wm)) != NULL)
		NewMenuItems(wm);

	/* Get WM_HINTS property - place in wmstep->xwmhints (defaults provided
	 * if property is not present, and set wmstep->hints &= ~WMHints)
	 * If present, set wmstep->hints |= WMHints.
	 * ** FREE old one
	 */
	GetWMHints(wmstep, display, window);

	/* Based on the way that the window_group field is used in olwm, in
	 * my opinion, and this is just one opinion, if no window mgr hints
	 * are provided, then the window_group field should be set to
	 * the transient parent IF there is a transient parent.   Prior to
	 * this, the window_group field was set to the window ID of it's own
	 * window if no hints were found.
	 * The transient parent is set on the call to GetAllDecorations to
	 * the window ID of the window that this window is transient for,
	 * or NULL.  It is used in several places, sometimes in conjunction
	 * with the window_group (unfortunately).
	 */
	if ( (!(wmstep->hints & WMHints)) && wmstep->transient_parent)
		wmstep->xwmhints->window_group = wmstep->transient_parent;

	/* Get WM_NORMAL_HINTS. place in wmstep->xnormalsize;  If not present, set
	 * wmstep-> hints &= (~WMNormalSizeHints); 
	 * --- NOTE - may be a problem - looks at normal hints width and ht. fields
	 */
	GetNormalHints(wmstep, display, window, screen);

	/* Once we have the normal hints:
	 * if USPosition is set, use prev.x, prev.y;
	 *  else determine the windows position after sizing the window.
	 * if USSize|PSize is set, use prev.width, prev.height.
	 *  else it is our right to resize the window such that the
	 *  title will fit.  Or, should we only pay attention to USSize,
	 *  and have the right to reset the window size if PSize is set.
	 */


	/* Read the WM_PROTOCOLS property.
	 * Fills in the wmstep.protocols.
	 */
	ReadProtocols(wm, wmstep-> child, window);

	GetColormapWindows(wm, display, window);

	/* Read in _OL_WIN_COLORS property */
	SetColors(wm, display, window);

	if (wmstep-> prev.width < minwidth)
		wmstep-> prev.width = minwidth;
	if (wmstep-> prev.height < minheight)
		wmstep-> prev.height = minheight;

	wmstep-> size    = WMNORMALSIZE;
	wmstep-> metrics = GetMetrics(wm);

	/* In following block, user didn't turn on USSize flag in the
	 * WM_NORMAL_HINTS struct. (Don't care about initial size - win mgr
	 * may resize it to fit title)
	 */
	if (!(wmstep-> xnormalsize.flags & USSize)) {
		int textarea;
		int textwid;
		int diff;
		int markwidth  = (HasPushpin(wm) ? ppWidth :
				(wmstep->decorations & WMHasFullMenu ?
					 mmWidth : 0) );

		/* wm->core.width is the total frame's width (including decorations.
	 	* Many of these macros are defined at the top of this file.
	 	* For example, ParentWidth() is passed this widget and the width
	 	* of the window that was mapped, prev.width.  The macros use the
	 	* values in wm->wmstep.metrics->* to get precise values based
	 	* on the scale - GetMetrics() called previously.
	 	*/
		if (currentGUI == OL_OPENLOOK_GUI) {
			wm-> core.width = ParentWidth(wm, wmstep-> prev.width);
			textarea = CoreW(wm) - (Offset(wm) + markwidth +
				2*wmstep->metrics->mark_title_gap + BorderX(wm));
		}
		else { /* Motif */
			int hwidth, vwidth;
	
			/* This number returned may not be perfect... */
			wm-> core.width = MParentWidth(wm, wmstep-> prev.width);
	
			if (Resizable(wm)) {
				vwidth = wmstep->metrics->motifVResizeBorderWidth;
				hwidth = wmstep->metrics->motifHResizeBorderWidth;
			}
			else {
				vwidth = wmstep->metrics->motifVNResizeBorderWidth;
				hwidth = wmstep->metrics->motifHNResizeBorderWidth;
			}
			textarea = wm->core.width - 2 * hwidth - 
				3 * wmstep->metrics->motifButtonWidth - 4;
			if (!(wmstep->decorations & WMMinimize))
				textarea += wmstep->metrics->motifButtonWidth;
			if (!(wmstep->decorations & WMMaximize))
				textarea += wmstep->metrics->motifButtonWidth;
			if (!(HasMenu(wm)))
				textarea += wmstep->metrics->motifButtonWidth;
   		} /* Motif */


		/* Get width of title */

		labeldata.font = wmrcs->font;
		labeldata.accelerator = NULL;
		labeldata.mnemonic = NULL;
		labeldata.flags = (unsigned char) NULL;
		if (currentGUI == OL_MOTIF_GUI &&
					  mcai->fontList)
			labeldata.font_list = mcai->fontList;
		else
			labeldata.font_list = ol_font_list;
		labeldata.label = wmstep->window_name;
		labeldata.justification = TL_CENTER_JUSTIFY;

		label_mdm = OlgCreateAttrs(screen,
				currentGUI == OL_MOTIF_GUI ?
				mcai->foreground : wmrcs->foregroundColor,
				currentGUI == OL_MOTIF_GUI ?
					(OlgBG *)&(mcai->foreground) :
					(OlgBG *)&(wmrcs->foregroundColor) ,
				(Boolean)FALSE, (Dimension)12);

		labeldata.normalGC = OlgGetFgGC(label_mdm);
		if (labeldata.font) /* SHOULDN'T BE NON_NULL */
			XSetFont(display, labeldata.normalGC,
				labeldata.font->fid);
		OlgSizeTextLabel(screen, wmstep->mdm,
				 &labeldata, &labelwidth, &labelheight);


		textwid  =  labelwidth;

		/* When bringing up the window initially from Withdrawn stae,
	 	 * ignore the fact the the title have a ':' in it.
	 	 */
		diff = textwid - textarea;

		/* If title is wider than client text area, reset prev.width.
	 	 * Should we check the maximum size here???
	 	 */

		if (diff > 0)
			wmstep-> prev.width += diff;
      } /* end if (USSize is false ) */

	/* Resize client window to prev.width by prev.height if they changed from
	 * original window attribute dimensions.  This can only happen if
	 * wmstep->windowAttributes == NULL (window manager was running before
	 * window was mapped).
	 */
	if (wmstep->prev.width != wmstep-> window_attributes.width ||
		wmstep->prev.height != wmstep-> window_attributes.height)

	XtResizeWidget(wmstep->child, wmstep->prev.width,
					wmstep->prev.height, 0);

	if ( !(wmstep->xnormalsize.flags & USPosition)) {
		if (CurrentX + wmstep-> prev.width > WidthOfScreen(screen) ||
		   (CurrentY + wmstep->prev.height > HeightOfScreen(screen)))
 			CurrentX = CurrentY = 0;

		wmstep->prev.x = CurrentX;
		wmstep->prev.y = CurrentY;
		CurrentX += Xincrement;
		CurrentY += Yincrement;
	} /* end else if (!USPosition) */

	/* The following block of code adjusts the x,y position of the
	 * decorated window's frame based on the window gravity.
	 * Do this for windows that are mapped AFTER the window manager is
	 * started, and only if the USPosition flag is set on the window's
	 * WM_NORMAL_HINTS property.
	 */
	/*if ( (wmstep->windowAttributes == NULL) &&*/
			if ((wmstep->xnormalsize.flags & PWinGravity) &&
			(wmstep-> xnormalsize.flags & USPosition) ) {
		switch(wmstep->xnormalsize.win_gravity) {
			/* If it's west, the x coord is O.K.;
			 * if it's east it must change;
			 * if it's north, the y coord is O.K.;
			 * if it's south, it must change;
			 * if it's center?  I have to think about it.
			 * Center means x/2 or y/2.  For now, just
			 * assume center means west.
			 */
			case NorthWestGravity:
			case NorthGravity:
			case WestGravity:
			case CenterGravity:
				break;

			case NorthEastGravity:
			case EastGravity:
				/* anchor on right */
				p->x  = (p->x + p->width +
   				  2 * wmstep->window_attributes.border_width) -
					(currentGUI == OL_OPENLOOK_GUI ?
						 ParentWidth(wm,p->width):
						MParentWidth(wm, p->width));
				break;
			case SouthWestGravity:
			case SouthGravity:
				p->y  = (p->y + p->height +
   				  2 * wmstep->window_attributes.border_width) -
					(currentGUI == OL_OPENLOOK_GUI ?
						 ParentHeight(wm,p->height):
						MParentHeight(wm, p->width));
				break;
			case SouthEastGravity:
				p->x  = (p->x + p->width +
   				  2 * wmstep->window_attributes.border_width) -
					(currentGUI == OL_OPENLOOK_GUI ?
						 ParentWidth(wm,p->width) :
						MParentWidth(wm, p->width));
				p->y  = (p->y + p->height +
   				  2 * wmstep->window_attributes.border_width) -
					(currentGUI == OL_OPENLOOK_GUI ?
						 ParentHeight(wm,p->height):
						MParentHeight(wm, p->width));
		} /* switch gravity */
	} /* if */

	if (wmstep-> destroy_child) {
		/* client window was just turned into a handle widget (child),
		 * adjust client widget's core width and height.
		 */
		wmstep-> child-> core.width  = wmstep-> prev.width;
		wmstep-> child-> core.height = wmstep-> prev.height;
	}

	/* Note, p points to wmstep->prev (prev.x,y, etc.).  Here, adjust
	 * overall frame geometry values.
	 */ 
	if (currentGUI == OL_OPENLOOK_GUI) {
		p->width = ParentWidth(wm, p-> width); 
		p->height = ParentHeight(wm, p-> height);
	}
	else {
		p->width = MParentWidth(wm, p-> width); 
		p->height = MParentHeight(wm, p-> height);
	}

	/* We only need to do this when moving from withdrawn to
	 * normal (or iconic, but this must still be done)
	 */
	XtConfigureWidget((Widget)wm, p->x, p->y,
			p->width, p->height, 0);

   /* If this client window plans on starting in iconic state and plans on
    * having an icon widget, we'll do a check first.  This is only a factor
    * in Motif mode, because we can't run out of space in openlook mode.
    * This code is done here and in Initialize.
    */
   if ( currentGUI == OL_MOTIF_GUI && motwmrcs->iconAutoPlace &&
			(wmstep->xwmhints->flags & StateHint &&
			wmstep->xwmhints->initial_state == IconicState ||
			GetWindowState(display, wmstep->window) ==
			IconicState) ) {
		if (!(wmstep->transient_parent) &&
			   find_highest_group(wm, NULL) == window) {
			icon_geometry = IconPosition(wm, wmstep);
			if (icon_geometry != NULL)
				SkipStateCheck = False; /* Default */
			else
				SkipStateCheck = True;
		}
   } /* multiple if */

	if (wmstep-> xwmhints-> flags & StateHint && !SkipStateCheck) {
		switch(wmstep-> xwmhints-> initial_state) {
			case IconicState:
	    /* get position of next icon - IconPosition() places
	     * a WMGeometry struct in wmstep->icon (it will contain the
	     * dimension and position of the icon) AND RETURN A POINTER to
	     * it (so p is now used to point to it).  Set (in wmstep
	     * part of widget) size to ICONIC, get the metrics for it,
	     * and adjust wm widgets core fields.
	     */
			wmstep-> size    = WMICONICNORMAL;
			if (!(wmstep->transient_parent) &&
			   find_highest_group(wm, NULL) == window) {
				if (!icon_geometry)
				   icon_geometry = IconPosition(wm, wmstep);
				wmstep-> metrics = GetMetrics(wm);
				CreateStepIcon(wm, icon_geometry);
			}
			SetWindowState(wm, IconicState, wm->wmstep.window);

		/* In Realize(), I unmap the client window if we think it's in
		* iconic state; it doesn't get unmapped if olwm is running
		* prior to the client being up already (the map is "trapped"
		* by the window manager); but if the client is
		* already up and mapped, then the XUnmapWindow() will generate a real
		* UnmapNotify event; this must be ignored by us or the
		* client will be inadvertantly dumped.
		*/
	    		if (wmstep->windowAttributes)
				wmstep->protocols |= IgnoreUnmap;
				break;
			case ZoomState:
			case DontCareState:
			case NormalState:
			case InactiveState:
			default:
			if (GetWindowState(display, wmstep->window) !=
                                                IconicState) {
				SetWindowState(wm, NormalState, wm->wmstep.window);
				wmstep->size = WMNORMALSIZE;
				checkstate++;
				wmstep->metrics = GetMetrics(wm);
                	}
			else { /* make it iconic */
				if (!(wmstep->transient_parent) &&
					find_highest_group(wm, NULL) ==
								window) {
					if (!icon_geometry)
					  icon_geometry = IconPosition(wm,
								wmstep);
					wmstep->size = WMICONICNORMAL;
					wmstep->metrics = GetMetrics(wm);
					SetWindowState(wm, IconicState,
							wm->wmstep.window);
					if (wmstep->windowAttributes)
					   wmstep->protocols |= IgnoreUnmap;
					CreateStepIcon(wm, icon_geometry);
				}
				else { /* Play it safe - set to
					*normal, checkstate. We'll have to
					*check this window out again
					* in wm.c, Reparent() at a later time.
					* Changes are that it's transient group
					* leader is in iconic state, so
					* the code in Reparent() and
					* StartWindowMappingProcess will
					* take care of it if it isn't taken
					* care of below this switch.
					*/
					checkstate++;
					wmstep->size = WMNORMALSIZE;
					SetWindowState(wm, NormalState,
							wm->wmstep.window);
				}
			} /* else possibly iconic */
			break;
		} /* switch */
	} /* StateHint */
	else { /* No statehint, assume normal */
		checkstate++;
		SetWindowState(wm, NormalState, wm->wmstep.window);
		wmstep->size = WMNORMALSIZE;
		wmstep->metrics = GetMetrics(wm);
		/* Check to see if there is no StateHint but the WM_STATE
		 * property says it is iconic - possible in a non-Xt app
		 * SkipStateCheck tells me it is ok to make another icon
		 * widget.  Any popup (group members) that should be iconified
		 * along with this one will get checked by the above code
		 * (checkstate).
		 */
		if (GetWindowState(display, wmstep->window) ==
				IconicState && !SkipStateCheck) {
                        if (!(wmstep->transient_parent) &&
                                find_highest_group(wm, NULL) ==
                                        window) {
				if (!icon_geometry)
                                	icon_geometry = IconPosition(wm, 
								wmstep);
                                wmstep->size = WMICONICNORMAL;
                                wmstep->metrics = GetMetrics(wm);
                                SetWindowState(wm, IconicState, wm->wmstep.window);
                                if (wmstep->windowAttributes)
                                        wmstep->protocols |= IgnoreUnmap;
                                CreateStepIcon(wm, icon_geometry);
			}
		} /* GetWindowState */
	} /* else no statehint */

	/*wmstep->decorations |= WMNotReparented;*/
	wmstep->focus = 1;
	if (checkstate) {
	/* I just decorated a window in normal state. Does this window have
	 * a leader that is currently iconic??
	 */
	Window leader = wmstep->xwmhints->window_group;
	Cardinal k;
	if  ( ((k = find_leader(leader)) < num_wmstep_kids) ||
		( (wmstep->transient_parent != NULL) &&
		( (k = find_leader(wmstep->transient_parent)) < num_wmstep_kids)) ) {
		WMStepWidget temp = (WMStepWidget)wmstep_kids[k];
		if (IsIconic(temp)) {
			/* Make this one iconic too!! */
			if (wmstep->olwa.win_type != XA_OL_WT_NOTICE(display)) {
				wmstep-> size    = WMICONICNORMAL;
				SetWindowState(wm, IconicState, temp->wmstep.window);
			}

			/* Note we still have a problem here - the
			 * window will still map in Reparent(), so
			 * we must tell Reparent() not to map us.
			 * We have an unused field, window_bw, in
			 * the WMStepPart - set it to 1 << 10.
			 * The exception to this rule is that we
			 * always want NOTICE windows to map.
			 */
		}
	}
	} /* end if (checkstate) */
} /* MoveFromWithdrawn */

#endif


/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * Destroy -
 ****************************procedure*header*****************************
 */
static void
Destroy OLARGLIST((w))
OLGRA(Widget, w)
{
	WMStepWidget wm = (WMStepWidget)w;
	WMStepPart * wmstep = &wm-> wmstep;
	Widget       child  = wmstep-> child;
	int i;
	WMMenuButtonData	*mbd;
	
	if (wmstep->window_name != NULL)
	{
		free(wmstep-> window_name);
		wmstep-> window_name = NULL;
	}

	if (wmstep->icon_name != NULL)
	{
		free(wmstep-> icon_name);
		wmstep->icon_name = NULL;
	}
	if (wmstep->xwmhints != NULL)
	{
		free(wmstep-> xwmhints);
		wmstep->xwmhints = NULL;
	}
	if (wmstep->num_cmap_windows > 0) {
		if (wmstep->subwindow_data != NULL)
		{
			free(wmstep->subwindow_data);
			wmstep->subwindow_data =NULL;
		}
		if (wmstep->colormap_windows != NULL)
		{
			free(wmstep->colormap_windows);
			wmstep->colormap_windows = NULL;
		}
	}
	if (wmstep-> icon) {
		if (currentGUI == OL_MOTIF_GUI && motwmrcs->iconAutoPlace) {
			/* free up the space in the map */
			if (wmstep->icon_map_pos_row >= 0)
				ReleaseMapPosition(wmstep->icon_map_pos_row,
						wmstep->icon_map_pos_col);
	
		}
	   free(wmstep-> icon);
	   wmstep->icon = NULL;
	}
	if (wmstep->csinfo != (MotCSInfo *)NULL)
	{
		XtFree((char *)(wmstep->csinfo));
		wmstep->csinfo = NULL;
	}
#ifdef POSTMARK
	if (wmstep-> classhint.res_name)
	{
	   free(wmstep-> classhint.res_name);
	   wmstep->classhint.res_name = NULL;
	}
	if (wmstep-> classhint.res_class)
	{
	   free(wmstep-> classhint.res_class);
	   wmstep->classhint.res_class = NULL;
	}
	/*
	 * also potentially destroy the icon_pixmap and icon_mask
	 */
#endif
	/*
	 * etc
	 */
	
	if (wmstep-> gc != (GC) 0)
	{
		XtReleaseGC((Widget)wm,wmstep->gc);
		wmstep->gc=NULL;
	}
	if (wmstep->mdm) {
		OlgDestroyAttrs(wmstep->mdm);
		wmstep->mdm = (OlgAttrs *) 0;
	}
	if (wmstep->label_mdm) {
		OlgDestroyAttrs(wmstep->label_mdm);
		wmstep->label_mdm = (OlgAttrs *) 0;
	}
	if (wmstep->private_db) {
		OlDestroyInputEventDB(wmstep->private_db);
		if (wmstep->private_keys) {
#ifdef DEBUG
fprintf(stderr,"Free: (Destroy) private_keys = %x\n", wmstep->private_keys);
#endif
			XtFree((char *)wmstep->private_keys);
			wmstep->private_keys=NULL;
		}
		for (i=0; i < wmstep->private_buttons_used; i++) {
			if (wmstep->private_buttons[i].helpData) {
			   if (wmstep->private_buttons[i].helpData->menuargs &&
		 wmstep->private_buttons[i].selectProc != OlwmSend_Msg &&
		 wmstep->private_buttons[i].selectProc != OlwmCircle_Down &&
		 wmstep->private_buttons[i].selectProc != OlwmCircle_Up &&
		 wmstep->private_buttons[i].selectProc != OlwmNext_Key &&
		 wmstep->private_buttons[i].selectProc != OlwmPrev_Key) {
#ifdef DEBUG
fprintf(stderr,"Free: (Destroy) helpData.args = %x\n", wmstep->private_buttons[i].helpData->menuargs);
fprintf(stderr, "AND the selectProc=%x\n", wmstep->private_buttons[i].selectProc);
#endif

				/* Only free this for certain arguments */
					XtFree((char *)
				wmstep->private_buttons[i].helpData->menuargs);
				wmstep->private_buttons[i].helpData->menuargs=NULL;
			    }
#ifdef DEBUG
fprintf(stderr,"Free: (Destroy) helpData = %x\n", wmstep->private_buttons[i].helpData);
#endif
			   XtFree((char *)wmstep->private_buttons[i].helpData);
			   wmstep->private_buttons[i].helpData=NULL;
			} /* if */
		} /* for */
		/* Now free the private_buttons data structure */
#ifdef DEBUG
fprintf(stderr,"Free (Destroy): private_buttons=%x\n",wmstep->private_buttons);
#endif
		XtFree((char *)wmstep->private_buttons);
		wmstep->private_buttons=NULL;

		/* One more thing: free the menubutton data structures -
		 * I wish I didn't do it this way!
		 */
		if (wmstep->menu_ext) {
			WMMenuButtonData *mbd = wmstep->menu_ext,
					*next = mbd->next;
			for( ; mbd; mbd = next, next = next->next) {
				if (mbd->label) {
#ifdef DEBUG
fprintf(stderr,"Free (Destroy):label=%x its str=%s\n", mbd->label,
				mbd->label);
#endif
					XtFree((char *)mbd->label);
					mbd->label=NULL;
				}
				if (mbd->accelerator) {
#ifdef DEBUG
fprintf(stderr,"Free (Destroy): accel=%x\n",mbd->accelerator);
#endif
					XtFree((char *)mbd->accelerator);
					mbd->accelerator=NULL;
				}

				if ((mbd->args != NULL) &&
	   				((mbd->menucb == OlwmExec)  ||
					(mbd->menucb == OlwmMenu)  || 
					(mbd->menucb == OlwmLower) ||
					(mbd->menucb == OlwmRaise) || 
					(mbd->menucb == OlwmRaise_Lower))) {
#ifdef DEBUG
fprintf(stderr,"Free (Destroy):args=%x\n", mbd->args);
#endif
						XtFree((char *)mbd->args);
						mbd->args=NULL;
				}
				XtFree((char *)mbd);
				mbd=NULL;
			} /* for */	
		} /* if wmstep->menu_ext */
	} /* wmstep->private_db */
	
	RemoveWidgetFromWidgetBuffer(wm, window_list);
	RemoveWidgetFromWidgetBuffer(wm, group_list);
	if ( (window_list->used == 0) || ( (window_list->used == 1) &&
			(window_list->p[0] == (Widget)help_parent) &&
			(!wm_help_win_mapped) ) )
	   {
	   	CurrentX = 0;
	   	CurrentY = 0;
	   	/* Reset icons (CurrentIconX, CurrentIconY) to their correct
	    	 * starting point - NOT to (0,0) - use ResetIconPosition(screen)
	    	 */
	    	ResetIconPosition(XtScreen(w));
	   }
	
	if (wmstep->destroy_child)
	   {
	   _XtUnregisterWindow(child-> core.window, child);
	   child-> core.window = 0;
	   XtDestroyWidget(child);
	   }
	
	if (wmstep->icon_widget) {
		if (wmstep->decorations & WMIconWindowReparented
				&& wmstep->xwmhints->icon_window) {
			Window window = wmstep->xwmhints->icon_window;

			XUnmapWindow(XtDisplay((Widget)wm), window);
			XReparentWindow(XtDisplay((Widget)wm), window, 
				RootWindowOfScreen(XtScreen(
				(Widget)wm)), wm->core.x, wm->core.y);
			XChangeSaveSet(XtDisplay((Widget)wm), window,
							SetModeDelete);
		}
		DestroyStepIcon(wm);
	}

	AddDeleteWMStep((Widget)wm, False);

	if (start_or_terminate == TERMINATE_FLAG) {
		if (ReadyToDie())
		/* Send olwsm the EXIT message (reply to BANG) */
         	   EnqueueWSMRequest(XtDisplay(w),
					RootWindowOfScreen(XtScreen(w)),
            				WSM_EXIT, &wsmr);
	}
	
} /* end of Destroy */

/*
 *************************************************************************
 * Initialize
 * - Initialize proc. of wmstep widget rec.
 * The Reparent() function creates a wmstepWidget with these resources:
 * XtNwindow, XtNwindowAttributes, XtNforeground, XtNbackground.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Initialize OLARGLIST((request, actual, args, num_args))
OLARG(Widget, request)
OLARG(Widget, actual)
OLARG(ArgList, args)
OLGRA(Cardinal *, num_args)
{
WMStepWidget    wm      = (WMStepWidget) actual;
WMStepPart *    wmstep  = &wm-> wmstep;
WMGeometry *    p       = &wmstep-> prev,
	   *	icon_geometry = NULL;
Display *       display = XtDisplay(wm);
Window          window  = wmstep-> window;
Screen *        screen  = XtScreen(wm);
Window          root;
unsigned int    depth;
Arg             arg[7];
int		k = 0;
int		checkstate = 0;
int		minwidth, minheight;
XFontStruct *   _OlGetDefaultFont();
OlgTextLbl	labeldata;
Dimension	labelwidth, labelheight;
OlgAttrs	*label_mdm = (OlgAttrs *)NULL;
MotifCompResourceInfo	*mcri = (MotifCompResourceInfo *)
						&(motCompRes[TITLE_COMP]);
MotCompAppearanceInfo	*mcai = (MotCompAppearanceInfo *)
							&(mcri->compai);
Boolean		SkipStateCheck = False; /* Motif mode - check if any room
					 * for more icons.
					 */


/* Get Open Look default font, but initialize most fields to NULL */
/* Will not use this font for drawing anymore */
if (wmstep->font == NULL)
   wmstep->font = _OlGetDefaultFont(wm, NULL);
wmstep-> menu_default = 0;
wmstep-> menu_pane_default = 0;
wmstep->mbd_menu_default = (WMMenuButtonData *)NULL;
wmstep-> default_cb = MenuChooseDefault;
wmstep-> cascade_default = 0;
wmstep-> window_name  = NULL;
wmstep-> icon_name    = NULL;
wmstep-> icon         = NULL;
wmstep-> gc           = (GC)0;
wmstep-> hints        = 0;
wmstep-> decorations  = 0;
wmstep-> protocols    = 0;
wmstep-> is_current   = False;
wmstep-> is_grabbed   = False;
wmstep-> subwindow_data = NULL;
wmstep-> colormap_windows = NULL;
wmstep-> num_cmap_windows = 0;
wmstep-> xwmhints = NULL;
wmstep-> olwincolors	= 0;
wmstep->mdm = wmstep->label_mdm = NULL;
wmstep->window_bw = 0;
wmstep->olwa.flags = 0;
wmstep->wmdh.flags = 0;
wmstep->transient_parent = (Window) NULL;
wmstep->icon_widget = (Widget)NULL;
wmstep->menu_ext = (WMMenuButtonData *)NULL;
wmstep->private_buttons_avail = wmstep->private_buttons_used = (short)0;
wmstep->private_db = (OlVirtualEventTable)NULL;
wmstep->private_buttons = (WMMenuDef *)NULL;
wmstep->num_mwm_msgs = 0;
wmstep->icon_pixmap = (Pixmap)NULL;
wmstep->icon_map_pos_row = wmstep->icon_map_pos_col = -1;
wmstep->csinfo = (MotCSInfo *)NULL;
wmstep->iconimagetopGC = wmstep->iconimagebotGC =
		wmstep->iconimagefgGC = wmstep->iconimagebgGC = (GC)NULL;

if (currentGUI == OL_OPENLOOK_GUI) {
	minwidth = MINIMUM_WIDTH;
	minheight = MINIMUM_HEIGHT;
}
else {
	minwidth = Mresminwidth;
	minheight = Mresminheight;
}


/* Create wmstep->gc (actually do XtGetGC()) - if it is non-null, then do
 * XtReleaseGC() prior to doing the Get on wmstep->gc.
 * Also get necessary GCs with Olg convenience routines,
 * and fill in global mmWidth, mmHeight, ppWidth, ppHeight (dims of pushpin,
 * window menu button)
 */
CreateGC(wm, wmstep, FALSE);

if (window != (Window) NULL)
   {
   if (wmstep-> saveSet)
      XChangeSaveSet(display, window, SetModeInsert);

   if ((wmstep-> child = XtWindowToWidget(display, window)) == NULL)
      {
		/* samChang: we don't want "wmstep->child" to inherit */
		/* the translation */
      static XtTranslations	empty_translations = 0;

      if (empty_translations == 0)
	empty_translations = XtParseTranslationTable("");
      XtSetArg(arg[0], XtNwindow, window);
      XtSetArg(arg[1], XtNmappedWhenManaged, False);
      XtSetArg(arg[2], XtNtranslations, empty_translations);
      /* wm is the name of the current widget */
      wmstep-> child = 
         XtCreateWidget("child", stubWidgetClass, XtParent(wm), arg, 3);
		/* delete self from the traversal list */
      _OlDeleteDescendant(wmstep-> child);
      wmstep-> destroy_child = TRUE;
      XtRealizeWidget(wmstep->child);
      }
   else
      {
      /* if XtWindowToWidget() returned a widget, then the window was already
       * reparented and is a handle widget.
       */
      FPRINTF((stderr, "%x widgetized as %x\n", window, wmstep-> child));
      wmstep-> destroy_child = FALSE;
      }

   /* Event handlers for the wm widget */

   AddRemoveEventHandlers(wm, True);

   /* Note - the following two routines have not been converted to use
    * XTextProperties, required for multiple encodings.
    */
   /* Get WM_NAME property (use for decoration title) */
   GetWindowName(wmstep, display, window);
   /* Get WM_ICON_NAME property - name of the icon */
   GetIconName(wmstep, display, window);

   /* The wmstep->windowAttributes field tells me if window attributes have
    * been retrieved for the window.  If the field != NULL, then the window
    * being attended to was ALREADY on the screen before the window mgr.
    * was cranked up.  Otherwise, it is being mapped after the win. mgr. was
    * started.  It is used as a flag in GetNormalHints().
    */
   if (wmstep-> windowAttributes != NULL)
      wmstep-> window_attributes = *wmstep-> windowAttributes;
   else
      XGetWindowAttributes(display, window, &wmstep-> window_attributes);

	/* For later use, if the window_attributes has a colormap
	 * of None, then set it to the DefaultColormap.
	 * Is that right??
	 */
    if (wmstep->window_attributes.colormap == None)
	wmstep->window_attributes.colormap = DefaultColormapOfScreen(screen);

   /* SAVE the original x, y, width, ht, and border width of the window
    * before the window manager decorates it.
    */
   wmstep-> prev.x            = wmstep-> window_attributes.x;
   wmstep-> prev.y            = wmstep-> window_attributes.y;
   wmstep-> prev.width        = wmstep-> window_attributes.width;
   wmstep-> prev.height       = wmstep-> window_attributes.height;
   wmstep-> prev.border_width = wmstep-> window_attributes.border_width;
   depth = wmstep-> window_attributes.depth;
/*
   XSetWindowBorderWidth(display, window, (unsigned long)0);
 */

   /* Lets shake things up - do the GetDecorations first, so that in
    * GetNormalHints(), when we set up the minimum sizes in motif mode,
    * we know what to set them to based on if it has a header, resize
    * corners, a border, a minimize button, and a maximize button.
    */
   GetAllDecorations(wmstep, display, window);

   /* Some of these decorations may be modified after we get
    * client specific resources - motif mode only
    */
   /* We need the XClassHint fields for the client, especially
    * in motif mode where we read client specific resources.
    * This code for the class hints used to be in GetWMHints()
    * I modified it so that it definitely places a value in
    * classhint.res_class.
    */
   if (!XGetClassHint(display, window, &wmstep-> classhint))
   {
   wmstep-> classhint.res_name  = strdup(wmstep->icon_name);
   wmstep-> classhint.res_class = strdup(wmstep->icon_name);
   }

   if (currentGUI == OL_MOTIF_GUI) {
	if ( (wmstep->csinfo = (MotCSInfo *)
			XtMalloc(sizeof(MotCSInfo)) ) != NULL)
		GetCSResources(wm);
   } /* if currentGUI == OL_MOTIF_GUI */

   /* If in motif mode, then if they don't want a border, leave the
    * original one on.  Keep in mind that without a border, you
    * can't have resize corners in motif.
    */
   if (currentGUI==OL_OPENLOOK_GUI || currentGUI == OL_MOTIF_GUI &&
	(wmstep->decorations & WMBorder))
		XSetWindowBorderWidth(display, window, (unsigned long)0);

   if  (currentGUI == OL_MOTIF_GUI &&
			(wmstep->menu_ext = GetMWMMenu(wm)) != NULL)
   	NewMenuItems(wm);

   /* Get WM_HINTS property - place in wmstep->xwmhints (defaults provided
    * if property is not present, and set wmstep->hints &= ~WMHints)
    * If present, set wmstep->hints |= WMHints.
    */
   GetWMHints(wmstep, display, window);

   /* Based on the way that the window_group field is used in olwm, in
    * my opinion, and this is just one opinion, if no window mgr hints
    * are provided, then the window_group field should be set to
    * the transient parent IF there is a transient parent.   Prior to
    * this, the window_group field was set to the window ID of it's own
    * window if no hints were found.
    * The transient parent is set on the call to GetAllDecorations to
    * the window ID of the window that this window is transient for,
    * or NULL.  It is used in several places, sometimes in conjunction
    * with the window_group (unfortunately).
    */
   if ( (!(wmstep->hints & WMHints)) && wmstep->transient_parent)
	wmstep->xwmhints->window_group = wmstep->transient_parent;

   /* Get WM_NORMAL_HINTS. place in wmstep->xnormalsize;  If not present, set
    * wmstep-> hints &= (~WMNormalSizeHints); 
    * --- NOTE - may be a problem - looks at normal hints width and ht. fields
    */
   GetNormalHints(wmstep, display, window, screen);
	/* Once we have the normal hints:
	 * if USPosition is set, use prev.x, prev.y;
	 *  else determine the windows position after sizing the window.
	 * if USSize|PSize is set, use prev.width, prev.height.
	 *  else it is our right to resize the window such that the
	 *  title will fit.  Or, should we only pay attention to USSize,
	 *  and have the right to reset the window size if PSize is set.
	 */

   /* Read properties pertaining to decorations - WM_TRANSIENT_FOR,
    * OL_WIN_ATTR, and if that isn't found, WM_DECORATION_HINTS.  Also
    * read OL_DECOR_ADD and _OL_DECOR_DELETE.  Fill in:
    * - wmstep->decorations,  with appropriate decoration flags;
    * - wmstep->olwa, with pointer to  OLWinAttr struct (if _OL_WIN_ATTR
    *    is present..) 
    * - wmstep->wmdh with ptr. to WMDecorationHints struct, if
    *   WM_DECORATION_HINTS is present (ignored if OL_WIN_ATTR is present).
    */

#ifdef OLDWM
   /* Magic numbers for menu defaults:
    * The initial default for limited menu is 0 because
    * the Dismiss button is the first button on the combined menu,
    * but the CLOSE button, the first full menu item on the combined
    * menu, is the second button, so set it = 1.
    */
   if( wmstep->decorations & WMHasFullMenu )
	wmstep->menu_default = 1;
    else
	if( wmstep->decorations & WMHasLimitedMenu)
		wmstep->menu_default = 0;
#endif

  /* Read the WM_PROTOCOLS property from the window being mapped; pass the
   * wm widget and the window (child) widget (the handle widget now).
   * This function fills in the wmstep.protocols field with the appropriate
   * flags of the protocols being participated in.
   */
   ReadProtocols(wm, wmstep-> child, window);

   GetColormapWindows(wm, display, window);

   /* Read in _OL_WIN_COLORS property */
   SetColors(wm, display, window);

   if (wmstep-> prev.width < minwidth)
      wmstep-> prev.width = minwidth;
   if (wmstep-> prev.height < minheight)
      wmstep-> prev.height = minheight;

   /* size field - tells us current size "state" (WmSize, enum in  wmP.h) */
   wmstep-> size    = WMNORMALSIZE;

   /* return ptr to array of "decoration values" useful with this clients
    * scale.
    */
   wmstep-> metrics = GetMetrics(wm);
        if (wmrcs->font != NULL) {
		int fh;
		fh =  OlFontHeight(wmrcs->font, ol_font_list)+3;
		wmstep->metrics->motifButtonHeight = fh;
		wmstep->metrics->motifButtonWidth = fh;
	}


	/* The following block of code is for a window that was mapped AFTER
	 * the window manager started, AND
	 * the user doesn't care about the initial size of the window (the
	 * USSize flag is not turned on in the WM_NORMAL_HINTS).
	 * The width of the client is adjusted such that the title bar contains
	 * the full title.
	 */
   if (wmstep-> windowAttributes == NULL && !(wmstep-> xnormalsize.flags & USSize))
      {
      int textarea;
      int textwid;
      int diff;
	int markwidth  = (HasPushpin(wm) ? ppWidth :
				(wmstep->decorations & WMHasFullMenu ?
					 mmWidth : 0) );

      /* wm->core.width is the total frame's width (including decorations.
       * Many of these macros are defined at the top of this file.
       * For example, ParentWidth() is passed this widget and the width
       * of the window that was mapped, prev.width.  The macros use the
       * values in wm->wmstep.metrics->* to get precise values based
       * on the scale - GetMetrics() called previously.
       */
   if (currentGUI == OL_OPENLOOK_GUI) {
	wm-> core.width = ParentWidth(wm, wmstep-> prev.width);
	textarea = CoreW(wm) - (Offset(wm) + markwidth +
			2*wmstep->metrics->mark_title_gap + BorderX(wm));
   }
   else { /* Motif */
	int hwidth, vwidth;

	/* This number returned may not be perfect... */
	wm-> core.width = MParentWidth(wm, wmstep-> prev.width);

	if (Resizable(wm)) {
		vwidth = wmstep->metrics->motifVResizeBorderWidth;
		hwidth = wmstep->metrics->motifHResizeBorderWidth;
	}
	else {
		vwidth = wmstep->metrics->motifVNResizeBorderWidth;
		hwidth = wmstep->metrics->motifHNResizeBorderWidth;
	}
	textarea = wm->core.width - 2 * hwidth - 
		3 * wmstep->metrics->motifButtonWidth - 4;
	if (!(wmstep->decorations & WMMinimize))
		textarea += wmstep->metrics->motifButtonWidth;
	if (!(wmstep->decorations & WMMaximize))
		textarea += wmstep->metrics->motifButtonWidth;
	if (!(HasMenu(wm)))
		textarea += wmstep->metrics->motifButtonWidth;
   } /* Motif */


	/* Get width of title */

			labeldata.font = wmrcs->font;
			labeldata.accelerator = NULL;
			labeldata.mnemonic = NULL;
			labeldata.flags = (unsigned char) NULL;
			if (currentGUI == OL_MOTIF_GUI &&
			  mcai->fontList)
				labeldata.font_list = mcai->fontList;
			else
				labeldata.font_list = ol_font_list;
			labeldata.label = wmstep->window_name;
			labeldata.justification = TL_CENTER_JUSTIFY;

			label_mdm = OlgCreateAttrs(screen,
				currentGUI == OL_MOTIF_GUI ?
				mcai->foreground : wmrcs->foregroundColor,
				currentGUI == OL_MOTIF_GUI ?
					(OlgBG *)&(mcai->foreground) :
					(OlgBG *)&(wmrcs->foregroundColor) ,
				(Boolean)FALSE, (Dimension)12);

			labeldata.normalGC = OlgGetFgGC(label_mdm);
			if (labeldata.font) /* SHOULDN'T BE NON_NULL */
				XSetFont(display, labeldata.normalGC,
						labeldata.font->fid);
			OlgSizeTextLabel(screen, wmstep->mdm,
				 &labeldata, &labelwidth, &labelheight);


	textwid  =  labelwidth;

	/* When bringing up the window initially from Withdrawn stae,
	 * ignore the fact the the title have a ':' in it.
	 */
	diff = textwid - textarea;
	/* If title is wider than client text area, reset prev.width.
	 * Should we check the maximum size here???
	 */

	if (diff > 0)
		wmstep-> prev.width += diff;

      FPRINTF((stderr, "textarea = %d textwid = %d diff = %d\n", 
         textarea, textwid, diff));
      } /* end if (USSize is false ) */

   FPRINTF((stderr, "USSIZE is %s\n", 
      (wmstep-> xnormalsize.flags & USSize) ? "true" : "false"));

   /* Resize client window to prev.width by prev.height if they changed from
    * original window attribute dimensions.  This can only happen if
    * wmstep->windowAttributes == NULL (window manager was running before
    * window was mapped).
    */
   if (wmstep-> prev.width != wmstep-> window_attributes.width ||
       wmstep-> prev.height != wmstep-> window_attributes.height)

      XtResizeWidget(wmstep->child, wmstep->prev.width,
					wmstep->prev.height, 0);

   if (wmstep-> windowAttributes != NULL)
      {
	/* This window was on screen BEFORE the window mgr. started;
	 * Adjust x,y position of client  window's frame.  Note that the
	 * window dimensions may have changed if the window dimensions
	 * were less than the minimums.
	 */
	if (currentGUI == OL_OPENLOOK_GUI) {
           wmstep-> prev.x -= OriginX(wm);
           wmstep-> prev.y -= OriginY(wm);

           if(wmstep-> prev.x <0)  wmstep-> prev.x = 0;
           if(wmstep-> prev.y <0)  wmstep-> prev.y = 0;

           if((wmstep-> prev.x+wmstep-> prev.width)>WidthOfScreen(screen))
               wmstep-> prev.x -= ((wmstep-> prev.x+wmstep-> prev.width) -
                                  WidthOfScreen(screen)+ 2*OriginX(wm));
           if((wmstep-> prev.y+wmstep-> prev.height)>HeightOfScreen(screen))
                wmstep-> prev.y -= ((wmstep-> prev.y+wmstep-> prev.height)-
                        HeightOfScreen(screen) + OriginX(wm) + OriginY(wm));

         } else {
	   wmstep-> prev.x -= MOriginX(wm);
	   wmstep-> prev.y -= MOriginY(wm);

           if(wmstep-> prev.x <0)  wmstep-> prev.x = 0;
           if(wmstep-> prev.y <0)  wmstep-> prev.y = 0;
           if((wmstep-> prev.x+wmstep-> prev.width)>WidthOfScreen(screen))
               wmstep-> prev.x -= ((wmstep-> prev.x+wmstep-> prev.width) -
                                  WidthOfScreen(screen)+ 2*MOriginX(wm));

           if((wmstep-> prev.y+wmstep-> prev.height)>HeightOfScreen(screen))
               wmstep-> prev.y -= ((wmstep-> prev.y+wmstep-> prev.height)-
                        HeightOfScreen(screen) + MOriginX(wm) + MOriginY(wm));
	 } /* Motif */
     } /* windowAttributes != NULL */
    else /* window mapped after olwm running already */
	if ( !(wmstep->xnormalsize.flags & USPosition) &&
	     GetWMState(display, window) != WithdrawnState ) {

		if (CurrentX + wmstep-> prev.width > WidthOfScreen(screen) ||
		   (CurrentY + wmstep->prev.height > HeightOfScreen(screen)))
 			CurrentX = CurrentY = 0;

		wmstep->prev.x = CurrentX;
		wmstep->prev.y = CurrentY;
		CurrentX += Xincrement;
		CurrentY += Yincrement;
	} /* end else if (!USPosition) */

   /* The following block of code adjusts the x,y position of the
    * decorated window's frame based on the window gravity.
    * Do this for windows that are mapped AFTER the window manager is
    * started, and only if the USPosition flag is set on the window's
    * WM_NORMAL_HINTS property.
    */
   if ( (wmstep-> windowAttributes == NULL) &&
			(wmstep->xnormalsize.flags & PWinGravity) &&
			(wmstep-> xnormalsize.flags & USPosition) ) {
		switch(wmstep->xnormalsize.win_gravity) {
			/* If it's west, the x coord is O.K.;
			 * if it's east it must change;
			 * if it's north, the y coord is O.K.;
			 * if it's south, it must change;
			 * if it's center?  I have to think about it.
			 * Center means x/2 or y/2.  For now, just
			 * assume center means west.
			 */
			case NorthWestGravity:
			case NorthGravity:
			case WestGravity:
			case CenterGravity:
				break;

			case NorthEastGravity:
			case EastGravity:
				/* anchor on right */
				p->x  = (p->x + p->width +
   				  2 * wmstep->window_attributes.border_width) -
					(currentGUI == OL_OPENLOOK_GUI ?
						 ParentWidth(wm,p->width):
						MParentWidth(wm, p->width));
				break;
			case SouthWestGravity:
			case SouthGravity:
				p->y  = (p->y + p->height +
   				  2 * wmstep->window_attributes.border_width) -
					(currentGUI == OL_OPENLOOK_GUI ?
						 ParentHeight(wm,p->height):
						MParentHeight(wm, p->width));
				break;
			case SouthEastGravity:
				p->x  = (p->x + p->width +
   				  2 * wmstep->window_attributes.border_width) -
					(currentGUI == OL_OPENLOOK_GUI ?
						 ParentWidth(wm,p->width) :
						MParentWidth(wm, p->width));
				p->y  = (p->y + p->height +
   				  2 * wmstep->window_attributes.border_width) -
					(currentGUI == OL_OPENLOOK_GUI ?
						 ParentHeight(wm,p->height):
						MParentHeight(wm, p->width));
		}
	}

   if (wmstep-> destroy_child)
      {
      /* client window was just turned into a handle widget (child),
       * adjust client widget's core width and height.
       */
      wmstep-> child-> core.width  = wmstep-> prev.width;
      wmstep-> child-> core.height = wmstep-> prev.height;
      }

   /* Note, p points to wmstep->prev (prev.x,y, etc.).  Here, adjust
    * overall frame geometry values.
    */ 
   wm-> core.x            = p-> x;
   wm-> core.y            = p-> y;
if (currentGUI == OL_OPENLOOK_GUI) {
   wm-> core.width        = p-> width = ParentWidth(wm, p-> width); 
   wm-> core.height       = p-> height = ParentHeight(wm, p-> height);
}
else {
   wm-> core.width        = p-> width = MParentWidth(wm, p-> width); 
   wm-> core.height       = p-> height = MParentHeight(wm, p-> height);
}
   wm-> core.depth        = depth;
   wm-> core.border_width = 0;

   /* Motif mode - iconAutoPlace resource.  This icon position is not
    * used in this Initialize function unless the client starts in
    * iconic state, but this resource needs to ascertain the icon position
    * now.
    */
   if (currentGUI == OL_MOTIF_GUI && !(motwmrcs->iconAutoPlace) &&
			!(wmstep->transient_parent) &&
			find_highest_group(wm, NULL) == window) {
/*  This is a check that probably should be done - what if you have a window
 * that is asking to start in iconic state, but isn't really allowed to?...
			wmstep->menu_functions & WMFuncMinimize)
 */
	icon_geometry = IconPosition(wm, wmstep);
   }
   /* If this client window plans on starting in iconic state and plans on
    * having an icon widget, we'll do a check first.  This is only a factor
    * in Motif mode, because we can't run out of space in openlook mode.
    */
	{
	unsigned long win_state;
	win_state = GetWindowState(display, wmstep->window);
   if ( currentGUI == OL_MOTIF_GUI && motwmrcs->iconAutoPlace &&
			(wmstep->xwmhints->flags & StateHint &&
			wmstep->xwmhints->initial_state == IconicState ||
			win_state == IconicState) ) {
		if (!(wmstep->transient_parent) &&
			   find_highest_group(wm, NULL) == window) {
			icon_geometry = IconPosition(wm, wmstep);
			if (icon_geometry != NULL) {
				SkipStateCheck = False; /* Default */
			}
			else {
				/* If SkipStateCheck then no more room for 
				 * icons
				 */
				SkipStateCheck = True;
			}
		}
   } /* multiple if */
  }

   if (wmstep->xwmhints->flags & StateHint && !SkipStateCheck)
      {
      switch(wmstep-> xwmhints-> initial_state)
         {
         case IconicState:
	    /* get position of next icon - IconPosition() places
	     * a WMGeometry struct in wmstep->icon (it will contain the
	     * dimension and position of the icon) AND RETURN A POINTER to
	     * it (so p is now used to point to it).  Set (in wmstep
	     * part of widget) size to ICONIC, get the metrics for it,
	     * and adjust wm widgets core fields.
	     */
		wmstep->size    = WMICONICNORMAL;
	    if (!wmstep->transient_parent &&
                                find_highest_group(wm, NULL) ==
                                        window) {
		/* Based on the above Skip check, there must be room for
		 * at least another icon here.
		 */
		if (!icon_geometry)
			icon_geometry = IconPosition(wm, wmstep);
		wmstep-> metrics = GetMetrics(wm);
		CreateStepIcon(wm, icon_geometry);
	   }
		SetWindowState(wm, IconicState, wm->wmstep.window);
	    /* In Realize(), I unmap the client window if we think it's in
	     * iconic state; it doesn't get unmapped if olwm is running
	     * prior to the client being up already (the map is "trapped"
	     * by the window manager); but if the client is
	     * already up and mapped, then the XUnmapWindow() will generate a real
	     * UnmapNotify event; this must be ignored by us or the
	     * client will be inadvertantly dumped.
	     */
	    	if (wmstep->windowAttributes)
			wmstep->protocols |= IgnoreUnmap;

            break;
         case ZoomState:
         case DontCareState:
         case NormalState:
         case InactiveState:
         default:
		if (GetWindowState(display, wmstep->window) !=
                                                IconicState) {
                        SetWindowState(wm, NormalState, wm->wmstep.window);
                        checkstate++;
			wmstep->metrics = GetMetrics(wm);
			wmstep->size = WMNORMALSIZE;
			break;
                }
		else {
			/* make it iconic */
			if (!(wmstep->transient_parent) &&
				find_highest_group(wm, NULL) ==
						window) {
				if (!icon_geometry)
				   icon_geometry = IconPosition(wm, wmstep);
				wmstep->size = WMICONICNORMAL;
				wmstep->metrics = GetMetrics(wm);
				SetWindowState(wm, IconicState,
						wm->wmstep.window);
				if (wmstep->windowAttributes)
					wmstep->protocols |= IgnoreUnmap;
				CreateStepIcon(wm, icon_geometry);
			}
			else { /* Play it safe - set to normal, checkstate.
				* We'll have to check this window out again
				* in wm.c, Reparent() at a later time.
				*/
				checkstate++;
				SetWindowState(wm, NormalState,
							wm->wmstep.window);
                                wmstep->size = WMNORMALSIZE;
			}
                } /* else WM_STATE == iconic */
            break;
         } /* switch */
      } /* if statehint && !SkipCheck */
   else {
		/* default: set state to normal */
		checkstate++;
		SetWindowState(wm, NormalState, wm->wmstep.window);
		wmstep->size = WMNORMALSIZE;
		wmstep->metrics = GetMetrics(wm);
		/* Check to see if there is no StateHint but the WM_STATE
		 * property says it is iconic - possible in a non-Xt app
		 * SkipStateCheck tells me it is ok to make another icon
		 * widget.  Any popup (group members) that should be iconified
		 * along with this one will get checked by the above code
		 * (checkstate).
		 */
		if (GetWindowState(display, wmstep->window) ==
				IconicState && !SkipStateCheck) {
                        if (!(wmstep->transient_parent) &&
                                find_highest_group(wm, NULL) ==
                                        window) {
				if (!icon_geometry)
                                	icon_geometry = IconPosition(wm, 
								wmstep);
                                wmstep->size = WMICONICNORMAL;
                                wmstep->metrics = GetMetrics(wm);
                                SetWindowState(wm, IconicState, wm->wmstep.window);
                                if (wmstep->windowAttributes)
                                        wmstep->protocols |= IgnoreUnmap;
                                CreateStepIcon(wm, icon_geometry);
			}
		} /* GetWindowState */
	} /* top else */

   wmstep-> decorations |= WMNotReparented;
   /* Set to 1, not 0, because of possible side effects in some cases */
   wmstep-> focus = 1;
   }
if (checkstate) {
	/* I just decorated a window in normal state. Does this window have
	 * a leader that is currently iconic??
	 */
	Window leader = wmstep->xwmhints->window_group;
	Cardinal k;
	if  ( ((k = find_leader(leader)) < num_wmstep_kids) ||
		( (wmstep->transient_parent != NULL) &&
		( (k = find_leader(wmstep->transient_parent)) < num_wmstep_kids)) ) {
		WMStepWidget temp = (WMStepWidget)wmstep_kids[k];
		if (IsIconic(temp)) {
			/* Make this one iconic too!! */
			if (wmstep->olwa.win_type != XA_OL_WT_NOTICE(display)) {
				wmstep->size    = WMICONICNORMAL;
				wmstep->metrics = GetMetrics(wm);
				SetWindowState(wm, IconicState, temp->wmstep.window);
				if (wmstep->windowAttributes)
					wmstep->protocols |= IgnoreUnmap;
			}

			/* Note we still have a problem here - the
			 * window will still map in Reparent(), so
			 * we must tell Reparent() not to map us.
			 * We have an unused field, window_bw, in
			 * the WMStepPart - set it to 1 << 10.
			 * The exception to this rule is that we
			 * always want NOTICE windows to map.
			 */
		}
	}
} /* end if (checkstate) */
/* Maintain a list of wm widgets in buffer window_list initialized at
 * top of function.
 */
AddWidgetToWidgetBuffer(wm, window_list);

AddDeleteWMStep(actual, True);

	if (label_mdm) /* local */
		OlgDestroyAttrs(label_mdm);
} /* end of Initialize */

/*
 *************************************************************************
 * Realize
 * - Realize procedure to create the wm widget window.
 *  valueMask - tells us which fields in the attributes struct to
 *		use in the XCreateWindow() call.
 ****************************procedure*header*****************************
 */
static void
Realize(widget, valueMask, attributes)
	register Widget        widget;
	Mask *                 valueMask;
	XSetWindowAttributes * attributes;
{
	WMStepWidget wm     = (WMStepWidget) widget;
	WMStepPart * wmstep = &wm-> wmstep;

	/* wmrcs->parentRelative resource - default is false??*/
	if (wmrcs->parentRelative) {
		Mask mask = *valueMask;

		attributes-> background_pixmap = ParentRelative;
		mask &= ~CWBackPixel;
		mask |= CWBackPixmap;
		*valueMask = mask;
	}

	XtCreateWindow((Widget)wm, (unsigned int)InputOutput,
			(Visual *)CopyFromParent, *valueMask, attributes);

	XGrabButton(
		XtDisplay(widget), AnyButton, AnyModifier, XtWindow(widget),
		False, ButtonPressMask, GrabModeSync, GrabModeSync, None, None
	);
	wmstep->is_grabbed = True;

	if (wmstep-> window != (Window) 0) {
						/* Window created */
	Position x;
	Position y;

	if (currentGUI == OL_MOTIF_GUI) {
		x = MOriginX(wm);
		y = MOriginY(wm);
		/* For motif, the cursor is the standard X cursor on the
		 * root window, but the cursor changes to the northwest
		 * arrow (XC_left_ptr) inside the decorations window.
		 */
   		XDefineCursor(XtDisplay(wm), XtWindow(wm), 
				motifWindowCursor);
	}
	else {
		x = OriginX(wm);
		y = OriginY(wm);
	}

	if (wmstep-> size == WMICONICNORMAL)
	   {
	   /* current state of window is iconic - For the sole purpose of
	    * getting the x,y position of the window when it is in the
	    * NORMAL state, get the metrics for the window when it WOULD
	    * be in normal state, get x,y, then reset the wm widgets
	    * step part's size and metrics back to to current iconic state.
	    */
	   wmstep-> size    = WMNORMALSIZE;
	   wmstep-> metrics = GetMetrics(wm);
		if (currentGUI == OL_OPENLOOK_GUI) {
			x = OriginX(wm);
			y = OriginY(wm);
		}
		else {
			x = MOriginX(wm);
			y = MOriginY(wm);
		}
	   wmstep-> size    = WMICONICNORMAL;
	   if (wmstep->icon_widget)
		/* Reset metrics if step has icon widget */
		wmstep->metrics = GetMetrics(wm);
	   }
	/* Reparent the new window to wm widgets core window */
	XReparentWindow(XtDisplay(wm), wm-> wmstep.window,
				wm-> core.window, x, y);

	if (wmstep-> size == WMFULLSIZE || wmstep-> size == WMNORMALSIZE)
	   XMapWindow(XtDisplay(wm), wmstep-> window);
	else {
	   XUnmapWindow(XtDisplay(wm), wmstep-> window);
	   if (wmstep->icon_widget)
	   	XtRealizeWidget(wmstep->icon_widget);

	/* Special case for windows starting in iconic state that use
	 * their own icon window (WM_HINTS.icon_window) - reparent the
	 * window to the wmstep core window, and map the icon window.
	 * This code is duplicated in OpenClose() in Menu.c.
	 */

	if ( (wmstep->xwmhints->icon_window != (Window)NULL) &&
 		(!(wmstep-> decorations & WMIconWindowReparented)) ) {
		/* icon_window - user supplied;
		 * !WMIconWindowReparented - olwm didn't reparent this icon
		 * window to the wmstep core window.
		 */
		XChangeSaveSet(XtDisplay(wm), wmstep-> xwmhints-> icon_window,
							SetModeInsert);
		if (currentGUI == OL_OPENLOOK_GUI) {
			XResizeWindow(XtDisplay(wm),
				wmstep-> xwmhints-> icon_window, 
				ol_icon_image_width, ol_icon_image_height);
			XReparentWindow(XtDisplay(wm),
				wmstep-> xwmhints-> icon_window, 
				XtWindow(wmstep->icon_widget), LineWidX(wm) +
				wmstep->metrics->selectwidth, LineWidY(wm) +
				wmstep->metrics->selectwidth);
		}
		else
			ReparentMotifIconWindow(XtDisplay(wm), wmstep);

		wmstep-> decorations |= WMIconWindowReparented;

		/* Now map icon window, set flag */
		XMapWindow(XtDisplay(wm), wmstep-> xwmhints-> icon_window);
		wmstep-> decorations |= WMIconWindowMapped;
	} /* end if (icon_window != NULL) && ~WMIconWindowReparented */
      } /* end else */

	if (wmstep-> decorations & WMPushpin)
	   SetPushpinState(wm, wmstep-> decorations & (WMPushpin | WMPinIn),
	   FALSE);
	} /* end if (wmstep->window != NULL */
} /* end of Realize */

/*
 *************************************************************************
 * Redisplay
 * - Wm widgets expose procedure.
 * Call DisplayWM() to redraw all decorations; unless rect.width==0 &&
 * rect.height == 0, ALL decorations are basically redrawn.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Redisplay(w, event, region)
	Widget w;
	XEvent *event; /* unused */
	Region region; /* unused */
{
	XRectangle	rect,
			*rectp = &rect;

  /* - clip and display, reset clip in DisplayWM() */
	if (region != (Region)NULL)
		XClipBox(region, rectp);
	else
		rectp =  (XRectangle *)NULL;
	DisplayWM((WMStepWidget)w, rectp);
} /* end of Redisplay */


/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */
