/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:include/cursorstr.h	1.3"

/* $XConsortium: cursorstr.h,v 1.6 89/08/04 18:23:09 rws Exp $ */
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved
******************************************************************/
#ifndef CURSORSTRUCT_H
#define CURSORSTRUCT_H 

#include "cursor.h"
#include "pixmap.h"
#include "misc.h"
/* 
 * device-independent cursor storage
 */

/*
 * source and mask point directly to the bits, which are in the server-defined
 * bitmap format.
 */
typedef struct _CursorBits {
    unsigned char *source;			/* points to bits */
    unsigned char *mask;			/* points to bits */
    unsigned short width, height, xhot, yhot;	/* metrics */
    int refcnt;					/* can be shared */
    pointer devPriv[MAXSCREENS];		/* set by pScr->RealizeCursor*/
} CursorBits, *CursorBitsPtr;

typedef struct _Cursor {
    CursorBitsPtr bits;
    unsigned short foreRed, foreGreen, foreBlue; /* device-independent color */
    unsigned short backRed, backGreen, backBlue; /* device-independent color */
    int refcnt;
    pointer devPriv[MAXSCREENS];		/* set by pScr->RealizeCursor*/
} CursorRec;

typedef struct _CursorMetric {
    unsigned short width, height, xhot, yhot;
} CursorMetricRec;

extern int		FreeCursor();
extern CursorPtr	AllocCursor();		/* also realizes it */
extern int		AllocGlyphCursor();	/* also realizes it */
				/* created from default cursor font */
extern CursorPtr	CreateRootCursor();

#endif /* CURSORSTRUCT_H */
