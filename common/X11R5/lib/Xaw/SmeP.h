/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:SmeP.h	1.2"
/*
 * $XConsortium: SmeP.h,v 1.4 89/12/11 15:20:22 kit Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 */

/*
 * SmeP.h - Private Header file for Sme object.
 *
 * This is the private header file for the Athena Sme object.
 * This object is intended to be used with the simple menu widget.  
 *
 * Date:    April 3, 1989
 *
 * By:      Chris D. Peterson
 *          MIT X Consortium 
 *          kit@expo.lcs.mit.edu
 */

#ifndef _XawSmeP_h
#define _XawSmeP_h

/***********************************************************************
 *
 * Sme Widget Private Data
 *
 ***********************************************************************/

#include <X11/RectObjP.h>
#include <X11/Xaw/Sme.h>

/************************************************************
 *
 * New fields for the Sme widget class record.
 *
 ************************************************************/

typedef struct _SmeClassPart {
  void (*highlight)();
  void (*unhighlight)();
  void (*notify)();	
  XtPointer extension;
} SmeClassPart;

/* Full class record declaration */
typedef struct _SmeClassRec {
    RectObjClassPart    rect_class;
    SmeClassPart	sme_class;
} SmeClassRec;

extern SmeClassRec smeClassRec;

/* New fields for the Sme widget record */
typedef struct {
    /* resources */
    XtCallbackList callbacks;	/* The callback list */

} SmePart;

/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _SmeRec {
  ObjectPart     object;
  RectObjPart    rectangle;
  SmePart	 sme;
} SmeRec;

/************************************************************
 *
 * Private declarations.
 *
 ************************************************************/

typedef void (*_XawEntryVoidFunc)();

#define XtInheritHighlight   ((_XawEntryVoidFunc) _XtInherit)
#define XtInheritUnhighlight XtInheritHighlight
#define XtInheritNotify      XtInheritHighlight

#endif /* _XawSmeP_h */
