/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5include:Xprotostr.h	1.1"
/* $XConsortium: Xprotostr.h,v 1.4 91/03/13 20:09:34 rws Exp $ */
#ifndef XPROTOSTRUCTS_H
#define XPROTOSTRUCTS_H

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/
#include <X11/Xmd.h>

/* Used by PolySegment */

typedef struct _xSegment {
    INT16 x1 B16, y1 B16, x2 B16, y2 B16;
} xSegment;

/* POINT */

typedef struct _xPoint {
	INT16		x B16, y B16;
} xPoint;

typedef struct _xRectangle {
    INT16 x B16, y B16;
    CARD16  width B16, height B16;
} xRectangle;

/*  ARC  */

typedef struct _xArc {
    INT16 x B16, y B16;
    CARD16   width B16, height B16;
    INT16   angle1 B16, angle2 B16;
} xArc;

#endif /* XPROTOSTRUCTS_H */
