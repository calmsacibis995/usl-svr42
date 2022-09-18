/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:RepeaterP.h	1.2"
/*
 * $XConsortium: RepeaterP.h,v 1.3 90/03/02 15:47:00 jim Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Jim Fulton, MIT X Consortium
 */

#ifndef _XawRepeaterP_h
#define _XawRepeaterP_h

#include <X11/Xaw/CommandP.h>
#include <X11/Xaw/Repeater.h>

typedef struct {			/* new fields in widget class */
    int dummy;
} RepeaterClassPart;

typedef struct _RepeaterClassRec {	/* Repeater widget class */
    CoreClassPart core_class;
    SimpleClassPart simple_class;
    LabelClassPart label_class;
    CommandClassPart command_class;
    RepeaterClassPart repeater_class;
} RepeaterClassRec;

typedef struct {			/* new fields in widget */
    /* resources... */
    int initial_delay;			/* initialDelay/Delay */
    int repeat_delay;			/* repeatDelay/Delay */
    int minimum_delay;			/* minimumDelay/MinimumDelay */
    int decay;				/* decay to minimum delay */
    Boolean flash;			/* flash/Boolean */
    XtCallbackList start_callbacks;	/* startCallback/StartCallback */
    XtCallbackList stop_callbacks;	/* stopCallback/StopCallback */
    /* private data... */
    int next_delay;			/* next amount for timer */
    XtIntervalId timer;			/* timer that will fire */
} RepeaterPart;

typedef struct _RepeaterRec {
    CorePart core;
    SimplePart simple;
    LabelPart label;
    CommandPart command;
    RepeaterPart repeater;
} RepeaterRec;

#define REP_DEF_DECAY 5			/* milliseconds */
#define REP_DEF_INITIAL_DELAY 200	/* milliseconds */
#define REP_DEF_MINIMUM_DELAY 10	/* milliseconds */
#define REP_DEF_REPEAT_DELAY 50		/* milliseconds */

/*
 * external declarations
 */
extern RepeaterClassRec repeaterClassRec;

#endif /* _XawRepeaterP_h */
