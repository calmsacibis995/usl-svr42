/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:include/pixmap.h	1.3"

/* $XConsortium: pixmap.h,v 5.2 89/06/09 18:22:03 keith Exp $ */
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
                        All Rights Reserved
******************************************************************/

#ifndef PIXMAP_H
#define PIXMAP_H

/* types for Drawable */
#define DRAWABLE_WINDOW 0
#define DRAWABLE_PIXMAP 1
#define UNDRAWABLE_WINDOW 2

/* flags to PaintWindow() */
#define PW_BACKGROUND 0
#define PW_BORDER 1

#define NullPixmap ((PixmapPtr)0)

typedef struct _Drawable *DrawablePtr;	
typedef struct _Pixmap *PixmapPtr;

typedef union _PixUnion {
    PixmapPtr		pixmap;
    unsigned long	pixel;
} PixUnion;

#define SamePixUnion(a,b,isPixel)\
    ((isPixel) ? (a).pixel == (b).pixel : (a).pixmap == (b).pixmap)

#define EqualPixUnion(as, a, bs, b)				\
    ((as) == (bs) && (SamePixUnion (a, b, as)))

#endif /* PIXMAP_H */
