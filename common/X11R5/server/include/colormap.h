/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:include/colormap.h	1.3"

/*
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



*/
/* $XConsortium: colormap.h,v 1.22 90/01/13 17:30:13 rws Exp $ */
#ifndef CMAP_H
#define CMAP_H 1

/* these follow X.h's AllocNone and AllocAll */
#define CM_PSCREEN 2
#define CM_PWIN	   3
/* Passed internally in colormap.c */
#define REDMAP 0
#define GREENMAP 1
#define BLUEMAP 2
#define PSEUDOMAP 3
#define AllocPrivate (-1)
#define AllocTemporary (-2)
#define DynamicClass  1

#ifdef notdef
/* Gets the color from a cell as an Rvalue */
#define RRED(cell) (((cell)->fShared) ? ((cell)->co.shco.red->color): \
		    ((cell)->co.local.red))
#define RGREEN(cell) (((cell)->fShared) ? ((cell)->co.shco.green->color): \
		    ((cell)->co.local.green))
#define RBLUE(cell) (((cell)->fShared) ? ((cell)->co.shco.blue->color): \
		    ((cell)->co.local.blue))

/* Gets the color from a cell as an L value */
#define LRED(cell) (((cell)->fShared) ? (&((cell)->co.shco.red->color)) : \
		    (&((cell)->co.local.red)))
#define LGREEN(cell) (((cell)->fShared) ? (&((cell)->co.shco.green->color)) : \
		    (&((cell)->co.local.green)))
#define LBLUE(cell) (((cell)->fShared) ? (&((cell)->co.shco.blue->color)) : \
		    (&((cell)->co.local.blue)))
#endif

/* Values for the flags field of a colormap. These should have 1 bit set
 * and not overlap */
#define IsDefault 1
#define AllAllocated 2
#define BeingCreated 4


typedef unsigned long	Pixel;
typedef struct _CMEntry *EntryPtr;
typedef struct _ColormapRec *ColormapPtr;

extern int CreateColormap();
extern int FindColor();
extern int FreeColormap();
extern int TellLostMap();
extern int TellGainedMap();
extern int IsMapInstalled();
extern void UninstallColormap();

#endif /* CMAP_H */
