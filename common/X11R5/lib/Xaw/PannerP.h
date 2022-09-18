/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:PannerP.h	1.2"
/*
 * $XConsortium: PannerP.h,v 1.18 90/04/11 17:05:11 jim Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Jim Fulton, MIT X Consortium
 */

#ifndef _XawPannerP_h
#define _XawPannerP_h

#include <X11/Xaw/Panner.h>
#include <X11/Xaw/SimpleP.h>		/* parent */

typedef struct {			/* new fields in widget class */
    int dummy;
} PannerClassPart;

typedef struct _PannerClassRec {	/* Panner widget class */
    CoreClassPart core_class;
    SimpleClassPart simple_class;
    PannerClassPart panner_class;
} PannerClassRec;

typedef struct {			/* new fields in widget */
    /* resources... */
    XtCallbackList report_callbacks;	/* callback/Callback */
    Boolean allow_off;			/* allowOff/AllowOff */
    Boolean resize_to_pref;		/* resizeToPreferred/Boolean */
    Pixel foreground;			/* foreground/Foreground */
    Pixel shadow_color;			/* shadowColor/ShadowColor */
    Dimension shadow_thickness;		/* shadowThickness/ShadowThickness */
    Dimension default_scale;		/* defaultScale/DefaultScale */
    Dimension line_width;		/* lineWidth/LineWidth */
    Dimension canvas_width;		/* canvasWidth/CanvasWidth */
    Dimension canvas_height;		/* canvasHeight/CanvasHeight */
    Position slider_x;			/* sliderX/SliderX */
    Position slider_y;			/* sliderY/SliderY */
    Dimension slider_width;		/* sliderWidth/SliderWidth */
    Dimension slider_height;		/* sliderHeight/SliderHeight */
    Dimension internal_border;		/* internalBorderWidth/BorderWidth */
    String stipple_name;		/* backgroundStipple/BackgroundStipple */
    /* private data... */
    GC slider_gc;			/* background of slider */
    GC shadow_gc;			/* edge of slider and shadow */
    GC xor_gc;				/* for doing XOR tmp graphics */
    double haspect, vaspect;		/* aspect ratio of core to canvas */
    Boolean rubber_band;		/* true = rubber band, false = move */
    struct {
	Boolean doing;			/* tmp graphics in progress */
	Boolean showing;		/* true if tmp graphics displayed */
	Position startx, starty;	/* initial position of slider */
	Position dx, dy;		/* offset loc for tmp graphics */
	Position x, y;			/* location for tmp graphics */
    } tmp;
    Position knob_x, knob_y;		/* real upper left of knob in canvas */
    Dimension knob_width, knob_height;	/* real size of knob in canvas */
    Boolean shadow_valid;		/* true if rects are valid */
    XRectangle shadow_rects[2];		/* location of shadows */
    Position last_x, last_y;		/* previous location of knob */
} PannerPart;

typedef struct _PannerRec {
    CorePart core;
    SimplePart simple;
    PannerPart panner;
} PannerRec;

#define PANNER_HSCALE(pw,val) ((pw)->panner.haspect * ((double) (val)))
#define PANNER_VSCALE(pw,val) ((pw)->panner.vaspect * ((double) (val)))

#define PANNER_DSCALE(pw,val) (Dimension)  \
  ((((unsigned long) (val)) * (unsigned long) pw->panner.default_scale) / 100L)
#define PANNER_DEFAULT_SCALE 8		/* percent */

#define PANNER_OUTOFRANGE -30000

/*
 * external declarations
 */
extern PannerClassRec pannerClassRec;

#endif /* _XawPannerP_h */
