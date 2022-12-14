/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:evtomask.c	1.2"
/* $XConsortium: evtomask.c,v 1.8 91/02/20 18:49:00 rws Exp $ */
/* Copyright    Massachusetts Institute of Technology    1987	*/

/*
*/

#include <X11/X.h>

#if defined(__STDC__)
#define Const const
#else
#define Const /**/
#endif

/*
 * This array can be used given an event type to determine the mask bits
 * that could have generated it.
 */
long Const _Xevent_to_mask [LASTEvent] = {
	0,						/* no event 0 */
	0,						/* no event 1 */
	KeyPressMask,					/* KeyPress */
	KeyReleaseMask,					/* KeyRelease */
	ButtonPressMask,				/* ButtonPress */
	ButtonReleaseMask,				/* ButtonRelease */
	PointerMotionMask|PointerMotionHintMask|Button1MotionMask|
		Button2MotionMask|Button3MotionMask|Button4MotionMask|
		Button5MotionMask|ButtonMotionMask,	/* MotionNotify */
	EnterWindowMask,				/* EnterNotify */
	LeaveWindowMask,				/* LeaveNotify */
	FocusChangeMask,				/* FocusIn */
	FocusChangeMask,				/* FocusOut */
	KeymapStateMask,				/* KeymapNotify */
	ExposureMask,					/* Expose */
	ExposureMask,					/* GraphicsExpose */
	ExposureMask,					/* NoExpose */
	VisibilityChangeMask,				/* VisibilityNotify */
	SubstructureNotifyMask,				/* CreateNotify */
	StructureNotifyMask|SubstructureNotifyMask,	/* DestroyNotify */
	StructureNotifyMask|SubstructureNotifyMask,	/* UnmapNotify */
	StructureNotifyMask|SubstructureNotifyMask,	/* MapNotify */
	SubstructureRedirectMask,			/* MapRequest */
	SubstructureNotifyMask|StructureNotifyMask,	/* ReparentNotify */
	StructureNotifyMask|SubstructureNotifyMask,	/* ConfigureNotify */
	SubstructureRedirectMask,			/* ConfigureRequest */
	SubstructureNotifyMask|StructureNotifyMask,	/* GravityNotify */
	ResizeRedirectMask,				/* ResizeRequest */
	SubstructureNotifyMask|StructureNotifyMask,	/* CirculateNotify */
	SubstructureRedirectMask,			/* CirculateRequest */
	PropertyChangeMask,				/* PropertyNotify */
	0,						/* SelectionClear */
	0,						/* SelectionRequest */
	0,						/* SelectionNotify */
	ColormapChangeMask,				/* ColormapNotify */
	0,						/* ClientMessage */
	0,						/* MappingNotify */
};
