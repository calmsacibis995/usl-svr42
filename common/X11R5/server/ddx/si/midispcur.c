/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/midispcur.c	1.2"

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

/* $XConsortium: midispcur.c,v 5.10 90/03/12 14:10:02 rws Exp $ */

/*
Copyright 1989 by the Massachusetts Institute of Technology
*/

/*
 * sidispcur.c
 *
 * SI cursor display routines.  Some functions have changed 
 * significantly from their midispcur.c base.
 */

#define NEED_EVENTS
# include   "X.h"
# include   "misc.h"
# include   "input.h"
# include   "cursorstr.h"
# include   "windowstr.h"
# include   "regionstr.h"
# include   "dixstruct.h"
# include   "scrnintstr.h"
# include   "servermd.h"
# include   "misprite.h"
# include   "mipointer.h"
# include   "gcstruct.h"
# include   "../si/sidep.h"

static CursorPtr  currentCursor = NullCursor;	/* Cursor being displayed */
static SICursor	SC;
static SIbitmap	SIsrc, SIisrc, SImask;

/* per-screen private data */

/* per-cursor per-screen private data */
typedef struct {
    PixmapPtr		sourceBits;	    /* source bits */
    PixmapPtr		maskBits;	    /* mask bits */
    PixmapPtr		invsrcBits;	    /* mask bits */
    int			index;		    /* SI cursor index */
} miDCCursorRec, *miDCCursorPtr;

/*
 * Initialization of the miPointerCursorFuncRec ...
 * probably shouldn't be in this file (leave it for now)
 *
 * A pointer to siPointerCursorFuncs is passed to miDCInitialize
 * from i386ScreenInit() located in siinit.c
 */
static long siEventTime();
static Bool siCursorOffScreen();
static void siCrossScreen();
extern void miPointerQueueEvent();

miPointerCursorFuncRec siPointerCursorFuncs = {
    siEventTime,
    siCursorOffScreen,
    siCrossScreen,
    miPointerQueueEvent,		/* use the one from mi */
};

static long
siEventTime (pScreen)
    ScreenPtr	pScreen;
{
    extern long lastEventTime;		/* defined in io/xwin_io.c */

    return lastEventTime;
}

static Bool
siCursorOffScreen(pScreen, x, y)
ScreenPtr	*pScreen;
int		*x, *y;
{
    return FALSE;			/* do anything here ?? */
}

static void
siCrossScreen(pScreen, entering)
ScreenPtr pScreen;
Bool      entering;
{
}

Bool
miDCInitialize (pScreen, cursorFuncs)
    ScreenPtr		    pScreen;
    miPointerCursorFuncPtr  cursorFuncs;
{

    if (!miSpriteInitialize (pScreen, cursorFuncs))
	return FALSE;

    return TRUE;
}

/*
 * SI start ... the following func has been tossed
 *

#define tossGC(gc)  (gc ? FreeGC (gc, (GContext) 0) : 0)
#define tossPix(pix)	(pix ? (*pScreen->DestroyPixmap) (pix) : TRUE)

static Bool
miDCCloseScreen (index, pScreen)
    ScreenPtr	pScreen;
{
    miDCScreenPtr   pScreenPriv;

    pScreenPriv = (miDCScreenPtr) pScreen->devPrivates[miDCScreenIndex].ptr;
    pScreen->CloseScreen = pScreenPriv->CloseScreen;
    tossGC (pScreenPriv->pSourceGC);
    tossGC (pScreenPriv->pMaskGC);
    tossGC (pScreenPriv->pSaveGC);
    tossGC (pScreenPriv->pRestoreGC);
    tossGC (pScreenPriv->pMoveGC);
    tossGC (pScreenPriv->pPixSourceGC);
    tossGC (pScreenPriv->pPixMaskGC);
    tossPix (pScreenPriv->pSave);
    tossPix (pScreenPriv->pTemp);
    xfree ((pointer) pScreenPriv);
    return (*pScreen->CloseScreen) (index, pScreen);
}
 *
 * SI end
 */

Bool
si_curs_realize (pScreen, pCursor)
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
{
    miDCCursorPtr   pPriv;
    GCPtr	    pGC;
    XID		    gcvals[3];

    if (pCursor->bits->refcnt <= 1)
	pCursor->bits->devPriv[pScreen->myNum] = (pointer)NULL;
    else			/* this cursor is already set up */
	return TRUE;

    pPriv = (miDCCursorPtr) xalloc (sizeof (miDCCursorRec));
    if (!pPriv)
	return FALSE;

    pPriv->index = 0;

    /*
     * Allocate the pixmaps for the source, mask, and invsrc bitmaps.
     */
    pPriv->sourceBits = (*pScreen->CreatePixmap) (pScreen, 
						  pCursor->bits->width, 
						  pCursor->bits->height, 1);
    if (!pPriv->sourceBits)
    {
	xfree ((pointer) pPriv);
	return FALSE;
    }

    pPriv->maskBits =  (*pScreen->CreatePixmap) (pScreen, 
						 pCursor->bits->width, 
						 pCursor->bits->height, 1);
    if (!pPriv->maskBits)
    {
	(*pScreen->DestroyPixmap) (pPriv->sourceBits);
	xfree ((pointer) pPriv);
	return FALSE;
    }

    pPriv->invsrcBits =  (*pScreen->CreatePixmap) (pScreen, 
						   pCursor->bits->width, 
						   pCursor->bits->height, 1);
    if (!pPriv->invsrcBits)
    {
	(*pScreen->DestroyPixmap) (pPriv->sourceBits);
	(*pScreen->DestroyPixmap) (pPriv->maskBits);
	xfree ((pointer) pPriv);
	return FALSE;
    }

    pCursor->bits->devPriv[pScreen->myNum] = (pointer) pPriv;

    /* create the two sets of bits, clipping as appropriate */

    pGC = GetScratchGC (1, pScreen);
    if (!pGC)
    {
	(void) si_curs_unrealize(pScreen, pCursor);
	return FALSE;
    }

    /*
     * Set up the source bitmap
     */
    ValidateGC ((DrawablePtr)pPriv->sourceBits, pGC);
    (*pGC->ops->PutImage) (pPriv->sourceBits, pGC, 1,
			   0, 0, pCursor->bits->width, pCursor->bits->height,
 			   0, XYPixmap, pCursor->bits->source);

    /*
     * Set up the mask bitmap
     */
    ValidateGC ((DrawablePtr)pPriv->sourceBits, pGC);
    (*pGC->ops->PutImage) (pPriv->maskBits, pGC, 1,
			   0, 0, pCursor->bits->width, pCursor->bits->height,
 			   0, XYPixmap, pCursor->bits->mask);

    /* 
     * Set up the inverted source bitmap
     */
    ValidateGC ((DrawablePtr)pPriv->invsrcBits, pGC);
    (*pGC->ops->PutImage) (pPriv->invsrcBits, pGC, 1,
			   0, 0, pCursor->bits->width, pCursor->bits->height,
 			   0, XYPixmap, pCursor->bits->mask);

    gcvals[0] = GXandInverted;
    ChangeGC (pGC, GCFunction, gcvals);
    ValidateGC ((DrawablePtr)pPriv->invsrcBits, pGC);
    (*pGC->ops->PutImage) (pPriv->invsrcBits, pGC, 1,
			   0, 0, pCursor->bits->width, pCursor->bits->height,
 			   0, XYPixmap, pCursor->bits->source);

    FreeScratchGC (pGC);
    return TRUE;
}

Bool
si_curs_unrealize (pScreen, pCursor)
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
{
    miDCCursorPtr   pPriv;

    pPriv = (miDCCursorPtr) pCursor->bits->devPriv[pScreen->myNum];
    if (pPriv && (pCursor->bits->refcnt <= 1))
    {
	(*pScreen->DestroyPixmap) (pPriv->sourceBits);
	(*pScreen->DestroyPixmap) (pPriv->maskBits);
	(*pScreen->DestroyPixmap) (pPriv->invsrcBits);
	xfree ((pointer) pPriv);
	pCursor->bits->devPriv[pScreen->myNum] = (pointer)NULL;
    }
    return TRUE;
}

si_curs_display(pScreen, pCursor, fg, bg)
ScreenPtr	pScreen;
CursorPtr	pCursor;
Pixel		fg;		    /* Foreground color */
Pixel		bg;		    /* Background color */
{
    miDCCursorPtr   pPriv;

    pPriv = (miDCCursorPtr) pCursor->bits->devPriv[pScreen->myNum];
    
    SIsrc.Bdepth = SIisrc.Bdepth = SImask.Bdepth = 1;
    SIsrc.Bwidth = SIisrc.Bwidth = SImask.Bwidth = pCursor->bits->width;
    SIsrc.Bheight = SIisrc.Bheight = SImask.Bheight = pCursor->bits->height;
    SImask.Bptr = (SIArray) pPriv->maskBits->devPrivate.ptr;
    SIsrc.Bptr = (SIArray) pPriv->sourceBits->devPrivate.ptr;
    SIisrc.Bptr = (SIArray) pPriv->invsrcBits->devPrivate.ptr;

    SC.SCwidth = SIsrc.Bwidth;
    SC.SCheight = SIsrc.Bheight;
    SC.SCfg = fg;
    SC.SCbg = bg;
    SC.SCmask = &SImask;
    SC.SCsrc = &SIsrc;
    SC.SCinvsrc = &SIisrc;
    if (!si_downloadcursor(pPriv->index, &SC)) {
	FatalError ("Cannot get memory for downloading cursor.");
    };
    si_turnoncursor(pPriv->index);
    return;
}


si_curs_undisplay(pScreen)
ScreenPtr	pScreen;
{
    si_turnoffcursor(0);
}


si_curs_move(pScreen, pCursor, x, y)
ScreenPtr	pScreen;
CursorPtr	pCursor;
int x, y;
{
    miDCCursorPtr   pPriv;

    pPriv = (miDCCursorPtr) pCursor->bits->devPriv[pScreen->myNum];
    
    x -= (int)pCursor->bits->xhot;
    y -= (int)pCursor->bits->yhot;
    si_movecursor(pPriv->index, x, y);
}


