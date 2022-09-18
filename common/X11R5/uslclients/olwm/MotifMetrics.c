/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olwm:MotifMetrics.c	1.4"
#endif

#include <stdio.h>

#include <X11/keysym.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/ShellP.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>

#include <Xol/OpenLookP.h>
#include <Xol/DynamicP.h>
#include <Xol/Stub.h>
#include <Xol/Olg.h>
#include <Xol/WSMcomm.h>
#include <Xol/Flat.h>

#include <wm.h>
#include <Xol/OlCursors.h>
#include <Xol/VendorI.h>
#include <WMStepP.h>

#include <Extern.h>


/* Some metric related functions used in motif mode only */
extern int	MBorderX	OL_ARGS((WMStepWidget));
extern int	MBorderY	OL_ARGS((WMStepWidget));
extern int	MParentHeight	OL_ARGS((WMStepWidget, int));
extern int	MOriginX	OL_ARGS((WMStepWidget));
extern int	MOriginY	OL_ARGS((WMStepWidget));

extern int	MChildWidth	OL_ARGS((WMStepWidget));
extern int	MChildWidth	OL_ARGS((WMStepWidget));
extern int	MChildHeight	OL_ARGS((WMStepWidget));
extern int	MNewChildWidth	OL_ARGS((WMStepWidget, int));
extern int	MNewChildHeight	OL_ARGS((WMStepWidget, int));


extern int  
MParentWidth OLARGLIST((wm, k))
OLARG(WMStepWidget, wm)
OLGRA(int, k)
{
int hwidth, ret;
	if (Resizable(wm)) {
		hwidth = wm->wmstep.metrics->motifHResizeBorderWidth;
	}
	else {
		hwidth = (wm->wmstep.decorations & WMBorder) ?
		   wm->wmstep.metrics->motifHNResizeBorderWidth:
		     wm->wmstep.prev.border_width;
	}

	/* The extra 2 is for the inner border surrounding the client.
	 * It could be a factor for the width - this border is
	 * vertically aligned with the edge of the outer buttons -
	 * the menu button and the maximize button - but it also may
	 * depend on the context of how this is used.  For example,
	 * you have to have that extra inner border around the client,
	 * accounting for 2 pixels; it's true that it aligns with the upper
	 * buttons, such as the menu button and the maximize button, but
	 * those are drawn as offsets to the border, not the client!
	 * Also, you don't size the parent (step widget) based on the
	 * button widths, but you do so based on the client - the button
	 * widths are used to determine the MINIMUM width.
	 */


	return ((wm->wmstep.decorations & WMBorder || Resizable(wm)) 
		? k + 2 * hwidth + 2 : k + 2 * hwidth);
/*
	return( k + 2 * hwidth + 2);
*/
} /* MParentWidth() */

extern int  
MParentHeight OLARGLIST((wm, k))
OLARG(WMStepWidget, wm)
OLGRA(int, k)
{
int vwidth, ret;
WMStepPart *wmstep = &(wm->wmstep);
	if (Resizable(wm)) {
		vwidth = wmstep->metrics->motifVResizeBorderWidth;
	}
	else {
		vwidth = wmstep->decorations & WMBorder ?
			wmstep->metrics->motifVNResizeBorderWidth:
			  wmstep->prev.border_width;
	}
	/* The extra 2 is for the inner border surrounding the client.
	 * It isn't a factor for the width because this border is
	 * vertically aligned with the edge of the outer buttons -
	 * the menu button and the maximize button.
	 */
	if (wmstep->decorations & WMBorder || Resizable(wm) || Header(wm))
		ret = k + 2 * vwidth + 2;
	else ret = k + 2 * vwidth;

	if (Header(wm))
		return(ret + wmstep->metrics->motifButtonHeight);
	else return (ret);
} /* MParentHeight */

/*
 * MBorderX: width of horizontal border - same as MOriginX
 */
extern int
MBorderX OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
	return(MOriginX(wm));
} /* MBorderX */

/*
 * MBorderY - same value as MBorderX (no difference between horizontal
 * and vertical values.
 */
extern int
MBorderY OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
	return(MOriginX(wm));
} /* MBorderY */

extern int
MOriginX OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
int hwidth;
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
	if (Resizable(wm)) {
		hwidth = wm->wmstep.metrics->motifHResizeBorderWidth;
	}
	else {
		hwidth = wmstep->decorations & WMBorder ?
			wm->wmstep.metrics->motifHNResizeBorderWidth:
			  wmstep->prev.border_width;
	}
	if (wmstep->decorations & WMBorder)
		return( hwidth + 1);
	else
/*
		return (hwidth);
 */
		return (0);
} /* MOriginX */

extern int
MOriginY OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
WMStepPart	*wmstep = (WMStepPart *)&(wm->wmstep);
int		vwidth;

	if (Resizable(wm)) {
		vwidth = wm->wmstep.metrics->motifHResizeBorderWidth;
	}
	else {
		vwidth = wm->wmstep.metrics->motifHNResizeBorderWidth;
	}
	if (wmstep->decorations & WMBorder) {
		if (Header(wm))
		   return(vwidth + 1 + wmstep->metrics->motifButtonHeight);
		else
		   return(vwidth + 1); 
	} /* Has border */
	else /* No border */
		if (Header(wm))
			return(2 + wmstep->metrics->motifButtonHeight);
		else
			return(0);
	/* If there is no border, then if there is a header,
	 * a shadow of thickness == 1 gets drawn around the title button.
	 */
} /* MOriginY */

extern int
MChildWidth OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
int hwidth;
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
	if (Resizable(wm)) {
		hwidth = wm->wmstep.metrics->motifHResizeBorderWidth;
	}
	else {
		hwidth = wm->wmstep.metrics->motifHNResizeBorderWidth;
	}
	if (wmstep->decorations & WMBorder)
		return(wm->core.width - 2 * hwidth - 2);
	else
		return (wm->core.width);
	/* If no border, then the core width is simply the width of
	 * the client + 2 * the clients original border width 
	 */
} /* MChildWidth */

extern int
MChildHeight OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
int vwidth;
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
	if (Resizable(wm)) {
		vwidth = wm->wmstep.metrics->motifVResizeBorderWidth;
	}
	else {
		vwidth = wm->wmstep.metrics->motifVNResizeBorderWidth;
	}
	if (wmstep->decorations & WMBorder) {
	  if (Header(wm))
		return(wm->core.height - 2 * vwidth - 2 -
			wm->wmstep.metrics->motifButtonHeight);
	  else
		return(wm->core.height - 2 * vwidth - 2);
	}
	else	/* No Border */
		/* If no header, then the core height equals the
		 * client height + the border around it; if header,
		 * then the core ht. == the client ht. + border around
		 * it + 2 + the button height.
		 */
		if (Header(wm))
			return(wm->core.height - 2 -
			wm->wmstep.metrics->motifButtonHeight);
		else
			return(wm->core.height);
		

} /* MChildHeight */

/* MNewChildWidth().  If core.width == width, then what would the
 * client window width be??
 */
extern int
MNewChildWidth OLARGLIST((wm, width))
OLARG(WMStepWidget, wm)
OLGRA(int, width)
{
int hwidth;
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
	if (!(wmstep->decorations & WMBorder))
		return(width); 
	if (Resizable(wm)) {
		hwidth = wm->wmstep.metrics->motifHResizeBorderWidth;
	}
	else {
		hwidth = wm->wmstep.metrics->motifHNResizeBorderWidth;
	}
		return(width - 2 * hwidth - 2);
} /* MNewChildWidth */

extern int
MNewChildHeight OLARGLIST((wm, height))
OLARG(WMStepWidget, wm)
OLGRA(int, height)
{
int vwidth;
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);

	if (Resizable(wm)) {
		vwidth = wm->wmstep.metrics->motifVResizeBorderWidth;
	}
	else {
		vwidth = wm->wmstep.metrics->motifVNResizeBorderWidth;
	}
	if (wmstep->decorations & WMBorder) {
	  if (Header(wm))
		return(height - 2 * vwidth - 2 -
			wm->wmstep.metrics->motifButtonHeight);
	  else
		return(height - 2 * vwidth - 2);
	}
	else { /* no border */
	  if (Header(wm))
		return(height - 2 - wmstep->metrics->motifButtonHeight);
	  else
		return(height);
	}
} /* MNewChildHeight */
