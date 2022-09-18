/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:include/colormapst.h	1.3"

/*
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved
*/
/* $XConsortium: colormapst.h,v 1.6 88/09/06 15:49:00 jim Exp $ */

#ifndef CMAPSTRUCT_H
#define CMAPSTRUCT_H 1

#include "colormap.h"
#include "screenint.h"

/* Shared color -- the color is used by AllocColorPlanes */
typedef struct
{
    unsigned short color;
    short  refcnt;
} SHAREDCOLOR;

/* LOCO -- a local color for a PseudoColor cell. DirectColor maps always
 * use the first value (called red) in the structure.  What channel they
 * are really talking about depends on which map they are in. */
typedef struct
{
    unsigned short	red, green, blue;
} LOCO;

/* SHCO -- a shared color for a PseudoColor cell. Used with AllocColorPlanes.
 * DirectColor maps always use the first value (called red) in the structure.
 * What channel they are really talking about depends on which map they
 * are in. */
typedef struct 
{
    SHAREDCOLOR *red, *green, *blue;
} SHCO;


/* color map entry */
typedef struct _CMEntry
{
    union
    {
	LOCO	local;
	SHCO	shco;
    } co;
    short	refcnt;
    Bool	fShared;
} Entry;

/* COLORMAPs can be used for either Direct or Pseudo color.  PseudoColor
 * only needs one cell table, we arbitrarily pick red.  We keep track
 * of that table with freeRed, numPixelsRed, and clientPixelsRed */

typedef struct _ColormapRec
{
    VisualPtr	pVisual;
    short	class;		/* PseudoColor or DirectColor */
    long	mid;		/* client's name for colormap */
    ScreenPtr	pScreen;	/* screen map is associated with */
    short	flags;		/* 1 = IsDefault
				 * 2 = AllAllocated */
    int		freeRed;
    int		freeGreen;
    int		freeBlue;
    int		*numPixelsRed;	
    int		*numPixelsGreen;	
    int		*numPixelsBlue;	
    Pixel	**clientPixelsRed;
    Pixel	**clientPixelsGreen;
    Pixel	**clientPixelsBlue;
    Entry	*red;
    Entry 	*green;
    Entry	*blue;
    pointer	devPriv;
} ColormapRec;
	      
#endif /* COLORMAP_H */
