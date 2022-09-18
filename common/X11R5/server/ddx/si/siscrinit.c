/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/siscrinit.c	1.9"

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

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved
******************************************************************/
/* $XConsortium: mfbscrinit.c,v 5.10 90/01/23 15:39:27 rws Exp $ */

#include "X.h"
#include "Xmd.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "resource.h"
#include "colormap.h"
#include "colormapst.h"		/* SI */
#include "si.h"			/* SI */ 
#include "mistruct.h"
#include "dix.h"
#include "mi.h"
#include "mibstore.h"
#include "servermd.h"
#include "simskbits.h"		/* SI */
#include "sidep.h"		/* SI */

#ifdef XWIN_SAVE_UNDERS		/* SI: save-unders */
#include "sisave.h"
int SUScrnPrivateIndex = 0;
int siDefaultSaveUnder = Always;
#else
int siDefaultSaveUnder = NotUseful;
#endif 				/* SI: save-unders */

extern RegionPtr mfbPixmapToRegion();
extern void miPaintWindow ();

#ifdef BEF_DT
int siWindowPrivateIndex = 0;			/* SI (mfb to si) */
#endif

int siGCPrivateIndex = 0; 			/* SI (mfb to si) */
static unsigned long siGeneration = 0;		/* SI (mfb to si) */

extern int defaultColorVisualClass;

/*
 * For SI, we allow only one visual per depth.  It *should* be ok
 * to have multiple depths of the same value though.  (Allowing multiple
 * visuals of equal depth.)
 */
static VisualRec *visuals;
static int numvisuals = 0;
static DepthRec *depths;
static int rootdepth;

/* Screen Interface Global Defines */

SIInfo	HWinfoSTR;
SIInfo	*HWinfo = &HWinfoSTR;
ScreenInterface	*HWroutines = NULL;
char	*siSTATEerr = "Can't Set Graphics State";


int siDefaultBackingStore = NotUseful;

#ifdef BACKING_STORE
extern void siSaveAreas();
extern void siRestoreAreas();

miBSFuncRec siBSFuncRec = {		/* SI (mfb to si) */
    siSaveAreas,			/* SI (mfb to si) */
    siRestoreAreas,			/* SI (mfb to si) */
    (void (*)()) 0,
    (PixmapPtr (*)()) 0,
    (PixmapPtr (*)()) 0,
};
#endif

/*ARGSUSED*/
static Bool
siCloseScreen (index, pScreen)
    int         index;
    ScreenPtr   pScreen;
{
    xfree (pScreen->devPrivate);
    return TRUE;
}

Bool
siAllocatePrivates(pScreen, pWinIndex, pGCIndex)
    ScreenPtr pScreen;
    int *pWinIndex, *pGCIndex;
{
    int i;

    if (siGeneration != serverGeneration)	/* SI (mfb to si) */
    {
#ifdef BEF_DT
	siWindowPrivateIndex = AllocateWindowPrivateIndex();
#endif
	siGCPrivateIndex = AllocateGCPrivateIndex();
	siGeneration = serverGeneration;	/* SI (mfb to si) */

	for (i = 0; i < numvisuals; i++) {
            visuals[i].vid = FakeClientID(0);
	    depths[i].numVids = 1;
            depths[i].vids = &visuals[i].vid;
        }
    }
#ifdef BEF_DT
    if (pWinIndex)
	*pWinIndex = siWindowPrivateIndex;
    if (pGCIndex)
	*pGCIndex = siGCPrivateIndex;
#endif

#ifdef XWIN_SAVE_UNDERS
	/*
	 * Allocate private screen structure (save unders) RJK
	 * and initialize the private Save Under pointer to Null.
	 */
    SUScrnPrivateIndex = AllocateScreenPrivateIndex();
    pScreen->devPrivates[SUScrnPrivateIndex].ptr =
	       (pointer) xalloc (sizeof (SUPrivScrn));
    SUPrivate(pScreen) = (SUPrivPtr) NULL;
#endif

#ifdef BEF_DT
    return (AllocateWindowPrivate(pScreen, siWindowPrivateIndex,
				  sizeof(siPrivWin)) &&
	    AllocateGCPrivate(pScreen, siGCPrivateIndex, sizeof(siPrivGC)));
#else
    return (AllocateGCPrivate(pScreen, siGCPrivateIndex, sizeof(siPrivGC)));
#endif
}

#ifdef BEF_DT

/* dts * (inch/dot) * (25.4 mm / inch) = mm */
static  unsigned char A[16] = {
		0x00, 0x01, 0x02, 0x03,
		0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0A, 0x0B,
		0x0C, 0x0D, 0x0E, 0x0F
     };
static unsigned char B[16] = {
		0x00, 0x10, 0x20, 0x30,
		0x40, 0x50, 0x60, 0x70,
		0x80, 0x90, 0xA0, 0xB0,
		0xC0, 0xD0, 0xE0, 0xF0
     };
#endif

Bool
siScreenInit(pScreen, pbits, xsize, ysize, dpix, dpiy, width)
    register ScreenPtr pScreen;
    pointer pbits;		/* pointer to screen bitmap */
    int xsize, ysize;		/* in pixels */
    int dpix, dpiy;		/* dots per inch */
    int width;			/* pixel width of frame buffer */
{
    register PixmapPtr pPixmap;
    Bool siInitializeColormap();	/* SI */ 
    SIVisualP pVisuals;			/* SI */
    int i;				/* SI */
    extern siCmap si_colormap;

    if ( (numvisuals == 0) || (siGeneration != serverGeneration) ) {
	numvisuals = si_GetInfoVal(SIvisualCNT);

#ifdef DELETE
/*
 * SCOTT:  Fix this when we can handle multiple depths.
 */
if (numvisuals != 1)
FatalError("numvisuals must currently be 1\n");
#endif

	pVisuals = si_GetInfoVal(SIvisuals);
	rootdepth = pVisuals->SVdepth;

	visuals = (VisualPtr)xalloc(sizeof(VisualRec) * numvisuals);
	if (!visuals)
	    return FALSE;

	depths = (DepthPtr)xalloc(sizeof(DepthRec) * numvisuals);
	if (!depths) {
	    xfree(visuals);
	    return FALSE;
	}
    }

#ifdef BEF_DT
    if (!siAllocatePrivates(pScreen, &siWindowPrivateIndex, &siGCPrivateIndex))
	return FALSE;
    if (!AllocateWindowPrivate(pScreen, siWindowPrivateIndex,
                               sizeof(siPrivWin)) ||
        !AllocateGCPrivate(pScreen, siGCPrivateIndex, sizeof(siPrivGC)))
        return FALSE;
#else
    if (!siAllocatePrivates(pScreen))
	return FALSE;
#endif

    pScreen->width = xsize;
    pScreen->height = ysize;
    pScreen->mmWidth = (xsize * 254) / (dpix * 10);
    pScreen->mmHeight = (ysize * 254) / (dpiy * 10);

    pScreen->numDepths = numvisuals;
    pScreen->allowedDepths = depths;

    pScreen->rootDepth = rootdepth;
    pScreen->minInstalledCmaps = 1;
    pScreen->maxInstalledCmaps = 1;
    pScreen->backingStoreSupport = siDefaultBackingStore;	/* SI */

    /*
     * Should we support save-unders if there is no support for
     * scr->mem and mem->scr bitblt. For now, don't support save-unders
     * if there is no support from the SDD
     */
    if (si_hassmbitblt && si_hasmsbitblt)
    	pScreen->saveUnderSupport = siDefaultSaveUnder;
    else
    	pScreen->saveUnderSupport = NotUseful;

    /* let CreateDefColormap do whatever it wants */
    pScreen->blackPixel =  pScreen->whitePixel = (Pixel) 0;

    /* cursmin and cursmax are device specific */

    pScreen->numVisuals = numvisuals;
    pScreen->visuals = visuals;

    pPixmap = (PixmapPtr)xalloc(sizeof(PixmapRec));
    if (!pPixmap)
	return FALSE;
    pPixmap->drawable.type = DRAWABLE_PIXMAP;
    pPixmap->drawable.depth = rootdepth;
    pPixmap->drawable.pScreen = pScreen;
    pPixmap->drawable.serialNumber = 0;
    pPixmap->drawable.x = 0;
    pPixmap->drawable.y = 0;
    pPixmap->drawable.width = xsize;
    pPixmap->drawable.height = ysize;
    pPixmap->refcnt = 1;
    pPixmap->devPrivate.ptr = pbits;
    pPixmap->devKind = PixmapBytePad(xsize, rootdepth);  /* SI */
    pScreen->devPrivate = (pointer)pPixmap;

    /* anything that cfb doesn't know about is assumed to be done
       elsewhere.  (we put in no-op only for things that we KNOW
       are really no-op).
    */

    pScreen->CreateWindow = siCreateWindow;		/* SI (mfb to si) */
    pScreen->DestroyWindow = siDestroyWindow;		/* SI (mfb to si) */
    pScreen->PositionWindow = siPositionWindow;		/* SI (mfb to si) */
    pScreen->ChangeWindowAttributes = siChangeWindowAttributes;	    /* SI */ 
    pScreen->RealizeWindow = siMapWindow;		/* SI (mfb to si) */
    pScreen->UnrealizeWindow = siUnmapWindow;		/* SI (mfb to si) */

    pScreen->RealizeFont = siRealizeFont; 		/* SI (mfb to si) */
    pScreen->UnrealizeFont = siUnrealizeFont;		/* SI (mfb to si) */
    pScreen->CloseScreen = siCloseScreen;		/* SI (from mfb) */
    pScreen->QueryBestSize = siQueryBestSize;		/* SI (mfb to si) */
    pScreen->GetImage = miGetImage;			/* SI (use mi) */
    pScreen->GetSpans = siGetSpans;			/* SI (from mfb) */
    pScreen->SourceValidate = (void (*)()) 0;
    pScreen->CreateGC = siCreateGC;			/* SI (mfb to si) */
    pScreen->CreatePixmap = siCreatePixmap;		/* SI (mfb to si) */
    pScreen->DestroyPixmap = siDestroyPixmap;		/* SI (mfb to si) */
    pScreen->ValidateTree = miValidateTree;

    pScreen->InstallColormap = siInstallColormap;	/* SI (mfb to si) */
    pScreen->UninstallColormap = siUninstallColormap;   /* SI (mfb to si)*/
    pScreen->ListInstalledColormaps = siListInstalledColormaps;    /* SI */
    pScreen->StoreColors = siStoreColors;	/* SI (NoopDDA -> si func) */
    pScreen->ResolveColor = siResolveColor;		/* SI (mfb to si) */

    pScreen->RegionCreate = miRegionCreate;
    pScreen->RegionInit = miRegionInit;
    pScreen->RegionCopy = miRegionCopy;
    pScreen->RegionDestroy = miRegionDestroy;
    pScreen->RegionUninit = miRegionUninit;
    pScreen->Intersect = miIntersect;
    pScreen->Inverse = miInverse;
    pScreen->Union = miUnion;
    pScreen->Subtract = miSubtract;
    pScreen->RegionReset = miRegionReset;
    pScreen->TranslateRegion = miTranslateRegion;
    pScreen->RectIn = miRectIn;
    pScreen->PointInRegion = miPointInRegion;
    pScreen->WindowExposures = miWindowExposures;
    pScreen->PaintWindowBackground = miPaintWindow;   /* SI */ 
    pScreen->PaintWindowBorder = miPaintWindow;         /* SI */ 
    pScreen->CopyWindow = siCopyWindow;	        	/* SI (mfb to si) */
    pScreen->ClearToBackground = miClearToBackground;

    pScreen->RegionNotEmpty = miRegionNotEmpty;
    pScreen->RegionEmpty = miRegionEmpty;
    pScreen->RegionExtents = miRegionExtents;
    pScreen->RegionAppend = miRegionAppend;
    pScreen->RegionValidate = miRegionValidate;
    pScreen->BitmapToRegion = mfbPixmapToRegion;
    pScreen->RectsToRegion = miRectsToRegion;
    pScreen->SendGraphicsExpose = miSendGraphicsExpose;

    pScreen->BlockHandler = NoopDDA;
    pScreen->WakeupHandler = NoopDDA;
    pScreen->blockData = (pointer)0;
    pScreen->wakeupData = (pointer)0;

    pScreen->CreateColormap = siInitializeColormap;
    pScreen->DestroyColormap = NoopDDA;
    /*
     * set default white and black values. These also can be reset by
     * command line options.
     */
    pScreen->whitePixel = 1;
    pScreen->blackPixel = 0;

    /* SI: start */
    pVisuals = si_GetInfoVal(SIvisuals);
    for (i = 0; i < numvisuals; i++, pVisuals++) {
	depths[i].depth = pVisuals->SVdepth;
	visuals[i].class = pVisuals->SVtype;

	/*
	 * See if we can fake a user specified visual
	 */
	switch (si_colormap.visual) {
	case StaticColor:
		if (pVisuals->SVtype == StaticColor)
			visuals[i].class = si_colormap.visual;
		break;
	case GrayScale:
		if (pVisuals->SVtype == PseudoColor)
			visuals[i].class = si_colormap.visual;
		break;
	case StaticGray:
		if ((pVisuals->SVtype == PseudoColor) ||
		    (pVisuals->SVtype == GrayScale))
			visuals[i].class = si_colormap.visual;
		break;
	}

	visuals[i].bitsPerRGBValue = pVisuals->SVbitsrgb;
	visuals[i].ColormapEntries = pVisuals->SVcmapsz;
	visuals[i].nplanes = pVisuals->SVdepth;
	if (visuals[i].class & DynamicClass) {
		visuals[i].redMask   = pVisuals->SVredmask;
		visuals[i].greenMask = pVisuals->SVgreenmask;
		visuals[i].blueMask  = pVisuals->SVbluemask;
		visuals[i].offsetRed   = pVisuals->SVredoffset;
		visuals[i].offsetGreen = pVisuals->SVgreenoffset;
		visuals[i].offsetBlue  = pVisuals->SVblueoffset;
	}
    }
    /* SI: end */

    pScreen->defColormap = FakeClientID(0);
    if (defaultColorVisualClass < 0)
    {
	i = 0;
    }
    else
    {
	for (i = 0;
	    (i < numvisuals) && (visuals[i].class != defaultColorVisualClass);
	    i++)
	    ;
	if (i >= numvisuals)
	    i = 0;
    }

    siinitstates(); /* SI */
    siinitfonts();  /* SI */

    pScreen->rootVisual = visuals[i].vid;
#ifdef BACKING_STORE
    miInitializeBackingStore (pScreen, &siBSFuncRec);	/* SI (mfb to si) */
#endif
#ifdef MITSHM
    ShmRegisterFbFuncs(pScreen);
#endif
    return TRUE;
}
