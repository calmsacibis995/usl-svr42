/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/mispritest.h	1.2"

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

/* $XConsortium: mispritest.h,v 5.10 90/02/02 19:07:57 keith Exp $ */

/*
Copyright 1989 by the Massachusetts Institute of Technology
*/

/*
 * mispritest.h
 *
 * mi sprite structures
 */

/*
 * per screen information
 */

typedef struct {
    Bool	    (*CloseScreen)();
    void	    (*GetImage)();
    void	    (*GetSpans)();
    void	    (*SourceValidate)();
    Bool	    (*CreateGC)();
    void	    (*BlockHandler)();
    void	    (*InstallColormap)();
    void	    (*StoreColors)();

    void	    (* PaintWindowBackground)();
    void	    (* PaintWindowBorder)();
    void	    (* CopyWindow)();
    void	    (* ClearToBackground)();

    void	    (* SaveDoomedAreas)();
    RegionPtr	    (* RestoreAreas)();

    CursorPtr	    pCursor;
    int		    x;
    int		    y;
    BoxRec	    saved;
    Bool	    isUp;
    Bool	    shouldBeUp;
    WindowPtr	    pCacheWin;
    Bool	    isInCacheWin;
    Bool	    checkPixels;
    xColorItem	    colors[2];
    ColormapPtr	    pInstalledMap;
    ColormapPtr	    pColormap;
    VisualPtr	    pVisual;
} miSpriteScreenRec, *miSpriteScreenPtr;

#define SOURCE_COLOR	0
#define MASK_COLOR	1

typedef struct {
    GCFuncs		*wrapFuncs;
    GCOps		*wrapOps;
} miSpriteGCRec, *miSpriteGCPtr;

extern void QueryGlyphExtents();

/*
 * Overlap BoxPtr and Box elements
 */
#define BOX_OVERLAP(pCbox,X1,Y1,X2,Y2) \
 	(((pCbox)->x1 <= (X2)) && ((X1) <= (pCbox)->x2) && \
	 ((pCbox)->y1 <= (Y2)) && ((Y1) <= (pCbox)->y2))

/*
 * Overlap BoxPtr, origins, and rectangle
 */
#define ORG_OVERLAP(pCbox,xorg,yorg,x,y,w,h) \
    BOX_OVERLAP((pCbox),(x)+(xorg),(y)+(yorg),(x)+(xorg)+(w),(y)+(yorg)+(h))

/*
 * Overlap BoxPtr, origins and RectPtr
 */
#define ORGRECT_OVERLAP(pCbox,xorg,yorg,pRect) \
    ORG_OVERLAP((pCbox),(xorg),(yorg),(pRect)->x,(pRect)->y, \
		(int)((pRect)->width), (int)((pRect)->height))
/*
 * Overlap BoxPtr and horizontal span
 */
#define SPN_OVERLAP(pCbox,y,x,w) BOX_OVERLAP((pCbox),(x),(y),(x)+(w),(y))

#define LINE_SORT(x1,y1,x2,y2) \
{ int _t; \
  if (x1 > x2) { _t = x1; x1 = x2; x2 = _t; } \
  if (y1 > y2) { _t = y1; y1 = y2; y2 = _t; } }

#define LINE_OVERLAP(pCbox,x1,y1,x2,y2,lw2) \
    BOX_OVERLAP((pCbox), (x1)-(lw2), (y1)-(lw2), (x2)+(lw2), (y2)+(lw2))

