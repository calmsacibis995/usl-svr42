/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/sipixmap.c	1.3"

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
/* pixmap management
   written by drewry, september 1986

   on a monchrome device, a pixmap is a bitmap.
*/

#include "Xmd.h"
#include "servermd.h"
#include "pixmapstr.h"
#include "si.h" 		/* SI: */
#include "mi.h"

PixmapPtr
siCreatePixmap (pScreen, width, height, depth)
    ScreenPtr	pScreen;
    int		width;
    int		height;
    int		depth;
{
    register PixmapPtr pPixmap;
    int size;

    size = PixmapBytePad(width, depth);
    pPixmap = (PixmapPtr)xalloc(sizeof(PixmapRec) + (height * size));
    if (!pPixmap)
	return NullPixmap;
    pPixmap->drawable.type = DRAWABLE_PIXMAP;
    pPixmap->drawable.class = 0;
    pPixmap->drawable.pScreen = pScreen;
    pPixmap->drawable.depth = depth;
    pPixmap->drawable.bitsPerPixel = depth;
    pPixmap->drawable.id = 0;
    pPixmap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    pPixmap->drawable.x = 0;
    pPixmap->drawable.y = 0;
    pPixmap->drawable.width = width;
    pPixmap->drawable.height = height;
    pPixmap->devKind = size;
    pPixmap->refcnt = 1;
    pPixmap->devPrivate.ptr = (pointer)(pPixmap + 1);
    return pPixmap;
}

Bool
siDestroyPixmap(pPixmap)
    PixmapPtr pPixmap;
{
    if (pPixmap != NullPixmap) {
	if (--pPixmap->refcnt > 0)
		return TRUE;
	xfree(pPixmap);
	pPixmap = NullPixmap;
    }
    return TRUE;
}
