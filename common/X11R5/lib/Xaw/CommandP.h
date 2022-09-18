/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:CommandP.h	1.2"
/*
* $XConsortium: CommandP.h,v 1.30 90/12/01 13:00:10 rws Exp $
*/


/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/* 
 * CommandP.h - Private definitions for Command widget
 * 
 */

#ifndef _XawCommandP_h
#define _XawCommandP_h

#include <X11/Xaw/Command.h>
#include <X11/Xaw/LabelP.h>

/***********************************************************************
 *
 * Command Widget Private Data
 *
 ***********************************************************************/

typedef enum {
  HighlightNone,		/* Do not highlight. */
  HighlightWhenUnset,		/* Highlight only when unset, this is
				   to preserve current command widget 
				   functionality. */
  HighlightAlways		/* Always highlight, lets the toggle widget
				   and other subclasses do the right thing. */
} XtCommandHighlight;

/************************************
 *
 *  Class structure
 *
 ***********************************/


   /* New fields for the Command widget class record */
typedef struct _CommandClass 
  {
    int makes_compiler_happy;  /* not used */
  } CommandClassPart;

   /* Full class record declaration */
typedef struct _CommandClassRec {
    CoreClassPart	core_class;
    SimpleClassPart	simple_class;
    LabelClassPart	label_class;
    CommandClassPart    command_class;
} CommandClassRec;

extern CommandClassRec commandClassRec;

/***************************************
 *
 *  Instance (widget) structure 
 *
 **************************************/

    /* New fields for the Command widget record */
typedef struct {
    /* resources */
    Dimension   highlight_thickness;
    XtCallbackList callbacks;

    /* private state */
    Pixmap      	gray_pixmap;
    GC          	normal_GC;
    GC          	inverse_GC;
    Boolean     	set;
    XtCommandHighlight	highlighted;
    /* more resources */
    int			shape_style;    
    Dimension		corner_round;
} CommandPart;


/*    XtEventsPtr eventTable;*/


   /* Full widget declaration */
typedef struct _CommandRec {
    CorePart         core;
    SimplePart	     simple;
    LabelPart	     label;
    CommandPart      command;
} CommandRec;

#endif /* _XawCommandP_h */


