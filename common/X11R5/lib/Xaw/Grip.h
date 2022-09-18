/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:Grip.h	1.2"
/* $XConsortium: Grip.h,v 1.18 91/07/26 19:42:40 converse Exp $ */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/*
 *  Grip.h - Public Definitions for Grip widget (used by VPane Widget)
 *
 */

#ifndef _XawGrip_h
#define _XawGrip_h

#include <X11/Xaw/Simple.h>

/***************************************************************************
 *
 * Grip Widget 
 *
 **************************************************************************/

/* Parameters:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 foreground	     Foreground		Pixel		XtDefaultForeground
 border		     BorderColor	Pixel		XtDefaultForeground
 borderWidth	     BorderWidth	Dimension	0
 callback	     Callback		Pointer		GripAction
 cursor		     Cursor		Cursor		None
 cursorName	     Cursor		String		NULL
 destroyCallback     Callback		Pointer		NULL
 height		     Height		Dimension	8
 mappedWhenManaged   MappedWhenManaged	Boolean		True
 pointerColor	     Foreground		Pixel		XtDefaultForeground
 pointerColorBackground Background	Pixel		XtDefaultBackground
 sensitive	     Sensitive		Boolean		True
 width		     Width		Dimension	8
 x		     Position		Position	0
 y		     Position		Position	0

*/

#define XtNgripTranslations "gripTranslations"

typedef struct _XawGripCallData {
  XEvent *event;		/* the event causing the GripAction */
  String *params;		/* the TranslationTable params */
  Cardinal num_params;		/* count of params */
} XawGripCallDataRec, *XawGripCallData,
    GripCallDataRec, *GripCallData; /* supported for R4 compatibility */

/* Class Record Constant */

extern WidgetClass gripWidgetClass;

typedef struct _GripClassRec *GripWidgetClass;
typedef struct _GripRec      *GripWidget;

#endif /* _XawGrip_h */
