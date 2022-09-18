/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:BoxP.h	1.2"
/*
* $XConsortium: BoxP.h,v 1.16 89/11/06 10:51:28 swick Exp $
*/


/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/* 
 * BoxP.h - Private definitions for Box widget
 * 
 */

#ifndef _XawBoxP_h
#define _XawBoxP_h

/***********************************************************************
 *
 * Box Widget Private Data
 *
 ***********************************************************************/

#include <X11/Xaw/Box.h>
#include <X11/CompositeP.h>
#include <X11/Xmu/Converters.h>

/* New fields for the Box widget class record */
typedef struct {int empty;} BoxClassPart;

/* Full class record declaration */
typedef struct _BoxClassRec {
    CoreClassPart	core_class;
    CompositeClassPart  composite_class;
    BoxClassPart	box_class;
} BoxClassRec;

extern BoxClassRec boxClassRec;

/* New fields for the Box widget record */
typedef struct {
    /* resources */
    Dimension   h_space, v_space;
    XtOrientation orientation;

    /* private state */
    Dimension	preferred_width, preferred_height;
    Dimension	last_query_width, last_query_height;
    XtGeometryMask last_query_mode;
} BoxPart;


/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _BoxRec {
    CorePart	    core;
    CompositePart   composite;
    BoxPart 	    box;
} BoxRec;

#endif /* _XawBoxP_h */
