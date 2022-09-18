/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:CallbackI.h	1.1"
/* $XConsortium: CallbackI.h,v 1.13 90/12/29 12:12:48 rws Exp $ */
/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/****************************************************************
 *
 * Callbacks
 *
 ****************************************************************/

typedef XrmResource **CallbackTable;

#define _XtCBCalling 1
#define _XtCBFreeAfterCalling 2

typedef struct internalCallbackRec {
    unsigned short count;
    char	   is_padded;	/* contains NULL padding for external form */
    char	   call_state;  /* combination of _XtCB{FreeAfter}Calling */
    /* XtCallbackList */
} InternalCallbackRec, *InternalCallbackList;

extern void _XtAddCallback(
#if NeedFunctionPrototypes
    InternalCallbackList*	/* callbacks */,
    XtCallbackProc		/* callback */,
    XtPointer 			/* closure */
#endif
);

extern void _XtAddCallbackOnce(
#if NeedFunctionPrototypes
    InternalCallbackList*	/* callbacks */,
    XtCallbackProc		/* callback */,
    XtPointer 			/* closure */
#endif
);

extern InternalCallbackList _XtCompileCallbackList(
#if NeedFunctionPrototypes
    XtCallbackList		/* xtcallbacks */
#endif
);

extern XtCallbackList _XtGetCallbackList(
#if NeedFunctionPrototypes
    InternalCallbackList*	/* callbacks */
#endif
);

extern void _XtRemoveAllCallbacks(
#if NeedFunctionPrototypes
    InternalCallbackList*	/* callbacks */
#endif
);

extern void _XtRemoveCallback(
#if NeedFunctionPrototypes
    InternalCallbackList*	/* callbacks */,
    XtCallbackProc		/* callback */,
    XtPointer			/* closure */
#endif
);
