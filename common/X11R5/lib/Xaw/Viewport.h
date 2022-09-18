/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xaw:Viewport.h	1.2"
/* $XConsortium: Viewport.h,v 1.21 91/07/22 19:05:23 converse Exp $ */

/************************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



********************************************************/

#ifndef _XawViewport_h
#define _XawViewport_h

#include <X11/Xaw/Form.h>
#include <X11/Xaw/Reports.h>
#include <X11/Xfuncproto.h>

/* Resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 allowHoriz	     Boolean		Boolean		False
 allowVert	     Boolean		Boolean		False
 background	     Background		Pixel		XtDefaultBackground
 border		     BorderColor	Pixel		XtDefaultForeground
 borderWidth	     BorderWidth	Dimension	1
 destroyCallback     Callback		Pointer		NULL
 foreceBars	     Boolean		Boolean		False
 height		     Height		Dimension	0
 mappedWhenManaged   MappedWhenManaged	Boolean		True
 reportCallback	     ReportCallback	Pointer		NULL
 sensitive	     Sensitive		Boolean		True
 useBottom	     Boolean		Boolean		False
 useRight	     Boolean		Boolean		False
 width		     Width		Dimension	0
 x		     Position		Position	0
 y		     Position		Position	0

*/

/* fields added to Form */
#ifndef _XtStringDefs_h_
#define XtNforceBars "forceBars"
#define XtNallowHoriz "allowHoriz"
#define XtNallowVert "allowVert"
#define XtNuseBottom "useBottom"
#define XtNuseRight "useRight"
#endif

extern WidgetClass viewportWidgetClass;

typedef struct _ViewportClassRec *ViewportWidgetClass;
typedef struct _ViewportRec  *ViewportWidget;

_XFUNCPROTOBEGIN

extern void XawViewportSetLocation (
#if NeedFunctionPrototypes
    Widget		/* gw */,
#if NeedWidePrototypes
    /* float */ double	/* xoff */,
    /* float */ double	/* yoff */
#else
    float		/* xoff */,
    float		/* yoff */
#endif
#endif
);

extern void XawViewportSetCoordinates (
#if NeedFunctionPrototypes
    Widget		/* gw */,
#if NeedWidePrototypes
    /* Position */ int	/* x */,
    /* Position */ int	/* y */
#else
    Position		/* x */,
    Position		/* y */
#endif
#endif
);

_XFUNCPROTOEND

#endif /* _XawViewport_h */
