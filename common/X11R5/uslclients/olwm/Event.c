/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olwm:Event.c	1.90"
#endif

/*
 *************************************************************************
 *
 * Description:
 *	This file contains event handlers
 *
 ******************************file*header********************************
 */
#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>

#include "Xol/OlClients.h"
#include <Xol/OpenLookP.h>
#include <Xol/Olg.h>
#include <Xol/WSMcomm.h>
#include <Xol/VendorI.h>
#include <wm.h>
#include <WMStepP.h>
#include <Extern.h>

/*
 *************************************************************************
 *
 * Forward Procedure declarations
 *
 **************************forward*declarations***************************
 */
extern void	AddRemoveEventHandlers OL_ARGS((WMStepWidget, Boolean));
extern void	CheckPostedMenu OL_ARGS((WMStepWidget));
extern void	IconEnterLeave OL_ARGS((Widget, XtPointer, XEvent *,
						Boolean *));
extern void	ClientEnterLeave OL_ARGS((Widget, XtPointer, XEvent *,
						Boolean *));
extern void	ClientSubstructureRedirect OL_ARGS((Widget, XtPointer,
						XEvent *, Boolean *));
extern void	ClientNonMaskable OL_ARGS((Widget, XtPointer, XEvent *,
						Boolean *));
extern void	ClientStructureNotify OL_ARGS((Widget, XtPointer, XEvent *,
						Boolean *));
static void	ClientPropertyChange OL_ARGS((Widget, XtPointer, XEvent *,
						Boolean *));
static void	ClientColormapChange OL_ARGS((Widget, XtPointer, XEvent *,
						Boolean *));
extern void	ClientFocusChange OL_ARGS((Widget, XtPointer, XEvent *,
						Boolean *));

static void	ClientMotionNotify OL_ARGS((Widget, XtPointer, XEvent *,
						Boolean *));

extern WMPiece	EventIn OL_ARGS((WMStepWidget, Position, Position));
static char *	FindHelpFile OL_ARGS((Widget, char *));  
extern unsigned long	FocusCount OL_NO_ARGS();
static char *	MakePath OL_ARGS((char *, char *));
extern void	IconButtonPress OL_ARGS((Widget, XtPointer, XEvent *,
						Boolean *));
extern void	IconExpose OL_ARGS((Widget, XtPointer, XEvent *,
						Boolean *));

static void	MotionTimerEvent OL_ARGS((XtPointer, XtIntervalId *));
extern void	RemoveWMColormaps OL_ARGS((WMStepWidget));

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

static unsigned long wm_focus_count = 0;

/* Addendum for motif decoration mode only */
static Cursor current_cursor = (Cursor)NULL;
static XtIntervalId	motion_timerid;

typedef struct _motion_info {
	int prev_x;
	int prev_y;
}  MOTION_INFO;

static MOTION_INFO	motion_info;

/*
 *************************************************************************
 *
 * Procedures
 *
 ***************************private*procedures****************************
 */

/*
 * AddEventHandlers - called from Initialize() -- adds event handlers
 * to the wmStepWidget.
 */
extern void
AddRemoveEventHandlers OLARGLIST((wm, add))
	OLARG(WMStepWidget,	wm)
	OLGRA(Boolean,		add)
{
	WMStepPart *    wmstep  = &wm-> wmstep;
	void	(*proc)();

	proc = (void (*)()) (add == True ?
			XtAddEventHandler : XtRemoveEventHandler);

	/* ClientEnterLeave() when mouse pointer enters or leaves
	 * window frame.
	 * Addendum - in my opinion, this should not get called unless
	 * you are either in a situation where wmrcs->pointerFocus is
	 * TRUE, or you are in motif mode and you are changing cursors
	 * for resize corners to show.  Otherwise, the mask isn't needed.
	 */

	if (wmrcs->pointerFocus || currentGUI == OL_MOTIF_GUI &&
					motwmrcs->resizeCursors) {

		(*proc)(wm, EnterWindowMask | LeaveWindowMask, False,
					ClientEnterLeave, NULL);
	}

	/* ClientSubstructureRedirect() for client "redirectable" requests, like
	 * attempts to reconfigure client window.
	 */
	(*proc)(wm, SubstructureRedirectMask, False,
				ClientSubstructureRedirect, NULL);

	/* ClientNonMaskable()- catch ClientMessage events */
	(*proc)(wm, NoEventMask, True, ClientNonMaskable, (XtPointer)wm);
	(*proc)(wmstep->child, NoEventMask, True, ClientNonMaskable,
							 (XtPointer)wm);

	/* ClientStructureNotify() - catch UnmapWindow, DestroyWindow...*/
	(*proc)(wmstep-> child, StructureNotifyMask, False,
				ClientStructureNotify, (XtPointer)wm);

	/* ClientPropertyChange() - property change on client window */
	(*proc)(wmstep-> child, PropertyChangeMask, False,
				ClientPropertyChange, (XtPointer)wm);

	/* ClientFocusChange() - FocusIn or FocusOut on client */
	(*proc)(wmstep-> child, FocusChangeMask, False,
				ClientFocusChange, (XtPointer)wm);

	/* ClientColormapChange() - client wants new colormap */
	(*proc)(wmstep-> child, ColormapChangeMask, False,
				ClientColormapChange, (XtPointer)wm);

} /* end of AddRemoveEventHandlers */


/*
 * ClientEnterLeave
 * - Event handler on client WMStepWidget - EnterNotify, LeaveNotify events.
 * The main concern with this code is if focus is Real-estate based; if so,
 * then the visual feedback must be adjusted for clients that are getting/losing
 * focus.
 * Addendum: for motif-based decoration set, we are concerned with
 * resizable windows - we want to change the pointer on these to reflect
 * the position of the pointer inside a resize corner or side.
 * - if resizable, then turn on motionnotify event handler - the handler
 * can check the pointer position.
 */

void
ClientEnterLeave OLARGLIST((w, client_data, event, cont_to_disp))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLARG(XEvent *, event)
OLGRA(Boolean *, cont_to_disp)
{
WMStepWidget wm = (WMStepWidget)w;
WMStepPart * wmstep = &wm-> wmstep;
XFocusChangeEvent focus_event;

switch (event-> type)
   {
   case EnterNotify:
#ifdef DEBUG
	fprintf(stderr,"Got EnterNotify, subwin=%ld\n",
					 event->xcrossing.subwindow);
#endif
	if (wm->wmstep.size == WMWITHDRAWN)
		break;
	if (currentGUI == OL_MOTIF_GUI && Resizable(wm) &&
					motwmrcs->resizeCursors) {
		if (event->xcrossing.mode == NotifyNormal &&
		    event->xcrossing.subwindow == (Window) 0) {
			/* What are we doing here?  Take advantage of the
			 * fact that we don't care about "phony" EnterNotify
			 * events, such as NotifyGrab and NotifyUngrab, only
			 * NotifyNormal;  and the wmstep window has no
			 * personal subwindows; it's only subwindow
			 * is the one reparented to it (the client window).
			 * so if the subwindow != 0, then the final ptr
			 * destination is not in the decoration area, but
			 * inside the client - forget about it, don't set
			 * up any motion hints for it.
			 */
			motion_info.prev_x = motion_info.prev_y = -16161;
			motion_timerid = OlAddTimeOut(w, (unsigned long)0,
				MotionTimerEvent, (XtPointer)wm);
		} /* inner if */
	}
      if (wmrcs->pointerFocus && !NumMenushellsPosted)
         {
         switch(event-> xcrossing.detail)
            {
            case NotifyAncestor:
#ifdef DEBUG
fprintf(stderr,"\tEnterNotify had detail = NotifyAncestor\n");
#endif
               SetFocus((Widget)wm, GainFocus, 1);
               break;
            case NotifyVirtual:
#ifdef DEBUG
fprintf(stderr,"\tEnterNotify had detail = NotifyVirtual\n");
#endif
               SetFocus((Widget)wm, GainFocus, 1);
               break;
            case NotifyInferior:
#ifdef DEBUG
fprintf(stderr,"\tEnterNotify had detail = NotifyInferior\n");
#endif
               break;
            case NotifyNonlinear:
#ifdef DEBUG
fprintf(stderr,"\tEnterNotify had detail = NotifyNonlinear\n");
#endif
               SetFocus((Widget)wm, GainFocus, 1);
               break;
            case NotifyNonlinearVirtual:
#ifdef DEBUG
fprintf(stderr,"\tEnterNotify had detail = NotifyNonlinearVirtual\n");
#endif
               SetFocus((Widget)wm, GainFocus, 1);
               break;
            }
         }
      break;
   case LeaveNotify:
#ifdef DEBUG
	fprintf(stderr,"Got LeaveNotify, subwin=%ld\n",
					 event->xcrossing.subwindow);
#endif
	if (currentGUI == OL_MOTIF_GUI && Resizable(wm) &&
		motwmrcs->resizeCursors) {
		/* turn on motion events */
#ifdef DEBUG
	fprintf(stderr,"Turning off Motion events, timers for this window\n");
#endif
		XtRemoveEventHandler((Widget)wm, PointerMotionMask,
			 False, ClientMotionNotify, NULL);
#ifdef DEBUG
fprintf(stderr,"Leavenotify reveived, call XtRemoveTimeOut, id=%ld\n",motion_timerid);
#endif
	if (motion_timerid) {
		XtRemoveTimeOut(motion_timerid);
		motion_timerid = (XtIntervalId)0;
	}

		XDefineCursor(XtDisplay(w), XtWindow(wm), motifWindowCursor);
	}
      if (wmrcs->pointerFocus && !NumMenushellsPosted)
         {
         switch(event-> xcrossing.detail)
            {
            case NotifyAncestor:
#ifdef DEBUG
fprintf(stderr,"\tLeaveNotify had detail = NotifyAncestor\n");
#endif

	       if(wm->wmstep.is_current)	{
		  wm->wmstep.is_current = 0;
		  if (currentGUI == OL_OPENLOOK_GUI)
			EraseBN(wm);
		  DisplayWM(wm,NULL);
	       }
               SetFocus(Frame, GainFocus, 0);
               break;
            case NotifyVirtual:
#ifdef DEBUG
fprintf(stderr,"\tLeaveNotify had detail = NotifyVirtual\n");
#endif

	       if(wm->wmstep.is_current)	{
		  wm->wmstep.is_current = 0;
		  if (currentGUI == OL_OPENLOOK_GUI)
			EraseBN(wm);
		  DisplayWM(wm,NULL);
	       }
               SetFocus(Frame, GainFocus, 0);
               break;
            case NotifyInferior:
#ifdef DEBUG
fprintf(stderr,"\tLeaveNotify had detail = NotifyInferior\n");
#endif
               break;
            case NotifyNonlinear:
#ifdef DEBUG
fprintf(stderr,"\tLeaveNotify had detail = NotifyNonlinear\n");
#endif
               break;
            case NotifyNonlinearVirtual:
#ifdef DEBUG
fprintf(stderr,"\tLeaveNotify had detail = NotifyNonlinearVirtual\n");
#endif
               break;
            }
         }
      break;
   default:
      break;
   }
} /* end of ClientEnterLeave */

/*
 * ClientMotionNotify - turned on/off by ClientEnterLeave.  Check the
 * pointers position - if in one of the resize area, then change the
 * pointer to reflect it.
 */
static void
ClientMotionNotify OLARGLIST((w, client_data, event, cont_to_disp))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLARG(XEvent *, event)
OLGRA(Boolean *, cont_to_disp)
{
WMPiece piece;
WMStepWidget wm = (WMStepWidget)w;
Cursor use_cursor;
#ifdef DEBUG
	fprintf(stderr,"Welcome to ClientMotionNotify, x = %d y=%d\n",
		event->xmotion.x, event->xmotion.y);
#endif
	XtRemoveEventHandler((Widget)wm, PointerMotionMask ,
				False, ClientMotionNotify, NULL);
	motion_timerid = OlAddTimeOut(w, (unsigned long)0,
				MotionTimerEvent, (XtPointer)wm);
} /* End of ClientMotionNotify */

static void
MotionTimerEvent OLARGLIST((client_data, id))
	OLARG(XtPointer, client_data)
	OLGRA(XtIntervalId *, id)
{
        WMStepWidget wm = (WMStepWidget)client_data;
	WMPiece	piece ;
	int	rootx,
		rooty,
		winx,
		winy;
	unsigned int mask;
	Window	child, root;
	Cursor use_cursor;

	/* Query pointer, tell me where it is... 
	 * Now, check this out:  if the call to XQueryPointer fails,
	 * then it is likely and probable that the window supplied is
	 * not valid - in other words, but a timing quirk, the decoration
	 * set is still around, but the window inside it is already
	 * unmapped.  "Timing quirk", in this case, is more than a figure
	 * of speech - remember how this function gets called:  it is
	 * called by the XtAppAddTimeout() function, meaning it is an event
	 * that gets dispatched from the olwm main loop.  I would like to
	 * choke it off at the main loop, but that would require checking
	 * every event that comes in.  This is better.
	 */
	if ( wm->wmstep.size == WMWITHDRAWN ||
			! (XQueryPointer(XtDisplay(wm), XtWindow(wm),
              			&root, &child, &rootx, &rooty,
				&winx, &winy, &mask) )) {
		if (motion_timerid) {
			XtRemoveTimeOut(motion_timerid);
			motion_timerid = 0;
		}
		return;
	}
	if (winx == motion_info.prev_x && winy == motion_info.prev_y) {
		/* skip the rest - set up a motion event handler because
		 * we are still in the window (haven't gotten a leave yet).
		 */
		XtAddEventHandler((Widget)wm, PointerMotionMask ,
					False, ClientMotionNotify, NULL);
		return;
	}
	motion_info.prev_x = winx;
	motion_info.prev_y = winy;
	piece = EventIn(wm, winx, winy);
	switch(piece) {
		case WM_T:
			use_cursor = t_cursor;
			break;
		case WM_B:
			use_cursor = b_cursor;
			break;
		case WM_L:
			use_cursor = l_cursor;
			break;
		case WM_R:
			use_cursor = r_cursor;
			break;
		case WM_NE:
			use_cursor = ur_cursor;
			break;
		case WM_SE:
			use_cursor = lr_cursor;
			break;
		case WM_NW:
			use_cursor = ul_cursor;
			break;
		case WM_SW:
			use_cursor = ll_cursor;
			break;
		default:
			use_cursor =  motifWindowCursor;
			break;
	} /* switch */
	if (!use_cursor)
		SetupWindowOpCursors(XtDisplay(wm));
	XDefineCursor(XtDisplay((Widget)wm), XtWindow((Widget)wm), use_cursor);

	/* I think the first argument is in milliseconds - this means
	 * we run the timer 200 times a second - it's actually less.
	 */
	motion_timerid = OlAddTimeOut((Widget)wm, (unsigned long)0,
				MotionTimerEvent, (XtPointer)wm);
/*XtAddTimeOut(5, MotionTimerEvent, (XtPointer)wm);*/
}


/*
 * ClientSubstructureRedirect
 * - Event handler added to WMStepWidget (e.g., parent of the client
 * widget.)  Sufficient for redirectable client window
 * request, such as ConfigureRequests).
 *  - Ignore CirculateRequest events;
 *  - Accept ConfigureRequest events;
 *      Look and listen:  Requests to configure a window at position x,y means
 * that you are configuring the OUTER FRAME to x,y.  However, Requests to
 * configure the window to dimensions width x height refer to the inner
 * client window.
 * We take care of window gravity for NW (default), NE, SW, and SE.
 */

extern void
ClientSubstructureRedirect OLARGLIST((w, client_data, event, cont_to_disp))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLARG(XEvent *, event)
OLGRA(Boolean *, cont_to_disp)
{
WMStepWidget wm = (WMStepWidget)w;
WMStepPart * wmstep = &wm-> wmstep;
WMGeometry new;
WMGeometry old;
XWindowChanges wc;
int i, dim_chg = 0, pos_chg = 0;
int action = WMSame;
Screen *screen = XtScreen(w);
WMSize size = wmstep->size; /* Save current size */

switch (event-> type)
   {
   case CirculateRequest:
      FPRINTF((stderr, "Ignoring CirculateRequest\n"));
      break;
   case ConfigureRequest:
      new.x            = old.x            = wm-> core.x;
      new.y            = old.y            = wm-> core.y;
      new.width        = old.width        = wmstep-> child-> core.width;
      new.height       = old.height       = wmstep-> child-> core.height;
      new.border_width = old.border_width = 0;
      wmstep->prev.border_width = new.border_width =
					 event-> xconfigurerequest.border_width;
      if (event-> xconfigurerequest.value_mask & CWX)
         new.x = event-> xconfigurerequest.x;
      if (event-> xconfigurerequest.value_mask & CWY)
         new.y = event-> xconfigurerequest.y;
	/* Set the new struct to the requested CLIENT width, ht. */
      if (event-> xconfigurerequest.value_mask & CWWidth){
         new.width = event-> xconfigurerequest.width;
	}
      if (event-> xconfigurerequest.value_mask & CWHeight)
         new.height = event-> xconfigurerequest.height;

      if (event-> xconfigurerequest.value_mask & CWStackMode)
         {
         if ((!event-> xconfigurerequest.value_mask & CWSibling) ||
              (event-> xconfigurerequest.above == None))
            switch(event-> xconfigurerequest.detail)
               {
               case Above:
                  action = WMRaise;                      break;
               case Below:
                  action = WMLower;                      break;
               case TopIf:
               case BottomIf:
               case Opposite:
               default:
                  break;
               }
         }
      /* Check dimension changes; if either width or height has changed,
       * call XtConfigureWidget() with over frame width (e.g., for
       * the WMStepWidget wm). Also call XConfigureWindow() on the
       * client WINDOW, too - INCLUDING change in position.
       *     This doesn't appear to check the requested
       * change in dimension for the minimum width and ht. allowed for
       * the overall frame (to include resize grabbers, header decorations, etc).
       * It DOES set the parent width equal to some minimum that doesn't appear
       * to compensate for the menu button or the pushpin, only the gaps and
       * the linewidths (e.g., border drawings) - that's ParentWidth().
       */
      if (new.width != old.width || new.height != old.height)
         {
	/* Says that one or both of width/ht. has changed on client.
	 * Adjust requested values for overall frame with decorations.
	 */ 
	 dim_chg++;
	/* NOT YET...
         new.width       = ParentWidth(wm, new.width);
         new.height      = ParentHeight(wm, new.height);
	 */
/*
         XtConfigureWidget(wm, new.x, new.y, new.width, new.height, 0);
         wc.x            = event-> xconfigurerequest.x;
         wc.y            = event-> xconfigurerequest.y;
         wc.width        = event-> xconfigurerequest.width;
         wc.height       = event-> xconfigurerequest.height;
         wc.border_width = 0; 
         wc.sibling      = None; 
         wc.stack_mode   = Above; 
         XConfigureWindow(event-> xconfigurerequest.display,
            event-> xconfigurerequest.window,
            event-> xconfigurerequest.value_mask, &wc);
*/

         } /* end if (wid /ht. changes) */

	 /* Check for change in position */
         if (new.x != old.x || new.y != old.y) {
		pos_chg++;
		if (wm->wmstep.xnormalsize.flags & PWinGravity) {
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
				new.x  = (new.x + new.width + 2 *
					event->xconfigurerequest.border_width) -
						 ParentWidth(wm,new.width);
				break;
			case SouthWestGravity:
			case SouthGravity:
				new.y  = (new.y + new.height + 2 *
					event->xconfigurerequest.border_width) -
						 ParentHeight(wm,new.height);
				break;
			case SouthEastGravity:
				new.x  = (new.x + new.width + 2 *
					event->xconfigurerequest.border_width) -
						 ParentWidth(wm,new.width);
				new.y  = (new.y + new.height + 2 *
					event->xconfigurerequest.border_width) -
						 ParentHeight(wm,new.height);
		   }
		}


	 }
	/* The requested width/height is in event->xconfigurerequest.width/
	 * height.
	 */
	if (currentGUI == OL_OPENLOOK_GUI) {
         new.width       = ParentWidth(wm, new.width);
         new.height      = ParentHeight(wm, new.height);
	}
	else { /* Motif */
         new.width       = MParentWidth(wm, new.width);
         new.height      = MParentHeight(wm, new.height);
	}
            /* XtMoveWidget(wm, new.x, new.y); */

	 /* action was initialized to WMSame; if a request came in to raise or
	  * lower a window, then do it.
	  */
         if (action != WMSame)
            RaiseLowerGroup(wm, action);

	 /* Send synthetic event telling client new position of window.
	  * Should be the position of the CLIENT window, not overall frame.
	  * More:
	  * We must be careful here, because if the client doesn't provide
	  * their own icon window, then we use the WMStepWidget core window
	  * to display the icon; as a result, if the client were to do
	  * something like reconfigure it's window while in iconic state,
	  * we don't want to reconfigure the icon!!
	  * On the other hand, the client would be unmapped if in iconic
	  * state, so it can be reconfigured now.
	  */
	if (dim_chg) {
		if (IsIconic(wm)) {
			wmstep->prev.x = new.x;
			wmstep->prev.y = new.x;
			wmstep->prev.width = new.width;
			wmstep->prev.height = new.height;
		}
		else	{
		if((new.x+new.width) > WidthOfScreen(screen))	{
		    new.x -= ((new.x+new.width) - WidthOfScreen(screen));
		    if (new.x <0) 	new.x = 0;
		}
		if((new.y+new.height) > HeightOfScreen(screen))	{
		    new.y -= ((new.y+new.height) - HeightOfScreen(screen));
		    if(new.y <0)	new.y=0;
		 }

		XtConfigureWidget((Widget)wm, new.x, new.y, new.width,
              							new.height,0);
		}
		if (IsIconic(wm)) {
			wm->wmstep.size = WMNORMALSIZE;
			wm->wmstep.metrics = GetMetrics(wm);
		}

#define REQUEST_W	event->xconfigurerequest.width
#define REQUEST_H	event->xconfigurerequest.height

		if (currentGUI == OL_OPENLOOK_GUI)
		{
			XtConfigureWidget(wm->wmstep.child,
            			OriginX(wm), OriginY(wm), 
				REQUEST_W, REQUEST_H, 0);

			SendConfigureNotify(XtDisplay(wmstep-> child), 
				XtWindow(wmstep-> child), new.x + OriginX(wm),
				new.y + OriginY(wm),
				REQUEST_W, REQUEST_H,
				event->xconfigurerequest.border_width);
		}
		else
		{
			XtConfigureWidget(wm->wmstep.child,
            			MOriginX(wm), MOriginY(wm), 
				REQUEST_W, REQUEST_H, 0);

			SendConfigureNotify(XtDisplay(wmstep-> child), 
				XtWindow(wmstep-> child), new.x + MOriginX(wm),
				new.y + MOriginY(wm),
				REQUEST_W, REQUEST_H,
				event->xconfigurerequest.border_width);
		}

#undef REQUEST_W
#undef REQUEST_H

		/* Reset size if we changed it */
		wm->wmstep.size = size;
		wm->wmstep.metrics = GetMetrics(wm);

	}
	else
		if (!dim_chg && !pos_chg) {
		/* Send the synthetic event with the
		 * OLD width and height because the
		 * dimensions didn't change.
		 */
		if (IsIconic(wm)) {
			wm->wmstep.size = WMNORMALSIZE;
			wm->wmstep.metrics = GetMetrics(wm);
		}
		if (currentGUI == OL_OPENLOOK_GUI)
            	  SendConfigureNotify(XtDisplay(wmstep-> child), 
            	  	XtWindow(wmstep-> child), new.x + OriginX(wm),
			new.y + OriginY(wm), old.width, old.height,
			new.border_width);
		else
            	  SendConfigureNotify(XtDisplay(wmstep-> child), 
            	  	XtWindow(wmstep-> child), new.x + MOriginX(wm),
			new.y + MOriginY(wm), old.width, old.height,
			new.border_width);

		/* Reset size if we changed it */
		wm->wmstep.size = size;
		wm->wmstep.metrics = GetMetrics(wm);
		}
		else
			if (!dim_chg && pos_chg) {
				if (IsIconic(wm)) {
					wmstep->prev.x = new.x;
					wmstep->prev.y = new.x;
				}
				else
         				XtConfigureWidget((Widget)wm,
						new.x, new.y,
						new.width, new.height, 0);
				/* Configure Child */
				if (IsIconic(wm)) {
					wm->wmstep.size = WMNORMALSIZE;
					wm->wmstep.metrics = GetMetrics(wm);
				}

				if (currentGUI == OL_OPENLOOK_GUI)
         				XtConfigureWidget(wm->wmstep.child,
						OriginX(wm), OriginY(wm),
						event->xconfigurerequest.width,
						event->xconfigurerequest.height,
						0);
				else
         				XtConfigureWidget(wm->wmstep.child,
						MOriginX(wm), MOriginY(wm),
						event->xconfigurerequest.width,
						event->xconfigurerequest.height,
						0);
				/* Send the synthetic event with the
				 * OLD width and height because the
				 * dimensions didn't change.
				 */
				   if (currentGUI == OL_OPENLOOK_GUI)
            			SendConfigureNotify(XtDisplay(wmstep-> child), 
				  XtWindow(wmstep-> child), new.x + OriginX(wm),
				    new.y + OriginY(wm), old.width, old.height,
							 new.border_width);
				    else
            			SendConfigureNotify(XtDisplay(wmstep-> child), 
				  XtWindow(wmstep->child), new.x + MOriginX(wm),
				    new.y + MOriginY(wm), old.width, old.height,
							 new.border_width);
				/* Reset size if we changed it */
				wm->wmstep.size = size;
				wm->wmstep.metrics = GetMetrics(wm);
			} /* end else if */
      FPRINTF((stderr, "Handled ClientConfigureRequest\n"));
      break;
   case MapRequest:
	/* What is the current state of the window?? */
	{ /* Begin block */
	unsigned long win_state = GetWindowState(XtDisplay(wmstep->child),
							wmstep->window);
	if (win_state == IconicState) {
		if (wmstep->size == WMICONICNORMAL ||
					wmstep-> size == WMICONICFULL) {
			if (wmstep->icon_widget)
				MenuOpenClose(w, (XtPointer)&wm, NULL);
		}
	}
#ifdef WITHDRAWNSTATE
	else
	  if (win_state == WithdrawnState) {
		/* Move to Normal (or iconic) */
		MoveFromWithdrawn(wm);
		StartWindowMappingProcess (wm);
	  }
#endif
	} /* end block */
		
      FPRINTF((stderr, "Handled MapRequest\n"));
      break;
   default:
      FPRINTF((stderr, "Unknown Client substructure request\n"));
      break;
   }

} /* end of ClientSubstructureRedirect */

#if !defined(HELP_MESSAGE)
#define HELP_MESSAGE(type, msg) \
			OlGetMessage(display, NULL, 0, OleNtitle, \
				type, OleCOlClientOlwmMsgs, msg, \
				NULL)
#endif

/*
 * ClientNonMaskable
 * - Handle NonMaskable events for client windows.  Event handler set
 * on the client widgets PARENT (the WMStepWidget) in Initialize().
 * The only client message that is being handled is for HELP.  
 *	- the first data field will contain the window where the pointer was
 *	  when HELP was pressed.
 *	- the 2nd data field will contain the x coord. of the ptr.
 *	- the 3nd data field will contain the x coord. of the ptr.
 *
 * First try to determine if the pointer was inside a window menu; if not,
 * call EventIn() to figure out the exact decoration that HELP key was asked
 * for.  EventIn() returns a WMPiece (enum in wmP.h)- just a flag that rep.
 * a decoration.
 */

extern void
ClientNonMaskable OLARGLIST((w, client_data, event, cont_to_disp))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLARG(XEvent *, event)
OLGRA(Boolean *, cont_to_disp)
{
#define MAIN_MENU	0
#define SECOND_MENU	1

Display * display = XtDisplay(w);
int       i;
Cardinal  id;
char *    text = (char *)NULL;
char *    title = (char *)NULL;
char *	  help_file_name = NULL;
char *    prefix;
char	*hmessage = NULL, *htype = NULL;
WMStepWidget	wm = (WMStepWidget)NULL;
Cardinal	which_menu = MAIN_MENU;

switch (event-> type)
   {
   case ClientMessage:
      if (event-> xclient.message_type == XA_OL_HELP_KEY(display))
         {
         WMPiece piece;
	 WMMenuInfo * WMMenu = WMCombinedMenu;
	 /* Set WMMenu to some default so the code will not break */
         if (w == WMMenu->MenuShell || w == WMMenu->CascadeShell)
            {
            piece = WM_BTN;
		/* OLDWM - We use gadgets for the oblong buttons on the menu,
		 * except the very first one - it's a MenuButton (the
		 * Dismiss Button.
		 */
		if (currentGUI == OL_OPENLOOK_GUI)
			id = OlFlatGetItemIndex(w == WMMenu->MenuShell ?
					WMMenu->menu : WMMenu->cascade,
                                        (Position) event->xclient.data.l[1],
                                        (Position) event->xclient.data.l[2]);
		else {
			/* Motif - must be w == MenuShell */
			if ( (id = OlFlatGetItemIndex(WMMenu->menu,
                                        (Position) event->xclient.data.l[1],
                                        (Position) event->xclient.data.l[2]))
				== OL_NO_ITEM)
			{
			which_menu = SECOND_MENU;
			id = OlFlatGetItemIndex(WMMenu->menu2,
                                        (Position) event->xclient.data.l[1],
                                        (Position) event->xclient.data.l[2]);
			}
		} /* Motif */
            }
	else
		if (w == help_shell) {
			/* Inside the help window.
			 */
			piece = WM_NULL;
			title = "Help Window"; /* should get replaced below
						* when OlGetMsg is called
						*/
			help_file_name = "help_window";
			piece = WM_NULL;
			htype = OleThelp;
			hmessage = OleMtitle_help;
		}
		else if (XtIsWidget(w) && w->core.name && strcmp(w->core.name,
				"olwmXYIcon") == 0 ) {
			/* Icon - I hope so */
			title = "Icon Help";
			if (currentGUI == OL_OPENLOOK_GUI)
				help_file_name = "icon";
			else
				help_file_name = "mot_icon";
			piece = WM_NULL;
			htype = OleTicon;
			hmessage = OleMtitle_icon;
		}

         else
	    /* Call EventIn() to find out the decoration where the help key was
	     * pressed.  We must have hit a decoration - can check first with
	     * XtIsSubclass().
	     */
		if (XtIsSubclass(w, wmstepWidgetClass)) {
			wm = (WMStepWidget)w;
			piece = EventIn(wm,
				(Position)event-> xclient.data.l[1], 
				(Position)event-> xclient.data.l[2]);
		}
		else
		{
			Global_Menu_Info	*gmi_ptr = global_menu_info;

				/* Mystery what this widget is */
			piece = WM_NULL;

			while (gmi_ptr)
			{
				if (gmi_ptr->MenuShell == w)
				{
					piece = WM_BTN;
					id = OL_NO_ITEM;
					break;
				}
				gmi_ptr = gmi_ptr->next;
			}
		}
		/* That mystery may be a special menu shell - a search should
		 * be done to narrow down the menu shell that this key press
		 * came from (if any), then the menu panes can be searched
		 * one at a time, with OlFlatItemIndex() to see if any
		 * return something besides OL_NO_ITEM.
		 */
/*
 * setarg the textedit widget with the appropriate text
 */
         switch (piece)
            {
            case WM_BTN:
                if ( id == OL_NO_ITEM ||
		     (which_menu != MAIN_MENU &&
		      (which_menu != SECOND_MENU || id != 0)) )
                {
			help_file_name = "menu_backgroun";
			title = HELP_MESSAGE(OleTmenu, OleMtitle_menu);
                }
                else if (w == WMMenu->MenuShell)
                {
			if (!called_filled_help)
				FillHelpTitles(WMMenu->MenuShell);
			if (which_menu == MAIN_MENU)
			{
				help_file_name = WMMenu->menu_items[id].
							helpData->helpfile;
				title = WMMenu->menu_items [id].
							helpData->helptitle;
			}
			else if (which_menu == SECOND_MENU)
			{
				help_file_name = WMMenu->menu_items2[id].
							helpData->helpfile;
				title = WMMenu->menu_items2 [id].
							helpData->helptitle;
			}
                }
                else
                {
			if (!called_filled_help)
				FillHelpTitles(WMMenu->MenuShell);

			help_file_name = WMMenu->cascade_items[id].
						helpData->helpfile;
			title = WMMenu->cascade_items [id].helpData->helptitle;
                }
                break;
            case WM_SE:
            case WM_SW:
            case WM_NW:
            case WM_NE:
		htype = OleTresizeCorner;
		hmessage = OleMtitle_resizeCorner;
		if (currentGUI == OL_OPENLOOK_GUI) {
			help_file_name = "resize_corner";
		}
		else {
			help_file_name = "mot_rszcnr";
		}
                break;
            case WM_BANNER:
                help_file_name = "win_title";
		htype = OleTtitle;
		hmessage = OleMtitle_title;
               /*title = HELP_MESSAGE(OleTtitle, OleMtitle_title);*/
                break;
            case WM_T:
            case WM_L:
            case WM_R:
            case WM_B:
		if (currentGUI == OL_OPENLOOK_GUI ||
		    !(wm->wmstep.decorations & WMResizable)){
			help_file_name = "border";
			htype = OleTborder;
			hmessage = OleMtitle_border;
		}
		else {
			/* Motif and resizable */
			help_file_name = "mot_rszhndl";
			htype = OleTresizeHandles;
			hmessage = OleMtitle_resizeHandles;
		}
                break;
            case WM_PP:
                help_file_name = "pushpin";
		htype = OleTpushpin;
		hmessage = OleMtitle_pushpin;
                break;
            case WM_MM:
                help_file_name = "window_menu_bu";
		htype = OleTmenuButton;
		hmessage = OleMtitle_menuButton;
                break;
            case WM_MAXB:
                help_file_name = "mot_maxb";
		htype = OleTmotifMaximizeButton;
		hmessage = OleMtitle_motifMaximizeButton;
                break;
            case WM_MINB:
                help_file_name = "mot_minb";
		htype = OleTmotifMinimizeButton;
		hmessage = OleMtitle_motifMinimizeButton;
                break;
            default:
		if (help_file_name == (char*)NULL)
		{
			help_file_name = "win_title";
			htype = OleTtitle;
			hmessage = OleMtitle_title;
		}
		break;
            }
		if (help_file_name)
		{
			text = FindHelpFile(w, help_file_name);
		}
		prefix = HELP_MESSAGE(OleTwinMgr,OleMtitle_winMgr);
		if (hmessage)
			title = HELP_MESSAGE(htype, hmessage);	


		/* Check if the help manager is running.  If it is not */
		/* running, then it's business as usual; otherwise,    */
		/* re-route help message to the help manager.          */

		if (XGetSelectionOwner(
			display, XA_WM_HELP_QUEUE(display)) == None) {

		    if(help_shell == (Widget)0)
			CreateHelpTree(XtDisplayOfObject(Frame));

		    SetHelpMsg(help_shell, event, text, prefix, title);

		} else {

		    GetWMDesktopHelp(XtDisplayOfObject(Frame), XtWindow(Frame),
			event, text, title, prefix);
		}
         }
	else
		if (event->xclient.message_type == XA_WM_CHANGE_STATE(display))
		{
			WMStepWidget wm      = (WMStepWidget)client_data;
			WMStepPart * wmstep  = &wm-> wmstep;

			if (event->xclient.data.l[0] == IconicState) {
				if ( !(IsIconic(wm)) ) {
					MenuOpenClose(w, (XtPointer)&wm,
							(XtPointer) wm);
				}
			} else
			if (event->xclient.data.l[0] == NormalState) {
				if (IsIconic(wm)) {
					MenuOpenClose(w, (XtPointer)&wm,
							(XtPointer) wm);
				}
				else
				  if (wm->wmstep.size == WMWITHDRAWN) {
					Boolean  cont_disp = False;
					XMapRequestEvent mapev;
					mapev.type = MapRequest;
					ClientSubstructureRedirect((Widget)wm,
						(XtPointer)NULL,
						(XEvent *)&mapev,
						&cont_disp);
				  }
			} else
			if (event->xclient.data.l[0] == WithdrawnState) {
				/* We may be in iconic or normal state */
				if (wmstep->size != WMWITHDRAWN) {
					XUnmapEvent unmapev;
					Boolean  cont_disp = False;

					unmapev.type = UnmapNotify;
					ClientStructureNotify((Widget)wm,
						(XtPointer)wm,
						(XEvent *) &unmapev,
						&cont_disp);
				}
			}
		} /* end if message_type == XA_WM_CHANGE_STATE */

      FPRINTF((stderr, "ClientClientMessage\n"));
      break;
   default:
      FPRINTF((stderr, "got non-maskable client event %d\n", event-> type));
   }

#undef MAIN_MENU
#undef SECOND_MENU
} /* end of ClientNonMaskable */


/* May be one of those convertible menus - moved from Withdrawn state to
 * override_redirect
 */

extern void
DumpParentCarefully OLARGLIST((w, restore_coords))
OLARG(Widget, w)
OLGRA(Boolean, restore_coords)
{
WMStepWidget wm      = (WMStepWidget)w;
Display *    display = XtDisplay(wm);
Window       window  = XtWindow(wm);
WMStepPart * wmstep  = &wm-> wmstep;
Screen		*screen = XtScreen(wm);
Window	root_win = RootWindowOfScreen(screen);
XEvent ev;
Boolean cont_disp;


	


/* Reparent the client window to root, destroy WMStepWidget, etc.
 * Before reparenting to the root window: check if there is a pending
 * ReparentNotify event on the queue FOR THIS CLIENT WINDOW - if there
 * is, then it means that the client (or someone) reparented it to another
 * window, and we shouldn't reparent it to root.
 * One more - ConfigureRequest events.
 */
	XSync(display,False);
#ifdef WITHDRAWNSTATE
	if (wmstep->icon_widget && wmstep->decorations & WMIconWindowReparented
				&& wmstep->xwmhints->icon_window) {
		Window window = wmstep->xwmhints->icon_window;

		XUnmapWindow(display, window);
		XReparentWindow(display, window, 
				root_win, wm->core.x, wm->core.y);
		XChangeSaveSet(display, window, SetModeDelete);
	}
#else
	if (wmstep->icon_widget) {
		if(wmstep->decorations & WMIconWindowReparented
				&& wmstep->xwmhints->icon_window) {
			Window window = wmstep->xwmhints->icon_window;

			XUnmapWindow(display, window);
			XReparentWindow(display, window,
			root_win, wm->core.x, wm->core.y);
			XChangeSaveSet(display, window, SetModeDelete);
		}
		DestroyStepIcon(wm);
	}
#endif

	XChangeSaveSet(display, wmstep->window, SetModeDelete);
#ifndef WITHDRAWNSTATE
	XGrabServer(display);
	AddRemoveEventHandlers(wm, False);
#endif

	XSetWindowBorderWidth(display, wmstep->window,
					wmstep->prev.border_width);
	if (restore_coords &&
		XCheckTypedWindowEvent(display, wmstep->window, ReparentNotify,
								&ev) == 0) {
		XReparentWindow(display,wmstep-> window, 
   			root_win, wm->core.x, wm->core.y);
#if 0
	/* it's a questionable whether we should send this */
		SendConfigureNotify(display,
				wmstep->window,
				wm->core.x, wm->core.y,
				wmstep->child->core.width,
				wmstep->child->core.height, 
				wmstep->prev.border_width);
#endif
	}
	if (wmstep->num_cmap_windows)
		RemoveWMColormaps(wm);
	XUngrabServer(display);
	XFlush(display);
	XtDestroyWidget((Widget)wm);
} /* DumpParentCarefully */


/*
 * ClientStructureNotify
 * - Event handler added to the child (client) widget of the
 * WMStepWidget (parent, or overall frame, widget).
 */

extern void
ClientStructureNotify OLARGLIST((w, client_data, event, cont_to_disp))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLARG(XEvent *, event)
OLGRA(Boolean *, cont_to_disp)
{
WMStepWidget wm      = (WMStepWidget)client_data;
WMStepPart * wmstep  = &wm-> wmstep;
Display *    display = XtDisplay(wm);
Window       window  = XtWindow(wm);
unsigned long state;

switch (event-> type)
   {
   case CirculateNotify: /* nop */
      FPRINTF((stderr, "ClientCirculateNotify\n"));     break;
   case ConfigureNotify: /* nop */
	if (event->xconfigure.override_redirect &&
		(state = GetWindowState(display, wmstep->window)) == WithdrawnState) {
		/* They switched out on us to override_redirect.  We
		 * can't destroy the parent here until they map the winodw,
		 * because in the time interval that we do thay, they can
		 * map the window window while we have the event handler set,
		 * and we would lose the event - play it safe.
		 * If selectDoesPreview == True there are problems:  the first
		 * time a popup menu pops down, we get an immediate configurenotify,
		 * with override_red true;  but the next time it's popped up,
		 * it goes STRAIGHT to a maprequest without becoming a popup
		 * menu again - the x,y coordinates given for it here are
		 * wrong for whatever reason, they are relative to the
		 * decoration shell (not that selectDoesPreview is true
		 * only in openlook).  For now, just configure the widget
		 * as is - they will likely map in the same spot, near the
		 * upper left corner.
		 */
		int	usewidth,
			useheight;

		if (currentGUI == OL_OPENLOOK_GUI) {
			usewidth = ParentWidth(wm,event->xconfigure.width);
			useheight = ParentHeight(wm,event->xconfigure.height);
		}
		else {
			usewidth = MParentWidth(wm,event->xconfigure.width);
			useheight = MParentHeight(wm,event->xconfigure.height);
		}
		XtConfigureWidget((Widget)wm,  event->xconfigure.x,
			event->xconfigure.y, usewidth, useheight, 0);

		wmstep->prev.border_width = event->xconfigure.border_width;
/* mlp - we need the window attributes to change also because when the
window comes up again, we position it by window attributes...
 - will this do it??
 */
            	SendConfigureNotify(XtDisplay(wmstep->child), 
				  XtWindow(wmstep-> child),
event->xconfigure.x, event->xconfigure.y,event->xconfigure.width,
event->xconfigure.height, event->xconfigure.border_width);

	}
	
      FPRINTF((stderr, "ClientConfigureNotify\n"));     break;
   case DestroyNotify:
	FPRINTF((stderr, "ClientDestroyNotify: window=%x\n", window));
	{
	int k;

	/* Make sure menu isn't posted for this step kid */

	CheckPostedMenu(wm);
	if ((k = IsPending(wmstep->window)) >= 0) {
		FPRINTF((stderr, "Client window has something pending\n"));
		AddDeletePendingList((Widget)wm, pending_kids[k].delete, False);
	}
	XtDestroyWidget((Widget)wm);

	break;
	}
   case GravityNotify: /* nop */
      FPRINTF((stderr, "ClientGravityNotify\n"));       break;
   case MapNotify:
      /* if Click-to-type focus model and NOT iconic, give it focus */
      if (wm == help_parent)
         {
         FPRINTF((stderr, "is the help parent\n"));
	wm_help_win_mapped = TRUE;
         }
	else {
		/* check override_redirect - is set then this may be one of
		 * those  override_redirect windows.
		 * ---- for convertible menus, have the menus send olwm a
		 * synthetic destroy window event.
		 */
		if (event->xmap.override_redirect) {
			/* Destroy decoration set, map */
			DumpParentCarefully((Widget)wm, (Boolean)True);
		}
		else {
			unsigned long state = GetWindowState(display, wmstep->window);
			if (state == IconicState)
				; /* Nothing to do */
			else
			  if (state == WithdrawnState) {
				/* Prepare for re-mapping */
				XtMapWidget((Widget)wm);
				XtMapWidget(wm->wmstep.child);
				SetFocus((Widget)wm, GainFocus, 1);
			}
		}			
	} /* not help parent */
		break;			
   case ReparentNotify:
	/* Turn off the WMNotReparented flag set earlier - unless reparented to some
	 * other window we don't know about
	 * Check for two things: 1 -change in override_redirect;
	 *			 2 - change in who it was reparented to.
	 */
	{
	Window window = event->xreparent.window;
	Window newparent = event->xreparent.parent;
	if (newparent !=  wm->core.window) {
		/* You're out */
		DumpParentCarefully((Widget)wm, (Boolean)False);
	}
	else
		if (event->xreparent.override_redirect) {
			DumpParentCarefully((Widget)wm, (Boolean)True);
		}
		else {
			wmstep-> decorations &= ~WMNotReparented;
		}
	}
		break;
   case UnmapNotify:
      if (wm == help_parent)
         {
         FPRINTF((stderr, "is the help parent\n"));
		/* Make sure menu isn't posted for this step kid */
		CheckPostedMenu(wm);
		if (wm_help_win_mapped == TRUE) {
			/* Help window was popped down somehow -
			 * if popped down via toolkit (e.g., with
			 * ESC key), then we must unmap the help_parent
			 * decoration frame.  Do this anyway to be sure.
			 * Also reset pin state in our decoration cache.
			 */
			unsigned long state = (WMPushpin|WMPinIn);
			wm_help_win_mapped = FALSE;
			XtUnmapWidget((Widget)help_parent);
			SetPushpinState(help_parent, state, FALSE);
		}
		if (!wmrcs->pointerFocus && (wmstep->protocols & HasFocus) )
			SetFocus((Widget)wm, LoseFocus,
				(wmstep->is_current == True ? 1 : 0) );
         }
      else
         if (wmstep-> protocols & IgnoreUnmap || 
             wmstep-> decorations & WMNotReparented)
            {
	    /* We set the IgnoreUnmap flag ourselves to tell us
	     * the unmapnotify event just received was caused by us.
	     * I unmap the client when going to iconic state.
	     * A special case arises when a window is mapped prior to
	     * the window mgr. running; when olwm reparents the window to
	     * a step widget window, it generates an unmapnotify event;
	     * we ignore this because prior to the reparenting, Initialize()
	     * sets a flag  WMNotReparented, warning us that the window hasn't been
	     * reparented yet, and the ensuing unmap should be ignored;
	     * However, if the window  was intended to be started in iconic
	     * state (has WM_HINTS.initial_state == Iconic) then we call
	     * XUnmapWindow() on the client window in Realize(), generating an
	     * additional unmapnotify (for this case, the IgnoreUnmap flag is
	     * set in Initialize(); as a result, we only turn off the IgnoreUnmap
	     * flag if the WMNotReparented flag isn't set (the flags must be
	     * handled independantly).
	     */
	    if (! (wmstep->decorations & WMNotReparented))
            	wmstep-> protocols &= ~IgnoreUnmap;
            FPRINTF((stderr, "ignore unmap\n"));
            }
         else
            {
	    /* UnmapNotify caused by the client - dump it */
	    int k;
	    WMState wms;

            FPRINTF((stderr, "destroyed the step parent window=%x\n", window));
            FPRINTF((stderr, "\twmstep->window=%x\n", wmstep->window));
		CheckPostedMenu(wm);
	    if ((k = IsPending(wmstep->window)) >= 0) {

		/* Make sure menu isn't posted for this step kid */
		FPRINTF((stderr, "Client window has something pending\n"));
		AddDeletePendingList((Widget)wm, pending_kids[k].delete,
								False);
	    }

		/* The following can happen if WE unmap the window
		 * ourselves, as we do below for group members that do
		 * not have the delete window protocol. and it generates
		 * an unmapnotify.  Skip all this yardwork.
		 */
		if (wmstep->size == WMWITHDRAWN)
			break;
#ifdef WITHDRAWNSTATE
	    /* Change state for the window to withdrawn - BE CAREFUL:
	     * We can go from iconic to withdrawn or from normal to
	     * withdrawn.
	     */
	    if (IsIconic(wm)) {
		int  i;
		if (wmstep->icon_widget) {

		   if (wmstep->is_current && currentGUI == OL_MOTIF_GUI &&
				motwmrcs->iconDecoration & ICON_ACTIVELABEL)
					/* unmap it */
				CheckAIWDown(wm);
		
		if (wmstep->xwmhints->window_group == wmstep->window)
			ConstructGroupList(wm, NoAppend, TRUE);
		else {
			/* window group isn't this window; who is it?? */
			int k = IsWMStepChild(wmstep->xwmhints->window_group);
			if (k < 0)
				ConstructGroupList(wm, NoAppend, TRUE);
			else
				ConstructGroupList(wm, NestedNoAppendAny, True);
		}
		if (currentGUI == OL_MOTIF_GUI)
			/* Release the map position */
			ReleaseMapPosition(wmstep->icon_map_pos_row,
					wmstep->icon_map_pos_col);
		/* The windows entire group is in group_list->array */
		 { /* Begin block */
			WMStepWidget temp;
			for (i = 1; i < group_list->used; i++) {
				temp = (WMStepWidget)(group_list->p[i]);
				if (temp->wmstep.protocols &
							DeleteWindow) {
					SendProtocolMessage(display,
					  temp->wmstep.window,
					  XA_WM_DELETE_WINDOW(display),
					  CurrentTime);
					continue;
				}
				SetWindowState(temp, WithdrawnState,
						wm->wmstep.window);
				MoveToWithdrawn(temp);
				temp->wmstep.size = WMWITHDRAWN;
			}
		  } /* end block */
		if (wmstep->decorations & WMIconWindowMapped) {
			if (wmstep->decorations & WMIconWindowReparented &&
				   wmstep->xwmhints->icon_window != (Window)NULL) {
				XUnmapWindow(display, wmstep->xwmhints-> icon_window);
				XReparentWindow(display, wmstep->xwmhints->icon_window, 
					RootWindowOfScreen(XtScreen(wm)), wm-> core.x,
					wm-> core.y);
			XChangeSaveSet(display, wmstep->xwmhints->icon_window,
				SetModeDelete);
			wmstep->decorations &= ~WMIconWindowReparented;
			} /* Reparented and icon_window */
		} /* IconWindowMapped */
		DestroyStepIcon(wm);
		wmstep-> decorations &= ~WMIconWindowMapped;
		} /* if wmstep->icon_widget */

		/* Set window state property */
		SetWindowState(wm, WithdrawnState, wmstep->window);
		MoveToWithdrawn(wm);

	    } /* If is iconic */
		else if
		   (wm->wmstep.size != WMWITHDRAWN) {
			/* Not iconic, must be normal - move to Withdrawn.
			 * First unmap all transients and members of
			 * window group
			 */
				int i;
				WMStepWidget tempwm;
				ConstructGroupList(wm, NoAppend, TRUE);

				XtUnmapWidget((Widget)wm);
				SetWindowState(wm, WithdrawnState,
							wmstep->window);
				MoveToWithdrawn(wm);

				/* Forcefully remove the windows
				 * transients
				 */
				for (i=1; i < group_list->used; i++) {
					tempwm = (WMStepWidget)group_list->p[i];
					if (tempwm->wmstep.size ==
							WMWITHDRAWN)
						continue;
					if (tempwm->wmstep.protocols &
							DeleteWindow) {
						SendProtocolMessage(display,
						  tempwm->wmstep.window,
						  XA_WM_DELETE_WINDOW(display),
						  CurrentTime);
						continue;
					}
				/* Don't unmap any transients or group
				 * members that are notice windows.
				 */
				if (tempwm->wmstep.decorations & WMUsesOlwa) {
				  if (tempwm->wmstep.olwa.flags & _OL_WA_WIN_TYPE &&
					tempwm->wmstep.olwa.win_type == 
					XA_OL_WT_NOTICE(display))
						continue;
				}
				SetWindowState(tempwm, WithdrawnState,
						wm->wmstep.window);
				XtUnmapWidget(tempwm->wmstep.child);
				XtUnmapWidget((Widget)tempwm);
				MoveToWithdrawn(tempwm);
				tempwm->wmstep.size = WMWITHDRAWN;
				} /* for */
		} /* else if not WMWITHDRAWN */
					
		wmstep->size = WMWITHDRAWN;
#else
	/* No Withdrawn state - get rid of decoration set */
	    if (IsIconic(wm)) {
		int  i;
		if (wmstep->icon_widget) {

		   if (wmstep->is_current && currentGUI == OL_MOTIF_GUI &&
				motwmrcs->iconDecoration & ICON_ACTIVELABEL)
					/* unmap it */
				CheckAIWDown(wm);
		
		if (wmstep->xwmhints->window_group == wmstep->window)
			ConstructGroupList(wm, NoAppend, TRUE);
		else {
			/* window group isn't this window; who is it?? */
			int k = IsWMStepChild(wmstep->xwmhints->window_group);
			if (k < 0)
				ConstructGroupList(wm, NoAppend, TRUE);
			else
				ConstructGroupList(wm, NestedNoAppendAny, True);
		}
		if (currentGUI == OL_MOTIF_GUI && motwmrcs->iconAutoPlace)
			/* Release the map position */
			ReleaseMapPosition(wmstep->icon_map_pos_row,
					wmstep->icon_map_pos_col);
		/* The windows entire group is in group_list->array */
		 { /* Begin block */
			WMStepWidget temp;
			for (i = 1; i < group_list->used; i++) {
				temp = (WMStepWidget)(group_list->p[i]);
				if (temp->wmstep.protocols &
							DeleteWindow) {
					SendProtocolMessage(display,
					  temp->wmstep.window,
					  XA_WM_DELETE_WINDOW(display),
					  CurrentTime);
					continue;
				}
				SetWindowState(temp, WithdrawnState,
						wm->wmstep.window);
				temp->wmstep.size = WMWITHDRAWN;
				/* Dump client - reparent */
				DumpParentCarefully((Widget)temp, True);
			}
		  } /* end block */
		} /* IconWindowMapped */
                if (wmstep->is_current) {
                        SetFocus((Widget)wm, LoseFocus, 1);
		}
		DumpParentCarefully((Widget)wm, True);
		/*DestroyStepIcon(wm);*/
		wmstep-> decorations &= ~WMIconWindowMapped;
		} /* if wmstep->icon_widget */
		else if
		   (wm->wmstep.size != WMWITHDRAWN) {
			/* Not iconic, must be normal - move to Withdrawn.
			 * First unmap all transients and members of
			 * window group
			 */
				int i;
				WMStepWidget tempwm;
				ConstructGroupList(wm, NoAppend, TRUE);

				XtUnmapWidget((Widget)wm);
				SetWindowState(wm, WithdrawnState,
							wmstep->window);

				/* Forcefully remove the windows
				 * transients
				 */
				for (i=1; i < group_list->used; i++) {
					tempwm = (WMStepWidget)group_list->p[i];
					if (tempwm->wmstep.size ==
							WMWITHDRAWN)
						continue;
					if (tempwm->wmstep.protocols &
							DeleteWindow) {
						SendProtocolMessage(display,
						  tempwm->wmstep.window,
						  XA_WM_DELETE_WINDOW(display),
						  CurrentTime);
						continue;
					}
				/* Don't unmap any transients or group
				 * members that are notice windows.
				 */
				if (tempwm->wmstep.decorations & WMUsesOlwa) {
				  if (tempwm->wmstep.olwa.flags & _OL_WA_WIN_TYPE &&
					tempwm->wmstep.olwa.win_type == 
					XA_OL_WT_NOTICE(display))
						continue;
				}
				SetWindowState(tempwm, WithdrawnState,
						wm->wmstep.window);
				XtUnmapWidget(tempwm->wmstep.child);
				XtUnmapWidget((Widget)tempwm);
				DumpParentCarefully((Widget)tempwm, True);
				} /* for */
                          /* Group leader */
                         if (wmstep->is_current) {
                                  SetFocus((Widget)wm, LoseFocus, 1);
                         }
                         DumpParentCarefully((Widget)wm, True);
		} /* else if not WMWITHDRAWN */
					
#endif
		if (start_or_terminate == TERMINATE_FLAG &&
			ReadyToDie() ) {
         	   EnqueueWSMRequest(XtDisplay(w),
					RootWindowOfScreen(XtScreen(w)),
            				WSM_EXIT, &wsmr);
		}
            }
      FPRINTF((stderr, "ClientUnmapNotify\n"));         break;
   default:
      FPRINTF((stderr, "Client unknown structure notify event\n"));
   }

} /* end of ClientStructureNotify */


/*
 * ClientPropertyChange
 * - Event handler added to client widget.
 * Therefore, w = client window widget, and client_data is the
 * parent (frame widget, WMStepWidget).
 */

static void
ClientPropertyChange OLARGLIST((w, client_data, event, cont_to_disp))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLARG(XEvent *, event)
OLGRA(Boolean *, cont_to_disp)
{
WMStepWidget parent = (WMStepWidget)client_data;
Display *    display = XtDisplay(w);
Screen *     screen  = XtScreen(w);
Window       window  = XtWindow(w);
WMStepPart * wmstep  = &parent-> wmstep;

int len;
XRectangle rect;
Atom	ol_decor_add;

if (event-> xproperty.state == PropertyNewValue)
   {
   if (event-> xproperty.atom == XA_WM_NORMAL_HINTS) {
      GetNormalHints(wmstep, display, window, screen);
	}
   else if (event-> xproperty.atom == XA_WM_TRANSIENT_FOR) {
	(void)XGetTransientForHint(display, window, &wmstep->transient_parent);
	/* Don't change any decorations for the window - assume the
	 * decorations stay the same.
	 */
   }
   else if (event-> xproperty.atom == XA_WM_HINTS) {
 	Window icon_window = (Window) 0;
	Pixmap icon_pixmap = (Pixmap)None;
	Pixmap icon_mask = (Pixmap)None;
	int icon_x = -10101, icon_y = -10101;

	/* Let's track the WM_HINTS property - very useful if someone
	 * changes the window_group of a window on the fly.
	 * To complicate things, suppose someone changes the icon pixmap,
	 * window, or icon_mask?  If they do and the window is currently
	 * in iconic state, then redisplay the icon - do it the easy
	 * way - call MenuRefresh().
	 */
	if (wmstep->icon != NULL) {
		icon_x = wmstep->icon->x;
		icon_y = wmstep->icon->y;
	}
	if (wmstep-> hints & WMHints) {
		if ((wmstep-> xwmhints-> flags & IconMaskHint))
			icon_mask = wmstep->xwmhints->icon_mask;
		if ((wmstep-> xwmhints-> flags & IconPixmapHint))
			icon_pixmap = wmstep->xwmhints->icon_pixmap;
		if ((wmstep-> xwmhints-> flags & IconWindowHint))
			icon_window = wmstep->xwmhints->icon_window;
		if ((wmstep-> xwmhints-> flags & IconPositionHint)) {
			icon_x = wmstep->xwmhints->icon_x;
			icon_y = wmstep->xwmhints->icon_y;
		}
	}
      GetWMHints(wmstep, display, window);
	if (wmstep->hints & WMHints) {
		int redisplay = 0;
		if ((wmstep-> xwmhints->flags & IconMaskHint) &&
		    (icon_mask != wmstep->xwmhints->icon_mask))
			redisplay++;
		if ((wmstep-> xwmhints->flags & IconPixmapHint) &&
		    (icon_pixmap != wmstep->xwmhints->icon_pixmap))
			redisplay++;
		if ((wmstep->xwmhints->flags & IconWindowHint) &&
		    (icon_window != wmstep->xwmhints->icon_window) ) {
			/* Is there an old icon window still around?  If
			 * the application used an icon window, then
			 * it's out of our hands now, because OpenClose()
			 * always reparents it to the root window and
			 * changes the save set mode.
			 * only have to deal with it if it's currently
			 * mapped.
			 */
				
			if (wmstep->decorations & WMIconWindowReparented &&
				wmstep->decorations & WMIconWindowMapped)
			{
			  if (wmstep->icon_widget) {
				/* Under the current implementation, the
				 * flags WMIconWindowMapped and WMIconWindow-
				 * Reparented are either both on or both off.
				 * In addition, if they are both on, then
				 * wmstep->icon_widget!= NULL, because
				 * we create and destroy the icon_widget as
				 * we need it.
				 */
				if (wmstep->decorations &
				   WMIconWindowReparented &&
				   wmstep->decorations & WMIconWindowMapped &&
				   icon_window != (Window)NULL) {
   					XUnmapWindow(display, icon_window);
   					XReparentWindow(display, icon_window,
      						RootWindowOfScreen(screen),
						parent->core.x, parent->core.y);
					XChangeSaveSet(display, icon_window,
							SetModeDelete);
				}
				/* Just throw this in - this flag can't be
				 * meaningful now, because if there was an
				 * application icon window mapped and
				 * reparented, it isn't anymore
				 */
				wmstep->decorations &= ~WMIconWindowReparented;
					/* Now bring on the new icon window!! */
				if (wmstep->xwmhints->icon_window !=
								(Window)0) {
					if (currentGUI == OL_OPENLOOK_GUI) {
						int	xoffset,
							yoffset;
						XChangeSaveSet(display, 
						  wmstep->xwmhints->icon_window,
						  SetModeInsert);
						XResizeWindow(display,
						  wmstep->xwmhints->icon_window,
						  ol_icon_image_width,
						  ol_icon_image_height);

			xoffset = (icon_width - ol_icon_image_width) / 2;
			yoffset = ICON_BORDER_WIDTH + ICON_IMAGE_PAD;

						XReparentWindow(display,
						  wmstep->xwmhints->icon_window,
						  XtWindow(
							wmstep->icon_widget),
							xoffset, yoffset);
					} /* Open Look */
					else
					   ReparentMotifIconWindow(display,
								wmstep);

					if (IsIconic(parent)) redisplay++;
				} /* end if new icon_window != NULL */
			  } /* if wmstep->icon_widget != NULL */
			  else
				wmstep->decorations &= ~
					(WMIconWindowReparented |
					 WMIconWindowMapped);
			} /* if WMIconWindowReparented && WMIconWindowMapped*/
	} /* if IconWindowHint && current icon_window != previous one */
		/* Note - if the current icon_window == NULL, then it is
		 * O.K. to not reparent it here and do other work for it,
		 * because WMIconWindowReparent flag would not be set yet,
		 * and this work would get done in OpenClose().
		 */
		if (wmstep-> xwmhints-> flags & IconPositionHint) {
			if ( (icon_x != wmstep->xwmhints->icon_x) ||
			     (icon_y != wmstep->xwmhints->icon_y) ) {
				icon_x = wmstep->xwmhints->icon_x;	
				icon_y = wmstep->xwmhints->icon_y;	
				if (wmstep->icon != NULL) {
					wmstep->icon->x = icon_x;
					wmstep->icon->y = icon_y;
					if (IsIconic(parent))
						XtMoveWidget((Widget)parent,
							icon_x, icon_y);
				}
			}
		} /* wmhints.flags & IconPositionHint */
		if ( (IsIconic(parent)) && redisplay)
			MenuRefresh((Widget)parent, (XtPointer)&parent,
					(XtPointer)parent);
	} /* end if (wmstep->hints & WMHints) */
   } /* end if (xproperty.atom == XA_WM_HINTS) */
   else if (event-> xproperty.atom == XA_WM_NAME)
      {
      /* New window name for title bar */
      GetWindowName(wmstep, display, window);

      /* Fill in XRectangle fields of title bars dimensions */
	
	if (wmstep-> decorations & WMHeader) {
            if (currentGUI == OL_OPENLOOK_GUI)
                rect = HeaderRect(parent);
            else        {
                rect.x      = MBorderX(parent);
                rect.y      = MBorderY(parent);
                rect.width  = parent->core.width - 2*MBorderX(parent);
                rect.height = wmstep->metrics->motifButtonHeight;
            }

		/* Do XClearArea on title area, then call the redrawing
		 * procedure
		 */
		OlClearRectangle(parent, &rect, False);
		DisplayWM(parent, &rect);
	}
      }
   else if (event-> xproperty.atom == XA_WM_ICON_NAME)
      {
      GetIconName(wmstep, display, window);
	if (IsIconic(parent)) {
		int diff = (2 * icon_height) / 3;
		XClearArea(XtDisplay(parent),XtWindow(parent),
			0, (2 * icon_height)/3,
			icon_width, icon_height - diff, True);
	}
	else
		/* If window uses icon name for title (no WM_NAME)*/
		if (wmstep-> decorations & WMHeader) {
			rect = HeaderRect(parent);
			OlClearRectangle(parent, &rect, False);
			DisplayWM(parent, &rect);
		}
      }
   else if (event-> xproperty.atom == XA_WM_COMMAND)
      {
	int k;
	/* Is this window on the pending list?? */
	if ( (k = IsPending(window)) >= 0) {
		/* The only way it can be on the pending list is if WE
		 * put it there after someone presses Quit on the window
		 * menu, and olwsm isn't running; so, do the rest.
		 * Be careful: the window that gets the delete message,
		 * if any, is the one in the delete field of the pending
		 * list.  This is the window that selected Quit from the
		 * window menu.
		 */
		WMStepWidget temp = (WMStepWidget)pending_kids[k].delete;

		if (temp->wmstep.protocols & DeleteWindow)
			SendProtocolMessage(display, temp->wmstep.window,
					 XA_WM_DELETE_WINDOW(display), CurrentTime);
		else
			XKillClient(display,temp->wmstep.window);
		AddDeletePendingList((Widget)parent,(Widget)temp, False);
	} /* end if (IsPending() >= 0) */
      } /* end if atom == WM_COMMAND */
   else if (event-> xproperty.atom == XA_WM_PROTOCOLS(display))
      {
      ReadProtocols(parent, w, window);
      }
   else if (event-> xproperty.atom == XA_OL_PIN_STATE(display)) {
}
   else if (event-> xproperty.atom == XA_OL_WIN_BUSY(display))
           {
	   /* Change property state on the window, let DisplayWM() take care
	    * of the rest.
	    */
	if (currentGUI == OL_OPENLOOK_GUI) {
           GetBusyState(wmstep, display, window);
           rect = HeaderRect(parent);
           OlClearRectangle(parent, &rect, False);
           DisplayWM(parent, &rect);
	}
/*
 * [un]grab keys and buttons???
 */
           }
   else if ((event-> xproperty.atom == XA_OL_WIN_ATTR(display)) ||
		(event->xproperty.atom == MWM_HINTS))
           {
	   unsigned long state = HasPushpin(parent) ?
				wmstep->decorations & (WMPushpin | WMPinIn) :
				0;
	   unsigned long newstate = 0;
	   Boolean	newheader,
			prevheader= (wmstep-> decorations & (WMHeader|WMResizable));
	   int		childwidth;
	   int		childheight;
	   unsigned long save_decorations = wmstep->decorations;

           GetAllDecorations(wmstep, display, window);
	   newheader = wmstep->decorations & (WMHeader | WMResizable);
           wmstep-> metrics = GetMetrics(parent);
	   if (newheader != prevheader) {
		/* Resize wmstep widget, adjust child wm->core.x, wm->core.y.
		 * The upper left corner of the header (wm->core.x,y) will
		 * stay at the same location;  therefore, send a synthetic
		 * configurenotify to the client alerting the window
		 * position change.  The reason we have to check to see if
		 * the header OR resizecorners were added or deleted is
		 * because in motif mode, the resize corners are on a border
		 * that is usually thicker than the normal border, and
		 * this would require the parent being resized.
		 */ 
		int newparentwidth;
		int newparentheight;

		childwidth = parent->wmstep.child->core.width;
		childheight = parent->wmstep.child->core.height;


		if (currentGUI == OL_OPENLOOK_GUI) {
			newparentwidth = ParentWidth(parent, childwidth);
			newparentheight = ParentHeight(parent, childheight);
		}
		else {
			newparentwidth = MParentWidth(parent, childwidth);
			newparentheight = MParentHeight(parent, childheight);
		}
		XtResizeWidget((Widget)parent, newparentwidth ,
					 newparentheight , (Dimension)0);

		if (currentGUI == OL_OPENLOOK_GUI)
			XtMoveWidget(wmstep->child, OriginX(parent),
							 OriginY(parent));
		else
			XtMoveWidget(wmstep->child, MOriginX(parent),
							 MOriginY(parent));
		SendWindowMovedEvent(parent);
	   }

           wmstep-> metrics = GetMetrics(parent);

	   /* Check the previous decoration values, see if they match.
	    * If not, try the significant discrepencies:  If the
	    * IconWindowMapped and the IconWindowReparented flags are
	    * set, and there is an icon window, then the icon window
	    * must be mapped - be sure to reset these flags for
	    * later use.
	    */
	   if (wmstep->decorations != save_decorations && IsIconic(parent)) {
		if (wmstep->xwmhints->icon_window) {
			if (save_decorations & WMIconWindowReparented)
				wmstep->decorations |= WMIconWindowReparented;
			if (save_decorations & WMIconWindowMapped)
				wmstep->decorations |= WMIconWindowMapped;
		}
	   }

	   if (currentGUI == OL_OPENLOOK_GUI) {
	     if ( (XtIsRealized((Widget)parent) == TRUE) && 
					(wmstep->decorations & WMPushpin)) {
		if ( (newstate = wmstep->decorations & (WMPushpin | WMPinIn))
				!= state)
			SetPushpinState(parent, newstate, TRUE);
             }
	   } /* if OPEN LOOK */

	   if (!(IsIconic(parent)))
		DisplayWM(parent, NULL);

	} /* end if (atom == XA_OL_WIN_ATTR(display)) */
   else if ( (event-> xproperty.atom == (ol_decor_add =
					XA_OL_DECOR_ADD(display))) ||
			 (event-> xproperty.atom == XA_OL_DECOR_DEL(display)) )
           {
	   Boolean	newheader,
			prevheader =  (wmstep-> decorations &
						(WMHeader|WMResizable));
	   int		childwidth;
	   int		childheight;

	   if (event-> xproperty.atom == ol_decor_add)
           	GetAddDecorations(wmstep, display, window);
	   else
           	GetDelDecorations(wmstep, display, window);
           wmstep-> metrics = GetMetrics(parent);
	   newheader = wmstep->decorations & (WMHeader|WMResizable);
	   if (newheader != prevheader) {
		/* Resize wmstep widget, adjust child wm->core.x, wm->core.y.
		 * The upper left corner of the header (wm->core.x,y) will
		 * stay at the same location;  therefore, send a synthetic
		 * configurenotify to the client alerting the window
		 * position change.
		 */ 
		int newparentwidth;
		int newparentheight;

		childwidth = parent->wmstep.child->core.width;
		childheight = parent->wmstep.child->core.height;

		if (currentGUI == OL_OPENLOOK_GUI) {
			newparentwidth = ParentWidth(parent, childwidth);
			newparentheight = ParentHeight(parent, childheight);
		}
		else {
			newparentwidth = MParentWidth(parent, childwidth);
			newparentheight = MParentHeight(parent, childheight);
		}
		XtResizeWidget((Widget)parent, newparentwidth,
					 newparentheight, (Dimension)0);
		if (currentGUI == OL_OPENLOOK_GUI)
			XtMoveWidget(wmstep->child, OriginX(parent),
							 OriginY(parent));
		else
			XtMoveWidget(wmstep->child, MOriginX(parent),
							 MOriginY(parent));
		SendWindowMovedEvent(parent);
	   }
           DisplayWM(parent, NULL);
           }
   else if (event-> xproperty.atom == XA_OL_WIN_COLORS(display))  {
           SetColors(parent, display, window);
}
   else if (event-> xproperty.atom == WM_COLORMAP_WINDOWS) {
		/* First of all, this must be a toplevel window;
		 * we'll read the property and change as needed.
		 * For now: pick the easy way out; dump the old, read in
		 * new.
		 */
		if (wmstep->num_cmap_windows > 0) {
			RemoveWMColormaps(parent);
		}
		/* Now we have removed all old contexts from the previous
		 * WM_COLORMAP_WINDOWS property that existed, and freed up
		 * the context data fields in the step widget;  go ahead
		 * and get the new stuff.
		 */
		GetColormapWindows(parent, display, wmstep->window);
		if (wmstep->is_current)
			WMInstallColormaps(parent);
	}
   }
   else
	if (event-> xproperty.state == PropertyDelete) {
		if (event-> xproperty.atom == XA_OL_WIN_COLORS(display))  {
			/* Adjust colors back to the "common" colors */
			wmstep->olwincolors = 0;
			wmstep->foreground_pixel = wmrcs->foregroundColor;
			if (!wmrcs->parentRelative) {
				parent->core.background_pixel =
						 wmrcs->backgroundColor;
				XSetWindowBackground(XtDisplay(parent),
				   XtWindow(parent), wmrcs->backgroundColor);
			}
			CreateGC(parent, wmstep, TRUE);
        		XClearArea(XtDisplay(parent), XtWindow(parent), 0, 0,
							 0, 0, False);
			DisplayWM(parent, NULL);
		}
	}
} /* end of ClientPropertyChange */

extern void
RemoveWMColormaps OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
int i;
Window window = wmstep->window;
Display	*display = XtDisplay((Widget) wm);

	if (wmstep->num_cmap_windows > 0) {
	   for (i=0; i < wmstep->num_cmap_windows; i++) {
		if (IsWMStepChild(wmstep->colormap_windows[i]) < 0) {
		    ReworkColormapInfo(display,
			wmstep->colormap_windows[i],
			DESTROYCMAP);
		}
	    }
	   XDeleteContext(display, window, 
			wm_unique_context);

	   if (wmstep->subwindow_data != NULL) {
			XtFree(wmstep->subwindow_data);
			wmstep->subwindow_data = NULL;
	   }
	   if (wmstep->colormap_windows != NULL) {
		XtFree(wmstep->colormap_windows);
		wmstep->colormap_windows = NULL;
	   }
	   wmstep->num_cmap_windows = 0;
	}
} /* RemoveWMColormaps */

/*
 * ClientColormapChange
 * event handler added to client window widget in Initialize().
 * client_data = client widgets parent, the WMStepWidget.
 *
 * - CurrentColormapWindow is the window that NOW has the Colormap Focus;
 * - CurrentColormap is a list of currently installed colormaps.
 */

static void
ClientColormapChange OLARGLIST((w, client_data, event, cont_to_disp))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLARG(XEvent *, event)
OLGRA(Boolean *, cont_to_disp)
{
WMStepWidget parent = (WMStepWidget)client_data;
Display *    display = XtDisplay(parent);
WMStepPart * wmstep  = &parent-> wmstep;
XtPointer	data;

if (event-> xcolormap.new)
   {
   if (event-> xcolormap.colormap == None)
      wmstep-> window_attributes.colormap = DefaultColormapOfScreen(XtScreen(w));
   else
      wmstep-> window_attributes.colormap = event-> xcolormap.colormap;

   /* CurrentColormapWindow is the window that NOW has the Colormap Focus */

	if (wmstep->is_current)
      {

	/* The list of currently installed maps may have to be altered-
	 * it doesn't matter, so long as the correct maps get installed
	 * if this client has the colormap focus.
	 */

	WMInstallColormaps(parent);

   } /* end if (the current colormap window == the one doing an install */
 } /* end (if it's new colormap being installed */
} /* ClientColormapChange */

#if !defined(LEADER)
#define LEADER(wm) (wm)->wmstep.xwmhints->window_group
#endif

/*
 * ClientFocusChange
 * - Event handler set on client window widget;
 * Looks for FocusIn or FocusOut events.
 * Update wmstep-> protocols field with HasFocus flag (either | it in
 * if getting focus or &=~ it out if losing it); then call DisplayWM() to
 * (un)highlight title bar if click-to-tpe (or other if real-estate).
 */

void
ClientFocusChange OLARGLIST((w, client_data, event, cont_to_disp))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLARG(XEvent *, event)
OLGRA(Boolean *, cont_to_disp)
{
WMStepWidget parent; /* = (WMStepWidget)client_data; */
Display *    display;
WMStepPart * wmstep;
XRectangle   rect;
Window TempWindow;
int	togglecurrent = 0;
int i;

/* If client_data == NULL, then this is a focus event for the root window.
 * Otherwise it is a focus event for a real client
 */

if (client_data != NULL) {
	parent = (WMStepWidget)client_data;
	wmstep = &parent->wmstep;
	if (wmstep->size == WMWITHDRAWN && event->type == FocusIn)
		return;
}
	display = XtDisplay(w);
        
FPRINTF((stderr, 
        "focus event type = %s send event = %d mode = %d detail = %d\n",
         event-> type == FocusOut ? "out" : "in",
         event-> xfocus.send_event, event-> xfocus.mode, event-> xfocus.detail));

/* Ignore the event if ev->xfocus.detail == either NotifyDetailNone or
 * NotifyNormal.
 */
/*
fprintf(stderr,"ClientFocusChange: event=%s\n", event->type==FocusIn ?
	"FocusIN" : "FocusOUT");
 */
#ifdef PFOCUS
if (WMCombinedMenu->MenuPane == (Widget)client_data)
	fprintf(stderr,"ChangeFocus: got event for MenuPane\n");
if (WMCombinedMenu->CascadePane == (Widget)client_data)
	fprintf(stderr,"ChangeFocus: got event for CascadePane\n");
if (event->xfocus.window == XtWindow(Frame))
	fprintf(stderr,"Focus event for root\n");
else
	fprintf(stderr,"Focus event for window=%lu\n",event->xfocus.window);
if (event->type == FocusIn)
	fprintf(stderr,"Event is a FOCUSIN\n");
  else 
	if (event->type == FocusOut)
		fprintf(stderr,"Event is a FOCUSOUT\n");
if (event->xfocus.detail == NotifyDetailNone)
fprintf(stderr,"It's a NotifyDetailNone\n");
if (event->xfocus.detail == NotifyInferior)
fprintf(stderr,"It's a NotifyInferior\n");
if (event->xfocus.detail == NotifyPointerRoot)
fprintf(stderr,"It's a NotifyPointerRoot\n");
if (event->xfocus.detail == NotifyPointer)
fprintf(stderr,"It's a NotifyPointer\n");
if (event->xfocus.detail == NotifyNonlinear)
fprintf(stderr,"It's a NotifyNonlinear\n");
if (event->xfocus.detail == NotifyNonlinearVirtual)
fprintf(stderr,"It's a NotifyNonlinearVirtual\n");
if (event->xfocus.detail == NotifyVirtual)
fprintf(stderr,"It's a NotifyVirtual\n");
if (event->xfocus.detail == NotifyAncestor)
fprintf(stderr,"It's a NotifyAncestor\n");
if (event->xfocus.mode == NotifyNormal)
	fprintf(stderr," *** And mode is NotifyNormal\n");
else
  if (event->xfocus.mode == NotifyGrab)
	fprintf(stderr," *** And mode is NotifyGrab\n");
    else if (event->xfocus.mode == NotifyUngrab)
	fprintf(stderr," *** And mode is NotifyUngrab\n");
#endif

if (event-> xfocus.detail == NotifyInferior ||
/* 
 * never see this happen...is it needed???
    event-> xfocus.detail == NotifyPointerRoot ||
 *
 */
    event-> xfocus.detail == NotifyDetailNone ||
    (event-> xfocus.mode   != NotifyNormal && event->xfocus.mode !=
	NotifyWhileGrabbed)) {
#ifdef PFOCUS
	fprintf(stderr,"Got new one in FocusChange...!!\n");
#endif
	/* If there is a menu posted, simply continue operating...
	 * This is handling a very simple case, the combined limited
	 * menu with the cascade shell.  But it can be easily modified
	 * to handle special menus.
	 */
	if (NumMenushellsPosted != 0) {
		if (menushells_posted[0] == WMCombinedMenu->MenuShell){
			/* In all likelihood, the limited menu was posted, the
		 	 * submenu (cascade menu) popped up, and then the
		 	 * cascade menu was dismissed, leaving the limited menu
		 	 * still posted; set focus to it.
		 	 */
                        OlSetInputFocus(WMCombinedMenu->menu,
                                        RevertToNone, CurrentTime);
		}
		return;
	}
	if ((event->xfocus.mode == NotifyNormal 
		/* Inserted by mlp - NotifyWhileGrabbed */
		|| event->xfocus.mode == NotifyWhileGrabbed) &&
				event->xfocus.detail == NotifyDetailNone) {
		if (event->xfocus.window == XtWindow(Frame)) {
			WMStepWidget wm;
			Window	focus_win;
			int	revert_to;
			XGetInputFocus(display, &focus_win, &revert_to);
			if (focus_win != None && focus_win !=
				  PointerRoot &&
				  XtWindowToWidget(display, focus_win) == NULL) {
				/* Could be an oerride_redirect - I know it isn't
				 * the root window, because we do have a widget
				 * on that.
				 */
				return;
			}
			/* Reset focus to another window */
			if ((wm = NextFocus((WMStepWidget)NULL, True))
							!= NULL) {
				SetFocus((Widget)wm, GainFocus, 1);
			}
			else {
				SetFocus(Frame,GainFocus, 0);
			}
		}
	} /* end if (focus detail == NotifyDetailNone) */
    }
	else if (event->xfocus.window == XtWindow(Frame))
		/* Should not have gotten this one... */
#if !defined(I18N)
		fprintf(stderr,"Win. Mgr: ClientFocusChange() for frame, skip\n");
#else
		OlVaDisplayWarningMsg(XtDisplay(w), OleNfocus, OleTfocusNever,
OleCOlClientOlwmMsgs, OleMfocus_focusNever, NULL);
#endif
else
   {
   switch (event-> xfocus.type)
      {
      case FocusIn:
	 /* Set wmstep's focus field to the FOCUS COUNTER - this is the
	  * count of how many times focus has changed.  Sort of an integrity
	  * count, because at any one time, wm_focus_count must be in
	  * someones wmstep->focus field in the window_list->
	  * Turn on HasFocus flag in wmstep's protocols field.
	  * Change the colormap focus window, install new cmap if needed.
	  */

         /*wmstep-> focus = ++wm_focus_count;*/
	if (wmrcs->pointerFocus == False && event->xfocus.detail ==
						NotifyPointer)
		/* Ignore - this tells us where the pointer is.  Not
		 * a factor in click-to-type
		 */
		return;
	 if (IsIconic(parent))
		/* This must be an "old" focus in event for a window
		 * just turned iconic, because icons can't really take
		 * focus - assume (we must) that it will be followed
		 * by a focus out.
		 */
		return;
         wmstep->protocols |= HasFocus;

	/* mlp - In SetFocus(), we set the CurrentColormapWindow to
	 * the step widget window; here we set it to the client
	 * widget window; if this gets called immediately after the
	 * Colormaps are installed in SetFocus(), then the "SAME"
	 * colormaps should get installed, and nothing should happen
	 * (WMInstallColormaps() shouldn't have to install anything)
	 */
	if (CurrentColormapWindow != XtWindow(w)) {
         CurrentColormapWindow = XtWindow(w);

	/* Take care of installing colormaps, if necessary. */
	 WMInstallColormaps(parent);
	}

	/* Make it current in all cases if get FocusIn.  In Motif mode,
	 * this will cause a redisplay of the decoration set for both the
	 * new and the old one that previously had focus.
	 */
	if (wmstep->is_current == False)
		SetCurrent(parent);
		/* Don't redo the displaying - keep toggle off */
#if defined(CHECKFOCUS)
		CurrentFocusWindow = parent;
		if ( (TempWindow = LEADER(parent)) == wmstep->window)
			CurrentFocusApplication = parent;
		else
			if ( (i = IsWMStepChild(TempWindow)) >= 0 ) {
				CurrentFocusApplication =
					 (WMStepWidget)wmstep_kids[i];
			}
			else
				CurrentFocusApplication = parent;
#endif

         break;
      case FocusOut:
	if (wmrcs->pointerFocus == False && event->xfocus.detail ==
						NotifyPointer)
		/* Ignore - this tells us where the pointer is.  Not
		 * a factor in click-to-type
		 */
		return;
	 /* Losing focus? turn off HasFocus flag in wmstep->protocols.
	  * Don't call SetCurrent() to turn off this being the current
	  * widget.  Assume that there was a reason for it losing focus.
	  * If someone else gets focus, then we'll make it current
	  * anyway.
	  */
         wmstep-> protocols &= ~HasFocus;
	/* We can't turn this off now, because if olwm is posting a menu for
	 * it, then we want it highlighted; however, someone else (another
	 * window)  may be posting a menu in which case we will want to
	 * unhighlight it, but possibly rehighlight it later!
	 */
	if (wmstep->is_current) {
		/* Are we posting a menu?  If so, is it for this client?
		 */
		if (NumMenushellsPosted) {
			/* look at the first one */
			if (menushells_posted[0] ==
					WMCombinedMenu->MenuShell) {
				if (WMCombinedMenu->w != (Widget)parent) {
					/* No match, un-current */
					wmstep->is_current = 0;
					togglecurrent++;
				}
			}
			else {
				Global_Menu_Info *gmi_ptr = global_menu_info;
				while(gmi_ptr) {
					if (gmi_ptr->MenuShell ==
						    menushells_posted[0])
						break;
					else
					   gmi_ptr = gmi_ptr->next;
				} /* while */
				if (gmi_ptr) {
					if (gmi_ptr->w != (Widget)parent) {
						wmstep->is_current = 0;
						togglecurrent++;
					}
				}
			} /* else */
		} /* if NumMenushellsPosted */
		else 
                    if(wmrcs->pointerFocus==False)      {
			wmstep->is_current = 0;
			togglecurrent++;
		    }
	} /* if current */


#if defined(CHECKFOCUS)
	if (CurrentFocusWindow == parent) {
		PreviousFocusWindow = CurrentFocusWindow;
		PreviousFocusApplication = CurrentFocusApplication;
		CurrentFocusWindow = (WMStepWidget)NULL;
		CurrentFocusApplication = (WMStepWidget) NULL;
	}
#endif
         break;
      default:
         break;
      } /* end switch (event.focus.type) */

   /* After adjusting internal flags, clear header (title bar) and call
    * DisplayWM()- that function uses a combination of the flags just
    * set and the input focus type (real-estate, click-to-type) to provide
    * feedback for either getting or losing focus (e.g., highlight or
    * unhighlight the title bar.
    */
   if (currentGUI == OL_OPENLOOK_GUI) {
   rect = HeaderRect(parent);
   if (wmrcs->pointerFocus) {
	/* For real-estate based input focus, there is an extra horizontal
	 * line drawn at the top and bottom of the header; to clear out
	 * the line at the top during FocusOut events, you must make the
	 * rectangle returned by HeaderRect() slightly higher.
	 */
	int diff = (int)LineWidY(parent);

	rect.y -=  diff;
	rect.height += diff;
   }
   switch (event-> xfocus.type) {
	case FocusIn:
		if ( (Header(parent)) && wmrcs->pointerFocus) {
			DrawBN(parent);	
			break;
		}
	/* Fall through */
	case FocusOut:
   		if (!(Header(parent)))
			break;
   		if (wmrcs->pointerFocus) {
			/* For real-estate based input focus, there is an extra
			 * horizontal line drawn at the top and bottom of the
			 * header; to clear out the line at the top during
			 * FocusOut events, you must make the rectangle
			 * returned by HeaderRect() slightly higher.
	 		 */
			int diff = (int)LineWidY(parent);

			rect.y -=  diff;
			rect.height += diff;
   		}
		if (wmrcs->pointerFocus || togglecurrent) {
			OlClearRectangle(parent, &rect, False);
			DisplayWM(parent, &rect);
		}
		break;
	default:
		break;
	} /* end switch */
	} /* Open Look GUI */
	else
		/* Motif - check togglecurrent */
		if (togglecurrent)
			DisplayWM(parent, NULL);
   } /* end else */

/* ** For future reference:
 * For motif,  when we get a focus out, it is usually after focus was
 * set to it by us, and therefore we would have called SetCurrent()
 * which would highlight the border;  but if we were to get focus
 * by getting a click inside the pane of a client, then we would
 * set the current here and call DisplayWM() - see above.
 */

} /* end of ClientFocusChange */

/*
 * FocusCount(): return the current focus counter.
 */

extern unsigned long
FocusCount()
{
return (++wm_focus_count);
}

/*
 * EventIn
 * - Called by (for example) ClientNonMaskable() when it receives a message
 * that a help key was pressed - it's sent an event (ClientMessage) with the
 * coordinates of the pointer. If the coordinates weren't on the window menu or
 * a menu button, then EventIn returns the window decoration that it is on.
 */

extern WMPiece
EventIn OLARGLIST((wm, x, y))
	OLARG(WMStepWidget,	wm)
	OLARG(Position,		x)
	OLGRA(Position,		y)
{
int width   = wm-> core.width;
int height  = wm-> core.height;
int XCorner = CornerX(wm);
int YCorner = CornerY(wm);
int xCorner = Cornerx(wm);
int yCorner = Cornery(wm);
int xborderwidth, yborderwidth, title_offset;
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);

WMPiece retval = WM_NULL;
int limit = Offset(wm);
int hasborder = wmstep->decorations & WMBorder;

/* If in iconic state, simply  return the banner part - can be
 * used in moving or selecting an icon (or double click)
 */
if (IsIconic(wm))
	return(WM_BANNER);

if (currentGUI == OL_MOTIF_GUI) {
	int butwidth = wm->wmstep.metrics->motifButtonWidth;
	int butheight = wm->wmstep.metrics->motifButtonHeight;

	if (hasborder) {
	  if (Resizable(wm)) {
		xborderwidth = wm->wmstep.metrics->motifHResizeBorderWidth;
		yborderwidth = wm->wmstep.metrics->motifVResizeBorderWidth;
	  }
	  else {
		xborderwidth = wm->wmstep.metrics->motifHNResizeBorderWidth;
		yborderwidth = wm->wmstep.metrics->motifVNResizeBorderWidth;
	  }
	} /* hasborder */
	else
		xborderwidth = yborderwidth = wmstep->prev.border_width;

	if (x > xborderwidth && x < width - xborderwidth &&
		y > yborderwidth && y < yborderwidth +
			 wm->wmstep.metrics->motifButtonHeight) {
		 /* in title bar */
		retval = WM_BANNER;
		if (x < xborderwidth + butwidth) {
			if (wmstep->decorations & WMMenuButton)
				retval = WM_MM;
		}
		else
			if (wmstep->decorations & WMMinimize &&
			    wmstep->decorations & WMMaximize) {
				/* 2 buttons */
				if (x < width - xborderwidth - 2 * butwidth)
					retval = WM_BANNER;
				else if (x < width - xborderwidth - butwidth)
					retval = WM_MINB;
				else
					retval = WM_MAXB;
			} /* 2 buttons */
			else
			  if (wmstep->decorations & WMMinimize ||
			    wmstep->decorations & WMMaximize) {
				/* one button */
				if (x > width - xborderwidth - butwidth) {
				   if (wmstep->decorations & WMMinimize)
					retval = WM_MINB;
				   else
					retval = WM_MAXB;
				} /* x < */
			   } /* one button */
			/* anything else defaults to title bar */
	} /* if in title bar */
	else /* are we in a resize corner, or on an edge */
		if (hasborder) {
			if (wmstep->decorations & WMResizable) {
				retval = WM_NULL;
				if (x < xborderwidth && (int)y < 
					    (int)(yborderwidth + butheight) ||
					 (x > xborderwidth &&
					(int)x < (int)(xborderwidth + butwidth)
					&& y < yborderwidth))
						retval = WM_NW;

				else
				  if (  ((x < xborderwidth) && (int)y >
		 (int)(wm->core.height - (butheight + yborderwidth))) ||
					  (x > xborderwidth &&
					  x < (xborderwidth + butwidth) &&
				(int)y > (int)(wm->core.height - yborderwidth))) {
						retval = WM_SW;
	}

				else
			if( ( ((int)x > (int)(wm->core.width - xborderwidth)) &&
					(int)y < (yborderwidth + butheight)) ||
			((int)x > (int)(wm->core.width - xborderwidth -
					butwidth) && (y < yborderwidth) ) ) {
						retval = WM_NE;
			}

				else

			if(( ((int)x > (int)(wm->core.width - xborderwidth)) &&
					  y > (int)(wm->core.height -
						(yborderwidth + butheight))) ||
			((int)x > (int)(wm->core.width - xborderwidth -
					butwidth) &&
			(int)y > (int)(wm->core.height - yborderwidth)) )
						retval = WM_SE;
				else
		if ((int)x > (int)(xborderwidth + butwidth) &&
			(int)x < (int)(wm->core.width-xborderwidth-butwidth) &&
				  (y < yborderwidth))
					retval = WM_T;
			else
		if ((int)x > (int)(xborderwidth + butwidth) &&
			(int)x < (int)(wm->core.width-xborderwidth-butwidth) &&
			(int)y > (int)(wm->core.height - yborderwidth))
					retval = WM_B;

				else if (y > (yborderwidth + butheight)&& 
				  (int)y < (int)(wm->core.height - yborderwidth -
					butheight)) {
					if (x < xborderwidth)
						retval = WM_L;
					else if ((int)x > (int)(wm->core.width - 
							xborderwidth))
						retval = WM_R;	
				}
			} /* resizable */
		} /* hasborder */
		/* else we will return an edge -
		 * just to indicate that we got a buttonpress, and
		 * we should give it focus (shouldn't interfere elsewhere)
		 */
		else
/* mlp - try NULL for this case 
			retval = WM_L;
 */
			retval = WM_NULL;
		return(retval);
}
else { /* Open Look */
if (x > xCorner && x < width - xCorner &&
    y > yCorner && y < yCorner + BannerHt(wm))
   {
   /* You are somewhere in the title bar */
   if (HasPushpin(wm))
	limit += ppWidth;
   else
	if (wmstep->decorations & WMHasFullMenu)
		limit += mmWidth;
   if (x >= Offset(wm) && x <= limit)
      /* You are in either the pushpin or the window menu mark - this assumes that
       * the pin or menu mark span the title bar vertically, because no y coords.
       * are checked.
       */
      if (HasPushpin(wm))
         retval = WM_PP;
      else
	 if (wmstep->decorations & WMHasFullMenu)
            retval = WM_MM;
         else
            retval = WM_BANNER;
   else
      retval = WM_BANNER;
   }
else
   {
   if (x <= XCorner)
      {
      if (y <= yCorner || (x <= xCorner && y <= YCorner) )
         retval = WM_NW;
      else
         if ( (y > height - yCorner) || (x <= xCorner && y >= height - YCorner))
            retval = WM_SW;
         else
		if (x <= xCorner)
			retval = WM_L;
		/* else leave retval as WM_NULL */
      }
   else
	/* The right side */
      if (x >= width - XCorner)
         {
         if (y <= yCorner || (x >= width - xCorner && y < YCorner) )
            retval = WM_NE;
         else
            if (y >= height - yCorner || (x >= width - xCorner && y < YCorner) )
               retval = WM_SE;
            else
		if (x >= width - xCorner)
			retval = WM_R;
		/* else == WM_NULL */
         }
      else
         if (y >= height - yCorner)
		retval = WM_B;
	else
		if (y <= yCorner)
			retval = WM_T;
   }
} /* Open Look */
return (retval);

} /* end of EventIn */


/*
 * _parse
 *
 * This function is used to _parse a given string using a given delimiter.
 * It can be used in lieu of strtok(3) when it is desireable to "find"
 * null string tokens.  That is if the delimiter is ':' and the string
 * "Test::string" is parsed, then this routine would return the tokens
 * "Test", NULL, and "string" whereas strtok returns "Test" and "string".
 *
 * Input:     char * string
 *            int    delimiter
 *
 * Output:    Pointer to the next token.
 *
 * Note: This routine does no analysis to determine when it has exhausted
 *       the original string.
 */
static char * _parse(string, delimiter)
char * string;
int    delimiter;
{
static   char * x = NULL;
register char * y;

if (string) x = string;
string = x;

if (x == NULL || *x == '\0')
   return NULL;
else
   {
   if ((y = strchr(x, delimiter)) == NULL)
      x = NULL;
   else
      {
      *y = '\0';
      x = y + 1;
      }
   return string;
   }

} /* end of _parse */


typedef struct SearchPath
   {
   char * path;
   struct SearchPath * nextPath;
   } SearchPath;

/*
 * ParseSearchPath
 * ---------------
 * The ParseSearchPath function parses a path-style string and generates
 * a SearchPath list.  The SearchPath list is the list of all of the
 * paths contained in the given path.
 */
static SearchPath * ParseSearchPath(path)
char * path;
{
char * temp;
char * pathplus;
SearchPath * new;
SearchPath * current = NULL;
SearchPath * pathlist = NULL;

pathplus = XtMalloc((path ? strlen(path) : 0) + 2);
if (path != NULL)
   {
   (void) strcpy(pathplus, path);
   (void) strcat(pathplus, ":");
   }
else
   (void) strcpy(pathplus, ":");

for (temp = _parse(pathplus, ':'); temp != NULL; temp = _parse(NULL, ':'))
   {
   new = (SearchPath *) XtMalloc(sizeof(SearchPath));
   new-> path = XtNewString(temp);
   new-> nextPath = NULL;

   if (pathlist == NULL)
      pathlist = new;
   else
      current-> nextPath = new;

   current = new;
   }

return (pathlist);

} /* end of ParseSearchPath */

#include <unistd.h>
#include <sys/stat.h>
static Bool
HelpFilePred OLARGLIST((filename))
OLGRA (String, filename)
{
#if defined(SVR4_0) || defined(SVR4)
struct stat                   status;

	return ((access(filename, R_OK|X_OK) == 0) &&
           (stat(filename, &status) == 0) &&
           ((status.st_mode & S_IFDIR) == S_IFDIR));
#else
   return (access(filename, R_OK|X_OK) == 0);
#endif
}

/*
 * FindHelpFile
 * Return the localized help file based on string "filename" for widget w.
 * Set helpDirectory and use OlFindHelpFile.  MakePath (pretty
 * self-explanatory) is used by this routine only.
 * (scottn 3/14/91)
 */

#define ENVHELPPATH     "OLWMHELPPATH"

static char *
FindHelpFile OLARGLIST((w, filename))
OLARG(Widget, w)
OLGRA(char *, filename)
{
  static SearchPath *StartHelpPath = NULL;
  static char * envPath = NULL;
  char *fullfilename = NULL;
  SearchPath *q = NULL;
  int j;
  static int firsttime = 1;
  Arg arg;
  Widget searchWidget = w;
  int num_arg = 0;
  
  if (firsttime) {
    envPath = getenv(ENVHELPPATH);
  
    if (envPath != NULL) 
      StartHelpPath = ParseSearchPath(envPath);
  
    if (filename != NULL) {
      for (q = StartHelpPath; q != NULL; q = q-> nextPath)
	{
	  fullfilename = MakePath(q-> path, filename);
	  if (access(fullfilename, 4) == 0) 
	    break;
	  else {
	    XtFree(fullfilename);
	    fullfilename = NULL;
	  }
	}
    }
      
    if (fullfilename) {
      char *tmp = strrchr(fullfilename, '/');

      *tmp = '\0';
      XtSetArg(arg, XtNhelpDirectory, fullfilename);
      num_arg = 1;
    }
    else if (currentGUI == OL_MOTIF_GUI) {
        char *tmp;

        tmp = XtResolvePathname(XtDisplayOfObject(searchWidget), "help",
                                    APPLICATION_CLASS, NULL, (String)NULL,
                                    (Substitution)NULL, (Cardinal)0,
                                    (XtFilePredicate) HelpFilePred);
        if (tmp && *tmp && strlen(tmp) < PATH_MAX) {
                XtSetArg(arg, XtNhelpDirectory, tmp);
                num_arg = 1;
        }
    }
    if (num_arg)
        OlSetApplicationValues(searchWidget, &arg, num_arg);

    firsttime = 0;
  }
  
  return(OlFindHelpFile(searchWidget, filename));

} /* end of FindHelpFile */

static char *
MakePath OLARGLIST((dir, base))
OLARG(char *, dir)
OLGRA(char *, base)
{
  char * path;
  
  if (dir != NULL && dir[0] != '\0')
    {
      path = malloc((unsigned)(strlen(dir) + strlen(base) + 2));
      (void) strcpy(path, dir);
      (void) strcat(path, "/");
      (void) strcat(path, base);
    }
  else
    {
      path = malloc((unsigned)(strlen(base) + 1));
      (void) strcpy(path, base);
    }
  
  return (path);
  
} /* end of MakePath */


/*
 * Button Press on an icon
 */
void
IconButtonPress OLARGLIST((w, client_data, event, cont_to_disp))
OLARG(Widget,		w)
OLARG(XtPointer,	client_data)
OLARG(XEvent *,		event)
OLGRA(Boolean *,	cont_to_disp)
{
WMStepWidget		wm    = (WMStepWidget)client_data;
WMPiece			piece = WM_BANNER;
OlVirtualEventRec	ve;
Display			*display = XtDisplay(wm);
WMStepPart		*wmstep = (WMStepPart *)&(wm->wmstep);

DragMask = event-> xbutton.state | 1<<(event-> xbutton.button+7);

OlLookupInputEvent(w, event, &ve, OL_DEFAULT_IE);
switch(ve.virtual_name)
   {
    case OL_SELECT:
    case OL_CONSTRAIN:
	if (event->type == ButtonPress) {
		if (!(FocusSwitchOK(wm))) {
			XBell(XtDisplay(w), 0);
			}
		else
			Select(wm, event, piece);
	}
      	break;
   case OL_MENU:
      Menu(wm, event, piece);
      break;
   default:
      OlReplayBtnEvent(w, NULL, event);
      break;
   }

}

#ifdef RAW
void
IconExposeRaw OLARGLIST((w, client_data, event, cont_to_disp))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLARG(XEvent *, event)
OLGRA(Boolean *, cont_to_disp)
{
WMStepWidget wm = (WMStepWidget)client_data;
	if (dispatch_expose == True)
		*cont_to_disp = True;
	else {
		if (w == dispatch_widget) {
			/* matching widgets */
			*cont_to_disp = False;
			if (event->xexpose.count == 0)
				/* no more */
				dispatch_expose = True;
		}
		else
			/* no match, continue */
			*cont_to_disp = True;
	}
} /* IconExposeRaw */
#endif

void
IconExpose OLARGLIST((w, client_data, event, cont_to_disp))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLARG(XEvent *, event)
OLGRA(Boolean *, cont_to_disp)
{
WMStepWidget wm = (WMStepWidget)client_data;
	if (event->xexpose.count == 0)
		DisplayWM(wm, NULL);
} /* IconExpose */

void
CheckPostedMenu OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
	if (NumMenushellsPosted > 0 && (WMCombinedMenu->w == (Widget)wm) ) {
		_OlPopdownCascade(_OlRootOfMenuStack((Widget)wm), False);
	}
}

void
IconEnterLeave OLARGLIST((w, client_data, event, cont_to_disp))
OLARG(Widget, w)
OLARG(XtPointer, client_data)
OLARG(XEvent *, event)
OLGRA(Boolean *, cont_to_disp)
{
WMStepWidget wm = (WMStepWidget)client_data;
WMStepPart * wmstep = &wm-> wmstep;
XFocusChangeEvent focus_event;

switch (event-> type)
{
   case EnterNotify:

      if (wmrcs->pointerFocus && !NumMenushellsPosted)
      {
         switch(event-> xcrossing.detail)
            {
            case NotifyAncestor:
#ifdef DEBUG
fprintf(stderr,"\tEnterNotify had detail = NotifyAncestor\n");
#endif
               SetFocus((Widget)wm, GainFocus, 1);
               break;
            case NotifyVirtual:
#ifdef DEBUG
fprintf(stderr,"\tEnterNotify had detail = NotifyVirtual\n");
#endif
               SetFocus((Widget)wm, GainFocus, 1);
               break;
            case NotifyInferior:
#ifdef DEBUG
fprintf(stderr,"\tEnterNotify had detail = NotifyInferior\n");
#endif
               break;
            case NotifyNonlinear:
#ifdef DEBUG
fprintf(stderr,"\tEnterNotify had detail = NotifyNonlinear\n");
#endif
               SetFocus((Widget)wm, GainFocus, 1);
               break;
            case NotifyNonlinearVirtual:
#ifdef DEBUG
fprintf(stderr,"\tEnterNotify had detail = NotifyNonlinearVirtual\n");
#endif
               SetFocus((Widget)wm, GainFocus, 1);
               break;
            }
         }
      break;
   case LeaveNotify:
#ifdef DEBUG
	fprintf(stderr,"Got LeaveNotify, subwin=%ld\n",
					 event->xcrossing.subwindow);
#endif

      if (wmrcs->pointerFocus && !NumMenushellsPosted)
         {
         switch(event-> xcrossing.detail)
            {
            case NotifyAncestor:
#ifdef DEBUG
fprintf(stderr,"\tLeaveNotify had detail = NotifyAncestor\n");
#endif

	       if(wm->wmstep.is_current)	{
		  wm->wmstep.is_current = 0;
		  DisplayWM(wm,NULL);
	       }
               SetFocus(Frame, GainFocus, 0);
               break;
            case NotifyVirtual:
#ifdef DEBUG
fprintf(stderr,"\tLeaveNotify had detail = NotifyVirtual\n");
#endif

	       if(wm->wmstep.is_current)	{
		  wm->wmstep.is_current = 0;
		  DisplayWM(wm,NULL);
	       }
               SetFocus(Frame, GainFocus, 0);
               break;
            case NotifyInferior:
#ifdef DEBUG
fprintf(stderr,"\tLeaveNotify had detail = NotifyInferior\n");
#endif
               break;
            case NotifyNonlinear:
#ifdef DEBUG
fprintf(stderr,"\tLeaveNotify had detail = NotifyNonlinear\n");
#endif
               break;
            case NotifyNonlinearVirtual:
#ifdef DEBUG
fprintf(stderr,"\tLeaveNotify had detail = NotifyNonlinearVirtual\n");
#endif
               break;
            }
         }
      break;
   default:
      break;
   }
} 


