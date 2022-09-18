/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:TextSrcP.h	1.2"
/*
* $XConsortium: TextSrcP.h,v 1.18 91/02/20 17:58:06 converse Exp $
*/


/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/* 
 * TextSrcP.h - Private definitions for TextSrc object
 * 
 */

#ifndef _XawTextSrcP_h
#define _XawTextSrcP_h

/***********************************************************************
 *
 * TextSrc Object Private Data
 *
 ***********************************************************************/

#include <X11/ObjectP.h>
#include <X11/Xaw/TextP.h>	/* This source works with the Text widget. */
#include <X11/Xaw/TextSrc.h>

/************************************************************
 *
 * New fields for the TextSrc object class record.
 *
 ************************************************************/

typedef struct _TextSrcClassPart {
  XawTextPosition	(*Read)();
  int			(*Replace)();
  XawTextPosition	(*Scan)();
  XawTextPosition       (*Search)();
  void                  (*SetSelection)();
  Boolean		(*ConvertSelection)();
} TextSrcClassPart;

/* Full class record declaration */
typedef struct _TextSrcClassRec {
    ObjectClassPart     object_class;
    TextSrcClassPart	textSrc_class;
} TextSrcClassRec;

extern TextSrcClassRec textSrcClassRec;

/* New fields for the TextSrc object record */
typedef struct {
    /* resources */
  XawTextEditType	edit_mode;
} TextSrcPart;

/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _TextSrcRec {
  ObjectPart    object;
  TextSrcPart	textSrc;
} TextSrcRec;

/************************************************************
 *
 * Private declarations.
 *
 ************************************************************/

typedef Boolean (*_XawBooleanFunc)();
typedef int (*_XawIntFunc)();
typedef XawTextPosition (*_XawTextPositionFunc)();
typedef void (*_XawTextVoidFunc)();

#define XtInheritRead                 ((_XawTextPositionFunc) _XtInherit)
#define XtInheritReplace              ((_XawIntFunc) _XtInherit)
#define XtInheritScan                 ((_XawTextPositionFunc) _XtInherit)
#define XtInheritSearch               ((_XawTextPositionFunc) _XtInherit)
#define XtInheritSetSelection         ((_XawTextVoidFunc) _XtInherit)
#define XtInheritConvertSelection     ((_XawBooleanFunc) _XtInherit)

#endif /* _XawTextSrcP_h */
