/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:PopupCB.c	1.1"
/* $XConsortium: PopupCB.c,v 1.2 91/01/06 13:32:35 rws Exp $ */

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#include "IntrinsicI.h"

/* ARGSUSED */
void XtCallbackNone(widget, closure, call_data)
    Widget  widget;
    XtPointer closure;
    XtPointer call_data;
{
    XtSetSensitive(widget, FALSE);
    _XtPopup((Widget) closure, XtGrabNone, FALSE);
} /* XtCallbackNone */

/* ARGSUSED */
void XtCallbackNonexclusive(widget, closure, call_data)
    Widget  widget;
    XtPointer closure;
    XtPointer call_data;
{

    XtSetSensitive(widget, FALSE);
    _XtPopup((Widget) closure, XtGrabNonexclusive, FALSE);
} /* XtCallbackNonexclusive */

/* ARGSUSED */
void XtCallbackExclusive(widget, closure, call_data)
    Widget  widget;
    XtPointer closure;
    XtPointer call_data;
{
    XtSetSensitive(widget, FALSE);
    _XtPopup((Widget) closure, XtGrabExclusive, FALSE);
} /* XtCallbackExclusive */
