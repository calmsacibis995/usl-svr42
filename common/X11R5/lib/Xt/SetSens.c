/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:SetSens.c	1.1"
/* $XConsortium: SetSens.c,v 1.3 91/01/06 13:32:42 rws Exp $ */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#include "IntrinsicI.h"
#include "StringDefs.h"

/*
 *	XtSetSensitive()
 */

static void SetAncestorSensitive(widget, ancestor_sensitive)
    register Widget widget;
    Boolean	    ancestor_sensitive;
{
    Arg			args[1];
    register Cardinal   i;
    register WidgetList children;

    if (widget->core.ancestor_sensitive == ancestor_sensitive) return;

    XtSetArg(args[0], XtNancestorSensitive, ancestor_sensitive);
    XtSetValues(widget, args, XtNumber(args));

    /* If widget's sensitive is TRUE, propagate new ancestor_sensitive to
       children's ancestor_sensitive; else do nothing as children's
       ancestor_sensitive is already FALSE */
    
    if (widget->core.sensitive && XtIsComposite(widget)) {
	children = ((CompositeWidget) widget)->composite.children;
	for (i=0; i < ((CompositeWidget)widget)->composite.num_children; i++) {
	    SetAncestorSensitive (children[i], ancestor_sensitive);
	}
    }
} /* SetAncestorSensitive */


#if NeedFunctionPrototypes
void XtSetSensitive(
    register Widget widget,
    _XtBoolean	    sensitive
    )
#else
void XtSetSensitive(widget, sensitive)
    register Widget widget;
    Boolean	    sensitive;
#endif
{
    Arg			args[1];
    register Cardinal   i;
    register WidgetList children;

    if (widget->core.sensitive == sensitive) return;

    XtSetArg(args[0], XtNsensitive, sensitive);
    XtSetValues(widget, args, XtNumber(args));

    /* If widget's ancestor_sensitive is TRUE, propagate new sensitive to
       children's ancestor_sensitive; else do nothing as children's
       ancestor_sensitive is already FALSE */
    
    if (widget->core.ancestor_sensitive && XtIsComposite (widget)) {
	children = ((CompositeWidget) widget)->composite.children;
	for (i = 0; i < ((CompositeWidget)widget)->composite.num_children; i++){
	    SetAncestorSensitive (children[i], sensitive);
	}
    }
} /* XtSetSensitive */
