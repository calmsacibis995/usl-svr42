/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:ActionHook.c	1.1"
/* $XConsortium: ActionHook.c,v 1.4 90/12/03 16:30:40 converse Exp $ */

/*LINTLIBRARY*/

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/* 
 * Contains XtAppAddActionHook, XtRemoveActionHook
 */

#include "IntrinsicI.h"


/*ARGSUSED*/
static void FreeActionHookList( widget, closure, call_data )
    Widget widget;		/* unused (and invalid) */
    XtPointer closure;		/* ActionHook* */
    XtPointer call_data;	/* unused */
{
    ActionHook list = *(ActionHook*)closure;
    while (list != NULL) {
	ActionHook next = list->next;
	XtFree( (XtPointer)list );
	list = next;
    }
}


XtActionHookId XtAppAddActionHook( app, proc, closure )
    XtAppContext app;
    XtActionHookProc proc;
    XtPointer closure;
{
    ActionHook hook = XtNew(ActionHookRec);
    hook->next = app->action_hook_list;
    hook->app = app;
    hook->proc = proc;
    hook->closure = closure;
    if (app->action_hook_list == NULL) {
	_XtAddCallback( &app->destroy_callbacks,
		        FreeActionHookList,
		        (XtPointer)&app->action_hook_list
		      );
    }
    app->action_hook_list = hook;
    return (XtActionHookId)hook;
}


void XtRemoveActionHook( id )
    XtActionHookId id;
{
    ActionHook *p, hook = (ActionHook)id;
    XtAppContext app = hook->app;
    for (p = &app->action_hook_list; p != NULL && *p != hook; p = &(*p)->next);
    if (p == NULL) {
#ifdef DEBUG
	XtAppWarningMsg(app, "badId", "xtRemoveActionHook", XtCXtToolkitError,
			"XtRemoveActionHook called with bad or old hook id",
			(String*)NULL, (Cardinal*)NULL);
#endif /*DEBUG*/	
	return;
    }
    *p = hook->next;
    XtFree( (XtPointer)hook );
}
