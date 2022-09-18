/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:ClockP.h	1.2"
/*
* $XConsortium: ClockP.h,v 1.21 90/10/22 14:43:22 converse Exp $
*/


/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#ifndef _XawClockP_h
#define _XawClockP_h

#include <X11/Xos.h>		/* Needed for struct tm. */
#include <X11/Xaw/Clock.h>
#include <X11/Xaw/SimpleP.h>

#define SEG_BUFF_SIZE		128
#define ASCII_TIME_BUFLEN	32	/* big enough for 26 plus slop */

/* New fields for the clock widget instance record */
typedef struct {
	 Pixel	fgpixel;	/* color index for text */
	 Pixel	Hipixel;	/* color index for Highlighting */
	 Pixel	Hdpixel;	/* color index for hands */
	 XFontStruct	*font;	/* font for text */
	 GC	myGC;		/* pointer to GraphicsContext */
	 GC	EraseGC;	/* eraser GC */
	 GC	HandGC;		/* Hand GC */
	 GC	HighGC;		/* Highlighting GC */
/* start of graph stuff */
	 int	update;		/* update frequence */
	 Dimension radius;		/* radius factor */
	 int	backing_store;	/* backing store type */
	 Boolean chime;
	 Boolean beeped;
	 Boolean analog;
	 Boolean show_second_hand;
	 Dimension second_hand_length;
	 Dimension minute_hand_length;
	 Dimension hour_hand_length;
	 Dimension hand_width;
	 Dimension second_hand_width;
	 Position centerX;
	 Position centerY;
	 int	numseg;
	 int	padding;
	 XPoint	segbuff[SEG_BUFF_SIZE];
	 XPoint	*segbuffptr;
	 XPoint	*hour, *sec;
	 struct tm  otm ;
	 XtIntervalId interval_id;
	 char prev_time_string[ASCII_TIME_BUFLEN];
   } ClockPart;

/* Full instance record declaration */
typedef struct _ClockRec {
   CorePart core;
   SimplePart simple;
   ClockPart clock;
   } ClockRec;

/* New fields for the Clock widget class record */
typedef struct {int dummy;} ClockClassPart;

/* Full class record declaration. */
typedef struct _ClockClassRec {
   CoreClassPart core_class;
   SimpleClassPart simple_class;
   ClockClassPart clock_class;
   } ClockClassRec;

/* Class pointer. */
extern ClockClassRec clockClassRec;

#endif /* _XawClockP_h */
