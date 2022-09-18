/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/mipointer.c	1.3"

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

/*
 * mipointer.c
 */

/* $XConsortium: mipointer.c,v 5.10 90/03/16 17:50:30 keith Exp $ */

/*
Copyright 1989 by the Massachusetts Institute of Technology
*/

# define NEED_EVENTS
# include   "X.h"
# include   "Xmd.h"
# include   "Xproto.h"
# include   "misc.h"
# include   "windowstr.h"
# include   "pixmapstr.h"
# include   "mi.h"
# include   "scrnintstr.h"
# include   "mipointrst.h"
# include   "cursorstr.h"
# include   "dixstruct.h"
# include   "sidep.h"

static int  miPointerScreenIndex;
static unsigned long miPointerGeneration = 0;

/*
 * until more than one pointer device exists.
 */

static miPointerRec miPointer;

static Bool miPointerRealizeCursor (),	    miPointerUnrealizeCursor ();
static Bool miPointerDisplayCursor ();
static void miPointerConstrainCursor (),    miPointerPointerNonInterestBox();
static void miPointerCursorLimits ();
static Bool miPointerSetCursorPosition ();
static void miPointerSetCursor();
static void miPointerCheckScreen();

static Bool miPointerCloseScreen();

extern void miRecolorCursor ();
extern void ProcessInputEvents ();
extern void NewCurrentScreen ();

Bool
miPointerInitialize (pScreen, spriteFuncs, cursorFuncs)
    ScreenPtr		    pScreen;
    miPointerSpriteFuncPtr  spriteFuncs;
    miPointerCursorFuncPtr  cursorFuncs;
{
    miPointerScreenPtr	pScreenPriv;

    if (miPointerGeneration != serverGeneration)
    {
	miPointerScreenIndex = AllocateScreenPrivateIndex();
	if (miPointerScreenIndex < 0)
	    return FALSE;
	miPointerGeneration = serverGeneration;
    }
    pScreenPriv = (miPointerScreenPtr) xalloc (sizeof (miPointerScreenRec));
    if (!pScreenPriv)
	return FALSE;
    pScreenPriv->funcs = spriteFuncs;
    pScreenPriv->pPointer = &miPointer;
    pScreenPriv->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = miPointerCloseScreen;
    pScreen->devPrivates[miPointerScreenIndex].ptr = (pointer) pScreenPriv;
    /*
     * set up screen cursor method table
     */
    pScreen->ConstrainCursor = miPointerConstrainCursor;
    pScreen->CursorLimits = miPointerCursorLimits;
    pScreen->DisplayCursor = miPointerDisplayCursor;
    pScreen->RealizeCursor = miPointerRealizeCursor;
    pScreen->UnrealizeCursor = miPointerUnrealizeCursor;
    pScreen->SetCursorPosition = miPointerSetCursorPosition;
    pScreen->RecolorCursor = miRecolorCursor;
    pScreen->PointerNonInterestBox = miPointerPointerNonInterestBox;
    /*
     * set up the pointer object
     */
    miPointer.pScreen = NULL;
    miPointer.pCursor = NULL;
    miPointer.limits.x1 = 0;
    miPointer.limits.x2 = 32767;
    miPointer.limits.y1 = 0;
    miPointer.limits.y2 = 32767;
    miPointer.x = 0;
    miPointer.y = 0;
    miPointer.funcs = cursorFuncs;
    return TRUE;
}

static Bool
miPointerCloseScreen (index, pScreen)
    int		index;
    ScreenPtr	pScreen;
{
    miPointerScreenPtr   pPriv;

    pPriv = (miPointerScreenPtr) pScreen->devPrivates[miPointerScreenIndex].ptr;
    pScreen->CloseScreen = pPriv->CloseScreen;
    si_turnoffcursor (0); /* turn off the cursor before closing the screen */
    xfree ((pointer) pPriv);
    return (*pScreen->CloseScreen) (index, pScreen);
}

/*
 * DIX/DDX interface routines
 */

static Bool
miPointerRealizeCursor (pScreen, pCursor)
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
{
    miPointerScreenPtr   pPriv;

    pPriv = (miPointerScreenPtr) pScreen->devPrivates[miPointerScreenIndex].ptr;
    return (*pPriv->funcs->RealizeCursor) (pScreen, pCursor);
}

static Bool
miPointerUnrealizeCursor (pScreen, pCursor)
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
{
    miPointerScreenPtr   pPriv;

    pPriv = (miPointerScreenPtr) pScreen->devPrivates[miPointerScreenIndex].ptr;
    return (*pPriv->funcs->UnrealizeCursor) (pScreen, pCursor);
}

static Bool
miPointerDisplayCursor (pScreen, pCursor)
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
{
    miPointerScreenPtr  pPriv;
    miPointerPtr	pPointer;

    pPriv = (miPointerScreenPtr) pScreen->devPrivates[miPointerScreenIndex].ptr;
    pPointer = pPriv->pPointer;
    pPointer->pCursor = pCursor;
    if (pCursor)
	(*pPriv->funcs->DisplayCursor) (pScreen, pCursor, pPointer->x, pPointer->y);
    return TRUE;
}

static void
miPointerConstrainCursor (pScreen, pBox)
    ScreenPtr	pScreen;
    BoxPtr	pBox;
{
    miPointerScreenPtr	pPriv;
    miPointerPtr	pPointer;

    pPriv = (miPointerScreenPtr) pScreen->devPrivates[miPointerScreenIndex].ptr;
    pPointer = pPriv->pPointer;
    pPointer->limits = *pBox;
}

/*ARGSUSED*/
static void
miPointerPointerNonInterestBox (pScreen, pBox)
    ScreenPtr	pScreen;
    BoxPtr	pBox;
{
    return;
}

/*ARGSUSED*/
static void
miPointerCursorLimits(pScreen, pCursor, pHotBox, pTopLeftBox)
    ScreenPtr	pScreen;
    CursorPtr	pCursor;
    BoxPtr	pHotBox;
    BoxPtr	pTopLeftBox;
{
    *pTopLeftBox = *pHotBox;
}

static Bool
miPointerSetCursorPosition(pScreen, newx, newy, generateEvent)
    ScreenPtr pScreen;
    int       newx, newy;
    Bool      generateEvent;
{
    miPointerSetCursor (pScreen, newx, newy, generateEvent, TRUE);
    return TRUE;
}

/*
 * Pointer/CursorDisplay interface routines
 */

/*
 * miPointerCheckCurrentScreen
 *
 * make pPointer->pScreen up to date, inform device of
 * any screen changes
 */

static void
miPointerCheckScreen (pScreen, pPointer)
    ScreenPtr	    pScreen;
    miPointerPtr    pPointer;
{
    /*
     * if the cursor has switched screens, disable the sprite
     * on the old screen
     */
    if (pPointer->pScreen != pScreen)
    {
	if (pPointer->pScreen)
	{
	    if (pPointer->pCursor)
	    {
	    	miPointerScreenPtr  pOldPriv;
    	
	    	pOldPriv = (miPointerScreenPtr) pPointer->pScreen->
				    	devPrivates[miPointerScreenIndex].ptr;
	    	(*pOldPriv->funcs->UndisplayCursor)
				    	(pPointer->pScreen, pPointer->pCursor);
	    }
	    (*pPointer->funcs->CrossScreen) (pPointer->pScreen, FALSE);
	}
	(*pPointer->funcs->CrossScreen) (pScreen, TRUE);
    }
    pPointer->pScreen = pScreen;
}

/*
 * miPointerDeltaCursor.  The pointer has moved dx,dy from it's previous
 * position.
 */

void
miPointerDeltaCursor (pScreen, dx, dy, generateEvent)
    ScreenPtr	pScreen;
    int		dx, dy;
    Bool	generateEvent;
{
    miPointerScreenPtr  pPriv;
    miPointerPtr	pPointer;

    pPriv = (miPointerScreenPtr) pScreen->devPrivates[miPointerScreenIndex].ptr;
    pPointer = pPriv->pPointer;
    miPointerSetCursor (pPointer->pScreen, pPointer->x + dx, pPointer->y + dy,
			generateEvent, FALSE);
}

/*
 * miPointerMoveCursor.  The pointer has moved to x,y
 */

void
miPointerMoveCursor (pScreen, x, y, generateEvent)
    ScreenPtr	pScreen;
    int		x, y;
    Bool	generateEvent;
{
    miPointerScreenPtr  pPriv;
    miPointerPtr	pPointer;

    pPriv = (miPointerScreenPtr) pScreen->devPrivates[miPointerScreenIndex].ptr;
    pPointer = pPriv->pPointer;
    miPointerSetCursor (pPointer->pScreen, x, y, generateEvent, FALSE);
}

/*
 * miPointerSetCursor.  The pointer has moved to x,y on screen pScreen
 */

static void
miPointerSetCursor (pScreen, x, y, generateEvent, afterEvents)
    ScreenPtr	pScreen;
    int		x, y;
    Bool	generateEvent;
    Bool	afterEvents;
{
    miPointerScreenPtr  pPriv;
    miPointerPtr	pPointer;
    ScreenPtr		newScreen;

    pPriv = (miPointerScreenPtr) pScreen->devPrivates[miPointerScreenIndex].ptr;
    pPointer = pPriv->pPointer;
    if (x < 0 || x >= pScreen->width || y < 0 || y >= pScreen->height)
    {
	/*
	 * if the pointer is not confined to the current screen,
	 * and the device can do something interesting when
	 * the pointer goes off the screen, remove the cursor
	 * and mark it as not-to-be-displayed, otherwise
	 * constrain it to the current screen.
	 */
	newScreen = pScreen;
	if (!PointerConfinedToScreen() &&
	    (*pPointer->funcs->CursorOffScreen) (&newScreen, &x, &y))
	{
	    if (newScreen != pScreen)
	    {
		NewCurrentScreen (newScreen, x, y);
		return;
	    }
	}
    }
    /*
     * constrain the hot-spot to the current
     * limits
     */
    if (x < pPointer->limits.x1)
	x = pPointer->limits.x1;
    if (x >= pPointer->limits.x2)
	x = pPointer->limits.x2 - 1;
    if (y < pPointer->limits.y1)
	y = pPointer->limits.y1;
    if (y >= pPointer->limits.y2)
	y = pPointer->limits.y2 - 1;
    if (pPointer->x == x && pPointer->y == y && pPointer->pScreen == pScreen)
	return;
    pPointer->x = x;
    pPointer->y = y;
    miPointerCheckScreen (pScreen, pPointer);
    if (generateEvent)
    {
	xEvent	xE;

	xE.u.u.type = MotionNotify;
	xE.u.keyButtonPointer.rootX = x;
	xE.u.keyButtonPointer.rootY = y;
	xE.u.keyButtonPointer.time = (*pPointer->funcs->EventTime)(pScreen);
	/*
	 * this call may change the cursor shape, in which case the
	 * new cursor will end up on the screen in the correct place.
	 */
	if (afterEvents)
	    (*pPointer->funcs->QueueEvent) (&xE, pPointer->pPointer, pScreen);
	else
	    (*pPointer->pPointer->processInputProc) (&xE, pPointer->pPointer, 1);
    }
    /*
     * avoid problems when this routine recurses
     */
    pPriv = (miPointerScreenPtr) pPointer->pScreen->devPrivates[miPointerScreenIndex].ptr;
    if (pPointer->pCursor)
	(*pPriv->funcs->MoveCursor) (pPointer->pScreen, pPointer->pCursor, 
				     pPointer->x, pPointer->y);
}

/*ARGSUSED*/
void
miPointerQueueEvent (pxE, pPointer, pScreen)
    xEvent	*pxE;
    DevicePtr	pPointer;
    ScreenPtr	pScreen;
{
    /* Smash time to currentTime, so we can process it in the more recent
     * past, which is the best dix knows about.
     */
    pxE->u.keyButtonPointer.time = currentTime.milliseconds;
    (*pPointer->processInputProc) (pxE, pPointer, 1);
}

void
miPointerPosition (pScreen, px, py)
    ScreenPtr	pScreen;
    short	*px, *py;
{
    miPointerScreenPtr  pPriv;
    miPointerPtr	pPointer;

    pPriv = (miPointerScreenPtr) pScreen->devPrivates[miPointerScreenIndex].ptr;
    pPointer = pPriv->pPointer;
    
    *px = pPointer->x;
    *py = pPointer->y;
}

void
miRegisterPointerDevice (pScreen, pDevice)
    ScreenPtr	pScreen;
    DevicePtr	pDevice;
{
    miPointerScreenPtr	pPriv;
    miPointerPtr	pPointer;

    pPriv = (miPointerScreenPtr) pScreen->devPrivates[miPointerScreenIndex].ptr;
    pPointer = pPriv->pPointer;
    pPointer->pPointer = pDevice;
}
