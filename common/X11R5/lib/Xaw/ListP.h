/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:ListP.h	1.2"
/*
 * $XConsortium: ListP.h,v 1.12 89/12/11 15:09:04 kit Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Chris D. Peterson, MIT X Consortium
 */


/* 
 * ListP.h - Private definitions for List widget
 * 
 * This is the List widget, it is useful to display a list, without the
 * overhead of having a widget for each item in the list.  It allows 
 * the user to select an item in a list and notifies the application through
 * a callback function.
 *
 *	Created: 	8/13/88
 *	By:		Chris D. Peterson
 *                      MIT - Project Athena
 */

#ifndef _XawListP_h
#define _XawListP_h

/***********************************************************************
 *
 * List Widget Private Data
 *
 ***********************************************************************/

#include <X11/Xaw/SimpleP.h>
#include <X11/Xaw/List.h>

#define NO_HIGHLIGHT            XAW_LIST_NONE
#define OUT_OF_RANGE            -1
#define OKAY                     0

/* New fields for the List widget class record */

typedef struct {int foo;} ListClassPart;

/* Full class record declaration */
typedef struct _ListClassRec {
    CoreClassPart	core_class;
    SimpleClassPart	simple_class;
    ListClassPart	list_class;
} ListClassRec;

extern ListClassRec listClassRec;

/* New fields for the List widget record */
typedef struct {
    /* resources */
    Pixel	foreground;
    Dimension	internal_width,
        	internal_height,
                column_space,
                row_space;
    int         default_cols;
    Boolean     force_cols,
                paste,
                vertical_cols;
    int         longest;
    int         nitems;		/* number of items in the list. */
    XFontStruct	*font;
    String *    list;
    XtCallbackList  callback;

    /* private state */

    int         is_highlighted,	/* set to the item currently highlighted. */
                highlight,	/*set to the item that should be highlighted.*/
                col_width,	/* width of each column. */
                row_height,	/* height of each row. */
                nrows,		/* number of rows in the list. */
                ncols;		/* number of columns in the list. */
    GC		normgc,		/* a couple o' GC's. */
                revgc,
                graygc;		/* used when inactive. */

} ListPart;


/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _ListRec {
    CorePart	core;
    SimplePart	simple;
    ListPart	list;
} ListRec;

#endif /* _XawListP_h */
