/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olwm:Icon.c	1.12"
#endif

/*
 *************************************************************************
 *
 * Description:
 *	This file contains icon related procedures.
 *
 ******************************file*header********************************
 */

#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>
#include <wm.h>
#include <Xol/VendorI.h>
#include <WMStepP.h>
#include <Extern.h>

/*
 *************************************************************************
 *
 * Forward Procedure declarations
 *
 **************************forward*declarations***************************
 */

extern WMGeometry *  IconPosition OL_ARGS((WMStepWidget, WMStepPart *));
static void          NextIconPosition OL_ARGS((Screen *, int *, int *,
					int, int, int));
extern void		PackIcons OL_ARGS((Widget));
extern void          RecordPosition OL_ARGS((WMStepWidget, WMGeometry *));
extern void          ResetIconPosition OL_ARGS((Screen *));

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */


/*
 *************************************************************************
 *
 * Procedures
 *
 ***************************private*procedures****************************
 */

/*
 * IconPosition
 * Called from Initialize() if window is initially brought up in
 * -iconic;
 * 	- from MenuOpenClose() if sent from normal or full size to iconic.
 * Return pointer to a WMGeometry struct (wmP.h) - it has 5 fields,
 *  x,y, width, height, and border_width.  If wmstep part of
 * wm widget has NULL icon field, malloc(), fill in struct.
 * This function, in effect gives a client a piece of real estate
 * on the screen, identified in wmstep->icon, where the clients icon may
 * appear on the screen.
 */

extern WMGeometry *
IconPosition(wm, wmstep)
WMStepWidget wm;
WMStepPart * wmstep;
{
WMGeometry * p = wmstep-> icon;

if (p == NULL)
   {
   p = wmstep-> icon = (WMGeometry *) XtMalloc(sizeof(WMGeometry));
   if ( (wmstep-> hints & WMHints) &&
	(wmstep->xwmhints->flags & IconPositionHint) ) {
		p->x = wmstep->xwmhints->icon_x;
		p->y = wmstep->xwmhints->icon_y;
		/* Supply width, height too -
		 * but these are different depending on mode.
		 */
		if (currentGUI == OL_MOTIF_GUI) {
			p-> width = icon_width;
			p-> height = icon_height;
		}
		else {
			p-> width = XIconIncrement;
			p-> height = YIconIncrement;
		}
   }
   else {
	  if (currentGUI == OL_OPENLOOK_GUI) {
		p-> x = CurrentIconX;
		p-> y = CurrentIconY;
		/* Update global variables where the NEXT icon will
		 * be placed
		 */
		NextIconPosition(XtScreen(wm), &CurrentIconX, &CurrentIconY, 
			XIconIncrement, YIconIncrement, wmrcs->iconGravity);
	p-> width = XIconIncrement;
	p-> height = YIconIncrement;
	   }
	   else { /* Motif */

		if (motwmrcs->iconAutoPlace)
			ConsumeIconPosition(wm, &p);
		else {
			/* Should only be called from Initialize() so
			 * we get the correct coordinates.
			 */
			p->x = wm->core.x + MOriginX(wm);
			p->y = wm->core.y + MOriginY(wm);
			p->width = mot_icon_iwidth,
			p->height = mot_icon_iheight + mot_icon_label_height;
		} /* else !motwmrcs->iconAutoPlace */
		if (p->x == NO_ICON_ROOM) {
			XtFree((char *)p);
			p = wmstep->icon =  NULL;
		}
	   } /* Motif */
	} /* else (iconPositionHint not supplied) */
  } /* p == NULL */
  else
	/* p != NULL , so this widget has been iconified at some time
	 * in the past - OpenLook, no problem; motif:  is the spot
	 * already taken where it was?? if so, we need a new spot;
	 * else give me the old spot again - consume it.
	 */
	if (currentGUI == OL_MOTIF_GUI && motwmrcs->iconAutoPlace) {
		/* Confirm: if old spot is taken, then call
		 * ConsumeIconPosition
		 */
		ConfirmIconPosition(wm, &p);
		if (p->x == NO_ICON_ROOM) {
			/* You're stuck - no more room!  Free the spot,
			 * reset position variables.
			 */
			XtFree((char *)p);
			p = wmstep->icon =  NULL;
			wm->wmstep.icon_map_pos_row =
			 wm->wmstep.icon_map_pos_col = -1;
		}
	}
return(p);

} /* end of IconPosition */

/*
 * NextIconPosition
 * - Called from IconPosition() only - determine next place on screen
 * where an icon may be placed.  
 * x = CurrentIconX, y = CurrentIconY.  These get updated.
 * xi = XIconIncrement, yi = YIconIncrement.
 */

static void
NextIconPosition OLARGLIST((screen, x, y, xi, yi, direction))
OLARG(Screen *, screen)
OLARG(int *, x)
OLARG(int *, y)
OLARG(int, xi)
OLARG(int, yi)
OLGRA(int, direction)
{
int newx = *x;
int newy = *y;
int width = WidthOfScreen(screen);
int height = HeightOfScreen(screen);

if (currentGUI == OL_MOTIF_GUI) {
	return;
}

switch (direction)
   {
   case WMNorthGravity:
      newx += xi;
      if (newx  + xi > width)
         { 
         newx = 0; 
         newy += yi;
         if (newy > height)
            newy = 0;
         }
      break;
   case WMSouthGravity:
      newx += xi;
      if (newx  + xi > width)
         { 
         newx = 0; 
         newy -= yi;
         if (newy < 0)
            newy = height - yi;
         }
      break;
   case WMEastGravity:
      newy += yi;
      if (newy + yi > height)
         { 
         newy = 0; 
         newx -= xi;
         if (newx < 0)
            newx = width - xi;
         }
      break;
   case WMWestGravity:
      newy += yi;
      if (newy + yi > height)
         { 
         newy = 0; 
         newx += xi;
         if (newx > width)
            newx = 0;
         }
      break;
   }

*x = newx;
*y = newy;

} /* end of NextIconPosition */

/*
 * RecordPosition
 *
 * Called from MenuOpenClose().
 * Get current position of WMStepWidget, returning complete geometry.
 * MenuOpenClose() saves the position (in wmstep->prev) when going
 * from normal to iconic.
 */

extern void
RecordPosition OLARGLIST((wm, p))
OLARG(WMStepWidget, wm)
OLGRA(WMGeometry *, p)
{
Widget w = IsIconic(wm) ? wm->wmstep.icon_widget : (Widget)wm;

p-> x = w-> core.x;
p-> y = w-> core.y;
p-> width = w-> core.width;
p-> height = w-> core.height;
p-> border_width = w-> core.border_width;

} /* end of RecordPosition */

/*
 * ResetIconPosition
 * - Called from SetupWindowManager() and WMDynamic(). 
 * Reset coordinates of next icon to "start over" (like the first one)
 * based on icon gravity.  Nothing to do for motif mode.
 */
extern void
ResetIconPosition OLARGLIST((screen))
OLGRA(Screen *, screen)
{

	if (currentGUI == OL_MOTIF_GUI)
		/* nothing to do */
		return;
	XIconIncrement = icon_width;
	YIconIncrement = icon_height;

switch (wmrcs->iconGravity)
   {
   case WMNorthGravity:
      CurrentIconX = 0;
      CurrentIconY = 0;
      break;
   case WMSouthGravity:
      CurrentIconX = 0;
      CurrentIconY = HeightOfScreen(screen) - YIconIncrement;
      break;
   case WMEastGravity:
      CurrentIconX = WidthOfScreen(screen) - XIconIncrement;
      CurrentIconY = 0;
      break;
   case WMWestGravity:
      CurrentIconX = 0;
      CurrentIconY = 0;
      break;
   } /* switch */
} /* end of ResetIconPosition */

/*
 * PackIcons.
 *	Loop through window list, reset icon positions according to gravity.
 */
extern void
PackIcons OLARGLIST((w))
OLGRA(Widget, w)
{
Screen		*screen = XtScreen(w);
WMStepWidget	wm;
WMStepPart	*wmstep;
WMGeometry	*p;
int		x, y, width, height;
int		i;

	if (currentGUI == OL_MOTIF_GUI) {
		/*
		fprintf(stderr,"****mlp - PackIcons not done yet for motif mode****\n");
		 */
		return;
	}
	ResetIconPosition(screen);
	for (i=0; i < window_list->used; i++) {
		wm = (WMStepWidget)window_list->p[i];
		wmstep = (WMStepPart *)&(wm->wmstep);
		if (wmstep->icon) {
			/* Save old (previous) icon position */
			x = wmstep->icon->x;
			y = wmstep->icon->y;
			width = wmstep->icon->width;
			height = wmstep->icon->height;
			XtFree((char *)(wmstep->icon));
			wmstep->icon = NULL;
		}
		if (wmstep->icon_widget) {
			/* currently iconic - let's repack this icon */
			/* Move the icon to its (maybe) new spot.
			 */
			p = IconPosition(wm, wmstep);
			XtMoveWidget(wmstep->icon_widget, wmstep->icon->x,
				wmstep->icon->y);
		} /* if wmstep->icon_widget */
	} /* for */
} /* PackIcons */
