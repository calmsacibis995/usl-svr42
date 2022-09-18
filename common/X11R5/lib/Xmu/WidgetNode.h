/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:WidgetNode.h	1.2"
/*
 * $XConsortium: WidgetNode.h,v 1.7 91/07/22 23:46:16 converse Exp $
 *
 * Copyright 1990 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Jim Fulton, MIT X Consortium
 */

#ifndef _XmuWidgetNode_h
#define _XmuWidgetNode_h

#include <X11/Xfuncproto.h>

/*
 * This is usually initialized by setting the first two fields and letting
 * rest be implicitly nulled (by genlist.sh, for example)
 */
typedef struct _XmuWidgetNode {
    char *label;			/* mixed case name */
    WidgetClass *widget_class_ptr;	/* addr of widget class */
    struct _XmuWidgetNode *superclass;	/* superclass of widget_class */
    struct _XmuWidgetNode *children, *siblings;	/* subclass links */
    char *lowered_label;		/* lowercase version of label */
    char *lowered_classname;		/* lowercase version of class_name */
    Bool have_resources;		/* resources have been fetched */
    XtResourceList resources;		/* extracted resource database */
    struct _XmuWidgetNode **resourcewn;	/* where resources come from */
    Cardinal nresources;		/* number of resources */
    XtResourceList constraints;		/* extracted constraint resources */
    struct _XmuWidgetNode **constraintwn;  /* where constraints come from */
    Cardinal nconstraints;		/* number of constraint resources */
    XtPointer data;			/* extra data */
} XmuWidgetNode;

#define XmuWnClass(wn) ((wn)->widget_class_ptr[0])
#define XmuWnClassname(wn) (XmuWnClass(wn)->core_class.class_name)
#define XmuWnSuperclass(wn) ((XmuWnClass(wn))->core_class.superclass)

					/* external interfaces */
_XFUNCPROTOBEGIN

extern void XmuWnInitializeNodes (
#if NeedFunctionPrototypes
    XmuWidgetNode *	/* nodearray */,
    int			/* nnodes */
#endif
);

extern void XmuWnFetchResources (
#if NeedFunctionPrototypes
    XmuWidgetNode *	/* node */,
    Widget		/* toplevel */,
    XmuWidgetNode *	/* topnode */
#endif
);

extern int XmuWnCountOwnedResources (
#if NeedFunctionPrototypes
    XmuWidgetNode *	/* node */,
    XmuWidgetNode *	/* ownernode */,
    Bool		/* constraints */
#endif
);

extern XmuWidgetNode *XmuWnNameToNode (
#if NeedFunctionPrototypes
    XmuWidgetNode *	/* nodelist */,
    int			/* nnodes */,
    _Xconst char *	/* name */
#endif
);

_XFUNCPROTOEND

#endif /* _XmuWidgetNode_h */

