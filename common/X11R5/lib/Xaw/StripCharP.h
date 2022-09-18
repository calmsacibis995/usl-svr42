/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:StripCharP.h	1.2"
/*
* $XConsortium: StripCharP.h,v 1.4 90/10/22 14:38:15 converse Exp $
*/


/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

#ifndef _XawStripChartP_h
#define _XawStripChartP_h

#include <X11/Xaw/StripChart.h>
#include <X11/Xaw/SimpleP.h>

#define NO_GCS 0
#define FOREGROUND 1 << 0
#define HIGHLIGHT  1 << 1
#define ALL_GCS (FOREGROUND | HIGHLIGHT)

/* New fields for the stripChart widget instance record */

typedef struct {
    Pixel	fgpixel;	/* color index for graph */
    Pixel	hipixel;	/* color index for lines */
    GC	fgGC;		/* graphics context for fgpixel */
    GC	hiGC;		/* graphics context for hipixel */
    
    /* start of graph stuff */
    
    int	update;		/* update frequence */
    int	scale;		/* scale factor */
    int	min_scale;	/* smallest scale factor */
    int	interval;	/* data point interval */
    XPoint * points ;	/* Poly point for repairing graph lines. */
    double max_value;	/* Max Value in window */
    double valuedata[2048];/* record of data points */
    XtIntervalId interval_id;
    XtCallbackList get_value; /* proc to call to fetch load pt */
    int jump_val;		/* Amount to jump on each scroll. */
} StripChartPart;

/* Full instance record declaration */
typedef struct _StripChartRec {
   CorePart core;
   SimplePart simple;
   StripChartPart strip_chart;
} StripChartRec;

/* New fields for the StripChart widget class record */
typedef struct {int dummy;} StripChartClassPart;

/* Full class record declaration. */
typedef struct _StripChartClassRec {
   CoreClassPart core_class;
   SimpleClassPart simple_class;
   StripChartClassPart strip_chart_class;
} StripChartClassRec;

/* Class pointer. */
extern StripChartClassRec stripChartClassRec;

#endif /* _XawStripChartP_h */
