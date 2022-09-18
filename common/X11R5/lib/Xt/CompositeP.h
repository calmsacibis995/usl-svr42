/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* ident	"@(#)R5Xt:CompositeP.h	1.2" */
/* $XConsortium: CompositeP.h,v 1.15 91/10/24 13:20:18 converse Exp $ */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#ifndef _XtCompositeP_h
#define _XtCompositeP_h

#include <X11/Composite.h>

/************************************************************************
 *
 * Additional instance fields for widgets of (sub)class 'Composite' 
 *
 ************************************************************************/

typedef struct _CompositePart {
    WidgetList  children;	     /* array of ALL widget children	     */
    Cardinal    num_children;	     /* total number of widget children	     */
    Cardinal    num_slots;           /* number of slots in children array    */
    XtOrderProc insert_position;     /* compute position of new child	     */
} CompositePart,*CompositePtr;

typedef struct _CompositeRec {
    CorePart      core;
    CompositePart composite;
} CompositeRec;

/*********************************************************************
 *
 *  Additional class fields for widgets of (sub)class 'Composite'
 *
 ********************************************************************/

typedef struct _CompositeClassPart {
    XtGeometryHandler geometry_manager;	  /* geometry manager for children   */
    XtWidgetProc      change_managed;	  /* change managed state of child   */
    XtWidgetProc      insert_child;	  /* physically add child to parent  */
    XtWidgetProc      delete_child;	  /* physically remove child	     */
    XtPointer	      extension;	  /* pointer to extension record     */
} CompositeClassPart,*CompositePartPtr;

typedef struct {
    XtPointer next_extension;	/* 1st 4 mandated for all extension records */
    XrmQuark record_type;	/* NULLQUARK; on CompositeClassPart */
    long version;		/* must be XtCompositeExtensionVersion */
    Cardinal record_size;	/* sizeof(CompositeClassExtensionRec) */
    Boolean accepts_objects;
} CompositeClassExtensionRec, *CompositeClassExtension;


typedef struct _CompositeClassRec {
     CoreClassPart      core_class;
     CompositeClassPart composite_class;
} CompositeClassRec;

externalref CompositeClassRec compositeClassRec;

#define XtCompositeExtensionVersion 1L
#define XtInheritGeometryManager ((XtGeometryHandler) _XtInherit)
#define XtInheritChangeManaged ((XtWidgetProc) _XtInherit)
#define XtInheritInsertChild ((XtWidgetProc) _XtInherit)
#define XtInheritDeleteChild ((XtWidgetProc) _XtInherit)

#endif /* _XtCompositeP_h */
/* DON'T ADD STUFF AFTER THIS #endif */
