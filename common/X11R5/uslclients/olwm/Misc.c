/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olwm:Misc.c	1.46"
#endif

/*
 *************************************************************************
 *
 * Description:
 *	This file contains miscellaneous routines
 *
 ******************************file*header********************************
 */

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/Xatom.h>

#include <Xol/OpenLookP.h>
#include <Xol/VendorI.h>
#include <Xol/Stub.h>
#include <Xol/DynamicP.h>
#include <wm.h>
#include <WMStepP.h>
#include <Extern.h>

#include <signal.h>
#include <limits.h>

/*
 *************************************************************************
 *
 * Forward Procedure declarations
 *
 **************************forward*declarations***************************
 */

extern void	AddDeletePendingList OL_ARGS((Widget, Widget, Boolean));
extern int	CmapInWMList OL_ARGS((Colormap));
extern void	GetAllDecorations OL_ARGS((WMStepPart *, Display *, Window));
extern void	GetColormapWindows OL_ARGS((WMStepWidget, Display *, Window));
extern void	GetWMHints OL_ARGS((WMStepPart *, Display *, Window));
extern void	GetNormalHints OL_ARGS((WMStepPart *, Display *,
					Window, Screen *));
extern void	GetBusyState OL_ARGS((WMStepPart *, Display *, Window));
extern void	GetDecorations OL_ARGS((WMStepPart *, Display *, Window));
extern void	GetAddDecorations OL_ARGS((WMStepPart *, Display *, Window));
extern void	GetDelDecorations OL_ARGS((WMStepPart *, Display *, Window));
extern void	GetWindowName OL_ARGS((WMStepPart *, Display *, Window));
extern void	GetIconName OL_ARGS((WMStepPart *, Display *, Window));
extern int	GetWindowState OL_ARGS((Display *, Window));
extern int	IsPending OL_ARGS((Window));
extern void	MotifModeStartup OL_ARGS((Boolean));
extern WMStepWidget NextFocus OL_ARGS((WMStepWidget, Boolean));
extern void	RemoveOldGrabs OL_ARGS((WMStepWidget));
extern int	ReworkColormapInfo OL_ARGS((Display *, Window, int));
extern void	SendWindowMovedEvent OL_ARGS((WMStepWidget));
extern void	SetColors OL_ARGS((WMStepWidget, Display *, Window));
extern void	SetWindowState OL_ARGS((WMStepWidget, int, Window));
extern void	SetPushpinState OL_ARGS((WMStepWidget, unsigned long, int));
extern void	SetFocus OL_ARGS((Widget, int, int));
extern void	SetCurrent OL_ARGS((WMStepWidget));
extern void	SetSignals OL_ARGS((Display *, Window, Screen *));
extern void	TrapSignal OL_NO_ARGS();

extern void	WMClientDecorConverter OL_ARGS((XrmValue *, Cardinal,
					XrmValue *, XrmValue *));
extern void	WMClientFunctionsConverter OL_ARGS((XrmValue *, Cardinal,
					XrmValue *, XrmValue *));
extern void	WMIconDecorConverter OL_ARGS((XrmValue *, Cardinal,
					XrmValue *, XrmValue *));
extern void	WMIconWinDimConverter OL_ARGS((XrmValue *, Cardinal,
					XrmValue *, XrmValue *));
extern void	WMInstallColormaps OL_ARGS((WMStepWidget));

extern void	WMTransientDecorConverter OL_ARGS((XrmValue *, Cardinal,
					XrmValue *, XrmValue *));
extern void	WMTransientFunctionsConverter OL_ARGS((XrmValue *, Cardinal,
					XrmValue *, XrmValue *));

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

static Display	*TrapDpy;
static Window	TrapRoot;
static Screen	*TrapScr;

#define CONSTRAIN

#ifdef CONSTRAIN

/* These are needed for OlQueryPointer() */
static int prevrootx;
static int prevrooty;
static int prevx;
static int prevy;

#endif




/*
 *************************************************************************
 *
 * Procedures
 *
 ***************************private*procedures****************************
 */

/* The following routines are used by olwsm, olwm, and olfm (if necessary) to
 * tell the system that it is running.  Primary usage is by olwm and olwsm-
 * olwm needs to know if olwsm is running, if not it needs to perform
 * termination procedures.
 * The following defined and structure are REPEATED from OpenLook.c,
 * because they are local to that file, yet we need them for the
 * functions here.  These functions are private to Open Look clients.
 * If the defines or the struct changes in OpenLook.c, it must change here.
 */

typedef struct {
	unsigned long	state;
} xPropOLManagerState;

#define NumPropOLManagerStateElements 1

void
SetOLManagerState(dpy,scr,wms)
Display *dpy;
Screen *scr;
OLManagerState *wms;
{
	Window rootwin = RootWindowOfScreen(scr);
	xPropOLManagerState	prop;
	Atom	OL_MANAGER_STATE = 0;

	OL_MANAGER_STATE = XInternAtom(dpy, "OL_MANAGER_STATE", False);

	if (wms == (OLManagerState *) 0)
		return;

	prop.state = wms->state;

	XChangeProperty(dpy, rootwin, OL_MANAGER_STATE, OL_MANAGER_STATE, 32,
				PropModeReplace, (unsigned char *) &prop,
				NumPropOLManagerStateElements);

	XFlush(dpy);
}

/* Constants defined in OlClients.h -
#define OLWSM_STATE	(1L<<0)	
#define OLWM_STATE	(1L<<1)	
#define OLFM_STATE	(1L<<2)	
 */

void
SetOlwmRunning(dpy,scr)
Display *dpy;
Screen *scr;
{
	OLManagerState wms;
	wms.state = (unsigned long) 0;

	/* First get the current property state */
	wms.state = GetOLManagerState(dpy,scr);
	wms.state |= OLWM_STATE;
	SetOLManagerState(dpy, scr, &wms);
}

void
SetOlwmNotRunning(dpy,scr)
Display *dpy;
Screen *scr;
{
	OLManagerState wms;
	wms.state = (unsigned long) 0;

	/* First get the current property state */
	wms.state = GetOLManagerState(dpy, scr);
	wms.state &= ~OLWM_STATE;
	SetOLManagerState(dpy,scr, &wms);
}


/*
 *************************************************************************
 * GetColormapWindows.
 *  - Retrieve the WM_COLORMAP_WINDOWS property from
 * the client window.  If it exists, and has windows other than
 * the toplevel window, then malloc space for it, point to it from
 * the step widget. If it doesn't exist, process all colormap related work
 * from the colormap field in the toplevel windows window attributes;
 * furthermore, the wmstep->cmaps field will be NULL;
 * but if it exists, then the field will be non-NULL, and any work related
 * to colormaps should be processed from the non-null list.
 *
 * Other tid-bits : each subwindow on the list should be watched for
 * ColormapNotify events for possible change in its colormap attribute.
 * Select for ColormapNotify events on the subwindows in the list that
 * we don't know about (e.g, aren't decorated).  If the list changes,
 * the "UnSelect" for the events on the windows no longer on the list.
 * Also on unmapnotify (or destroyNotify too?), when "undecorating"
 * a window (NOT when iconizing) do the same.  Simply, do
 * XSelectInput(display, window, NoEventMask);
 * This XContext scheme will not work if a toplevel window is on the
 * list of WM_COLORMAP_WINDOWS; to get it to work, you must have
 * a different context for it.  For example, the current context
 * for all windows is the (window ID / 2) + (the window ID % 2);
 * One idea is, have this be the context for subwindows that are on
 * a list, and another context, such as divide by 4 and add %4,
 * be the context for windows if the HAVE a WM_COLORMAP_LIST.:
 ****************************procedure*header*****************************
 */
extern void
GetColormapWindows OLARGLIST((wm, display, window))
OLARG(WMStepWidget, wm)
OLARG(Display *, display)
OLGRA(Window,  window)
{
int count_return;
Window *cmap_windows_ret;
int window_on_list = 0;
int i, j, max_installed;
XWindowAttributes xwa;
WMStepPart *wmstep = &wm->wmstep;
Window *save_windows;
SubWindowInfo *sub;
int index;

	if (XGetWMColormapWindows(display,window, &cmap_windows_ret,
						&count_return) == 0) {
#if defined(DEBUG)
		fprintf(stderr,"Warning, Can't read WM_COLORMAP_WINDOWS property on Window %lu\n",window);
#endif
		return;
	}
	if (count_return == 0) {
		return;
	}

	for (i=0; i < count_return; i++,window_on_list++) {
		if (cmap_windows_ret[i] == window)
			break;
	}
	/* Make sure that the toplevel window is on the list, AND only
	 * concern ourselves with the max. number of colormaps that can
	 * be installed at any one time.
	 */
	max_installed = window_on_list < count_return ?
					count_return : count_return + 1;

	save_windows = (Window *)XtMalloc(max_installed *
					 sizeof(Window));

	sub = (SubWindowInfo *)XtMalloc(sizeof(SubWindowInfo) *
					max_installed);

	i = j = 0;
	/* If the toplevel window isn't in the list, then make it the
	 * first one.
	 */
	/* Let j index into the cmap_windows_ret array */
	for (; i < max_installed && j < count_return; j++) {
		/* First, do we already know about the window on the list? */
		if (cmap_windows_ret[j] == window) {
			save_windows[i++] =  cmap_windows_ret[j];
			continue;
		}
		if (XGetWindowAttributes(display,
				cmap_windows_ret[j],&xwa) == 0) {
	OlVaDisplayWarningMsg(display, OleNbadWindowAttr, OleTnotFound,
		OleCOlClientOlwmMsgs, OleMbadWindowAttr_notFound,
					cmap_windows_ret[j]);
				continue;
		}
		save_windows[i] =  cmap_windows_ret[j];
		/* If the window here on the list
		 * is the parent, then put it on the save
		 * list but DONT save the context with
		 * a sub structure.
		 * Then in WMInstallColormaps, just check
		 * each window on the list for the parent.
		 */
		sub[i].wm	= (Widget)wm;
		sub[i].colormap = xwa.colormap;
		sub[i].topwindow = window;
		sub[i].window = cmap_windows_ret[j];
		XSaveContext(display,cmap_windows_ret[j],
			wm_unique_context,
				(XtPointer)(&sub[i]));
		XSelectInput(display,sub[i].window,
			StructureNotifyMask|ColormapChangeMask);
		i++;
		/* If you get a StructureNOtify and the window
		 * has been destroyed, then there's no need to
		 * look for events anymnore, and you can remove
		 * the context (XDeleteContext().  Just check the
		 * event when it comes in to see if we own it; if
		 * not, then dump it.  If we do (e.g., it's a top-
		 * level window and it's decorated) then dispatching
		 * the event is just fine.
		 */
	}
		/* Don't save a context if it's a toplevel window
		 * that is on the list; we can GET to this window
		 * information later on if we need it and events
		 * will be processed for it properly anyway;
		 * DO save the window ID in save_windows; when this
		 * is checked when installing colormaps, the
		 * window manager will know that a context (in
		 * the "sub" regard) does not exist for it because
		 * it is a decorated window - got that?
		 * The reason we can't save a context for it
		 * is that it may have a context already saved
		 * for it because it had it's own WM_COLORMAP_WINDOWS
		 * property - total of two different contexts,
		 * one for toplevel windows, another for subwindows.
		 *
		 * We select for ColormapNotify events on this window;
		 * if one occurs, the event will be reported in the main
		 * loop; when it comes in, check the "new" field in the
		 * event structure; if set to True, then it is the result
		 * of the colormap window attribute changing for a window;
		 * if so, check to see if the window is a StepChild, and
		 * dispatch the event if it is; else, find out who the
		 * toplevel window is that is associated with it (do a
		 * findcontext), and adjust the info accordingly.
		 */
	wmstep->num_cmap_windows = i;
	wmstep->colormap_windows = save_windows;
	
	XSaveContext(display,window, wm_unique_context,
			(XtPointer)save_windows);
	if (cmap_windows_ret)
		XFree((char *)cmap_windows_ret);
} /* GetColormapWindows */


/*
 *************************************************************************
 * GetWMHints
 *	-Called from Initialize() - get WM_HINTS property (win. mgr. hints)
 * First arg is wmstep widget part.
 ****************************procedure*header*****************************
 */
extern void
GetWMHints OLARGLIST((wmstep, display, window))
OLARG(WMStepPart *, wmstep)
OLARG(Display *, display)
OLGRA(Window, window)
{

wmstep-> hints = 0;

if ((wmstep-> xwmhints = XGetWMHints(display, window)) != NULL)
   {
   wmstep-> hints |= WMHints;
   if (!(wmstep-> xwmhints-> flags & WindowGroupHint))
      wmstep-> xwmhints-> window_group = window;
   if (!(wmstep-> xwmhints-> flags & IconMaskHint))
      wmstep-> xwmhints-> icon_mask = None;
   if (!(wmstep-> xwmhints-> flags & IconPixmapHint))
      wmstep-> xwmhints-> icon_pixmap = None;
   }
else
   {
   wmstep-> hints &= ~(WMHints);

   /* Don't forget to free this in the Destroy function for the widget */
   wmstep-> xwmhints = (XWMHints *)calloc(sizeof(XWMHints), 1);

   wmstep-> xwmhints-> window_group = window;
   wmstep-> xwmhints-> flags        = InputHint;
   wmstep-> xwmhints-> input        = True;
   }
#ifdef POSTMARK
/*
if (!XGetClassHint(display, window, &wmstep-> classhint))
   {
   wmstep-> classhint.res_name  = strdup(wmstep-> icon_name);
   wmstep-> classhint.res_class = NULL;
   }
 */

if (wmstep-> xwmhints-> icon_pixmap == None && wmstep-> classhint.res_name != NULL)
   {
   char * iconname;
   char * maskname;

   iconname = XGetDefault(display, wmstep-> classhint.res_name, "iconfile");
   FPRINTF((stderr, "iconfile for '%s' is '%s'\n", 
      wmstep-> classhint.res_name, iconname));

   if (iconname)
      {
      unsigned int    icon_width;
      unsigned int    icon_height;
      int    icon_x;
      int    icon_y;
      Pixmap icon;
      Pixmap mask;
      if (XReadBitmapFile(display, window, iconname, 
         &icon_width, &icon_height, &icon, &icon_x, &icon_y) != BitmapSuccess)
#if !defined(I18N)
         (void)fprintf(stderr, "can't read bitmap file!\n");
#else
	OlVaDisplayWarningMsg(display, OleNbadBitmap, OleTbadRead,
			OleCOlClientOlwmMsgs, OleMbadBitmap_badRead, NULL);
#endif
         {
         wmstep-> xwmhints-> flags |= IconPixmapHint;
         wmstep-> xwmhints-> icon_pixmap = icon;
/*
            wmstep-> destroyhints |= DestroyIconMask;
*/
         maskname = XGetDefault(display, wmstep-> classhint.res_name, "iconmask");
         if (maskname)
            {
            if (XReadBitmapFile(display, window, maskname,
               &icon_width, &icon_height, &mask, &icon_x, &icon_y) != BitmapSuccess)
#if !defined(I18N)
               (void)fprintf(stderr, "can't read bitmap file!\n");
#else
	OlVaDisplayWarningMsg(display, OleNbadBitmap, OleTbadRead,
			OleCOlClientOlwmMsgs, OleMbadBitmap_badRead, NULL);
#endif
            wmstep-> xwmhints-> flags |= IconMaskHint;
            wmstep-> xwmhints-> icon_mask = mask;
/*
            wmstep-> destroyhints |= DestroyIconMask;
*/
            }
         }
      }
   }
#endif /* POSTMARK */

} /* end of GetWMHints */

/*
 *************************************************************************
 * GetNormalHints
 *
 * Called from Initialize(), and ClientPropertyChange(). Get
 * WM_NORMAL_HINTS property.
 ****************************procedure*header*****************************
 */

extern void
GetNormalHints OLARGLIST((wmstep, display, window, screen))
OLARG(WMStepPart *, wmstep)
OLARG(Display *, display)
OLARG(Window, window)
OLGRA(Screen *, screen)
{

/*
 * in R4 it may be desirable to use:
 * XGetWMNormalHints(display, window, &wmstep-> xnormalsize, &supplied_return);
 * since the size of the structure changed!
 */
long	supplied;
int	minwidth,
	minheight;

memset(&wmstep-> xnormalsize, 0, sizeof(wmstep-> xnormalsize));
if (currentGUI == OL_OPENLOOK_GUI) {
	minwidth = MINIMUM_WIDTH;
	minheight = MINIMUM_HEIGHT;
}
else {
	if (wmstep->decorations & WMResizable) {
		minwidth = Mresminwidth;
		minheight = Mresminheight;
	}
	else {
		minwidth = Mnoresminwidth;
		minheight = Mnoresminheight;
	}
} /* else (Motif) */

if (XGetWMNormalHints(display, window, &wmstep-> xnormalsize,&supplied))
   {
   if (!(wmstep-> xnormalsize.flags & PMinSize))
      {
      wmstep-> xnormalsize.min_width = minwidth;
      wmstep-> xnormalsize.min_height = minheight;
      }
   else
      {
      if (wmstep-> xnormalsize.min_width < minwidth)
         wmstep-> xnormalsize.min_width = minwidth;
      if (wmstep-> xnormalsize.min_height < minheight)
         wmstep-> xnormalsize.min_height = minheight;
      }
   if (!(wmstep-> xnormalsize.flags & PBaseSize))
      {
	/* Base width/height not provided, use minimum width, ht. */
      wmstep-> xnormalsize.base_width = wmstep-> xnormalsize.min_width;
      wmstep-> xnormalsize.base_height = wmstep-> xnormalsize.min_height;
      }
   else
      {

	/* Base size provided; if minimum size is not, use base size in
	 * it's place.
	 */
	if (!(wmstep-> xnormalsize.flags & PMinSize)) {
		if (wmstep-> xnormalsize.base_width > minwidth)
			wmstep->xnormalsize.min_width =
					wmstep->xnormalsize.base_width;
		if (wmstep-> xnormalsize.base_height > minheight)
			wmstep->xnormalsize.min_height =
					wmstep->xnormalsize.base_height;
	}

	/* Base width and height - make base width (height) at least min_width
	 * (height).  The only time this may "ruin" a client's width/height
	 * ratio is if the minimum width (or height) provided is less than
	 * MINIMUM_WIDTH or MINIMUM_HEIGHT.
	 */
      if (wmstep-> xnormalsize.base_width < wmstep-> xnormalsize.min_width)
         wmstep-> xnormalsize.base_width = wmstep-> xnormalsize.min_width;
      if (wmstep-> xnormalsize.base_height < wmstep-> xnormalsize.min_height)
         wmstep-> xnormalsize.base_height = wmstep-> xnormalsize.min_height;
      }
   if (!(wmstep-> xnormalsize.flags & PMaxSize))
      {
      wmstep-> xnormalsize.max_width = 0;
      wmstep-> xnormalsize.max_height = 0;
      }
   if (wmstep-> xnormalsize.max_width && 
       wmstep-> xnormalsize.max_width < wmstep-> xnormalsize.min_width)
      wmstep-> xnormalsize.max_width = 0;
   
   if (wmstep-> xnormalsize.max_height && 
       wmstep-> xnormalsize.max_height < wmstep-> xnormalsize.min_height)
      wmstep-> xnormalsize.max_height = 0;

   /* the windowAttributes field of wmstep widget struct will be NULL at this
    * point IF the window was attempting to be mapped AFTER window manager 
    * had started; if window was already mapped prior to starting window mgr.,
    * then windowAttributes field has a valid pointer.
    */
   if (wmstep-> windowAttributes != NULL || (wmstep-> hints & WMNormalSizeHints))
      ;  /* reparenting a mapped window */
   else
      {
	/*
	  * If the USPosition flag is on, then client should have placed
	  * the window where the user wanted it.  Set:
          * wmstep-> prev.x      = wmstep->windowattributes.x;
          * wmstep-> prev.y      = wmstep->windowattributes.y;
	  *
	  * This can be done in Initialize() by checking to see if USPosition
	  * is true.
	  * The same is true when USPosition is not set - in Initialize,
	  * deduce the position of the next window to be mapped on the
	  * screen (diagonally across the screen, left to right- reset
	  * CurrentX, CurrentY.
	  * 
	  * If the USSize flag is turned on, Initialize() will look at
	  * the window attributes width, height (and set wmstep->prev.width,
	  * prev.height appropriately).  If this flag is off, then
	  * we may resize the window if necessary such that the full title
	  * is visible in the title bar.
	  */
      }
   wmstep-> hints |= WMNormalSizeHints;
   }
else
   {
   wmstep-> hints &= (~WMNormalSizeHints);
   wmstep-> xnormalsize.min_width = minwidth;
   wmstep-> xnormalsize.min_height = minheight;
   }

} /* end of GetNormalHints */

/*
 *************************************************************************
 * GetBusyState 
 *
 ****************************procedure*header*****************************
 */

extern void
GetBusyState OLARGLIST((wmstep, display, window))
OLARG(WMStepPart *, wmstep)
OLARG(Display *, display)
OLGRA(Window, window)
{
long state;

if (GetWMWindowBusy(display, window, &state) == Success)
   {
   if (state)
      wmstep-> decorations |= WMBusy;
   else
      wmstep-> decorations &= ~WMBusy;
   }

} /* end of GetBusyState */

/*
 *************************************************************************
 * GetMotifDecorations.
 *
 ****************************procedure*header*****************************
 */
void
GetMotifDecorations OLARGLIST((wmstep, display, window))
OLARG(WMStepPart *, wmstep)
OLARG(Display *, display)
OLGRA(Window, window)
{
/* At this point, 
 * if transient window, decorations = limited menu and header;
 *	else it has full menu, resize corners, and header.
 */

Boolean transient = (wmstep->transient_parent != (Window)NULL);

/* If it has the transient property, then it must have a limited
 * menu - it isn't allowed to have a maximize or minimize button
 * on it.  But, some decorations can be removed from it.
 */

	/* This flag tells me that I make use of the MWM_HINTS
	 * property.  If I do, then I may be dealing with a
	 * Motif client or a MooLit client, but not a pure Open Look
	 * client (pre-GS4M)
	 */
	wmstep->decorations |= WMUsesMwmh;

	if (wmstep->mwmh.flags & MWM_HINTS_DECORATIONS) {
		unsigned int decor = wmstep->mwmh.decorations;
		/* Keep in mind, no minimize or maximize buttons if
		 * transient.
		 */
		if (decor & MWM_DECOR_ALL) {
			/* turn off these decorations */
			if (decor & MWM_DECOR_RESIZEH)
				wmstep->decorations &= ~WMResizable;
			if (decor & MWM_DECOR_TITLE) {
				wmstep->decorations &= ~WMHeader;
			}
			  if (currentGUI == OL_MOTIF_GUI) {
			if (decor & MWM_DECOR_MINIMIZE)
				wmstep->decorations &= ~WMMinimize;
			if (decor & MWM_DECOR_MAXIMIZE)
				wmstep->decorations &= ~WMMaximize;
			  }
			if (decor & MWM_DECOR_BORDER)
				wmstep->decorations &= ~WMBorder;
			if (decor & MWM_DECOR_MENU)  {
				wmstep->decorations &= 
					~(WMHasFullMenu|WMHasLimitedMenu);
				wmstep->decorations &= ~WMMenuButton;
			}
			/* Fix up - if resizable then it has a border */
			if (wmstep->decorations & WMResizable) {
				wmstep->decorations |= WMBorder;
			}
			/* if has any buttons then it has a title bar -
			 * motif mode only
			 */
			  if (currentGUI == OL_MOTIF_GUI)
			if (wmstep->decorations & (WMHasFullMenu|
				WMHasLimitedMenu| WMMenuButton |
				WMMinimize|WMMaximize) ) {
					wmstep->decorations |= WMHeader;
			}
		}
		else {
			/* If transient, then you can't specify the
			 * decorations outright, but you can take away.
			 */
			if (!transient) {
				wmstep->decorations = 0;
				if (decor & MWM_DECOR_RESIZEH)
					wmstep->decorations |= WMResizable;
				if (decor & MWM_DECOR_TITLE)
					wmstep->decorations |= WMHeader;
				if (decor & MWM_DECOR_BORDER)
					wmstep->decorations |= WMBorder;
				/* Minimize/Maximize buttons in Motif mode */
				   if (currentGUI == OL_MOTIF_GUI) {
				if (decor & MWM_DECOR_MINIMIZE)
					wmstep->decorations |= WMMinimize;
				if (decor & MWM_DECOR_MAXIMIZE)
					wmstep->decorations |= WMMaximize;
				   } /* Motif GUI */
				if (decor & MWM_DECOR_MENU) {
					wmstep->decorations |= WMHasFullMenu;
					wmstep->decorations |= WMMenuButton;
				}
			} /* not transient */
			else {
				/* transient */
				if (!(decor & MWM_DECOR_RESIZEH)) {
					wmstep->decorations &= ~WMResizable;
				}
				if (!(decor & MWM_DECOR_MENU)) {
					wmstep->decorations &= ~WMMenuButton;
				}
				if (!(decor & MWM_DECOR_BORDER)) {
					wmstep->decorations &= ~WMBorder;
				}
				if (!(decor & MWM_DECOR_TITLE)) {
					wmstep->decorations &= ~WMHeader;
				}
			} /* else transient */
		} /* ALL not specified */ 

			/* Fix up - if resizable then it has a border */
			if (wmstep->decorations & WMResizable)
				wmstep->decorations |= WMBorder;
			if (wmstep->decorations & WMMenuButton ||
			    wmstep->decorations & WMMinimize ||
			    wmstep->decorations & WMMaximize)
				wmstep->decorations |= WMHeader;
			    
	} /* Decorations flag */

	/* Now check the functions */

	if (wmstep->mwmh.flags & MWM_HINTS_FUNCTIONS) {
		unsigned int funcs = wmstep->mwmh.functions;
		if (funcs & MWM_FUNC_ALL) {
			if (funcs & MWM_FUNC_RESIZE) {
				wmstep->decorations &=~WMResizable;
				wmstep->menu_functions &= ~WMFuncResize;
			}
			if (funcs & MWM_FUNC_MOVE)
				wmstep->menu_functions &=~WMFuncMove;
			if (funcs & MWM_FUNC_CLOSE)
				wmstep->menu_functions &=~WMFuncClose;
			if (funcs & MWM_FUNC_MINIMIZE) {
				wmstep->menu_functions &=~WMFuncMinimize;
				if (currentGUI == OL_MOTIF_GUI)
					wmstep->decorations &=~WMMinimize;
			}
			if (funcs & MWM_FUNC_MAXIMIZE) {
				if (currentGUI == OL_MOTIF_GUI)
					wmstep->decorations &=~WMMaximize;
				wmstep->menu_functions &= ~ WMFuncMaximize;
			}
		}
		else {
			/* watch out here */
			wmstep->menu_functions = 0;
			if (funcs & MWM_FUNC_RESIZE)
				wmstep->menu_functions |= WMFuncResize;
			else
				wmstep->decorations &= ~WMResizable;
			if (funcs & MWM_FUNC_MOVE)
				wmstep->menu_functions |= WMFuncMove;
			if (funcs & MWM_FUNC_CLOSE)
				wmstep->menu_functions |= WMFuncClose;
			if (!transient) {
				if (funcs & MWM_FUNC_MINIMIZE)
				   wmstep->menu_functions |= WMFuncMinimize;
				if (funcs & MWM_FUNC_MAXIMIZE)
				   wmstep->menu_functions |= WMFuncMaximize;
			}
		} /* else */
	} /* flags & functions */
	if (wmstep->menu_functions) {
		/* There is at least one menu function.
		 * WMMenuButton tells me that the window has a
		 * menu button decoration.  But if the button isn't
		 * present, then it's still possible to bring up a
		 * window menu with the keyboard.
		 */
		if ( !(wmstep->decorations &
				(WMHasFullMenu | WMHasLimitedMenu) ) ) {
			if (wmstep->transient_parent)
				wmstep->decorations |= WMHasLimitedMenu;
			else
				wmstep->decorations |= WMHasFullMenu;
		}
	}
		

	/* This tells us that it is a Moolit Motif mode client, OR a
	 * strictly Motif client.
	 */
	wmstep->decorations |= WMUsesMwmh;

} /* GetMotifDecorations */

/*
 *************************************************************************
 * GetAllDecorations - OpenLook and Motif, if they exist.
 *
 ****************************procedure*header*****************************
 */
extern void 
GetAllDecorations OLARGLIST((wmstep, display, window))
OLARG(WMStepPart *, wmstep)
OLARG(Display *, display)
OLGRA(Window, window)
{
Atom * atoms = NULL;
Atom mwm_hints = MWM_HINTS;
PropMwmHints	mwmh;
int	n;
int	go = 0;

/* Both GUIs - look for transient hint */
/* Remember, if you get into this function, then regardless of the mode
 * you are in, you must be dealing with either a moolit or a MOTIF client
 * and not an Open Look (non-moolit) client.
 */
	if (XGetTransientForHint(display, window, &wmstep-> transient_parent)){
   		   wmstep-> decorations = WMHeader | WMHasLimitedMenu |
			WMBorder|WMResizable|WMMenuButton;
		   wmstep->menu_functions= WMFuncResize | WMFuncMove
						| WMFuncClose;
	}
	else { /* No TRANSIENT_FOR hint */
		/* Open Look - forget about minimize, maximize buttons, funcs-
		 * or should we?? See how they are used.
		 */
		if (currentGUI == OL_OPENLOOK_GUI) {
   		wmstep-> decorations = WMHeader | WMHasFullMenu |
			WMResizable|/*WMMinimize|WMMaximize|*/WMBorder;
		wmstep->menu_functions = WMFuncMove | WMFuncResize |
			WMFuncClose /*| WMFuncMinimize|WMFuncMaximize*/;
		}
		else { /* MOTIF */
		/* In Motif, we have the WMMenuButton decoration -
		 * this isn't necessary in Open Look mode, because
		 * in OL, just the presence of the WMHasFullMenu or
		 * WMHasLimitedMenu is sufficient to draw or not to
		 * draw a button (Limited menu might be a pin)
		 */
   		wmstep-> decorations = WMHeader | WMHasFullMenu |
				WMResizable|WMMinimize|WMMaximize|WMBorder|
				WMMenuButton;
		wmstep->menu_functions = WMFuncMove | WMFuncResize |
			WMFuncClose | WMFuncMinimize|WMFuncMaximize;
		}
	}
		/* First check to see if it is an Open Look or MooLIT client:
		 * read Open Look properties first.
		 */

		if (GetOLWinAttr(display, window, &wmstep-> olwa)  == Success){
			go++;
			wmstep->decorations |= WMUsesOlwa;
		}
		else if
	    	   (GetWMDecorationHints(display, window, &wmstep-> wmdh) ==
							 Success) {
			go++;
			wmstep->decorations |= WMUsesWmdh;
		}
		else if  ((atoms = GetAtomList(
				display, window,
				XA_OL_DECOR_DEL(display), &n, False)) != NULL ||
	    		(atoms = GetAtomList(
				display, window,
			 	XA_OL_DECOR_ADD(display), &n, False)) != NULL){
					go++;
					free(atoms);
					atoms = NULL;
				}

		/* if the currentGUI is Open Look, AND an Open Look property
		 * is present, then we don't need the motif decorations,
		 * just call GetDecorations.
		 */
		if (go && currentGUI == OL_OPENLOOK_GUI)
			GetDecorations(wmstep, display, window);
		else
			/* This isn't an OPEN LOOK/MooLIT client, and/or the
			 * current mode isn't Open Look - it's Motif.  Check to
			 * see if it is strictly a MOTIF client; if so,
			 * we'll deal with it.
			 *  -- _OlGetMWMHints() is in Vendor.c. --
			 * If it has MWM_HINTS, we can not assume that
			 * it is a MooLIT client even if it also has
			 * OLWinAttrs on it; therefore, we must disect the
			 * the MWM_HINTS property in GetMotifDecorations()
			 * very, very carefully.
			 */
		
			if (_OlGetMWMHints(display, window, &wmstep->mwmh)
							 == Success) {
				GetMotifDecorations(wmstep, display, window);
			}
			else /* neither is present, get O.L. defaults */
				GetDecorations(wmstep, display, window);
} /* GetAllDecorations */

/*
 *************************************************************************
 * GetDecorations  - read in  OPEN LOOK decoration properties.
 *
 * Read:
 *   - WM_TRANSIENT_FOR property into wmstep->transient_parent,
 *   fill in wmdtep->decorations is present;
 *   - OL_WIN_ATTR property into wmstep->olwa;
 *   fill in wmstep->decorations if present, use defaults if needed;
 *   - if OL_WIN_ATTR not present, read WM_DECORATION_HINTS;
 *    once again, fill in decorations if present, use defaults if needed;
 *
 *   - Call GetAddDecorations() and GetDelDecorations() to read
 *    _OL_DECOR_ADD and _OL_DECOR_DELETE properties (if present)
 *   
 ****************************procedure*header*****************************
 */

extern void
GetDecorations OLARGLIST((wmstep, display, window))
OLARG(WMStepPart *, wmstep)
OLARG(Display *, display)
OLGRA(Window, window)
{
Boolean transient = (wmstep->transient_parent != (Window)NULL);

if ( (wmstep->decorations & WMUsesOlwa)|| GetOLWinAttr(display, window,
		&wmstep->olwa) == Success )
   {
   if (wmstep-> olwa.flags & _OL_WA_WIN_TYPE)
      {
      Atom win_type = wmstep-> olwa.win_type;
     
      if (win_type == XA_OL_WT_BASE(display)) {
	 if (currentGUI == OL_OPENLOOK_GUI)
		/* Note how the WMHasFullMenu implies the presence of
		 * a full menu AND a menu button.
		 */
            wmstep-> decorations = WMHeader | WMResizable | WMHasFullMenu;
	 else {
		wmstep->decorations = WMHeader|WMResizable | WMHasFullMenu|
			WMMinimize | WMMaximize | WMBorder|WMMenuButton;
		wmstep->menu_functions = WMFuncClose|WMFuncMove|WMFuncResize |
			WMFuncMinimize | WMFuncMaximize;
	 } /* MOTIF */
	} /* BASE */
      else if (win_type == XA_OL_WT_PROP(display)) {
	if (currentGUI == OL_OPENLOOK_GUI)
         wmstep-> decorations = WMHeader | WMHasLimitedMenu | WMPushpin;
	else {
	  wmstep->decorations = WMHeader | WMHasLimitedMenu | WMBorder |
		WMMenuButton;
	  wmstep->menu_functions = WMFuncMove | WMFuncClose;
	}
      } /* PROP */
      else if (win_type == XA_OL_WT_CMD(display)) {
	if (currentGUI == OL_OPENLOOK_GUI)
         wmstep-> decorations = WMHeader | WMHasLimitedMenu | WMPushpin |
				WMResizable;
	else {
		wmstep->decorations = WMHeader | WMHasLimitedMenu | WMResizable|
			WMBorder | WMMenuButton;
		wmstep->menu_functions = WMFuncMove|WMFuncResize|WMFuncClose;
	} /* Motif */
	} /* CMD */
      else if (win_type == XA_OL_WT_HELP(display)) {
		if (currentGUI == OL_OPENLOOK_GUI)
         	  wmstep-> decorations = WMHeader | WMHasLimitedMenu |
			WMPushpin | WMResizable;
		else {
		wmstep->decorations = WMHeader | WMHasLimitedMenu | WMResizable|
			WMBorder | WMMenuButton;
		wmstep->menu_functions = WMFuncMove|WMFuncResize|WMFuncClose;
		} /* MOTIF */
	} /* HELP */
      else if (win_type == XA_OL_WT_NOTICE(display) ||
		win_type == XA_OL_WT_OTHER(display)) {
	 /* You can move these windows, but no decorations */
		wmstep->decorations = WMUsesOlwa;
	if (currentGUI == OL_MOTIF_GUI)
		wmstep->menu_functions = WMFuncMove | WMFuncClose;
	} /* NOTICE or OTHER */
      else /* Assume base window */
	if (currentGUI == OL_OPENLOOK_GUI)
         wmstep->decorations = WMHeader | WMResizable | WMHasFullMenu;
	else {
		wmstep->decorations = WMHeader | WMResizable | WMHasFullMenu|
			WMBorder |WMMinimize | WMMaximize |
			WMMenuButton;
		wmstep->menu_functions = WMFuncClose|WMFuncMove|WMFuncResize |
			WMFuncMinimize | WMFuncMaximize;
	} /* MOTIF */
      } /* WIN_TYPE flag of olwa specified */
   else /* WIN_TYPE not specified, assume base window unless transient */
	if (!transient) {
	   if (currentGUI == OL_OPENLOOK_GUI)
		wmstep-> decorations = WMHeader | WMResizable | WMHasFullMenu;
	   else {
		wmstep->decorations = WMHeader | WMResizable | WMHasFullMenu|
			WMBorder |WMMinimize | WMMaximize;
		wmstep->menu_functions = WMFuncClose|WMFuncMove|WMFuncResize |
			WMFuncMinimize | WMFuncMaximize;
	   }
	} /* Not transient */
	else { /* transient */
		/* Don't give them a pushpin here... */
		if (currentGUI == OL_OPENLOOK_GUI)
			wmstep->decorations = WMHeader | WMResizable |
				WMHasLimitedMenu ;
		else {
		wmstep->decorations = WMHeader | WMHasLimitedMenu | 
			WMResizable| WMBorder|WMMenuButton;
		wmstep->menu_functions = WMFuncMove|WMFuncResize|WMFuncClose;
		} /* Motif */
	} /* transient */

   if (currentGUI == OL_OPENLOOK_GUI && wmstep-> olwa.flags & _OL_WA_PIN_STATE)
      {
      wmstep-> decorations |= WMPushpin;
      if (wmstep-> olwa.pin_state)
         wmstep-> decorations |= WMPinIn;
      else
	if ( (help_parent != NULL) && (window == help_parent->wmstep.window)) {
		/* This is our help window - force the pin to be in, I don't
		 * care what it says. (If vendor shell caches the value,
		 * it may not work anyway because our values may be diff.
		 */
         wmstep-> decorations |= WMPinIn;
	}
      }
   if (wmstep-> olwa.flags & _OL_WA_CANCEL)
      if (wmstep-> olwa.cancel)
         wmstep-> decorations |= WMCancel;
   if (wmstep-> olwa.flags & _OL_WA_MENU_TYPE)
      {
      Atom menu_type = wmstep-> olwa.menu_type;
      wmstep-> decorations &= (~(WMHasFullMenu | WMHasLimitedMenu));
      if (menu_type == XA_OL_MENU_LIMITED(display))
         wmstep-> decorations |= WMHasLimitedMenu;
      else
      if (menu_type == XA_OL_MENU_FULL(display))
         wmstep-> decorations |= WMHasFullMenu;
      else
      if (menu_type == XA_OL_NONE(display))
         ;
      else
         ;
      } /* flags & MENU_TYPE */
   FPRINTF((stderr,"decorations set using OLWinAttr\n"));
   } /* GetOLWinAttr */
else if ( (wmstep->decorations & WMUsesWmdh) || GetWMDecorationHints(display,
			window, &wmstep->wmdh) == Success)
   {
   Atom menu_type = wmstep-> wmdh.menu_type;
   if (menu_type == MENU_FULL) {
	/* THIS should really be overriden by the transient property, if
	 * present.
	 */
	if (currentGUI == OL_OPENLOOK_GUI)
		wmstep->decorations = WMHeader | WMResizable | WMHasFullMenu;
	else {
		wmstep->decorations = WMHeader | WMResizable | WMHasFullMenu|
			WMMenuButton | WMBorder |WMMinimize | WMMaximize;
		wmstep->menu_functions = WMFuncClose|WMFuncMove|WMFuncResize |
			WMFuncMinimize | WMFuncMaximize;
	} /* MOTIF */
   } /* menu_type == FULL */
   else if (menu_type == MENU_LIMITED) {
	if (currentGUI == OL_OPENLOOK_GUI)
		wmstep-> decorations = WMHeader | WMHasLimitedMenu |
			WMResizable;
	else {
		wmstep->decorations = WMHeader | WMHasLimitedMenu |
			WMMenuButton | WMResizable | WMBorder;
		wmstep->menu_functions = WMFuncMove | WMFuncResize |
			WMFuncClose;
	} /* MOTIF */
   } /* menu_type == LIMITED */
   else if (menu_type == MENU_NONE) {
	wmstep-> decorations = WMNoDecorations; 
	if (currentGUI == OL_MOTIF_GUI)
		wmstep->menu_functions = WMFuncMove | WMFuncClose;
   } /* menu_type == MENU_NONE */
    else /* default */
	if (!transient) {
		if (currentGUI == OL_OPENLOOK_GUI)
		  wmstep-> decorations = WMHeader | WMResizable | WMHasFullMenu;
		else {
			wmstep->decorations = WMHeader | WMResizable |
				WMMenuButton | WMHasFullMenu| WMBorder|
				WMMinimize | WMMaximize;
			wmstep->menu_functions = WMFuncClose | WMFuncMove |
				WMFuncResize | WMFuncMinimize | WMFuncMaximize;
		} /* MOTIF */
	} /* Not transient */
	else { /* transient */
		if (currentGUI == OL_OPENLOOK_GUI)
			/* Don't give them a pushpin here... */
			wmstep->decorations = WMHeader | WMResizable |
					WMHasLimitedMenu ;
		else {
			wmstep->decorations = WMHeader | WMHasLimitedMenu |
				WMMenuButton | WMResizable | WMBorder;
			wmstep->menu_functions = WMFuncMove | WMFuncResize |
					WMFuncClose;
		} /* Motif */
	} /* transient */

   if (currentGUI == OL_OPENLOOK_GUI &&
	wmstep-> wmdh.flags & WMDecorationPushpin && menu_type != MENU_FULL)
      {
      wmstep-> decorations |= WMPushpin;
      if (wmstep-> wmdh.pushpin_initial_state)
         wmstep-> decorations |= WMPinIn;
      }
   if (wmstep-> wmdh.flags & WMDecorationHeader) {
		wmstep-> decorations |= WMHeader;
   }
   if (wmstep-> wmdh.flags & WMDecorationResizeable) {
      wmstep-> decorations |= WMResizable;
	if (currentGUI == OL_MOTIF_GUI) {
		wmstep->menu_functions |= WMFuncResize;
		wmstep->decorations |= WMBorder;
	}
   }
  } /* WMDecorationHints */
GetAddDecorations(wmstep, display, window);
GetDelDecorations(wmstep, display, window);

} /* end of GetDecorations */

/*
 * GetAddDecorations 
 * A factor in OPEN LOOK or MooLIT applications.
 */

extern void
GetAddDecorations OLARGLIST((wmstep, display, window))
OLARG(WMStepPart *, wmstep)
OLARG(Display *, display)
OLGRA(Window, window)
{
Atom * atoms = NULL;
int    n;

atoms = GetAtomList(display, window, XA_OL_DECOR_ADD(display), &n, False);

if (atoms != NULL)
   {
   while (n--)
      {
      if (atoms[n] == XA_OL_DECOR_CLOSE(display)) {
         wmstep-> decorations |= WMHasFullMenu;
	/* This must be an OPEN LOOK application - add in the functionality,
	 * too.
	 */
	}
      else
         if (atoms[n] == XA_OL_DECOR_RESIZE(display))
            wmstep-> decorations |= WMResizable;
         else
            if (atoms[n] == XA_OL_DECOR_HEADER(display))
               wmstep-> decorations |= WMHeader;
            else
               if (atoms[n] == XA_OL_DECOR_PIN(display) && currentGUI == OL_OPENLOOK_GUI)
                  wmstep-> decorations |= WMPushpin;
      }
   free(atoms);
   }

} /* end of GetAddDecorations */

/*
 *************************************************************************
 * GetDelDecorations 
 * A factor in OPEN LOOK or MooLIT applications.
 *
 ****************************procedure*header*****************************
 */

extern void
GetDelDecorations OLARGLIST((wmstep, display, window))
OLARG(WMStepPart *, wmstep)
OLARG(Display *, display)
OLGRA(Window, window)
{
Atom * atoms = NULL;
int    n;

atoms = GetAtomList(display, window, XA_OL_DECOR_DEL(display), &n, False);

if (atoms != NULL)
   {
   while (n--)
      {
      if (atoms[n] == XA_OL_DECOR_CLOSE(display))
         wmstep-> decorations &= ~WMHasFullMenu;
      else
         if (atoms[n] == XA_OL_DECOR_RESIZE(display)) {
            wmstep-> decorations &= ~WMResizable;
	}
         else
            if (atoms[n] == XA_OL_DECOR_HEADER(display))
               wmstep-> decorations &= ~WMHeader;
            else
               if (atoms[n] == XA_OL_DECOR_PIN(display))
                  wmstep-> decorations &= ~WMPushpin;
      }
   free(atoms);
   }

} /* end of GetDelDecorations */

/*
 *************************************************************************
 * GetWindowName
 * - Called from Initialize() and ClientPropertyChange.  Get windows title
 * (XA_WM_NAME property)
 ****************************procedure*header*****************************
 */

extern void
GetWindowName OLARGLIST((wmstep, display, window))
OLARG(WMStepPart *, wmstep)
OLARG(Display *, display)
OLGRA(Window, window)
{
/*
 * in R4 use:
 * XGetWMName(display, window, &text_prop_return);
 * then find the string to use...
 */
XTextProperty tp;
int count = 0;
char **namelist = (char **)0;

if (wmstep-> window_name) {
	free(wmstep-> window_name);
	wmstep->window_name = NULL;
}

if (XGetWMName(display, window, &tp) != False) {
	/* Use XTextPropertyToStringList(), save the list of
	 * returned strings in wmstep->window_name
	 */
	if (XTextPropertyToStringList(&tp, &namelist, &count)
		== False) {
		if (namelist) {
			XFreeStringList(namelist);
			namelist = (char **)0;
		}
	}
}

	if (namelist == NULL || *namelist[0] == '\0')
		wmstep-> window_name =
#if !defined(I18N)
			strdup("(Untitled)");
#else
			strdup( OlGetMessage(display, NULL, 0,
					OleNnoTitle, OleTnoTitle,
					OleCOlClientOlwmMsgs,
					OleMnoTitle_noTitle, NULL));
#endif
	else
		wmstep->window_name = strdup(*namelist);

if (namelist) {
	XFreeStringList(namelist);
	namelist = (char **)0;
}

} /* end of GetWindowName */

/*
 *************************************************************************
 * GetIconName
 *
 ****************************procedure*header*****************************
 */

extern void
GetIconName OLARGLIST((wmstep, display, window))
OLARG(WMStepPart *, wmstep)
OLARG(Display *, display)
OLGRA(Window, window)
{
/*
 * in R4 use:
 * XGetWMIconName(display, window, &text_prop_return);
 * then find the string to use...
 */
XTextProperty tp;
int count = 0;
char **namelist = (char **)NULL;

if (wmstep-> icon_name) {
	free(wmstep-> icon_name);
	wmstep->icon_name = NULL;
}

if (XGetWMIconName(display, window, &tp) != False) {
	/* Use XTextPropertyToStringList(), save the list of
	 * returned strings in wmstep->window_name
	 */
	if (XTextPropertyToStringList(&tp, &namelist, &count)
		== False) {
		if (namelist) {
			XFreeStringList(namelist);
			namelist = (char **)0;
		}
	}
}


if (namelist == NULL || *namelist[0] == '\0')
   wmstep-> icon_name = strdup(wmstep-> window_name);
else
   wmstep-> icon_name = strdup(*namelist);
if (namelist) {
   XFreeStringList(namelist);
   namelist = (char **)0;
}

} /* end of GetIconName */

/*
 *************************************************************************
 * GetWindowState
 * - Called from ReparentWindows().  Return current state of
 * the window (normal, iconic, withdrawn, etc.).
 *
 ****************************procedure*header*****************************
 */

extern int
GetWindowState OLARGLIST((display, window))
OLARG(Display *, display)
OLGRA(Window, window)
{

return (GetWMState(display, window));

} /* end of GetWindowState */

/*
 *************************************************************************
 * SetWindowState
 *
 * IconicState
 * NormalState
 * WithdrawnState
 *
 ****************************procedure*header*****************************
 */

extern void
SetWindowState OLARGLIST((wm, state, icon_window))
OLARG(WMStepWidget, wm)
OLARG(int, state)
OLGRA(Window, icon_window)
{
long array[2];
Window window = wm-> wmstep.window;
Display * dpy = XtDisplay((Widget)wm);
Atom	wm_state = XA_WM_STATE(dpy);

array[0] = state;
/*array[1] = wm->wmstep.xwmhints->icon_window;*/
array[1] = icon_window;

XChangeProperty(dpy, window, wm_state, wm_state, 32, 
   PropModeReplace, (unsigned char *)array, 2);

} /* end of SetWindowState */

/*
 *************************************************************************
 * SetPushpinState
 *
 * WMPushpinIsOut
 * WMPushpinIsIn
 *
 ****************************procedure*header*****************************
 */

extern void
SetPushpinState OLARGLIST((wm, state, unmap))
OLARG(WMStepWidget, wm)
OLARG(unsigned long, state)
OLGRA(int, unmap)
{
Display *	dpy = XtDisplay((Widget)wm);

if (currentGUI == OL_MOTIF_GUI)
	return;
wm-> wmstep.decorations &= ~(WMPinIn | WMPushpin);
wm-> wmstep.decorations |= state;

if (state & WMPinIn)
   state = WMPushpinIsIn;
else
   state = WMPushpinIsOut;

XChangeProperty(dpy, wm-> wmstep.window, XA_OL_PIN_STATE(dpy), 
   XA_INTEGER, 32, PropModeReplace, (unsigned char *)&state, 1);

if (unmap && state == WMPushpinIsOut)
   MenuDismiss((Widget)wm, (XtPointer)&wm, (XtPointer)NULL);


} /* end of SetPushpinState */

#if !defined(MAXCOLORMAPS)
#define MAXCOLORMAPS 20
#endif
/*
 *************************************************************************
 * SetFocus - called from many places;
 * From StartWindowManager(), called with args Frame (root window widget),
 * GainFocus.
 * From ClientEnterLeave() event handler, for pointer EnterNotify and
 * LeaveNotify events when the focus is real-estate based.
 * If called with from = LoseFocus, find a window in window_list->that will take
 * focus, and recursively call SetFocus with that WMStepWidget andi
 * from = GainFocus; Install colormap for window gaining focus, if needed
 * setcurrent - if == True, make this the current step widget; false -
 * don't
 *
 ****************************procedure*header*****************************
 */

void
SetFocus OLARGLIST((w, from, setcurrent))
OLARG(Widget, w)
OLARG(int , from)
OLGRA(int, setcurrent)
{
WMStepWidget wm        = (WMStepWidget)w;
Display *    display   = XtDisplay(wm);
WMStepPart * wmstep;
Window       window;
unsigned int model;
unsigned int has_focus;
int	i, j, k, cmapcount = 0;
XtPointer	data;
Window		*cmap_window_list;

if (w != Frame) {
	wmstep = (WMStepPart *)&(wm->wmstep);
	model     = wmstep-> protocols & FocusModel;
	has_focus = wmstep-> protocols & HasFocus;
	window = wmstep->window;
}
if (w == Frame)
   {
   /* case 1 - set focus to root window widget(Frame). (ignore 2nd arg) */
   CurrentColormapWindow = XtWindow(w);


   /*
    * CmapInWMList returns -1 if not in list, the index otherwise
    *
   if (!(CmapInWMList(DefaultColormapOfScreen(XtScreen(Frame))) )) {
    */
/*
   if ((CmapInWMList(DefaultColormapOfScreen(XtScreen(Frame))) )) {
 *
 	/* Place at head of list  = 0 */


/*
	if (MaxColormapCount > 1) {
		int last;
		if (CurrentColormapCount < MaxColormapCount) {
			last = CurrentColormapCount;
			CurrentColormapCount++;
		}
		else
			last = CurrentColormapCount - 1;
				
		for (i = last; i > 0; i--)
			CurrentColormap[i] = CurrentColormap[i-1];
	} /* end if (MaxColormapCount > 1) */

	if (CurrentColormap != DefaultColormapOfScreen(XtScreen(w))){
		XInstallColormap(display,
			 DefaultColormapOfScreen(XtScreen(w)));
		CurrentColormap = DefaultColormapOfScreen(XtScreen(w));
	}
/*
		XInstallColormap(display, CurrentColormap);
   }
 */
   if (wmrcs->pointerFocus) /* Real estate based */ {
      XSetInputFocus(display, PointerRoot, RevertToPointerRoot, LastEventTime);
}
   else /* Click-To-Type */ {
      XSetInputFocus(display, XtWindow(w), RevertToNone, LastEventTime);
	}
   }
else
   {
      if (from == LoseFocus)
         {
         WMStepWidget  candidate  = NULL;
         unsigned long last_focus = wm-> wmstep.focus;
         unsigned long temp_focus = 0;
         int i;

	 /* Loop through all decorated windows in window_list-> find the window
	  * that has the highest index in the list that is capable of getting
	  * input focus - the criteria:
	  * 	- not the current window (it is LOSING focus).
	  *	- not the help window.
	  *	- icon only as a last resort
	  */
	 if (!(wmrcs->pointerFocus))
	 	candidate = NextFocus((WMStepWidget)w, False);
         if (candidate)
            SetFocus((Widget)candidate, GainFocus, setcurrent);
         else
            {
	    /* No candidates in window_list (or pointerFocus) ->
	     * to get input focus, set to root
	     */
	if (wm->wmstep.is_current) {
		RemoveOldGrabs(wm);
		wm->wmstep.is_current = 0;
		if (wmstep->size != WMWITHDRAWN) {
			/* This window just became un-current */
			if (currentGUI == OL_OPENLOOK_GUI) {
				XRectangle rect;
				rect = HeaderRect(wm);
				OlClearRectangle(wm, &rect, False);
				EraseSelectBorder(wm);
			}
			else {
				if (IsIconic(wm) &&
		motwmrcs->iconDecoration & ICON_ACTIVELABEL)
					/* unmap it */
				CheckAIWDown(wm);
			} /* Motif mode */

			/* All cases, redisplay it */
			DisplayWM(wm, NULL);
		}
	} /* is current */

   if (wmrcs->pointerFocus) /* Real estate based */ {
      XSetInputFocus(display, PointerRoot, RevertToPointerRoot, CurrentTime);
}
   else /* Click-To-Type */ {
      XSetInputFocus(display, RootWindowOfScreen(XtScreen(wm)), RevertToNone,
							CurrentTime);
	}

            FPRINTF((stderr,"no candidate!!!\n"));
            }
         }
      else /* called with arg = GainFocus */
         {
	if (IsIconic(wm)) {
		if (wmrcs->pointerFocus) /* Real estate based */ {
		   XSetInputFocus(display, PointerRoot,
			RevertToPointerRoot, LastEventTime);
		}
		else /* Click-To-Type */ {
			XSetInputFocus(display, RootWindowOfScreen(
						XtScreen(wm)), 
				RevertToNone, LastEventTime);
		}
		if (setcurrent && !(wmstep->is_current))
               		SetCurrent(wm);
		return;
	}
 	CurrentColormapWindow = XtWindow(w);
	WMInstallColormaps(wm);

         switch (model) /* taken from window's protocols */
            {
            case NoInput:
		if (wmrcs->pointerFocus) /* Real estate based */ {
		   XSetInputFocus(display, PointerRoot,
			RevertToPointerRoot, LastEventTime);
		}
		else /* Click-To-Type */ {
			XSetInputFocus(display, RootWindowOfScreen(
						XtScreen(wm)), 
				RevertToNone, LastEventTime);
		}
		if (setcurrent && !(wmstep->is_current))
               		SetCurrent(wm);
               break;
            case Passive:
            case LocallyActive:
		if (!(wmstep->protocols & HasFocus))
			XSetInputFocus(display, window, RevertToNone,
							LastEventTime);
		if (setcurrent && !(wmstep->is_current))
               		SetCurrent(wm);
               break;
            case GloballyActive:
		if (!(wmstep->protocols & HasFocus)) {
/*
			XSetInputFocus(display, RootWindowOfScreen(
					XtScreen(wm)), RevertToNone,
					LastEventTime);
 */
		if (wmrcs->pointerFocus)
			XSetInputFocus(display, PointerRoot,
				RevertToPointerRoot, LastEventTime);
		else
			XSetInputFocus(display, RootWindowOfScreen(
					XtScreen(wm)), RevertToNone,
					LastEventTime);
		SendProtocolMessage(display, window,
				XA_WM_TAKE_FOCUS(display), CurrentTime);
	   }
		if (setcurrent && !(wmstep->is_current))
               		SetCurrent(wm);
               break;
            } /* switch */
	 /* if it uses motif input modal tricks, then this is the time to
	  * set them up.
	  */
	if (wm->wmstep.decorations & WMUsesMwmh &&
			   wm->wmstep.mwmh.flags & MWM_HINTS_INPUT_MODE) {
		/* what kind of input mode? */
		switch(wm->wmstep.mwmh.inputMode){
			case MWM_INPUT_SYSTEM_MODAL:
				/* Grab Pointer, forget about */
				break;
		} /* switch */
	} /* uses MWMH */
         } /* else Gainfocus */
   }

} /* end of SetFocus */

/*
 *************************************************************************
 * SetCurrent
 *
 ****************************procedure*header*****************************
 */
extern void
SetCurrent OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
	Cardinal	n;
	Cardinal	nchildren = num_wmstep_kids;
	WidgetList	children  = wmstep_kids;
	WMStepWidget	owm;
	XRectangle   rect;

	/* First thing, take care of the  focus count - used to be
	 * in ClientFocusChange, but now it's here where we can
	 * control it, and this function is called from there.
	 */ 
	/*wm->wmstep.focus = ++wm_focus_count; */

	XUngrabButton(
		XtDisplay((Widget)wm), AnyButton, AnyModifier,
			XtWindow((Widget)wm)
	);
	wm->wmstep.is_grabbed = False;
	
	/* The following two sentences should probably be executed before 
	 * the ungrab.  This should be re-visited.
	 */

	wm->wmstep.focus = FocusCount();
	if (wm->wmstep.is_current)
		return;

	for (n = 0; n < nchildren; n++) {
		owm = (WMStepWidget)children[n];
		if (owm != wm && !owm->wmstep.is_grabbed) {
			XGrabButton(XtDisplay((Widget)owm), AnyButton, 
				AnyModifier, XtWindow((Widget)owm),
				False, ButtonPressMask, GrabModeSync, 
				GrabModeSync, None, None);
			owm->wmstep.is_grabbed = True;
		}

		if ( XtClass(owm) == wmstepWidgetClass
		     && (owm-> wmstep.is_current == True) ) {
			owm-> wmstep.is_current = False;
			if (currentGUI == OL_OPENLOOK_GUI) {
				rect = HeaderRect(owm);
				OlClearRectangle(owm, &rect, False);
				EraseSelectBorder(owm);
				if (wmrcs->pointerFocus)
					EraseBN(owm);
			}
			else {
				if (IsIconic(owm) &&
		motwmrcs->iconDecoration & ICON_ACTIVELABEL)
					/* unmap it */
				CheckAIWDown(owm);
			} /* Motif mode */

			/* All cases, redisplay it */
			DisplayWM(owm, NULL);

			/* Undo the thickened border here.
			 * If it's iconic, then the title was bold -
			 * redisplay it so it loses the bold title,
			 * gets normal one. (XClearArea(), with arg 7
			 * == True, generates expose event.)
			 */
			if (IsIconic(owm)) {
				int diff = (2 * icon_height) / 3;
				XClearArea(XtDisplay(owm),XtWindow(owm),
					0, (2 * icon_height)/3,
					icon_width, icon_height - diff, True);
			}
			/* Remove Any grabs associated with any private key
			 * database.
			 */
			if (owm->wmstep.private_db)
				RemoveOldGrabs(owm);
		} /* end if (is_current) */
	} /* end for loop */

	wm-> wmstep.is_current = True;

	/* Grab the keys on the root window.  Only grab keys if they
	 * have an accelerator - otherwise, what's the point?
	 */
	if (wm->wmstep.private_db != NULL) {
		int k = 0;
		WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
		for (k=0; k < wmstep->private_buttons_used; k++)
		    if (wmstep->private_keys[k].default_value != NULL)
			OlGrabVirtualKey(Frame,
				wmstep->private_keys[k].virtual_name, False,
				GrabModeAsync, GrabModeSync);
	} /* private_db != NULL */

	/* Provide feedback here- thickened border ... 
	 * If it's iconic, then redisplay it so the title
	 * get's drawn with the bold font because it's been selected.
	 */
	if (IsIconic(wm)) {
		int diff;
		if (currentGUI == OL_OPENLOOK_GUI) {
			diff = (2 * icon_height) / 3;
			XClearArea(XtDisplay(wm),XtWindow(wm),
				0, (2 * icon_height)/3,
				icon_width, icon_height - diff, True);
			DrawSelectBorder(wm);
		}
		else {
			Boolean temp_bool;
			DisplayMotifIcon(wm);
			/* Call the expose proc for the occassions
			 * that this function is called directly
			 * and we need the active label displayed
			 * without waiting for it to be exposed.
			 */
			if (motwmrcs->iconDecoration & ICON_ACTIVELABEL)
				MotifAIWExpose(wm->wmstep.icon_widget,
				 (XtPointer)NULL, (XEvent *)NULL, &temp_bool);
		}
	}
	else {
		/* all cases, redisplay it */
		DisplayWM(wm, NULL);
	}
	return;		
} /* end of SetCurrent */

extern void
RemoveOldGrabs OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
	if (wm->wmstep.private_db != NULL) {
		int k = 0;
		for (k=0; k < wm->wmstep.private_buttons_used; k++)
			if(wm->wmstep.private_keys[k].default_value 
								!= NULL)
				    OlUngrabVirtualKey(Frame,
				     wm->wmstep.private_keys[k].virtual_name);
	} /* private_db != NULL */
}


/*
 *************************************************************************
 * SendWindowMovedEvent(): Send synthetic event to client when window
 * is moved by window manager (with mouse or keyboard).
 *
 ****************************procedure*header*****************************
 */
void
SendWindowMovedEvent OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
	XEvent	ev;
	Window window = wm->wmstep.window;
	Display *dsp = XtDisplay(wm);

	ev.xconfigure.type = ConfigureNotify;
	ev.xconfigure.display = dsp;
	ev.xconfigure.event = window;
	ev.xconfigure.window = window;
if (currentGUI == OL_OPENLOOK_GUI) {
	ev.xconfigure.x = wm->core.x + OriginX(wm);
	ev.xconfigure.y = wm->core.y + OriginY(wm);
	ev.xconfigure.width = ChildWidth(wm);
	ev.xconfigure.height = ChildHeight(wm);
}
else {
/* OriginX (Y) is the position of the client relative to the position of
 * the step widgets origin (e.g., not the root window - the step widgets
 * origin is at (0,0))
 */
	ev.xconfigure.x = wm->core.x + MOriginX(wm);
	ev.xconfigure.y = wm->core.y +  MOriginY(wm);
	ev.xconfigure.width = MChildWidth(wm);
	ev.xconfigure.height = MChildHeight(wm);
} /* Motif */
	ev.xconfigure.border_width = 0;
	ev.xconfigure.above = None;
	ev.xconfigure.override_redirect = False;
	ev.xconfigure.send_event = True;
	XSendEvent(dsp, window, False, StructureNotifyMask, &ev);
} /* SendWindowMovedEvend */


/*
 *************************************************************************
 * CmapInWMList.
 * - Is the colormap in the "install" list?
 ****************************procedure*header*****************************
 */
/*
extern int
CmapInWMList(cmap)
Colormap cmap;
{
int	i,
	len = CurrentColormapCount;

	for (i=0; i<len; i++)
		if (CurrentColormap[i] == cmap)
			return(i);
	return(-1);
} /* CmapInWMList */

/*
 *************************************************************************
 * ReworkColormapInfo():  If a new colormap is installed for a window
 * that isn't decorated (not a toplevel, a subwindow of one),
 * OR a subwindow that we were tracking for a colormap change is either no
 * longer on the list  or has been destroyed, then ditch it from our
 * knowledge.
 * op is either
 *	NEWCMAP - new colormap attribute for window;
 *	DESTROYCMAP - no longer need to keep track of this window.  This
 * 	  can result from either the window being tracked (or subwindow)
 * 	  getting a DestroyNotify or the WM_COLORMAP_WINDOWS property
 *	  changing and not including it anymore.
 *
 ****************************procedure*header*****************************
 */
extern int
ReworkColormapInfo OLARGLIST((dsp, window, op))
OLARG(Display *, dsp)
OLARG(Window, window)
OLGRA(int, op)
{
XtPointer subdata;
SubWindowInfo *subinfo;
XWindowAttributes xwa;

	switch(op) {
		case DESTROYCMAP:
			XSelectInput(dsp,window,
					NoEventMask);
			if(XFindContext(dsp,window,
				wm_unique_context,
				(caddr_t *)&subdata) == 0) {
				subinfo = (SubWindowInfo *)subdata;
				/* We no longer need the context; the
				 * window entry may very well be sitting
				 * around in the WM_COLORMAP_WINDOWS
				 * property, but that's O.K. - when
				 * the code that tries to install this
				 * window's colormap gets executed, it
				 * will not find it's context and thus
				 * it's colormap will not exist.  Something
				 * to look at at a later date...
				 */
				XDeleteContext(dsp, window,
					wm_unique_context);
			}
			break;
		case  NEWCMAP:
			/* A subwindow on a WM_COLORMAP_WINDOWS list
			 * most likely had a change in it's colormap.
			 * Fix up the context for it and if the
			 * toplevel window has input focus now, call
			 * WMInstallColormaps().
			 */
			if(XFindContext(dsp, window,
				wm_unique_context,
				(caddr_t *)&subdata) == 0) {
				Boolean flag = False;

				subinfo = (SubWindowInfo *)subdata;
				if (XGetWindowAttributes(dsp,
					window,&xwa) == 0) {
#if !defined(I18N)
					fprintf(stderr,"Warning: Can't get window attributes for WM_COLORMAP_WINDOWS subwindow %lu\n",window);
#else
	OlVaDisplayWarningMsg(dsp, OleNbadWindowAttr, OleTnotFound,
			OleCOlClientOlwmMsgs, OleMbadWindowAttr_notFound,
							window ,NULL);
#endif
					subinfo->colormap =
						DefaultColormapOfScreen(
							XtScreen(Frame));
					break;
				}
				if (xwa.colormap == None)
					xwa.colormap = DefaultColormapOfScreen(
						XtScreen(Frame));
				if (subinfo->colormap != xwa.colormap) {
					flag = True;
					subinfo->colormap = xwa.colormap;
				}
				if ( (CurrentColormapWindow ==
						 subinfo->topwindow) ||
				   CurrentColormapWindow ==
				     		XtWindow(subinfo->wm) )
					WMInstallColormaps(
						(WMStepWidget)subinfo->wm);
			} /* end if() */
			break;
		default: break;
		} /* end switch */
} /* ReworkColormapInfo */

/*
 *************************************************************************
 * WMInstallColormaps
 * - Install colormap for window of step widget wm.
 * Use WM_COLORMAP_WINDOWS property if present.
 *
 ****************************procedure*header*****************************
 */
extern void
WMInstallColormaps(wm)
WMStepWidget wm;
{
int		i,j,k;
Display		*display = XtDisplay(wm);
WMStepPart	*wmstep = &wm->wmstep;
int		num_cmap_windows = wmstep->num_cmap_windows;
XtPointer	data;

	/* If num_cmap_windows >0, this indicates that the WM_COLORMAP_WINDOWS
	 * property is present.
	 */
	if (num_cmap_windows == 0) {
		if (CurrentColormap !=
		    wmstep->window_attributes.colormap) {
			XInstallColormap(display,
				wmstep->window_attributes.colormap);
			CurrentColormap =
				wmstep->window_attributes.colormap;
		}
		return;
	} /* end if (num_cmap_windows == 0) */

	/* You got this far: go after the colormap_window */
	if (XFindContext(display,wmstep->window,
		wm_unique_context,
		(caddr_t *)&data) == 0) {
		/* XFindContext() passed, this window has sub-colormap windows. 
		/* Install all colormaps on the list;  don't bother with
		 * the ones already installed.  Put them at the head of the
		 * list. 
		 */
		XtPointer	subdata;
		SubWindowInfo	*sub;
		Window		*cmap_window_list;
		int		cmapcount,
				num_already = 0;
		Colormap	cmaparray[MAXCOLORMAPS];
		Colormap	tmp_cmap[MAXCOLORMAPS],
				*already_cmaps = NULL;

		cmap_window_list = (Window *)data;
		cmapcount = 0;
		for (i=0; i < num_cmap_windows; i++) {
			if (wmstep->window == cmap_window_list[i]) {
				cmaparray[cmapcount++] =
					wmstep->
					  window_attributes.colormap;
				continue;
			}
			if (XFindContext(display,cmap_window_list[i],
				wm_unique_context,
					(caddr_t *)&subdata) == 0) {
				sub = (SubWindowInfo *)subdata;
				cmaparray[cmapcount++] =
						 sub->colormap;
			}
		} /* end for loop */

		already_cmaps = XListInstalledColormaps(
				display, wmstep->window, &num_already);
		for (i=cmapcount-1; i>=0; i--) {
			for (j=0; j < num_already; j++) {
				if (cmaparray[i] == already_cmaps[j]) {
					tmp_cmap[i] = -1;
					continue;
				}
				tmp_cmap[i] = cmaparray[i];
			}
		}
		if (num_already > 0 && already_cmaps)
			XFree(already_cmaps);
		/* bump out the colormaps that we know will be replaced */
		if (cmapcount > MaxCmapsOfScreen(XtScreen((Widget)wm))) 
			cmapcount = MaxCmapsOfScreen(XtScreen((Widget)wm)) ;

		if (tmp_cmap[cmapcount-1] != -1) {
			XInstallColormap(display, cmaparray[cmapcount-1]);
			CurrentColormap = cmaparray[cmapcount-1];
		}
		for (i=cmapcount-2; i>=0; i--)
			if (tmp_cmap[i] != -1 && 
				tmp_cmap[i] != tmp_cmap[i+1]) {
				XInstallColormap(display, cmaparray[i]);
				CurrentColormap = cmaparray[i];
			}
	} /* FindContext */
		/* At this point, you have gathered all the colormaps
		 * on the toplevel window's sublist.  Some of these
		 * may have been other toplevel windows, but you don't
		 * care.
		 */

		/* FIrst, take out all the COMMON colormaps - don't
		 * install these.  Place into an extra array, tmp_cmap.
		 * Indices;
		 * 	i indexes into the new temporary array, tmp_cmap.
		 *	j references into the toplevel list of windows
		 *		that were in it's WM_COLORMAP_WINDOWS prop.
		 *	k references the CurrentColormap[] array (the list
		 *		of currently installed colormaps.
		 */
} /* end WMInstallColormaps() */

/*
 *************************************************************************
 * NextFocus() -
 * find the window that has the highest wmstep.focus value - this is
 * presumably the window that most previously had input focus.
 * Ignore = a widget that you want to skip over.
 * icons - tells me if I should consider icons for the next window.
 ****************************procedure*header*****************************
 */
extern WMStepWidget
NextFocus OLARGLIST((ignore, icons))
OLARG(WMStepWidget, ignore)
OLGRA(Boolean, icons)
{
int i;
WMStepWidget temp;
unsigned long maxfocus = 0;
int index = -1;

   for (i = 0; i < window_list->used; i++) {
      temp = (WMStepWidget)window_list->p[i];
	/* Skip it if the window is iconic, OR it is the window managers
	 * help widget and it happens to be unmapped now.
	 * The first part of the if: if (iconic && !icon_widget) - an app
	 * can have several windows in iconic state, but only one will
	 * house the icon widget (the leader)
	 */
#ifdef WITHDRAWNSTATE
	if (temp->wmstep.size == WMWITHDRAWN)
		continue;
#endif
	if( (IsIconic(temp) && !temp->wmstep.icon_widget ) ||
	    (temp->wmstep.icon_widget && !icons) ||
		((!wm_help_win_mapped) && temp == help_parent) ||
						temp == ignore)
		continue;
	if (maxfocus == 0 || (temp->wmstep.focus > maxfocus)) {
		index = i;
		maxfocus = temp->wmstep.focus;
	}
   }
/*
   if (index == -1) {
	maxfocus = 0;
	for (i = 0; i < window_list->used; i++) {
		temp = (WMStepWidget)window_list->p[i];
		if(((!wm_help_win_mapped) && temp == help_parent) ||
						temp == ignore)
			continue;
		if (maxfocus == 0 || temp->wmstep.focus > maxfocus) {
			index = i;
			maxfocus = temp->wmstep.focus;
		}
   	}
	if (index == -1)
		temp = NULL;
	else
		temp = (WMStepWidget)(window_list->p[index]);
   }
 */
   if (index == -1)
	temp = NULL;
   else
	temp = (WMStepWidget)(window_list->p[index]);
   return(temp);
} /* NextFocus */


/*
 *************************************************************************
 * SetColors.
 *  - Read OLWinColors property to set window specific colors.
 *
 ****************************procedure*header*****************************
 */
extern void
SetColors OLARGLIST((wm, display, window))
OLARG(WMStepWidget, wm)
OLARG(Display *, display)
OLGRA(Window, window)
{
	OLWinColors	color_struct;
	XColor		color;
	Colormap	cmap;
	Pixel		fore = (Pixel)0, back = (Pixel)0, border = (Pixel)0;
	int		change = 0;
#ifdef USEBUF
	char		buf[BUFSIZ];
#endif
	if (GetOLWinColors(display, window, &color_struct) != Success)
		return;

	cmap = DefaultColormapOfScreen(XtScreen(wm));

	if (color_struct.flags & _OL_WC_FOREGROUND)
	{
		color.red = (unsigned short) color_struct.fg_red;
		color.green = (unsigned short) color_struct.fg_green;
		color.blue = (unsigned short) color_struct.fg_blue;
		color.flags = DoRed | DoGreen | DoBlue;

		if (XAllocColor(display, cmap, &color) == 0)
		{
#if !defined(I18N)
			fprintf(stderr,"Window Manager Warning: Can't set client foreground color from _OL_WIN_COLORS\n");
#else
			OlVaDisplayWarningMsg(display, OleNcolor, OleTwinColors,
			   OleCOlClientOlwmMsgs, OleMcolor_winColors,
		OlGetMessage(display, NULL, 0, OleNcolor,
			OleTforeground, OleCOlClientOlwmMsgs,
			OleMcolor_foreground, NULL),  NULL);
#endif
		}
		else {
			fore = color.pixel;
			change = 2;
		}
	}
	if (color_struct.flags & _OL_WC_BACKGROUND)
	{
		color.red = (unsigned short) color_struct.bg_red;
		color.green = (unsigned short) color_struct.bg_green;
		color.blue = (unsigned short) color_struct.bg_blue;
		color.flags = DoRed | DoGreen | DoBlue;

		if (XAllocColor(display, cmap, &color) == 0)
		{
#if !defined(I18N)
			fprintf(stderr,"Window Manager Warning: Can't set client background color from _OL_WIN_COLORS\n");
#else
			OlVaDisplayWarningMsg(display, OleNcolor, OleTwinColors,
			   OleCOlClientOlwmMsgs, OleMcolor_winColors,
		OlGetMessage(display, NULL, 0, OleNcolor,
			OleTbackground, OleCOlClientOlwmMsgs,
			OleMcolor_background, NULL), NULL);
#endif
		}
		else {
			back = color.pixel;
			change = 3;
		}
	}
	if ( color_struct.flags & _OL_WC_BORDER)
	{
		/* Only get border for the 2-dimensional case */
		color.red = (unsigned short) color_struct.bd_red;
		color.green = (unsigned short) color_struct.bd_green;
		color.blue = (unsigned short) color_struct.bd_blue;
		color.flags = DoRed | DoGreen | DoBlue;

		if (XAllocColor(display, cmap, &color) == 0)
		{
#if !defined(I18N)
			fprintf(stderr, "Window Manager Warning: Can't set client border color from _OL_WIN_COLORS\n");
#else
			OlVaDisplayWarningMsg(display, OleNcolor, OleTwinColors,
			   OleCOlClientOlwmMsgs, OleMcolor_winColors,
				OlGetMessage(display, NULL, 0, OleNcolor,
				OleTborder, OleCOlClientOlwmMsgs,
				OleMcolor_border, NULL), NULL);
#endif
		}
		else {
			border = color.pixel;
			change--;
			if (!OlgIs3d())
#if !defined(I18N)
				fprintf(stderr,"Window Manager Warning: Border Color Affects 2-Dimensional Graphics Only...\n");
#else
			OlVaDisplayWarningMsg(display, OleNdimension,
			    OleTborder, OleCOlClientOlwmMsgs,
			    OleMdimension_border,
			    NULL);
#endif
		}
		/* The border can be save in wm->core.border_pixel.
		 * Unfortunately, it will be difficult to use if we
		 * want to use the CalcRects routine.  However, we
		 * can make an exception for 2-D stuff.
		 */
	}

         if (!wmrcs->parentRelative)
            {
            wm-> core.background_pixel = wmrcs->backgroundColor;
            XSetWindowBackground(XtDisplay(wm), XtWindow(wm),
					wmrcs->backgroundColor);
            }
	 if (wmrcs->foregroundColor != wmrcs->backgroundColor)
         	wm-> wmstep.foreground_pixel = wmrcs->foregroundColor;

	if (fore != 0)
		wm->wmstep.foreground_pixel = fore;
         if (back != 0 && !(wmrcs->parentRelative)) {
            wm-> core.background_pixel = back;
            XSetWindowBackground(XtDisplay(wm), XtWindow(wm),
					back);
	}
	if (border != 0)
		/* Set up the border color - but it will be used for
		 * the 2-D case only.
		 */
		wm->core.border_pixel = border;

	/* NOTE: I CAN'T resolve colors for this one - the input window header
	 * color is global and can't change because of this window.
	 * ....the programmer did it, let him or her undo it.
	ResolveColors();
	 */
		
	if (change != 0) {
		wm->wmstep.olwincolors = 1;
		if (change >= 0) /* something other than the border changed */
			CreateGC(wm, &(wm->wmstep), TRUE);
		if ( (change >= 0) || !(OlgIs3d())) /* either something
						     * other than the
						     * border changed, or the
						     * border only did, and
						     * it's 2-D.  Only a
						     * factor if we go
						     * for the diff. color bd.
						     * in 2-D.
						     */
			DisplayWM(wm, NULL);
	}
} /* SetColors */

/*
 *************************************************************************
 * AddDeletePendingList - this routine adds and deletes the wmstep widgets
 * from the global array of widgets that are "Pending" removal from the system.
 * A WMStepWidget is pending if:
 *	-The workspace manager isn't running;
 *	-Someone selected Quit from the window menu, AND
 *	 they asked for WM_SAVE_YOURSELF;
 *	-We therefore must wait for a WM_COMMAND property to be written on
 *	 the window that is the window_group of the window for which you
 *	 selected Quit from.
 *	-The delete Widget is the one that Quit was selected for - it is
 *	 normally the same as pending, unless we have an application with
 *	 multiple base windows, and one is the window_group leader of the
 *	 other.
 ****************************procedure*header*****************************
 */
extern void
AddDeletePendingList OLARGLIST((pending, delete,  add_it))
	OLARG(Widget,	pending)
	OLARG(Widget,	delete)
	OLGRA(Boolean,	add_it)
{
	if (add_it == True) {
		int i;
		if (num_pending_kids == num_pending_slots) {
			num_pending_slots += 4;
			pending_kids = (PendingInfo *)
					XtRealloc((XtPointer) pending_kids,
					    num_pending_slots *
						 sizeof(PendingInfo));
		}
		pending_kids[num_pending_kids].pending = pending;
		pending_kids[num_pending_kids++].delete = delete;
	} else {
		Cardinal i;

			/* Find the kid in the list, remove it, and
			 * coalesce the list.				*/

		for (i=0; i < num_pending_kids; ++i) {
			if (pending_kids[i].pending == pending) {
				--num_pending_kids;
				while(i < num_pending_kids) {
					pending_kids[i] = pending_kids[i+1];
					++i;
				}
			}
		}
	}
} /* END OF AddDeletePendingList() */

/*
 *************************************************************************
 * IsPending
 * Pass in Window ID, return index in pending list if there, else -1.
 ****************************procedure*header*****************************
 */
extern int
IsPending(window)
Window window;
{
int i;
WMStepWidget temp;

	for (i=0; i< num_pending_kids; i++) {
		temp = (WMStepWidget)pending_kids[i].pending;
		if (temp->wmstep.window == window)
			return(i);
	}
	return(-1);
}	


/*
 *************************************************************************
 * SetSignals.
 * - Set up signal handling - use TrapSignal() on lethal signals.
 ****************************procedure*header*****************************
 */
extern void
SetSignals(dpy, root, scr)
Display *dpy;
Window root;
Screen *scr;
{
	signal(SIGHUP, (void(*)())TrapSignal);
	signal(SIGTERM, (void(*)())TrapSignal);
#ifdef DEBUG
	signal(SIGINT, (void(*)())TrapSignal);
	signal(SIGQUIT, (void(*)())TrapSignal);
#else
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
/*
	signal(SIGILL, (void(*)())TrapSignal);
	signal(SIGTRAP, (void(*)())TrapSignal);
	signal(SIGIOT, (void(*)())TrapSignal);
	signal(SIGABRT, (void(*)())TrapSignal);
	signal(SIGEMT, (void(*)())TrapSignal);
	signal(SIGFPE, (void(*)())TrapSignal);
	signal(SIGBUS, (void(*)())TrapSignal);
	signal(SIGSEGV, (void(*)())TrapSignal);
	signal(SIGSYS, (void(*)())TrapSignal);
	signal(SIGPIPE, (void(*)())TrapSignal);
	signal(SIGALRM, (void(*)())TrapSignal);
 */
#endif

	TrapDpy = dpy;
	TrapRoot = root;
	TrapScr = scr;
}

/*
 *************************************************************************
 * TrapSignal.
 *  - Signal handler for the "kill" signal.
 ****************************procedure*header*****************************
 */

extern void
TrapSignal()
{
	int i;
	WMStepWidget	temp;
	Display 	*dsp;
	Window		root_win = RootWindowOfScreen(TrapScr);
	/* We got a "bad" signal - we just got killed.  Reset the cursor to
	 * the whatever it defaults to;
	 * then make input focus PointerRoot;
	 * Inform the world that olwm isn't running anymore;
	 */
	SetOlwmNotRunning(TrapDpy,TrapScr);
	for (i=0; i < num_wmstep_kids; i++) {
		temp = (WMStepWidget)	wmstep_kids[i];
		if (IsIconic(temp)) {
			/* we need the BASE window - that's the one that
			 * has it's decoration window reset to the new
			 * position.
			 */
			WMStepPart *wmstep = (WMStepPart *)&(temp->wmstep);

			if (wmstep->icon_widget) {
			   if (wmstep-> decorations & WMIconWindowMapped &&
			    wmstep->decorations & WMIconWindowReparented &&
			    (wmstep->xwmhints->icon_window != (Window)NULL)) {
				dsp = XtDisplay(temp);
				XUnmapWindow(dsp,
					wmstep->xwmhints->icon_window);
				XReparentWindow(dsp,
					wmstep->xwmhints->icon_window, 
					RootWindowOfScreen(XtScreen(temp)),
					temp->core.x, temp->core.y);
				XChangeSaveSet(dsp,	
					wmstep->xwmhints->icon_window,
					SetModeDelete);
			   }
			} /* if */
		} /* end if (Iconic() ) */
		XSetWindowBorderWidth(XtDisplay(temp),temp->wmstep.window,
				(unsigned int)temp->wmstep.prev.border_width);
		if (temp->wmstep.size == WMWITHDRAWN) {
			dsp = XtDisplay(temp);
			XUnmapWindow(dsp, temp->wmstep.window);
			XReparentWindow(dsp, temp->wmstep.window, 
				root_win, temp->core.x, temp->core.y);
			XChangeSaveSet(dsp, temp->wmstep.window, SetModeDelete);
		}
	} /* end for */
	XUndefineCursor(TrapDpy, TrapRoot);
	XSetInputFocus(TrapDpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XInstallColormap(TrapDpy, DefaultColormapOfScreen(TrapScr));
	XSync(TrapDpy, False);

	exit(0);
}

#define CONSTRAIN
#ifdef CONSTRAIN

extern void
ResetConstrainPoints(event)
XEvent *event;
{

prevx = event-> xbutton.x;
prevy = event-> xbutton.y;
prevrootx = event-> xbutton.x_root;
prevrooty = event-> xbutton.y_root;
}


/*
 *************************************************************************
 * OlButtonState().   See if an Open Look virtual button event exists
 * for the button/modifier combination passed in.
 *
 ****************************procedure*header*****************************
 */

static OlVirtualName
OlButtonState OLARGLIST((w, button, modifiers))
OLARG(Widget, w)
OLARG(unsigned int, button)
OLGRA(unsigned int, modifiers)
{
OlVirtualEventRec	ve;
XButtonEvent dummy_btn_event;
int i;
int j;

dummy_btn_event.type = ButtonPress;
dummy_btn_event.button = button;
dummy_btn_event.state = modifiers;

OlLookupInputEvent(w, (XEvent *)&dummy_btn_event, &ve, OL_DEFAULT_IE);

return (ve.virtual_name);

} /* end of OlButtonState */

/*
 *************************************************************************
 * OlQueryPointer.  A fancy way of saying XQueryPointer(), but if
 * the OL_CONSTRAIN button event occurs, resolve it by possibly
 * warping the pointer.  It's faster to warp the pointer by an amount
 * relative to where the pointer currently is rather than warping to an
 * absolute x, y position.
 ****************************procedure*header*****************************
 */

extern int
OlQueryPointer OLARGLIST((w, root, child, rootx, rooty, winx, winy))
OLARG(Widget, w)
OLARG(Window *, root)
OLARG(Window *, child)
OLARG(int *, rootx)
OLARG(int *, rooty)
OLARG(int *, winx)
OLGRA(int *, winy)
{
unsigned int mask;
Display * display = XtDisplay(w);
Window    window  = XtWindow(w);
OlVirtualName ol_event;
int deltax = 0;
int deltay = 0;

#define DONE          0
#define POLL          1

#define ConstrainX    2
#define ConstrainY    1
#define Unconstrained 0

static int constraint;

XQueryPointer(display, window, root, child, rootx, rooty, winx, winy, &mask);
ol_event = OlButtonState(w, (mask & 0xff00) >>8 /* btn */,
						 mask & 0xff /* mod */);

if (ol_event == OL_CONSTRAIN)
   {
   if (constraint == Unconstrained)
      {
      deltax = *rootx - prevrootx;
      deltay = *rooty - prevrooty;
      deltax = abs(deltax);
      deltay = abs(deltay);
      if (deltax || deltay)
         constraint = deltax < deltay ? ConstrainX : ConstrainY;
      }
   }
else
   if (ol_event != OL_SELECT)
      {
      constraint = Unconstrained;
      return (DONE);
      }
   else
      constraint = Unconstrained;;

switch (constraint)
   {
   case Unconstrained:
      break;
   case ConstrainX:
	deltax = *rootx - prevrootx;
      *winx = prevx;
      *rootx = prevrootx;
      break;
   case ConstrainY:
	deltay = *rooty - prevrooty;
      *winy = prevy;
      *rooty = prevrooty;
      break;
   }
prevrootx = *rootx;
prevrooty = *rooty;
prevx = *winx;
prevy = *winy;

if (constraint != Unconstrained && (deltax || deltay) )
   XWarpPointer(display, None, None, 0, 0, 0, 0,
			constraint == ConstrainX ? -deltax : 0,
			constraint == ConstrainY ? -deltay : 0);

return (POLL);

} /* end of OlQueryPointer */

#endif


/*
 *************************************************************************
 *
 *  GetMwmMessages.
 *	- Read _MWM_MESSAGES property.  Motif clients behold.
 *	Mwm seems to allow clients to specify the MWM_MESSAGES atom
 *	in WM_PROTOCOLS (in addition to WM_DELETE_WINDOW, etc.). It
 *	checks for it when it reads the MWM_MESSAGES property.
 *
 ****************************procedure*header*****************************
 */

#define MAX_MWM_MSGS	50 /* Limit number of messages */

void
GetMwmMessages OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
	WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
	Window w;
	int i, status;
	Atom actual_type_ret;
	int actual_format_ret;
	unsigned long nitems_ret, bytes_after_ret;
	long *prop_ret = NULL;
	Atom MWM_MSGS, MWM_OFFSET;
	Display *dsp = XtDisplay((Widget)wm);


	if (wmstep->mwm_msgs) {
		XtFree((char *)(wmstep->mwm_msgs));
		wmstep->mwm_msgs = NULL;
	}
	wmstep->num_mwm_msgs = 0;

 /* _MWM_MESSAGES atom defines */

#define _XA_MOTIF_WM_MESSAGES	"_MOTIF_WM_MESSAGES"
#define _XA_MWM_MESSAGES	_XA_MOTIF_WM_MESSAGES

/* atom that enables client frame offset messages */
#define _XA_MOTIF_WM_OFFSET	"_MOTIF_WM_OFFSET"

	MWM_MSGS = XInternAtom(dsp, _XA_MWM_MESSAGES, False);
	MWM_OFFSET = XInternAtom(dsp, _XA_MOTIF_WM_OFFSET, False);

	status = XGetWindowProperty(dsp, wmstep->window, 
			MWM_MSGS, (long)0,
			(long)MAX_MWM_MSGS, False, AnyPropertyType,
			&actual_type_ret, &actual_format_ret,
			&nitems_ret, &bytes_after_ret,
			(unsigned char **)&prop_ret);


	if ((status != Success) || (actual_type_ret == None) ||
					(actual_format_ret != 32))
		/* Property not found, or bad type. */
		wmstep->mwm_msgs = NULL;
	else
		if (!(wmstep->mwm_msgs = (long *)XtMalloc(nitems_ret *
							 sizeof (long)))) {
#if !defined(I18N)
		   fprintf(stderr,"Insufficient memory for window management data");
#else
		   OlVaDisplayWarningMsg(dsp, OleNspace, OleTmemory,
			OleCOlClientOlwmMsgs, OleMspace_memory, NULL);
#endif
	}
	else {
		/*
		 * Store protocols; check for a special motif 
		 * protocol, an atom MWM_OFFSET.
		 */

		wmstep->num_mwm_msgs = nitems_ret;
		for (i = 0; i < nitems_ret; i++) {
			if ((wmstep->mwm_msgs[i] = prop_ret[i]) == MWM_OFFSET)
				wmstep->protocols |= MwmOffset;
		} /* for */
	} /* else */

    if (prop_ret) {
	XFree ((char *)prop_ret);
    }

} /* GetMwmMessages */
#undef MAX_MWM_MSGS


/*
 *************************************************************************
 *  MotifModeStartup().
 *
 *  Show (unshow) the busy glass when starting the win mgr.
 *
 ****************************procedure*header*****************************
 */

void
MotifModeStartup OLARGLIST((flag))
OLGRA(Boolean,  flag)
{
	Display		*display = XtDisplay(Frame);
	Pixmap		pm;
	Pixmap		mskpm;
	XColor		colors[2];


	if (flag) {
		/* Put everything on hold- grab pointer with busy
		 * cursor.
		 */
		XGrabPointer(display, DefaultRootWindow(display), False, 
			0, GrabModeAsync, GrabModeAsync, None, 
			GetOlBusyCursor(XtScreen(Frame)),
			CurrentTime);
		XGrabKeyboard (display, DefaultRootWindow(display), False, 
			GrabModeAsync, GrabModeAsync, CurrentTime);
	} /* if flag */
	else {
		XUngrabPointer(display, CurrentTime);
		XUngrabKeyboard(display, CurrentTime);
	} /* else !flag */

} /* MotifModeStartup */

/*
 *************************************************************************
 * WMIconWinDimConverter.  Converter for some motif resources - convert
 * string in form [0-9][0-9]*[xX][0-9][0-9]* to width, height, place
 * in IconWinDim struct.
 ****************************procedure*header*****************************
 */

extern void
WMIconWinDimConverter OLARGLIST((args, num_args, fromVal, toVal))
OLARG(XrmValue *, args)
OLARG(Cardinal, num_args)
OLARG(XrmValue *, fromVal)
OLGRA(XrmValue *, toVal)
{
static IconWinDim iwdims;
char *p = (char *)fromVal->addr;
char *heightp;

	iwdims.width = iwdims.height = 0;

	if (p && *p) {
		heightp = strpbrk(p, "xX");
		if (heightp != NULL && heightp != p) {
			iwdims.width = atol(p);
			iwdims.height = atol(++heightp);          
		}
	}

	toVal-> addr = (caddr_t)&iwdims;
	toVal-> size = sizeof(IconWinDim);

} /* WMIconWinDim */

/*
 *************************************************************************
 * WMIconDecorConverter.  Converter for motif resource - convert
 * string to correct icon decoration #define.  The legal strings are
 * "image", "label", and "activelabel", separated by
 ****************************procedure*header*****************************
 */
#define I_IMAGE	"image"
#define I_LABEL	"label"
#define I_ACTIVELABEL	"activelabel"

extern void
WMIconDecorConverter OLARGLIST((args, num_args, fromVal, toVal))
OLARG(XrmValue *, args)
OLARG(Cardinal, num_args)
OLARG(XrmValue *, fromVal)
OLGRA(XrmValue *, toVal)
{
char		*p = (char *)fromVal->addr;
static long	icondecor;
char		*decor;

/* The legal values that can be declared as resources are "label", "image",
 * and "activelabel".
 */


	icondecor = (long)0;
	if (p && *p) {
		/* Get each icon part separately - parse on the line with
		 * NextToken(), in Parse.c
		 */
		for (decor = NextToken(&p); decor != NULL;
						decor = NextToken(&p)) {
			StringToLCString(decor);
			if (!strcmp(decor, I_IMAGE))
				icondecor |= ICON_IMAGE;
			else
			  if (!strcmp(decor, I_LABEL))
				icondecor |= ICON_LABEL;
			  else
				if (!strcmp(decor, I_ACTIVELABEL))
					icondecor |= ICON_ACTIVELABEL;
				else {
				/* mlp - bad icon specification */
				OlVaDisplayWarningMsg(XtDisplay(Frame),
					OleNiconDecor, OleTbadResSpec,
					OleCOlClientOlwmMsgs,
					OleMiconDecor_badResSpec, NULL);
				icondecor = (ICON_ACTIVELABEL | ICON_LABEL |
								ICON_IMAGE);
				} /* else bad icon spec */
		} /* for */
	} /* if p && *p */
	/* Can't have only an active label - defaults to all in that
	 * case.
	 */
	if (icondecor == ICON_ACTIVELABEL)
		icondecor = ICON_IMAGE | ICON_LABEL | ICON_ACTIVELABEL;
	toVal-> addr = (caddr_t)&icondecor;
	toVal-> size = sizeof(long);

} /* WMIconDecorConverter */

#define F_ALL "all"
#define F_BEHAV	"behavior"
#define F_KILL	"kill"
#define F_MOVE	"move"
#define F_NONE	"none"
#define F_PLACE	"place"
#define F_QUIT	"quit"
#define F_RESIZE	"resize"
#define F_RESTART	"restart"
/*
 *************************************************************************
 * WMShowFeedbackConverter.  Converter for motif resource showFeedback.
 * - convert string to correct feedback options.   Legal strings:
 * all behavior kill move none placement quit resize restart
 ****************************procedure*header*****************************
 */

extern void
WMShowFeedbackConverter OLARGLIST((args, num_args, fromVal, toVal))
OLARG(XrmValue *, args)
OLARG(Cardinal, num_args)
OLARG(XrmValue *, fromVal)
OLGRA(XrmValue *, toVal)
{
char		*p = (char *)fromVal->addr;
static long	feedbackoptions;
char		*feedback;


	feedbackoptions = (long)0;
	if (p && *p) {
		/* Get each option - parse on the line with
		 * NextToken(), in Parse.c
		 */
		for (feedback = NextToken(&p); feedback != NULL;
					feedback = NextToken(&p)) {
			StringToLCString(feedback);
			if (!strcmp(feedback, F_NONE)) {
				feedbackoptions = FEED_NONE;
				break;
			}
			else
			  if (!strcmp(feedback, F_ALL)) {
				feedbackoptions |= FEED_ALL;
				break;
			  }
			  if (!strcmp(feedback, F_BEHAV))
				feedbackoptions |=
					FEED_BEHAVIOR_SWITCH;
			  else
				if (!strcmp(feedback, F_KILL))
				  feedbackoptions |= FEED_KILL;
			  else
				if (!strcmp(feedback, F_MOVE))
				  feedbackoptions |= FEED_KILL;
			  else
				if (!strcmp(feedback, F_PLACE))
				  feedbackoptions |= FEED_INITPLACEMENT;
			  else
				if (!strcmp(feedback, F_QUIT))
				  feedbackoptions |= FEED_QUIT;
			  else
				if (!strcmp(feedback, F_RESIZE))
				  feedbackoptions |= FEED_RESIZE;
			  else
				if (!strcmp(feedback, F_RESTART))
				  feedbackoptions |= FEED_RESTART;
				else {
				/* mlp - bad feedback spec */
				OlVaDisplayWarningMsg(XtDisplay(Frame),
					OleNshowFeedback, OleTbadResSpec,
					OleCOlClientOlwmMsgs,
					OleMshowFeedback_badResSpec, NULL);
				/* give current default */
				feedbackoptions = FEED_MOVE |
					FEED_RESIZE;
				} /* else bad icon spec */
		} /* for */
	} /* if p && *p */

	toVal-> addr = (caddr_t)&feedbackoptions;
	toVal-> size = sizeof(long);

} /* WMShowFeedbackConverter */

/*
 *************************************************************************
 * WMClientDecorConverter.  Converter for motif resource clientDecoration,
 * a client specific resource.
 * - convert string to decorations.
 ****************************procedure*header*****************************
 */

#define ALL "all"
#define BORDER	"border"
#define MAXIMIZE	"maximize"
#define MINIMIZE	"minimize"
#define NONE	"none"
#define	RESIZEH	"resizeh"
#define MENU	"menu"
#define TITLE	"title"

extern void
WMClientDecorConverter OLARGLIST((args, num_args, fromVal, toVal))
OLARG(XrmValue *, args)
OLARG(Cardinal, num_args)
OLARG(XrmValue *, fromVal)
OLGRA(XrmValue *, toVal)
{
char		*p = (char *)fromVal->addr;
static long	decorations;
char		*decor;
Window		transient_parent;
Window		window;
Boolean		subtract;


	decorations = (long)0;
	if (p && *p) {
		decor = NextToken(&p);
		if (*decor == '-')
			/* start with all decorations, subtract */
			decorations = WMHeader | WMHasFullMenu |
					WMResizable| WMMinimize | WMMaximize |
					WMBorder;
			/* else start with no decorations - set to 0 above */
	

		/* Get each option - parse on the line with
		 * NextToken(), in Parse.c
		 */
		for (; decor != NULL;
					decor = NextToken(&p)) {
			/* Watch out for + and - signs */
			if (*decor == '-')
				subtract = True;
			else
				subtract = False;
			if (*decor == '+' || *decor == '-')
				decor = NextToken(&p);
			StringToLCString(decor);
			if (!strcmp(decor, ALL) ){
				if (subtract)
					decorations = WMNoDecorations;
				else
				  decorations = WMHeader |
					WMResizable| WMMinimize | WMMaximize |
					WMMenuButton | WMBorder |
					WMHasFullMenu | WMHasLimitedMenu;
				break;
			}
			else
			  if (!strcmp(decor, BORDER) ) {
				if (subtract)
					decorations &= ~WMBorder;
				else
					decorations |= WMBorder;
			}
			else
			  if (!strcmp(decor, MINIMIZE) ) {
				if (subtract)
					decorations &= ~WMMinimize;
				else
					decorations |= WMMinimize;
			}
			else
			  if (!strcmp(decor, MAXIMIZE) ) {
				if (subtract)
					decorations &= ~WMMaximize;
				else
					decorations |= WMMaximize;
			}
			else
			  if (!strcmp(decor, RESIZEH ) ) {
				if (subtract)
					decorations &= ~WMResizable;
				else
					decorations |=  WMResizable;
			}
			else
			  if (!strcmp(decor, MENU) ) {
				if (subtract)
					decorations &= ~WMMenuButton;
				else
					decorations |=  WMMenuButton;
			}
				else {
					/* bad feedback spec */
					OlVaDisplayWarningMsg(XtDisplay(Frame),
						OleNclientDecorations,
						OleTbadResSpec,
						OleCOlClientOlwmMsgs,
					   OleMclientDecorations_badResSpec,
						NULL);
					decorations = ULONG_MAX;
					break;
				} /* else bad spec */
		} /* for */
	} /* if p && *p */

	if (decorations != ULONG_MAX)
		decorations |= (WMHasFullMenu|WMHasLimitedMenu);
	toVal-> addr = (caddr_t)&decorations;
	toVal-> size = sizeof(long);

} /* WMClientDecorConverter */

#undef ALL
#undef BORDER
#undef MAXIMIZE
#undef MINIMIZE
#undef NONE
#undef	RESIZEH
#undef MENU
#undef TITLE

/*
 *************************************************************************
 * WMClientFunctionsConverter.  Converter for motif resource clientFunctions,
 * a client specific resource.
 * - convert string to menu functions.
 ****************************procedure*header*****************************
 */

#define ALL "all"
#define MOVE	"move"
#define RESIZE	"resize"
#define MINIMIZE	"minimize"
#define MAXIMIZE	"maximize"
#define NONE	"none"
#define CLOSE	"close"

extern void
WMClientFunctionsConverter OLARGLIST((args, num_args, fromVal, toVal))
OLARG(XrmValue *, args)
OLARG(Cardinal, num_args)
OLARG(XrmValue *, fromVal)
OLGRA(XrmValue *, toVal)
{
char		*p = (char *)fromVal->addr;
static long	functions;
char		*func;
Window		transient_parent;
Window		window;
Boolean		subtract;


	functions = (long)0;
	if (p && *p) {
		func = NextToken(&p);
			if (*func == '-') {
				  functions = WMFuncMove | WMFuncResize |
					WMFuncMinimize | WMFuncMaximize |
					WMFuncClose;
			}

		/* Get each option - parse on the line with
		 * NextToken(), in Parse.c
		 */
		for (; func != NULL; func = NextToken(&p)) {
			/* Watch out for + and - signs */
			if (*func == '-')
				subtract = True;
			else
				subtract = False;
			StringToLCString(func);
			if (!strcmp(func, ALL)){
				if (subtract)
					functions = 0;
				else
				  functions = WMFuncMove | WMFuncResize |
					WMFuncMinimize | WMFuncMaximize |
					WMFuncClose;
				break;
			}
			else
			  if (!strcmp(func, NONE) ) {
				if (subtract)
					/* all */
				  	functions = WMFuncMove | WMFuncResize |
					 WMFuncMinimize | WMFuncMaximize |
					 WMFuncClose;
				else
					functions = 0;
			}
			else
			  if (!strcmp(func, MINIMIZE) ) {
				if (subtract)
					functions &= ~WMFuncMinimize;
				else
					functions |= WMFuncMinimize;
			}
			else
			  if (!strcmp(func, MAXIMIZE) ) {
				if (subtract)
					functions &= ~WMFuncMaximize;
				else
					functions |= WMFuncMaximize;
			}
			else
			  if (!strcmp(func, RESIZE ) ) {
				if (subtract)
					functions &= ~WMFuncResize;
				else
					functions |=  WMFuncResize;
			}
/* Don't have WMFuncMenu, but should...
			else
			  if (!strcmp(func, MENU) ) {
				if (subtract)
					decorations &= ~WMHasFullMenu;
				else
					decorations |=  WMHasFullMenu;
			}
 */
			else
			  if (!strcmp(func, CLOSE) ) {
				if (subtract)
					functions &= ~WMFuncClose;
				else
					functions |=  WMFuncClose;
			}
				else {
					/* bad feedback spec */
					OlVaDisplayWarningMsg(XtDisplay(Frame),
						OleNclientFunctions,
						OleTbadResSpec,
						OleCOlClientOlwmMsgs,
					   OleMclientFunctions_badResSpec,
						NULL);
					functions = ULONG_MAX;
					break;
				} /* else bad spec */
		} /* for */
	} /* if p && *p */

	toVal-> addr = (caddr_t)&functions;
	toVal-> size = sizeof(long);

} /* WMClientFunctionsConverter */

/*
 *************************************************************************
 * WMTransientDecorConverter.  Converter for motif resource transientDecoration,
 * a global resource.
 * - convert string to decorations.
 ****************************procedure*header*****************************
 */

#define ALL "all"
#define BORDER	"border"
#define MAXIMIZE	"maximize"
#define MINIMIZE	"minimize"
#define NONE	"none"
#define	RESIZEH	"resizeh"
#define MENU	"menu"
#define TITLE	"title"

extern void
WMTransientDecorConverter OLARGLIST((args, num_args, fromVal, toVal))
OLARG(XrmValue *, args)
OLARG(Cardinal, num_args)
OLARG(XrmValue *, fromVal)
OLGRA(XrmValue *, toVal)
{
char		*p = (char *)fromVal->addr;
static long	decorations;
char		*decor;
Window		transient_parent;
Window		window;
Boolean		subtract;


	decorations = (long)0;
	if (p && *p) {
		decor = NextToken(&p);
		if (*decor == '-')
			/* start with default decorations, subtract */
			decorations = WMHeader | WMHasFullMenu |
					WMResizable | WMBorder;
			/* else start with no decorations - set to 0 above */
	

		/* Get each option - parse on the line with
		 * NextToken(), in Parse.c
		 */
		for (; decor != NULL;
					decor = NextToken(&p)) {
			/* Watch out for + and - signs */
			if (*decor == '-')
				subtract = True;
			else
				subtract = False;
			if (*decor == '+' || *decor == '-')
				decor = NextToken(&p);
			StringToLCString(decor);
			if (!strcmp(decor, ALL) ){
			/* What does "all" mean to transientDecoration?
			 * Does it mean all (default) or all (all full
			 * decorations) - assume only transient decorations.
			 */
				if (subtract)
					decorations = WMNoDecorations;
				else
				  decorations = WMHeader | WMResizable|
					WMMenuButton | WMBorder |
					WMHasFullMenu | WMHasLimitedMenu;
				break;
			}
			else
			  if (!strcmp(decor, BORDER) ) {
				if (subtract)
					decorations &= ~WMBorder;
				else
					decorations |= WMBorder;
			}
			else
			  if (!strcmp(decor, MINIMIZE) ) {
				/* No minimize */
				continue;
			}
			else
			  if (!strcmp(decor, MAXIMIZE) ) {
				/* No maximize */
				continue;
			}
			else
			  if (!strcmp(decor, RESIZEH ) ) {
				if (subtract)
					decorations &= ~WMResizable;
				else
					decorations |=  WMResizable;
			}
			else
			  if (!strcmp(decor, MENU) ) {
				if (subtract)
					decorations &= ~WMMenuButton;
				else
					decorations |=  WMMenuButton;
			}
				else {
					/* bad feedback spec */
					OlVaDisplayWarningMsg(XtDisplay(Frame),
						OleNclientDecorations,
						OleTbadResSpec,
						OleCOlClientOlwmMsgs,
					   OleMclientDecorations_badResSpec,
						NULL);
					decorations = ULONG_MAX;
					break;
				} /* else bad spec */
		} /* for */
	} /* if p && *p */

	if (decorations != ULONG_MAX)
		decorations |= (WMHasFullMenu|WMHasLimitedMenu);
	toVal-> addr = (caddr_t)&decorations;
	toVal-> size = sizeof(long);

} /* WMTransientDecorConverter */

#undef ALL
#undef BORDER
#undef MAXIMIZE
#undef MINIMIZE
#undef NONE
#undef	RESIZEH
#undef MENU
#undef TITLE

/*
 *************************************************************************
 * WMTransientFunctionsConverter.  Converter for motif resource
 * transientFunctions, a global resource.
 * - convert string to menu functions.
 ****************************procedure*header*****************************
 */

#define ALL "all"
#define MOVE	"move"
#define RESIZE	"resize"
#define MINIMIZE	"minimize"
#define MAXIMIZE	"maximize"
#define NONE	"none"
#define CLOSE	"close"

extern void
WMTransientFunctionsConverter OLARGLIST((args, num_args, fromVal, toVal))
OLARG(XrmValue *, args)
OLARG(Cardinal, num_args)
OLARG(XrmValue *, fromVal)
OLGRA(XrmValue *, toVal)
{
char		*p = (char *)fromVal->addr;
static long	functions;
char		*func;
Window		transient_parent;
Window		window;
Boolean		subtract;


	functions = (long)0;
	if (p && *p) {
		func = NextToken(&p);
			if (*func == '-') {
				  functions = WMFuncMove | WMFuncResize |
					WMFuncClose;
			}

		/* Get each option - parse on the line with
		 * NextToken(), in Parse.c
		 */
		for (; func != NULL; func = NextToken(&p)) {
			/* Watch out for + and - signs */
			if (*func == '-')
				subtract = True;
			else
				subtract = False;
			StringToLCString(func);
			if (!strcmp(func, ALL)){
				/* Same here as for transientDecoration -	
				 * what does it mean exactly - all functions,
				 * or all functions for transient windows,
				 * excluding minimize and maximize.
				 */
				if (subtract)
					functions = 0;
				else
				  functions = WMFuncMove | WMFuncResize |
					WMFuncClose;
				break;
			}
			else
			  if (!strcmp(func, NONE) ) {
				if (subtract)
					/* all */
				  	functions = WMFuncMove | WMFuncResize |
					 WMFuncClose;
				else
					functions = 0;
			}
			else
			  if (!strcmp(func, MINIMIZE) ) {
				/* No minimize */
				continue;
			}
			else
			  if (!strcmp(func, MAXIMIZE) ) {
				/* No maximize */
				continue;
			}
			else
			  if (!strcmp(func, RESIZE ) ) {
				if (subtract)
					functions &= ~WMFuncResize;
				else
					functions |=  WMFuncResize;
			}
/* Don't have WMFuncMenu, but should...
			else
			  if (!strcmp(func, MENU) ) {
				if (subtract)
					decorations &= ~WMHasFullMenu;
				else
					decorations |=  WMHasFullMenu;
			}
 */
			else
			  if (!strcmp(func, CLOSE) ) {
				if (subtract)
					functions &= ~WMFuncClose;
				else
					functions |=  WMFuncClose;
			}
				else {
					/* bad feedback spec */
					OlVaDisplayWarningMsg(XtDisplay(Frame),
						OleNclientFunctions,
						OleTbadResSpec,
						OleCOlClientOlwmMsgs,
					   OleMclientFunctions_badResSpec,
						NULL);
					functions = ULONG_MAX;
					break;
				} /* else bad spec */
		} /* for */
	} /* if p && *p */

	toVal-> addr = (caddr_t)&functions;
	toVal-> size = sizeof(long);

} /* WMTransientFunctionsConverter */
