/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:GripP.h	1.2"
/*
* $XConsortium: GripP.h,v 1.14 89/05/11 01:05:27 kit Exp $
*/


/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/*
 *  GripP.h - Private definitions for Grip widget (Used by VPane Widget)
 *
 */

#ifndef _XawGripP_h
#define _XawGripP_h

#include <X11/Xaw/Grip.h>
#include <X11/Xaw/SimpleP.h>

/*****************************************************************************
 *
 * Grip Widget Private Data
 *
 *****************************************************************************/

#define DEFAULT_GRIP_SIZE 8

/* New fields for the Grip widget class record */
typedef struct {int empty;} GripClassPart;

/* Full Class record declaration */
typedef struct _GripClassRec {
    CoreClassPart    core_class;
    SimpleClassPart  simple_class;
    GripClassPart    grip_class;
} GripClassRec;

extern GripClassRec gripClassRec;

/* New fields for the Grip widget record */
typedef struct {
  XtCallbackList grip_action;
} GripPart;

/*****************************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************************/

typedef struct _GripRec {
   CorePart    core;
   SimplePart  simple;
   GripPart    grip;
} GripRec;

#endif /* _XawGripP_h */

