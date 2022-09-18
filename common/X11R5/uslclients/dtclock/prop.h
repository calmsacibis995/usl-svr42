/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtclock:prop.h	1.2"
#endif

/*
 * prop.h
 *
 */

#ifndef _prop_h
#define _prop_h

#define CHIME  "chime"
#define MODES  "mode"
#define TICKS  "tick"

typedef enum { ChimeNone, ChimeTraditional, ChimeShipBells } ChimeMenuIndex;
typedef enum { ModesAnalog, ModesDigital } ModesMenuIndex;
typedef enum { TicksSecond, TicksMinute } TicksMenuIndex;

extern void PropertyCB(Widget, XtPointer, XtPointer);

extern PopupGizmo PropertiesPrompt;

#endif /* _prop_h */
