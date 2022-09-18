/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:Clock.h	1.2"
/*
* $XConsortium: Clock.h,v 1.29 90/10/22 14:43:12 converse Exp $
*/


/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#ifndef _XawClock_h
#define _XawClock_h

/***********************************************************************
 *
 * Clock Widget
 *
 ***********************************************************************/

#include <X11/Xmu/Converters.h>

/* Parameters:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 analog		     Boolean		Boolean		True
 background	     Background		Pixel		white
 backingStore	     BackingStore	BackingStore	default
 border		     BorderColor	Pixel		Black
 borderWidth	     BorderWidth	Dimension	1
 chime		     Boolean		Boolean		False
 destroyCallback     Callback		Pointer		NULL
 font		     Font		XFontStruct*	fixed
 foreground	     Foreground		Pixel		black
 hand		     Foreground		Pixel		black
 height		     Height		Dimension	164
 highlight	     Foreground		Pixel		black
 mappedWhenManaged   MappedWhenManaged	Boolean		True
 padding	     Margin		int		8
 update		     Interval		int		60 (seconds)
 width		     Width		Dimension	164
 x		     Position		Position	0
 y		     Position		Position	0

*/

/* Resource names used to the clock widget */

		/* color of hands */
#define XtNhand "hands"


		/* Boolean: digital if FALSE */
#define XtNanalog "analog"

		/* Boolean:  */
#define XtNchime "chime"

		/* Int: amount of space around outside of clock */
#define XtNpadding "padding"

typedef struct _ClockRec *ClockWidget;  /* completely defined in ClockPrivate.h */
typedef struct _ClockClassRec *ClockWidgetClass;    /* completely defined in ClockPrivate.h */

extern WidgetClass clockWidgetClass;

#endif /* _XawClock_h */
/* DON'T ADD STUFF AFTER THIS #endif */
