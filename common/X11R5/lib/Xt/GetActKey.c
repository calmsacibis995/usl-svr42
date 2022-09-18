/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xt:GetActKey.c	1.1"
/* $XConsortium: GetActKey.c,v 1.4 91/01/10 14:10:30 converse Exp $ */

/*LINTLIBRARY*/

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#include "IntrinsicI.h"

KeySym XtGetActionKeysym(event, modifiers_return)
    XEvent *event;
    Modifiers *modifiers_return;
{
    TMKeyContext tm_context= _XtGetPerDisplay(event->xany.display)->tm_context;
    Modifiers modifiers;
    KeySym keysym;

    if (event->xany.type != KeyPress && event->xany.type != KeyRelease)
	return NoSymbol;

    if (tm_context != NULL
	&& event == tm_context->event
	&& event->xany.serial == tm_context->serial ) {

	if (modifiers_return != NULL)
	    *modifiers_return = tm_context->modifiers;
	return tm_context->keysym;
    }

    XtTranslateKeycode( event->xany.display, (KeyCode)event->xkey.keycode,
		        event->xkey.state, &modifiers, &keysym );

    if (modifiers_return != NULL)
	*modifiers_return = event->xkey.state & modifiers;

    return keysym;
}
