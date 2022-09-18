/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:LabelP.h	1.2"
/*
* $XConsortium: LabelP.h,v 1.27 91/06/22 19:34:58 rws Exp $
*/


/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/* 
 * LabelP.h - Private definitions for Label widget
 * 
 */

#ifndef _XawLabelP_h
#define _XawLabelP_h

/***********************************************************************
 *
 * Label Widget Private Data
 *
 ***********************************************************************/

#include <X11/Xaw/Label.h>
#include <X11/Xaw/SimpleP.h>

/* New fields for the Label widget class record */

typedef struct {int foo;} LabelClassPart;

/* Full class record declaration */
typedef struct _LabelClassRec {
    CoreClassPart	core_class;
    SimpleClassPart	simple_class;
    LabelClassPart	label_class;
} LabelClassRec;

extern LabelClassRec labelClassRec;

/* New fields for the Label widget record */
typedef struct {
    /* resources */
    Pixel	foreground;
    XFontStruct	*font;
    char	*label;
    XtJustify	justify;
    Dimension	internal_width;
    Dimension	internal_height;
    Pixmap	pixmap;
    Boolean	resize;
    unsigned char encoding;
    Pixmap	left_bitmap;

    /* private state */
    GC		normal_GC;
    GC          gray_GC;
    Pixmap	stipple;
    Position	label_x;
    Position	label_y;
    Dimension	label_width;
    Dimension	label_height;
    Dimension	label_len;
    int		lbm_y;			/* where in label */
    unsigned int lbm_width, lbm_height;	 /* size of pixmap */
} LabelPart;


/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _LabelRec {
    CorePart	core;
    SimplePart	simple;
    LabelPart	label;
} LabelRec;

#define LEFT_OFFSET(lw) ((lw)->label.left_bitmap \
			 ? (lw)->label.lbm_width + (lw)->label.internal_width \
			 : 0)

#endif /* _XawLabelP_h */
