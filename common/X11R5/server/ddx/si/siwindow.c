/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/siwindow.c	1.6"

/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 *	All rights reserved.
 */

#include "X.h"
#include "scrnintstr.h"

#ifdef XWIN_SAVE_UNDERS
#include "Xproto.h"
#include "validate.h"
#include "windowstr.h"
#include "mistruct.h"
#include "regionstr.h"
#include "sisave.h"
static void siFakeComputeBorderClip();
#else
#include "windowstr.h"
#include "mistruct.h"
#include "regionstr.h"
#endif

extern WindowPtr *WindowTable;

Bool
siCreateWindow(pWin)
register WindowPtr pWin;
{
    return (TRUE);
}

Bool 
siDestroyWindow(pWin)
WindowPtr pWin;
{
    return (TRUE);
}

Bool 
siMapWindow(pWindow)
WindowPtr pWindow;
{
#ifdef XWIN_SAVE_UNDERS
    RegionRec saveReg;
    ScreenPtr pScreen;

    pScreen = pWindow->drawable.pScreen;

    /*
     * First determine whether we need to proceed or not.
     *
     * If save-unders are not supported for this screen,
     * OR there aren't any save-under windows,
     * OR pWindow is not viewable
     * OR pWindow has no parent (root window)
     * OR pWindow has an empty borderSize
     * OR pWindow is not a save-under window and doesn't
     *    intersect any save-under windows,
     *
     * then simply return.
     */
    if ((!SUSupport(pScreen)) ||
        (!(SUWindowsMapped() || pWindow->saveUnder == TRUE)) ||
 	(!(pWindow->viewable)) ||
        (!(pWindow->parent)) ||
        (REGION_NUM_RECTS(&pWindow->borderSize) == 0) || 
        (((pWindow->saveUnder != TRUE) &&
          !SUCheckDrawable((DrawablePtr) pWindow))))
    {
        return (TRUE);
    }

    /*
     * See if "pWindow" is obstructed by any of the currently mapped
     * save-under windows.  Since the window hasn't TRULY been mapped
     * yet, we must pre-compute the window's borderClip by calling 
     * siFakeComputeBorderClip().
     */
    saveReg = pWindow->borderClip;	/* save whatever is there */
    (* pScreen->RegionInit)(&pWindow->borderClip, NullBox, 0);
    siFakeComputeBorderClip(pWindow, &pWindow->borderClip);

    if (SUCheckDrawable((DrawablePtr)pWindow))
    {
        siSUScanWindows(pWindow, IncludeInferiors, NULL, NULL);
    }

    /*
     * If "pWindow" has the save-under attribute set, then call
     * siSUMapWindow() to add it to the list of currently mapped
     * save-under windows.
     */ 
    if (pWindow->saveUnder == TRUE)
         siSUMapWindow(pWindow);

    /*
     * Free fake borderClip and restore the saved clip.
     */
    (* pScreen->RegionUninit)(&pWindow->borderClip);
    pWindow->borderClip = saveReg;

#endif /* XWIN_SAVE_UNDERS */

    return (TRUE);
}

/* (x, y) is the upper left corner of the window on the screen 
   do we really need to pass this?  (is it a;ready in pWin->absCorner?)
   we only do the rotation for pixmaps that are 32 bits wide (padded
or otherwise.)
   mfbChangeWindowAttributes() has already put a copy of the pixmap
in pPrivWin->pRotated*
*/

Bool 
siPositionWindow(pWin, x, y)
register WindowPtr pWin;
int x, y;
{
#ifdef XWIN_SAVE_UNDERS
    RegionRec saveReg;
    ScreenPtr pScreen;

#ifdef BUG
    if (SUWindowsMapped() && pWin->viewable &&
        ((pWin->valdata->before.oldAbsCorner.x != pWin->drawable.x) ||
         (pWin->valdata->before.oldAbsCorner.y != pWin->drawable.y)))
#else
    if (SUWindowsMapped() && pWin->viewable)
#endif
    {
	/*
         * See if former location conflicts with (non-inferior) save-under
         * windows.  Unfortunately, the window size/location (borderSize) has
         * already been updated ... thus, we must recreate its prior location
         * from the not-yet-updated borderClip (oh well!).
         */ 
        saveReg = pWin->borderSize;
        pScreen = pWin->drawable.pScreen; 
	(* pScreen->RegionInit)(&pWin->borderSize, &(pWin->borderClip.extents), 0);
        if (SUCheckDrawable((DrawablePtr)pWin))
        {
            siSUScanWindows(pWin, IncludeInferiors, NULL, NULL);
        }
	(* pScreen->RegionUninit)(&pWin->borderSize);
        pWin->borderSize = saveReg;
    }
#endif /* XWIN_SAVE_UNDERS */

    return (TRUE);
}

Bool 
siUnmapWindow(pWindow)
    WindowPtr pWindow;
{
#ifdef XWIN_SAVE_UNDERS
    if (!SUWindowsMapped())
        return (TRUE);

    if (pWindow->saveUnder == TRUE)
    {
        siSUUnmapWindow(pWindow);
    }

    /*
     * See if the un-mapped window conflicts with (non-inferior)
     * save-under windows.
     */
    if (SUCheckDrawable((DrawablePtr)pWindow))
    {
        siSUScanWindows(pWindow, IncludeInferiors, NULL, NULL);
    }
#endif /* XWIN_SAVE_UNDERS */

    return (TRUE);
}

/* 
 * UNCLEAN!
 * this code calls the bitblt helper code directly.
 * siCopyWindow copies only the parts of the destination that are
 * visible in the source.
 */
void 
siCopyWindow(pWin, ptOldOrg, prgnSrc)
WindowPtr pWin;
DDXPointRec ptOldOrg;
RegionPtr prgnSrc;
{
    DDXPointPtr pptSrc;
    register DDXPointPtr ppt;
    RegionPtr prgnDst;
    register BoxPtr pbox;
    register int dx, dy;
    register int i, nbox;
    WindowPtr pwinRoot;

    pwinRoot = WindowTable[pWin->drawable.pScreen->myNum];

    prgnDst = (* pWin->drawable.pScreen->RegionCreate)(NULL, 1);

    dx = ptOldOrg.x - pWin->drawable.x;
    dy = ptOldOrg.y - pWin->drawable.y;
    (* pWin->drawable.pScreen->TranslateRegion)(prgnSrc, -dx, -dy);
    (* pWin->drawable.pScreen->Intersect)(prgnDst, &pWin->borderClip, prgnSrc);

    pbox = REGION_RECTS(prgnDst);
    nbox = REGION_NUM_RECTS(prgnDst);
    if(!(pptSrc = (DDXPointPtr )ALLOCATE_LOCAL(nbox * sizeof(DDXPointRec))))
	return;
    ppt = pptSrc;

    for (i=nbox; --i >= 0; ppt++, pbox++)
    {
	ppt->x = pbox->x1 + dx;
	ppt->y = pbox->y1 + dy;
    }

    /* SI: START */
    siCopyBitblt(pWin, (DrawablePtr)pwinRoot, (DrawablePtr)pwinRoot, prgnDst, pptSrc);
    /* SI: END */

    DEALLOCATE_LOCAL(pptSrc);
    (* pWin->drawable.pScreen->RegionDestroy)(prgnDst);
}


Bool
siChangeWindowAttributes(pWin, mask)
register WindowPtr pWin;
register unsigned long mask;
{
#ifdef XWIN_SAVE_UNDERS
     register unsigned long index;

     while (mask)
     {
	index = lowbit (mask);
	mask &= ~index;

	if (index == CWSaveUnder)
	{
              /*
               * If save_under is set to True, the attribute will be
               * ignored until the window is mapped (MapWindow()).
               * We need only worry about turning OFF save-unders for
               * those windows that are currently mapped.
               */
              if (pWin->saveUnder == xFalse)
              {
                  siSUDeleteWindow(pWin);
              }
	}
     }
#endif /* XWIN_SAVE_UNDERS */

    return (TRUE);
}

#ifdef XWIN_SAVE_UNDERS
static void 
siFakeComputeBorderClip(pWindow, pBorderClip)
WindowPtr pWindow;
RegionPtr pBorderClip;
{
    ScreenPtr pScreen;
    WindowPtr pParent, pChild;
    Bool (* Subtract)();

    pScreen = pWindow->drawable.pScreen;
    Subtract = pScreen->Subtract;

    /* 
     * Generate pWindow's border clip by first computing the
     * intersection of pWindow's borderSize and its parents
     * borderClip.
     */
    pParent = pWindow->parent;
    if (REGION_NUM_RECTS(&pParent->borderClip) > 0)
    {
        (* pScreen->Intersect)(pBorderClip, &pParent->borderClip,
    	    &pWindow->borderSize);
    }
    else
    {
        (* pScreen->RegionCopy)(pBorderClip, &pWindow->borderSize);
    }


    /*
     * Finally, subtract from pBorderClip those sibling windows that
     * are higher in the stacking order.
     */
    for (pChild = pParent->firstChild; pChild; pChild = pChild->nextSib)
    {
	if (pChild == pWindow)
	    break;
	if (pChild->viewable)
	{
	    (* Subtract)(pBorderClip, pBorderClip, &pChild->borderSize);
	}
    }
}
#endif /* XWIN_SAVE_UNDERS */

void 
miClearToBackground(pWin, x, y, w, h, generateExposures)
    WindowPtr pWin;
    short x,y;
    unsigned short w,h;
    Bool generateExposures;
{
    BoxRec box;
    RegionRec	reg;
    RegionPtr pBSReg = NullRegion;
    ScreenPtr	pScreen;
#ifdef ASSERTION_BUG
    BoxPtr  extents;
    int	    x1, y1, x2, y2;

    /* compute everything using ints to avoid overflow */

    x1 = pWin->drawable.x + x;
    y1 = pWin->drawable.y + y;
    if (w)
        x2 = x1 + (int) w;
    else
        x2 = x1 + (int) pWin->drawable.width - (int) x;
    if (h)
        y2 = y1 + h;	
    else
        y2 = y1 + (int) pWin->drawable.height - (int) y;


    extents = &pWin->clipList.extents;
    
    /* clip the resulting rectangle to the window clipList extents.  This
     * makes sure that the result will fit in a box, given that the
     * screen is < 32768 on a side.
     */

    if (x1 < extents->x1)
	x1 = extents->x1;
    if (x2 > extents->x2)
	x2 = extents->x2;
    if (y1 < extents->y1)
	y1 = extents->y1;
    if (y2 > extents->y2)
	y2 = extents->y2;

    box.x1 = x1;
    box.x2 = x2;
    box.y1 = y1;
    box.y2 = y2;
#else
    box.x1 = pWin->drawable.x + x;
    box.y1 = pWin->drawable.y + y;
    if (w)
        box.x2 = box.x1 + (int) w;
    else
        box.x2 = box.x1 + (int) pWin->drawable.width - (int) x;
    if (h)
        box.y2 = box.y1 + h;	
    else
        box.y2 = box.y1 + (int) pWin->drawable.height - (int) y;
#endif

    pScreen = pWin->drawable.pScreen;
    (*pScreen->RegionInit) (&reg, &box, 1);
    if (pWin->backStorage)
    {
	/*
	 * If the window has backing-store on, call through the
	 * ClearToBackground vector to handle the special semantics
	 * (i.e. things backing store is to be cleared out and
	 * an Expose event is to be generated for those areas in backing
	 * store if generateExposures is TRUE).
	 */
	pBSReg = (* pScreen->ClearBackingStore)(pWin, x, y, w, h,
						 generateExposures);
    }

    (* pScreen->Intersect)(&reg, &reg, &pWin->clipList);
    if (generateExposures)
	(*pScreen->WindowExposures)(pWin, &reg, pBSReg);
    else if (pWin->backgroundState != None)
#ifdef XWIN_SAVE_UNDERS
    {
	/*
         * Before painting the window background, make sure that
         * there are no obstructing save-under windows.  This check
         * must be made BEFORE clipping occurs!
         */
        if (SUCheckDrawable((DrawablePtr) pWin))
        {
            siSUScanWindows(pWin, ClipByChildren, NULL, NULL);
        }
    }
#endif

        (*pScreen->PaintWindowBackground)(pWin, &reg, PW_BACKGROUND);
    (* pScreen->RegionUninit)(&reg);
    if (pBSReg)
	(* pScreen->RegionDestroy)(pBSReg);
}
