/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/siclip.c	1.3"

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

/************************************************************
Copyright 1989 by The Massachusetts Institute of Technology
********************************************************/

#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "misc.h"
#include "dixfontstr.h"
#include "fontstruct.h"
#include "gc.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "region.h"
#include "regionstr.h"

#include "si.h"
#include "sidep.h"
#include "mistruct.h"
#include "mibstore.h"

#include "simskbits.h"

void
siDestroyClip(pGC)
    GCPtr       pGC;
{
    if(pGC->clientClipType == CT_NONE)
        return;
    else if (pGC->clientClipType == CT_PIXMAP)
    {
        siDestroyPixmap((PixmapPtr)(pGC->clientClip));
    }
    else
    {
        /* we know we'll never have a list of rectangles, since
           ChangeClip immediately turns them into a region
        */
        (*pGC->pScreen->RegionDestroy)(pGC->clientClip);
    }
    pGC->clientClip = NULL;
    pGC->clientClipType = CT_NONE;
}

void
siChangeClip(pGC, type, pvalue, nrects)
    GCPtr       pGC;
    int         type;
    pointer     pvalue;
    int         nrects;
{
    siDestroyClip(pGC);
    if(type == CT_PIXMAP)
    {
        pGC->clientClip = (pointer) (*pGC->pScreen->BitmapToRegion)((PixmapPtr)pvalue);
        (*pGC->pScreen->DestroyPixmap)(pvalue);
    }
    else if (type == CT_REGION) {
        /* stuff the region in the GC */
        pGC->clientClip = pvalue;
    }
    else if (type != CT_NONE)
    {
        pGC->clientClip = (pointer) (*pGC->pScreen->RectsToRegion)(nrects,
                                                    (xRectangle *)pvalue,
                                                    type);
        xfree(pvalue);
    }
    pGC->clientClipType = (type != CT_NONE && pGC->clientClip) ? CT_REGION :
                                                                 CT_NONE;
    pGC->stateChanges |= GCClipMask;
}

void
siCopyClip (pgcDst, pgcSrc)
    GCPtr pgcDst, pgcSrc;
{
    RegionPtr prgnNew;

    switch(pgcSrc->clientClipType)
    {
      case CT_PIXMAP:
        ((PixmapPtr) pgcSrc->clientClip)->refcnt++;
        /* Fall through !! */
      case CT_NONE:
        siChangeClip(pgcDst, (int)pgcSrc->clientClipType, pgcSrc->clientClip,
                      0);
        break;
      case CT_REGION:
        prgnNew = (*pgcSrc->pScreen->RegionCreate)(NULL, 1);
        (*pgcSrc->pScreen->RegionCopy)(prgnNew,
                                       (RegionPtr)(pgcSrc->clientClip));
        siChangeClip(pgcDst, CT_REGION, (pointer)prgnNew, 0);
        break;
    }
}
