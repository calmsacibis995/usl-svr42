/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:TextSinkP.h	1.2"
/*
* $XConsortium: TextSinkP.h,v 1.4 90/04/30 17:46:39 converse Exp $
*/


/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/* 
 * TextSinkP.h - Private definitions for TextSink object
 * 
 */

#ifndef _XawTextSinkP_h
#define _XawTextSinkP_h

/***********************************************************************
 *
 * TextSink Object Private Data
 *
 ***********************************************************************/

#include <X11/ObjectP.h>
#include <X11/Xaw/TextP.h>	/* This source works with the Text widget. */
#include <X11/Xaw/TextSrcP.h>	/* This source works with the Text Source. */
#include <X11/Xaw/TextSink.h>

/************************************************************
 *
 * New fields for the TextSink object class record.
 *
 ************************************************************/

typedef struct _TextSinkClassPart {
  void (*DisplayText)();
  void (*InsertCursor)();
  void (*ClearToBackground)();
  void (*FindPosition)();
  void (*FindDistance)();
  void (*Resolve)();
  int  (*MaxLines)();
  int  (*MaxHeight)();
  void (*SetTabs)();		
  void (*GetCursorBounds)();	
} TextSinkClassPart;

/* Full class record declaration */
typedef struct _TextSinkClassRec {
    ObjectClassPart     object_class;
    TextSinkClassPart	text_sink_class;
} TextSinkClassRec;

extern TextSinkClassRec textSinkClassRec;

/* New fields for the TextSink object record */
typedef struct {
    /* resources */
    XFontStruct	*font;		/* Font to draw in. */
    Pixel foreground;		/* Foreground color. */
    Pixel background;		/* Background color. */

    /* private state. */
    Position *tabs;		/* The tab stops as pixel values. */
    short    *char_tabs;	/* The tabs stops as character values. */
    int      tab_count;		/* number of items in tabs */

} TextSinkPart;

/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _TextSinkRec {
  ObjectPart    object;
  TextSinkPart	text_sink;
} TextSinkRec;

/************************************************************
 *
 * Private declarations.
 *
 ************************************************************/

typedef int (*_XawSinkIntFunc)();
typedef void (*_XawSinkVoidFunc)();

#define XtInheritDisplayText          ((_XawSinkVoidFunc) _XtInherit)
#define XtInheritInsertCursor         ((_XawSinkVoidFunc) _XtInherit)
#define XtInheritClearToBackground    ((_XawSinkVoidFunc) _XtInherit)
#define XtInheritFindPosition         ((_XawSinkVoidFunc) _XtInherit)
#define XtInheritFindDistance         ((_XawSinkVoidFunc) _XtInherit)
#define XtInheritResolve              ((_XawSinkVoidFunc) _XtInherit)
#define XtInheritMaxLines             ((_XawSinkIntFunc) _XtInherit)
#define XtInheritMaxHeight            ((_XawSinkIntFunc) _XtInherit)
#define XtInheritSetTabs              ((_XawSinkVoidFunc) _XtInherit)
#define XtInheritGetCursorBounds      ((_XawSinkVoidFunc) _XtInherit)

#endif /* _XawTextSinkP_h */
