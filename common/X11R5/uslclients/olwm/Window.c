/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olwm:Window.c	1.43"
#endif

/*
 *************************************************************************
 *
 * Description:
 *	This file contains routines that manipulate windows.
 *
 ******************************file*header********************************
 */

#include <stdio.h>

#include "X11/IntrinsicP.h"
#include <X11/Xatom.h>
#include <Xol/OpenLookP.h>
#include <Xol/DynamicP.h>
#include <Xol/OlCursors.h>
#include <Xol/VendorI.h>
#include <wm.h>
#include <WMStepP.h>
#include <Extern.h>
#include <signal.h>
#include <X11/cursorfont.h>
/* Put this here for now */
#define OUTLINE_WIDTH 2

#define ACTIVELABEL

/*
 *************************************************************************
 *
 * Forward Procedure declarations
 *
 **************************forward*declarations***************************
 */

extern void	CreateMotifFeedbackWindow OL_ARGS((Display *, Screen *));
static void	HandleMoveCmd OL_ARGS((Widget, OlVirtualName, int *, int *));
extern void	DragWindowAround OL_ARGS((WMStepWidget, XEvent *,int));
static Position	Normalize OL_ARGS((Position, Dimension, Dimension,
					Dimension, Dimension, int));
extern void	MoveWindow OL_ARGS((WMStepWidget, int));
static void	PollDragWindowAround OL_ARGS((XEvent *, int));
static void	PollDragTimer OL_ARGS((XtPointer,XtIntervalId));

static void	PollResizeWindow OL_ARGS((int, WMPiece));
extern void	ResizeWindow OL_ARGS((WMStepWidget, XEvent *, WMPiece));
extern void	RestackWindows OL_ARGS((Display *, WidgetBuffer *, int));
extern void	RaiseLowerGroup OL_ARGS((WMStepWidget, int));
static void	SetLimits OL_ARGS((WMStepWidget, int, int, int,
				int, int, int, WMPiece));
extern void	SetupWindowOpCursors OL_ARGS((Display *));
extern void	TimeOut();
static void	UpdateMotifFeedbackWindow OL_ARGS((WMStepWidget, int,
								int, int));
extern int	Window_List_Position OL_ARGS((WMStepWidget));

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define MOVEINC		5	/* in pixels	*/

/* Define a timeout interval (in seconds) for the alarm clock */
#if !defined(TIMEOUT)
#define TIMEOUT	(unsigned int)15
#endif

#if !defined(TIMER_COUNT)
#define TIMER_COUNT 500
#endif

#if !defined(CONSTRAIN)
#define CONSTRAIN
#endif

/* For Motif mode */
#define OP_MOVE	1
#define OP_SIZE 2

typedef struct {
	int value1, value2;
	int prev_value1, prev_value2;
	int current_prev_value1, current_prev_value2;
	Widget w;
	Boolean timeout_set;
} DragValues; 

static DragValues	drag_values;
static Boolean		OperationTimedOut = False;

/* Motif mode: window to display dimension (if resizing) or
 * position (if moving)
 */
typedef struct _MotifFeedbackData {
	Window window;
	int	x, y;
	int	width, height;
} MotifFeedbackData;

static MotifFeedbackData mfd;

/*
 *************************************************************************
 *
 * Procedures
 *
 ***************************private*procedures****************************
 */

/*
 * DoMove - figures out how much to modify a horizontal or vertical
 * position value depending on the type of move command.
 * This routine is called by the drag routines.
 */
static void
HandleMoveCmd OLARGLIST((w, request, horiz_ret, vert_ret))
	OLARG( Widget,		w)
	OLARG( OlVirtualName,	request)
	OLARG( int *,		horiz_ret)
	OLGRA( int *,		vert_ret)
{
	typedef struct {
		OlVirtualName	name;
		int		sign;
		Boolean		use_horiz;
		Boolean		get_count;
	} MoveCmd;
	static OLconst MoveCmd	moveCmds[] = {
		{ OL_MOVERIGHT,	 1,	True,	False	},
		{ OL_MOVELEFT,	-1,	True,	False	},
		{ OL_MOVEDOWN,	 1,	False,	False	},
		{ OL_MOVEUP,	-1,	False,	False	},
		{ OL_MULTIRIGHT, 1,	True,	True	},
		{ OL_MULTILEFT,	-1,	True,	True	},
		{ OL_MULTIDOWN,	 1,	False,	True	},
		{ OL_MULTIUP,	-1,	False,	True	}
	};
	OLconst MoveCmd *	cmds = moveCmds;
	Cardinal		i;

	for (i=0; i < XtNumber(moveCmds); ++cmds, ++i)
	{
		if (request == cmds->name)
		{
			int *	to_change = (cmds->use_horiz == True ?
						horiz_ret : vert_ret);

			*to_change += MOVEINC * cmds->sign *
					(cmds->get_count == True ?
					_OlGetMultiObjectCount(w) * 2 : 1);
			break;
		}
	}
} /* END OF HandleMoveCmd() */

/*
 * DragWindowAround
 * Called from Select to move window around screen;
 * If called from Menu_Move(), event will == NULL.
 * Change: if called from Menu_Move(), also call with
 * buttondown == 0 (button is up);
 * if called from Select(), set buttondown to 1 (down).
 *
 */
extern void
DragWindowAround OLARGLIST((wm, event, buttondown))
OLARG(WMStepWidget, wm)
OLARG(XEvent *, event)
OLGRA(int, buttondown)
{
Screen	*screen = XtScreen((Widget)wm);
Display *display = XtDisplay((Widget)wm);
int	i;
int	cx     = Offset(wm);
int 	cy     = Offset(wm);
int	startonscreenx,
	endonscreenx,
	startonscreeny,
	endonscreeny;
int	showcursor;
Widget	w = (Widget)wm;
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);

showcursor = 1;

if (currentGUI == OL_MOTIF_GUI && fleur_cursor == (Cursor)0) {
	SetupWindowOpCursors(display);
}

ConstructGroupList(wm, NoAppend, wmrcs->windowLayering);

/* These are the min. and max. coords on the root win. that a window can
 * be moved to.
 */
WMminx = cx;
WMmaxx = WidthOfScreen(screen) - cx;
WMminy = cy;
WMmaxy = HeightOfScreen(screen) - cy;

if (IsIconic(wm)) {
   group_list->used = 1;
   wm->wmstep.dragX = wmstep->icon_widget->core.x;
   wm->wmstep.dragY = wmstep->icon_widget->core.y;
   w = wmstep->icon_widget;
}
else {

   /* Loop through the widget list in the group, set the drag[XY] fields
    * in each wmstep part to the CURRENT x,y position.
    */
   for (i = 0; i < group_list->used; i++)
      {
      WMStepWidget wm = (WMStepWidget)group_list->p[i];
#ifdef WITHDRAWNSTATE
      if(wm->wmstep.size == WMWITHDRAWN)
	continue;
#endif
      wm-> wmstep.dragX = wm-> core.x;
      wm-> wmstep.dragY = wm-> core.y;
      }
}

/* Let offsetx, offsety be the initial coords. of ptr for the move. */
	if (event != NULL) {
		offsetx  = event-> xbutton.x;
		offsety  = event-> xbutton.y;
#if defined(CONSTRAIN)
		ResetConstrainPoints(event);
#endif
	}
	/* Grab the pointer */
	OlGrabDragPointer(w, currentGUI == OL_OPENLOOK_GUI ?
		OlGetMoveCursor(w) :
		fleur_cursor, RootWindowOfScreen(screen));


	/* And Grab the keyboard - for use with mouseless move (or
 	 * a combination of both)...
 	 */
	if (XGrabKeyboard(display, RootWindowOfScreen(screen),False,
		 GrabModeAsync, GrabModeAsync, CurrentTime) != GrabSuccess) {
		OlUngrabDragPointer(w);
#if !defined(I18N)
		fprintf(stderr,"Window Manager Warning: Can't Grab Keyboard, Window Move Operation Failed\n");
#else
	OlVaDisplayWarningMsg(display, OleNbadKeyboard, OleTnoGrab,
	   OleCOlClientOlwmMsgs,
	OleMbadKeyboard_noGrab,
	OlGetMessage(display, NULL, 0, OleNmove, OleTmove,
	  OleCOlClientOlwmMsgs, OleMmove_move, NULL));
#endif
	}
	/*Grab the server last*/
	if (!wmrcs->moveOpaque) {
		XSync(display, False);
		XGrabServer(display);
	}
/* Should do the XFlush before you grab the server */
	XFlush(display);
	if ((event == (XEvent *) NULL) && showcursor) {
		int	startonscreenx,
			endonscreenx,
			startonscreeny,
			endonscreeny,
			warpx, warpy;
#ifdef CONSTRAIN
		Window root, child;
		int winx, winy, rootx, rooty;
		unsigned int mask;
		XButtonEvent ev;
#endif
		/* Used keyboard to get here, warp pointer to middle of
		 * client.  You really want to warp it to the middle of
		 * the part of the client now on the screen, so figure out
		 * what piece of the client is on the screen.
		 */
		startonscreenx = MAX(w->core.x,0);
		startonscreeny = MAX((int)w->core.y,0);
		endonscreenx = MIN((int)w->core.x + (int)w->core.width,
				   WidthOfScreen(screen) -1);
		endonscreeny = MIN((int)w->core.y + (int)w->core.height,
				   HeightOfScreen(screen) -1);
		warpx = (endonscreenx - startonscreenx) / 2;
		warpy = (endonscreeny - startonscreeny) / 2;

		/* Check the call out - warp to wm relative coords.
		 * If any of the left or upper parts of the decorated
		 * window is off the screen, add the absolute value of
		 * that part off the screen onto 1/2 the part that's on
		 * the screen when warping the pointer, BECAUSE the warp
		 * is relative to the upper left corner of the decorated
		 * window.
		 */

		XWarpPointer( display, None,
				 XtWindow(w),
				 0,0,
				 (unsigned int) 0, (unsigned int) 0,
				 (int)w->core.x < 0 ?
					warpx =	abs((int)w->core.x) + warpx
						: warpx,
				 (int)w->core.y < 0 ?
					warpy =	abs((int)w->core.y) + warpy
						: warpy);
		/* Now the pointer is warped to somewhere in the window
		 * pane - SIMULATE the button press being done there-
		 * set offsetx, offsety as above - where would the
		 * xbutton.x and xbutton.y be?  Since these are
		 * wm window relative coordinates (NOT root relative),
		 * then it would be at exactly warpx, warpy (I think).
		 */
		offsetx = warpx;
		offsety = warpy;
#ifdef CONSTRAIN
		XQueryPointer(display, XtWindow(w),
              			&root, &child, &rootx, &rooty,
				&winx, &winy, &mask);
		ev.x = winx;
		ev.y = winy;
		ev.x_root =  rootx;
		ev.y_root =  rooty;
		ResetConstrainPoints((XEvent *)&ev);
#endif
	}

	TargetStepwidget = wm;
	if(currentGUI == OL_MOTIF_GUI &&
		(motwmrcs->showFeedback & FEED_MOVE)) {
		XMapRaised(display, mfd.window);
		UpdateMotifFeedbackWindow(wm, wmstep->dragX, wmstep->dragY,
							OP_MOVE);
	}
	PollDragWindowAround(event,buttondown);

} /* end of DragWindowAround */

/*
 * Normalize
 * How to to use: pass either a vertical set of numbers or a
 * horizontal set of numbers.  For example, pass an x coord,
 * the max. width a window can be, the min. x coord, the max.
 * x coord, the screen width, and the horizontal pause.
 */

static Position
Normalize OLARGLIST((xy, wid_ht, min, max, scr_wid_ht, pause))
	OLARG( Position ,	xy)
	OLARG( Dimension,	wid_ht)
	OLARG( Dimension,	min)
	OLARG( Dimension,	max)
	OLARG( Dimension,	scr_wid_ht)
	OLGRA( int,		pause)
{
int xy_1 = (int)xy;
int xy_2 = (int)xy + wid_ht;

if (xy_2 < (int)min)
   xy_1 = -(wid_ht - min);
else
   if (xy_1 > (int)max)
      xy_1 = max;
   else
      if (-pause <= xy_1 && xy_1 <= 0)
         xy_1 = 0;
      else
         if ((int)scr_wid_ht <= xy_2 && xy_2 <= (int)scr_wid_ht + pause)
            xy_1 = scr_wid_ht - wid_ht;

return ((Position)xy_1);

} /* end of Normalize */

extern int
Window_List_Position OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
int i;
	for (i = 0; i < window_list->used; i++)
   		if (window_list->p[i] == (Widget)wm)
			return(i);
	return(window_list->used);
}

/*
 * MoveWindow
 * - Called from RaiseLowerGroup();
 *   value of which is either WMRaise or WMLower.
 *    if which == WMRaise then move wm to highest index in the window_list->
 *    if which == WMLower then move wm to lowest (1st) index in the window_list.
 */
extern void
MoveWindow(wm, which)
	WMStepWidget wm;
	int which;
{
WMStepWidget temp;
int i;

/* first find the window in the list, i = index */
for (i = 0; i < window_list->used; i++)
   if (window_list->p[i] == (Widget)wm)
      break;

if ( (i = Window_List_Position(wm)) == window_list->used)
   FPRINTF((stderr, "Couldn't find widget %x\n", wm));
else
   if (which == WMRaise)
      {
      if (i == window_list->used - 1) /* already at top of list */
         ;
      else
         {
	 /* Copy the next highest WidgetBuf entry in the list to where wm
	  * is in the list (at position i).
	  */
         bcopy(&(window_list->p[i + 1]), &(window_list->p[i]), 
            (window_list->used - i - 1) * sizeof(Widget));

	 /* Place wm in highest index (used) in window_list->*/
         window_list->p[window_list->used - 1] = (Widget)wm;
         }
      }
   else
      {
      if (i == 0)
         ;
      else
         {
         bcopy(&(window_list->p[0]), &(window_list->p[1]), i * sizeof(Widget));
         window_list->p[0] = (Widget)wm;
         }
      }

} /* end of MoveWindow */


/*
 * TimeOut()
 */

void
TimeOut()
{
	DragValues *current_drag_values = &drag_values;

	signal(SIGALRM,(void (*)())TimeOut);
	if ( (current_drag_values->value1 ==
					current_drag_values->prev_value1) &&
	    (current_drag_values->value2 ==
					current_drag_values->prev_value2) ) {
		OperationTimedOut = True;
	}
	else {
		current_drag_values->prev_value1=
				current_drag_values->current_prev_value1;
		current_drag_values->prev_value2 = 
				current_drag_values->current_prev_value2;
		alarm(TIMEOUT);
	}
}

#if !defined(ALL_BUTTONMASK)
#define ALL_BUTTONMASK	(Button1Mask | Button2Mask | Button3Mask | \
			Button4Mask | Button5Mask)
#endif

/*
 * PollDragWindowAround - Called from DragWindowAround();
 * The cursor will always be in a spot where we can control it;
 * If we got here via the keyboard, then the pointer is in the middle
 * of the client area ON the screen; otherwise it
 * can be just about anywhere on the screen; we warp it (if
 * using the keyboard) relative to the current position.
 * ADDITIONALLY - if we got here via the keyboard, then we not only
 * warped the pointer to the middle of the area on the screen, but we also SET
 * offsetx, offsety to SIMULATE a buttonpress event.  So, we can
 * reuse the same code for "mouse only" operations.
 *
 * For drawing the floating borders, don't redraw if in the
 * exact same spot; use the LinesNowDrawn variable as insurance so
 * we never lose track of WHEN the lines are drawn.
 * 
 * The function is nothing more than a huge for() loop that looks for
 * 2 event types: ButtonRelease, KeyPress.
 */


static void
PollDragWindowAround(event,buttondown)
XEvent *event;
int buttondown;
{
WMStepWidget	wm = TargetStepwidget;
Widget		w  = IsIconic(wm) ? wm->wmstep.icon_widget :
				(Widget)TargetStepwidget;

XEvent		ev;
Window		root;
Window		child;
int		*winx = &drag_values.value1,
		*winy = &drag_values.value2;
int		*prev_winx = &drag_values.prev_value1,
		*prev_winy = &drag_values.prev_value2,
		*current_prev_winx = &drag_values.current_prev_value1,
		*current_prev_winy = &drag_values.current_prev_value2;
int		rootx, rooty;
unsigned int	mask;
/* Each window being moved requires 4 XRectange structs;  we'll limit
 * the maximum number that can be moved to 25 (200/4) .
 */
#define MAXRECTS 200
XRectangle	rects[MAXRECTS];
XRectangle	*r = &rects[0];
XRectangle	*rptr;
int		num_rects = 0;
int		i;
int		swinx;
int		swiny;
Position	newx;
Position	newy;
Dimension	scrwidth  = WidthOfScreen(XtScreen(wm));
Dimension	scrheight = HeightOfScreen(XtScreen(wm));

int		done = 0; 
int		retval_event;
Boolean		LinesNowDrawn = False;
Boolean		warp_flag = False;
static	int	eventcount = 1; /* NOT USED - leave this here for now */
int		timer_count = 0;

	*current_prev_winx = *prev_winx = offsetx;
	*current_prev_winy = *prev_winy = offsety;
	if (!wmrcs->moveOpaque && !buttondown) {
		/* The server is grabbed, start out with equal x,y values,
		 * set the timer.
		 */
		OperationTimedOut = False;
		*winx = offsetx;
		*winy = offsety;
		drag_values.w = w;
		drag_values.timeout_set = True;
		timer_count = 0;
		signal(SIGALRM,(void (*)())TimeOut);
		alarm(TIMEOUT);
	}
	else
		*winx = *winy = 0;

	/* Coordinates returned by QueryPointer() -
 	 * rootx,y = root coords. of ptr.
 	 * child = child window ptr is in.
 	 * win_x,y = coords of ptr relative to child.
 	 * mask = current state of modifier keys and mouse buttons.
 	 */
	for(;;) {
		if ( (XCheckMaskEvent(XtDisplay(wm),
			wmrcs->moveOpaque ?
			 ButtonPressMask | ButtonReleaseMask | KeyPressMask |
								ExposureMask :
			 ButtonPressMask | ButtonReleaseMask | ExposureMask |
						 KeyPressMask, &ev) != True) ) {
			/* Get pointer coordinates relative to root window
			 * origin and WMStep window.  Note, if XQueryPointer()
			 * returns false, the pointer is not on the same screen
			 * as the specified window (w below).
			 */
#if !defined(CONSTRAIN)
			XQueryPointer(XtDisplay(w), XtWindow(w),
              			&root, &child, &rootx, &rooty,
				winx, winy, &mask);
#else
			/* Ignore the return value from OlQueryPointer()
			 * because the button may be in the down OR up
			 * state; OlQueryPointer() returns 0 if in
			 * up state.
			 */
			(void)OlQueryPointer(w, &root, &child, &rootx, &rooty,
				winx, winy);
#endif
				if (OperationTimedOut == True) {
					done = 1;
					drag_values.timeout_set = False;
				}
		}
	else {
	
	OlVirtualEventRec	ve;

	*winx = *current_prev_winx;
	*winy = *current_prev_winy;
	OlLookupInputEvent(w, &ev, &ve, OL_DEFAULT_IE);

redo:
	switch(ve.virtual_name) {
		  default:
			if (ev.type == ButtonRelease) {
				done = 1;
				break;
			}
			else
				if (ev.type == Expose) {
					if (ev.xany.window !=
							XtWindow(motifAIW))
						XtDispatchEvent(&ev);
					break;
				 }
				else
					if (ev.type != KeyPress)
						break;
					else /* keypress */ {
					  if (ev.xkey.state &
						ALL_BUTTONMASK) {
						ev.xkey.state &=
							~ALL_BUTTONMASK;

						OlLookupInputEvent(w, &ev,
							 &ve, OL_DEFAULT_IE);

						switch(ve.virtual_name) {
		  					default:
								break;
    							case OL_CANCEL:
							case OL_STOP:
    							case OL_DROP:
    							case OL_DEFAULTACTION:
							case OL_MOVERIGHT:
							case OL_MOVEUP:
							case OL_MOVELEFT: 
							case OL_MOVEDOWN:
							case OL_MULTIRIGHT:
							case OL_MULTIUP:
							case OL_MULTILEFT: 
							case OL_MULTIDOWN:
							    goto redo;
							    break;
						} /* end inner switch */
							break;
					} /* end if (mouse button pressed) */
					else /* unknown key to us- ignore it*/
						break;
				} /* end else (if !keypress) */
    		  case OL_CANCEL:
		  case OL_STOP:
			done = 2;
			break;
    		  case OL_DROP:
    		  case OL_DEFAULTACTION:
			done = 1;
			break;
		  case OL_SELECTKEY:
			buttondown = 1; 
			drag_values.timeout_set = False;
			break;
    		  case OL_MULTIRIGHT:
    		  case OL_MULTIUP:
    		  case OL_MULTILEFT: 
    		  case OL_MULTIDOWN:
    		  case OL_MOVERIGHT:
    		  case OL_MOVEUP:
    		  case OL_MOVELEFT: 
    		  case OL_MOVEDOWN:
			warp_flag = True;
			HandleMoveCmd((Widget)wm, ve.virtual_name, winx, winy);
			break;
		  } /* end outer switch */
		*winx = *winx * eventcount;
		*winy = *winy * eventcount;
	}  /* end else (got a hopefully useful event) */
		if (done != 0) {
   			GC  gc = wm-> wmstep.gc;

			/* Got a ButtonRelease, DROP, or STOP */
			XAllowEvents(XtDisplay(w),AsyncKeyboard,CurrentTime);
			XUngrabKeyboard(XtDisplay(w),CurrentTime);
			XAllowEvents(XtDisplay(w),AsyncPointer,CurrentTime);
   			OlUngrabDragPointer((Widget)wm);
			/* The second argument possibly should be AsyncKeyboard */
			/*
			XAllowEvents(XtDisplay(w),AsyncKeyboard,CurrentTime);
			XUngrabKeyboard(XtDisplay(w),CurrentTime);
*/
			if (!wmrcs->moveOpaque) {
				XUngrabServer(XtDisplay((Widget)w));
				XFlush(XtDisplay((Widget)w));
			}
			/* Set this value to False just in case it was set,
			 * so the alarm doesn't continue to reset itself.
			 */
			drag_values.timeout_set = False;
			OperationTimedOut = False;
			if (!wmrcs->moveOpaque) {
				int	iconx = w->core.x,
					icony = w->core.y;
				if(LinesNowDrawn) {
					XSetFunction(XtDisplay(Frame),
							 gc, GXinvert);
					XFillRectangles(XtDisplay(Frame),
						XtWindow(Frame), gc,
						rects, num_rects);
					XSetFunction(XtDisplay(Frame),
						 gc, GXcopy);
					LinesNowDrawn = False;
					num_rects = 0;
				}
				if (currentGUI == OL_MOTIF_GUI)
					XUnmapWindow(XtDisplay(wm), mfd.window);
				if (done == 2)
		 			return;
				for (i=0; num_rects < MAXRECTS &&
					 i < group_list->used; i++) {
         				WMStepWidget wm =
					 (WMStepWidget)group_list->p[i];
					Widget targetWidget = 
					  IsIconic(wm) ?
						wm->wmstep.icon_widget :
						 (Widget)wm;
#ifdef WITHDRAWNSTATE
					if (wm->wmstep.size == WMWITHDRAWN)
						continue;
#endif
					/* The above works because if
					 * the window is iconic, then
					 * group_list->used should
					 * equal 1.
					 */
         				newx = wm-> wmstep.dragX;
         				newy = wm-> wmstep.dragY;
    		 
					if (IsIconic(wm) && wmrcs->iconGrid)
						;
					else {
         				newx = Normalize(newx,
						targetWidget-> core.width,
				 	   WMminx, WMmaxx, scrwidth, PAUSEX);
         				newy = Normalize(newy,
						targetWidget->core.height,
				 	   WMminy, WMmaxy, scrheight, PAUSEY);
					}
         					if ((IsIconic(wm) && (newx !=
						   iconx || newy != icony)) ||
						  (!(IsIconic(wm)) && (
							newx != wm-> core.x ||
						    newy != wm-> core.y))) {
					if (currentGUI == OL_MOTIF_GUI &&
						IsIconic(wm)) {
						int maprow, mapcol;
						int retx, rety;
						Boolean taken;
					   if (motwmrcs->iconAutoPlace) {
						taken =
			DetermineMapPosFromXY(XtScreen(targetWidget),
					newx, newy, &maprow, &mapcol);
						if (!taken && maprow != -1 &&
							mapcol != -1) {
					DetermineXYFromMapPos(XtScreen(
			targetWidget), maprow, mapcol, &retx, &rety);
					newx = retx; newy = rety;
					MoveMapPosition(wm, maprow, mapcol);
					if (wm->wmstep.is_current &&
					  (motwmrcs->iconDecoration &
						ICON_ACTIVELABEL) )
						  CheckAIWDown(wm);
						}
						else {
							/* taken */
						/* Is it taken by us?? */
					if (maprow !=
						wm->wmstep.icon_map_pos_row
			|| mapcol != wm->wmstep.icon_map_pos_col)
						XBell(XtDisplay(targetWidget),
									0);
						return;
						} /* else */
					   } /* if iconAutoPlace */
					   else { /* No iconAutoPlace, just
						   * move active label if any
						   */
					if (wm->wmstep.is_current &&
					  (motwmrcs->iconDecoration &
						ICON_ACTIVELABEL) )
						  CheckAIWDown(wm);
					   } /* else !iconAutoPlace */
					} /* MOTIF */
						
			
            						XtMoveWidget(
								targetWidget,
								 newx, newy);
							if (currentGUI ==
					OL_MOTIF_GUI && IsIconic(wm) &&
					(motwmrcs->iconDecoration &
					ICON_ACTIVELABEL) ) {
						MoveAIW(wm);
					}
							if (!IsIconic(wm)) {
						  SendWindowMovedEvent(wm);
							} /* !(IsIconic) */
						} /* x or y changed */
				} /* end the little for () */
			} /* end (if !moveOpaque) */
			else
				/* This shouldn't be necessary, because we
				 * will not have moveOpaque in MOTIF, because
				 * of the enforced grid for icons.
				 */
				if (currentGUI == OL_MOTIF_GUI)
					XUnmapWindow(XtDisplay(wm), mfd.window);
			return;
		} /* end (if done != 0 ) */
		else {
			/* No change in button state, continue */
   			GC gc		= wm-> wmstep.gc;
			int num_rects_drawn = num_rects;

			if ( (*current_prev_winx == *winx) &&
					 (*current_prev_winy == *winy) ) {
				continue;
			}
			*current_prev_winx = *winx;
			*current_prev_winy = *winy;
			/* make winx, winy = the DIFFERENCE in where the
			 * pointer is now and where the pointer was when we
			 * started.  For starting out with the keyboard, the
			 * pointer was at (offsetx,offsety).  
			 */
   			*winx -= offsetx;
   			*winy -= offsety;

	/* Warp the pointer by winx,winy relative to its current position
	 * if you used the keyboard successfully.  If the pointer is warped to
	 * a spot potentially off the screen, then the usual server behavior is
	 * to warp it to the outer edge of the grabbed pointer window - since we
	 * grabbed the pointer on the root window, the pointer is delineated by
	 * the root window (usually the screen width and height).
 	 */
			if (warp_flag == True) {
				/* warp pointer by winx, winy, relative
				 * to current position.
				 */
				XWarpPointer( XtDisplay((Widget)wm), None,
					XtWindow(w),
					0,0,
					(unsigned int) 0, (unsigned int) 0,
					*current_prev_winx, *current_prev_winy);
				warp_flag = False;
			}
			
			rptr = r = &rects[0];
			num_rects = 0;
			for (i = 0; num_rects < MAXRECTS && i < group_list->used;
								 i++) {
				WMStepWidget wm =
					 (WMStepWidget)group_list->p[i];
				Widget w = IsIconic(wm) ?
					    wm->wmstep.icon_widget:
					    (Widget)wm;
#ifdef WITHDRAWNSTATE
				if (wm->wmstep.size == WMWITHDRAWN)
					continue;
#endif

				/* Save original "window moved difference"
				 * in swinx, swiny
				 */
				swinx = *winx;
				swiny = *winy;
				if (wmrcs->iconGrid && wmrcs->iconGridSize &&
         			    (wm-> wmstep.size == WMICONICNORMAL ||
          			     wm-> wmstep.size == WMICONICFULL)) {
					/* Moving an icon?  Can only move by the
					 * grid size
					 */
					*winx = (*winx / IconGridX) *
						       IconGridX;
					*winy = (*winy / IconGridY) *
						       IconGridY;
				} /* end if (iconic) */
      
				/* The NEW position (newx, newy) of each window
				 * being moved equals the old position + the
				 * difference in movement - SAVE in dragX, dragY,
				 */
				newx = wm-> wmstep.dragX = w-> core.x + *winx;
				newy = wm-> wmstep.dragY = w-> core.y + *winy;
     

	/* After adjusting the position of the window given the NEW coordinates,
	 * Normalize() may adjust the floating border to line up on the side of the
	 * screen - this depends on the distance the border now is (based on
	 * where the pointer is).  Provide an "empty area" (PAUSEX or PAUSEY
	 * pixels) around the borders that gives up time to line up the border
	 * with the edge of the screen.  Also constrains movement of window to
	 * maximum and minimum screen boundaries. 
	 */
				/* Don't pause at screens end if there's an
				 * icon grid and the object being moved is
				 * iconic.
				 */
				if (IsIconic(wm) && wmrcs->iconGrid)
						;
				else {
				newx = Normalize(newx,
						 w-> core.width, WMminx,
		 				WMmaxx, scrwidth, PAUSEX);
				newy =  Normalize(newy,
						 w-> core.height, WMminy,
		 				WMmaxy, scrheight, PAUSEY);
				}

				/* (winx, winy) is still the difference in movement
				 * of the PRIMARY window being moved.
				 */
				*winx = swinx;
				*winy = swiny;

				if (wmrcs->moveOpaque) {
				/* moveOpaque resource tells us not to draw the
				 * floating border during the move, just do it.
				 */
					if (newx != w-> core.x ||
						 	newy != w-> core.y) {
						XtMoveWidget(w, newx, newy);
							if (!IsIconic(wm)) {
						SendWindowMovedEvent(wm);
							}
					}
				}
      				else {
					int usex,usey;

					usex = newx;
					usey = newy;

					if (currentGUI == OL_MOTIF_GUI &&
				(motwmrcs->showFeedback & FEED_MOVE &&
				wm == TargetStepwidget))
						UpdateMotifFeedbackWindow(wm, 
							usex, usey, OP_MOVE);
					if (LinesNowDrawn == True) {
						/* Erase them first.  For
						 * them to have been drawn,
						 * num_rects must be
						 * initialized to the
						 * correct number, and all
						 * rectangles must be
						 * filled in!
						 */
						XSetFunction(XtDisplay(Frame),
						gc, GXinvert);
						XFillRectangles(XtDisplay(Frame),
					 	XtWindow(Frame), gc,
							rects, num_rects_drawn);
						XSetFunction(XtDisplay(Frame),
							 gc, GXcopy);
						/* Reset to False - is done ONCE
						 * for each SET of rectangles
						 *  previously drawn.
						 */
						LinesNowDrawn = False;
					}
					r->x = usex;
					r->y = usey;
					r->width = w-> core.width;
					r++->height = OUTLINE_WIDTH;
					r->x = usex;
					r->y = usey + w->core.height -
						OUTLINE_WIDTH;
					r->width = w-> core.width;
					r++->height = OUTLINE_WIDTH;
					r->x = usex;
					r->y = usey + OUTLINE_WIDTH;
					r->width = OUTLINE_WIDTH;
					r++->height = w-> core.height -
						2 * OUTLINE_WIDTH;
					r->x = usex + w->core.width -
						OUTLINE_WIDTH;
					r->y = usey + OUTLINE_WIDTH;
					r->width = OUTLINE_WIDTH;
					r++->height = w-> core.height - 
						2 * OUTLINE_WIDTH;
					num_rects += 4;
				}
			}
			if (!wmrcs->moveOpaque) {
				XSetFunction(XtDisplay(Frame), gc, GXinvert);
				XFillRectangles(XtDisplay(Frame),
					 XtWindow(Frame), gc, rects, num_rects);
				XSetFunction(XtDisplay(Frame), gc, GXcopy);
				LinesNowDrawn = True;
			}

		} /* end else (no change, continue */
	} /* end the BIG for loop */
} /* end of PollDragWindowAround */

#if !defined(WARP_PTR)
#define WARP_PTR(Display,Window,x,y) \
	XWarpPointer(Display, None, Window, 0, 0,		\
			(unsigned int) 0, (unsigned int) 0,	\
			x, y);
#endif

/*
 * PollResizeWindow
 * Keep track of the absolute position of the floating border.  For
 * mouseless operations, we must always keep track of the cursor
 * position, too.  If invoked from the Resize menu button on the window menu,
 * the cursor will "cling" to the first vertical border it gets near
 * (top or bottom) as well as the first horizontal border it gets near.
 * 
 * Note: you can only resize one window at a time - unlike moving
 * windows!!
 */
static void
PollResizeWindow OLARGLIST((buttondown,piece))
OLARG(int, buttondown)
OLGRA(WMPiece, piece)
{
Widget       w  = (Widget)TargetStepwidget;
WMStepWidget wm = TargetStepwidget;

static int	eventcount = 1;
XEvent		ev;
Window		root;
Window		child;
int		*rootx = &drag_values.value1,
		*rooty = &drag_values.value2,
		*prev_rootx = &drag_values.prev_value1,
		*prev_rooty = &drag_values.prev_value2,
		*current_prev_rootx = &drag_values.current_prev_value1,
		*current_prev_rooty = &drag_values.current_prev_value2;
int		winx = -1, winy = -1;
/*
int		rootx = -1, rooty = -1, winx = -1, winy = -1;
int		prev_rootx = -1, prev_rooty = -1;
int		current_prev_winx = -1, current_prev_winy = -1;
*/
unsigned int	mask;
XRectangle	rects[4];
XRectangle	*r;
WMStepWidget	h= (WMStepWidget)w;
XSizeHints	*size =	&h-> wmstep.xnormalsize;
int		screenwid = WidthOfScreen(XtScreen(TargetStepwidget));
int		screenht = HeightOfScreen(XtScreen(TargetStepwidget));
int		done = 0; 
int		retval_event;
int		left_border, right_border, top_border, bottom_border;
Boolean		LinesNowDrawn = False;
int		savebuttondown = buttondown;
int		sidepress = 0;

/* We'll need these for the minimum and max. width and height
 * for keyboard ops.
 */
int min_width = TargetStepwidget-> wmstep.xnormalsize.min_width;
int min_height = TargetStepwidget-> wmstep.xnormalsize.min_height;
int max_width = TargetStepwidget-> wmstep.xnormalsize.max_width;
int max_height = TargetStepwidget-> wmstep.xnormalsize.max_height;

#if !defined(THRESHOLD)
#define THRESHOLD 7
#endif

	if (min_width < 0) min_width = 0;
	if (max_width < 0) max_width = 0;
	if (min_height < 0) min_height = 0;
	if (max_height < 0) max_height = 0;

	if (min_width < 1) min_width = 1;
	if (min_height < 1) min_height = 1;
	
   if (currentGUI == OL_OPENLOOK_GUI) {
	min_width = ParentWidth(TargetStepwidget, min_width);
	min_height = ParentHeight(TargetStepwidget, min_height);
	if (max_width)
		max_width = ParentWidth(TargetStepwidget, max_width);
	if (max_height)
		max_height = ParentHeight(TargetStepwidget, max_height);
   }
   else {
	min_width = MParentWidth(TargetStepwidget, min_width);
	min_height = MParentHeight(TargetStepwidget, min_height);
	if (max_width)
		max_width = MParentWidth(TargetStepwidget, max_width);
	if (max_height)
		max_height = MParentHeight(TargetStepwidget, max_height);
   }



/* Variables designed explicitly for keyboard use.
 *    first_horizontal - it tells us, when using the
 * arrow keys to resize a window, the first horizontal adjustment
 * made (left = WM_L or right = WM_R). 
 *    first_vertical - tells us the first vertical adjustment attempted
 * (WM_T = up or WM_B = down). 
 * If either of these variables are set to WM_SE, then
 * they have not been used in this sequence yet.
 * When starting via a ButtonPress on a resize corner, these variable are
 * set appropriately to maintain consistency in the event loop.
 */

	if (buttondown == 0) {
		/* We got here via the keyboard or menu */
		left_border = MAX((int)wm->core.x,0);
		right_border = MIN( ( (int)wm->core.x + (int)wm->core.width),
			(screenwid-1) );
		top_border = MAX( (int)wm->core.y, 0);
		bottom_border = MIN( ((int)wm->core.y + (int)wm->core.height),
			screenht - 1);
		*rootx = *rooty = *prev_rootx = *prev_rooty = -1;
		drag_values.timeout_set = True;
		signal(SIGALRM,(void (*)())TimeOut);
		alarm(TIMEOUT);
	}
	else {
	/* piece must be set to something useful if the button is down.
	 * In addition, ResizeWindow() would have already called SetLimits()
	 * for each of these.
	 */ 
		switch(piece) {
			case WM_SE:
				first_vertical = WM_B;
				first_horizontal = WM_R;
				break;
			case WM_SW:
				first_vertical = WM_B;
				first_horizontal = WM_L;
				break;
			case WM_NE:
				first_vertical = WM_T;
				first_horizontal = WM_R;
				break;
			case WM_NW:
				first_vertical = WM_T;
				first_horizontal = WM_L;
				break;
			case WM_T:
			case WM_B:
				first_vertical = piece;
				first_horizontal = WM_SE;
				sidepress++;
				break;
			case WM_L:
			case WM_R:
				first_horizontal = piece;
				first_vertical = WM_SE;
				sidepress++;
				break;
			default:
				/* we shouldn't get here... */
				break;
		} /* end switch */
	} /* end else */

	/* Here is the deal: if we got here via the menu/keyboard, then
 	 * we have the rectangular area of the window that is ON
 	 * the screen; the pointer is right in the middle.  We must look for a:
 	 *	 KeyPress - to see which direction to go in and which border(s)
	 * to anchor;
 	 *	ButtonPress - doesn't mean much to me;
 	 *	Pointer motion "NEAR" the border - that's the key. Near the border -
 	 * that means within a certain number of pixels, let's say, a
	 * THRESHOLD.  When the pointer gets within this area, that's the border
	 * that we pick on to hook the cursor to.
 	 */

	/* Start the BIG for loop here...*/
	for(;;) {
		if ( (XCheckMaskEvent(XtDisplay(wm),
			ButtonPressMask | ButtonReleaseMask | KeyPressMask, &ev)
			!= True) ) {
			/* Get pointer coordinates relative to root window
			 * origin.  Note, if XQueryPointer() returns false,
			 * the pointer is not on the same screen as the
			 * specified window (w below).
			 * Addendum:  still_to_go is true is sidepress is
			 * false - the button hasn't been pressed on a side
			 * border of a motif resize decoration.
			 */
			Boolean still_to_go = (  (first_vertical == WM_SE ||
				first_horizontal == WM_SE) && !sidepress);
	
			/* Save the most recent rootx, rooty  */
			*current_prev_rootx = *rootx,
			*current_prev_rooty = *rooty;

			/* If this statement gets executed, then
			 * rootx and rooty get set or reset to absolute root pos.,
			 * and winx, winy get set to wm relative position.
			 */
#if !defined(CONSTRAIN)
			XQueryPointer(XtDisplay(w), XtWindow(w),
				/*
              			&root, &child, &rootx, &rooty,
				*/
              			&root, &child, rootx, rooty,
				&winx, &winy, &mask);
#else
			(void)OlQueryPointer(w, &root, &child, rootx, rooty,
				&winx, &winy);
#endif


			if (OperationTimedOut == True) {
				done = 1;
				drag_values.timeout_set = False;
			}
			else {
			    if (still_to_go) {
				/* look at (rootx, rooty) to determine
				 * "where" we are.
				 * Rootx (rooty) will fall somewhere within
				 * the horizontal (vertical) borders. How close?
				 */
				switch(first_horizontal) {
					default:
						break;
					case WM_SE:
					if (*rootx - left_border < THRESHOLD) {
						first_horizontal = WM_L;
						SetLimits(TargetStepwidget,
						  TargetStepwidget-> core.x +
				 		  TargetStepwidget-> core.width,
						  0, -max_width, -min_width,
						   0,0, WM_L);
					if (currentGUI == OL_MOTIF_GUI) {
					  Cursor cursor;
					  switch(first_vertical) {	
					   case WM_SE:
						cursor = l_cursor;
						break;
					   case WM_T:
						cursor = ul_cursor;
						break;
					   case WM_B:
						cursor = ll_cursor;
						break;
					  }
	OlGrabDragPointer((Widget)wm, cursor, XtWindow(XtParent(wm)));
					} /* Motif */


					}
					else
				  	if (right_border - *rootx < THRESHOLD) {
						first_horizontal = WM_R;
						/* The right border moves (left
						 * border anchored)
						 */
      						SetLimits(TargetStepwidget,
						   TargetStepwidget-> core.x,
				 		   0, min_width, max_width, 0,
						   0, WM_R);
					if (currentGUI == OL_MOTIF_GUI) {
					  Cursor cursor;
					  switch(first_vertical) {	
					   case WM_SE:
						cursor = r_cursor;
						break;
					   case WM_T:
						cursor = ur_cursor;
						break;
					   case WM_B:
						cursor = lr_cursor;
						break;
					  }
	OlGrabDragPointer((Widget)wm, cursor, XtWindow(XtParent(wm)));
					} /* Motif */

				  	  }
						break;
				} /* end switch first_horiz*/
				switch(first_vertical) {
					default:
						break;
					case WM_SE:
					  if (*rooty - top_border < THRESHOLD) {
						first_vertical = WM_T;
					if (currentGUI == OL_MOTIF_GUI) {
					  Cursor cursor;
					  switch(first_horizontal) {	
					   case WM_SE:
						cursor = t_cursor;
						break;
					   case WM_L:
						cursor = ul_cursor;
						break;
					   case WM_R:
						cursor = ur_cursor;
						break;
					  }
	OlGrabDragPointer((Widget)wm, cursor, XtWindow(XtParent(wm)));
					} /* Motif */
      						SetLimits(TargetStepwidget, 0,
			     			   TargetStepwidget-> core.y +
					           TargetStepwidget-> core.height, 
                				    0, 0, -max_height,
						     -min_height, WM_T);
					  }
					  else
				  	    if (bottom_border - *rooty < THRESHOLD) {
						first_vertical = WM_B;
					if (currentGUI == OL_MOTIF_GUI) {
					  Cursor cursor;
					  switch(first_horizontal) {	
					   case WM_SE:
						cursor = b_cursor;
						break;
					   case WM_L:
						cursor = ll_cursor;
						break;
					   case WM_R:
						cursor = lr_cursor;
						break;
					  }
	OlGrabDragPointer((Widget)wm, cursor, XtWindow(XtParent(wm)));
					} /* Motif */

      						SetLimits(TargetStepwidget, 0,
				 	  	  TargetStepwidget-> core.y, 0, 0,
					   	  min_height, max_height, WM_B);
					    }
						break;
				} /* end switch first_vertical */
			    } /* end if (still_to_go) */
			    else
				/* Both the vertical and horizontal directions
				 * are taken care of.
				 * Addendum: OR one of the vertical/horiz.
				 * directions aren't taken care of, but
				 * we got here via a buttonpress on a motif
				 * resize border (sidepress = 1) - motif
				 * decoration only.
				 */
				if ( (first_vertical != WM_SE) &&
					(first_horizontal != WM_SE) &&
					(*rootx == *current_prev_rootx) &&
					(*rooty == *current_prev_rooty) ) 
						continue;
				/* I would think about this a little more */
			 
			     /* Continue until at least one of these are set */
			     if ( (first_vertical == WM_SE) &&
					(first_horizontal == WM_SE) )
				continue;
			    /* otherwise, continue loop processing (below the
			     * else for the keyboard.
			     */
			} /* end (else OperationTimedOut != True) */
		} /* end, if(XCheckMaskEvent() fails */
		else { /* Using the Keyboard, go to work */
			OlVirtualEventRec	ve;

resizeredo:
			OlLookupInputEvent(w, &ev, &ve, OL_DEFAULT_IE);
			switch(ve.virtual_name) {
		  		case OL_SELECTKEY:
					buttondown = 1; 
					drag_values.timeout_set = False;
					break;
				case OL_CANCEL:
				case OL_STOP:
					done = 2;
					break;
				case OL_DROP:
				case OL_DEFAULTACTION:
					done = 1;
					break;
				case OL_MOVEDOWN:
					if (first_vertical == WM_SE) {
						int	new_key_y;

						first_vertical = WM_B;
					if (currentGUI == OL_MOTIF_GUI) {
					  Cursor cursor;
					  switch(first_horizontal) {	
					   case WM_SE:
						cursor = b_cursor;
						break;
					   case WM_L:
						cursor = ll_cursor;
						break;
					   case WM_R:
						cursor = lr_cursor;
						break;
					  }
	OlGrabDragPointer((Widget)wm, cursor, XtWindow(XtParent(wm)));
					} /* Motif */
						SetLimits(TargetStepwidget, 0,
				 	  	  TargetStepwidget-> core.y, 0, 0,
						  min_height, max_height, WM_B);
					/* Warp pointer to bottom_border (but same
					 * horizontal position.  Note that if
					 * rootx == -1, then we have yet to
					 * query the pointer (a keypress go here
					 * before the ptr moved.
					 */
						WARP_PTR(XtDisplay(wm),
							XtWindow(XtParent(wm)),
							(*rootx == -1) ?
					 (right_border - left_border)/2:*rootx,
							bottom_border)
#if !defined(CONSTRAIN)
						XQueryPointer(XtDisplay(w),
						 XtWindow(w), &root, &child,
						 rootx, rooty, &winx, &winy,
						 &mask);

#else
						(void)OlQueryPointer(w, &root,
							&child, rootx, rooty,
							&winx, &winy);
#endif

						/* Hopefully, if bottom of window is
						 * off screen, the warp will halt at
						 * bottom of screen.	
						 */
					}
					else {
						int warp_diff = 5;
						if (size-> flags & PResizeInc) {
		  				  warp_diff = size-> height_inc;
						}
						/* If first_vertical == WM_T,
						 * check DragY (the lower border
						 * anchor)
						 */
						if (first_vertical == WM_T) {
							/* the pointer should now
							 * be at the top border... 
							 * where exactly?? The idea
							 * is, don't let the ptr
				 			 * go below WMminy.  
							 */
							int total_warp_pos;

							total_warp_pos = *rooty +
								 warp_diff;
							if(total_warp_pos > WMmaxy){
							 warp_diff = WMmaxy - *rooty;
							 *rooty = WMmaxy;
							}
							else
							 *rooty= total_warp_pos;
						} /* end if f_v == WM_T */
						else
						  *rooty += warp_diff;
						  /* only warp by warp_diff */	
						  if (warp_diff > 0)
							WARP_PTR(XtDisplay(wm),
								None, 0,
								warp_diff)
					} /* end else (first_vert == WM_SE) */
					break;
				case OL_MOVELEFT:
					if (first_horizontal == WM_SE) {
						first_horizontal = WM_L;
					if (currentGUI == OL_MOTIF_GUI) {
					  Cursor cursor;
					  switch(first_vertical) {	
					   case WM_SE:
						cursor = l_cursor;
						break;
					   case WM_T:
						cursor = ul_cursor;
						break;
					   case WM_B:
						cursor = ll_cursor;
						break;
					  }
	OlGrabDragPointer((Widget)wm, cursor, XtWindow(XtParent(wm)));
					} /* Motif */
						SetLimits(TargetStepwidget,
						  TargetStepwidget-> core.x +
				 		  TargetStepwidget-> core.width,
						  0, -max_width, -min_width,
						   0,0, WM_L);
						/* Warp pointer to left_border,
						 * keep same vertical position.
					 	 */
						WARP_PTR(XtDisplay(wm),
							XtWindow(XtParent(wm)),
							left_border,
							(*rootx == -1) ?
					 (bottom_border - top_border)/2 : *rooty)
#if !defined(CONSTRAIN)
						XQueryPointer(XtDisplay(w),
						   XtWindow(w), &root, &child,
						   rootx, rooty, &winx, &winy,
						    &mask);

#else
						(void)OlQueryPointer(w, &root,
							&child, rootx, rooty,
							&winx, &winy);
#endif

					}
					else { /* first_horiz != WM_SE */
						int warp_diff = -5;
						if (size-> flags & PResizeInc) {
		  				    warp_diff = -size-> width_inc;
						}

						/* Be careful if moving left AND
				 		 * first_horizontal = WM_R.
				 		 */
						if (first_horizontal == WM_R) {
							/* Pointer should now be at
							 * top border
				 			 */
							int total_warp_pos;
							/* total_warp_pos is where
							 * you were GOING to warp to
				 			 */
							total_warp_pos = *rootx +
								 warp_diff;
							if(total_warp_pos < WMminx){
							  warp_diff= WMminx - *rootx;
							  *rootx = WMminx;
							}
							else
							  *rootx = total_warp_pos;
						}
						else /* first_horiz == WM_L */
						  *rootx += warp_diff;
						  /* only warp by warp_diff */	
						  if (warp_diff != 0)
							WARP_PTR(XtDisplay(wm),
								None,warp_diff,
								0)
					} /* end else (!= WM_SE */
					break;
    	  			case OL_MOVEUP:
					if (first_vertical == WM_SE) {
						first_vertical = WM_T;
					if (currentGUI == OL_MOTIF_GUI) {
					  Cursor cursor;
					  switch(first_horizontal) {	
					   case WM_SE:
						cursor = t_cursor;
						break;
					   case WM_L:
						cursor = ul_cursor;
						break;
					   case WM_R:
						cursor = ur_cursor;
						break;
					  }
	OlGrabDragPointer((Widget)wm, cursor, XtWindow(XtParent(wm)));
					} /* Motif */
      						SetLimits(TargetStepwidget, 0,
			     			   TargetStepwidget-> core.y +
					       	   TargetStepwidget-> core.height, 
                				    0, 0, -max_height,
						    -min_height, WM_T);
						/* Warp pointer to top_border,
						 * keep same horizontal position
					 	 */
						WARP_PTR(XtDisplay(wm),
							XtWindow(XtParent(wm)),
							(*rootx == -1) ?
					 (right_border - left_border)/2:*rootx,
							top_border)
#if !defined(CONSTRAIN)
						XQueryPointer(XtDisplay(w),
							XtWindow(w), &root,
							&child, rootx, rooty,
							&winx, &winy, &mask);
#else
						(void)OlQueryPointer(w, &root,
							&child, rootx, rooty,
							&winx, &winy);
#endif

					}
					else { /* first_vert != WM_SE */
						int warp_diff = -5;
						if (size-> flags & PResizeInc) {
		  				   warp_diff = -size-> height_inc;
						}
						if (first_vertical == WM_B) {
							/* Ptr should now be at the
				 			 * top border
				 			 */
							int total_warp_pos;
							total_warp_pos= *rooty +
								 warp_diff;
							if(total_warp_pos < WMminy){
							  warp_diff = WMminy -
									*rooty;
							  *rooty = WMminy;
							}
							else
							 *rooty = total_warp_pos;
						}
						else
							*rooty += warp_diff;
						if (warp_diff != 0)
							WARP_PTR(XtDisplay(wm),
								None, 0,
								warp_diff)
					}
					break;
    	  			case OL_MOVERIGHT:
					if (first_horizontal == WM_SE) {
						int	new_key_x;
			
						/* Rt border moves, left anchored */
						first_horizontal = WM_R;
					if (currentGUI == OL_MOTIF_GUI) {
					  Cursor cursor;
					  switch(first_vertical) {	
					   case WM_SE:
						cursor = r_cursor;
						break;
					   case WM_T:
						cursor = ur_cursor;
						break;
					   case WM_B:
						cursor = lr_cursor;
						break;
					  }
	OlGrabDragPointer((Widget)wm, cursor, XtWindow(XtParent(wm)));
					} /* Motif */
      						SetLimits(TargetStepwidget,
						   TargetStepwidget-> core.x,
				 		   0, min_width, max_width, 0, 0,
						   WM_R);
						WARP_PTR(XtDisplay(wm),
							XtWindow(XtParent(wm)),
							right_border,
							(*rootx == -1) ?
					 (bottom_border - top_border)/2: *rooty)
#if !defined(CONSTRAIN)
						XQueryPointer(XtDisplay(w),
							XtWindow(w), &root, &child,
							rootx, rooty, &winx,
							&winy, &mask);
#else
						(void)OlQueryPointer(w, &root,
							&child, rootx, rooty,
							&winx, &winy);
#endif
					} /* end if (first_horiz == WM_SE */
					else {
						int warp_diff = 5;

						if (size-> flags & PResizeInc) {
		  				   warp_diff = size-> width_inc;
						}
						if (first_horizontal == WM_L) {
						   int total_warp_pos;

						   total_warp_pos = *rootx +
								 warp_diff;
						   if(total_warp_pos > WMmaxx){
						     warp_diff = WMmaxx- *rootx;
						     *rootx = WMmaxx;
						   }
						   else
							*rootx = total_warp_pos;
						} /* end if( f_h == WM_L) */
						else
							*rootx += warp_diff;
						if (warp_diff)
							WARP_PTR(XtDisplay(wm),
								None,warp_diff,
								0)
					}
					break;
				default:
					if (ev.type == ButtonRelease)
						done = 1;
					else
					  if (ev.type != KeyPress)
						break;
					  else /* keypress */ {
					    if (ev.xkey.state &
						ALL_BUTTONMASK) {
						ev.xkey.state &=
							~ALL_BUTTONMASK;

						OlLookupInputEvent(w, &ev,
							 &ve, OL_DEFAULT_IE);

						switch(ve.virtual_name) {
		  					default:
								break;
    							case OL_CANCEL:
							case OL_STOP:
    							case OL_DROP:
    							case OL_DEFAULTACTION:
							case OL_MOVERIGHT:
							case OL_MOVEUP:
							case OL_MOVELEFT: 
							case OL_MOVEDOWN:
								goto resizeredo;
								break;
						} /* end inner switch */
							break;
					} /* end if (mouse button pressed) */
					else /* unknown key to us- ignore it*/
						break;
					} /* end else keypress */
					break;
			} /* end switch */
		  /* Keep going until we get arrow key or done */
		  if( (!done) && (first_horizontal == WM_SE) &&
				 		(first_vertical == WM_SE))
				continue;
		} /* end else if(processing keyboard or buttonpress event) */
		if  (done != 0) {
			/* Would you like to do resizeOpaque - don't draw any lines,
			 * don't grab any server!
			 */
			XAllowEvents(XtDisplay(w),AsyncKeyboard,CurrentTime);
			XUngrabKeyboard(XtDisplay(w),CurrentTime);
			XAllowEvents(XtDisplay(w),AsyncPointer,CurrentTime);
   			OlUngrabDragPointer((Widget)wm);
   			NowResizingWindow = 0;
			/* The 2nd arg possibly should be AsyncKeyboard */
			/*
			XAllowEvents(XtDisplay(w),AsyncKeyboard,CurrentTime);
			XUngrabKeyboard(XtDisplay(w),CurrentTime);
			 */
			if (currentGUI == OL_MOTIF_GUI) {
				/* Unmap feedback window */
				XUnmapWindow(XtDisplay((Widget)wm), mfd.window);
			}
			if (LinesNowDrawn == True) {
				/* erase them */
				XSetFunction(XtDisplay(w), h-> wmstep.gc,
								 GXinvert);
				XFillRectangles(XtDisplay(Frame),
		 		    XtWindow(Frame), h->wmstep.gc, rects, 4);
				XSetFunction(XtDisplay(w), h-> wmstep.gc,
							 GXcopy);
			}
			XUngrabServer(XtDisplay((Widget)w));
			XFlush(XtDisplay((Widget)w));

			OperationTimedOut = False;
			drag_values.timeout_set = False;

   			if ( (done != 2) && (offsetx != w-> core.width ||
					 offsety != w-> core.height) ) {
				int childwidth, childheight;
				int	savex = w->core.x,
					savey = w->core.y;

      				XtConfigureWidget(w, NewX, NewY, offsetx,
					 offsety, w-> core.border_width);
				/* Why not configure the inside widget
				 * rather than the window?
				 */
/*
      				XResizeWindow(XtDisplay(w), h-> wmstep.window, 
					ChildWidth(h), ChildHeight(h));
 */
				if (currentGUI == OL_OPENLOOK_GUI) {
					childwidth = ChildWidth(h);
					childheight = ChildHeight(h);
				}
				else {
					childwidth = MChildWidth(h);
					childheight = MChildHeight(h);
				}
      				XtResizeWidget(h->wmstep.child,childwidth,
					 childheight,
					h->wmstep.child->core.border_width);
				/* Send synthetic move event (root x,y coords)
				 * in all cases of reconfigured window.
				 */
					SendWindowMovedEvent(wm);
			}
			else
				/* after all the resizing, no change;
				 * you started with the button down,
				 * using the pointer, so un-highlight
				 * the resize corner.
				 */
				if (savebuttondown)
					ShowResizeFeedback(wm, piece, 0);
			return;
		} /* end if(done != 0) */
		else { /* keep it going (!done) */
			/* You get here knowing that SOMETHING is set,
			 * either first_horizontal or first_vertical, not
			 * necessarily both.  The pointer is in a place that you
	 		 * can control...
			 * Addendum for MOOLIT, motif mode: it's possible that
			 * the pointer grabbed WM_T, WM_B, WM_L, or WM_R,
			 * meaning that only one of first_horizontal or
			 * first_vertical is set;  in that case, only one
			 * direction may resize, and the pointer is oblivious
			 * to the other until the other direction is set -
			 * that can only happen if a keypress occurs in
			 * that other direction.
			 */
   		Widget Frame = XtParent(w);
   		int x1;
   		int x2;
   		int y1;
   		int y2;
   		int DragX = wm-> wmstep.dragX;
   		int DragY = wm-> wmstep.dragY;
		/* DragX is not applicable until first_horizontal is set;
		 * DragY is not applicable until first_vertical is set.
		 */
		int base_width = wm-> wmstep.xnormalsize.base_width;
		int base_height = wm-> wmstep.xnormalsize.base_height;
		int prev_width = offsetx, prev_height = offsety;

		/* We enter here with a meaningful rootx if first_horizontal
		 * is set, and a meaningful rooty if first_vertical is set.
		 */

   if (first_horizontal != WM_SE) {
	/* For minx, rootx can't be < minx because the ptr can't be off the
	 * screen!!
	 */
   	if (WMminx && *rootx < WMminx) *rootx = WMminx;
	/* Note WMmaxx can == 0.  Consider a window
	 * that is flush against the left side of screen - and a wide resize
	 * corner where the x value for the ptr can initially be > 0
	 */
   	if (WMmaxx && *rootx > WMmaxx ||
	    /* Window is flush against the left side of screen - but it's
	     * already the minimum width - can't move the left border in
	     * this case.  if core.x == 0 and WMMaxx == 0, then WMMaxx is
	     * REALLY the maximum x allowed - so check to see what corner
	     * is being grabbed.
	     */
	    wm->core.x == 0 && !WMmaxx && *rootx > WMmaxx &&
	    first_horizontal == WM_L || first_horizontal == WM_NW ||
	    first_horizontal == WM_SW)
			 *rootx = WMmaxx;
   }
   /* Be careful of window flush against the top - y=0 */
   if (first_vertical != WM_SE) {
   	if (WMminy && *rooty < WMminy) *rooty = WMminy;
   	if ( (WMmaxy && *rooty > WMmaxy) ||
	    /* If WMMaxy == 0 and wm->core.y == 0 then the window is flush
	     * against the top and you're trying to move the top border when
	     * the window is probably ALREADY the minimum size.
	     * We need to make insurance checks to see if first_vertical (in
	     * this case) == on of the tops (WM_T or WM_NW or WM_NE).
	     */
	    (wm->core.y == 0 && !WMmaxy  && *rooty > WMmaxy &&
	      (first_vertical == WM_T || first_vertical == WM_NW ||
	      first_vertical == WM_NE)) )
			*rooty = WMmaxy;
   }

	if(first_horizontal != WM_SE) {
		if (*rootx < DragX)
			{ x1 = *rootx; x2 = DragX; }
		else
			{ x2 = *rootx; x1 = DragX; }
		if (first_vertical == WM_SE) {
			/* No change for vertical positional - constant y */
			y1 = wm->core.y;
			y2 = wm->core.y + (Position)wm->core.height;
		}
	}
	if (first_vertical != WM_SE) {
		if (*rooty < DragY)
			{ y1 = *rooty; y2 = DragY; }
		else
			{ y2 = *rooty; y1 = DragY; }
		if (first_horizontal == WM_SE) {
			/* y changes, x (width) stays the same */
			x1 = wm->core.x;
			x2 = wm->core.x + (Position)wm->core.width;
		}
	}
	offsetx = x2 - x1;
	offsety = y2 - y1;
	NewX = x1;
	NewY = y1;

	/* Adjust to resize increments - BUT be sure that dragX, dragY stay
	 * anchored... Deal with the client window separately.  Take
	 * the childwidth of the offsets (dimensions) found and later add on
	 * the decoration border.
	 */
	if (size-> flags & PResizeInc) {
		int pre_offsetx = offsetx;
		int pre_offsety = offsety;
		int diff;
		if (first_horizontal != WM_SE) {
			/* first the width */
			int new_child_wid;

			if (currentGUI == OL_OPENLOOK_GUI)
 				new_child_wid = NewChildWidth(wm, offsetx);
			else
 				new_child_wid = MNewChildWidth(wm, offsetx);
			diff = offsetx - new_child_wid;
			offsetx -= diff;
			offsetx = base_width + 
		  	((offsetx - base_width) / size-> width_inc) *
							 size-> width_inc;
			offsetx += diff;
		}
		if (first_vertical != WM_SE) {
			int new_child_ht;

			if (currentGUI == OL_OPENLOOK_GUI)
 				new_child_ht = NewChildHeight(wm, offsety);
			else
 				new_child_ht = MNewChildHeight(wm, offsety);

			diff = offsety - new_child_ht;
			offsety -= diff;
			offsety = base_height + 
		  	((offsety - base_height) / size-> height_inc) *
							 size-> height_inc;
			offsety += diff;
		}
		if ( (first_horizontal != WM_SE ) &&
				 (diff = offsetx - pre_offsetx) != 0) {
			/* Adjust NewX, NewY */
			switch(first_horizontal){
				/* diff = current width - previous width */
				case WM_L:
					if (diff < 0) { /* it got smaller */
						NewX -= diff;
					}
					else {
						NewX += diff;
					}
						x1 = NewX;
					break;
				case WM_R: break;
				default: break;
			} /* end switch */
		}
		if ( (first_vertical != WM_SE) &&
				 (diff = offsety - pre_offsety) != 0) {
			/* Adjust NewX, NewY */
			switch(first_vertical){
				case WM_T:
					/* diff = current height - prev. ht. */
					if (diff < 0) /* it got smaller */
						NewY -= diff;
					else
						NewY += diff;
					y1 = NewY;
					break;
				case WM_B: break;
				default: break;
			} /* end switch */
		}
	} /* end if (sizehints->flags & PResizeInc) */
	if ( (LinesNowDrawn == True) && (prev_width == offsetx &&
					prev_height == offsety))
		continue;

	/* Motif mode - update the feedback box, if present */
	if (currentGUI == OL_MOTIF_GUI &&
				(motwmrcs->showFeedback & FEED_RESIZE))
		UpdateMotifFeedbackWindow(wm, MNewChildWidth(wm, offsetx),
				MNewChildHeight(wm, offsety), OP_SIZE);

        if (LinesNowDrawn == True) {
		/* Otherwise erase the lines-
		 * rect still contains the correct erasable values
		 */
		XSetFunction(XtDisplay(w), h-> wmstep.gc, GXinvert);
		XFillRectangles(XtDisplay(Frame),
			 XtWindow(Frame), h->wmstep.gc, rects, 4);
		XSetFunction(XtDisplay(w), h-> wmstep.gc, GXcopy);
		LinesNowDrawn = False;
	}
		
		r = &rects[0];
       		r->x = x1;
         	r->y = y1;
         	r->width = offsetx;
        	r++->height = OUTLINE_WIDTH;
         	r->x = x1;
         	r->y = y1 + offsety -
			OUTLINE_WIDTH;
         	r->width = offsetx;
         	r++->height = OUTLINE_WIDTH;
         	r->x = x1;
         	r->y = y1 + OUTLINE_WIDTH;
         	r->width = OUTLINE_WIDTH;
         	r++->height = offsety -
			2 * OUTLINE_WIDTH;
         	r->x = x1 + offsetx -
			OUTLINE_WIDTH;
         	r->y = y1 + OUTLINE_WIDTH;
         	r->width = OUTLINE_WIDTH;
         	r++->height = offsety - 
			2 * OUTLINE_WIDTH;


	XSetFunction(XtDisplay(w), h-> wmstep.gc, GXinvert);
	XFillRectangles(XtDisplay(Frame),
		 XtWindow(Frame), h->wmstep.gc, rects, 4);
	XSetFunction(XtDisplay(w), h-> wmstep.gc, GXcopy);
	LinesNowDrawn = True;

		} /* end else (keep going) */
	} /* end big outer for() */
} /* end of PollResizeWindow */

/*
 * ResizeWindow
 *
 */

extern void
ResizeWindow OLARGLIST((wm, event, piece))
OLARG(WMStepWidget,	wm)
OLARG(XEvent *,		event)
OLGRA(WMPiece,		piece)
{
static int showcursor = 1;
int min_width = wm-> wmstep.xnormalsize.min_width;
int min_height = wm-> wmstep.xnormalsize.min_height;
int max_width = wm-> wmstep.xnormalsize.max_width;
int max_height = wm-> wmstep.xnormalsize.max_height;
#ifdef USEBUF
char	buf[BUFSIZ];
#endif

	if (min_width < 0) min_width = 0;
	if (max_width < 0) max_width = 0;
	if (min_height < 0) min_height = 0;
	if (max_height < 0) max_height = 0;

	if (min_width < 1) min_width = 1;
	if (min_height < 1) min_height = 1;

	if (currentGUI == OL_OPENLOOK_GUI) {
		min_width = ParentWidth(wm, min_width);
		min_height = ParentHeight(wm, min_height);
		if (max_width)
 			max_width = ParentWidth(wm, max_width);
		if (max_height)
			max_height = ParentHeight(wm, max_height);
	}
	else { /* Motif */
		min_width = MParentWidth(wm, min_width);
		min_height = MParentHeight(wm, min_height);
		if (max_width)
 			max_width = MParentWidth(wm, max_width);
		if (max_height)
			max_height = MParentHeight(wm, max_height);
	} /* Motif */

	/* During the resize operation, offsetx and offsety will rep. the
	 * width and ht. of the window...
	 */
	offsetx  = wm->core.width;
	offsety  = wm->core.height;


	FPRINTF((stderr,"\n\n min %dx%d max %dx%d\n", 
		min_width, min_height, max_width, max_height));

	/* Always reset these to 0 - if using keyboard to resize, SetLimits()
 	 * is called in phases (either horizontal or vertical axis, one at a
 	 * time).  When it is called for one of these, the other must not
 	 * be reset to 0 in case it is ALREADY set; on the other hand,
 	 * it must be set to 0 beforehand because code in
 	 * PollResizeWindow() depends on it's value.
 	 */
	WMminx = WMmaxx = WMminy = WMmaxy = 0;
	if (event != (XEvent *) 0) {
  		switch(piece) {
			case WM_SE:
      				SetLimits(wm, wm-> core.x, wm-> core.y, min_width,
					 max_width, min_height, max_height,piece);
				break;
			case WM_SW:
				SetLimits(wm, wm-> core.x + wm-> core.width,
					 wm-> core.y, -max_width, -min_width,
					 min_height, max_height, piece);
				break;
			case WM_NW:
				SetLimits(wm, wm-> core.x + wm-> core.width,
				   wm-> core.y + wm-> core.height, -max_width,
				   -min_width, -max_height, -min_height, piece);
				break;
			case WM_NE:
				SetLimits(wm, wm-> core.x,
				  wm-> core.y + wm-> core.height, min_width,
				  max_width, -max_height, -min_height, piece);
				break;
			case WM_T:
      				SetLimits(wm, 0,
			     		wm->core.y + wm->core.height, 
                			0, 0, -max_height,
					-min_height, WM_T);
				break;
			case WM_B:
      				SetLimits(wm, 0,
					wm->core.y, 0, 0,
					min_height, max_height, WM_B);
				break;
			case WM_L:
				SetLimits(wm,
					wm->core.x + wm->core.width,
					0, -max_width, -min_width,
					0,0, WM_L);
				break;
			case WM_R:
      				SetLimits(wm,
					wm->core.x,
					0, min_width, max_width, 0,
					0, WM_R);
				break;
		} /* end switch */
#if defined(CONSTRAIN)
		ResetConstrainPoints(event);
#endif
	} /* end if (event != NULL) */


	/* Grab pointer for all occasions on root window*/
	if (currentGUI == OL_OPENLOOK_GUI)
		OlGrabDragPointer((Widget)wm, OlGetPanCursor((Widget)wm),
				 	XtWindow(XtParent((Widget)wm)));
	else {
		Cursor cursor;

		if (fleur_cursor == (Cursor)0)
			SetupWindowOpCursors(XtDisplay(wm));
		if (event == (XEvent *)0)
			cursor = fleur_cursor;
		else
			switch(piece) {
				case WM_NE:
					cursor = ur_cursor;
					break;
				case WM_SE:
					cursor = lr_cursor;
					break;
				case WM_NW:
					cursor = ul_cursor;
					break;
				case WM_SW:
					cursor = ll_cursor;
					break;
				case WM_L:
					cursor = l_cursor;
					break;
				case WM_R:
					cursor = r_cursor;
					break;
				case WM_B:
					cursor = b_cursor;
					break;
				case WM_T:
					cursor = t_cursor;
					break;
				default:
					cursor = fleur_cursor;
					break;
			} /* switch */
		OlGrabDragPointer((Widget)wm, cursor,
				 	XtWindow(XtParent(wm)));
	} /* Motif */

	if (event == (XEvent *) 0) {
		wm->wmstep.dragX = wm->core.x;
		wm->wmstep.dragY = wm->core.y;
		first_vertical = first_horizontal = WM_SE;
	}
	if (XGrabKeyboard(XtDisplay(wm),XtWindow(XtParent(wm)),False, GrabModeAsync,
 			GrabModeAsync, CurrentTime) != GrabSuccess) {
#if !defined(I18N)
	   fprintf(stderr,"Warning: Keyboard Grab Failed for Resize operation\n");
#else
	OlVaDisplayWarningMsg(XtDisplay(wm), OleNbadKeyboard, OleTnoGrab,
	   OleCOlClientOlwmMsgs,
	OleMbadKeyboard_noGrab,
	OlGetMessage(XtDisplay(wm), NULL, 0, OleNresize, OleTresize,
	  OleCOlClientOlwmMsgs, OleMresize_resize, NULL));
#endif
	}
	/*Grab the server last*/
	XSync(XtDisplay(wm),False);
	XGrabServer(XtDisplay(wm));
	XFlush(XtDisplay(wm));
	if ((event == (XEvent *) NULL) && showcursor) {
		int	startonscreenx,
			endonscreenx,
			startonscreeny,
			endonscreeny,
			warpx, warpy;

#ifdef CONSTRAIN
		Window root, child;
		int winx, winy, rootx, rooty;
		unsigned int mask;
		XButtonEvent ev;
#endif
		/* Used keyboard to get here, so warp pointer to middle of
		 * client.  You really want to warp it to the middle of
		 * the part of the client now on the screen, so figure out
		 * what piece of the client is on the screen.
		 */
		startonscreenx = MAX(wm->core.x,0);
		startonscreeny = MAX((int)wm->core.y,0);
		endonscreenx = MIN((int)wm->core.x + (int)wm->core.width,
				   WidthOfScreen(XtScreen(wm)) -1);
		endonscreeny = MIN((int)wm->core.y + (int)wm->core.height,
				   HeightOfScreen(XtScreen(wm)) -1);
		warpx = (endonscreenx - startonscreenx) / 2;
		warpy = (endonscreeny - startonscreeny) / 2;
		/* Check the call out - warp to wm relative coords. */
		XWarpPointer( XtDisplay((Widget)wm), None,
				 XtWindow((Widget)wm),
				 0,0,
				 (unsigned int) 0, (unsigned int) 0,
				 (int)wm->core.x < 0 ?
					warpx =	abs((int)wm->core.x) + warpx
						: warpx,
				 (int)wm->core.y < 0 ?
					warpy =	abs((int)wm->core.y) + warpy
						: warpy);
		/* Now the pointer is warped to the middle of the window
		 * pane on the screen...
		 */
#ifdef CONSTRAIN
		XQueryPointer(XtDisplay((Widget)wm), XtWindow((Widget)wm),
              			&root, &child, &rootx, &rooty,
				&winx, &winy, &mask);
		ev.x = winx;
		ev.y = winy;
		ev.x_root =  rootx;
		ev.y_root =  rooty;
		ResetConstrainPoints((XEvent *)&ev);
#endif
	}

	NowResizingWindow = 1;
	TargetStepwidget = wm;

	XSync(XtDisplay(wm),False);
	if(currentGUI == OL_MOTIF_GUI &&
				(motwmrcs->showFeedback & FEED_RESIZE)) {
		XMapRaised(XtDisplay(wm), mfd.window);
		UpdateMotifFeedbackWindow(wm, MNewChildWidth(wm, offsetx),
				MNewChildHeight(wm, offsety), OP_SIZE);
	}
	PollResizeWindow((event==(XEvent *)0) ? 0:1,piece);

} /* end of ResizeWindow */

/*
 * RestackWindows
 * - Called from RaiseLowerGroup() with list == group_list->
 * Call XConfigureWindow() to raise or lower each window in list.
 */
extern void
RestackWindows(display, list, which)
Display * display;
WidgetBuffer * list;
int which;
{
int            i       = (which == WMRaise ? list-> used - 1 : 0);
WMStepWidget	wm = (WMStepWidget)list->p[i];
WMStepPart	*wmstep = (WMStepPart *)&(wm->wmstep);
Window         window  = XtWindow(list-> p[i]);
unsigned int	value_mask = CWStackMode;
XWindowChanges wc;
int	step;

wc.sibling = None;

switch(which)
   {
   case WMRaise:
      wc.stack_mode = Above;
      XConfigureWindow(display, window, value_mask, &wc);
      if (wmstep->icon_widget) {
	if (currentGUI == OL_OPENLOOK_GUI ||
		(currentGUI == OL_MOTIF_GUI &&
		(  (!(motwmrcs->iconDecoration & ICON_ACTIVELABEL) ||
		 motwmrcs->iconDecoration&ICON_ACTIVELABEL &&
				!(wm->wmstep.is_current)))   ))
	XConfigureWindow(display, XtWindow(wmstep->icon_widget),
					 value_mask, &wc);
	else {
		RaiseAIWGroup(wm, NULL);
	}
      } /* if icon_widget */
      wc.stack_mode = Below;
	step = -1;
	i--;
      break;
   case WMLower:
      wc.stack_mode = Below;
      XConfigureWindow(display, window, value_mask, &wc);
      if (wmstep->icon_widget) {
	if (currentGUI == OL_OPENLOOK_GUI ||
		(currentGUI == OL_MOTIF_GUI &&
		(  (!(motwmrcs->iconDecoration & ICON_ACTIVELABEL) ||
		 motwmrcs->iconDecoration&ICON_ACTIVELABEL &&
				!(wm->wmstep.is_current)))   ))
		XConfigureWindow(display, XtWindow(wmstep->icon_widget),
					 value_mask, &wc);
	else
		LowerAIWGroup(wm, NULL);
      }
      wc.stack_mode = Above;
	step = 1;
	i = 1;
      break;
   default:
      break;
   }
value_mask |= CWSibling;
/*
for (--i; i >= 0; i--)
 */
for (; i >= 0 && i < list->used; i+= step)
   {
   wc.sibling = window;
   window = XtWindow(list-> p[i]);
   XConfigureWindow(display, window, value_mask, &wc);
   if ( (wm = (WMStepWidget)(list->p[i]))->wmstep.icon_widget) {
	if (currentGUI == OL_OPENLOOK_GUI || currentGUI == OL_MOTIF_GUI &&
		(  (!(motwmrcs->iconDecoration & ICON_ACTIVELABEL) ||
		 motwmrcs->iconDecoration&ICON_ACTIVELABEL &&
				!(wm->wmstep.is_current))) )
		   XConfigureWindow(display, XtWindow(wm->wmstep.icon_widget),
				value_mask, &wc);
	else { /* Must be motif without active label */
		if (wc.stack_mode == Above)
			/* Raise icon (and label) above the window */
			RaiseAIWGroup(wm, window);
		else
			/* Lower icon and label below the window */
			LowerAIWGroup(wm, window);
	}
		
   } /* if */
   } /* for */


} /* end of RestackWindows */

/*
 * RaiseLowerGroup
 * - Called from: WMReparent() callback, Select(), MenuBack(),
 * MenuOwner(), ClientSubstructureRedirect().
 *  - "which" arg = [WMRaise, WMLower]
 */
extern void
RaiseLowerGroup(wm, which)
WMStepWidget wm;
int          which;
{
WMStepWidget temp;
int          i;
Window leader = find_highest_group(wm, (Window) 0);
int idx;
WMStepWidget targetwm = NULL;

/* Gather list of the WMStepWidget window's group members -
 * if windowLayering is on, get group members, fill in group_list->WidgetBuffer
 * list.  For each member of list (including wm), set
 *  wm->wmstep.decorations |= WMSelected
 */
	/* First get the group list of this wm, and all it's descendents;
	 * the idea is you want to put this window on top of it's
	 * children, AND all other windows at least on the same level
	 * (e.g., that have the same parent as this window; so use
	 * No AppendAny; then gather all windows in the same group; the
	 * ones not touched are the parents; the parents must go on
	 * top, followed by these; can use the WMSelected flag to tell
	 * us that these windows were already touched.
	 */
	/* For WMRaise, if this is a transient window, or it has a
	 * window group other than itself, then raise the whole group.
	 */
/*
   if (which == WMRaise) {
*/


	if ( (idx=IsWMStepChild(leader)) >= 0) {
		ConstructGroupList(targetwm = (WMStepWidget)wmstep_kids[idx],
						 NestedNoAppendAny,TRUE);
	}
	else
		if (leader != wm->wmstep.window)
			/* leader is invisible - could be any window not
			 * yet mapped, or this may have been done
			 * intentionally.
			 */
			ConstructGroupList(targetwm = wm,
					NestedNoAppendAny,TRUE);
	if (group_list->used == 0)
		/* Shouldn't happen because the group should at least have
		 * the wmstep widget wm in it.
		 */
		return;
	if (which == WMRaise) {
		MoveWindow(targetwm, WMRaise);
		for (i = 1; i < group_list->used; i++) {
			temp = (WMStepWidget)group_list->p[i];
			MoveWindow(temp, WMRaise);
		}
		RestackWindows(XtDisplay(wm), group_list, which);
	}
/*
   }
 */
	/* Now raise/lower the window and it's children */
	/* mlp: This may come back to haunt me one day:
	 * if wm is iconic, then I don't want to go through
	 * this extra work for nothing - it is causing an extra
	 * expose event on the icon, and if in motif mode with
	 * active label feature, it causes problems when moving a
	 * window, such as an expose at the old location.
	 */
	if (which == WMRaise && IsIconic(wm))
		return;
	if (which == WMRaise) {
		ConstructGroupList(wm, NoAppend, wmrcs->windowLayering);
	}
	else
		if (targetwm) /* for lower */
			wm = targetwm;

if (which == WMRaise)
   {
   /* MoveWindow() moves wm to the highest index in the window_list->array */
   MoveWindow(wm, WMRaise);

   /* I think the intention here is, now that wm is on "top" of window_list->
    * go through the group members, move each one up within the window_list->
    * Start i at 1, presumably because group_list->will have wm (the group
    * leader of SOME group) in index 0.  But will this move the group members
    * higher than the wm window in the window_list->?
    */
   for (i = 1; i < group_list->used; i++)
      {
      temp = (WMStepWidget)group_list->p[i];
      MoveWindow(temp, WMRaise);
      }
   } /* if WMRaise */
else /* Lower windows - like selecting "Back" */
   {
   /* Successively move each window to the front (lowest index) of the
    * window_list->*/
   for (i = 1; i < group_list->used; i++)
      {
      temp = (WMStepWidget)group_list->p[i];
      MoveWindow(temp, WMLower);
      }

   /* Lower the group leader window (wm) last */
   MoveWindow(wm, WMLower);
   }
RestackWindows(XtDisplay(wm), group_list, which);

} /* end of RaiseLowerGroup */

/*
 * SetLimits
 * For keyboard operations, piece can be WM_T, WM_R, WM_L, WM_B.
 * Otherwise, the usual: WM_SE, WM_SW, WM_NE, WM_NW.
 * Keyboard caveats:
 * 	if (piece == WM_T or WM_B) => use args wm, y, miny, maxy,
 * and set DragY only.
 * 	if (piece == WM_L or WM_R) => use args wm, x, minx, maxx,
 * and set DragX only.
 * Called from ResizeWindow() if resize operation started with
 * a SELECT ButtonPress on a resize corner;
 * Called from PollResizeWindow() otherwise (resize started with
 * a menu button or accelerator).  In this case, the constraint
 * variables, WMminx, WMmaxx, WMminy, and WMmaxy are set to 0
 * in ResizeWindow() and not reset to 0 here.
 */

static void
SetLimits OLARGLIST((wm, x, y, minx, maxx, miny, maxy, piece))
	OLARG( WMStepWidget,	wm)
	OLARG( int,	x)
	OLARG( int,	y)
	OLARG( int,	minx)
	OLARG( int,	maxx)
	OLARG( int,	miny)
	OLARG( int,	maxy)
	OLGRA( WMPiece,		piece)
{
Position DragX;
Position DragY;
Screen *screen = XtScreen(wm);
Position screenht = HeightOfScreen(screen);
Position screenwid = WidthOfScreen(screen);

	if (piece == WM_L || piece == WM_R) {
		wm-> wmstep.dragX = DragX = x;
	}
	else
   		if (piece == WM_T || piece == WM_B) {
			wm-> wmstep.dragY = DragY = y;
   		}
     		else {
			wm-> wmstep.dragX = DragX = x;
			wm-> wmstep.dragY = DragY = y;
     		}

/* Keep in mind, the 4 values being set refer to the min. and max. position
 * of the BORDER that is being dragged, NOT the x,y position of the
 * upper left corner of the window (though that is related...
 */

	if (minx)
		WMminx = DragX + minx;
	if(maxx)
		WMmaxx = DragX + maxx;
	/* If dragging with upper  right or lower right, don't allow to drag all the
	 * way to the edge - restrict by the Offset(wm).
	 */
	if (piece == WM_NE || piece == WM_SE || piece == WM_R) {
		if (WMminx < Offset(wm))
   			WMminx = Offset(wm);
		if (maxx == 0)
			WMmaxx = screenwid - 1;
	}


	if ( (piece == WM_NW || piece == WM_SW || piece == WM_L) &&
				( WMmaxx > (screenwid - Offset(wm)) ) )
   		WMmaxx = screenwid - Offset(wm);

	if(miny)
		WMminy = DragY + miny;

	if(maxy)
		WMmaxy = DragY + maxy;

	if  (piece == WM_SW || piece == WM_SE || piece == WM_B) {
			if (WMminy < Offset(wm))
   				WMminy = Offset(wm);
			if (maxy = 0)
				WMmaxy = screenht - 1;
	}


	if (piece == WM_NE || piece == WM_NW || piece == WM_T) {
			if ( WMmaxy > (screenht - Offset(wm)))
   				WMmaxy = screenht - Offset(wm);
	}

FPRINTF((stderr,"\n\n limits: min %dx%d max %dx%d\n", 
         WMminx, WMminy, WMmaxx, WMmaxy));

} /* end of SetLimits */

extern void
SetupWindowOpCursors OLARGLIST((display))
OLGRA(Display *, display)
{
	fleur_cursor = XCreateFontCursor(display, XC_fleur);
	ul_cursor = XCreateFontCursor(display, XC_top_left_corner);
	ur_cursor = XCreateFontCursor(display, XC_top_right_corner);
	ll_cursor = XCreateFontCursor(display, XC_bottom_left_corner);
	lr_cursor = XCreateFontCursor(display, XC_bottom_right_corner);
	l_cursor = XCreateFontCursor(display, XC_left_side);
	r_cursor = XCreateFontCursor(display, XC_right_side);
	t_cursor = XCreateFontCursor(display, XC_top_side);
	b_cursor = XCreateFontCursor(display, XC_bottom_side);
} /* SetupWindowOpCursors */

#define MAX_FEEDBACK_CHARS	11
#define FEEDBACK_BORDER_WIDTH	1
/* Size the window to have the widest possible string - 
 * 4 places for each dimension or position - actually
 * the following isn't possible, but I think an 'x' is
 * wider than a ','.  And 3 seems like a wide-looking digit,
 * without going through the width of each digit.
 */
#define BIG_FEEDBACK_STRING	"(-3333x-3333)"
/*
 * CreateMotifFeedbackWindow.  Map it when needed, unmap it when
 * not needed - start with it unmapped.
 */
extern void
CreateMotifFeedbackWindow OLARGLIST((dsp, scr))
OLARG(Display *, dsp)
OLGRA(Screen *, scr)
{
Window	parent = RootWindowOfScreen(scr);
int	scrwid = WidthOfScreen(scr);
int	scrht = HeightOfScreen(scr);
int	bgpixel = wmrcs->backgroundColor;
int	winwidth,
	winht;
int	textwidth;
XFontStruct *font;
MotifCompResourceInfo	*mcri = (MotifCompResourceInfo *)
				&(motCompRes[FEEDBACK_COMP]);
MotCompAppearanceInfo	*mcai = (MotCompAppearanceInfo *) &(mcri->compai);
unsigned long		valuemask;
XSetWindowAttributes	attributes;


	if (mcai->fontList) {
		winwidth = OlTextWidth(mcai->fontList,
			(unsigned char *)BIG_FEEDBACK_STRING,
			strlen(BIG_FEEDBACK_STRING));
		winht = mcai->fontList->max_bounds.ascent +
			mcai->fontList->max_bounds.descent;
	}
	else
		if (ol_font_list) 	{
			winwidth = OlTextWidth(ol_font_list,
				(unsigned char *)BIG_FEEDBACK_STRING,
					strlen(BIG_FEEDBACK_STRING));
			winht = ol_font_list->max_bounds.ascent +
				ol_font_list->max_bounds.descent;
		}
	else {
		int		dummy, ascent, descent;
		XCharStruct	overall;

		if (wmrcs->font == NULL)
			/* shouldn't happen */
			wmrcs->font = _OlGetDefaultFont(Frame, OlDefaultFixedFont);

		XTextExtents(wmrcs->font, BIG_FEEDBACK_STRING,
			strlen(BIG_FEEDBACK_STRING), &dummy,
			&ascent, &descent, &overall);
		winwidth = overall.width;
		winht = ascent + descent;
	} /* else */
	winwidth += 2 * FEEDBACK_BORDER_WIDTH;
	winht += 4*FEEDBACK_BORDER_WIDTH;
	
	mfd.width = winwidth;
	mfd.height = winht;
	mfd.x = scrwid/2 - mfd.width/2;
	mfd.y = scrht/2 - mfd.height/2;
	mfd.window = XCreateSimpleWindow(dsp, parent, mfd.x, mfd.y,
			mfd.width, mfd.height, 0, 0, bgpixel);
	/* Map window when needed, then unmap it */

	/* Make it a saveunder window */
	valuemask = CWSaveUnder;
	attributes.save_under = TRUE;
	XChangeWindowAttributes (dsp, mfd.window, valuemask, &attributes);


} /* CreateMotifFeedbackWindow */

/*
 * DestroyMotifFeedbackWindow
 */
extern void
DestroyMotifFeedbackWindow OLARGLIST((dsp, scr))
OLARG(Display *, dsp)
OLGRA(Screen *, scr)
{
	if (currentGUI == OL_MOTIF_GUI && mfd.window != (Window)NULL)
		XDestroyWindow(dsp, mfd.window);
	mfd.window = (Window)NULL;
}

/*
 * UpdateMotifFeedbackWindow.
 * m_or_r  =  OP_MOVE, if in the middle of a move operation,
 *	   = OP_RESIZE, if in the middle of a resize operation.
 *  If moving, then arg2 = x, else arg2 = width;
 *	if moving, then arg3 = y, else arg3 = height.
 */

static void
UpdateMotifFeedbackWindow OLARGLIST((wm, x_or_width, y_or_height, m_or_r))
OLARG(WMStepWidget, wm)
OLARG(int, x_or_width)
OLARG(int, y_or_height)
OLGRA(int, m_or_r)
{
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
Display *display = XtDisplay(wm);
char	text[14]; 
GC	text_gc;
int	textarea ;
OlgTextLbl	labeldata;
Dimension	labelwidth, labelheight;
MotifCompResourceInfo	*mcri = (MotifCompResourceInfo *)
					&(motCompRes[FEEDBACK_COMP]);
MotCompAppearanceInfo	*mcai = (MotCompAppearanceInfo *) &(mcri->compai);
XGCValues       values;


	switch(m_or_r) {
		default:
			break;
		case OP_MOVE:
			/* The '(' is left justified, the ')' is
			 * right justified, and the number (x,y)
			 * is centered in the middle of the window,
			 * separated by a comma.  Watch.
			 */
			sprintf((char *)&(text[0]),
				"(%4d,%-4d)\0",x_or_width,y_or_height);
			break;
		case OP_SIZE:
			/* No parenthesis, just center it and separate
			 * with a small x.
			 */
			sprintf((char *)&(text[0]),
				"%4dx%-d\0",x_or_width,y_or_height);
			break;
	} /* switch (m_or_r) */
		
	labeldata.font_list = NULL;
	labeldata.font = NULL;
	if (mcai->fontList) {
		labeldata.font_list = mcai->fontList;
		labeldata.font = wmrcs->font;
	}
	else
		if (ol_font_list) 	{
			labeldata.font_list = ol_font_list;
			labeldata.font = wmrcs->font;
		}
	else {
		if (wmrcs->font == NULL)
			/* shouldn't happen */
			wmrcs->font = _OlGetDefaultFont(Frame, OlDefaultFixedFont);
		labeldata.font = wmrcs->font;
	} /* else */

	values.foreground = mcai->foreground;
	values.font = wmrcs->font->fid;
	text_gc = XtGetGC(Frame, 
		(unsigned) GCFont | GCForeground , &values);
	labeldata.normalGC = text_gc;

	/* Unused here, but set to prevent problems */
	labeldata.inverseGC = text_gc;

	labeldata.accelerator = NULL;
	labeldata.mnemonic = NULL;
	labeldata.flags = (unsigned char) NULL;
	labeldata.label = (char *)text;
	labeldata.justification = TL_CENTER_JUSTIFY;

	textarea = mfd.width - 2;

	/* We figured out the label height from
	 * OlFontHeight() in InitIconDims; we will
	 * place the label in this area.
	 */
	XClearWindow(display, mfd.window);
	OlgDrawTextLabel(XtScreen(wm), mfd.window, wmstep->label_mdm, 
			FEEDBACK_BORDER_WIDTH,
			FEEDBACK_BORDER_WIDTH + 1,
			mfd.width - 2 * FEEDBACK_BORDER_WIDTH,
			mfd.height - 4 * FEEDBACK_BORDER_WIDTH,
			&labeldata);
	/* Redraw the border around the window - probably not necessary
	 * every time, but do it for now.
	 */
        _OlgDrawBorderShadow(XtScreen((Widget)wm), mfd.window,
	  wm->wmstep.mdm, OL_SHADOW_OUT, 
	  1, 0, 0, mfd.width, mfd.height,
	  mcri->topGC, mcri->botGC);
	XtReleaseGC(Frame, text_gc);
	text_gc=NULL;
} /* UpdateFeedbackWindow */

