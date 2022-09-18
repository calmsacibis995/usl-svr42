/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:Reports.h	1.2"
/*
 * $XConsortium: Reports.h,v 1.3 90/02/28 18:46:46 jim Exp $
 *
 * Copyright 1990 Massachusetts Institute of Technology
 *
 *
 */

#ifndef _Xaw_Reports_h
#define _Xaw_Reports_h

/*
 * XawPannerReport - this structure is used by the reportCallback of the
 * Panner, Porthole, Viewport, and Scrollbar widgets to report its position.
 * All fields must be filled in, although the changed field may be used as
 * a hint as to which fields have been altered since the last report.
 */
typedef struct {
    unsigned int changed;		/* mask, see below */
    Position slider_x, slider_y;	/* location of slider within outer */
    Dimension slider_width, slider_height;  /* size of slider */
    Dimension canvas_width, canvas_height;  /* size of canvas */
} XawPannerReport;

#define XawPRSliderX		(1 << 0)
#define XawPRSliderY		(1 << 1)
#define XawPRSliderWidth	(1 << 2)
#define XawPRSliderHeight	(1 << 3)
#define XawPRCanvasWidth	(1 << 4)
#define XawPRCanvasHeight	(1 << 5)
#define XawPRAll		(63)	/* union of above */

#define XtNreportCallback "reportCallback"
#define XtCReportCallback "reportCallback"

#endif /* _Xaw_Reports_h */
