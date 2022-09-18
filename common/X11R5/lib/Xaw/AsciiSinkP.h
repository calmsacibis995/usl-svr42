/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:AsciiSinkP.h	1.2"
/*
* $XConsortium: AsciiSinkP.h,v 1.2 89/10/04 13:56:34 kit Exp $
*/


/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/* 
 * asciiSinkP.h - Private definitions for asciiSink object
 * 
 */

#ifndef _XawAsciiSinkP_h
#define _XawAsciiSinkP_h

/***********************************************************************
 *
 * AsciiSink Object Private Data
 *
 ***********************************************************************/

#include <X11/Xaw/TextSinkP.h> 
#include <X11/Xaw/AsciiSink.h> 

/************************************************************
 *
 * New fields for the AsciiSink object class record.
 *
 ************************************************************/

typedef struct _AsciiSinkClassPart {
  int foo;
} AsciiSinkClassPart;

/* Full class record declaration */

typedef struct _AsciiSinkClassRec {
    ObjectClassPart     object_class;
    TextSinkClassPart	text_sink_class;
    AsciiSinkClassPart	ascii_sink_class;
} AsciiSinkClassRec;

extern AsciiSinkClassRec asciiSinkClassRec;

/* New fields for the AsciiSink object record */
typedef struct {
    /* public resources */
    Boolean echo;
    Boolean display_nonprinting;

    /* private state */
    GC normgc, invgc, xorgc;
    Pixmap insertCursorOn;
    XawTextInsertState laststate;
    short cursor_x, cursor_y;	/* Cursor Location. */
} AsciiSinkPart;

/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _AsciiSinkRec {
    ObjectPart          object;
    TextSinkPart	text_sink;
    AsciiSinkPart	ascii_sink;
} AsciiSinkRec;

#endif /* _XawAsciiSinkP_h */

