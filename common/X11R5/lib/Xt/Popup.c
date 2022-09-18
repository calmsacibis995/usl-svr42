/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:Popup.c	1.1"
/* $XConsortium: Popup.c,v 1.30 91/05/09 18:07:59 swick Exp $ */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#include "IntrinsicI.h"
#include "Shell.h"
#include "ShellP.h"
#include "StringDefs.h"

#if NeedFunctionPrototypes
void _XtPopup(
    Widget      widget,
    XtGrabKind  grab_kind,
    _XtBoolean  spring_loaded
    )
#else
void _XtPopup(widget, grab_kind, spring_loaded)
    Widget      widget;
    XtGrabKind  grab_kind;
    Boolean     spring_loaded;
#endif
{
    register ShellWidget shell_widget = (ShellWidget) widget;

    if (! XtIsShell(widget)) {
	XtAppErrorMsg(XtWidgetToApplicationContext(widget),
		"invalidClass","xtPopup",XtCXtToolkitError,
                "XtPopup requires a subclass of shellWidgetClass",
                  (String *)NULL, (Cardinal *)NULL);
    }

    if (! shell_widget->shell.popped_up) {
	XtGrabKind call_data = grab_kind;
	XtCallCallbacks(widget, XtNpopupCallback, (XtPointer)&call_data);
	shell_widget->shell.popped_up = TRUE;
	shell_widget->shell.grab_kind = grab_kind;
	shell_widget->shell.spring_loaded = spring_loaded;
	if (shell_widget->shell.create_popup_child_proc != NULL) {
	    (*(shell_widget->shell.create_popup_child_proc))(widget);
	}
	if (grab_kind == XtGrabExclusive) {
	    XtAddGrab(widget, TRUE, spring_loaded);
	} else if (grab_kind == XtGrabNonexclusive) {
	    XtAddGrab(widget, FALSE, spring_loaded);
	}
	XtRealizeWidget(widget);
	XMapRaised(XtDisplay(widget), XtWindow(widget));
    } else
	XRaiseWindow(XtDisplay(widget), XtWindow(widget));

} /* _XtPopup */

void XtPopup (widget, grab_kind)
    Widget  widget;
    XtGrabKind grab_kind;
{
    switch (grab_kind) {

      case XtGrabNone:
      case XtGrabExclusive:
      case XtGrabNonexclusive:
	break;

      default:
	XtAppWarningMsg(
		XtWidgetToApplicationContext(widget),
		"invalidGrabKind","xtPopup",XtCXtToolkitError,
		"grab kind argument has invalid value; XtGrabNone assumed",
		(String *)NULL, (Cardinal *)NULL);
	grab_kind = XtGrabNone;
    }
	
    _XtPopup(widget, grab_kind, FALSE);
} /* XtPopup */

void XtPopupSpringLoaded (widget)
    Widget widget;
{
    _XtPopup(widget, XtGrabExclusive, True);
}

void XtPopdown(widget)
    Widget  widget;
{
    /* Unmap a shell widget if it is mapped, and remove from grab list */

    register ShellWidget shell_widget = (ShellWidget) widget;

    if (! XtIsShell(widget)) {
	XtAppErrorMsg(XtWidgetToApplicationContext(widget),
		"invalidClass","xtPopdown",XtCXtToolkitError,
            "XtPopdown requires a subclass of shellWidgetClass",
              (String *)NULL, (Cardinal *)NULL);
    }

    if (shell_widget->shell.popped_up) {
	XtGrabKind grab_kind = shell_widget->shell.grab_kind;
	XtUnmapWidget(widget);
	XWithdrawWindow(XtDisplay(widget), XtWindow(widget),
			XScreenNumberOfScreen(XtScreen(widget)));
	if (grab_kind != XtGrabNone) {
	    XtRemoveGrab(widget);
	}
	shell_widget->shell.popped_up = FALSE;
	XtCallCallbacks(widget, XtNpopdownCallback, (XtPointer)&grab_kind);
    }
} /* XtPopdown */

/* ARGSUSED */
void XtCallbackPopdown(widget, closure, call_data)
    Widget  widget;
    XtPointer closure;
    XtPointer call_data;
{
    register XtPopdownID id = (XtPopdownID) closure;

    XtPopdown(id->shell_widget);
    if (id->enable_widget != NULL) {
	XtSetSensitive(id->enable_widget, TRUE);
    }
} /* XtCallbackPopdown */



