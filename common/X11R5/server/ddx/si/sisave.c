/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/sisave.c	1.7"

/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyright (c) 1988, 1989, 1990 AT&T
 *	All Rights Reserved 
 */

#include "X.h"
#include "misc.h"
#include "miscstruct.h"
#include "regionstr.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "servermd.h"

#include "gcstruct.h"
#include "si.h"
#include "sisave.h"

extern void miSpriteRemoveCursor ();
extern void miSpriteRestoreCursor ();

#define ADD        0x1
#define SUBTRACT   0x2

extern WindowPtr *WindowTable;

/*
 * SUWindows: points to the list of all currently mapped save-under windows.
 * SURegion : represents the UNION of all "uncorrupted" portions of windows
 *            pointed to by SUWindows.
 */
SUWindowPtr  SUWindows = ((SUWindowPtr) 0);
RegionPtr    SURegion = NullRegion;

/* static functions */
static void siSUSaveImage();
static void siSUModifyRegion();
static void siSUFreeWindow();
static SUWindowPtr siSUFindWindow();

/* global functions */
void siSUMapWindow();
void siSUUnmapWindow();
void siSUDeleteWindow();
void siSUScanWindows();
void siSURestoreImage();
void siSUCheckStack();		/* see dix/window.c */
void siSUFreeImage();

#define SUBoxesAreSame(X, Y)  \
    (((X).x1 == (Y).x1) && \
     ((X).x2 == (Y).x2) && \
     ((X).y1 == (Y).y1) && \
     ((X).y2 == (Y).y2))

#define SUBoxesOverlap(X, Y) \
    (!(((X)->x2 <= (Y)->x1) || \
       ((X)->x1 >= (Y)->x2) || \
       ((X)->y2 <= (Y)->y1) || \
       ((X)->y1 >= (Y)->y2)))

/*
 * siSUMapWindow() - add 'pWin' to the list of currently
 * mapped save-under windows.
 */
void
siSUMapWindow(pWin)
WindowPtr pWin;
{
    SUWindowPtr pSUWin;
    ScreenPtr   pScreen;

    /*
     * If pWin is obstructed then do not save its image.
     * As an enhancement, one may consider adding partial
     * image saves.
     */
    if (REGION_NUM_RECTS(&pWin->borderClip) != 1)
	return;

    pScreen = pWin->drawable.pScreen;
    if (!(pSUWin = (SUWindowPtr) xalloc(sizeof(SUWindowRec))))
        FatalError("siSUMapWindow: no memory to allocate data structures");

#ifdef SAVETRACE
    ErrorF("siSUMapWindow: mapping a save-under window\n");
#endif

    /*
     * Initialize the new SUWindowRec structure ... saving the
     * window pointer, pWin, as well as its initial geometry.
     * Add the structure to the linked list pointed to by SUWindows.
     */
    pSUWin->pWin = pWin;
    pSUWin->box = pWin->borderSize.extents;
    pSUWin->pCache = NullSIbitmap;
    pSUWin->pImageClip = NullRegion;
    pSUWin->next = SUWindows;
    SUWindows = pSUWin;

    /*
     * Call siSUSaveImage() to capture the screen image and modify SURegion
     * to include the geometry of the save-under window.
     * NOTE: siSUSaveImage() also sets pSUWin->pCache and pSUWin->pImageClip.
     */
    siSUSaveImage(pSUWin);
    siSUModifyRegion(pSUWin, ADD, pSUWin->pImageClip);
}

/*
 * siSUUnmapWindow() - delete 'pWin' from the list of currently
 * mapped save-under windows.
 */ 
void
siSUUnmapWindow(pWin)
WindowPtr pWin;
{
    SUWindowPtr pSUWin, pSUPrevWin;
    SUPrivPtr     pSUPriv;
    ScreenPtr	  pScreen;

    pSUWin = pSUPrevWin = SUWindows;
    pScreen = pWin->drawable.pScreen;

#ifdef SAVETRACE
    ErrorF("siSUUnmapWindow: un-mapping a save-under window\n");
#endif

    if ((pSUWin = siSUFindWindow(pWin, &pSUPrevWin)) == (SUWindowPtr) NULL)
	return;

    /*
     * If the window hasn't changed in size or location AND
     * its parent is realized (ancestors are still mapped) then 
     * the save-under image can be restored.  Save the current
     * state of the window in the private structure of pScreen
     * so that the image is restored during the next call to
     * HandleExposures().
     */
    if (SUBoxesAreSame(pSUWin->box, pWin->borderSize.extents) &&
	pWin->parent && pWin->parent->realized)
    {
	/*
	 * Initialize the private save-under structure.  Note
	 * that the intersection of the window clip (borderClip)
	 * and the Image Clip (pImageClip) forms the "restorable" 
	 * portion of the saved image.
	 */
	pSUPriv = (SUPrivPtr) xalloc(sizeof(SUPrivRec));
	pSUPriv->pWin = pSUWin->pWin;
	pSUPriv->pCache = pSUWin->pCache;
	pSUPriv->pImageClip = pSUWin->pImageClip;
	(* pScreen->Intersect) (pSUPriv->pImageClip,
		&pWin->borderClip, pSUPriv->pImageClip);

	/*
	 * Attach private structure to appropriate screen structure
	 */
	SUPrivate(pScreen) = pSUPriv;
        siSUFreeWindow(pSUWin, pSUPrevWin, FALSE);
    }
    else
    {
        siSUFreeWindow(pSUWin, pSUPrevWin, TRUE);
    }
}

/*
 * siSUDeleteWindow() - scans SUWindows looking for 'pWin'.
 * If found, it will be removed from the linked list. 
 */
void
siSUDeleteWindow(pWin)
WindowPtr pWin;
{
    SUWindowPtr pSUWin, pSUPrevWin;

#ifdef SAVETRACE
    ErrorF("siSUDeleteWindow: deleting a save-under window\n");
#endif

    if (pSUWin = siSUFindWindow(pWin, &pSUPrevWin))
    {
        siSUFreeWindow(pSUWin, pSUPrevWin, TRUE);
    }
}

/*
 * siSUScanWindows() - Scan the list of save-under windows pointed to
 * by SUWindows ... discarding those portions of the saved images that
 * either conflict with pWin or conflict with some region/box withing pWin. 
 */
void
siSUScanWindows(pWin, subWindowMode, pCheckReg, pCheckBox)
WindowPtr pWin;
int       subWindowMode;
RegionPtr pCheckReg;
BoxPtr    pCheckBox;
{
    register	SUWindowPtr pSUWin;
    register	ScreenPtr pScreen;
    RegionPtr   pWinClip, pWinBorderSize;
    BoxPtr      pSUBox, pWinBox;
    int         (*RectIn)();
    Bool	(*Subtract)();

    /*
     * Determine which clip list to use based on subWindowMode.
     */
    switch (subWindowMode)
    {
        case IncludeInferiors:
            pWinClip = &pWin->borderClip;
            break;
        case ClipByChildren:
        default:
            pWinClip = &pWin->clipList;
            break;
    }
       
    /*
     * set up locals for frequently referenced entities.
     */
    pScreen = pWin->drawable.pScreen;
    pWinBox = &(pWin->borderSize.extents);
    pWinBorderSize = &(pWin->borderSize);
    RectIn = pScreen->RectIn;
    Subtract = pScreen->Subtract;

    for (pSUWin = SUWindows; pSUWin; pSUWin = pSUWin->next)
    {
        /*
         * The basis for this test is to determine which portions of
         * the save-under images should be discarded.
	 *
	 * In English ... IF:
	 *    1) pWin is not the save-under window being tested AND
         *    2) pWin intersects pSUWin's image clip AND
         *    3) the save-under window doesn't intersect pWinClip (this means
         *       that pWin MIGHT be underneath the save-under window) AND
         *    4) either pCheckReg (a region withing pWin) is not specified OR
         *       the save-under window intersects pCheckReg AND 
         *    5) either pCheckBox (a box withing pWin) is not specified OR
         *       the save-under window intersects pCheckBox THEN
         *
         *    Subtract pWin->borderSize from pSUWin->pImageClip AND
	 *    Subtract pWin->borderSize from SURegion
         */
        pSUBox = &(pSUWin->box);
        if ((pWin != pSUWin->pWin) &&
	    ((* RectIn)(pSUWin->pImageClip, pWinBox) != rgnOUT) &&
            ((* RectIn)(pWinClip, pSUBox) == rgnOUT) &&
            (!pCheckReg || ((* RectIn)(pCheckReg, pSUBox) != rgnOUT)) &&
            (!pCheckBox || SUBoxesOverlap(pCheckBox, pSUBox)))
        { 
#ifdef SAVETRACE
	    siSUPrintRegion(pWinBorderSize, "****SUBTRACTING");
#endif
	    (* Subtract)(pSUWin->pImageClip, pSUWin->pImageClip,pWinBorderSize);
	    siSUModifyRegion(pSUWin, SUBTRACT, pWinBorderSize);
        }
    }
}

/*
 * siSUModifyRegion() - ADD/SUBTRACT pRegion from SURegion. 
 */
static void
siSUModifyRegion(pSUWin, op, pRegion)
SUWindowPtr pSUWin;
int op;
RegionPtr pRegion;
{
    ScreenPtr         pScreen;
    SUWindowPtr       pTmpWin;
    Bool	      (* Union)();
    int		      (* RectIn)();

    pScreen = pSUWin->pWin->drawable.pScreen;
    RectIn = pScreen->RectIn;
    Union = pScreen->Union;

    /*
     * If SURegion doesn't exist, then create it with arbitrary
     * room for growth
     */
    if (!SURegion)
        SURegion = (* pScreen->RegionCreate)(NullBox, 10);

    if (op == SUBTRACT)
    {
	/*
	 * Recompute SURegion, excluding pRegion.  That is, first
	 * subtract pRegion from SURegion and then put back
	 * those portion(s) of pRegion that intersect the
         * remaining save-under windows.
	 */
	(* pScreen->Subtract)(SURegion, SURegion, pRegion);
	for (pTmpWin = SUWindows; pTmpWin; pTmpWin = pTmpWin->next)
	{
	    if ((* RectIn)(pRegion, &(pTmpWin->box)))
	    {
	        (* Union)(SURegion, SURegion, pTmpWin->pImageClip);
            }
	}
    }
    else /* op == ADD */
    {
        (* Union)(SURegion, SURegion, pRegion);
    }
}

/*
 * siSUFindWindow() - Locate 'pWin' from the list of currently mapped 
 * save under windows. 
 */
static SUWindowPtr
siSUFindWindow(pWin, pSUPrevWinPtr)
WindowPtr pWin;
SUWindowPtr *pSUPrevWinPtr;
{
    SUWindowPtr pSUWin, pSUPrevWin;
    
    *pSUPrevWinPtr = (SUWindowPtr)NULL;
    for (pSUWin = SUWindows; pSUWin; pSUPrevWin = pSUWin, pSUWin = pSUWin->next)
    {
	if (pWin == pSUWin->pWin)
	{
	    *pSUPrevWinPtr = pSUPrevWin;
	    break;
	}
    }
    return (pSUWin);
}

/*
 * siSUFreeWindow() - Remove 'pSUWin' from SUWindows and free
 * memory associated with it.
 */
static void
siSUFreeWindow(pSUWin, pSUPrevWin, DoFreeImage)
SUWindowPtr pSUWin, pSUPrevWin;
Bool DoFreeImage;
{
    ScreenPtr pScreen;

    /*
     * Remove pSUWin from SUWindows.
     */
    if (SUWindows == pSUWin)
    {
	SUWindows = pSUWin->next;
    }
    else
    {
	pSUPrevWin->next = pSUWin->next;
    }

    /*
     * Subtract pSUWin from SURegion
     */ 
    siSUModifyRegion(pSUWin, SUBTRACT, pSUWin->pImageClip);

    /* 
     * Free data associated with pSUWin
     */
    if (DoFreeImage == TRUE)
    {
        pScreen = pSUWin->pWin->drawable.pScreen;
	/*
	 * Free the cache memory.
	 * DestroyPixmap ()
	 */
	si_CacheFree(pSUWin->pCache,pSUWin->SUCacheType);
	
        if (pSUWin->pImageClip != NullRegion)
            (* pScreen->RegionDestroy)(pSUWin->pImageClip);
    }
    xfree(pSUWin);
}

/*
 * siSUSaveImage() - save the screen image that will be obscured once 
 * pSUWin is mapped to the screen.
 */
static void
siSUSaveImage(pSUWin)
SUWindowPtr pSUWin;
{
    register ScreenPtr pScreen;
    register BoxPtr    pBox;
    int		       srcx, srcy, destx, desty, width, height; 
    SIbitmap		*pImg;
    SIBool		cache_failed = FALSE;

    DrawablePtr	       pDest;
    GCPtr	pGC;
    siPrivGCPtr devPriv;
    SIGStateP	pGS;

    pScreen = pSUWin->pWin->drawable.pScreen;

    pDest = (DrawablePtr) WindowTable[pScreen->myNum];
    pGC = GetScratchGC(pDest->depth, pScreen);
    devPriv = ((siPrivGCPtr) (pGC->devPrivates[siGCPrivateIndex].ptr));
    pGS = &devPriv->GState;

    pBox = &(pSUWin->box);
    pDest = ((DrawablePtr)0);
    /* pSrc = (DrawablePtr) WindowTable[pScreen->myNum]; */

    /*
     * if the SDD supports cache memory, get the cache from SDD, else
     * allocate it here.
     */
    srcx = pBox->x1;
    srcy = pBox->y1;
    destx = 0;
    desty = 0;
    width = pBox->x2 - pBox->x1;
    height = pBox->y2 - pBox->y1;

    pImg = (SIbitmap *) xalloc (sizeof(SIbitmap));
    pImg->Btype = Z_PIXMAP;
    pImg->Bdepth = pScreen->rootDepth;
    pImg->Bwidth = width;
    pImg->Bheight = height;
    pImg->BorgX = srcx; 
    pImg->BorgY = srcy;

    /*
     * try to allocate cache memory
     *    if (cache_memory is allocated)
     * 		pImag->Bptr = allocated area;
     *		save the image in pImg->Bptr;
     *    else
     * 		allocate memory for the save-under image.
     */

    pSUWin->SUCacheType = SU_SDDCACHE;
    pImg->Bptr = NULL;
    si_CacheAlloc (pImg, SHORTTERM_MEM);
    if (!pImg->Bptr) {
	    pImg->Bptr = (SIArray)
		( xalloc((PixmapBytePad(width,pScreen->rootDepth)) * height) );
    	    pSUWin->SUCacheType = SU_LOCALCACHE;
    }

    miSpriteRemoveCursor(screenInfo.screens[0]);
    si_PrepareGS(pGC);
    si_SMbitblt (pImg, srcx, srcy, destx, desty, width, height);
    miSpriteRestoreCursor(screenInfo.screens[0]);

    /*
     * update the link list
     */
    pSUWin->pCache = pImg;

    /*
     * When the screen image is first saved, the portion of the image 
     * that can be restored (i.e., is uncorrupted) is the entire image.
     */
    pSUWin->pImageClip = (* pScreen->RegionCreate)(&(pSUWin->box), 1);
    FreeScratchGC(pGC);
}

/*
 * siSURestoreImage() - Copy the contents of the save-under window image
 * back to the screen w.r.t. the Image Clip List pointed to by pImageClip
 * (see dix/window.c).
 */
void
siSURestoreImage(pScreen)
ScreenPtr pScreen;
{
    WindowPtr          pWin;
    SUPrivPtr	       pSUPriv;
    register BoxPtr    pBox;
    RegionPtr          pImageClip;
    int		       numRects, i;
    int		       srcx, srcy, destx, desty, width, height; 
    DrawablePtr	       pDest;
    GCPtr	       pGC;
    SIbitmap		*pImg;

    pSUPriv = SUPrivate(pScreen); 
    pWin = pSUPriv->pWin;

    /*
     * Copy the save-under image back to the screen. 
     */
    if (pSUPriv->pCache == NullSIbitmap)
	return;
    else
	pImg = pSUPriv->pCache;

    pImageClip = pSUPriv->pImageClip;
    pDest = (DrawablePtr) WindowTable[pScreen->myNum];
    pGC = GetScratchGC(pDest->depth, pScreen);
    numRects = REGION_NUM_RECTS(pImageClip);
    pBox = REGION_RECTS(pImageClip);

    miSpriteRemoveCursor(screenInfo.screens[0]);
    si_PrepareGS(pGC);
    for (i = 0; i < numRects; i++, pBox++)
    {
        srcx = pBox->x1 - pWin->borderSize.extents.x1;
        srcy = pBox->y1 - pWin->borderSize.extents.y1;
        destx = pBox->x1;
        desty = pBox->y1;
        width = pBox->x2 - pBox->x1;
        height = pBox->y2 - pBox->y1;

	si_MSbitblt (pImg, srcx, srcy, destx, desty, width, height);
#ifdef SAVETRACE
	  ErrorF("++RESTORE rectangle %d ==> width %d, height %d at (%d, %d)\n",
			i, width, height, srcx, srcy);
#endif /* SAVETRACE */
    }
    miSpriteRestoreCursor(screenInfo.screens[0]);
    FreeScratchGC(pGC);
}

/*
 * siSUFreeImage() - frees all save-under memory associated with pScreen
 * (see dix/window.c).
 */
void
siSUFreeImage(pScreen)
ScreenPtr pScreen;
{
    SUPrivPtr   pSUPriv;

    if (!(pSUPriv = SUPrivate(pScreen)))
	return;

    /*
     * Free the cache memory
     * The validity checking is done in si_CacheFree
     */
    si_CacheFree (pSUPriv->pCache,pSUPriv->SUCacheType);

    if (pSUPriv->pImageClip != NullRegion)
        (* pScreen->RegionDestroy)(pSUPriv->pImageClip);
    xfree(pSUPriv);
    SUPrivate(pScreen) = (SUPrivPtr) NULL; 
}

/*
 * siSUCheckStack() will determine whether a change in window stacking order
 * affects save-under windows (see dix/window.c).
 */
void
siSUCheckStack(pWin, pSib)
WindowPtr pWin;
WindowPtr pSib;
{
    register WindowPtr pTmpWin;
    WindowPtr pParent;
    ScreenPtr pScreen;
    RegionPtr totalClip;
    BoxPtr    pWinBox;
    Bool      (* Union)();

    if (!(pWin && pWin->viewable))
        return;

    pParent = pWin->parent;
    if (!(pParent && SUCheckDrawable((DrawablePtr) pParent)))
	return;

    pScreen = pParent->drawable.pScreen;
    Union = pScreen->Union;
    pWinBox = (* pScreen->RegionExtents)(&pWin->borderSize);
    totalClip = (* pScreen->RegionCreate)(NULL, 1);

    /*
     * Merge in the sibling windows (at their old places).
     * TotalClip will be the union of all mapped siblings whose borderSize
     * intersects pWin.
     */
    for (pTmpWin = pSib; pTmpWin != NullWindow; pTmpWin = pTmpWin->nextSib) 
    {
        if (pTmpWin->mapped && (*pScreen->RectIn)(&pTmpWin->borderSize,pWinBox))
        {
            (* Union)(totalClip, totalClip, &pTmpWin->borderSize);
	}
    }

    /*
     * Disregard save-unders for all windows that intersect TotalClip 
     */
    if (SUCheckRegion((DrawablePtr) pParent, totalClip))
    {
        siSUScanWindows(pParent, ClipByChildren, totalClip, NullRegion); 
    }
    (* pScreen->RegionDestroy)(totalClip);
}

#ifdef SAVETRACE
siSUPrintRegion(pReg, header)
RegionPtr pReg;
char *header;
{
    int i, numRects;
    BoxPtr pBox;

    numRects = REGION_NUM_RECTS(pReg);
    pBox = REGION_RECTS(pReg);
    for (i = 0; i < numRects; i++, pBox++)
    {
	ErrorF("%s rectangle %d ==> width %d x height %d at (%d, %d)\n",
		header, i, pBox->x2 - pBox->x1, pBox->y2 - pBox->y1,
		pBox->x1, pBox->y1);
    }
}
#endif

/*
 * siSUComputeBox() will compute the bounding box for the
 * passed array of points.  It will return a pointer to the
 * static data. 
 */ 
BoxPtr
siSUComputeBox(pDraw, pGC, npts, pPts)
DrawablePtr pDraw;
GCPtr	pGC;
int	npts;
xPoint	*pPts;
{
    register int i, xMin, xMax, yMin, yMax;
    static BoxRec box;
    int xorg, yorg;

    xMin = yMin = MAXSHORT;
    xMax = yMax = MINSHORT;
    for(i = 0; i < npts; i++)
    {
     	xMin = min (xMin, pPts[i].x);
	yMin = min (yMin, pPts[i].y);
	xMax = max (xMax, pPts[i].x);
	yMax = max (yMax, pPts[i].y);
    }
    xorg = (int)pDraw->x;
    yorg = (int)pDraw->y;
    box.x1 = xMin + xorg;
    box.y1 = yMin + yorg;
    box.x2 = xMax + xorg;
    box.y2 = yMax + yorg;
    return(&box);
}

void
siTestRects(pDrawable, pGC, nrectFill, prectInit)
DrawablePtr pDrawable;
GCPtr	    pGC;
int	    nrectFill; 		/* number of rectangles to fill */
xRectangle  *prectInit;  	/* Pointer to first rectangle to fill */
{
    register int i, xMin, yMin, xMax, yMax;
    int xorg, yorg;
    BoxRec box;

    xMin = yMin = MAXSHORT; 
    xMax = yMax = MINSHORT;
    for(i = 0; i < nrectFill; i++)
    {
        xMin = min (xMin, prectInit[i].x);
        yMin = min (yMin, prectInit[i].y);
        xMax = max (xMax, (prectInit[i].x + (int) prectInit[i].width));
        yMax = max (yMax, (prectInit[i].y + (int) prectInit[i].height));
    }
    xorg = (int)pDrawable->x;
    yorg = (int)pDrawable->y;
    box.x1 = xMin + xorg;
    box.y1 = yMin + yorg;
    box.x2 = xMax + xorg;
    box.y2 = yMax + yorg;
    if (SUCheckBox(pDrawable, &box))
    {
        siSUScanWindows(pDrawable, pGC->subWindowMode, NULL, &box);
    }
}

