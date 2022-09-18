/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:include/pixmapstr.h	1.3"

/* $XConsortium: pixmapstr.h,v 5.0 89/06/09 15:00:35 keith Exp $ */
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
                        All Rights Reserved
******************************************************************/

#ifndef PIXMAPSTRUCT_H
#define PIXMAPSTRUCT_H
#include "pixmap.h"
#include "screenint.h"
#include "miscstruct.h"

typedef struct _Drawable {
    unsigned char	type;	/* DRAWABLE_<type> */
    unsigned char	class;	/* specific to type */
    unsigned char	depth;
    unsigned char	bitsPerPixel;
    unsigned long	id;	/* resource id */
    short		x;	/* window: screen absolute, pixmap: 0 */
    short		y;	/* window: screen absolute, pixmap: 0 */
    unsigned short	width;
    unsigned short	height;
    ScreenPtr		pScreen;
    unsigned long	serialNumber;
} DrawableRec;

/*
 * PIXMAP -- device dependent 
 */

typedef struct _Pixmap {
    DrawableRec		drawable;
    int			refcnt;
    int			devKind;
    DevUnion		devPrivate;
} PixmapRec;

#endif /* PIXMAPSTRUCT_H */
