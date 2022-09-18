/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:EventI.h	1.1"
/* $XConsortium: EventI.h,v 1.18 91/01/29 10:56:58 rws Exp $ */
/* $oHeader: EventI.h,v 1.3 88/08/24 09:21:11 asente Exp $ */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/* 
 * Event.h - exported types and functions for toolkit event handler
 * 
 * Author:	Charles Haynes
 * 		Digital Equipment Corporation
 * 		Western Software Laboratory
 * Date:	Sun Dec  6 1987
 */

#ifndef _Event_h_
#define _Event_h_

typedef struct _XtGrabRec  *XtGrabList;

extern void _XtEventInitialize(
#if NeedFunctionPrototypes
    void
#endif
);

extern void _XtRegisterWindow(
#if NeedFunctionPrototypes
    Window 	/* window */,
    Widget 	/* widget */
#endif
);

extern void _XtUnregisterWindow(
#if NeedFunctionPrototypes
    Window 	/* window */,
    Widget 	/* widget */
#endif
);

typedef struct _XtEventRec {
     XtEventTable	next;
     EventMask		mask;
     XtEventHandler	proc;
     XtPointer		closure;
     unsigned int	select:1;
     unsigned int	async:1; /* not used, here for Digital extension? */
} XtEventRec;

typedef struct _XtGrabRec {
    XtGrabList next;
    Widget   widget;
    unsigned int exclusive:1;
    unsigned int spring_loaded:1;
}XtGrabRec;


extern void _XtFreeEventTable(
#if NeedFunctionPrototypes
    XtEventTable*	/* event_table */
#endif
);

extern Boolean _XtOnGrabList(
#if NeedFunctionPrototypes
    Widget	/* widget */,
    XtGrabRec*	/* grabList */
#endif
);

extern void _XtRemoveAllInputs(
#if NeedFunctionPrototypes
    XtAppContext /* app */
#endif
);

extern EventMask _XtConvertTypeToMask(
#if NeedFunctionPrototypes
    int		/* eventType */
#endif
);

#endif /* _Event_h_ */
