/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* ident	"@(#)R5Xt:Composite.h	1.2" */
/* $XConsortium: Composite.h,v 1.12 91/10/24 13:19:40 converse Exp $ */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#ifndef _XtComposite_h
#define _XtComposite_h

typedef struct _CompositeClassRec *CompositeWidgetClass;

typedef Cardinal (*XtOrderProc)(
#if NeedFunctionPrototypes
    Widget 	/* child */
#endif
);

_XFUNCPROTOBEGIN

extern void XtManageChildren(
#if NeedFunctionPrototypes
    WidgetList 		/* children */,
    Cardinal 		/* num_children */
#endif
);

extern void XtManageChild(
#if NeedFunctionPrototypes
    Widget 		/* child */
#endif
);

extern void XtUnmanageChildren(
#if NeedFunctionPrototypes
    WidgetList 		/* children */,
    Cardinal 		/* num_children */
#endif
);

extern void XtUnmanageChild(
#if NeedFunctionPrototypes
    Widget 		/* child */
#endif
);

_XFUNCPROTOEND

#ifndef COMPOSITE
externalref WidgetClass compositeWidgetClass;
#endif

#endif /* _XtComposite_h */
/* DON'T ADD STUFF AFTER THIS #endif */
